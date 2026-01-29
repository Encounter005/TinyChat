#include "FileServer.h"
#include "UploadCallData.h"
#include "file.grpc.pb.h"
#include "infra/LogManager.h"
#include <grpcpp/security/server_credentials.h>

FileServer::FileServer()
    : _service(std::make_unique<FileTransport::AsyncService>()) {}

FileServer::~FileServer() {
    Stop();
}

void FileServer::SetupServer() {
    ServerBuilder builder;
    builder.AddListeningPort(
        _host + ":" + _port, grpc::InsecureServerCredentials());

    builder.RegisterService(_service.get());
    builder.SetMaxReceiveMessageSize(1024 * 1024 * 1024);   // 1GB
    _cq     = builder.AddCompletionQueue();
    _server = builder.BuildAndStart();

    LOG_INFO("[FileServer] Server listening on {}:{}", _host, _port);
}

void FileServer::Run(const std::string& host, const std::string& port) {
    _host = host, _port = port;
    SetupServer();
    _event_loop = std::thread(&FileServer::StartEventLoop, this);
    auto call_data = new UploadCallData(_service.get(), _cq.get());

    _event_loop.join();

    LOG_INFO("[FileServer] Server stopped");
}

void FileServer::StartEventLoop() {
    void* tag;
    bool  ok;

    while (true) {
        if (!_cq->Next(&tag, &ok)) {
            LOG_ERROR("[FileServer] CQ shutdown");
            break;
        }

        static_cast<UploadCallData*>(tag)->Proceed(ok);
    }
}

void FileServer::Stop() {
    if (_server) {
        _server->Shutdown();
        _cq->Shutdown();
        if (_event_loop.joinable()) {
            _event_loop.join();
        }
    }
}
