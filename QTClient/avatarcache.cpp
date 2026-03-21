#include "avatarcache.h"
#include "botuser.h"
#include "datapaths.h"
#include "file.grpc.pb.h"
#include "file.pb.h"
#include "qcoreapplication.h"
#include "qdir.h"
#include "qfile.h"
#include "qfileinfo.h"
#include "qmutex.h"
#include "qobject.h"
#include "qpixmap.h"
#include "qregion.h"
#include "qsettings.h"
#include "qthread.h"
#include <grpc++/grpc++.h>
#include <grpcpp/channel.h>
#include <grpcpp/client_context.h>
#include <grpcpp/create_channel.h>
#include <grpcpp/security/credentials.h>

QString AvatarCache::Cachepath(int uid, const QString& iconName) {
    return DataPaths::AvatarPath(uid, iconName);
}

QPixmap AvatarCache::PixmapOrPlaceholder(int uid, const QString& iconName) {
    if (IsBotUid(uid)) {
        QPixmap pix(BotAvatarPath());
        if (!pix.isNull()) return pix;
    }
    QString p = Cachepath(uid, iconName);
    if (QFile::exists(p)) {
        QPixmap pix(p);
        if (!pix.isNull()) return pix;
    }

    EnsureDownloaded(uid, iconName);
    return QPixmap(":/img/head_1.jpg");   // 默认头像
}

void AvatarCache::EnsureDownloaded(int uid, const QString& iconName) {

    if (IsBotUid(uid)) return;
    if (iconName.isEmpty()) return;
    const QString p = Cachepath(uid, iconName);
    if (QFile::exists(p)) return;

    const QString key = DownloadKey(uid, iconName);
    {
        QMutexLocker lock(&_mtx);
        if (_downloading.contains(key)) return;
        _downloading.insert(key);
    }

    auto* t = QThread::create([this, uid, iconName, p, key]() {
        QString err;
        bool    ok = DownloadBlocking(iconName, p, err);
        {
            QMutexLocker lock(&_mtx);
            _downloading.remove(key);
        }

        if (ok)
            emit avatarReady(uid, iconName);
        else {
            QFile::remove(p);
            emit avatarDownloadFailed(uid, iconName, err);
        }
    });

    connect(t, &QThread::finished, t, &QObject::deleteLater);
    t->start();
}

bool AvatarCache::DownloadBlocking(
    const QString& iconName, const QString& savePath, QString& err) {
    QMutexLocker grpc_lock(&_grpc_mtx);

    QString appPath = QCoreApplication::applicationDirPath();
    QString cfgPath
        = QDir::toNativeSeparators(appPath + QDir::separator() + "config.ini");
    QSettings settings(cfgPath, QSettings::IniFormat);
    QString   host = settings.value("FileServer/host").toString();
    QString   port = settings.value("FileServer/port").toString();

    if (host.isEmpty() || port.isEmpty()) {
        err = "invalid fileserver host/port";
        return false;
    }

    auto channel = grpc::CreateChannel(
        (host + ":" + port).toStdString(), grpc::InsecureChannelCredentials());
    auto stub = FileService::FileTransport::NewStub(channel);
    if (!stub) {
        err = "create grpc stub failed";
        return false;
    }

    grpc::ClientContext           ctx;
    FileService::DownloadRequest  req;
    FileService::DownloadResponse resp;
    req.set_file_name(iconName.toStdString());
    req.set_start_offset(0);
    auto reader = stub->DownloadFile(&ctx, req);

    QDir().mkpath(QFileInfo(savePath).absolutePath());
    QFile out(savePath);
    if (!out.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        err = "open local file failed";
        return false;
    }

    bool gotData = false;
    while (reader->Read(&resp)) {
        if (resp.has_chunk()) {
            const std::string chunk = resp.chunk();
            if (!chunk.empty()) {
                if (out.write(chunk.data(), static_cast<qint64>(chunk.size()))
                    != static_cast<qint64>(chunk.size())) {
                    out.close();
                    err = "write local file failed";
                    return false;
                }
                gotData = true;
            }
        }
    }
    out.close();
    grpc::Status st = reader->Finish();
    if (!st.ok()) {
        err = QString::fromStdString(st.error_message());
        return false;
    }

    if (!gotData) {
        err = "empty stream";
        return false;
    }

    return true;
}

QString AvatarCache::DownloadKey(int uid, const QString& iconName) const {
    return QString::number(uid) + ":" + iconName;
}
