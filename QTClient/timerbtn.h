#ifndef TIMEBTN_H
#define TIMEBTN_H

#include <QWidget>
#include <QPushButton>
#include <QTimer>


class TimerBtn : public QPushButton
{
public:
    TimerBtn(QWidget *parent = nullptr) : QPushButton(parent), _counter(10)
    {
        _timer = new QTimer(this);
        connect(_timer, &QTimer::timeout, [this](){
            _counter--;
            if(_counter <= 0) {
                _timer->stop();
                _counter = 10;
                this->setText("获取");
                this->setEnabled(true);
                return;
            }
            this->setText(QString::number(_counter));
        });
    }
    ~TimerBtn();
    virtual void mouseReleaseEvent(QMouseEvent* e) override;


private:
    QTimer* _timer;
    int _counter;

};

#endif // TIMEBTN_H
