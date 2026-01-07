#ifndef LOGICSYSTEM_H_
#define LOGICSYSTEM_H_
#include "const.h"
#include "singleton.h"
#include "message.grpc.pb.h"
#include "message.pb.h"
#include <grpcpp/client_context.h>

using message::GetVarifyRsp;
using message::GetVarifyReq;
using message::VarifyService;

class HttpConnection;
using HttpHandler = std::function<void(std::shared_ptr<HttpConnection>)>;

class LogicSystem : public SingleTon<LogicSystem> {

public:
    friend class SingleTon<LogicSystem>;
    friend class HttpConnection;

    ~LogicSystem() = default;
    bool HandleGet(const std::string&, std::shared_ptr<HttpConnection>);
    bool HandlePost(const std::string&, std::shared_ptr<HttpConnection>);
    void RegGet(const std::string&, HttpHandler);
    void RegPost(const std::string&, HttpHandler);

private:
    LogicSystem();
    // KEY: router
    // VALUE: call_back_function
    std::unordered_map<std::string, HttpHandler> _post_handlers;
    std::unordered_map<std::string, HttpHandler> _get_handlers;

};

#endif   // LOGICSYSTEM_H_
