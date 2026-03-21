#ifndef ASTRBOTCLIENT_H_
#define ASTRBOTCLIENT_H_

#include <string>

class AstrBotClient {
public:
    struct Config {
        std::string base_url;
        std::string api_key;
        std::string username_prefix;
        std::string session_prefix;
        int         timeout_ms       = 8000;
        bool        enable_streaming = false;
    };

    explicit AstrBotClient(const Config& config);

    static Config SanitizeConfig(const Config& config);
    static std::string ParseAnswerForTest(const std::string& body);

    bool Chat(
        int uid, const std::string& query, const std::string& platform,
        std::string& answer, std::string& err_msg) const;

    std::string BuildChatBody(
        int uid, const std::string& query, const std::string& platform) const;

private:
    static bool ParseBaseUrl(
        const std::string& base_url, std::string& host, std::string& port);
    static std::string ParseAnswerFromResponse(const std::string& body);
    static std::string NormalizePlatform(const std::string& platform);

private:
    Config _cfg;
};

#endif   // ASTRBOTCLIENT_H_
