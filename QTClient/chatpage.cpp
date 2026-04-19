#include "chatpage.h"
#include "animationtiming.h"
#include "avatarcache.h"
#include "botplatformsettings.h"
#include "botuser.h"
#include "chat_time.h"
#include "chatitembase.h"
#include "clickedlabel.h"
#include "datapaths.h"
#include "docpayload.h"
#include "file.grpc.pb.h"
#include "filebubble.h"
#include "onlyofficeurl.h"
#include "onlyofficeviewerdialog.h"
#include "picturebubble.h"
#include "qjsonarray.h"
#include "qjsondocument.h"
#include "tcpmanager.h"
#include "textbubble.h"
#include "ui_chatpage.h"
#include "usermanager.h"

#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDir>
#include <QEasingCurve>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <QPainter>
#include <QParallelAnimationGroup>
#include <QPointer>
#include <QPropertyAnimation>
#include <QPushButton>
#include <QSettings>
#include <QStyleOption>
#include <QThread>
#include <QUrlQuery>
#include <QUuid>
#include <grpcpp/client_context.h>
#include <grpcpp/create_channel.h>
#include <grpcpp/security/credentials.h>

namespace {
const QString kImagePayloadPrefix = "[img]";

QString BuildMsgId(qint64 ts) {
    return QString("msg_%1_%2")
        .arg(ts)
        .arg(QUuid::createUuid().toString(QUuid::WithoutBraces));
}

QString BuildRemoteImageName(int uid, qint64 ts, const QString &local_path) {
    const QString ext      = QFileInfo(local_path).suffix().toLower();
    const QString safe_ext = ext.isEmpty() ? QStringLiteral("png") : ext;
    const QString random   = QUuid::createUuid().toString(QUuid::WithoutBraces);
    return QString("img_%1_%2_%3.%4")
        .arg(uid)
        .arg(ts)
        .arg(random)
        .arg(safe_ext);
}

QString BuildRemoteFileName(int uid, qint64 ts, const QString &local_path) {
    const QString ext      = QFileInfo(local_path).suffix().toLower();
    const QString safe_ext = ext.isEmpty() ? QStringLiteral("bin") : ext;
    const QString random   = QUuid::createUuid().toString(QUuid::WithoutBraces);
    return QString("file_%1_%2_%3.%4")
        .arg(uid)
        .arg(ts)
        .arg(random)
        .arg(safe_ext);
}

QUrl BuildOnlyOfficeUrl(const QString &remote_name) {
    QString app_path = QCoreApplication::applicationDirPath();
    QString config_path
        = QDir::toNativeSeparators(app_path + QDir::separator() + "config.ini");
    QSettings     settings(config_path, QSettings::IniFormat);
    const QString via_gate
        = settings.value("OnlyOffice/open_via_gate", "true").toString();
    const bool use_gate = via_gate.compare("false", Qt::CaseInsensitive) != 0
                          && via_gate != "0";

    QString host;
    int     port = 0;
    QString path;
    if (use_gate) {
        host = settings.value("GateServer/host").toString();
        port = settings.value("GateServer/port").toInt();
        path = QStringLiteral("/onlyoffice/editor");
    } else {
        host = settings.value("OnlyOffice/host").toString();
        port = settings.value("OnlyOffice/port").toInt();
        path = settings.value("OnlyOffice/path", "/").toString();
    }

    return BuildOnlyOfficeUrlFromParts(host, port, path, remote_name);
}

QString FormatChatTime(qint64 timestamp_secs) {
    return FormatChatTimeText(timestamp_secs);
}

void PlayBubbleEnterAnimation(QWidget *item, QWidget *bubble) {
    if (item == nullptr || bubble == nullptr) {
        return;
    }

    const int target_height = qMax(item->sizeHint().height(), item->height());
    const int target_width  = qMax(bubble->sizeHint().width(), bubble->width());
    item->setMaximumHeight(0);
    bubble->setMaximumWidth(0);

    auto *group = new QParallelAnimationGroup(item);

    auto *height_anim = new QPropertyAnimation(item, "maximumHeight", group);
    height_anim->setDuration(UiAnim::kBubbleHeightMs);
    height_anim->setStartValue(0);
    height_anim->setEndValue(target_height);
    height_anim->setEasingCurve(QEasingCurve::OutCubic);
    group->addAnimation(height_anim);

    auto *width_anim = new QPropertyAnimation(bubble, "maximumWidth", group);
    width_anim->setDuration(UiAnim::kBubbleWidthMs);
    width_anim->setStartValue(0);
    width_anim->setEndValue(target_width);
    width_anim->setEasingCurve(QEasingCurve::OutBack);
    group->addAnimation(width_anim);

    QPointer<QWidget> guarded_item(item);
    QPointer<QWidget> guarded_bubble(bubble);
    QObject::connect(
        group,
        &QParallelAnimationGroup::finished,
        item,
        [guarded_item, guarded_bubble]() {
            if (guarded_item) {
                guarded_item->setMaximumHeight(QWIDGETSIZE_MAX);
            }
            if (guarded_bubble) {
                guarded_bubble->setMaximumWidth(QWIDGETSIZE_MAX);
            }
        });
    group->start(QAbstractAnimation::DeleteWhenStopped);
}

void OpenOnlyOfficeViewer(QWidget *parent, const QUrl &url) {
    auto *viewer = new OnlyOfficeViewerDialog(parent);
    viewer->setAttribute(Qt::WA_DeleteOnClose);
    viewer->loadUrl(url);
    viewer->show();
    viewer->raise();
    viewer->activateWindow();
}
}   // namespace

ChatPage::ChatPage(QWidget *parent) : QWidget(parent), ui(new Ui::ChatPage) {
    ui->setupUi(this);
    ui->recv_btn->SetState("normal", "hover", "press");
    ui->send_btn->SetState("normal", "hover", "press");

    ui->emo_label->SetState(
        "normal", "hover", "press", "normal", "hover", "press");
    ui->file_label->SetState(
        "normal", "hover", "press", "normal", "hover", "press");

    connect(
        ui->file_label,
        &ClickedLabel::clicked,
        this,
        &ChatPage::slot_switch_to_upload);
    connect(
        ui->emo_label,
        &ClickedLabel::clicked,
        this,
        &ChatPage::slot_pick_image);

    ui->chatEdit->installEventFilter(this);
}

ChatPage::~ChatPage() {
    delete ui;
}

void ChatPage::paintEvent(QPaintEvent *event) {
    QStyleOption opt;
    opt.init(this);
    QPainter p(this);
    style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
}

bool ChatPage::eventFilter(QObject *watched, QEvent *event) {
    if (watched == ui->chatEdit && event != nullptr) {
        if (event->type() == QEvent::FocusIn) {
            AnimateInputAreaHeight(240);
        } else if (event->type() == QEvent::FocusOut) {
            AnimateInputAreaHeight(200);
        }
    }
    return QWidget::eventFilter(watched, event);
}

void ChatPage::AnimateInputAreaHeight(int target_height) {
    target_height = qBound(180, target_height, 280);
    auto *anim
        = new QPropertyAnimation(ui->chatEdit, "maximumHeight", ui->chatEdit);
    anim->setDuration(UiAnim::kInputFocusMs);
    anim->setStartValue(ui->chatEdit->maximumHeight());
    anim->setEndValue(target_height);
    anim->setEasingCurve(QEasingCurve::OutCubic);
    anim->start(QAbstractAnimation::DeleteWhenStopped);
}

void ChatPage::SetUserInfo(std::shared_ptr<UserInfo> user_info) {
    if (!user_info) {
        _user_info = std::make_shared<UserInfo>(
            0, QStringLiteral("Unknown"), QString());
    } else {
        _user_info = user_info;
    }

    // AI Bot 名称兜底，避免在部分链路被错误覆盖
    if (IsBotUid(_user_info->_uid)) {
        _user_info->_name = BOT_NAME;
    }

    // 设置 ui 界面
    if (IsBotUid(_user_info->_uid)) {
        ui->title_label->setText(QStringLiteral("%1 · %2").arg(
            BOT_NAME,
            BotPlatformSettings::DisplayNameForPlatform(
                BotPlatformSettings::LoadPlatformForBot())));
    } else {
        ui->title_label->setText(_user_info->_name);
    }
    ui->chat_data_list->removeAllItem();
    for (auto &msg : _user_info->_chat_msgs) {
        AppendChatMsg(msg);
    }
}

void ChatPage::on_recv_btn_clicked() {
    // 文件接收改为点击文件气泡触发，这里保留空实现
}

void ChatPage::on_send_btn_clicked() {
    if (_user_info == nullptr) {
        qDebug() << "friend_info is empty";
        return;
    }
    auto     user_info              = UserManager::getInstance()->GetUserInfo();
    auto     pTextEdit              = ui->chatEdit;
    ChatRole role                   = ChatRole::SELF;
    QString  userName               = user_info->_name;
    const QVector<MsgInfo> &msgList = pTextEdit->getMsgList();
    QJsonObject             textObj;
    QJsonArray              textArray;
    int                     txt_size = 0;

    auto flush_text_batch = [&]() {
        if (textArray.isEmpty()) {
            return;
        }
        textObj["text_array"] = textArray;
        textObj["fromuid"]    = user_info->_uid;
        textObj["touid"]      = _user_info->_uid;
        if (IsBotUid(_user_info->_uid)) {
            textObj["bot_platform"] = BotPlatformSettings::LoadPlatformForBot();
        }
        QJsonDocument doc(textObj);
        QByteArray    jsonData = doc.toJson(QJsonDocument::Compact);
        emit          TcpManager::getInstance()
            -> sig_send_data(ReqId::ID_TEXT_CHAT_MSG_REQ, jsonData);
        txt_size  = 0;
        textArray = QJsonArray();
        textObj   = QJsonObject();
    };

    auto append_text_to_batch
        = [&](const QString &content, const QString &msg_id, qint64 ts) {
              QJsonObject obj;
              obj["content"]   = content;
              obj["msgid"]     = msg_id;
              obj["timestamp"] = ts;
              textArray.append(obj);

              auto txt_msg = std::make_shared<TextChatData>(
                  msg_id, content, user_info->_uid, _user_info->_uid, ts);
              emit sig_append_send_chat_msg(txt_msg);
          };

    auto cache_local_image
        = [](const QString &src, const QString &remote_name) {
              const QString target_path
                  = DataPaths::ImagesDir() + QDir::separator() + remote_name;
              if (QFile::exists(target_path)) {
                  return;
              }
              QDir().mkpath(QFileInfo(target_path).absolutePath());
              QFile::copy(src, target_path);
          };
    for (int i = 0; i < msgList.size(); ++i) {
        // 消息内容长度不合规就跳过
        if (msgList[i].content.length() <= 0) {
            pTextEdit->clear();
            ui->chatEdit->setFocus();
            qDebug() << "Cannot send empty message";
            return;
        }
        if (msgList[i].content.length() > 1024) {
            continue;
        }
        QString       type      = msgList[i].msgFlag;
        ChatItemBase *pChatItem = new ChatItemBase(role);
        pChatItem->setUserName(userName);
        pChatItem->setUserIcon(
            AvatarCache::getInstance()->PixmapOrPlaceholder(
                user_info->_uid, user_info->_icon));
        QWidget *pBubble = nullptr;
        if (type == "text") {
            qint64  msg_ts     = QDateTime::currentSecsSinceEpoch();
            QString uuidString = BuildMsgId(msg_ts);
            pBubble            = new TextBubble(role, msgList[i].content);
            auto bubble_frame  = qobject_cast<BubbleFrame *>(pBubble);
            if (bubble_frame) {
                bubble_frame->setTimeText(FormatChatTime(msg_ts));
            }
            if (txt_size + msgList[i].content.length() > 1024) {
                flush_text_batch();
            }
            txt_size += msgList[i].content.length();
            QByteArray utf8Message = msgList[i].content.toUtf8();
            append_text_to_batch(
                QString::fromUtf8(utf8Message), uuidString, msg_ts);
        } else if (type == "image") {
            flush_text_batch();
            const qint64  msg_ts     = QDateTime::currentSecsSinceEpoch();
            const QString msg_id     = BuildMsgId(msg_ts);
            const QString local_path = msgList[i].content;
            pBubble = new PictureBubble(QPixmap(msgList[i].content), role);
            auto picture_bubble = qobject_cast<PictureBubble *>(pBubble);
            auto bubble_frame   = qobject_cast<BubbleFrame *>(picture_bubble);
            if (bubble_frame) {
                bubble_frame->setTimeText(FormatChatTime(msg_ts));
            }
            if (picture_bubble) {
                picture_bubble->setUploadProgress(0);
            }
            const QString remote_name
                = BuildRemoteImageName(user_info->_uid, msg_ts, local_path);
            _image_msgid_to_remote[msg_id] = remote_name;
            if (picture_bubble) {
                UploadImageAsync(
                    local_path,
                    remote_name,
                    [picture_bubble](int progress) {
                        picture_bubble->setUploadProgress(progress);
                    },
                    [=]() {
                        cache_local_image(local_path, remote_name);
                        if (picture_bubble) {
                            picture_bubble->setUploadDone();
                        }
                        const QString payload = BuildImagePayload(remote_name);
                        QJsonObject   single_text_obj;
                        QJsonArray    single_text_array;
                        QJsonObject   msg_obj;
                        msg_obj["content"]   = payload;
                        msg_obj["msgid"]     = msg_id;
                        msg_obj["timestamp"] = msg_ts;
                        single_text_array.append(msg_obj);
                        single_text_obj["text_array"] = single_text_array;
                        single_text_obj["fromuid"]    = user_info->_uid;
                        single_text_obj["touid"]      = _user_info->_uid;
                        if (IsBotUid(_user_info->_uid)) {
                            single_text_obj["bot_platform"]
                                = BotPlatformSettings::LoadPlatformForBot();
                        }
                        QJsonDocument doc(single_text_obj);
                        QByteArray    jsonData
                            = doc.toJson(QJsonDocument::Compact);
                        emit TcpManager::getInstance() -> sig_send_data(
                            ReqId::ID_TEXT_CHAT_MSG_REQ, jsonData);
                        auto image_msg = std::make_shared<TextChatData>(
                            msg_id,
                            payload,
                            user_info->_uid,
                            _user_info->_uid,
                            msg_ts);
                        emit sig_append_send_chat_msg(image_msg);
                    },
                    [picture_bubble](const QString &) {
                        if (picture_bubble) {
                            picture_bubble->setUploadFailed();
                        }
                    });
            }
        } else if (type == "file") {
            flush_text_batch();
            const qint64    msg_ts     = QDateTime::currentSecsSinceEpoch();
            const QString   msg_id     = BuildMsgId(msg_ts);
            const QString   local_path = msgList[i].content;
            const QFileInfo fi(local_path);
            const QString   display_name = fi.fileName();
            const qint64    file_size    = fi.size();
            const QString   remote_name
                = BuildRemoteFileName(user_info->_uid, msg_ts, local_path);
            const bool is_doc = IsOfficeDocument(local_path);

            auto *file_bubble = new FileBubble(display_name, role);
            file_bubble->setSelectable(false);
            file_bubble->setTransferProgress(0);
            if (is_doc) {
                file_bubble->setIconPath(":/img/document.png");
            }
            pBubble           = file_bubble;
            auto bubble_frame = qobject_cast<BubbleFrame *>(file_bubble);
            if (bubble_frame) {
                bubble_frame->setTimeText(FormatChatTime(msg_ts));
            }

            UploadFileAsync(
                local_path,
                remote_name,
                [file_bubble](int progress) {
                    if (file_bubble) {
                        file_bubble->setTransferProgress(progress);
                    }
                },
                [=]() {
                    const QString target_path = DataPaths::FilesDir()
                                                + QDir::separator()
                                                + remote_name;
                    if (!QFile::exists(target_path)) {
                        QDir().mkpath(QFileInfo(target_path).absolutePath());
                        QFile::copy(local_path, target_path);
                    }

                    const QString payload = BuildFilePayload(
                        remote_name, display_name, file_size, is_doc);
                    QJsonObject one_obj;
                    QJsonArray  one_arr;
                    QJsonObject m;
                    m["content"]   = payload;
                    m["msgid"]     = msg_id;
                    m["timestamp"] = msg_ts;
                    one_arr.append(m);
                    one_obj["text_array"] = one_arr;
                    one_obj["fromuid"]    = user_info->_uid;
                    one_obj["touid"]      = _user_info->_uid;
                    if (IsBotUid(_user_info->_uid)) {
                        one_obj["bot_platform"]
                            = BotPlatformSettings::LoadPlatformForBot();
                    }
                    QJsonDocument doc(one_obj);
                    QByteArray    jsonData = doc.toJson(QJsonDocument::Compact);
                    emit          TcpManager::getInstance()
                        -> sig_send_data(ReqId::ID_TEXT_CHAT_MSG_REQ, jsonData);

                    auto file_msg = std::make_shared<TextChatData>(
                        msg_id,
                        payload,
                        user_info->_uid,
                        _user_info->_uid,
                        msg_ts);
                    if (file_bubble) {
                        file_bubble->setTransferDone();
                    }
                    emit sig_append_send_chat_msg(file_msg);
                },
                [file_bubble](const QString &) {
                    if (file_bubble) {
                        file_bubble->setTransferFailed();
                    }
                });
        }
        // 发送消息
        if (pBubble != nullptr) {
            pChatItem->setWidget(pBubble);
            ui->chat_data_list->appendChatItem(pChatItem);
            PlayBubbleEnterAnimation(pChatItem, pBubble);
        }
    }
    flush_text_batch();
}

void ChatPage::slot_switch_to_upload() {
    const QStringList files = QFileDialog::getOpenFileNames(
        this,
        tr("选择文件"),
        QString(),
        tr("All Files (*.*);;Documents (*.doc *.docx *.xls *.xlsx *.ppt "
           "*.pptx)"));
    if (files.isEmpty()) {
        return;
    }
    ui->chatEdit->insertFileFromUrl(files);
    ui->chatEdit->setFocus();
}

void ChatPage::slot_pick_image() {
    const QString file_name = QFileDialog::getOpenFileName(
        this,
        tr("选择图片"),
        QString(),
        tr("Images (*.png *.jpg *.jpeg *.bmp *.webp *.gif)"));
    if (file_name.isEmpty()) {
        return;
    }
    ui->chatEdit->insertFileFromUrl(QStringList() << file_name);
    ui->chatEdit->setFocus();
}

QString ChatPage::BuildImagePayload(const QString &remote_name) {
    return kImagePayloadPrefix + remote_name;
}

bool ChatPage::IsImagePayload(const QString &content, QString *remote_name) {
    if (!content.startsWith(kImagePayloadPrefix)) {
        return false;
    }
    if (remote_name) {
        *remote_name = content.mid(kImagePayloadPrefix.length());
    }
    return true;
}


void ChatPage::UploadFileAsync(
    const QString &local_path, const QString &remote_name,
    const std::function<void(int)>             &on_progress,
    const std::function<void()>                &on_success,
    const std::function<void(const QString &)> &on_error) {
    auto *t = QThread::create([=]() {
        QString app_path    = QCoreApplication::applicationDirPath();
        QString config_path = QDir::toNativeSeparators(
            app_path + QDir::separator() + "config.ini");
        QSettings settings(config_path, QSettings::IniFormat);
        QString   host = settings.value("FileServer/host").toString();
        QString   port = settings.value("FileServer/port").toString();
        if (host.isEmpty() || port.isEmpty()) {
            QMetaObject::invokeMethod(
                this,
                [=]() {
                    on_error(QStringLiteral("invalid fileserver host/port"));
                },
                Qt::QueuedConnection);
            return;
        }

        QFile file(local_path);
        if (!file.open(QIODevice::ReadOnly)) {
            QMetaObject::invokeMethod(
                this,
                [=]() { on_error(QStringLiteral("open local file failed")); },
                Qt::QueuedConnection);
            return;
        }

        QFileInfo file_info(local_path);
        qint64    file_size = file_info.size();
        if (file_size <= 0) {
            QMetaObject::invokeMethod(
                this,
                [=]() { on_error(QStringLiteral("file is empty")); },
                Qt::QueuedConnection);
            return;
        }

        const QByteArray all_data = file.readAll();
        const QString    md5      = QString(
            QCryptographicHash::hash(all_data, QCryptographicHash::Md5)
                .toHex());
        file.seek(0);

        auto channel = grpc::CreateChannel(
            (host + ":" + port).toStdString(),
            grpc::InsecureChannelCredentials());
        auto stub = FileService::FileTransport::NewStub(channel);

        grpc::ClientContext         ctx;
        FileService::UploadResponse response;
        auto                        writer = stub->UploadFile(&ctx, &response);
        if (!writer) {
            QMetaObject::invokeMethod(
                this,
                [=]() {
                    on_error(QStringLiteral("create grpc writer failed"));
                },
                Qt::QueuedConnection);
            return;
        }

        FileService::UploadRequest meta_req;
        FileService::FileMeta     *meta = meta_req.mutable_meta();
        meta->set_file_name(remote_name.toStdString());
        meta->set_total_size(file_size);
        meta->set_file_md5(md5.toStdString());
        if (!writer->Write(meta_req)) {
            QMetaObject::invokeMethod(
                this,
                [=]() { on_error(QStringLiteral("send meta failed")); },
                Qt::QueuedConnection);
            return;
        }

        const qint64 chunk_size = 2 * 1024 * 1024;
        QByteArray   buffer;
        buffer.resize(static_cast<int>(chunk_size));
        qint64 sent = 0;
        while (!file.atEnd()) {
            qint64 n = file.read(buffer.data(), chunk_size);
            if (n <= 0) {
                break;
            }
            FileService::UploadRequest chunk_req;
            chunk_req.set_chunk(buffer.constData(), static_cast<size_t>(n));
            if (!writer->Write(chunk_req)) {
                QMetaObject::invokeMethod(
                    this,
                    [=]() { on_error(QStringLiteral("send chunk failed")); },
                    Qt::QueuedConnection);
                return;
            }
            sent += n;
            const int progress = static_cast<int>((sent * 100) / file_size);
            QMetaObject::invokeMethod(
                this, [=]() { on_progress(progress); }, Qt::QueuedConnection);
        }

        writer->WritesDone();
        grpc::Status status = writer->Finish();
        if (!status.ok() || !response.success()) {
            const QString err
                = status.ok() ? QString::fromStdString(response.message())
                              : QString::fromStdString(status.error_message());
            QMetaObject::invokeMethod(
                this, [=]() { on_error(err); }, Qt::QueuedConnection);
            return;
        }

        QMetaObject::invokeMethod(
            this,
            [=]() {
                on_progress(100);
                on_success();
            },
            Qt::QueuedConnection);
    });
    connect(t, &QThread::finished, t, &QObject::deleteLater);
    t->start();
}

void ChatPage::UploadImageAsync(
    const QString &local_path, const QString &remote_name,
    const std::function<void(int)>             &on_progress,
    const std::function<void()>                &on_success,
    const std::function<void(const QString &)> &on_error) {
    UploadFileAsync(local_path, remote_name, on_progress, on_success, on_error);
}

void ChatPage::DownloadFileAsync(
    const QString &remote_name, const QString &save_path,
    const std::function<void(int)>             &on_progress,
    const std::function<void()>                &on_success,
    const std::function<void(const QString &)> &on_error) {
    if (QFile::exists(save_path)) {
        on_success();
        return;
    }

    auto *t = QThread::create([=]() {
        QString app_path    = QCoreApplication::applicationDirPath();
        QString config_path = QDir::toNativeSeparators(
            app_path + QDir::separator() + "config.ini");
        QSettings settings(config_path, QSettings::IniFormat);
        QString   host = settings.value("FileServer/host").toString();
        QString   port = settings.value("FileServer/port").toString();
        if (host.isEmpty() || port.isEmpty()) {
            QMetaObject::invokeMethod(
                this,
                [=]() {
                    on_error(QStringLiteral("invalid fileserver host/port"));
                },
                Qt::QueuedConnection);
            return;
        }

        auto channel = grpc::CreateChannel(
            (host + ":" + port).toStdString(),
            grpc::InsecureChannelCredentials());
        auto                stub = FileService::FileTransport::NewStub(channel);
        grpc::ClientContext ctx;
        FileService::DownloadRequest req;
        req.set_file_name(remote_name.toStdString());
        req.set_start_offset(0);
        auto reader = stub->DownloadFile(&ctx, req);

        QDir().mkpath(QFileInfo(save_path).absolutePath());
        QFile out(save_path);
        if (!out.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            QMetaObject::invokeMethod(
                this,
                [=]() { on_error(QStringLiteral("open save file failed")); },
                Qt::QueuedConnection);
            return;
        }

        FileService::DownloadResponse resp;
        bool                          got_data = false;
        qint64                        total    = 0;
        qint64                        received = 0;
        while (reader->Read(&resp)) {
            if (resp.has_meta()) {
                total = resp.meta().total_size();
            }
            if (resp.has_chunk()) {
                const std::string chunk = resp.chunk();
                if (!chunk.empty()) {
                    out.write(chunk.data(), static_cast<qint64>(chunk.size()));
                    got_data = true;
                    received += static_cast<qint64>(chunk.size());
                    if (total > 0) {
                        const int progress
                            = static_cast<int>((received * 100) / total);
                        QMetaObject::invokeMethod(
                            this,
                            [=]() { on_progress(progress); },
                            Qt::QueuedConnection);
                    }
                }
            }
        }
        out.close();

        grpc::Status status = reader->Finish();
        if (!status.ok() || !got_data) {
            QFile::remove(save_path);
            const QString err
                = status.ok() ? QStringLiteral("empty file stream")
                              : QString::fromStdString(status.error_message());
            QMetaObject::invokeMethod(
                this, [=]() { on_error(err); }, Qt::QueuedConnection);
            return;
        }
        QMetaObject::invokeMethod(
            this,
            [=]() {
                on_progress(100);
                on_success();
            },
            Qt::QueuedConnection);
    });
    connect(t, &QThread::finished, t, &QObject::deleteLater);
    t->start();
}

void ChatPage::DownloadImageAsync(
    const QString                              &remote_name,
    const std::function<void(const QPixmap &)> &on_success,
    const std::function<void(const QString &)> &on_error) {
    const QString save_path
        = DataPaths::ImagesDir() + QDir::separator() + remote_name;
    if (QFile::exists(save_path)) {
        QPixmap pix(save_path);
        if (!pix.isNull()) {
            on_success(pix);
            return;
        }
    }

    auto *t = QThread::create([=]() {
        QString app_path    = QCoreApplication::applicationDirPath();
        QString config_path = QDir::toNativeSeparators(
            app_path + QDir::separator() + "config.ini");
        QSettings settings(config_path, QSettings::IniFormat);
        QString   host = settings.value("FileServer/host").toString();
        QString   port = settings.value("FileServer/port").toString();
        if (host.isEmpty() || port.isEmpty()) {
            QMetaObject::invokeMethod(
                this,
                [=]() {
                    on_error(QStringLiteral("invalid fileserver host/port"));
                },
                Qt::QueuedConnection);
            return;
        }

        auto channel = grpc::CreateChannel(
            (host + ":" + port).toStdString(),
            grpc::InsecureChannelCredentials());
        auto                stub = FileService::FileTransport::NewStub(channel);
        grpc::ClientContext ctx;
        FileService::DownloadRequest req;
        req.set_file_name(remote_name.toStdString());
        req.set_start_offset(0);
        auto reader = stub->DownloadFile(&ctx, req);

        QDir().mkpath(QFileInfo(save_path).absolutePath());
        QFile out(save_path);
        if (!out.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            QMetaObject::invokeMethod(
                this,
                [=]() { on_error(QStringLiteral("open save image failed")); },
                Qt::QueuedConnection);
            return;
        }

        FileService::DownloadResponse resp;
        bool                          got_data = false;
        while (reader->Read(&resp)) {
            if (resp.has_chunk()) {
                const std::string chunk = resp.chunk();
                if (!chunk.empty()) {
                    out.write(chunk.data(), static_cast<qint64>(chunk.size()));
                    got_data = true;
                }
            }
        }
        out.close();

        grpc::Status status = reader->Finish();
        if (!status.ok() || !got_data) {
            QFile::remove(save_path);
            const QString err
                = status.ok() ? QStringLiteral("empty image stream")
                              : QString::fromStdString(status.error_message());
            QMetaObject::invokeMethod(
                this, [=]() { on_error(err); }, Qt::QueuedConnection);
            return;
        }

        QPixmap pix(save_path);
        if (pix.isNull()) {
            QMetaObject::invokeMethod(
                this,
                [=]() { on_error(QStringLiteral("load image failed")); },
                Qt::QueuedConnection);
            return;
        }
        QMetaObject::invokeMethod(
            this, [=]() { on_success(pix); }, Qt::QueuedConnection);
    });
    connect(t, &QThread::finished, t, &QObject::deleteLater);
    t->start();
}

void ChatPage::AppendChatMsg(std::shared_ptr<TextChatData> msg) {
    auto       self_info = UserManager::getInstance()->GetUserInfo();
    ChatRole   role;
    QString    image_remote_name;
    QString    file_remote_name;
    QString    file_display_name;
    qint64     file_size = 0;
    const bool is_image = IsImagePayload(msg->_msg_content, &image_remote_name);
    FilePayloadInfo file_info;
    const bool      is_file = ParseFilePayload(msg->_msg_content, &file_info);
    const bool      is_doc
        = is_file
          && (file_info.is_doc || IsOfficeDocument(file_info.display_name));
    if (is_file) {
        file_remote_name  = file_info.remote_name;
        file_display_name = file_info.display_name;
        file_size         = file_info.size;
        qDebug() << "[chat-page] file payload parsed"
                 << "msg_id=" << msg->_msg_id << "remote=" << file_remote_name
                 << "name=" << file_display_name << "size=" << file_size
                 << "is_doc=" << is_doc;
    } else if (msg->_msg_content.startsWith("[file]")) {
        qDebug() << "[chat-page] file payload parse failed"
                 << "msg_id=" << msg->_msg_id
                 << "content=" << msg->_msg_content;
    }
    // todo... 添加聊天显示
    if (msg->_from_uid == self_info->_uid) {
        role                    = ChatRole::SELF;
        ChatItemBase *pChatItem = new ChatItemBase(role);

        pChatItem->setUserName(self_info->_name);
        pChatItem->setUserIcon(
            AvatarCache::getInstance()->PixmapOrPlaceholder(
                self_info->_uid, self_info->_icon));
        QWidget *pBubble = nullptr;
        if (is_file) {
            auto *file_bubble = new FileBubble(file_display_name, role);
            file_bubble->setSelectable(false);
            file_bubble->setReceived(false);
            if (is_doc) {
                file_bubble->setIconPath(":/img/document.png");
            }
            pBubble = file_bubble;
        } else if (is_image) {
            const QString local_image = DataPaths::ImagesDir()
                                        + QDir::separator() + image_remote_name;
            QPixmap       pix(local_image);
            if (pix.isNull()) {
                pix = QPixmap(msg->_msg_content);
            }
            pBubble             = new PictureBubble(pix, role);
            auto picture_bubble = qobject_cast<PictureBubble *>(pBubble);
            if (picture_bubble) {
                picture_bubble->setUploadDone();
                if (pix.isNull()) {
                    DownloadImageAsync(
                        image_remote_name,
                        [picture_bubble](const QPixmap &downloaded) {
                            picture_bubble->setPicture(downloaded);
                        },
                        [](const QString &) {});
                }
            }
        } else {
            pBubble = new TextBubble(role, msg->_msg_content);
        }
        auto bubble_frame = qobject_cast<BubbleFrame *>(pBubble);
        if (bubble_frame) {
            bubble_frame->setTimeText(FormatChatTime(msg->_timestamp));
        }
        pChatItem->setWidget(pBubble);
        ui->chat_data_list->appendChatItem(pChatItem);
        PlayBubbleEnterAnimation(pChatItem, pBubble);
    } else {
        role                    = ChatRole::OTHER;
        ChatItemBase *pChatItem = new ChatItemBase(role);
        auto          friend_info
            = UserManager::getInstance()->GetFriendById(msg->_from_uid);
        QString    peer_name = QString::number(msg->_from_uid);
        QString    peer_icon;
        int        peer_uid    = msg->_from_uid;
        const bool is_ai_reply = IsBotUid(msg->_from_uid);
        if (is_ai_reply) {
            auto bot_info = BuildBotUserInfo();
            peer_name     = bot_info->_name;
            peer_icon     = bot_info->_icon;
            peer_uid      = bot_info->_uid;
        } else if (friend_info) {
            peer_name = friend_info->_name;
            peer_icon = friend_info->_icon;
            peer_uid  = friend_info->_uid;
        }
        pChatItem->setUserName(peer_name);
        pChatItem->setUserIcon(
            AvatarCache::getInstance()->PixmapOrPlaceholder(
                peer_uid, peer_icon));
        QWidget *pBubble = nullptr;
        if (is_file) {
            auto *file_bubble = new FileBubble(file_display_name, role);
            file_bubble->setSelectable(false);
            file_bubble->setReceived(false);
            if (is_doc) {
                file_bubble->setIconPath(":/img/document.png");
            }
            const QString msg_id          = msg->_msg_id;
            _pending_file_bubbles[msg_id] = file_bubble;
            _pending_file_remote[msg_id]  = file_remote_name;
            _pending_file_name[msg_id]    = file_display_name;
            connect(
                file_bubble,
                &FileBubble::sig_clicked,
                this,
                [this, msg_id, file_bubble, file_info, is_doc]() {
                    if (!file_bubble) {
                        return;
                    }
                    if (is_doc) {
                        QMessageBox box(this);
                        box.setWindowTitle(tr("文档操作"));
                        box.setText(tr("请选择操作"));
                        QPushButton *btn_edit = box.addButton(
                            tr("在线编辑"), QMessageBox::AcceptRole);
                        QPushButton *btn_download = box.addButton(
                            tr("下载"), QMessageBox::DestructiveRole);
                        box.addButton(tr("取消"), QMessageBox::RejectRole);
                        box.exec();

                        if (box.clickedButton() == btn_edit) {
                            const QUrl url
                                = BuildOnlyOfficeUrl(file_info.remote_name);
                            if (url.host().isEmpty()) {
                                QMessageBox::warning(
                                    this,
                                    tr("打开失败"),
                                    tr("GateServer 地址未配置"));
                                return;
                            }
                            OpenOnlyOfficeViewer(this, url);
                            return;
                        }
                        if (box.clickedButton() != btn_download) {
                            return;
                        }
                    } else {
                        const auto ret = QMessageBox::question(
                            this,
                            tr("接收文件"),
                            tr("是否接收该文件？"),
                            QMessageBox::Yes | QMessageBox::No,
                            QMessageBox::Yes);
                        if (ret != QMessageBox::Yes) {
                            return;
                        }
                    }
                    const QString save_path = DataPaths::FilesDir()
                                              + QDir::separator()
                                              + file_info.remote_name;
                    file_bubble->setTransferProgress(0);
                    DownloadFileAsync(
                        file_info.remote_name,
                        save_path,
                        [file_bubble](int progress) {
                            if (file_bubble) {
                                file_bubble->setTransferProgress(progress);
                            }
                        },
                        [this, msg_id, file_bubble]() {
                            if (file_bubble) {
                                file_bubble->setTransferDone();
                                file_bubble->setReceived(true);
                            }
                            qDebug() << "[chat-page] file download finished"
                                     << "msg_id=" << msg_id;
                            _selected_file_msgids.remove(msg_id);
                        },
                        [file_bubble, msg_id](const QString &err) {
                            if (file_bubble) {
                                file_bubble->setTransferFailed();
                            }
                            qDebug() << "[chat-page] file download failed"
                                     << "msg_id=" << msg_id << "error=" << err;
                        });
                });
            pBubble = file_bubble;
        } else if (is_image) {
            const QString local_image = DataPaths::ImagesDir()
                                        + QDir::separator() + image_remote_name;
            QPixmap       pix(local_image);
            pBubble             = new PictureBubble(pix, role);
            auto picture_bubble = qobject_cast<PictureBubble *>(pBubble);
            if (picture_bubble) {
                picture_bubble->setUploadDone();
                if (pix.isNull()) {
                    DownloadImageAsync(
                        image_remote_name,
                        [picture_bubble](const QPixmap &downloaded) {
                            picture_bubble->setPicture(downloaded);
                        },
                        [](const QString &) {});
                }
            }
        } else {
            const auto format = is_ai_reply
                                    ? TextBubble::ContentFormat::Markdown
                                    : TextBubble::ContentFormat::PlainText;
            pBubble           = new TextBubble(role, msg->_msg_content, format);
        }
        auto bubble_frame = qobject_cast<BubbleFrame *>(pBubble);
        if (bubble_frame) {
            bubble_frame->setTimeText(FormatChatTime(msg->_timestamp));
        }
        pChatItem->setWidget(pBubble);
        ui->chat_data_list->appendChatItem(pChatItem);
        PlayBubbleEnterAnimation(pChatItem, pBubble);
    }
}
