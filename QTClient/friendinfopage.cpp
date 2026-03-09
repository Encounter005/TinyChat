#include "friendinfopage.h"
#include "avatarcache.h"
#include "ui_friendinfopage.h"
#include <QDebug>

FriendInfoPage::FriendInfoPage(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::FriendInfoPage),_user_info(nullptr)
{
    ui->setupUi(this);
    ui->msg_chat->SetState("normal","hover","press");
    ui->video_chat->SetState("normal","hover","press");
    ui->voice_chat->SetState("normal","hover","press");

    connect(AvatarCache::getInstance().get(),
            &AvatarCache::avatarReady,
            this,
            [this](int uid, const QString&) {
                if (!_user_info || uid != _user_info->_uid) {
                    return;
                }
                QPixmap pixmap = AvatarCache::getInstance()->PixmapOrPlaceholder(
                    _user_info->_uid, _user_info->_icon);
                ui->icon_lb->setPixmap(pixmap.scaled(
                    ui->icon_lb->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
            });

    connect(AvatarCache::getInstance().get(),
            &AvatarCache::avatarUpdated,
            this,
            [this](int uid, const QString&) {
                if (!_user_info || uid != _user_info->_uid) {
                    return;
                }
                QPixmap pixmap = AvatarCache::getInstance()->PixmapOrPlaceholder(
                    _user_info->_uid, _user_info->_icon);
                ui->icon_lb->setPixmap(pixmap.scaled(
                    ui->icon_lb->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
            });
}

FriendInfoPage::~FriendInfoPage()
{
    delete ui;
}

void FriendInfoPage::SetInfo(std::shared_ptr<UserInfo> user_info)
{
    _user_info = user_info;
    // 加载图片
    QPixmap pixmap = AvatarCache::getInstance()->PixmapOrPlaceholder(
        user_info->_uid, user_info->_icon);

    // 设置图片自动缩放
    ui->icon_lb->setPixmap(pixmap.scaled(ui->icon_lb->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
    ui->icon_lb->setScaledContents(true);

    ui->name_lb->setText(user_info->_name);
    ui->nick_lb->setText(user_info->_nick);
    ui->bak_lb->setText(user_info->_nick);
}

void FriendInfoPage::on_msg_chat_clicked()
{
    qDebug() << "msg chat btn clicked";
    emit sig_jump_chat_item(_user_info);
}
