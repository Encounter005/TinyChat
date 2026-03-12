#include "AiServiceImpl.h"
#include "infra/ConfigManager.h"
#include "infra/LogManager.h"
#include <grpcpp/security/server_credentials.h>
#include <grpcpp/server_builder.h>

auto main(int argc, char* argv[]) -> int {

    try {
        auto              cfg  = ConfigManager::getInstance();
        const std::string host = (*cfg)["AiServer"]["host"];
        const std::string port = (*cfg)["AiServer"]["port"];
        const std::string addr = host + ":" + port;

        VaneClient::Config vc;
        vc.base_url          = (*cfg)["Vane"]["base_url"];
        vc.chat_provider_id  = (*cfg)["Vane"]["chat_provider_id"];
        vc.chat_model_key    = (*cfg)["Vane"]["chat_model_key"];
        vc.emb_provider_id   = (*cfg)["Vane"]["embedding_provider_id"];
        vc.emb_model_key     = (*cfg)["Vane"]["embedding_model_key"];
        vc.optimization_mode = (*cfg)["Vane"]["optimization_mode"];
        vc.sources           = {"web"};

        AiServiceImpl service(vc);
        grpc::ServerBuilder builder;
        builder.AddListeningPort(addr, grpc::InsecureServerCredentials());
        builder.RegisterService(&service);

        auto server = builder.BuildAndStart();
        if(!server) {
            LOG_ERROR("[AiServer] start failed: {}", addr);
            return 1;
        }

        LOG_INFO("[AiServer] listening on {}", addr);
        server->Wait();
        return 0;

    } catch (const std::exception& e) {
        LOG_ERROR("[AiServer] exception: {}", e.what());
        return 1;
    }

    return 0;
}
