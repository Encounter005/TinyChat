#include "contactuseritem.h"
#include "ui_contactuseritem.h"
#include <QPixmap>

ContactUserItem::ContactUserItem(QWidget *parent)
    : ListItemBase(parent)
    , ui(new Ui::ContactUserItem)
{
    ui->setupUi(this);
    SetItemType(ListItemType::CONTACT_USER_ITEM);
    ui->redpoint->raise();
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
    QPixmap pixmap(_info->_icon);
    ui->icon_label->setPixmap(pixmap.scaled(ui->icon_label->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
    ui->icon_label->setScaledContents(true);
    ui->user_name_label->setText(_info->_name);
}

void ContactUserItem::SetInfo(std::shared_ptr<AuthRsp> auth_rsp)
{
    _info = std::make_shared<UserInfo>(auth_rsp);
    QPixmap pixmap(_info->_icon);
    ui->icon_label->setPixmap(pixmap.scaled(ui->icon_label->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
    ui->icon_label->setScaledContents(true);
    ui->user_name_label->setText(_info->_name);
}

void ContactUserItem::SetInfo(int uid, QString name, QString icon)
{
    _info = std::make_shared<UserInfo>(uid, name, icon);
    QPixmap pixmap(_info->_icon);
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
