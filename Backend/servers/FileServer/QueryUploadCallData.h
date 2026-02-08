#ifndef QUERYUPLOADCALLDATA_H_
#define QUERYUPLOADCALLDATA_H_

#include "CallData.h"
#include "const.h"
#include "file.grpc.pb.h"

class QueryUploadCallData : public CallData {
public:
    QueryUploadCallData(
        FileService::FileTransport::AsyncService* service,
        grpc::ServerCompletionQueue*              cq);

    void Proceed(bool ok) override;

private:
    FileService::FileTransport::AsyncService* _service;
    grpc::ServerCompletionQueue*              _cq;

    grpc::ServerContext                                        _ctx;
    FileService::QueryUploadRequest                            _request;
    FileService::UploadStatus                                  _response;
    grpc::ServerAsyncResponseWriter<FileService::UploadStatus> _responder;

    CallState _state;
};

#endif   // QUERYUPLOADCALLDATA_H_
