#ifndef DOWNLOADCALLDATA_H_
#define DOWNLOADCALLDATA_H_

#include "CallData.h"
#include "const.h"
#include "file.grpc.pb.h"
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <cstdint>
#include <fstream>
#include <vector>

class DownloadCallData : public CallData {
public:
    DownloadCallData(
        FileService::FileTransport::AsyncService* service,
        grpc::ServerCompletionQueue*              cq);
    void Proceed(bool ok) override;

private:
    std::string GenerateSessionID();   // 新增：生成会话ID

private:
    FileService::FileTransport::AsyncService*              _service;
    grpc::ServerCompletionQueue*                           _cq;
    grpc::ServerContext                                    _context;
    FileService::DownloadRequest                           _request;
    grpc::ServerAsyncWriter<FileService::DownloadResponse> _writer;
    CallState                                              _state;
    std::ifstream                                          _file_stream;
    std::vector<char>                                      _buffer;

    // 新增成员变量
    std::string              _session_id;           // 下载会话ID
    int64_t                  _current_offset = 0;   // 当前发送位置
    int64_t                  _start_offset   = 0;   // 起始偏移量
    int64_t                  _file_size      = 0;   // 文件总大小
    static constexpr int64_t SAVE_BREAKPOINT_INTERVAL
        = 20 * 1024 * 1024;   // 20MB
};

#endif   // DOWNLOADCALLDATA_H_
