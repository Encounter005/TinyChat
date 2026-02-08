#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <iomanip>
#include <grpcpp/grpcpp.h>
#include "file.grpc.pb.h"
#include <openssl/md5.h>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

using grpc::Channel;
using grpc::ClientContext;
using grpc::ClientReader;
using grpc::Status;

using FileService::FileTransport;
using FileService::DownloadRequest;
using FileService::DownloadResponse;
using FileService::QueryDownloadRequest;
using FileService::DownloadStatus;

using int64 = long long;

// ============================================================================
// 工具函数
// ============================================================================

std::string calculateFileMD5(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) return "";

    MD5_CTX md5Context;
    MD5_Init(&md5Context);
    const int bufferSize = 8192;
    std::vector<char> buffer(bufferSize);

    while (file) {
        file.read(buffer.data(), bufferSize);
        MD5_Update(&md5Context, buffer.data(), file.gcount());
    }

    unsigned char digest[MD5_DIGEST_LENGTH];
    MD5_Final(digest, &md5Context);
    char hexDigest[MD5_DIGEST_LENGTH * 2 + 1];
    for (int i = 0; i < MD5_DIGEST_LENGTH; i++) {
        sprintf(&hexDigest[i * 2], "%02x", (unsigned int)digest[i]);
    }
    hexDigest[MD5_DIGEST_LENGTH * 2] = '\0';
    return std::string(hexDigest);
}

std::string generateUUID() {
    boost::uuids::random_generator gen;
    boost::uuids::uuid uuid = gen();
    return boost::uuids::to_string(uuid);
}

std::string formatBytes(int64 bytes) {
    const char* units[] = {"B", "KB", "MB", "GB"};
    int unitIndex = 0;
    double size = bytes;

    while (size >= 1024 && unitIndex < 3) {
        size /= 1024;
        unitIndex++;
    }

    char buffer[50];
    sprintf(buffer, "%.2f %s", size, units[unitIndex]);
    return std::string(buffer);
}

// ============================================================================
// 下载客户端类
// ============================================================================

class DownloadClient {
public:
    DownloadClient(std::shared_ptr<Channel> channel)
        : stub_(FileTransport::NewStub(channel)) {}

    // 查询下载断点
    bool queryDownloadStatus(const std::string& fileName,
                             const std::string& sessionId,
                             DownloadStatus& status) {
        ClientContext context;
        QueryDownloadRequest request;
        request.set_file_name(fileName);
        request.set_session_id(sessionId);

        Status rpcStatus = stub_->QueryDownloadStatus(&context, request, &status);

        if (!rpcStatus.ok()) {
            std::cerr << "[-] QueryDownloadStatus failed: "
                      << rpcStatus.error_message() << std::endl;
            return false;
        }

        return true;
    }

    // 下载文件
    bool downloadFile(const std::string& fileName,
                      const std::string& outputPath,
                      const std::string& sessionId = "") {
        std::string usedSessionId = sessionId.empty() ? generateUUID() : sessionId;

        std::cout << "\n========================================" << std::endl;
        std::cout << "[*] Downloading: " << fileName << std::endl;
        std::cout << "[*] Output: " << outputPath << std::endl;
        std::cout << "[*] Session ID: " << usedSessionId << std::endl;
        std::cout << "========================================" << std::endl;

        // 查询断点
        DownloadStatus downloadStatus;
        int64 resumeOffset = 0;

        if (queryDownloadStatus(fileName, usedSessionId, downloadStatus)) {
            if (downloadStatus.has_breakpoint()) {
                resumeOffset = downloadStatus.resume_offset();
                std::cout << "[✓] Breakpoint found at: "
                          << formatBytes(resumeOffset) << std::endl;
                std::cout << "[i] Total file size: "
                          << formatBytes(downloadStatus.file_size()) << std::endl;
            }
        }

        // 打开输出文件
        std::ofstream outFile;
        if (resumeOffset > 0) {
            outFile.open(outputPath, std::ios::binary | std::ios::app);
            std::cout << "[i] Resuming from offset: " << formatBytes(resumeOffset) << std::endl;
        } else {
            outFile.open(outputPath, std::ios::binary);
        }

        if (!outFile.is_open()) {
            std::cerr << "[-] Failed to open output file" << std::endl;
            return false;
        }

        // 创建下载流
        ClientContext context;
        DownloadRequest request;
        request.set_file_name(fileName);
        request.set_start_offset(resumeOffset);

        std::unique_ptr<ClientReader<DownloadResponse>> reader(
            stub_->DownloadFile(&context, request));

        // 接收数据
        DownloadResponse response;
        int64 totalDownloaded = resumeOffset;
        int64 fileSize = 0;
        int64 lastReportOffset = resumeOffset;

        std::cout << "[<<<] Downloading..." << std::endl;

        while (reader->Read(&response)) {
            if (response.has_meta()) {
                fileSize = response.meta().total_size();
                std::cout << "[i] File size: " << formatBytes(fileSize) << std::endl;
            } else if (response.has_chunk()) {
                const std::string& chunk = response.chunk();
                outFile.write(chunk.data(), chunk.size());
                totalDownloaded += chunk.size();

                // 显示进度
                if (totalDownloaded - lastReportOffset >= 10 * 1024 * 1024) {
                    double progress = fileSize > 0 ?
                        (double)totalDownloaded / fileSize * 100.0 : 0.0;
                    std::cout << "\r    Progress: " << std::fixed << std::setprecision(1)
                              << progress << "% (" << formatBytes(totalDownloaded);
                    if (fileSize > 0) {
                        std::cout << "/" << formatBytes(fileSize);
                    }
                    std::cout << ")" << std::flush;
                    lastReportOffset = totalDownloaded;
                }
            }
        }

        outFile.close();

        // 检查结果
        Status status = reader->Finish();

        if (status.ok()) {
            std::cout << "\n[OK] Download completed!" << std::endl;
            std::cout << "[i] Total downloaded: " << formatBytes(totalDownloaded) << std::endl;

            // 计算MD5
            std::string downloadedMd5 = calculateFileMD5(outputPath);
            std::cout << "[i] MD5: " << downloadedMd5 << std::endl;

            return true;
        } else {
            std::cerr << "\n[FAIL] Download failed: " << status.error_message() << std::endl;
            return false;
        }
    }

private:
    std::unique_ptr<FileTransport::Stub> stub_;
};

// ============================================================================
// 主程序
// ============================================================================

int main(int argc, char** argv) {
    if (argc < 3) {
        std::cout << "Usage: " << argv[0] << " <filename> <output_path> [server_address] [session_id]\n"
                  << "\nArguments:"
                  << "\n  filename       Name of the file to download from server"
                  << "\n  output_path    Where to save the downloaded file"
                  << "\n  server_address Server address (default: 0.0.0.0:50051)"
                  << "\n  session_id     Optional session ID for resuming download"
                  << std::endl;
        return 1;
    }

    std::string fileName = argv[1];
    std::string outputPath = argv[2];
    std::string serverAddress = argc > 3 ? argv[3] : "0.0.0.0:50051";
    std::string sessionId = argc > 4 ? argv[4] : "";

    auto channel = grpc::CreateChannel(serverAddress, grpc::InsecureChannelCredentials());
    DownloadClient client(channel);

    std::cout << "[*] Server: " << serverAddress << std::endl;
    std::cout << "========================================" << std::endl;

    bool success = client.downloadFile(fileName, outputPath, sessionId);

    return success ? 0 : 1;
}
