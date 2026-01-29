#include "searchlist.h"
#include "qjsondocument.h"
#include "tcpmanager.h"
#include "customedit.h"
#include "adduseritem.h"
#include "findsuccessdialog.h"
#include "findfaildlg.h"
#include "usermanager.h"


SearchList::SearchList(QWidget *parent):QListWidget(parent),_find_dialog(nullptr), _search_edit(nullptr), _send_pending(false)
{
    Q_UNUSED(parent);
    this->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    this->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    // 安装事件过滤器
    this->viewport()->installEventFilter(this);
    //连接点击的信号和槽
    connect(this, &QListWidget::itemClicked, this, &SearchList::slot_item_clicked);
    //添加条目
    addTipItem();
    //连接搜索条目
    connect(TcpManager::getInstance().get(), &TcpManager::sig_user_search, this, &SearchList::slot_user_search);
}

void SearchList::CloseFindDialog()
{
    if(_find_dialog){
        _find_dialog->hide();
        _find_dialog = nullptr;
    }

}

void SearchList::SetSearchEdit(QWidget *edit)
{
    _search_edit = edit;
}

void SearchList::waitPending(bool pending)
{
    if(pending){
        _loadingDialog = new LoadingDialog(this);
        _loadingDialog->setModal(true);
        _loadingDialog->show();
        _send_pending = pending;
    } else {
        _loadingDialog->hide();
        _loadingDialog->deleteLater();
        _send_pending = pending;
    }
}

void SearchList::addTipItem()
{
    auto *invalid_item = new QWidget();
    QListWidgetItem *item_tmp = new QListWidgetItem;
    //qDebug()<<"chat_user_wid sizeHint is " << chat_user_wid->sizeHint();
    item_tmp->setSizeHint(QSize(250,10));
    this->addItem(item_tmp);
    invalid_item->setObjectName("invalid_item");
    this->setItemWidget(item_tmp, invalid_item);
    item_tmp->setFlags(item_tmp->flags() & ~Qt::ItemIsSelectable);

    auto *add_user_item = new AddUserItem();
    QListWidgetItem *item = new QListWidgetItem;
    //qDebug()<<"chat_user_wid sizeHint is " << chat_user_wid->sizeHint();
    item->setSizeHint(add_user_item->sizeHint());
    this->addItem(item);
    this->setItemWidget(item, add_user_item);
}

void SearchList::slot_item_clicked(QListWidgetItem *item)
{
    QWidget* widget = this->itemWidget(item);
    if(!widget) {
        qDebug() << "slot item clicked widget is nullptr";
        return;
    }

    ListItemBase *customItem = qobject_cast<ListItemBase*>(widget);
    if(!customItem) {
        qDebug() << "slot item clicked widget is nullptr";
        return;
    }

    auto itemType = customItem->GetItemType();
    if(itemType == ListItemType::INVALID_ITEM) {
        qDebug() << "slot invalid item clicked";
        return;
    }
    if(itemType == ListItemType::ADD_USER_TIP_ITEM) {
        if(_send_pending) {
            return;
        }
        if(!_search_edit) {
            return;
        }
        waitPending(true);
        auto search_edit = dynamic_cast<CustomEdit*>(_search_edit);
        auto uid = search_edit->text().toInt();

        QJsonObject jsonObj;
        jsonObj["uid"] = uid;
        QJsonDocument doc(jsonObj);
        QByteArray jsonData = doc.toJson(QJsonDocument::Compact);
        emit TcpManager::getInstance()->sig_send_data(ReqId::ID_SEARCH_USER_REQ, jsonData);
        return;
    }

    // 清除弹窗
    CloseFindDialog();
}

void SearchList::slot_user_search(std::shared_ptr<SearchInfo> si)
{
    waitPending(false);

    if( si == nullptr ) {
        _find_dialog.reset(new FindFailDlg(this));
    } else {
        auto self_uid = UserManager::getInstance()->GetUid();
        if(si->_uid == self_uid) {
            return;
        }
        //此处分两种情况，一种是搜多到已经是自己的朋友了，一种是未添加好友
        //查找是否已经是好友
        bool isExist = UserManager::getInstance()->CheckFriendById(si->_uid);
        if(isExist) {
            //此处处理已经添加的好友，实现页面跳转
            //跳转到聊天界面指定的item中
            emit sig_jump_chat_item(si);
            return;
        }
        _find_dialog.reset(new FindSuccessDialog(this));
        std::dynamic_pointer_cast<FindSuccessDialog>(_find_dialog)->SetSearchInfo(si);
    }
    _find_dialog->show();
}
