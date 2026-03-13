#include "userinfopage.h"
#include "animationtiming.h"
#include "avatarcache.h"
#include "global.h"
#include "httpmanager.h"
#include "imagecropperdialog.h"
#include "imagecropperlabel.h"
#include "qdatetime.h"
#include "qjsondocument.h"
#include "ui_userinfopage.h"
#include "usermanager.h"
#include <QBuffer>
#include <QDebug>
#include <QDir>
#include <QFileDialog>
#include <QMessageBox>
#include <QPainter>
#include <QGraphicsOpacityEffect>
#include <QParallelAnimationGroup>
#include <QPropertyAnimation>
#include <QStandardPaths>
#include <QTimer>

namespace {

static QPixmap MakeSquareAvatar(const QPixmap& src, int sizePx) {
    if (src.isNull() || sizePx <= 0) return QPixmap();

    // 等比放大到至少覆盖 sizePx x sizePx
    QPixmap scaled = src.scaled(
        sizePx,
        sizePx,
        Qt::KeepAspectRatioByExpanding,
        Qt::SmoothTransformation);

    // 居中裁切成正方形
    int x = (scaled.width() - sizePx) / 2;
    int y = (scaled.height() - sizePx) / 2;

    QPixmap out(sizePx, sizePx);
    out.fill(Qt::transparent);

    QPainter p(&out);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setRenderHint(QPainter::SmoothPixmapTransform, true);
    p.drawPixmap(0, 0, scaled, x, y, sizePx, sizePx);
    p.end();

    return out;
}
}   // namespace

UserInfoPage::UserInfoPage(QWidget* parent)
    : QWidget(parent), ui(new Ui::UserInfoPage) {
    ui->setupUi(this);
    auto icon = UserManager::getInstance()->GetIcon();
    auto uid  = UserManager::getInstance()->GetUid();
    qDebug() << "icon is " << icon;
    QPixmap pixmap = AvatarCache::getInstance()->PixmapOrPlaceholder(uid, icon);
    QPixmap scaledPixmap = pixmap.scaled(
        ui->head_lb->size(),
        Qt::KeepAspectRatio,
        Qt::SmoothTransformation);          // 将图片缩放到label的大小
    ui->head_lb->setPixmap(scaledPixmap);   // 将缩放后的图片设置到QLabel上
    ui->head_lb->setScaledContents(
        true);   // 设置QLabel自动缩放图片内容以适应大小
    // 获取nick
    auto nick = UserManager::getInstance()->GetNick();
    // 获取name
    auto name = UserManager::getInstance()->GetName();
    // 描述
    auto desc = UserManager::getInstance()->GetDesc();
    ui->uid_ed->setText(QString::number(uid));
    ui->nick_ed->setText(nick);
    ui->name_ed->setText(name);
    ui->desc_ed->setText(desc);

    ui->nick_ed->installEventFilter(this);
    ui->name_ed->installEventFilter(this);
    ui->desc_ed->installEventFilter(this);
    QTimer::singleShot(0, this, [this]() { playPageIntroAnimation(); });

    connect(
        AvatarCache::getInstance().get(),
        &AvatarCache::avatarReady,
        this,
        [this](int changed_uid, const QString&) {
            int uid = UserManager::getInstance()->GetUid();
            if (changed_uid != uid) {
                return;
            }
            auto    icon = UserManager::getInstance()->GetIcon();
            QPixmap pixmap
                = AvatarCache::getInstance()->PixmapOrPlaceholder(uid, icon);
            ui->head_lb->setPixmap(pixmap.scaled(
                ui->head_lb->size(),
                Qt::KeepAspectRatio,
                Qt::SmoothTransformation));
        });

    connect(
        AvatarCache::getInstance().get(),
        &AvatarCache::avatarUpdated,
        this,
        [this](int changed_uid, const QString&) {
            int uid = UserManager::getInstance()->GetUid();
            if (changed_uid != uid) {
                return;
            }
            auto    icon = UserManager::getInstance()->GetIcon();
            QPixmap pixmap
                = AvatarCache::getInstance()->PixmapOrPlaceholder(uid, icon);
            ui->head_lb->setPixmap(pixmap.scaled(
                ui->head_lb->size(),
                Qt::KeepAspectRatio,
                Qt::SmoothTransformation));
        });

    connect(
        HttpManager::getInstance().get(),
        &HttpManager::sig_user_mod_finish,
        this,
        [this](ReqId id, QString res, ErrorCodes err) {
            if (id != ReqId::ID_UPDATE_ICON || err != ErrorCodes::SUCCESS)
                return;
            QJsonDocument doc = QJsonDocument::fromJson(res.toUtf8());
            if (!doc.isObject()) return;
            auto obj = doc.object();
            if (obj["error"].toInt() != 0) return;
            QString icon = obj["icon"].toString();
            if (icon.isEmpty()) return;
            auto user   = UserManager::getInstance()->GetUserInfo();
            user->_icon = icon;
        });
}

UserInfoPage::~UserInfoPage() {
    delete ui;
}

bool UserInfoPage::eventFilter(QObject *watched, QEvent *event)
{
    if (event != nullptr
        && (watched == ui->nick_ed || watched == ui->name_ed
            || watched == ui->desc_ed)) {
        if (event->type() == QEvent::FocusIn) {
            animateInputHeight(qobject_cast<QLineEdit *>(watched), 40);
        } else if (event->type() == QEvent::FocusOut) {
            animateInputHeight(qobject_cast<QLineEdit *>(watched), 30);
        }
    }
    return QWidget::eventFilter(watched, event);
}

void UserInfoPage::animateInputHeight(QLineEdit *edit, int target_height)
{
    if (edit == nullptr) {
        return;
    }
    auto *anim = new QPropertyAnimation(edit, "maximumHeight", edit);
    anim->setDuration(UiAnim::kInputFocusMs);
    anim->setStartValue(edit->maximumHeight());
    anim->setEndValue(target_height);
    anim->setEasingCurve(QEasingCurve::OutCubic);
    anim->start(QAbstractAnimation::DeleteWhenStopped);
}

void UserInfoPage::playPageIntroAnimation()
{
    auto animate_one = [](QWidget *w, int delay_ms) {
        if (!w) {
            return;
        }

        QTimer::singleShot(delay_ms, w, [w]() {
            const QPoint end_pos = w->pos();
            const QPoint start_pos = end_pos + QPoint(36, 0);
            w->move(start_pos);

            auto *group = new QParallelAnimationGroup(w);

            auto *pos_anim = new QPropertyAnimation(w, "pos", group);
            pos_anim->setDuration(UiAnim::kListItemFadeMs);
            pos_anim->setStartValue(start_pos);
            pos_anim->setEndValue(end_pos);
            pos_anim->setEasingCurve(QEasingCurve::OutCubic);
            group->addAnimation(pos_anim);

            auto *effect = new QGraphicsOpacityEffect(w);
            effect->setOpacity(0.0);
            w->setGraphicsEffect(effect);

            auto *opacity_anim = new QPropertyAnimation(effect, "opacity", group);
            opacity_anim->setDuration(UiAnim::kListItemFadeMs);
            opacity_anim->setStartValue(0.0);
            opacity_anim->setEndValue(1.0);
            opacity_anim->setEasingCurve(QEasingCurve::OutCubic);
            group->addAnimation(opacity_anim);

            QObject::connect(group, &QParallelAnimationGroup::finished, w, [w, end_pos]() {
                w->move(end_pos);
                w->setGraphicsEffect(nullptr);
            });
            group->start(QAbstractAnimation::DeleteWhenStopped);
        });
    };

    animate_one(ui->head_lb, 0);
    animate_one(ui->up_btn, 40);
    animate_one(ui->uid_ed, 80);
    animate_one(ui->nick_ed, 120);
    animate_one(ui->name_ed, 160);
    animate_one(ui->desc_ed, 200);
    animate_one(ui->submit_btn, 240);
}

// 上传头像
void UserInfoPage::on_up_btn_clicked() {
    // 1. 让对话框也能选 *.webp
    QString filename = QFileDialog::getOpenFileName(
        this,
        tr("选择图片"),
        QString(),
        tr("图片文件 (*.png *.jpg *.jpeg *.bmp *.webp)"));
    if (filename.isEmpty()) return;

    // 2. 直接用 QPixmap::load() 加载，无需手动区分格式
    QPixmap inputImage;
    if (!inputImage.load(filename)) {
        QMessageBox::critical(
            this,
            tr("错误"),
            tr("加载图片失败！请确认已部署 WebP 插件。"),
            QMessageBox::Ok);
        return;
    }

    QPixmap image = ImageCropperDialog::getCroppedImage(
        filename, 600, 400, CropperShape::CIRCLE);
    if (image.isNull()) return;

    constexpr int kAvatarSize = 256;
    QPixmap       avatar      = MakeSquareAvatar(image, kAvatarSize);
    if (avatar.isNull()) return;


    QPixmap scaled = avatar.scaled(
        ui->head_lb->size(),
        Qt::KeepAspectRatio,
        Qt::SmoothTransformation);    // 将图片缩放到label的大小
    ui->head_lb->setPixmap(scaled);   // 将缩放后的图片设置到QLabel上
    ui->head_lb->setScaledContents(
        true);   // 设置QLabel自动缩放图片内容以适应大小
    // 生成iconName + 本地cache

    int     uid      = UserManager::getInstance()->GetUid();
    QString iconName = QString("avatar_%1_%2.png")
                           .arg(uid)
                           .arg(QDateTime::currentSecsSinceEpoch());
    QString localPath = AvatarCache::getInstance()->Cachepath(uid, iconName);
    avatar.save(localPath, "PNG", 9);

    UserManager::getInstance()->GetUserInfo()->_icon = iconName;
    emit AvatarCache::getInstance() -> avatarUpdated(uid, iconName);

    // http request
    QByteArray raw;
    QBuffer    buffer(&raw);
    buffer.open(QIODevice::WriteOnly);
    avatar.save(&buffer, "PNG", 9);

    QJsonObject json;
    json["uid"]       = uid;
    json["ext"]       = "png";
    json["image_b64"] = QString::fromLatin1(raw.toBase64());


    qDebug() << "ready to send post request ..........";
    qDebug() << "data size is " << "raw: " << raw.size()
             << " base64: " << json["image_b64"].toString().size()
             << " document size is: " << QJsonDocument(json).toJson().size();
    HttpManager::getInstance()->PostHttpReq(
        QUrl(gate_url_prefix + "/user_update_icon"),
        json,
        ReqId::ID_UPDATE_ICON,
        Modules::USERMOD);
}
