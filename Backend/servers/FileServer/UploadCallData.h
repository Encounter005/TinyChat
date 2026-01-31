#ifndef UPLOADCALLDATA_H_
#define UPLOADCALLDATA_H_

#include "CallData.h"
#include "const.h"
#include "infra/LogManager.h"
#include <condition_variable>

class UploadCallData : public CallData {
public:
    UploadCallData(
        FileTransport::AsyncService* service, grpc::ServerCompletionQueue* cq);

    void Proceed(bool ok) override;

private:
    FileTransport::AsyncService* _service;
    ServerCompletionQueue*       _cq;

    ServerContext                                    _ctx;
    ServerAsyncReader<UploadResponse, UploadRequest> _reader;

    UploadRequest  _request;
    UploadResponse _response;

    CallState   _state;
    std::string _current_md5;
    std::string _current_filename;
};

#endif   // UPLOADCALLDATA_H_
