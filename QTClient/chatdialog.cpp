#include "chatdialog.h"
#include "contactuseritem.h"
#include "usermanager.h"
#include "ui_chatdialog.h"
#include "tcpmanager.h"
#include "chatuserwidget.h"
#include "contactuserlist.h"
#include "loadingdialog.h"
#include "customedit.h"
#include <QAction>
#include <QRandomGenerator>
#include <QLineEdit>
#include <QPixmap>

ChatDialog::ChatDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::ChatDialog),
    _mode(ChatUIMode::ChatMode),
    _state(ChatUIMode::ChatMode),
    _b_loading(false),
    _cur_chat_uid(-1),
    _last_widget(nullptr)
{
    ui->setupUi(this);
    ui->add_btn->SetState("normal", "hover", "press");
    ui->add_btn->setProperty("state", "normal");

    //search_edit settings
    QAction* searchAction =  new QAction(ui->search_edit);
    searchAction->setIcon(QIcon("./img/search.png"));
    ui->search_edit->addAction(searchAction, QLineEdit::LeadingPosition);
    ui->search_edit->setPlaceholderText(QStringLiteral("搜索"));

    // 创建一个清除动作并设置图表
    QAction* clearAction = new QAction(ui->search_edit);
    clearAction->setIcon(QIcon("./img/close_transparent.png"));
    ui->search_edit->addAction(clearAction, QLineEdit::TrailingPosition);

    connect(ui->search_edit, &QLineEdit::textChanged, [clearAction](const QString &text){
        if(!text.isEmpty()) {
            clearAction->setIcon(QIcon("./img/close_search.png"));
        } else {
            clearAction->setIcon(QIcon("./img/close_transparent.png"));
        }
    });

    // 当需要显示清除图标时，更改为实际的清除图标
    connect(clearAction, &QAction::triggered, [this, clearAction](){
        ui->search_edit->clear();
        clearAction->setIcon(QIcon("./img/close_transparent.png"));
        ui->search_edit->clearFocus();
        ShowSearch(false);
    });

    ui->search_edit->SetMaxLength(15);
    addChatUserList();

    // load user
    connect(ui->chat_user_list, &ChatUserList::sig_loading_chat_user, this, &ChatDialog::slot_loading_chat_user);
    auto user_info = UserManager::getInstance()->GetUserInfo();
    QPixmap pixmap(user_info->_icon);
    ui->side_head_label->setPixmap(pixmap);
    QPixmap scaledPixmap = pixmap.scaled(ui->side_head_label->size(), Qt::KeepAspectRatio);
    ui->side_head_label->setPixmap(scaledPixmap);
    ui->side_head_label->setScaledContents(true);

    ui->side_chat_label->setProperty("state", "normal");
    ui->side_chat_label->SetState("normal",
                                  "hover",
                                  "pressed",
                                  "selected_normal",
                                  "selected_hover",
                                  "selected_pressed");
    ui->side_contact_label->SetState("normal",
                                  "hover",
                                  "pressed",
                                  "selected_normal",
                                  "selected_hover",
                                  "selected_pressed");

    AddLBGroup(ui->side_chat_label);
    AddLBGroup(ui->side_contact_label);

    connect(ui->side_chat_label, &StateWidget::clicked, this, &ChatDialog::slot_side_chat);
    connect(ui->side_contact_label, &StateWidget::clicked, this, &ChatDialog::slot_side_contact);

    // 连接输入框搜索变化
    connect(ui->search_edit, &QLineEdit::textChanged, this, &ChatDialog::slot_text_changed);
    ShowSearch(false);

    this->installEventFilter(this);
    // 设置聊天label选中状态
    ui->side_chat_label->SetSelected(true);
    // 设置选中条目
    // SetSelectChatItem();
    // 更新聊天界面信息
    // SetSelectChatPage();

    // 连接加载联系人的信号和槽函数
    connect(ui->con_user_list, &ContactUserList::sig_loading_contact_user, this, &ChatDialog::slot_loading_contact_user);
    // 连接联系人页面点击好友申请条目的信号
    connect(ui->con_user_list, &ContactUserList::sig_switch_apply_friend_page, this, &ChatDialog::slot_switch_apply_friend_page);
    // 连接清除搜索框操作
    connect(ui->friend_apply_page, &ApplyFriendPage::sig_show_search, this, &ChatDialog::slot_show_search);
    // 连接联系人列表点击切换聊天界面信号
    connect(ui->con_user_list, &ContactUserList::sig_switch_chat_page, this, &ChatDialog::slot_switch_chat_page);
    // set Search_list
    ui->search_list->SetSearchEdit(ui->search_edit);

    // 连接申请添加好友信号
    connect(TcpManager::getInstance().get(), &TcpManager::sig_friend_apply, this, &ChatDialog::slot_apply_friend);
    // 连接认证添加好友信号
    connect(TcpManager::getInstance().get(), &TcpManager::sig_add_auth_friend, this, &ChatDialog::slot_add_auth_friend);
    // 连接自己认证回复信号
    connect(TcpManager::getInstance().get(), &TcpManager::sig_auth_rsp, this, &ChatDialog::slot_auth_rsp);
    // 连接点击联系人item发出的信号和用户信息展示槽
    //connect(ui->con_user_list, &ContactUserList::sig_switch_friend_info_page, this, &ChatDialog::slot_friend_info_page);
    // 设置中心部件为ChatPage
    // ui->stackedWidget->setCurrentWidget(ui->chat_page);
    // 连接searchlist跳转聊天信号
    connect(ui->search_list, &SearchList::sig_jump_chat_item, this, &ChatDialog::slot_jump_chat_item);
    // 连接好友信息界面发送的点击事件
    // connect(ui->friend_info_page, &FriendInfoPage::sig_jump_chat_item, this, &ChatDialog::slot_jump_chat_item_from_infopage);
    // 连接聊天列表点击信号
    connect(ui->chat_user_list, &QListWidget::itemClicked, this, &ChatDialog::slot_item_clicked);
    // 连接对端通知
    connect(TcpManager::getInstance().get(), &TcpManager::sig_text_chat_msg, this, &ChatDialog::slot_text_chat_msg);
    connect(ui->chat_page, &ChatPage::sig_append_send_chat_msg, this, &ChatDialog::slot_append_send_chat_msg);


    // connect(ui->search_edit, &QLineEdit::textChanged, this, &ChatDialog::slot_text_changed);
    // connect(ui->search_edit, &CustomEdit::sig_mouse_clicked, this, [this](){
    //     QString text = ui->search_edit->text();
    //     qDebug() << "text is: " << text;
    //     slot_text_changed(text);
    // });
    //test data
    msgs = std::vector<QString>{
                                "hello world !",
                                "nice to meet u",
                                "New year，new life",
                                "You have to love yourself",
                                "My love is written in the wind ever since the whole world is you"
    };

    names = std::vector<QString> {
                                 "llfc",
                                 "zack",
                                 "golang",
                                 "cpp",
                                 "java",
                                 "nodejs",
                                 "python",
                                 "rust"
    };

    heads = std::vector<QString> {
                                 "./img/head_1.jpg",
                                 "./img/head_2.jpg",
                                 "./img/head_3.jpg",
                                 "./img/head_4.jpg",
                                 "./img/head_5.jpg"
    };

}

ChatDialog::~ChatDialog()
{
    delete ui;
}

void ChatDialog::ShowSearch(bool bsearch)
{
    if(bsearch){
        ui->chat_user_list->hide();
        ui->con_user_list->hide();
        ui->search_list->show();
        _mode = ChatUIMode::SearchMode;
    }else if(_state == ChatUIMode::ChatMode){
        ui->chat_user_list->show();
        ui->con_user_list->hide();
        ui->search_list->hide();
        _mode = ChatUIMode::ChatMode;
        ui->search_list->CloseFindDialog();
        ui->search_edit->clear();
        ui->search_edit->clearFocus();
    }else if(_state == ChatUIMode::ContactMode){
        ui->chat_user_list->hide();
        ui->search_list->hide();
        ui->con_user_list->show();
        _mode = ChatUIMode::ContactMode;
        ui->search_list->CloseFindDialog();
        ui->search_edit->clear();
        ui->search_edit->clearFocus();
    }else if(_state == ChatUIMode::SettingsMode){
        ui->chat_user_list->hide();
        ui->search_list->hide();
        ui->con_user_list->show();
        _mode = ChatUIMode::ContactMode;
        ui->search_list->CloseFindDialog();
        ui->search_edit->clear();
        ui->search_edit->clearFocus();
    }

}

void ChatDialog::addChatUserList()
{
    //先按照好友列表加载聊天记录，等以后客户端实现聊天记录数据库之后再按照最后信息排序
    auto friend_list = UserManager::getInstance()->GetChatListPerPage();
    if (friend_list.empty() == false) {
        qDebug() << "friend_list is not empty";
        for(auto & friend_ele : friend_list){
            auto find_iter = _chat_items_added.find(friend_ele->_uid);
            if(find_iter != _chat_items_added.end()){
                continue;
            }
            auto *chat_user_wid = new ChatUserWidget();
            auto user_info = std::make_shared<UserInfo>(friend_ele);
            chat_user_wid->SetInfo(user_info);
            QListWidgetItem *item = new QListWidgetItem;
            //qDebug()<<"chat_user_wid sizeHint is " << chat_user_wid->sizeHint();
            item->setSizeHint(chat_user_wid->sizeHint());
            ui->chat_user_list->addItem(item);
            ui->chat_user_list->setItemWidget(item, chat_user_wid);
            _chat_items_added.insert(friend_ele->_uid, item);
        }

        //更新已加载条目
        UserManager::getInstance()->UpdateChatLoadedCount();
    }

    // for(int i = 0; i < 50; ++i) {
    //     int randomvalue = QRandomGenerator::global()->bounded(100);
    //     int msg_i = randomvalue % msgs.size();
    //     int head_i = randomvalue % heads.size();
    //     int name_i = randomvalue % names.size();
    //     auto* chat_user_widget = new ChatUserWidget();
    //     chat_user_widget->SetInfo(names[name_i], heads[head_i], msgs[msg_i]);
    //     QListWidgetItem* item = new QListWidgetItem;
    //     item->setSizeHint(chat_user_widget->sizeHint());
    //     ui->chat_user_list->addItem(item);
    //     ui->chat_user_list->setItemWidget(item, chat_user_widget);
    // }
}

void ChatDialog::AddLBGroup(StateWidget *lb)
{
    _lb_list.push_back(lb);
}

void ChatDialog::CloseFindDlg()
{
    ui->search_list->CloseFindDialog();
}

void ChatDialog::ClearLabelState(StateWidget *lb)
{
    for(auto& elm : _lb_list) {
        if(elm == lb ) continue;
        elm->ClearState();
    }
}

void ChatDialog::UpdateChatMsg(std::vector<std::shared_ptr<TextChatData> > msgdata)
{
    for(auto & msg : msgdata){
        if(msg->_from_uid != _cur_chat_uid){
            break;
        }

        ui->chat_page->AppendChatMsg(msg);
    }

}

void ChatDialog::SetSelectChatPage( int uid )
{
    qDebug() << "uid is: " << uid;
    if( ui->chat_user_list->count() <= 0){
        return;
    }

    if (uid == 0) {
        auto item = ui->chat_user_list->item(0);
        //转为widget
        QWidget* widget = ui->chat_user_list->itemWidget(item);
        if (!widget) {
            return;
        }

        auto con_item = qobject_cast<ChatUserWidget*>(widget);
        if (!con_item) {
            return;
        }

        //设置信息
        auto user_info = con_item->GetUserInfo();
        ui->chat_page->SetUserInfo(user_info);
        return;
    }

    auto find_iter = _chat_items_added.find(uid);
    if(find_iter == _chat_items_added.end()){
        return;
    }

    //转为widget
    QWidget *widget = ui->chat_user_list->itemWidget(find_iter.value());
    if(!widget){
        return;
    }

    //判断转化为自定义的widget
    // 对自定义widget进行操作， 将item 转化为基类ListItemBase
    ListItemBase *customItem = qobject_cast<ListItemBase*>(widget);
    if(!customItem){
        qDebug()<< "qobject_cast<ListItemBase*>(widget) is nullptr";
        return;
    }

    auto itemType = customItem->GetItemType();
    if(itemType == CHAT_USER_ITEM){
        auto con_item = qobject_cast<ChatUserWidget*>(customItem);
        if(!con_item){
            return;
        }

        //设置信息
        auto user_info = con_item->GetUserInfo();
        ui->chat_page->SetUserInfo(user_info);

        return;
    }
}

void ChatDialog::SetSelectChatItem( int uid )
{
    if(ui->chat_user_list->count() <= 0){
        return;
    }

    if(uid == 0){
        ui->chat_user_list->setCurrentRow(0);
        QListWidgetItem *firstItem = ui->chat_user_list->item(0);
        if(!firstItem){
            qDebug("this user doesn't have friend ;-;");
            return;
        }

        //转为widget
        QWidget *widget = ui->chat_user_list->itemWidget(firstItem);
        if(!widget){
            return;
        }

        auto con_item = qobject_cast<ChatUserWidget*>(widget);
        if(!con_item){
            return;
        }

        _cur_chat_uid = con_item->GetUserInfo()->_uid;

        return;
    }

    auto find_iter = _chat_items_added.find(uid);
    if(find_iter == _chat_items_added.end()){
        qDebug() << "uid " <<uid<< " not found, set curent row 0";
        ui->chat_user_list->setCurrentRow(0);
        return;
    }

    ui->chat_user_list->setCurrentItem(find_iter.value());

    _cur_chat_uid = uid;

}



void ChatDialog::slot_loading_chat_user()
{
    if(_b_loading) {
        return;
    }

    _b_loading = true;
    LoadingDialog* loadingDialog = new LoadingDialog(this);
    loadingDialog->setModal(true);
    loadingDialog->show();
    qDebug()<< "add new user data to list ....";
    loadMoreChatUser();
    loadingDialog->deleteLater();
    _b_loading = false;
}

void ChatDialog::slot_side_chat()
{
    qDebug()<< "receive side chat clicked";
    ClearLabelState(ui->side_chat_label);
    ui->stackedWidget->setCurrentWidget(ui->chat_page);
    _state = ChatUIMode::ChatMode;
    ShowSearch(false);

}

void ChatDialog::slot_side_contact()
{
    qDebug() << "receive side contact clicked";
    ClearLabelState(ui->side_contact_label);
    if(_last_widget == nullptr) {
        ui->stackedWidget->setCurrentWidget(ui->friend_apply_page);
        _last_widget = ui->friend_apply_page;
    } else {
        ui->stackedWidget->setCurrentWidget(_last_widget);
    }
    _state = ChatUIMode::ContactMode;
    ShowSearch(false);
}

void ChatDialog::slot_text_changed(const QString& str) {
    if(!str.isEmpty()) {
        ShowSearch(true);
    }
}

void ChatDialog::slot_apply_friend(std::shared_ptr<AddFriendApply> apply)
{
    qDebug() << "receive apply friend slot, applyuid is " << apply->_from_uid << " name is "
             << apply->_name << " desc is " << apply->_desc;

    bool b_already = UserManager::getInstance()->AlreadyApply(apply->_from_uid);
    if(b_already){
        return;
    }

    UserManager::getInstance()->AddApplyList(std::make_shared<ApplyInfo>(apply));
    ui->side_contact_label->ShowRedPoint(true);
    ui->con_user_list->ShowRedPoint(true);
    ui->friend_apply_page->AddNewApply(apply);

}

void ChatDialog::slot_add_auth_friend(std::shared_ptr<AuthInfo> auth_info)
{
    qDebug() << "receive slot_add_auth__friend uid is " << auth_info->_uid
             << " name is " << auth_info->_name << " nick is " << auth_info->_nick;
    //判断如果已经是好友则跳过
    auto bfriend = UserManager::getInstance()->CheckFriendById(auth_info->_uid);
    if(bfriend){
        return;
    }
    UserManager::getInstance()->AddFriend(auth_info);
    int randomValue = QRandomGenerator::global()->bounded(100); // 生成0到99之间的随机整数
    int str_i = randomValue % strs.size();
    int head_i = randomValue % heads.size();
    int name_i = randomValue % names.size();
    auto* chat_user_wid = new ChatUserWidget();
    auto user_info = std::make_shared<UserInfo>(auth_info);
    chat_user_wid->SetInfo(user_info);
    QListWidgetItem* item = new QListWidgetItem;
    //qDebug()<<"chat_user_wid sizeHint is " << chat_user_wid->sizeHint();
    item->setSizeHint(chat_user_wid->sizeHint());
    ui->chat_user_list->insertItem(0, item);
    ui->chat_user_list->setItemWidget(item, chat_user_wid);
    _chat_items_added.insert(auth_info->_uid, item);
}

void ChatDialog::slot_auth_rsp(std::shared_ptr<AuthRsp> auth_rsp)
{
    qDebug() << "receive slot_auth_rsp uid is " << auth_rsp->_uid
             << " name is " << auth_rsp->_name << " nick is " << auth_rsp->_nick;
    //判断如果已经是好友则跳过
    auto bfriend = UserManager::getInstance()->CheckFriendById(auth_rsp->_uid);
    if(bfriend){
        return;
    }
    UserManager::getInstance()->AddFriend(auth_rsp);
    int randomValue = QRandomGenerator::global()->bounded(100); // 生成0到99之间的随机整数
    int str_i = randomValue % strs.size();
    int head_i = randomValue % heads.size();
    int name_i = randomValue % names.size();
    auto* chat_user_wid = new ChatUserWidget();
    auto user_info = std::make_shared<UserInfo>(auth_rsp);
    chat_user_wid->SetInfo(user_info);
    QListWidgetItem* item = new QListWidgetItem;
    //qDebug()<<"chat_user_wid sizeHint is " << chat_user_wid->sizeHint();
    item->setSizeHint(chat_user_wid->sizeHint());
    ui->chat_user_list->insertItem(0, item);
    ui->chat_user_list->setItemWidget(item, chat_user_wid);
    _chat_items_added.insert(auth_rsp->_uid, item);
}

void ChatDialog::slot_append_send_chat_msg(std::shared_ptr<TextChatData> msgdata)
{
    if (_cur_chat_uid == 0) {
        return;
    }

    auto find_iter = _chat_items_added.find(_cur_chat_uid);
    if (find_iter == _chat_items_added.end()) {
        return;
    }

    //转为widget
    QWidget* widget = ui->chat_user_list->itemWidget(find_iter.value());
    if (!widget) {
        return;
    }

    //判断转化为自定义的widget
    // 对自定义widget进行操作， 将item 转化为基类ListItemBase
    ListItemBase* customItem = qobject_cast<ListItemBase*>(widget);
    if (!customItem) {
        qDebug() << "qobject_cast<ListItemBase*>(widget) is nullptr";
        return;
    }

    auto itemType = customItem->GetItemType();
    if (itemType == CHAT_USER_ITEM) {
        auto con_item = qobject_cast<ChatUserWidget*>(customItem);
        if (!con_item) {
            return;
        }

        //设置信息
        auto user_info = con_item->GetUserInfo();
        user_info->_chat_msgs.push_back(msgdata);
        std::vector<std::shared_ptr<TextChatData>> msg_vec;
        msg_vec.push_back(msgdata);
        UserManager::getInstance()->AppendFriendChatMsg(_cur_chat_uid,msg_vec);
        return;
    }

}

void ChatDialog::slot_text_chat_msg(std::shared_ptr<TextChatMsg> msg)
{
    auto find_iter = _chat_items_added.find(msg->_from_uid);
    if(find_iter != _chat_items_added.end()){
        qDebug() << "set chat item msg, uid is " << msg->_from_uid;
        QWidget *widget = ui->chat_user_list->itemWidget(find_iter.value());
        auto chat_wid = qobject_cast<ChatUserWidget*>(widget);
        if(!chat_wid){
            return;
        }
        chat_wid->updateLastMsg(msg->_chat_msgs);
        //更新当前聊天页面记录
        UpdateChatMsg(msg->_chat_msgs);
        UserManager::getInstance()->AppendFriendChatMsg(msg->_from_uid,msg->_chat_msgs);
        return;
    }

    //如果没找到，则创建新的插入listwidget

    auto* chat_user_wid = new ChatUserWidget();
    //查询好友信息
    auto fi_ptr = UserManager::getInstance()->GetFriendById(msg->_from_uid);
    chat_user_wid->SetInfo(fi_ptr);
    QListWidgetItem* item = new QListWidgetItem;
    //qDebug()<<"chat_user_wid sizeHint is " << chat_user_wid->sizeHint();
    item->setSizeHint(chat_user_wid->sizeHint());
    chat_user_wid->updateLastMsg(msg->_chat_msgs);
    UserManager::getInstance()->AppendFriendChatMsg(msg->_from_uid,msg->_chat_msgs);
    ui->chat_user_list->insertItem(0, item);
    ui->chat_user_list->setItemWidget(item, chat_user_wid);
    _chat_items_added.insert(msg->_from_uid, item);

}

void ChatDialog::slot_loading_contact_user()
{
    qDebug() << "slot loading contact user";
    if(_b_loading){
        return;
    }

    _b_loading = true;
    LoadingDialog *loadingDialog = new LoadingDialog(this);
    loadingDialog->setModal(true);
    loadingDialog->show();
    qDebug() << "add new data to list.....";
    loadMoreConUser();
    // 加载完成后关闭对话框
    loadingDialog->deleteLater();

    _b_loading = false;

}

void ChatDialog::slot_switch_apply_friend_page()
{
    qDebug()<<"receive switch apply friend page sig";
    _last_widget = ui->friend_apply_page;
    ui->stackedWidget->setCurrentWidget(ui->friend_apply_page);

}

void ChatDialog::slot_show_search(bool show)
{
    ShowSearch(show);

}

void ChatDialog::slot_jump_chat_item(std::shared_ptr<SearchInfo> si)
{
    qDebug() << "slot jump chat item " << endl;
    auto find_iter = _chat_items_added.find(si->_uid);
    if(find_iter != _chat_items_added.end()){
        qDebug() << "jump to chat item , uid is " << si->_uid;
        ui->chat_user_list->scrollToItem(find_iter.value());
        ui->side_chat_label->SetSelected(true);
        SetSelectChatItem(si->_uid);
        //更新聊天界面信息
        SetSelectChatPage(si->_uid);
        slot_side_chat();
        return;
    }

    //如果没找到，则创建新的插入listwidget

    auto* chat_user_wid = new ChatUserWidget();
    auto user_info = std::make_shared<UserInfo>(si);
    chat_user_wid->SetInfo(user_info);
    QListWidgetItem* item = new QListWidgetItem;
    //qDebug()<<"chat_user_wid sizeHint is " << chat_user_wid->sizeHint();
    item->setSizeHint(chat_user_wid->sizeHint());
    ui->chat_user_list->insertItem(0, item);
    ui->chat_user_list->setItemWidget(item, chat_user_wid);

    _chat_items_added.insert(si->_uid, item);

    ui->side_chat_label->SetSelected(true);
    SetSelectChatItem(si->_uid);
    //更新聊天界面信息
    SetSelectChatPage(si->_uid);
    slot_side_chat();
}

void ChatDialog::slot_item_clicked(QListWidgetItem *item)
{
    QWidget *widget = ui->chat_user_list->itemWidget(item); // 获取自定义widget对象
    if(!widget){
        qDebug()<< "slot item clicked widget is nullptr";
        return;
    }


    // 对自定义widget进行操作， 将item 转化为基类ListItemBase
    ListItemBase *customItem = qobject_cast<ListItemBase*>(widget);
    if(!customItem){
        qDebug()<< "slot item clicked widget is nullptr";
        return;
    }

    auto itemType = customItem->GetItemType();
    if(itemType == ListItemType::INVALID_ITEM
        || itemType == ListItemType::GROUP_TIP_ITEM){
        qDebug()<< "slot invalid item clicked ";
        return;
    }


    if(itemType == ListItemType::CHAT_USER_ITEM){
        // 创建对话框，提示用户
        qDebug()<< "contact user item clicked ";

        auto chat_wid = qobject_cast<ChatUserWidget*>(customItem);
        auto user_info = chat_wid->GetUserInfo();
        //跳转到聊天界面
        ui->chat_page->SetUserInfo(user_info);
        _cur_chat_uid = user_info->_uid;
        SetSelectChatPage(user_info->_uid);
        return;
    }

}

void ChatDialog::slot_switch_chat_page(std::shared_ptr<UserInfo> user_info)
{
    auto find_iter = _chat_items_added.find(user_info->_uid);
    if(find_iter != _chat_items_added.end()){
        qDebug() << "jump to chat item , uid is " << user_info->_uid;
        ui->chat_user_list->scrollToItem(find_iter.value());
        ui->side_chat_label->SetSelected(true);
        SetSelectChatItem(user_info->_uid);
        //更新聊天界面信息
        SetSelectChatPage(user_info->_uid);
        slot_side_chat();
        return;
    }

    //如果没找到，则创建新的插入listwidget
    auto* chat_user_wid = new ChatUserWidget();
    chat_user_wid->SetInfo(user_info);
    QListWidgetItem* item = new QListWidgetItem;
    //qDebug()<<"chat_user_wid sizeHint is " << chat_user_wid->sizeHint();
    item->setSizeHint(chat_user_wid->sizeHint());
    ui->chat_user_list->insertItem(0, item);
    ui->chat_user_list->setItemWidget(item, chat_user_wid);

    _chat_items_added.insert(user_info->_uid, item);

    ui->side_chat_label->SetSelected(true);
    SetSelectChatItem(user_info->_uid);
    //更新聊天界面信息
    SetSelectChatPage(user_info->_uid);
    slot_side_chat();
}

void ChatDialog::slot_focus_out()
{
    qDebug()<< "receive focus out signal";
    ShowSearch(false);
}

bool ChatDialog::eventFilter(QObject *watched, QEvent *event)
{
    if(event->type() == QEvent::MouseButtonPress) {
        QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
        handleGlobalMousePress(mouseEvent);
    }
    return QDialog::eventFilter(watched, event);
}

void ChatDialog::handleGlobalMousePress(QMouseEvent *event)
{
    // 实现点击位置的判断和处理逻辑
    // 先判断是否处于搜索模式，如果不处于搜索模式则直接返回
    if(_mode != ChatUIMode::SearchMode) {
        return;
    }
    QPoint posInSearchList = ui->search_list->mapFromGlobal(event->globalPos());
    if(!ui->search_list->rect().contains(posInSearchList)) {
        // 如果不再聊天列表内，清空输入框
        ui->search_list->clear();
        ShowSearch(false);
    }
}

void ChatDialog::loadMoreConUser()
{
    auto friend_list = UserManager::getInstance()->GetConListPerPage();
    if (friend_list.empty() == false) {
        for(auto & friend_ele : friend_list){
            auto *chat_user_wid = new ContactUserItem();
            chat_user_wid->SetInfo(friend_ele->_uid,friend_ele->_name,
                                   friend_ele->_icon);
            QListWidgetItem *item = new QListWidgetItem;
            //qDebug()<<"chat_user_wid sizeHint is " << chat_user_wid->sizeHint();
            item->setSizeHint(chat_user_wid->sizeHint());
            ui->con_user_list->addItem(item);
            ui->con_user_list->setItemWidget(item, chat_user_wid);
        }

        //更新已加载条目
        UserManager::getInstance()->UpdateContactLoadedCount();
    }

}

void ChatDialog::loadMoreChatUser()
{
    auto friend_list = UserManager::getInstance()->GetChatListPerPage();
    if (friend_list.empty() == false) {
        for(auto & friend_ele : friend_list){
            auto find_iter = _chat_items_added.find(friend_ele->_uid);
            if(find_iter != _chat_items_added.end()){
                continue;
            }
            auto *chat_user_wid = new ChatUserWidget();
            auto user_info = std::make_shared<UserInfo>(friend_ele);
            chat_user_wid->SetInfo(user_info);
            QListWidgetItem *item = new QListWidgetItem;
            //qDebug()<<"chat_user_wid sizeHint is " << chat_user_wid->sizeHint();
            item->setSizeHint(chat_user_wid->sizeHint());
            ui->chat_user_list->addItem(item);
            ui->chat_user_list->setItemWidget(item, chat_user_wid);
            _chat_items_added.insert(friend_ele->_uid, item);
        }

        //更新已加载条目
        UserManager::getInstance()->UpdateChatLoadedCount();
    }

}

