#ifndef CONTACTUSETITEM_H
#define CONTACTUSETITEM_H

#include <QDialog>
#include <memory>
#include "userdata.h"
#include "listitembase.h"

namespace Ui {
class ContactUsetItem;
}

class ContactUsetItem : public ListItemBase
{
    Q_OBJECT

public:
    explicit ContactUsetItem(QWidget *parent = nullptr);
    ~ContactUsetItem();
    QSize sizeHint() const override;
    void SetInfo(std::shared_ptr<AuthInfo> auth_info);
    void SetInfo(std::shared_ptr<AuthRsp> auth_rsp);
    void SetInfo(int uid, QString name, QString icon);
    void ShowRedPoint(bool show = false);

private:
    Ui::ContactUsetItem *ui;
    std::shared_ptr<UserInfo> _info;
};

#endif // CONTACTUSETITEM_H
