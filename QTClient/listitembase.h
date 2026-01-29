#ifndef LISTITEMBASE_H
#define LISTITEMBASE_H
#include <QObject>
#include <QWidget>
#include <QPaintEvent>
#include "global.h"

class ListItemBase : public QWidget
{
    Q_OBJECT
public:
    explicit ListItemBase(QWidget* parent = nullptr);
    void SetItemType(ListItemType itemType);
    ListItemType GetItemType();
    virtual void painEvent(QPaintEvent* event);
private:
    ListItemType _itemType;
public slots:

signals:

};

#endif // LISTITEMBASE_H
