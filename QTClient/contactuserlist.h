#ifndef CONTACTUSERLIST_H
#define CONTACTUSERLIST_H

#include "userdata.h"
#include <QListWidget>
#include <QObject>
#include <QEvent>
#include <memory>
#include <QWheelEvent>
#include <QScrollEvent>
#include <QScrollBar>


class ContactUserItem;

class ContactUserList : public QListWidget
{
    Q_OBJECT
public:
    ContactUserList(QWidget* parent = nullptr);
    void ShowRedPoint(bool bshow = true);
protected:
    bool eventFilter(QObject* watched, QEvent* event) override;
private:
    void addContactUserList();
public slots:
    void slot_item_clicked(QListWidgetItem* item);
    void slot_auth_rsp(std::shared_ptr<AuthRsp> auth_rsp);
    void slot_add_auth_firend(std::shared_ptr<AuthInfo>);

signals:
    void sig_loading_contact_user();
    void sig_switch_apply_friend_page();
    void sig_switch_friend_info_page();
    void sig_switch_chat_page(std::shared_ptr<UserInfo> user_info);
private:
    ContactUserItem* _add_friend_item;
    QListWidgetItem* _group_item;
};

#endif // CONTACTUSERLIST_H
