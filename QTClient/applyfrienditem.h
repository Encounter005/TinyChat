#ifndef APPLYFRIENDITEM_H
#define APPLYFRIENDITEM_H

#include <QWidget>
#include "listitembase.h"
#include "userdata.h"

namespace Ui {
class ApplyFriendItem;
}

class ApplyFriendItem : public ListItemBase
{
    Q_OBJECT

public:
    explicit ApplyFriendItem(QWidget *parent = nullptr);
    ~ApplyFriendItem();
    void SetInfo(std::shared_ptr<ApplyInfo> apply_info);
    void ShowAddBtn(bool show);
    QSize sizeHint() const override;
    int GetUid();
signals:
    void sig_auth_friend(std::shared_ptr<ApplyInfo> apply_info);


private:
    Ui::ApplyFriendItem *ui;
    bool _added;
    std::shared_ptr<ApplyInfo> _apply_info;
};

#endif // APPLYFRIENDITEM_H
