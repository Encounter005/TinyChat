#include "chatuserwidget.h"
#include "avatarcache.h"
#include "listitembase.h"
#include "ui_chatuserwidget.h"
#include <QDateTime>

namespace {
const QString kImagePayloadPrefix = "[img]";
const QString kFilePayloadPrefix = "[file]";

qint64 ExtractTsFromMsgId(const QString& msg_id)
{
    if (!msg_id.startsWith("msg_")) {
        return 0;
    }
    const int second = msg_id.indexOf('_', 4);
    if (second <= 4) {
        return 0;
    }
    bool ok = false;
    const qint64 ts = msg_id.mid(4, second - 4).toLongLong(&ok);
    return ok ? ts : 0;
}

QString FormatItemTime(qint64 ts)
{
    if (ts <= 0) {
        return QString();
    }
    return QDateTime::fromSecsSinceEpoch(ts).toString("hh:mm");
}

QString ToPreviewText(const QString &content)
{
    if (content.startsWith(kImagePayloadPrefix)) {
        return QStringLiteral("[图片]");
    }
    if (content.startsWith(kFilePayloadPrefix)) {
        return QStringLiteral("[文件]");
    }
    return content;
}
}

ChatUserWidget::ChatUserWidget(QWidget* parent)
    : ListItemBase(parent), ui(new Ui::ChatUserWidget) {
    ui->setupUi(this);
    SetItemType(ListItemType::CHAT_USER_ITEM);

    connect(
        AvatarCache::getInstance().get(),
        &AvatarCache::avatarReady,
        this,
        [this](int uid, const QString&) {
            if (!_user_info || uid != _user_info->_uid) {
                return;
            }
            QPixmap pixmap = AvatarCache::getInstance()->PixmapOrPlaceholder(
                _user_info->_uid, _user_info->_icon);
            ui->icon_label->setPixmap(pixmap.scaled(
                ui->icon_label->size(),
                Qt::KeepAspectRatio,
                Qt::SmoothTransformation));
        });

    connect(
        AvatarCache::getInstance().get(),
        &AvatarCache::avatarUpdated,
        this,
        [this](int uid, const QString& iconName) {
            if (!_user_info || uid != _user_info->_uid) return;
            _user_info->_icon = iconName;
            QPixmap pix  = AvatarCache::getInstance()->PixmapOrPlaceholder(
                uid, _user_info->_icon);
            ui->icon_label->setPixmap(pix.scaled(
                ui->icon_label->size(),
                Qt::KeepAspectRatio,
                Qt::SmoothTransformation));
        });
}

ChatUserWidget::~ChatUserWidget() {
    delete ui;
}

void ChatUserWidget::SetInfo(std::shared_ptr<UserInfo> user_info) {
    if (!user_info) {
        _user_info = std::make_shared<UserInfo>(0, QStringLiteral("Unknown"), QString());
    } else {
        _user_info = user_info;
    }
    // 加载图片
    QPixmap pixmap = AvatarCache::getInstance()->PixmapOrPlaceholder(
        _user_info->_uid, _user_info->_icon);

    // 设置图片自动缩放
    ui->icon_label->setPixmap(pixmap.scaled(
        ui->icon_label->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));

    ui->user_name_label->setText(_user_info->_name);
    ui->user_chat_label->setText(_user_info->_last_msg);
    ui->time_label->clear();
}

void ChatUserWidget::SetInfo(std::shared_ptr<FriendInfo> friend_info) {
    if (!friend_info) {
        _user_info = std::make_shared<UserInfo>(0, QStringLiteral("Unknown"), QString());
    } else {
        _user_info = std::make_shared<UserInfo>(friend_info);
    }
    // 加载图片
    QPixmap pixmap = AvatarCache::getInstance()->PixmapOrPlaceholder(
        _user_info->_uid, _user_info->_icon);

    // 设置图片自动缩放
    ui->icon_label->setPixmap(pixmap.scaled(
        ui->icon_label->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));

    ui->user_name_label->setText(_user_info->_name);
    ui->user_chat_label->setText(_user_info->_last_msg);
    ui->time_label->clear();
}



void ChatUserWidget::SetInfo(QString name, QString head, QString msg) {
    _name = name;
    _head = head;
    _msg  = msg;
    QPixmap pixmap(_head);
    ui->icon_label->setPixmap(pixmap.scaled(
        ui->icon_label->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));

    ui->user_name_label->setText(_name);
    ui->user_chat_label->setText(_msg);
    ui->time_label->clear();
}


std::shared_ptr<UserInfo> ChatUserWidget::GetUserInfo() {
    return _user_info;
}

void ChatUserWidget::updateLastMsg(
    const std::vector<std::shared_ptr<TextChatData>>& msgs) {
    if (!_user_info) {
        return;
    }

    for (const auto& msg : msgs) {
        if (!msg) {
            continue;
        }
        bool exists = false;
        for (const auto& old : _user_info->_chat_msgs) {
            if (old && old->_msg_id == msg->_msg_id) {
                exists = true;
                break;
            }
        }
        if (!exists) {
            _user_info->_chat_msgs.push_back(msg);
        }
    }

    QString last_msg;
    qint64  latest_ts = -1;
    for (const auto& msg : _user_info->_chat_msgs) {
        if (!msg) {
            continue;
        }
        qint64 ts = msg->_timestamp;
        if (ts <= 0) {
            ts = ExtractTsFromMsgId(msg->_msg_id);
        }
        if (ts >= latest_ts) {
            latest_ts = ts;
            last_msg  = ToPreviewText(msg->_msg_content);
        }
    }

    _user_info->_last_msg = last_msg;
    ui->user_chat_label->setText(_user_info->_last_msg);
    ui->time_label->setText(FormatItemTime(latest_ts));
}
