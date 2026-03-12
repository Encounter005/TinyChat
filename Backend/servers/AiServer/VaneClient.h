#ifndef VANECLIENT_H_
#define VANECLIENT_H_

#include <string>
#include <vector>
class VaneClient {
public:
    struct Config {
        std::string              base_url;
        std::string              chat_provider_id;
        std::string              chat_model_key;
        std::string              emb_provider_id;
        std::string              emb_model_key;
        std::string              optimization_mode;
        std::vector<std::string> sources;
        int                      timeout_ms = 8000;
    };
    explicit VaneClient(const Config& config);
    bool Search(
        const std::string&                                      query,
        const std::vector<std::pair<std::string, std::string>>& history,
        std::string& answer, std::string& err_msg) const;

    static std::string ParseMessageFromSearchResponse(const std::string& body);

private:
    std::string BuildSearchBody(
        const std::string&                                      query,
        const std::vector<std::pair<std::string, std::string>>& history) const;
private:
    Config _cfg;
};

#endif   // VANECLIENT_H_
