#include "customedit.h"
#include <QMouseEvent>

CustomEdit::CustomEdit(QWidget* parent) :QLineEdit(parent), _max_len(0) {
    connect(this, &QLineEdit::textChanged, this, &CustomEdit::limitTextLength);
}

void CustomEdit::SetMaxLength(int maxLen)
{
    _max_len = maxLen;
}

void CustomEdit::mousePressEvent(QMouseEvent *event)
{
    if(event->button() == Qt::LeftButton) {
        emit sig_mouse_clicked();
    }
    // 关键：一定要调用基类
    QLineEdit::mousePressEvent(event);
}


