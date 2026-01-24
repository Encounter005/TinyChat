#ifndef CONST_H_
#define CONST_H_

#include <assert.h>
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/beast/http.hpp>
#include <cstdint>
#include <json/json.h>
#include <json/reader.h>
#include <json/value.h>

namespace beast = boost::beast;
namespace http  = beast::http;
namespace net   = boost::asio;
using tcp       = boost::asio::ip::tcp;

class ConfigManager;

enum class ErrorCodes : int {
    SUCCESS                 = 0,
    ERROR_JSON              = 1001,   // json parse error
    RPC_FAILED              = 1002,   // rpc request error
    VARIFY_EXPIRED          = 1003,   // varify_code expired
    VARIFY_CODE_ERROR       = 1004,
    USER_EXIST              = 1005,
    PASSWORD_ERROR          = 1006,
    EMAIL_NOT_MATCH         = 1007,
    MYSQL_CONNECTION_ERROR  = 1008,
    SQL_ERROR               = 1009,
    SQL_STAND_EXCEPTION     = 1010,
    MYSQL_UNKNOWN_ERROR     = 1011,
    SQL_OPERATION_EXCEPTION = 1012,
    PASSWORD_RESET_ERROR    = 1013,
    UID_INVALID             = 1014,
    TOKEN_INVALID           = 1015,
    REDIS_ERROR             = 1016,
};

constexpr const char* ErrorMsg(ErrorCodes code) {
    switch (code) {
    case ErrorCodes::SUCCESS: return "success";

    case ErrorCodes::ERROR_JSON: return "json parse error";

    case ErrorCodes::RPC_FAILED: return "rpc request failed";

    case ErrorCodes::VARIFY_EXPIRED: return "verify code expired";

    case ErrorCodes::VARIFY_CODE_ERROR: return "verify code error";

    case ErrorCodes::USER_EXIST: return "user already exists";

    case ErrorCodes::PASSWORD_ERROR: return "password error";

    case ErrorCodes::EMAIL_NOT_MATCH: return "email not match";

    case ErrorCodes::MYSQL_CONNECTION_ERROR: return "mysql connection error";

    case ErrorCodes::SQL_ERROR: return "sql error";

    case ErrorCodes::SQL_STAND_EXCEPTION: return "sql standard exception";

    case ErrorCodes::MYSQL_UNKNOWN_ERROR: return "mysql unknown error";

    case ErrorCodes::SQL_OPERATION_EXCEPTION: return "sql operation exception";

    case ErrorCodes::PASSWORD_RESET_ERROR: return "password reset error";

    case ErrorCodes::UID_INVALID: return "uid invalid";

    case ErrorCodes::TOKEN_INVALID: return "token invalid";

    case ErrorCodes::REDIS_ERROR: return "redis operation error";

    default: return "unknown error";
    }
}

enum class MsgId : uint16_t {
    ID_ECHO                     = 1,
    ID_GET_VARIFY_CODE          = 101,   // 获取验证码
    ID_REG_USER                 = 102,   // 注册用户
    ID_RESET_PWD                = 103,   // 重置密码
    ID_LOGIN_USER               = 104,   // 用户登录
    ID_CHAT_LOGIN_REQ           = 105,   // 登陆聊天服务器
    ID_CHAT_LOGIN_RSP           = 106,   // 登陆聊天服务器回包
    ID_SEARCH_USER_REQ          = 107,   // 用户搜索请求
    ID_SEARCH_USER_RSP          = 108,   // 搜索用户回包
    ID_ADD_FRIEND_REQ           = 109,   // 添加好友申请
    ID_ADD_FRIEND_RSP           = 110,   // 申请添加好友回复
    ID_NOTIFY_ADD_FRIEND_REQ    = 111,   // 通知用户添加好友申请
    ID_AUTH_FRIEND_REQ          = 113,   // 认证好友请求
    ID_AUTH_FRIEND_RSP          = 114,   // 认证好友回复
    ID_NOTIFY_AUTH_FRIEND_REQ   = 115,   // 通知用户认证好友申请
    ID_TEXT_CHAT_MSG_REQ        = 117,   // 文本聊天信息请求
    ID_TEXT_CHAT_MSG_RSP        = 118,   // 文本聊天信息回复
    ID_NOTIFY_TEXT_CHAT_MSG_REQ = 119,   // 通知用户文本聊天信息
    ID_NOTIFY_OFF_LINE_REQ      = 120,   // 通知用户下线
    ID_HEART_BEAT_REQ           = 121,   // 心跳请求
    ID_HEARTBEAT_RSP            = 122,   // 心跳回复
};

constexpr MsgId INVALID_MSG_ID = static_cast<MsgId>(0);
constexpr MsgId ReqToRsp(MsgId req) {
    switch (req) {
    case MsgId::ID_CHAT_LOGIN_REQ: return MsgId::ID_CHAT_LOGIN_RSP;

    case MsgId::ID_SEARCH_USER_REQ: return MsgId::ID_SEARCH_USER_RSP;

    case MsgId::ID_ADD_FRIEND_REQ: return MsgId::ID_ADD_FRIEND_RSP;

    case MsgId::ID_AUTH_FRIEND_REQ: return MsgId::ID_AUTH_FRIEND_RSP;

    case MsgId::ID_TEXT_CHAT_MSG_REQ: return MsgId::ID_TEXT_CHAT_MSG_RSP;

    case MsgId::ID_HEART_BEAT_REQ: return MsgId::ID_HEARTBEAT_RSP;

    default: return INVALID_MSG_ID;
    }
}


#endif   // CONST_H_
