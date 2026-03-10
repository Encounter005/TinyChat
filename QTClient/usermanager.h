#ifndef USERMANAGER_H
#define USERMANAGER_H
#include "singleton.h"
#include "userdata.h"
#include <QJsonArray>
#include <QMap>
#include <QObject>
#include <memory>
#include <vector>

class UserManager : public QObject,
                    public SingleTon<UserManager>,
                    public std::enable_shared_from_this<UserManager> {
    friend class SingleTon<UserManager>;
    Q_OBJECT
public:
    ~UserManager() = default;
    void    SetToken(QString token);
    QString GetName();
    int     GetUid();
    QString GetIcon();
    QString GetDesc();
    QString GetNick();

    std::vector<std::shared_ptr<ApplyInfo>> GetApplyList();
    bool                                    CheckFriendById(int uid);
    void                                    UpdateChatLoadedCount();
    bool                                    AlreadyApply(int uid);
    void AddApplyList(std::shared_ptr<ApplyInfo> app);
    std::vector<std::shared_ptr<FriendInfo>> GetChatListPerPage();
    void                      AddFriend(std::shared_ptr<AuthInfo> auth_info);
    void                      AddFriend(std::shared_ptr<AuthRsp> auth_rsp);
    void                      AppendApplyList(QJsonArray array);
    void                      AppendFriendList(QJsonArray array);
    std::shared_ptr<UserInfo> GetUserInfo();
    void                      SetUserInfo(std::shared_ptr<UserInfo> user_info);
    void                      AppendFriendChatMsg(
                             int friend_id, std::vector<std::shared_ptr<TextChatData>> msgs);
    std::shared_ptr<FriendInfo>              GetFriendById(int uid);
    std::vector<std::shared_ptr<FriendInfo>> GetConListPerPage();
    void                                     UpdateContactLoadedCount();
    void                                     AppendMessagesForOwner(
                                            int ownerUid, const std::vector<std::shared_ptr<TextChatData>>& msgs);
    void UpdateFriendIcon(int uid, const QString& icon);

private:
    UserManager();
    QString                                  _token;
    std::shared_ptr<UserInfo>                _user_info;
    std::vector<std::shared_ptr<ApplyInfo>>  _apply_list;
    std::vector<std::shared_ptr<FriendInfo>> _friend_list;
    QMap<int, std::shared_ptr<FriendInfo>>   _friend_map;
    int                                      _chat_loaded;
    int                                      _contact_loaded;
public slots:
    void slot_add_friend_rsp(std::shared_ptr<AuthRsp> rsp);
    void slot_add_friend_auth(std::shared_ptr<AuthInfo> auth);
};

#endif   // USERMANAGER_H
