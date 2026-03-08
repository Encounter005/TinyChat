#include "UserController.h"
#include "common/const.h"
#include "core/HttpConnection.h"
#include "core/HttpResponse.h"
#include "grpcClient/FileClient.h"
#include "grpcClient/StatusClient.h"
#include "infra/LogManager.h"
#include "repository/UserRepository.h"
#include "service/UserService.h"
#include <boost/beast/core/buffers_to_string.hpp>
#include <json/reader.h>
#include <json/value.h>
#include <memory>

namespace {

// 返回 true 表示解码成功，false 表示输入非法
static bool Base64Decode(const std::string &in, std::string &out) {
    static const std::array<int, 256> kDecTable = [] {
        std::array<int, 256> t{};
        t.fill(-1);
        for (int i = 'A'; i <= 'Z'; ++i) t[static_cast<size_t>(i)] = i - 'A';
        for (int i = 'a'; i <= 'z'; ++i)
            t[static_cast<size_t>(i)] = i - 'a' + 26;
        for (int i = '0'; i <= '9'; ++i)
            t[static_cast<size_t>(i)] = i - '0' + 52;
        t[static_cast<size_t>('+')] = 62;
        t[static_cast<size_t>('/')] = 63;
        return t;
    }();

    out.clear();
    out.reserve((in.size() * 3) / 4);

    int  val           = 0;
    int  valb          = -8;
    int  padding_count = 0;
    bool seen_padding  = false;

    for (unsigned char c : in) {
        if (std::isspace(c)) continue;   // 容忍空白字符

        if (c == '=') {
            seen_padding = true;
            ++padding_count;
            continue;
        }

        if (seen_padding) {
            // '=' 后面不允许再出现有效字符
            return false;
        }

        int d = kDecTable[static_cast<size_t>(c)];
        if (d < 0) return false;

        val = (val << 6) | d;
        valb += 6;
        if (valb >= 0) {
            out.push_back(static_cast<char>((val >> valb) & 0xFF));
            valb -= 8;
        }
    }

    // padding 只能是 0/1/2
    if (padding_count > 2) return false;

    // 简单合法性检查：有效长度 mod 4 不能为 1
    size_t effective_len = 0;
    for (unsigned char c : in) {
        if (!std::isspace(c)) ++effective_len;
    }
    if (effective_len % 4 == 1) return false;

    return true;
}

std::string BuildIconName(int uid) {
    const auto now = static_cast<long long>(std::time(nullptr));
    static thread_local std::mt19937_64 rng{
        std::random_device{}()};   // 生成随机值
    const unsigned int rr = static_cast<unsigned int>(rng() & 0xFFFFFFFULL);
    return "avatar_" + std::to_string(uid) + "_" + std::to_string(now) + "_"
           + std::to_string(rr) + ".png";
}
}   // namespace
void UserController::Register(LogicSystem &logic) {
    logic.RegPost("/user_register", RegisterUser);
}

void UserController::ResetPwd(LogicSystem &logic) {
    logic.RegPost("/reset_pwd", ResetUserPwd);
}

void UserController::Login(LogicSystem &logic) {
    logic.RegPost("/user_login", UserLogin);
}

void UserController::UpdateIcon(LogicSystem &logic) {
    logic.RegPost("/user_update_icon", UpdateUserIcon);
}



void UserController::RegisterUser(std::shared_ptr<HttpConnection> connection) {

    auto body = boost::beast::buffers_to_string(
        connection->GetRequest().body().data());

    Json::Value  src, rsp;
    Json::Reader reader;
    if (!reader.parse(body, src)) {
        HttpResponse::Error(connection, ErrorCodes::ERROR_JSON);
        return;
    }

    auto res = UserService::Register(
        src["user"].asString(),
        src["email"].asString(),
        src["passwd"].asString(),
        src["confirm"].asString(),
        src["varifycode"].asString());


    if (res.IsOK()) {
        HttpResponse::OK(connection);
    } else {
        HttpResponse::Error(connection, res.Error());
    }
}


void UserController::ResetUserPwd(std::shared_ptr<HttpConnection> connection) {
    auto body
        = beast::buffers_to_string(connection->GetRequest().body().data());

    Json::Value  src, rsp;
    Json::Reader reader;
    if (!reader.parse(body, src)) {
        HttpResponse::Error(connection, ErrorCodes::ERROR_JSON);
        return;
    }

    auto res = UserService::ResetPass(
        src["user"].asString(),
        src["email"].asString(),
        src["passwd"].asString(),
        src["varifycode"].asString());

    if (res.IsOK()) {
        HttpResponse::OK(connection);
    } else {
        HttpResponse::Error(connection, res.Error());
    }
}


void UserController::UserLogin(std::shared_ptr<HttpConnection> connection) {
    auto body
        = beast::buffers_to_string(connection->GetRequest().body().data());
    Json::Value  src, data;
    Json::Reader reader;
    if (!reader.parse(body, src)) {
        HttpResponse::Error(connection, ErrorCodes::ERROR_JSON);
        return;
    }

    auto res = UserService::Login(src["email"].asString());

    UserInfo userInfo = res.Value();
    auto     passwd   = src["passwd"].asString();
    if (passwd != userInfo.pwd) {
        HttpResponse::Error(connection, ErrorCodes::PASSWORD_ERROR);
        return;
    }

    auto reply = StatusClient::getInstance()->GetChatServer(userInfo.uid);
    if (reply.error()) {
        LOG_ERROR("[StatusClient] get chat server failed");
        HttpResponse::Error(connection, ErrorCodes::RPC_FAILED);
    }

    LOG_INFO("successd to load user_info uid is: {}", userInfo.uid);
    data["token"] = reply.token();
    data["name"]  = userInfo.name;
    data["host"]  = reply.host();
    data["uid"]   = userInfo.uid;
    data["port"]  = reply.port();

    HttpResponse::OK(connection, data);
}

void UserController::UpdateUserIcon(
    std::shared_ptr<HttpConnection> connection) {
    auto body
        = beast::buffers_to_string(connection->GetRequest().body().data());

    Json::Value  src;
    Json::Reader reader;
    if (!reader.parse(body, src)) {
        HttpResponse::Error(connection, ErrorCodes::ERROR_JSON);
        return;
    }

    if (!src.isMember("uid") || !src.isMember("image_b64")) {
        HttpResponse::Error(connection, ErrorCodes::ERROR_JSON);
        return;
    }


    const int         uid      = src["uid"].asInt();
    const std::string image_64 = src["image_b64"].asString();
    std::string       image_bytes;
    if (!Base64Decode(image_64, image_bytes)) {
        HttpResponse::Error(connection, ErrorCodes::ERROR_JSON);
        return;
    }

    const std::string          icon = BuildIconName(uid);
    std::vector<unsigned char> bytes(image_bytes.begin(), image_bytes.end());

    auto fileClient = FileClient::getInstance();
    auto upRes      = fileClient->UploadBytes(icon, bytes);

    if (!upRes.IsOK()) {
        HttpResponse::Error(connection, upRes.Error());
        return;
    }

    auto dbRes = UserRepository::UpdateUserIcon(uid, icon);
    if (!dbRes.IsOK()) {
        HttpResponse::Error(connection, dbRes.Error());
        return;
    }

    Json::Value data;
    data["icon"] = icon;
    HttpResponse::OK(connection, icon);
}
