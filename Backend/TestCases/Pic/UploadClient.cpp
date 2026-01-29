#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <cstring>
#include <filesystem> // C++17 标准库，用于目录遍历
#include <grpcpp/grpcpp.h>
#include "file.grpc.pb.h"

// OpenSSL for MD5 calculation
#include <openssl/md5.h>

namespace fs = std::filesystem; // 别名简化代码

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;
using FileService::FileTransport;
using FileService::UploadRequest;
using FileService::UploadResponse;
using FileService::FileMeta;

// --- 保留你之前的工具函数 ---

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

long getFileSize(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary | std::ios::ate);
    if(file.is_open()) {
        return file.tellg();
    }
    return -1;
}

// --- 修改后的核心上传函数 ---

bool uploadSingleFile(FileTransport::Stub* stub, const fs::path& filePath) {
    ClientContext context;
    UploadResponse response;
    std::string filenameStr = filePath.string();

    // 1. 准备元数据
    std::string md5Hash = calculateFileMD5(filenameStr);
    long fileSize = getFileSize(filenameStr);
    std::string basename = filePath.filename().string(); // 自动提取文件名

    if (md5Hash.empty() || fileSize < 0) {
        std::cerr << "[-] Skip file (Access error): " << basename << std::endl;
        return false;
    }

    // 创建流
    std::unique_ptr<grpc::ClientWriter<UploadRequest>> writer(stub->UploadFile(&context, &response));

    // 2. 发送元数据
    UploadRequest metaRequest;
    FileMeta* meta = metaRequest.mutable_meta();
    meta->set_file_name(basename);
    meta->set_total_size(fileSize);
    meta->set_file_md5(md5Hash);

    if (!writer->Write(metaRequest)) {
        std::cerr << "[-] Failed to send metadata for: " << basename << std::endl;
        return false;
    }

    std::cout << "[>>>] Uploading: " << basename << " (" << fileSize << " bytes)" << std::endl;

    // 3. 分块发送内容
    std::ifstream file(filenameStr, std::ios::binary);
    const int chunkSize = 1024 * 1024 * 2; // 1MB
    std::vector<char> buffer(chunkSize);

    while (file) {
        file.read(buffer.data(), chunkSize);
        int bytesRead = file.gcount();
        if (bytesRead > 0) {
            UploadRequest chunkRequest;
            chunkRequest.set_chunk(buffer.data(), bytesRead);
            if (!writer->Write(chunkRequest)) break;
        }
    }
    file.close();

    // 4. 结束并获取结果
    writer->WritesDone();
    Status status = writer->Finish();
    if (status.ok() && response.success()) {
        std::cout << "[ OK ] Finished: " << basename << std::endl;
        return true;
    } else {
        std::cerr << "[FAIL] Error: " << status.error_message() << std::endl;
        return false;
    }
}

// --- 新增：遍历文件夹逻辑 ---

void uploadDirectory(FileTransport::Stub* stub, const std::string& dirPath) {
    if (!fs::exists(dirPath) || !fs::is_directory(dirPath)) {
        std::cerr << "Error: " << dirPath << " is not a valid directory." << std::endl;
        return;
    }

    int successCount = 0;
    int failCount = 0;

    // 递归遍历文件夹 (recursive_directory_iterator)
    for (const auto& entry : fs::recursive_directory_iterator(dirPath)) {
        if (entry.is_regular_file()) { // 只上传普通文件，跳过子目录本身
            if (uploadSingleFile(stub, entry.path())) {
                successCount++;
            } else {
                failCount++;
            }
        }
    }

    std::cout << "\n--- Upload Summary ---" << std::endl;
    std::cout << "Success: " << successCount << std::endl;
    std::cout << "Failed:  " << failCount << std::endl;
}

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <directory_path> [server_address]" << std::endl;
        return 1;
    }

    std::string targetDir = argv[1];
    std::string server_address = (argc > 2) ? argv[2] : "0.0.0.0:50051";

    auto channel = grpc::CreateChannel(server_address, grpc::InsecureChannelCredentials());
    auto stub = FileTransport::NewStub(channel);

    std::cout << "[*] Target Directory: " << targetDir << std::endl;
    std::cout << "[*] Server Address:    " << server_address << std::endl;
    std::cout << "------------------------------------------" << std::endl;

    uploadDirectory(stub.get(), targetDir);

    return 0;
}
