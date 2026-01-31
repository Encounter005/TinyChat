#ifndef DOWNLOADCALLDATA_H_
#define DOWNLOADCALLDATA_H_

#include "CallData.h"
#include "const.h"
#include "file.pb.h"
#include <fstream>
#include <vector>

class DownloadCallData : public CallData {
public:
        DownloadCallData(FileTransport::AsyncService* service, ServerCompletionQueue* cq);
        void Proceed(bool ok) override;
private:
        FileTransport::AsyncService* _service;
        ServerCompletionQueue* _cq;
        ServerContext _context;
        DownloadRequest _request;
        ServerAsyncWriter<DownloadResponse> _writer;
        CallState _state;
        std::ifstream _file_stream;
        std::vector<char> _buffer;


};

#endif   // DOWNLOADCALLDATA_H_
