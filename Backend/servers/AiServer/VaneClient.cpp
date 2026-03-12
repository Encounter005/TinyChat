#include "VaneClient.h"
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/beast/core/error.hpp>
#include <boost/beast/core/tcp_stream.hpp>
#include <boost/beast/http/field.hpp>
#include <boost/beast/http/message_fwd.hpp>
#include <boost/beast/http/string_body_fwd.hpp>
#include <boost/beast/version.hpp>
#include <exception>
#include <json/reader.h>
#include <json/value.h>
#include <json/writer.h>
#include <sstream>

namespace net   = boost::asio;
namespace beast = boost::beast;
namespace http  = beast::http;
using tcp       = net::ip::tcp;

VaneClient::VaneClient(const Config& config) : _cfg(config) {}

bool VaneClient::Search(
    const std::string&                                      query,
    const std::vector<std::pair<std::string, std::string>>& history,
    std::string& answer, std::string& err_msg) const {

    try {
        auto pos = _cfg.base_url.find(':');
        if (pos == std::string::npos) {
            err_msg = "invalid base_url";
            return false;
        }
        const std::string host = _cfg.base_url.substr(0, pos);
        const std::string port = _cfg.base_url.substr(pos + 1);

        net::io_context   ioc;
        tcp::resolver     resolver(ioc);
        beast::tcp_stream stream(ioc);
        auto const        results = resolver.resolve(host, port);
        stream.connect(results);

        http::request<http::string_body> req{
            http::verb::post, "/api/search", 11};
        req.set(http::field::host, host);
        req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
        req.set(http::field::content_type, "application/json");
        req.body() = BuildSearchBody(query, history);
        req.prepare_payload();

        http::write(stream, req);

        beast::flat_buffer                buffer;
        http::response<http::string_body> res;
        http::read(stream, buffer, res);

        beast::error_code ec;
        stream.socket().shutdown(tcp::socket::shutdown_both, ec);

        if (res.result_int() < 200 || res.result_int() >= 300) {
            err_msg = "vane http status = " + std::to_string(res.result_int());
            return false;
        }
        answer = ParseMessageFromSearchResponse(res.body());
        if (answer.empty()) {
            err_msg = "empty message in vane response";
            return false;
        }

        return true;

    } catch (const std::exception& e) {
        err_msg = e.what();
        return false;
    }
}

std::string VaneClient::ParseMessageFromSearchResponse(
    const std::string& body) {
    Json::CharReaderBuilder rb;
    std::string             errs;
    std::istringstream      iss(body);
    Json::Value             root;
    if (!Json::parseFromStream(rb, iss, &root, &errs)) {
        return "";
    }
    if (!root.isObject() || !root["message"].isString()) {
        return "";
    }

    return root["message"].asString();
}

std::string VaneClient::BuildSearchBody(
    const std::string&                                      query,
    const std::vector<std::pair<std::string, std::string>>& history) const {

    Json::Value root;
    root["chatModel"]["providerId"]      = _cfg.chat_provider_id;
    root["chatModel"]["key"]             = _cfg.chat_model_key;
    root["embeddingModel"]["providerId"] = _cfg.emb_provider_id;
    root["embeddingModel"]["key"]        = _cfg.emb_model_key;
    root["optimizationMode"]             = _cfg.optimization_mode;

    for (const auto& s : _cfg.sources) {
        root["sources"].append(s);
    }
    root["query"]  = query;
    root["stream"] = false;

    for (const auto& [role, content] : history) {
        Json::Value one(Json::arrayValue);
        one.append(role);
        one.append(content);
        root["history"].append(one);
    }

    Json::StreamWriterBuilder builder;
    builder["indentation"] = "";
    return Json::writeString(builder, root);
}
