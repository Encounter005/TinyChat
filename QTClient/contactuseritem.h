#ifndef CONTACTUSERITEM_H
#define CONTACTUSERITEM_H

#include <QWidget>
#include "listitembase.h"
#include "userdata.h"

namespace Ui {
class ContactUserItem;
}

class ContactUserItem : public ListItemBase
{
    Q_OBJECT

public:
    explicit ContactUserItem(QWidget *parent = nullptr);
    ~ContactUserItem();
    QSize sizeHint() const override;
    void SetInfo(std::shared_ptr<AuthInfo> auth_info);
    void SetInfo(std::shared_ptr<AuthRsp> auth_rsp);
    void SetInfo(int uid, QString name, QString icon);
    void ShowRedPoint(bool show = false);
    std::shared_ptr<UserInfo> GetUserInfo();

private:
    Ui::ContactUserItem *ui;
    std::shared_ptr<UserInfo> _info;
};

#endif // CONTACTUSERITEM_H
