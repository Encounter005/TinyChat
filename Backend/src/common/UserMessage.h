#ifndef USERMESSAGE_H_
#define USERMESSAGE_H_

#include <json/value.h>
#include <string>

static const std::string USER_UID_PREFIX   = "utoken_";
static const std::string USER_EMAIL_PREFIX = "uemail_";
static const std::string USER_BASE_PREFIX  = "user:base:";
static const std::string USER_IP_PREFIX    = "user:ip:";
static const std::string NAME_EMAIL_PREFIX = "name:email:";
static const std::string FRIEND_APPLY_PREFIX = "friend:apply:";
static const std::string FRIEND_LIST_PREFIX  = "friend:list:";
static const std::string OFFLINE_MSG_PREFIX  = "offline:msg:";

struct UserInfo {
    UserInfo()
        : name("")
        , nick("")
        , email("")
        , desc("")
        , back("")
        , icon("")
        , pwd("") {}
    std::string name;
    std::string nick;
    std::string email;
    std::string desc;
    std::string back;
    std::string icon;
    std::string pwd;
    int         uid;
    int         sex;
};

struct ApplyInfo {
    ApplyInfo(
        int uid, std::string name, std::string desc, std::string icon,
        std::string nick, int sex, int status)
        : _uid(uid)
        , _name(name)
        , _desc(desc)
        , _icon(icon)
        , _nick(nick)
        , _sex(sex)
        , _status(status) {}

    int         _uid;
    std::string _name;
    std::string _desc;
    std::string _icon;
    std::string _nick;
    int         _sex;
    int         _status;
};


class UserJsonMapper {
public:
    static Json::Value ToJson(const UserInfo& dto) {
        Json::Value v;
        v["uid"]   = dto.uid;
        v["name"]  = dto.name;
        v["email"] = dto.email;
        v["nick"]  = dto.nick;
        v["desc"]  = dto.desc;
        v["back"]  = dto.back;
        v["icon"]  = dto.icon;
        return v;
    }
    static UserInfo FromJson(const Json::Value& src) {
        UserInfo userInfo;
        userInfo.uid   = src["uid"].asInt();
        userInfo.name  = src["name"].asString();
        userInfo.email = src["email"].asString();
        userInfo.nick  = src["nick"].asString();
        userInfo.desc  = src["desc"].asString();
        userInfo.back  = src["back"].asString();
        userInfo.icon  = src["icon"].asString();
        return userInfo;
    }
};

class ApplyInfoJsonMapper {
public:
    // ApplyInfo -> Json
    static Json::Value ToJson(const ApplyInfo& dto) {
        Json::Value v;
        v["uid"]    = dto._uid;
        v["name"]   = dto._name;
        v["desc"]   = dto._desc;
        v["icon"]   = dto._icon;
        v["nick"]   = dto._nick;
        v["sex"]    = dto._sex;
        v["status"] = dto._status;
        return v;
    }

    // Json -> ApplyInfo (补充，方便你后续使用)
    static ApplyInfo FromJson(const Json::Value& src) {
        return ApplyInfo(
            src["uid"].asInt(),
            src["name"].asString(),
            src["desc"].asString(),
            src["icon"].asString(),
            src["nick"].asString(),
            src["sex"].asInt(),
            src["status"].asInt());
    }
};

#endif   // USERMESSAGE_H_
