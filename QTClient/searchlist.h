#ifndef SEARCHLIST_H
#define SEARCHLIST_H

#include <QObject>
#include <QWidget>
#include <QEvent>
#include <QListWidget>
#include <QWheelEvent>
#include <QScrollEvent>
#include <QScrollBar>
#include "loadingdialog.h"
#include "userdata.h"

class SearchList : public QListWidget
{
    Q_OBJECT
public:
    SearchList(QWidget* parent = nullptr);
    void CloseFindDialog();
    void SetSearchEdit(QWidget* edit);
protected:
    bool eventFilter(QObject* watched, QEvent* event) override {
        if(watched == this->viewport()) {
            if(event->type() == QEvent::Enter) {
                this->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
            } else if(event->type() == QEvent::Leave) {
                this->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
            }
        }
        // 检查事件是否是鼠标滚轮事件
        if (watched == this->viewport() && event->type() == QEvent::Wheel) {
            QWheelEvent *wheelEvent = static_cast<QWheelEvent*>(event);
            int numDegrees = wheelEvent->angleDelta().y() / 8;
            int numSteps = numDegrees / 15; // 计算滚动步数
            // 设置滚动幅度
            this->verticalScrollBar()->setValue(this->verticalScrollBar()->value() - numSteps);
            return true; // 停止事件传递
        }
        return QListWidget::eventFilter(watched, event);
    }
private:
    void waitPending(bool pending = true);
    bool _send_pending;
    void addTipItem();
    std::shared_ptr<QDialog> _find_dialog;
    QWidget* _search_edit;
    LoadingDialog* _loadingDialog;
private slots:
    void slot_item_clicked(QListWidgetItem* item);
    void slot_user_search(std::shared_ptr<SearchInfo> si);
signals:
    void sig_jump_chat_item(std::shared_ptr<SearchInfo> si);
};

#endif // SEARCHLIST_H
