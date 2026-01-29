#include "contactusetitem.h"
#include "ui_contactusetitem.h"

ContactUsetItem::ContactUsetItem(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::ContactUsetItem)
{
    ui->setupUi(this);
}

ContactUsetItem::~ContactUsetItem()
{
    delete ui;
}
