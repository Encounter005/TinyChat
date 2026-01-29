#include "friendlabel.h"
#include "ui_friendlabel.h"
#include <QFont>
#include <QFontMetrics>



FriendLabel::~FriendLabel()
{
    delete ui;
}


FriendLabel::FriendLabel(QWidget *parent) : QFrame(parent), ui(new Ui::FriendLabel)
{
    ui->setupUi(this);
    ui->close_lb->SetState("normal",
                           "hover",
                           "pressed",
                           "selected_normal",
                           "selected_hover",
                           "selected_pressed");
    connect(ui->close_lb, &ClickedLabel::clicked, this, &FriendLabel::slot_close);
}



void FriendLabel::SetText(QString text)
{
    _text = text;
    ui->tip_lb->setText(_text);
    ui->tip_lb->adjustSize();
    QFontMetrics fontMetrics(ui->tip_lb->font());
    auto textWidth = fontMetrics.horizontalAdvance(ui->tip_lb->text());
    auto textHeight = fontMetrics.height();
    qDebug() << "ui->tip_lb.lineWidth() is: " << textWidth;
    qDebug() << "ui->tip_lb.height() is: " << textHeight;
    this->setFixedHeight(textHeight + 2);
    this->setFixedWidth(textWidth + ui->close_lb->width() + 20);
    qDebug() << "this->setFixedHeight is: " << this->height();
    _width = this->width();
    _height = this->height();
}



int FriendLabel::Width()
{
    return _width;
}



int FriendLabel::Height()
{
    return _height;
}



QString FriendLabel::Text()
{
    return _text;
}



void FriendLabel::slot_close()
{
    emit sig_close(_text);
}

