#ifndef CHATUSERWIDGET_H
#define CHATUSERWIDGET_H

#include <QWidget>
#include "listitembase.h"
#include "userdata.h"

namespace Ui {
class ChatUserWidget;
}

class ChatUserWidget : public ListItemBase
{
    Q_OBJECT

public:
    explicit ChatUserWidget(QWidget *parent = nullptr);
    ~ChatUserWidget();
    QSize sizeHint() const override
    {
        return QSize(250, 70);
    }
    void SetInfo(QString name, QString head, QString msg);
    void SetInfo(std::shared_ptr<UserInfo> user_info);
    void SetInfo(std::shared_ptr<FriendInfo> friend_info);
    std::shared_ptr<UserInfo> GetUserInfo();
    void updateLastMsg(std::vector<std::shared_ptr<TextChatData>> msgs);

private:
    Ui::ChatUserWidget *ui;
    QString _name;
    QString _head;
    QString _msg;
    std::shared_ptr<UserInfo> _user_info;
};

#endif // CHATUSERWIDGET_H
