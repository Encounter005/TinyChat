#include "listitembase.h"
#include <QStyleOption>
#include <QPainter>
#include <QEasingCurve>

ListItemBase::ListItemBase(QWidget* parent)
    : QWidget(parent)
    , _itemType(ListItemType::INVALID_ITEM)
    , _hover_progress(0.0)
    , _hover_anim(nullptr) {
    setAttribute(Qt::WA_Hover, true);
    setMouseTracking(true);
}



void ListItemBase::SetItemType(ListItemType itemType)
{
    _itemType = itemType;
}



ListItemType ListItemBase::GetItemType()
{
    return _itemType;
}

qreal ListItemBase::hoverProgress() const
{
    return _hover_progress;
}

void ListItemBase::setHoverProgress(qreal value)
{
    _hover_progress = qBound(0.0, value, 1.0);
    update();
}

void ListItemBase::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);

    if (_itemType == ListItemType::CHAT_USER_ITEM
        || _itemType == ListItemType::CONTACT_USER_ITEM) {
        const int a = static_cast<int>(110.0 * _hover_progress);
        if (a > 0) {
            p.setPen(Qt::NoPen);
            p.setBrush(QColor(79, 134, 255, a));
            p.drawRoundedRect(rect().adjusted(2, 2, -2, -2), 10, 10);
        }
    }
}

void ListItemBase::enterEvent(QEvent* event)
{
    animateHoverTo(1.0);
    QWidget::enterEvent(event);
}

void ListItemBase::leaveEvent(QEvent* event)
{
    animateHoverTo(0.0);
    QWidget::leaveEvent(event);
}

void ListItemBase::animateHoverTo(qreal target)
{
    if (_hover_anim == nullptr) {
        _hover_anim = new QVariantAnimation(this);
        _hover_anim->setEasingCurve(QEasingCurve::OutCubic);
        connect(_hover_anim, &QVariantAnimation::valueChanged, this, [this](const QVariant& v) {
            setHoverProgress(v.toReal());
        });
    }

    _hover_anim->stop();
    _hover_anim->setDuration(220);
    _hover_anim->setStartValue(_hover_progress);
    _hover_anim->setEndValue(target);
    _hover_anim->start();
}

