#ifndef CUSTOMEDIT_H
#define CUSTOMEDIT_H
#include <QObject>
#include <QLineEdit>
#include <QDebug>

class CustomEdit : public QLineEdit
{
    Q_OBJECT
public:
    CustomEdit(QWidget* parent = nullptr);
    void SetMaxLength(int maxLen);
protected:
    void focusOutEvent(QFocusEvent* event) override {
        // 执行失去焦点时的处理逻辑
        //qDebug() << "CustomizeEdit focusout";
        // 调用基类的focusOutEvent()方法，保证基类的行为得到执行
        QLineEdit::focusOutEvent(event);
        //发送失去焦点得信号
        emit sig_focus_out();
    }
    void mousePressEvent(QMouseEvent* event);
    int _max_len;
private:
    void limitTextLength(QString text) {
        if(_max_len <= 0) {
            return;
        }

        QByteArray byteArray = text.toUtf8();
        if(byteArray.size() > _max_len) {
            byteArray = byteArray.left(_max_len);
            this->setText(QString::fromUtf8(byteArray));
        }
    }

signals:
    void sig_focus_out();
    void sig_mouse_clicked();
};

#endif // CUSTOMEDIT_H
