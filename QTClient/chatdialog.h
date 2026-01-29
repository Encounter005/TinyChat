#ifndef CHATDIALOG_H
#define CHATDIALOG_H

#include <QDialog>
#include <QList>
#include <QListWidget>
#include "global.h"
#include "statewidget.h"
#include "userdata.h"

namespace Ui {
class ChatDialog;
}

class ChatDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ChatDialog(QWidget *parent = nullptr);
    ~ChatDialog();
    void ShowSearch(bool bsearch=false);
    void addChatUserList();
    void ClearLabelState(StateWidget* lb);
    void UpdateChatMsg(std::vector<std::shared_ptr<TextChatData>> msgdata);


public slots:
    void slot_loading_chat_user();
    void slot_side_chat();
    void slot_side_contact();
    void slot_text_changed(const QString& str);
    void slot_apply_friend(std::shared_ptr<AddFriendApply> apply);
    void slot_add_auth_friend(std::shared_ptr<AuthInfo> auth_info);
    void slot_auth_rsp(std::shared_ptr<AuthRsp> auth_rsp);
    void slot_append_send_chat_msg(std::shared_ptr<TextChatData> msgdata);
    void slot_text_chat_msg(std::shared_ptr<TextChatMsg> msg);
    void slot_loading_contact_user();
    void slot_switch_apply_friend_page();
    void slot_show_search(bool show);
    void slot_jump_chat_item(std::shared_ptr<SearchInfo> si);
    void slot_item_clicked(QListWidgetItem *item);
    void slot_switch_chat_page(std::shared_ptr<UserInfo> user_info);
    void slot_focus_out();

private:
    bool eventFilter(QObject* watched, QEvent* event);
    void handleGlobalMousePress(QMouseEvent* event);
    void loadMoreConUser();
    void loadMoreChatUser();
    void SetSelectChatPage(int uid );
    void SetSelectChatItem(int uid );
    void AddLBGroup(StateWidget* );
    void CloseFindDlg();

private:
    Ui::ChatDialog *ui;
    ChatUIMode _mode;
    ChatUIMode _state;
    bool _b_loading;
    int _cur_chat_uid;
    // test
    std::vector<QString> names;
    std::vector<QString> heads;
    std::vector<QString> msgs;

    QList<StateWidget*> _lb_list;
    QWidget* _last_widget;
    QMap<int, QListWidgetItem*> _chat_items_added;
};

#endif // CHATDIALOG_H
