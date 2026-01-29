#ifndef CHATPAGE_H
#define CHATPAGE_H

#include "userdata.h"
#include <QWidget>
#include <QMap>
#include <QPaintEvent>

namespace Ui {
class ChatPage;
}

class ChatPage : public QWidget
{
    Q_OBJECT

public:
    explicit ChatPage(QWidget *parent = nullptr);
    ~ChatPage();
    virtual void paintEvent(QPaintEvent* event) override;
    void SetUserInfo(std::shared_ptr<UserInfo> user_info);
    void AppendChatMsg(std::shared_ptr<TextChatData> msg);
private:
    Ui::ChatPage *ui;
    std::shared_ptr<UserInfo> _user_info;
    QMap<QString, QWidget*> _bubble_map;
signals:
    void sig_append_send_chat_msg(std::shared_ptr<TextChatData>);
private slots:
    void on_recv_btn_clicked();
    void on_send_btn_clicked();

};

#endif // CHATPAGE_H
