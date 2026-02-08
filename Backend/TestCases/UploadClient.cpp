#include "file.grpc.pb.h"
#include <chrono>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <grpcpp/grpcpp.h>
#include <iostream>
#include <random>
#include <string>
#include <thread>
#include <vector>

// OpenSSL for MD5 calculation
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <openssl/md5.h>

namespace fs = std::filesystem;

using grpc::Channel;
using grpc::ClientContext;
using grpc::ClientReader;
using grpc::ClientWriter;
using grpc::Status;

using FileService::ChunkInfo;
using FileService::DownloadRequest;
using FileService::DownloadResponse;
using FileService::DownloadStatus;
using FileService::FileMeta;
using FileService::FileTransport;
using FileService::QueryDownloadRequest;
using FileService::QueryUploadRequest;
using FileService::UploadRequest;
using FileService::UploadResponse;
using FileService::UploadStatus;

using int64 = long long;
// ============================================================================
// 工具函数
// ============================================================================

std::string calculateFileMD5(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) return "";

    MD5_CTX md5Context;
    MD5_Init(&md5Context);
    const int         bufferSize = 8192;
    std::vector<char> buffer(bufferSize);

    while (file) {
        file.read(buffer.data(), bufferSize);
        MD5_Update(&md5Context, buffer.data(), file.gcount());
    }

    unsigned char digest[MD5_DIGEST_LENGTH];
    MD5_Final(digest, &md5Context);
    char hexDigest[MD5_DIGEST_LENGTH * 2 + 1];
    for (int i = 0; i < MD5_DIGEST_LENGTH; i++) {
        sprintf(&hexDigest[i * 2], "%02x", (unsigned int) digest[i]);
    }
    hexDigest[MD5_DIGEST_LENGTH * 2] = '\0';
    return std::string(hexDigest);
}

// 计算数据块的MD5
std::string calculateDataMD5(const std::vector<char>& data) {
    MD5_CTX md5Context;
    MD5_Init(&md5Context);
    MD5_Update(&md5Context, data.data(), data.size());

    unsigned char digest[MD5_DIGEST_LENGTH];
    MD5_Final(digest, &md5Context);
    char hexDigest[MD5_DIGEST_LENGTH * 2 + 1];
    for (int i = 0; i < MD5_DIGEST_LENGTH; i++) {
        sprintf(&hexDigest[i * 2], "%02x", (unsigned int) digest[i]);
    }
    hexDigest[MD5_DIGEST_LENGTH * 2] = '\0';
    return std::string(hexDigest);
}

int64 getFileSize(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary | std::ios::ate);
    if (file.is_open()) {
        return file.tellg();
    }
    return -1;
}

std::string generateUUID() {
    boost::uuids::random_generator gen;
    boost::uuids::uuid             uuid = gen();
    return boost::uuids::to_string(uuid);
}

// 格式化字节大小
std::string formatBytes(int64 bytes) {
    const char* units[]   = {"B", "KB", "MB", "GB"};
    int         unitIndex = 0;
    double      size      = bytes;

    while (size >= 1024 && unitIndex < 3) {
        size /= 1024;
        unitIndex++;
    }

    char buffer[50];
    sprintf(buffer, "%.2f %s", size, units[unitIndex]);
    return std::string(buffer);
}

// ============================================================================
// 断点上传客户端
// ============================================================================

class ResumableUploadClient {
public:
    ResumableUploadClient(std::shared_ptr<Channel> channel)
        : stub_(FileTransport::NewStub(channel)) {}

    // 查询上传断点
    bool queryUploadStatus(const std::string& fileMd5, UploadStatus& status) {
        ClientContext      context;
        QueryUploadRequest request;
        request.set_file_md5(fileMd5);

        Status rpcStatus = stub_->QueryUploadStatus(&context, request, &status);

        if (!rpcStatus.ok()) {
            std::cerr << "[-] QueryUploadStatus failed: "
                      << rpcStatus.error_message() << std::endl;
            return false;
        }

        return true;
    }

    // 上传单个文件（支持断点续传）
    bool uploadFile(
        const std::string& filePath, bool simulateFailure = false,
        int failureAtMB = 0) {
        std::string filename = fs::path(filePath).filename().string();
        std::string fileMd5  = calculateFileMD5(filePath);
        int64       fileSize = getFileSize(filePath);

        if (fileMd5.empty() || fileSize < 0) {
            std::cerr << "[-] Cannot access file: " << filename << std::endl;
            return false;
        }

        std::cout << "\n========================================" << std::endl;
        std::cout << "[*] File: " << filename << std::endl;
        std::cout << "[*] Size: " << formatBytes(fileSize) << std::endl;
        std::cout << "[*] MD5:  " << fileMd5 << std::endl;
        std::cout << "========================================" << std::endl;

        // 1. 查询断点
        UploadStatus uploadStatus;
        bool         hasBreakpoint = false;
        int64        resumeOffset  = 0;

        if (queryUploadStatus(fileMd5, uploadStatus)) {
            if (uploadStatus.has_breakpoint()) {
                hasBreakpoint = true;
                resumeOffset  = uploadStatus.resume_offset();

                std::cout << "[✓] Breakpoint found!" << std::endl;
                std::cout << "    Resume offset: " << formatBytes(resumeOffset)
                          << std::endl;
                std::cout << "    Chunks uploaded: "
                          << uploadStatus.uploaded_chunks_size() << std::endl;

                // 验证已上传分块的MD5
                for (int i = 0; i < uploadStatus.uploaded_chunks_size(); i++) {
                    const ChunkInfo& chunk = uploadStatus.uploaded_chunks(i);
                    std::cout << "    - Chunk " << chunk.chunk_index() << ": "
                              << chunk.chunk_md5().substr(0, 8) << "..."
                              << std::endl;
                }
            } else {
                std::cout << "[i] No breakpoint found, starting fresh upload"
                          << std::endl;
            }
        }

        // 2. 打开文件并定位
        std::ifstream file(filePath, std::ios::binary);
        if (!file.is_open()) {
            std::cerr << "[-] Failed to open file" << std::endl;
            return false;
        }

        if (resumeOffset > 0) {
            file.seekg(resumeOffset);
            std::cout << "[i] Resuming from offset: "
                      << formatBytes(resumeOffset) << std::endl;
        }

        // 3. 创建上传流
        ClientContext                                context;
        UploadResponse                               response;
        std::unique_ptr<ClientWriter<UploadRequest>> writer(
            stub_->UploadFile(&context, &response));

        // 4. 发送元数据
        UploadRequest metaRequest;
        FileMeta*     meta = metaRequest.mutable_meta();
        meta->set_file_name(filename);
        meta->set_total_size(fileSize);
        meta->set_file_md5(fileMd5);
        meta->set_resume_offset(resumeOffset);

        if (!writer->Write(metaRequest)) {
            std::cerr << "[-] Failed to send metadata" << std::endl;
            return false;
        }

        // 5. 分块上传
        const int         chunkSize = 2 * 1024 * 1024;   // 2MB
        std::vector<char> buffer(chunkSize);
        int64             totalUploaded    = resumeOffset;
        int64             lastReportOffset = resumeOffset;

        std::cout << "[>>>] Uploading..." << std::endl;

        while (file) {
            file.read(buffer.data(), chunkSize);
            int bytesRead = file.gcount();

            if (bytesRead > 0) {
                UploadRequest chunkRequest;
                chunkRequest.set_chunk(buffer.data(), bytesRead);

                if (!writer->Write(chunkRequest)) {
                    std::cerr << "[-] Write failed" << std::endl;
                    break;
                }

                totalUploaded += bytesRead;

                // 显示进度
                if (totalUploaded - lastReportOffset
                    >= 10 * 1024 * 1024) {   // 每10MB显示一次
                    double progress = (double) totalUploaded / fileSize * 100.0;
                    std::cout << "\r    Progress: " << std::fixed
                              << std::setprecision(1) << progress << "% ("
                              << formatBytes(totalUploaded) << "/"
                              << formatBytes(fileSize) << ")" << std::flush;
                    lastReportOffset = totalUploaded;
                }

                // 模拟中断（测试用）
                if (simulateFailure && failureAtMB > 0
                    && totalUploaded >= (int64) failureAtMB * 1024 * 1024) {
                    std::cout << "\n[!] Simulating failure at "
                              << formatBytes(totalUploaded) << std::endl;
                    return false;   // 中断上传
                }
            }
        }

        file.close();

        // 6. 完成上传
        writer->WritesDone();
        Status status = writer->Finish();

        if (status.ok() && response.success()) {
            std::cout << "\n[OK] Upload completed successfully!" << std::endl;
            return true;
        } else {
            std::cerr << "\n[FAIL] Upload failed: " << status.error_message()
                      << std::endl;
            return false;
        }
    }

private:
    std::unique_ptr<FileTransport::Stub> stub_;
};

// ============================================================================
// 断点下载客户端
// ============================================================================

class ResumableDownloadClient {
public:
    ResumableDownloadClient(std::shared_ptr<Channel> channel)
        : stub_(FileTransport::NewStub(channel)) {}

    // 查询下载断点
    bool queryDownloadStatus(
        const std::string& fileName, const std::string& sessionId,
        DownloadStatus& status) {
        ClientContext        context;
        QueryDownloadRequest request;
        request.set_file_name(fileName);
        request.set_session_id(sessionId);

        Status rpcStatus
            = stub_->QueryDownloadStatus(&context, request, &status);

        if (!rpcStatus.ok()) {
            std::cerr << "[-] QueryDownloadStatus failed: "
                      << rpcStatus.error_message() << std::endl;
            return false;
        }

        return true;
    }

    // 下载文件（支持断点续传）
    bool downloadFile(
        const std::string& fileName, const std::string& outputPath,
        bool simulateFailure = false, int failureAtMB = 0) {
        std::string sessionId = generateUUID();

        std::cout << "\n========================================" << std::endl;
        std::cout << "[*] Downloading: " << fileName << std::endl;
        std::cout << "[*] Session ID: " << sessionId << std::endl;
        std::cout << "[*] Output: " << outputPath << std::endl;
        std::cout << "========================================" << std::endl;

        // 1. 查询断点
        DownloadStatus downloadStatus;
        bool           hasBreakpoint = false;
        int64          resumeOffset  = 0;

        if (queryDownloadStatus(fileName, sessionId, downloadStatus)) {
            if (downloadStatus.has_breakpoint()) {
                hasBreakpoint = true;
                resumeOffset  = downloadStatus.resume_offset();

                std::cout << "[✓] Breakpoint found!" << std::endl;
                std::cout << "    Resume offset: " << formatBytes(resumeOffset)
                          << std::endl;
                std::cout << "    File size: "
                          << formatBytes(downloadStatus.file_size())
                          << std::endl;
            } else {
                std::cout << "[i] No breakpoint found, starting fresh download"
                          << std::endl;
            }
        } else {
            // 首次下载，获取文件大小
            resumeOffset = 0;
        }

        // 2. 打开输出文件
        std::ofstream outFile;
        if (resumeOffset > 0) {
            outFile.open(outputPath, std::ios::binary | std::ios::app);
            std::cout << "[i] Resuming download at offset: "
                      << formatBytes(resumeOffset) << std::endl;
        } else {
            outFile.open(outputPath, std::ios::binary);
        }

        if (!outFile.is_open()) {
            std::cerr << "[-] Failed to open output file" << std::endl;
            return false;
        }

        // 3. 创建下载流
        ClientContext   context;
        DownloadRequest request;
        request.set_file_name(fileName);
        request.set_start_offset(resumeOffset);

        std::unique_ptr<ClientReader<DownloadResponse>> reader(
            stub_->DownloadFile(&context, request));

        // 4. 接收数据
        DownloadResponse response;
        int64            totalDownloaded  = resumeOffset;
        int64            fileSize         = 0;
        bool             firstChunk       = true;
        int64            lastReportOffset = resumeOffset;

        std::cout << "[<<<] Downloading..." << std::endl;

        while (reader->Read(&response)) {
            if (response.has_meta()) {
                // 接收到元数据
                fileSize = response.meta().total_size();
                std::cout << "[i] File size: " << formatBytes(fileSize)
                          << std::endl;
            } else if (response.has_chunk()) {
                // 接收到数据块
                const std::string& chunk = response.chunk();
                outFile.write(chunk.data(), chunk.size());
                totalDownloaded += chunk.size();

                // 显示进度
                if (totalDownloaded - lastReportOffset >= 10 * 1024 * 1024) {
                    double progress = fileSize > 0 ? (double) totalDownloaded
                                                         / fileSize * 100.0
                                                   : 0.0;
                    std::cout << "\r    Progress: " << std::fixed
                              << std::setprecision(1) << progress << "% ("
                              << formatBytes(totalDownloaded);
                    if (fileSize > 0) {
                        std::cout << "/" << formatBytes(fileSize);
                    }
                    std::cout << ")" << std::flush;
                    lastReportOffset = totalDownloaded;
                }

                // 模拟中断
                if (simulateFailure && failureAtMB > 0
                    && totalDownloaded >= (int64) failureAtMB * 1024 * 1024) {
                    std::cout << "\n[!] Simulating failure at "
                              << formatBytes(totalDownloaded) << std::endl;
                    outFile.close();
                    return false;
                }
            }
        }

        outFile.close();

        // 5. 检查结果
        Status status = reader->Finish();

        if (status.ok()) {
            std::cout << "\n[OK] Download completed successfully!" << std::endl;
            std::cout << "[i] Total downloaded: "
                      << formatBytes(totalDownloaded) << std::endl;

            // 计算下载文件的MD5
            std::string downloadedMd5 = calculateFileMD5(outputPath);
            std::cout << "[i] Downloaded MD5: " << downloadedMd5 << std::endl;

            return true;
        } else {
            std::cerr << "\n[FAIL] Download failed: " << status.error_message()
                      << std::endl;
            return false;
        }
    }

private:
    std::unique_ptr<FileTransport::Stub> stub_;
};

// ============================================================================
// 主程序
// ============================================================================

void printUsage(const char* programName) {
    std::cout << "Usage: " << programName << " <command> [options]\n"
              << "\nCommands:\n"
              << "  upload <file>              Upload a single file\n"
              << "  upload-dir <directory>     Upload all files in directory\n"
              << "  upload-test <file>         Test resumable upload (simulate "
                 "failure)\n"
              << "  download <file> <output>   Download a file\n"
              << "  download-test <file> <out> Test resumable download "
                 "(simulate failure)\n"
              << "\nOptions:\n"
              << "  --server <address>         Server address (default: "
                 "0.0.0.0:50051)\n"
              << "  --fail-at <MB>             Simulate failure at N MB\n"
              << std::endl;
}

int main(int argc, char** argv) {
    if (argc < 2) {
        printUsage(argv[0]);
        return 1;
    }

    std::string command       = argv[1];
    std::string serverAddress = "0.0.0.0:50051";
    int         failureAtMB   = 0;

    // 解析选项
    for (int i = 2; i < argc; i++) {
        if (std::string(argv[i]) == "--server" && i + 1 < argc) {
            serverAddress = argv[++i];
        } else if (std::string(argv[i]) == "--fail-at" && i + 1 < argc) {
            failureAtMB = std::atoi(argv[++i]);
        }
    }

    // 创建gRPC通道
    auto channel = grpc::CreateChannel(
        serverAddress, grpc::InsecureChannelCredentials());

    std::cout << "[*] Server: " << serverAddress << std::endl;
    std::cout << "========================================" << std::endl;

    if (command == "upload" && argc >= 3) {
        // 上传单个文件
        ResumableUploadClient client(channel);
        std::string           filePath = argv[2];
        return client.uploadFile(filePath) ? 0 : 1;

    } else if (command == "upload-dir" && argc >= 3) {
        // 上传整个目录
        ResumableUploadClient client(channel);
        std::string           dirPath = argv[2];

        if (!fs::exists(dirPath) || !fs::is_directory(dirPath)) {
            std::cerr << "[-] Invalid directory: " << dirPath << std::endl;
            return 1;
        }

        int successCount = 0;
        int failCount    = 0;

        for (const auto& entry : fs::recursive_directory_iterator(dirPath)) {
            if (entry.is_regular_file()) {
                if (client.uploadFile(entry.path().string())) {
                    successCount++;
                } else {
                    failCount++;
                }
            }
        }

        std::cout << "\n========================================" << std::endl;
        std::cout << "Upload Summary:" << std::endl;
        std::cout << "  Success: " << successCount << std::endl;
        std::cout << "  Failed:  " << failCount << std::endl;
        std::cout << "========================================" << std::endl;

        return 0;

    } else if (command == "upload-test" && argc >= 3) {
        // 测试断点续传上传（模拟失败）
        ResumableUploadClient client(channel);
        std::string           filePath = argv[2];

        std::cout << "[TEST MODE] Simulating failure at " << failureAtMB
                  << " MB" << std::endl;

        // 第一次上传（会失败）
        std::cout << "\n--- First upload attempt (will fail) ---" << std::endl;
        client.uploadFile(filePath, true, failureAtMB);

        // 等待用户按键继续
        std::cout << "\n[Press Enter to resume upload...]";
        std::cin.get();

        // 第二次上传（断点续传）
        std::cout << "\n--- Second upload attempt (resume) ---" << std::endl;
        bool success = client.uploadFile(filePath, false, 0);

        if (success) {
            std::cout << "\n[TEST PASSED] Resumable upload works!" << std::endl;
        } else {
            std::cout << "\n[TEST FAILED] Resumable upload failed!"
                      << std::endl;
        }

        return success ? 0 : 1;

    } else if (command == "download" && argc >= 4) {
        // 下载单个文件
        ResumableDownloadClient client(channel);
        std::string             fileName   = argv[2];
        std::string             outputPath = argv[3];

        return client.downloadFile(fileName, outputPath) ? 0 : 1;

    } else if (command == "download-test" && argc >= 4) {
        // 测试断点续传下载（模拟失败）
        ResumableDownloadClient client(channel);
        std::string             fileName   = argv[2];
        std::string             outputPath = argv[3];

        std::cout << "[TEST MODE] Simulating failure at " << failureAtMB
                  << " MB" << std::endl;

        // 第一次下载（会失败）
        std::cout << "\n--- First download attempt (will fail) ---"
                  << std::endl;
        client.downloadFile(fileName, outputPath, true, failureAtMB);

        // 等待用户按键继续
        std::cout << "\n[Press Enter to resume download...]";
        std::cin.get();

        // 第二次下载（断点续传）
        std::cout << "\n--- Second download attempt (resume) ---" << std::endl;
        bool success = client.downloadFile(fileName, outputPath, false, 0);

        if (success) {
            std::cout << "\n[TEST PASSED] Resumable download works!"
                      << std::endl;
        } else {
            std::cout << "\n[TEST FAILED] Resumable download failed!"
                      << std::endl;
        }

        return success ? 0 : 1;

    } else {
        printUsage(argv[0]);
        return 1;
    }
}
