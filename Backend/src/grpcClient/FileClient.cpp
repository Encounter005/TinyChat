#include "FileClient.h"
#include "common/const.h"
#include "file.grpc.pb.h"
#include "file.pb.h"
#include "grpcClient/StatusClient.h"
#include "infra/ConfigManager.h"
#include "infra/LogManager.h"
#include "repository/FileRepository.h"
#include <openssl/md5.h>
#include <grpcpp/channel.h>
#include <grpcpp/client_context.h>
#include <grpcpp/create_channel.h>
#include <grpcpp/security/credentials.h>

using FileService::FileMeta;
using FileService::UploadRequest;
using FileService::UploadResponse;
using FileService::DownloadRequest;
using FileService::DownloadResponse;

std::string FileClient::CalcMD5(const std::vector<unsigned char>& bytes) {
    unsigned char digest[MD5_DIGEST_LENGTH];
    MD5(bytes.data(), bytes.size(), digest);
    static const char* hex = "0123456789abcdef";
    std::string        out;
    out.reserve(MD5_DIGEST_LENGTH * 2);
    for (int i = 0; i < MD5_DIGEST_LENGTH; ++i) {
        out.push_back(hex[(digest[i] >> 4) & 0x0F]);
        out.push_back(hex[digest[i] & 0x0F]);
    }
    return out;
}
FileClient::FileClient() {
    auto config = ConfigManager::getInstance();
    auto host   = (*config)["FileServer"]["host"];
    auto port   = (*config)["FileServer"]["port"];

    auto channel = grpc::CreateChannel(
        host + ":" + port, grpc::InsecureChannelCredentials());
    _stub = FileService::FileTransport::NewStub(channel);
}


Result<void> FileClient::UploadBytes(
    const std::string& file_name, const std::vector<unsigned char>& bytes) {

    if (file_name.empty() || bytes.empty()) {
        return Result<void>::Error(ErrorCodes::ERROR_JSON);
    }

    grpc::ClientContext         ctx;
    FileService::UploadResponse resp;
    auto                        writer = _stub->UploadFile(&ctx, &resp);
    if (!writer) {
        return Result<void>::Error(ErrorCodes::RPC_FAILED);
    }

    FileService::UploadRequest metaReq;
    auto*                      meta = metaReq.mutable_meta();
    meta->set_file_name(file_name);
    meta->set_total_size(static_cast<int64_t>(bytes.size()));
    meta->set_file_md5(CalcMD5(bytes));
    meta->set_resume_offset(0);

    if (!writer->Write(metaReq)) {
        writer->WritesDone();
        writer->Finish();
        return Result<void>::Error(ErrorCodes::RPC_FAILED);
    }

    constexpr size_t CHUNK = 1024 * 1024;
    for (size_t off = 0; off < bytes.size(); off += CHUNK) {
        const size_t               n = std::min(CHUNK, bytes.size() - off);
        FileService::UploadRequest chunkReq;
        chunkReq.set_chunk(
            reinterpret_cast<const char*>(bytes.data() + off), n);
        if (!writer->Write(chunkReq)) {
            writer->WritesDone();
            writer->Finish();
            return Result<void>::Error(ErrorCodes::RPC_FAILED);
        }
    }

    writer->WritesDone();
    const grpc::Status st = writer->Finish();
    if (!st.ok() || !resp.success()) {
        return Result<void>::Error(ErrorCodes::RPC_FAILED);
    }
    return Result<void>::OK();
}

Result<std::vector<unsigned char>> FileClient::DownloadBytes(
    const std::string& file_name) {
    if (file_name.empty()) {
        return Result<std::vector<unsigned char>>::Error(ErrorCodes::ERROR_JSON);
    }

    grpc::ClientContext ctx;
    DownloadRequest req;
    req.set_file_name(file_name);
    req.set_start_offset(0);

    auto reader = _stub->DownloadFile(&ctx, req);
    if (!reader) {
        return Result<std::vector<unsigned char>>::Error(ErrorCodes::RPC_FAILED);
    }

    DownloadResponse resp;
    std::vector<unsigned char> out;
    bool got_chunk = false;
    while (reader->Read(&resp)) {
        if (resp.has_chunk()) {
            const std::string& chunk = resp.chunk();
            if (!chunk.empty()) {
                out.insert(out.end(), chunk.begin(), chunk.end());
                got_chunk = true;
            }
        }
    }

    const grpc::Status status = reader->Finish();
    if (!status.ok() || !got_chunk) {
        return Result<std::vector<unsigned char>>::Error(ErrorCodes::RPC_FAILED);
    }
    return Result<std::vector<unsigned char>>::OK(std::move(out));
}
