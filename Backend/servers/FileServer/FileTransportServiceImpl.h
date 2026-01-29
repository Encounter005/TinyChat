#ifndef FILETRANSPORTSERVICEIMPL_H_
#define FILETRANSPORTSERVICEIMPL_H_

#include "FileWorker.h"
#include "const.h"
#include "file.pb.h"
#include <grpcpp/support/sync_stream.h>
#include <memory>

class FileTransportServiceImpl final : public FileTransport::Service {
public:
    Status UploadFile(
        ServerContext* context, ServerReader<UploadRequest>* reader,
        UploadResponse* response) override;
    Status DownloadFile(
        ServerContext* context, const DownloadRequest* request,
        ServerWriter<DownloadResponse>* writer) override;
};


#endif   // FILETRANSPORTSERVICEIMPL_H_
