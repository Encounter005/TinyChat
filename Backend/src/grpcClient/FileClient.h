#ifndef FILECLIENT_H_
#define FILECLIENT_H_

#include "common/result.h"
#include "common/singleton.h"
#include "file.grpc.pb.h"
#include <memory>
#include <string>
#include <vector>

class FileClient : public SingleTon<FileClient> {
    friend class SingleTon<FileClient>;

public:
    Result<void> UploadBytes(const std::string& file_name, const std::vector<unsigned char>& bytes);

private:
    FileClient();
    std::string CalcMD5(const std::vector<unsigned char>& bytes);
    std::unique_ptr<FileService::FileTransport::Stub> _stub;
};

#endif   // FILECLIENT_H_
