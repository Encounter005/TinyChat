#include "contactuseritem.h"
#include "avatarcache.h"
#include "ui_contactuseritem.h"
#include <QPixmap>

ContactUserItem::ContactUserItem(QWidget *parent)
    : ListItemBase(parent)
    , ui(new Ui::ContactUserItem)
{
    ui->setupUi(this);
    SetItemType(ListItemType::CONTACT_USER_ITEM);
    ui->redpoint->raise();

    connect(AvatarCache::getInstance().get(),
            &AvatarCache::avatarReady,
            this,
            [this](int uid, const QString&) {
                if (!_info || uid != _info->_uid) {
                    return;
                }
                QPixmap pixmap = AvatarCache::getInstance()->PixmapOrPlaceholder(
                    _info->_uid, _info->_icon);
                ui->icon_label->setPixmap(pixmap.scaled(
                    ui->icon_label->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
            });

    connect(AvatarCache::getInstance().get(),
            &AvatarCache::avatarUpdated,
            this,
            [this](int uid, const QString&) {
                if (!_info || uid != _info->_uid) {
                    return;
                }
                QPixmap pixmap = AvatarCache::getInstance()->PixmapOrPlaceholder(
                    _info->_uid, _info->_icon);
                ui->icon_label->setPixmap(pixmap.scaled(
                    ui->icon_label->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
            });
}

ContactUserItem::~ContactUserItem()
{
    delete ui;
}

QSize ContactUserItem::sizeHint() const
{
    return QSize(250, 70);
}

void ContactUserItem::SetInfo(std::shared_ptr<AuthInfo> auth_info)
{
    _info = std::make_shared<UserInfo>(auth_info);
    QPixmap pixmap = AvatarCache::getInstance()->PixmapOrPlaceholder(
        _info->_uid, _info->_icon);
    ui->icon_label->setPixmap(pixmap.scaled(ui->icon_label->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
    ui->icon_label->setScaledContents(true);
    ui->user_name_label->setText(_info->_name);
}

void ContactUserItem::SetInfo(std::shared_ptr<AuthRsp> auth_rsp)
{
    _info = std::make_shared<UserInfo>(auth_rsp);
    QPixmap pixmap = AvatarCache::getInstance()->PixmapOrPlaceholder(
        _info->_uid, _info->_icon);
    ui->icon_label->setPixmap(pixmap.scaled(ui->icon_label->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
    ui->icon_label->setScaledContents(true);
    ui->user_name_label->setText(_info->_name);
}

void ContactUserItem::SetInfo(int uid, QString name, QString icon)
{
    _info = std::make_shared<UserInfo>(uid, name, icon);
    QPixmap pixmap = AvatarCache::getInstance()->PixmapOrPlaceholder(
        _info->_uid, _info->_icon);
    ui->icon_label->setPixmap(pixmap.scaled(ui->icon_label->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
    ui->icon_label->setScaledContents(true);
    ui->user_name_label->setText(_info->_name);
}

void ContactUserItem::ShowRedPoint(bool show)
{
    if(show) {
        ui->redpoint->show();
    } else {
        ui->redpoint->hide();
    }
}

std::shared_ptr<UserInfo> ContactUserItem::GetUserInfo()
{
    return _info;
}
