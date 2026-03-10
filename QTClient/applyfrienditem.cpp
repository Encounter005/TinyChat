#include "applyfrienditem.h"
#include "avatarcache.h"
#include "ui_applyfrienditem.h"

ApplyFriendItem::ApplyFriendItem(QWidget* parent)
    : ListItemBase(parent), ui(new Ui::ApplyFriendItem), _added(false) {
    ui->setupUi(this);
    SetItemType(ListItemType::APPLY_FRIEND_ITEM);
    ui->addBtn->SetState("normal", "hover", "press");
    connect(ui->addBtn, &ClickedBtn::clicked, [this]() {
        emit this->sig_auth_friend(_apply_info);
    });

    connect(
        AvatarCache::getInstance().get(),
        &AvatarCache::avatarReady,
        this,
        [this](int uid, const QString&) {
            if (!_apply_info || uid != _apply_info->_uid) {
                return;
            }
            QPixmap pixmap = AvatarCache::getInstance()->PixmapOrPlaceholder(
                _apply_info->_uid, _apply_info->_icon);
            ui->icon_lb->setPixmap(pixmap.scaled(
                ui->icon_lb->size(),
                Qt::KeepAspectRatio,
                Qt::SmoothTransformation));
        });

    connect(
        AvatarCache::getInstance().get(),
        &AvatarCache::avatarUpdated,
        this,
        [this](int uid, const QString& iconName) {
            if (!_apply_info || uid != _apply_info->_uid) return;
            _apply_info->_icon = iconName;   // 关键：写回新 iconName
            QPixmap pix  = AvatarCache::getInstance()->PixmapOrPlaceholder(
                uid, _apply_info->_icon);
            ui->icon_lb->setPixmap(pix.scaled(
                ui->icon_lb->size(),
                Qt::KeepAspectRatio,
                Qt::SmoothTransformation));
        });
}

ApplyFriendItem::~ApplyFriendItem() {
    delete ui;
}

void ApplyFriendItem::SetInfo(std::shared_ptr<ApplyInfo> apply_info) {
    _apply_info = apply_info;
    // 加载图片
    QPixmap pixmap = AvatarCache::getInstance()->PixmapOrPlaceholder(
        _apply_info->_uid, _apply_info->_icon);
    // 设置图片自动缩放
    ui->icon_lb->setPixmap(pixmap.scaled(
        ui->icon_lb->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
    ui->user_name_lb->setText(_apply_info->_name);
    ui->user_chat_lb->setText(_apply_info->_desc);
}

void ApplyFriendItem::ShowAddBtn(bool show) {
    if (show) {
        ui->addBtn->show();
        ui->already_add_lb->hide();
        _added = false;
    } else {
        ui->addBtn->hide();
        ui->already_add_lb->show();
        _added = true;
    }
}

QSize ApplyFriendItem::sizeHint() const {
    return QSize(250, 80);
}

int ApplyFriendItem::GetUid() {
    return _apply_info->_uid;
}
