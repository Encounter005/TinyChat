#include "contactuseritem.h"
#include "avatarcache.h"
#include "ui_contactuseritem.h"
#include <QPixmap>
#include <QStyle>

ContactUserItem::ContactUserItem(QWidget* parent)
    : ListItemBase(parent), ui(new Ui::ContactUserItem) {
    ui->setupUi(this);
    SetItemType(ListItemType::CONTACT_USER_ITEM);
    ui->redpoint->raise();
    ui->redpoint->setProperty("unread", false);
    ui->redpoint->hide();

    connect(
        AvatarCache::getInstance().get(),
        &AvatarCache::avatarReady,
        this,
        [this](int uid, const QString&) {
            if (!_info || uid != _info->_uid) {
                return;
            }
            QPixmap pixmap = AvatarCache::getInstance()->PixmapOrPlaceholder(
                _info->_uid, _info->_icon);
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
            if (!_info || uid != _info->_uid) return;
            _info->_icon = iconName;  
            QPixmap pix  = AvatarCache::getInstance()->PixmapOrPlaceholder(
                uid, _info->_icon);
            ui->icon_label->setPixmap(pix.scaled(
                ui->icon_label->size(),
                Qt::KeepAspectRatio,
                Qt::SmoothTransformation));
        });
}

ContactUserItem::~ContactUserItem() {
    delete ui;
}

QSize ContactUserItem::sizeHint() const {
    return QSize(250, 70);
}

void ContactUserItem::SetInfo(std::shared_ptr<AuthInfo> auth_info) {
    _info          = std::make_shared<UserInfo>(auth_info);
    QPixmap pixmap = AvatarCache::getInstance()->PixmapOrPlaceholder(
        _info->_uid, _info->_icon);
    ui->icon_label->setPixmap(pixmap.scaled(
        ui->icon_label->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
    ui->user_name_label->setText(_info->_name);
}

void ContactUserItem::SetInfo(std::shared_ptr<AuthRsp> auth_rsp) {
    _info          = std::make_shared<UserInfo>(auth_rsp);
    QPixmap pixmap = AvatarCache::getInstance()->PixmapOrPlaceholder(
        _info->_uid, _info->_icon);
    ui->icon_label->setPixmap(pixmap.scaled(
        ui->icon_label->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
    ui->user_name_label->setText(_info->_name);
}

void ContactUserItem::SetInfo(int uid, QString name, QString icon) {
    _info          = std::make_shared<UserInfo>(uid, name, icon);
    QPixmap pixmap = AvatarCache::getInstance()->PixmapOrPlaceholder(
        _info->_uid, _info->_icon);
    ui->icon_label->setPixmap(pixmap.scaled(
        ui->icon_label->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
    ui->user_name_label->setText(_info->_name);
}

void ContactUserItem::ShowRedPoint(bool show) {
    ui->redpoint->setProperty("unread", show);
    ui->redpoint->style()->unpolish(ui->redpoint);
    ui->redpoint->style()->polish(ui->redpoint);
    ui->redpoint->setVisible(show);
}

std::shared_ptr<UserInfo> ContactUserItem::GetUserInfo() {
    return _info;
}
