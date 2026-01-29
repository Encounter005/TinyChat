#include "chatuserlist.h"

ChatUserList::ChatUserList(QWidget* parent) {
    Q_UNUSED(parent);
    this->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    this->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    this->viewport()->installEventFilter(this);
}

bool ChatUserList::eventFilter(QObject *watched, QEvent *event)
{
    if(watched == this->viewport()) {
        if(event->type() == QEvent::Enter) {
            this->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        } else if(event->type() == QEvent::Leave) {
            this->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        }
    }

    if(watched == this->viewport() && event->type() == QEvent::Wheel) {
        QWheelEvent* wheelEdit = static_cast<QWheelEvent*>(event);
        int numDegrees = wheelEdit->angleDelta().y() / 8;
        int numSteps = numDegrees / 15; // 滚动步数
        // 滚动幅度
        this->verticalScrollBar()->setValue(this->verticalScrollBar()->value() - numSteps);

        // 检查是否滚动到底部
        QScrollBar* scrollBar = this->verticalScrollBar();
        int maxScrollValue = scrollBar->maximum();
        int currentValue = scrollBar->value();

        if(maxScrollValue - currentValue <= 0) {
            qDebug() << "load more chat user";
            emit sig_loading_chat_user();
        }
        return true;
    }
    return QListWidget::eventFilter(watched, event);
}
