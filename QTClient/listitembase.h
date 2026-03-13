#ifndef LISTITEMBASE_H
#define LISTITEMBASE_H
#include <QObject>
#include <QWidget>
#include <QPaintEvent>
#include <QVariantAnimation>
#include "global.h"

class ListItemBase : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(qreal hoverProgress READ hoverProgress WRITE setHoverProgress)
public:
    explicit ListItemBase(QWidget* parent = nullptr);
    void SetItemType(ListItemType itemType);
    ListItemType GetItemType();
    qreal hoverProgress() const;
    void setHoverProgress(qreal value);

protected:
    void paintEvent(QPaintEvent* event) override;
    void enterEvent(QEvent* event) override;
    void leaveEvent(QEvent* event) override;

private:
    void animateHoverTo(qreal target);
    ListItemType _itemType;
    qreal _hover_progress;
    QVariantAnimation* _hover_anim;
public slots:

signals:

};

#endif // LISTITEMBASE_H
