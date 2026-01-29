#ifndef APPLYFRIENDPAGE_H
#define APPLYFRIENDPAGE_H

#include <QWidget>
#include <unordered_map>
#include "userdata.h"

namespace Ui {
class ApplyFriendPage;
}

class ApplyFriendItem;

class ApplyFriendPage : public QWidget
{
    Q_OBJECT

public:
    explicit ApplyFriendPage(QWidget *parent = nullptr);
    ~ApplyFriendPage();
    void AddNewApply(std::shared_ptr<AddFriendApply> apply);
public slots:
    void slot_auth_rsp(std::shared_ptr<AuthRsp>);
signals:
    void sig_show_search(bool);
protected:
    void painEvent(QPaintEvent* event);

private:
    void loadApplyList();
private:
    Ui::ApplyFriendPage *ui;
    std::unordered_map<int, ApplyFriendItem*> _unauth_items;
};

#endif // APPLYFRIENDPAGE_H
