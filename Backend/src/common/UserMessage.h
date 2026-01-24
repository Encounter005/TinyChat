#ifndef USERMESSAGE_H_
#define USERMESSAGE_H_

#include <string>
#include <json/value.h>

static const std::string USER_UID_PREFIX = "utoken_";
static const std::string USER_EMAIL_PREFIX = "uemail_";
static const std::string USER_BASE_PREFIX = "user:base:";
static const std::string USER_IP_PREFIX = "user:ip";

struct UserInfo {
    UserInfo() : name(""), nick(""), email(""), desc(""), back(""), icon(""), pwd("") {}
    std::string name;
    std::string nick;
    std::string email;
    std::string desc;
    std::string back;
    std::string icon;
    std::string pwd;
    int uid;
    int sex;
};


class UserJsonMapper {
public:
    static Json::Value ToJson(const UserInfo& dto) {
        Json::Value v;
        v["uid"]   = dto.uid;
        v["name"]  = dto.name;
        v["email"] = dto.email;
        v["nick"] = dto.nick;
        v["desc"] = dto.desc;
        v["back"] = dto.back;
        v["icon"] = dto.icon;
        return v;
    }
    static UserInfo FromJson(const Json::Value& src) {
        UserInfo userInfo;
        userInfo.uid = src["uid"].asInt();
        userInfo.name = src["name"].asString();
        userInfo.email = src["email"].asString();
        userInfo.nick = src["nick"].asString();
        userInfo.desc = src["desc"].asString();
        userInfo.back = src["back"].asString();
        userInfo.icon = src["icon"].asString();
        return userInfo;
    }


};


#endif // USERMESSAGE_H_
