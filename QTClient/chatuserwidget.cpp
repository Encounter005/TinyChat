#include "chatuserwidget.h"
#include "ui_chatuserwidget.h"
#include "listitembase.h"

ChatUserWidget::ChatUserWidget(QWidget *parent) :  ListItemBase(parent)
    , ui(new Ui::ChatUserWidget)
{
    ui->setupUi(this);
    SetItemType(ListItemType::CHAT_USER_ITEM);
}

ChatUserWidget::~ChatUserWidget()
{
    delete ui;
}

void ChatUserWidget::SetInfo(std::shared_ptr<UserInfo> user_info)
{
    _user_info = user_info;
    // 加载图片
    QPixmap pixmap(_user_info->_icon);

    // 设置图片自动缩放
    ui->icon_label->setPixmap(pixmap.scaled(ui->icon_label->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
    ui->icon_label->setScaledContents(true);

    ui->user_name_label->setText(_user_info->_name);
    ui->user_chat_label->setText(_user_info->_last_msg);
}

void ChatUserWidget::SetInfo(std::shared_ptr<FriendInfo> friend_info)
{
    _user_info = std::make_shared<UserInfo>(friend_info);
    // 加载图片
    QPixmap pixmap(_user_info->_icon);

    // 设置图片自动缩放
    ui->icon_label->setPixmap(pixmap.scaled(ui->icon_label->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
    ui->icon_label->setScaledContents(true);

    ui->user_name_label->setText(_user_info->_name);
    ui->user_chat_label->setText(_user_info->_last_msg);

}



void ChatUserWidget::SetInfo(QString name, QString head, QString msg)
{
    _name = name;
    _head = head;
    _msg = msg;
    QPixmap pixmap(_head);
    ui->icon_label->setPixmap(pixmap.scaled(ui->icon_label->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
    ui->icon_label->setScaledContents(true);

    ui->user_name_label->setText(_name);
    ui->user_chat_label->setText(_msg);
}


std::shared_ptr<UserInfo> ChatUserWidget::GetUserInfo()
{
    return _user_info;
}

void ChatUserWidget::updateLastMsg(std::vector<std::shared_ptr<TextChatData> > msgs)
{
    QString last_msg = "";
    for (auto& msg : msgs) {
        last_msg = msg->_msg_content;
        _user_info->_chat_msgs.push_back(msg);
    }

    _user_info->_last_msg = last_msg;
    ui->user_chat_label->setText(_user_info->_last_msg);

}
