#ifndef QUERYDOWNLOADCALLDATA_H_
#define QUERYDOWNLOADCALLDATA_H_

#include "CallData.h"
#include "const.h"
#include "file.grpc.pb.h"
#include <grpcpp/server_context.h>

class QueryDownloadCallData : public CallData {
public:
    QueryDownloadCallData(
        FileService::FileTransport::AsyncService* service,
        grpc::ServerCompletionQueue*              cq);

    void Proceed(bool ok) override;

private:
    FileService::FileTransport::AsyncService* _service;
    grpc::ServerCompletionQueue*              _cq;

    grpc::ServerContext                                          _ctx;
    FileService::QueryDownloadRequest                            _request;
    FileService::DownloadStatus                                  _response;
    grpc::ServerAsyncResponseWriter<FileService::DownloadStatus> _responder;

    CallState _state;
};

#endif   // QUERYDOWNLOADCALLDATA_H_
