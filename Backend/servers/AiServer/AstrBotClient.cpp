#include "AstrBotClient.h"

#include "infra/LogManager.h"

#include <algorithm>
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/beast/core/error.hpp>
#include <boost/beast/core/tcp_stream.hpp>
#include <boost/beast/http/field.hpp>
#include <boost/beast/http/message_fwd.hpp>
#include <boost/beast/http/string_body_fwd.hpp>
#include <boost/beast/version.hpp>
#include <cctype>
#include <exception>
#include <json/reader.h>
#include <json/value.h>
#include <json/writer.h>
#include <sstream>
#include <vector>

namespace net   = boost::asio;
namespace beast = boost::beast;
namespace http  = beast::http;
using tcp       = net::ip::tcp;

namespace {
std::string Trim(std::string value) {
    value.erase(
        value.begin(),
        std::find_if(value.begin(), value.end(), [](unsigned char ch) {
            return !std::isspace(ch);
        }));
    value.erase(
        std::find_if(
            value.rbegin(),
            value.rend(),
            [](unsigned char ch) { return !std::isspace(ch); })
            .base(),
        value.end());
    return value;
}

std::string FindFirstStringField(const Json::Value& value) {
    static const std::vector<std::string> keys
        = {"message", "answer", "content", "text", "data"};

    if (value.isObject()) {
        for (const auto& key : keys) {
            if (value.isMember(key) && value[key].isString()
                && !value[key].asString().empty()) {
                return value[key].asString();
            }
        }

        for (const auto& member_name : value.getMemberNames()) {
            const std::string nested = FindFirstStringField(value[member_name]);
            if (!nested.empty()) {
                return nested;
            }
        }
        return "";
    }

    if (value.isArray()) {
        for (const auto& item : value) {
            const std::string nested = FindFirstStringField(item);
            if (!nested.empty()) {
                return nested;
            }
        }
    }

    return "";
}
}   // namespace

AstrBotClient::AstrBotClient(const Config& config) : _cfg(SanitizeConfig(config)) {}

AstrBotClient::Config AstrBotClient::SanitizeConfig(const Config& config) {
    Config sanitized = config;
    sanitized.base_url = Trim(sanitized.base_url);
    sanitized.api_key = Trim(sanitized.api_key);
    sanitized.username_prefix = Trim(sanitized.username_prefix);
    sanitized.session_prefix = Trim(sanitized.session_prefix);
    return sanitized;
}

std::string AstrBotClient::ParseAnswerForTest(const std::string& body) {
    return ParseAnswerFromResponse(body);
}

bool AstrBotClient::Chat(
    int uid, const std::string& query, const std::string& platform,
    std::string& answer, std::string& err_msg) const {
    if (_cfg.api_key.empty()) {
        err_msg = "astrbot api_key is empty";
        return false;
    }

    std::string host;
    std::string port;
    if (!ParseBaseUrl(_cfg.base_url, host, port)) {
        err_msg = "invalid astrbot base_url";
        return false;
    }

    try {
        net::io_context   ioc;
        tcp::resolver     resolver(ioc);
        beast::tcp_stream stream(ioc);
        auto const        results = resolver.resolve(host, port);
        stream.connect(results);

        http::request<http::string_body> req{
            http::verb::post, "/api/v1/chat", 11};
        req.set(http::field::host, host);
        req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
        req.set(http::field::content_type, "application/json");
        req.set(http::field::authorization, "Bearer " + _cfg.api_key);
        req.body() = BuildChatBody(uid, query, platform);
        req.prepare_payload();

        LOG_INFO(
            "[AstrBotClient] request uid={} platform={} host={}:{} body={}",
            uid,
            platform,
            host,
            port,
            req.body());

        http::write(stream, req);

        beast::flat_buffer                buffer;
        http::response<http::string_body> res;
        http::read(stream, buffer, res);

        beast::error_code ec;
        stream.socket().shutdown(tcp::socket::shutdown_both, ec);

        if (res.result_int() < 200 || res.result_int() >= 300) {
            err_msg
                = "astrbot http status = " + std::to_string(res.result_int())
                  + ", body = " + res.body();
            LOG_ERROR("[AstrBotClient] {}", err_msg);
            return false;
        }

        LOG_INFO(
            "[AstrBotClient] response status={} body={}",
            res.result_int(),
            res.body());

        answer = ParseAnswerFromResponse(res.body());
        if (answer.empty()) {
            err_msg = "empty message in astrbot response";
            LOG_ERROR("[AstrBotClient] {}", err_msg);
            return false;
        }
        LOG_INFO("[AstrBotClient] parsed answer={}", answer);
        return true;
    } catch (const std::exception& e) {
        err_msg = e.what();
        LOG_ERROR("[AstrBotClient] exception={}", err_msg);
        return false;
    }
}

std::string AstrBotClient::BuildChatBody(
    int uid, const std::string& query, const std::string& platform) const {
    Json::Value       root;
    const std::string normalized_platform = NormalizePlatform(platform);
    root["username"] = _cfg.username_prefix + std::to_string(uid);
    root["session_id"]
        = _cfg.session_prefix + normalized_platform + "_" + std::to_string(uid);
    root["message"]          = query;
    root["enable_streaming"] = _cfg.enable_streaming;

    Json::StreamWriterBuilder builder;
    builder["indentation"] = "";
    return Json::writeString(builder, root);
}

bool AstrBotClient::ParseBaseUrl(
    const std::string& base_url, std::string& host, std::string& port) {
    std::string           value        = Trim(base_url);
    constexpr const char* http_prefix  = "http://";
    constexpr const char* https_prefix = "https://";
    if (value.rfind(http_prefix, 0) == 0) {
        value.erase(0, std::char_traits<char>::length(http_prefix));
    } else if (value.rfind(https_prefix, 0) == 0) {
        value.erase(0, std::char_traits<char>::length(https_prefix));
    }

    const auto slash = value.find('/');
    if (slash != std::string::npos) {
        value = value.substr(0, slash);
    }

    const auto pos = value.rfind(':');
    if (pos == std::string::npos || pos == 0 || pos + 1 >= value.size()) {
        return false;
    }

    host = value.substr(0, pos);
    port = value.substr(pos + 1);
    return true;
}

std::string AstrBotClient::ParseAnswerFromResponse(const std::string& body) {
    Json::CharReaderBuilder rb;
    std::string             errs;
    std::istringstream      iss(body);
    Json::Value             root;
    if (Json::parseFromStream(rb, iss, &root, &errs)) {
        return FindFirstStringField(root);
    }

    std::istringstream lines(body);
    std::string        line;
    std::string        latest_answer;
    while (std::getline(lines, line)) {
        line = Trim(line);
        if (line.rfind("data:", 0) != 0) {
            continue;
        }
        std::string payload = Trim(line.substr(5));
        if (payload.empty() || payload == "[DONE]") {
            continue;
        }

        Json::Value        sse_root;
        std::string        sse_errs;
        std::istringstream sse_stream(payload);
        if (Json::parseFromStream(rb, sse_stream, &sse_root, &sse_errs)) {
            const std::string candidate = FindFirstStringField(sse_root);
            if (!candidate.empty()) {
                latest_answer = candidate;
            }
            continue;
        }

        latest_answer = payload;
    }

    return latest_answer;
}

std::string AstrBotClient::NormalizePlatform(const std::string& platform) {
    std::string normalized = Trim(platform);
    std::transform(
        normalized.begin(),
        normalized.end(),
        normalized.begin(),
        [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
    if (normalized.empty()) {
        return "astrbot";
    }
    return normalized;
}
