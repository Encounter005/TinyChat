#ifndef FILEREPOSITORY_H_
#define FILEREPOSITORY_H_

#include "common/const.h"
#include "common/result.h"
#include "common/singleton.h"
#include <cstdint>
#include <vector>

// 上传进度结构
struct UploadProgress {
    std::string file_md5;
    std::string file_name;
    int         total_size;
    int         uploaded_bytes;
    std::string status;   // "uploading" | "completed" | "cancelled"
    int         updated_at;
};

// 下载进度结构
struct DownloadProgress {
    std::string file_name;
    std::string session_id;
    int64_t     downloaded_bytes;
    int         updated_at;
};

// 分块信息结构
struct ChunkInfo {
    int32_t     chunk_index;
    std::string chunk_md5;
    int64_t     chunk_offset;
};

class FileRepository : public SingleTon<FileRepository> {
    friend class SingleTon<FileRepository>;

public:
    // 块校验相关常量
    static constexpr int DEFAULT_BLOCK_SIZE = 1024 * 1024;        // 1MB
    static constexpr int RESUME_BLOCK_SIZE  = 20 * 1024 * 1024;   // 20MB

    static Result<void> SaveUploadProgress(
        const std::string& file_md5, const std::string& file_name,
        int total_size, int uploaded_bytes,
        const std::string& status = "uploading");

    static Result<UploadProgress> GetUploadProgress(
        const std::string& file_md5);

    static Result<void> UpdateUploadProgress(
        const std::string& file_md5, int uploaded_bytes);

    static Result<void> MarkUploadComplete(const std::string& file_md5);
    static Result<void> MarkUploadCancelled(const std::string& file_md5);
    static Result<void> DeleteUploadProgress(const std::string& file_md5);
    static Result<bool> ExistsUploadProgress(const std::string& file_md5);
    static Result<int>  GetUploadedBytes(const std::string& file_md5);
    static Result<void> SetUploadExpire(
        const std::string& file_md5, int seconds);

    static Result<void> SaveBlockCheckpoint(
        const std::string& file_md5, int block_index,
        const std::string& block_md5);

    static Result<std::string> GetBlockCheckpoint(
        const std::string& file_md5, int block_index);

    static Result<int> VerifyUploadBlocks(
        const std::string& file_md5, int reported_offset,
        int block_size = DEFAULT_BLOCK_SIZE);

    static Result<void> DeleteBlockCheckpoints(const std::string& file_md5);
    static Result<int>  CountCompletedBlocks(
         const std::string& file_md5, int block_size = DEFAULT_BLOCK_SIZE);

    // 下载断点管理方法
    static Result<void> SaveDownloadProgress(
        const std::string& file_name, const std::string& session_id,
        int64_t downloaded_bytes);

    static Result<DownloadProgress> GetDownloadProgress(
        const std::string& file_name, const std::string& session_id);

    static Result<void> UpdateDownloadProgress(
        const std::string& file_name, const std::string& session_id,
        int64_t downloaded_bytes);

    static Result<void> DeleteDownloadProgress(
        const std::string& file_name, const std::string& session_id);

    // 辅助方法
    static Result<std::vector<ChunkInfo>> GetUploadedChunks(
        const std::string& file_md5, int block_size = RESUME_BLOCK_SIZE);

    // 下载进度辅助方法
    static Result<bool> ExistsDownloadProgress(
        const std::string& file_name, const std::string& session_id);
    static Result<void> SetDownloadExpire(
        const std::string& file_name, const std::string& session_id,
        int seconds);

private:
    FileRepository() = default;

    static std::string FormatProgressKey(const std::string& file_md5);
    static std::string FormatBlockKey(const std::string& file_md5);
    static std::string FormatBlockFieldKey(int block_index);
    static std::string FormatDownloadProgressKey(
        const std::string& file_name, const std::string& session_id);
    static int         GetCurrentTimestamp();
    static std::string CalculateMD5(const std::string& data);
};


#endif   // FILEREPOSITORY_H_
