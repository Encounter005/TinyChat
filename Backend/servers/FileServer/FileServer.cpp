#include "FileServer.h"
#include "CallData.h"
#include "UploadCallData.h"
#include "DownloadCallData.h"
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
    builder.SetMaxReceiveMessageSize(1024 * 1024 * 1024 * 10);   // 10GB
    for (int i = 0; i < _cq_num; ++i) {
        _cqs.emplace_back(builder.AddCompletionQueue());
    }
    _server = builder.BuildAndStart();

    LOG_INFO("[FileServer] Server listening on {}:{}", _host, _port);
}

void FileServer::Run(const std::string& host, const std::string& port) {
    _host = host, _port = port;
    SetupServer();

    for (int i = 0; i < _cq_num; ++i) {
        _threads.emplace_back(&FileServer::StartEventLoop, this, i);
    }

    for (int i = 0; i < _cq_num; ++i) {
        new UploadCallData(_service.get(), _cqs[i].get());
        new DownloadCallData(_service.get(), _cqs[i].get());
    }

    for (auto& t : _threads) {
        t.join();
    }


    LOG_INFO("[FileServer] Server stopped");
}

void FileServer::StartEventLoop(int idx) {
    auto& cq = _cqs[idx];
    void* tag;
    bool  ok;
    while (cq->Next(&tag, &ok)) {
        auto* call = static_cast<CallData*>(tag);
        call->Proceed(ok);
    }
}

void FileServer::Stop() {
    if (!_server) {
        return;
    }
    _server->Shutdown();
    for (auto& cq : _cqs) {
        cq->Shutdown();
    }

    for (auto& t : _threads) {
        if (t.joinable()) t.join();
    }
    _cqs.clear();
    _server.reset();
}
