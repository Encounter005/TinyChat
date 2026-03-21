#ifndef AVATARCACHE_H
#define AVATARCACHE_H

#include "qobject.h"
#include "qpixmap.h"
#include "qset.h"
#include "singleton.h"
#include <QMutex>
#include <QObject>

class AvatarCache : public QObject, public SingleTon<AvatarCache> {
    friend class SingleTon<AvatarCache>;
    Q_OBJECT
public:
    QString Cachepath(int uid, const QString& iconName);
    QPixmap PixmapOrPlaceholder(int uid, const QString& iconName);
    void    EnsureDownloaded(int uid, const QString& iconName);
signals:
    void avatarReady(int uid, QString iconName);
    void avatarUpdated(int uid, QString iconName);
    void avatarDownloadFailed(int uid, QString iconName, QString reason);

private:
    AvatarCache() = default;
    bool DownloadBlocking(
        const QString& iconName, const QString& savePath, QString& err);
    QString DownloadKey(int uid, const QString& iconName) const;

private:
    mutable QMutex _mtx;
    mutable QMutex _grpc_mtx;
    QSet<QString>  _downloading;
};

#endif   // AVATARCACHE_H
