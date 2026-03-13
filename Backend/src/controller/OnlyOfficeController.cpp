#include "OnlyOfficeController.h"

#include "core/HttpResponse.h"
#include "grpcClient/FileClient.h"
#include "infra/ConfigManager.h"
#include "infra/LogManager.h"
#include <boost/beast/core/buffers_to_string.hpp>
#include <boost/beast/core/ostream.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <cctype>
#include <iomanip>
#include <json/json.h>
#include <openssl/hmac.h>
#include <regex>
#include <sstream>

namespace {
namespace beast = boost::beast;
namespace http  = beast::http;
namespace net   = boost::asio;
using tcp       = boost::asio::ip::tcp;

std::string Base64UrlEncode(const std::string& in) {
    static const char* kTable
        = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string out;
    out.reserve(((in.size() + 2U) / 3U) * 4U);

    int val = 0;
    int valb = -6;
    for (unsigned char c : in) {
        val = (val << 8) + static_cast<int>(c);
        valb += 8;
        while (valb >= 0) {
            out.push_back(kTable[(val >> valb) & 0x3F]);
            valb -= 6;
        }
    }
    if (valb > -6) {
        out.push_back(kTable[((val << 8) >> (valb + 8)) & 0x3F]);
    }
    while (out.size() % 4U != 0U) {
        out.push_back('=');
    }

    for (char& ch : out) {
        if (ch == '+') {
            ch = '-';
        } else if (ch == '/') {
            ch = '_';
        }
    }

    while (!out.empty() && out.back() == '=') {
        out.pop_back();
    }
    return out;
}

std::string BuildJwt(const Json::Value& payload, const std::string& secret) {
    Json::Value header;
    header["alg"] = "HS256";
    header["typ"] = "JWT";

    Json::StreamWriterBuilder writer;
    writer["indentation"] = "";

    const std::string header_json = Json::writeString(writer, header);
    const std::string payload_json = Json::writeString(writer, payload);
    const std::string signing_input
        = Base64UrlEncode(header_json) + "." + Base64UrlEncode(payload_json);

    unsigned int sig_len = 0;
    unsigned char* sig = HMAC(
        EVP_sha256(),
        secret.data(),
        static_cast<int>(secret.size()),
        reinterpret_cast<const unsigned char*>(signing_input.data()),
        static_cast<int>(signing_input.size()),
        nullptr,
        &sig_len);
    if (sig == nullptr || sig_len == 0U) {
        return "";
    }

    const std::string sig_str(
        reinterpret_cast<const char*>(sig), static_cast<size_t>(sig_len));
    return signing_input + "." + Base64UrlEncode(sig_str);
}

std::string GetFileExt(const std::string& file_name) {
    const auto pos = file_name.find_last_of('.');
    if (pos == std::string::npos || pos + 1 >= file_name.size()) {
        return "docx";
    }
    std::string ext = file_name.substr(pos + 1);
    for (char& c : ext) {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    return ext;
}

std::string GetDocumentType(const std::string& ext) {
    if (ext == "xls" || ext == "xlsx") {
        return "cell";
    }
    if (ext == "ppt" || ext == "pptx") {
        return "slide";
    }
    return "word";
}

std::string UrlEncode(const std::string& raw) {
    std::ostringstream oss;
    oss << std::hex << std::uppercase;
    for (unsigned char c : raw) {
        if (std::isalnum(c) != 0 || c == '-' || c == '_' || c == '.'
            || c == '~') {
            oss << static_cast<char>(c);
        } else {
            oss << '%' << std::setw(2) << std::setfill('0')
                << static_cast<int>(c);
        }
    }
    return oss.str();
}

std::string GetGatewayBaseUrl() {
    auto config = ConfigManager::getInstance();
    auto onlyoffice_base = (*config)["OnlyOffice"]["gateway_base_url"];
    if (!onlyoffice_base.empty()) {
        return onlyoffice_base;
    }

    const std::string host = (*config)["GateServer"]["host"];
    const std::string port = (*config)["GateServer"]["port"];
    if (host.empty() || port.empty()) {
        return "";
    }
    return "http://" + host + ":" + port;
}

std::string GetOnlyOfficeServerUrl() {
    auto config = ConfigManager::getInstance();
    return (*config)["OnlyOffice"]["document_server_url"];
}

std::string GetOnlyOfficeJwtSecret() {
    auto config = ConfigManager::getInstance();
    return (*config)["OnlyOffice"]["jwt_secret"];
}

void SendRawJson(
    const std::shared_ptr<HttpConnection>& connection,
    const Json::Value& json) {
    connection->GetResponse().result(http::status::ok);
    connection->GetResponse().set(http::field::content_type, "application/json");
    beast::ostream(connection->GetResponse().body()) << json.toStyledString();
}

Result<std::vector<unsigned char>> DownloadFromHttpUrl(const std::string& url) {
    static const std::regex kUrlPattern(
        R"(^(https?)://([^/:]+)(?::(\d+))?(\/.*)?$)",
        std::regex::icase);
    std::smatch match;
    if (!std::regex_match(url, match, kUrlPattern)) {
        return Result<std::vector<unsigned char>>::Error(ErrorCodes::ERROR_JSON);
    }

    std::string scheme = match[1].str();
    std::string host = match[2].str();
    std::string port = match[3].matched ? match[3].str() : "";
    std::string target = match[4].matched ? match[4].str() : "/";
    for (char& c : scheme) {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    if (scheme != "http") {
        LOG_ERROR("[OnlyOffice] callback download only supports http, got: {}", scheme);
        return Result<std::vector<unsigned char>>::Error(ErrorCodes::RPC_FAILED);
    }
    if (port.empty()) {
        port = "80";
    }

    try {
        net::io_context ioc;
        tcp::resolver resolver(ioc);
        beast::tcp_stream stream(ioc);

        const auto results = resolver.resolve(host, port);
        stream.connect(results);

        http::request<http::string_body> req{http::verb::get, target, 11};
        req.set(http::field::host, host);
        req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);

        http::write(stream, req);

        beast::flat_buffer buffer;
        http::response<http::dynamic_body> res;
        http::read(stream, buffer, res);

        beast::error_code ec;
        stream.socket().shutdown(tcp::socket::shutdown_both, ec);

        if (res.result() != http::status::ok) {
            LOG_ERROR(
                "[OnlyOffice] callback download failed, status={} url={}",
                static_cast<unsigned int>(res.result()),
                url);
            return Result<std::vector<unsigned char>>::Error(ErrorCodes::RPC_FAILED);
        }

        const std::string body = beast::buffers_to_string(res.body().data());
        std::vector<unsigned char> bytes(body.begin(), body.end());
        if (bytes.empty()) {
            return Result<std::vector<unsigned char>>::Error(ErrorCodes::RPC_FAILED);
        }
        return Result<std::vector<unsigned char>>::OK(std::move(bytes));
    } catch (const std::exception& e) {
        LOG_ERROR("[OnlyOffice] callback download exception: {}", e.what());
        return Result<std::vector<unsigned char>>::Error(ErrorCodes::RPC_FAILED);
    }
}

}   // namespace

void OnlyOfficeController::Register(LogicSystem& logic) {
    logic.RegGet("/onlyoffice/editor", OpenEditor);
    logic.RegGet("/onlyoffice/file", DownloadFile);
    logic.RegPost("/onlyoffice/callback", Callback);
}

void OnlyOfficeController::OpenEditor(std::shared_ptr<HttpConnection> connection) {
    const auto& params = connection->GetParams();
    const auto file_it = params.find("file");
    if (file_it == params.end() || file_it->second.empty()) {
        HttpResponse::Error(connection, ErrorCodes::ERROR_JSON, "missing file param");
        return;
    }

    const std::string file_name = file_it->second;
    const std::string gateway_base = GetGatewayBaseUrl();
    const std::string ds_url = GetOnlyOfficeServerUrl();
    const std::string jwt_secret = GetOnlyOfficeJwtSecret();

    if (gateway_base.empty() || ds_url.empty() || jwt_secret.empty()) {
        HttpResponse::Error(
            connection,
            ErrorCodes::ERROR_JSON,
            "OnlyOffice config missing: gateway_base_url/document_server_url/jwt_secret");
        return;
    }

    const std::string encoded_name = UrlEncode(file_name);
    const std::string file_url
        = gateway_base + "/onlyoffice/file?file=" + encoded_name;
    const std::string callback_url = gateway_base + "/onlyoffice/callback";
    const std::string ext = GetFileExt(file_name);

    Json::Value config;
    config["documentType"] = GetDocumentType(ext);
    config["type"] = "desktop";

    config["document"]["fileType"] = ext;
    config["document"]["key"] = file_name;
    config["document"]["title"] = file_name;
    config["document"]["url"] = file_url;

    config["document"]["permissions"]["edit"] = true;
    config["document"]["permissions"]["download"] = true;
    config["document"]["permissions"]["print"] = true;

    config["editorConfig"]["mode"] = "edit";
    config["editorConfig"]["lang"] = "zh-CN";
    config["editorConfig"]["callbackUrl"] = callback_url;

    const std::string token = BuildJwt(config, jwt_secret);
    if (token.empty()) {
        HttpResponse::Error(connection, ErrorCodes::RPC_FAILED, "build jwt failed");
        return;
    }

    Json::StreamWriterBuilder writer;
    writer["indentation"] = "";
    const std::string config_json = Json::writeString(writer, config);

    std::ostringstream html;
    html << "<!doctype html><html><head><meta charset=\"utf-8\">"
         << "<title>TinyChat OnlyOffice</title>"
         << "<style>html,body,#editor{height:100%;margin:0;padding:0;}</style>"
         << "<script src=\"" << ds_url
         << "/web-apps/apps/api/documents/api.js\"></script>"
         << "</head><body><div id=\"editor\"></div><script>"
         << "var config=" << config_json << ";"
         << "config.token=\"" << token << "\";"
         << "new DocsAPI.DocEditor('editor', config);"
         << "</script></body></html>";

    connection->GetResponse().result(http::status::ok);
    connection->GetResponse().set(http::field::content_type, "text/html; charset=utf-8");
    beast::ostream(connection->GetResponse().body()) << html.str();
}

void OnlyOfficeController::DownloadFile(std::shared_ptr<HttpConnection> connection) {
    const auto& params = connection->GetParams();
    const auto file_it = params.find("file");
    if (file_it == params.end() || file_it->second.empty()) {
        HttpResponse::Error(connection, ErrorCodes::ERROR_JSON, "missing file param");
        return;
    }

    const std::string file_name = file_it->second;
    auto file_client = FileClient::getInstance();
    auto download_res = file_client->DownloadBytes(file_name);
    if (!download_res.IsOK()) {
        HttpResponse::Error(connection, download_res.Error(), "download file failed");
        return;
    }

    connection->GetResponse().result(http::status::ok);
    connection->GetResponse().set(
        http::field::content_type,
        "application/octet-stream");
    connection->GetResponse().set(
        http::field::content_disposition,
        "inline; filename=\"" + file_name + "\"");
    const auto& bytes = download_res.Value();
    beast::ostream(connection->GetResponse().body()).write(
        reinterpret_cast<const char*>(bytes.data()),
        static_cast<std::streamsize>(bytes.size()));
}

void OnlyOfficeController::Callback(std::shared_ptr<HttpConnection> connection) {
    const auto body = boost::beast::buffers_to_string(
        connection->GetRequest().body().data());
    LOG_INFO("[OnlyOffice] callback body: {}", body);

    Json::Value src;
    Json::Reader reader;
    if (!reader.parse(body, src)) {
        Json::Value bad;
        bad["error"] = 1;
        SendRawJson(connection, bad);
        return;
    }

    const auto& params = connection->GetParams();
    std::string file_name;
    const auto file_it = params.find("file");
    if (file_it != params.end() && !file_it->second.empty()) {
        file_name = file_it->second;
    }
    if (file_name.empty() && src.isMember("key") && src["key"].isString()) {
        file_name = src["key"].asString();
    }
    if (file_name.empty()) {
        Json::Value bad;
        bad["error"] = 1;
        SendRawJson(connection, bad);
        return;
    }
    const int status = src.get("status", 0).asInt();
    const std::string download_url = src.get("url", "").asString();
    LOG_INFO(
        "[OnlyOffice] callback status={} file={} has_url={}",
        status,
        file_name,
        !download_url.empty());

    // 2: MustSave, 6: ForceSave
    if ((status == 2 || status == 6) && !download_url.empty()) {
        auto download_res = DownloadFromHttpUrl(download_url);
        if (!download_res.IsOK()) {
            Json::Value bad;
            bad["error"] = 1;
            SendRawJson(connection, bad);
            return;
        }

        auto upload_res
            = FileClient::getInstance()->UploadBytes(file_name, download_res.Value());
        if (!upload_res.IsOK()) {
            LOG_ERROR("[OnlyOffice] callback upload failed file={}", file_name);
            Json::Value bad;
            bad["error"] = 1;
            SendRawJson(connection, bad);
            return;
        }
        LOG_INFO("[OnlyOffice] callback saved file={} status={}", file_name, status);
    }

    Json::Value rsp;
    rsp["error"] = 0;
    SendRawJson(connection, rsp);
}
