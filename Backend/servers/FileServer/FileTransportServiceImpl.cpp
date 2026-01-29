#include "FileTransportServiceImpl.h"
#include "FileWorkerPool.h"
#include "TaskQueue.h"
#include "infra/LogManager.h"
#include <grpcpp/support/status.h>

Status FileTransportServiceImpl::UploadFile(
    ServerContext* context, ServerReader<UploadRequest>* reader,
    UploadResponse* response) {
    UploadRequest request;
    std::string   current_md5;
    std::string   current_filename;
    bool          has_meta   = false;

    while (reader->Read(&request)) {
        if (request.has_meta()) {
            has_meta         = true;
            const auto& meta = request.meta();
            current_md5      = meta.file_md5();
            current_filename = meta.file_name();

            FileWorkerPool::getInstance()->Submit(
                FsTask::createMeta(current_filename, current_md5));
        } else if(request.has_chunk()) {
            if(!has_meta) {
                return Status(grpc::INVALID_ARGUMENT, "Meta First");
            }

            FileWorkerPool::getInstance()->Submit(FsTask::createData(current_md5, request.chunk()));
        }
    }

    if(has_meta) {
        FileWorkerPool::getInstance()->Submit(FsTask::createEnd(current_md5));
    }

    //  返回响应
    response->set_success(true);
    response->set_message("Upload received successfully.");
    return Status::OK;
}


Status FileTransportServiceImpl::DownloadFile(
    ServerContext* context, const DownloadRequest* request,
    ServerWriter<DownloadResponse>* writer) {

    return Status::OK;
}
