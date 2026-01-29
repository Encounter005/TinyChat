#ifndef GLOBAL_H
#define GLOBAL_H

#include <QWidget>
#include <functional>
#include <memory>
#include <QObject>
#include <QMap>
#include <QJsonObject>
#include "QStyle"
#include "QJsonObject"

/*
* @brief repolish 刷新qss样式
*/
extern std::function<void(QWidget*)> repolish;
extern std::function<QString(QString)> xorString;
extern std::function<QString(QString)> toMD5;
extern QString gate_url_prefix;
const int tip_offset = 5;
const int MIN_APPLY_LABEL_ED_LEN = 40;
const QString add_prefix = "添加标签";
enum ReqId{
    ID_GET_VARIFY_CODE = 101, //获取验证码
    ID_REG_USER = 102, //注册用户
    ID_RESET_PWD = 103, //重置密码
    ID_LOGIN_USER = 104, //用户登录
    ID_CHAT_LOGIN_REQ = 105, //登陆聊天服务器
    ID_CHAT_LOGIN_RSP= 106, //登陆聊天服务器回包
    ID_SEARCH_USER_REQ = 107, //用户搜索请求
    ID_SEARCH_USER_RSP = 108, //搜索用户回包
    ID_ADD_FRIEND_REQ = 109,  //添加好友申请
    ID_ADD_FRIEND_RSP = 110, //申请添加好友回复
    ID_NOTIFY_ADD_FRIEND_REQ = 111,  //通知用户添加好友申请
    ID_AUTH_FRIEND_REQ = 113,  //认证好友请求
    ID_AUTH_FRIEND_RSP = 114,  //认证好友回复
    ID_NOTIFY_AUTH_FRIEND_REQ = 115, //通知用户认证好友申请
    ID_TEXT_CHAT_MSG_REQ  = 117,  //文本聊天信息请求
    ID_TEXT_CHAT_MSG_RSP  = 118,  //文本聊天信息回复
    ID_NOTIFY_TEXT_CHAT_MSG_REQ = 119, //通知用户文本聊天信息
    ID_NOTIFY_OFF_LINE_REQ = 120, //通知用户下线
    ID_HEART_BEAT_REQ = 121,      //心跳请求
    ID_HEARTBEAT_RSP = 122,       //心跳回复
};


enum ErrorCodes {
    SUCCESS = 0,
    ERROR_JSON = 1, // Json解析错误
    ERROR_NETWORK = 2, // 网络错误
};

enum Modules {
    REGISTERMOD = 0,
    RESETMOD = 1,
    LOGINMOD = 2,
};

enum TipErr {
    TIP_SUCCESS = 0,
    TIP_EMAIL_ERR = 1,
    TIP_PWD_ERR = 2,
    TIP_CONFIRM_ERR = 3,
    TIP_PWD_CONFIRM_ERR = 4,
    TIP_VARIFY_ERR = 5,
    TIP_USER_ERR = 6,

};

// 点击状态
enum ClickLbState{
    Normal = 0,
    Selected = 1
};

struct ServerInfo {
    QString Host;
    QString Port;
    QString Token;
    int Uid;
};

// 聊天界面几种模式
enum ChatUIMode {
    SearchMode, // 搜索模就式
    ChatMode, // 聊天模式
    ContactMode, // 联系模式
    SettingsMode, // 设置模式
};

struct MsgInfo {
    QString msgFlag; // text, image, file
    QString content; // 表示文件和图像的url信息
    QString pixmap; // 文件和图像的缩略图
};

//
enum ListItemType {
    CHAT_USER_ITEM, // 聊天用户
    CONTACT_USER_ITEM, // 联系人用户
    SEARCH_USER_ITEM, // 搜索到的用户
    ADD_USER_TIP_ITEM, // 提示添加用户
    INVALID_ITEM, // 不可点击条目
    GROUP_TIP_ITEM, // 分组提示条目
    LINE_ITEM, // 分割线
    APPLY_FRIEND_ITEM, // 好友申请
};

enum class ChatRole {
    SELF,
    OTHER,
};
const std::vector<QString>  strs = {
    "hello world !",
    "nice to meet u",
    "New year，new life",
    "You have to love yourself",
    "My love is written in the wind ever since the whole world is you"
};

const std::vector<QString> heads = {
    "./img/head_1.jpg",
    "./img/head_2.jpg",
    "./img/head_3.jpg",
    "./img/head_4.jpg",
    "./img/head_5.jpg"
};

const std::vector<QString> names = {
    "HanMeiMei",
    "Lily",
    "Ben",
    "Androw",
    "Max",
    "Summer",
    "Candy",
    "Hunter"
};

const int CHAT_COUNT_PER_PAGE = 13;

#endif // GLOBAL_H
