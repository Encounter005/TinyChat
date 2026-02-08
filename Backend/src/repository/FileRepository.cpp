#include "FileRepository.h"
#include "hiredis.h"
#include "infra/ConfigManager.h"
#include "infra/LogManager.h"
#include "infra/RedisManager.h"
#include "read.h"
#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <openssl/md5.h>
#include <string>

static constexpr const char* UPLOAD_PROGRESS_PREFIX = "upload_progress:";
static constexpr const char* UPLOAD_BLOCK_PREFIX    = "upload_blocks:";
static constexpr int         DEFAULT_EXPIRE_SECONDS = 24 * 3600;   // 24小时
static constexpr int         BLOCK_CHECKPOINT_EXPIRE_SECONDS = 7 * 24 * 3600;
using int64                                                  = int64_t;

std::string FileRepository::FormatProgressKey(const std::string& file_md5) {
    return std::string(UPLOAD_PROGRESS_PREFIX) + file_md5;
}

std::string FileRepository::FormatBlockKey(const std::string& file_md5) {
    return std::string(UPLOAD_BLOCK_PREFIX) + file_md5;
}

std::string FileRepository::FormatBlockFieldKey(int block_index) {
    return "block_" + std::to_string(block_index);
}

int FileRepository::GetCurrentTimestamp() {
    return std::chrono::duration_cast<std::chrono::seconds>(
               std::chrono::system_clock::now().time_since_epoch())
        .count();
}

std::string FileRepository::CalculateMD5(const std::string& data) {
    unsigned char digest[MD5_DIGEST_LENGTH];
    MD5((unsigned char*) data.data(), data.size(), digest);

    char hexDigest[MD5_DIGEST_LENGTH * 2 + 1];
    for (int i = 0; i < MD5_DIGEST_LENGTH; i++) {
        sprintf(&hexDigest[i * 2], "%02x", (unsigned int) digest[i]);
    }
    return std::string(hexDigest);
}

// ============================================================================
// 上传进度管��实现
// ============================================================================

Result<void> FileRepository::SaveUploadProgress(
    const std::string& file_md5, const std::string& file_name, int total_size,
    int uploaded_bytes, const std::string& status) {

    auto redisManager = RedisManager::getInstance();
    if (!redisManager) {
        LOG_ERROR("[FileRepository] RedisManager instance is null");
        return Result<void>::Error(ErrorCodes::REDIS_ERROR);
    }

    std::string key       = FormatProgressKey(file_md5);
    int         timestamp = GetCurrentTimestamp();

    auto hset_file_name = redisManager->HSet(key, "file_name", file_name);
    if (!hset_file_name) {
        LOG_ERROR(
            "[FileRepository] Failed to set file_name for md5: {}", file_md5);
        return Result<void>::Error(ErrorCodes::REDIS_ERROR);
    }

    auto hset_total_size
        = redisManager->HSet(key, "total_size", std::to_string(total_size));
    if (!hset_total_size) {
        LOG_ERROR(
            "[FileRepository] Failed to set total_size for md5: {}", file_md5);
        return Result<void>::Error(ErrorCodes::REDIS_ERROR);
    }

    auto hset_uploaded_bytes = redisManager->HSet(
        key, "uploaded_bytes", std::to_string(uploaded_bytes));
    if (!hset_uploaded_bytes) {
        LOG_ERROR(
            "[FileRepository] Failed to set uploaded_bytes for md5: {}",
            file_md5);
        return Result<void>::Error(ErrorCodes::REDIS_ERROR);
    }

    auto hset_status = redisManager->HSet(key, "status", status);
    if (!hset_status) {
        LOG_ERROR(
            "[FileRepository] Failed to set status for md5: {}", file_md5);
        return Result<void>::Error(ErrorCodes::REDIS_ERROR);
    }

    auto hset_updated_at
        = redisManager->HSet(key, "updated_at", std::to_string(timestamp));
    if (!hset_updated_at) {
        LOG_WARN(
            "[FileRepository] Failed to set updated_at for md5: {}", file_md5);
    }

    auto set_expire = SetUploadExpire(file_md5, DEFAULT_EXPIRE_SECONDS);
    if (!set_expire.IsOK()) {
        LOG_WARN(
            "[FileRepository] Failed to set expiration for md5: {}", file_md5);
    }

    LOG_INFO(
        "[FileRepository] Saved upload progress: md5={}, name={}, total={}, "
        "uploaded={}, status={}",
        file_md5,
        file_name,
        total_size,
        uploaded_bytes,
        status);

    return Result<void>::OK();
}

Result<UploadProgress> FileRepository::GetUploadProgress(
    const std::string& file_md5) {

    auto redisManager = RedisManager::getInstance();
    if (!redisManager) {
        LOG_ERROR("[FileRepository] RedisManager instance is null");
        return Result<UploadProgress>::Error(ErrorCodes::REDIS_ERROR);
    }

    std::string key = FormatProgressKey(file_md5);

    auto exists = ExistsUploadProgress(file_md5);
    if (!exists.IsOK() || !exists.Value()) {
        LOG_DEBUG(
            "[FileRepository] No upload progress found for md5: {}", file_md5);
        return Result<UploadProgress>::Error(ErrorCodes::REDIS_ERROR);
    }

    UploadProgress progress;
    progress.file_md5 = file_md5;

    auto file_name = redisManager->HGet(key, "file_name");
    if (file_name.empty()) {
        LOG_ERROR("[FileRepository] file_name not found for md5: {}", file_md5);
        return Result<UploadProgress>::Error(ErrorCodes::REDIS_ERROR);
    }
    progress.file_name = file_name;

    auto total_size_str = redisManager->HGet(key, "total_size");
    if (total_size_str.empty()) {
        LOG_ERROR(
            "[FileRepository] total_size not found for md5: {}", file_md5);
        return Result<UploadProgress>::Error(ErrorCodes::REDIS_ERROR);
    }
    progress.total_size = std::atoll(total_size_str.c_str());

    auto uploaded_bytes_str = redisManager->HGet(key, "uploaded_bytes");
    if (uploaded_bytes_str.empty()) {
        LOG_ERROR(
            "[FileRepository] uploaded_bytes not found for md5: {}", file_md5);
        return Result<UploadProgress>::Error(ErrorCodes::REDIS_ERROR);
    }
    progress.uploaded_bytes = std::atoll(uploaded_bytes_str.c_str());

    auto status = redisManager->HGet(key, "status");
    if (status.empty()) {
        LOG_ERROR("[FileRepository] status not found for md5: {}", file_md5);
        return Result<UploadProgress>::Error(ErrorCodes::REDIS_ERROR);
    }
    progress.status = status;

    auto updated_at_str = redisManager->HGet(key, "updated_at");
    if (!updated_at_str.empty()) {
        progress.updated_at = std::atoi(updated_at_str.c_str());
    }

    LOG_DEBUG(
        "[FileRepository] Retrieved upload progress: md5={}, uploaded={}, "
        "total={}, status={}",
        file_md5,
        progress.uploaded_bytes,
        progress.total_size,
        progress.status);

    return Result<UploadProgress>::OK(progress);
}

Result<void> FileRepository::UpdateUploadProgress(
    const std::string& file_md5, int uploaded_bytes) {

    auto redisManager = RedisManager::getInstance();
    if (!redisManager) {
        LOG_ERROR("[FileRepository] RedisManager instance is null");
        return Result<void>::Error(ErrorCodes::REDIS_ERROR);
    }

    std::string key = FormatProgressKey(file_md5);

    auto hset_uploaded = redisManager->HSet(
        key, "uploaded_bytes", std::to_string(uploaded_bytes));
    if (!hset_uploaded) {
        LOG_ERROR(
            "[FileRepository] Failed to update uploaded_bytes for md5: {}",
            file_md5);
        return Result<void>::Error(ErrorCodes::REDIS_ERROR);
    }

    int  timestamp = GetCurrentTimestamp();
    auto hset_updated
        = redisManager->HSet(key, "updated_at", std::to_string(timestamp));
    if (!hset_updated) {
        LOG_WARN(
            "[FileRepository] Failed to update updated_at for md5: {}",
            file_md5);
    }

    LOG_DEBUG(
        "[FileRepository] Updated upload progress: md5={}, uploaded_bytes={}",
        file_md5,
        uploaded_bytes);

    return Result<void>::OK();
}

Result<void> FileRepository::MarkUploadComplete(const std::string& file_md5) {
    auto redisManager = RedisManager::getInstance();
    if (!redisManager) {
        LOG_ERROR("[FileRepository] RedisManager instance is null");
        return Result<void>::Error(ErrorCodes::REDIS_ERROR);
    }

    std::string key = FormatProgressKey(file_md5);

    auto hset_status = redisManager->HSet(key, "status", "completed");
    if (!hset_status) {
        LOG_ERROR(
            "[FileRepository] Failed to mark upload complete for md5: {}",
            file_md5);
        return Result<void>::Error(ErrorCodes::REDIS_ERROR);
    }

    LOG_INFO("[FileRepository] Marked upload complete for md5: {}", file_md5);
    return Result<void>::OK();
}

Result<void> FileRepository::MarkUploadCancelled(const std::string& file_md5) {
    auto redisManager = RedisManager::getInstance();
    if (!redisManager) {
        LOG_ERROR("[FileRepository] RedisManager instance is null");
        return Result<void>::Error(ErrorCodes::REDIS_ERROR);
    }

    std::string key = FormatProgressKey(file_md5);

    auto hset_status = redisManager->HSet(key, "status", "cancelled");
    if (!hset_status) {
        LOG_ERROR(
            "[FileRepository] Failed to mark upload cancelled for md5: {}",
            file_md5);
        return Result<void>::Error(ErrorCodes::REDIS_ERROR);
    }

    LOG_INFO("[FileRepository] Marked upload cancelled for md5: {}", file_md5);
    return Result<void>::OK();
}

Result<void> FileRepository::DeleteUploadProgress(const std::string& file_md5) {
    auto redisManager = RedisManager::getInstance();
    if (!redisManager) {
        LOG_ERROR("[FileRepository] RedisManager instance is null");
        return Result<void>::Error(ErrorCodes::REDIS_ERROR);
    }

    std::string key = FormatProgressKey(file_md5);

    auto del_result = redisManager->Del(key);
    if (!del_result) {
        LOG_WARN(
            "[FileRepository] Failed to delete upload progress for md5: {} "
            "(key may not exist)",
            file_md5);
    }

    LOG_INFO("[FileRepository] Deleted upload progress for md5: {}", file_md5);
    return Result<void>::OK();
}

Result<bool> FileRepository::ExistsUploadProgress(const std::string& file_md5) {
    auto redisManager = RedisManager::getInstance();
    if (!redisManager) {
        LOG_ERROR("[FileRepository] RedisManager instance is null");
        return Result<bool>::Error(ErrorCodes::REDIS_ERROR);
    }

    std::string key    = FormatProgressKey(file_md5);
    bool        exists = redisManager->ExistsKey(key);

    return Result<bool>::OK(exists);
}

Result<int> FileRepository::GetUploadedBytes(const std::string& file_md5) {
    auto progress = GetUploadProgress(file_md5);
    if (!progress.IsOK()) {
        return Result<int>::Error(progress.Error());
    }
    return Result<int>::OK(progress.Value().uploaded_bytes);
}

Result<void> FileRepository::SetUploadExpire(
    const std::string& file_md5, int seconds) {

    auto redisManager = RedisManager::getInstance();
    if (!redisManager) {
        LOG_ERROR("[FileRepository] RedisManager instance is null");
        return Result<void>::Error(ErrorCodes::REDIS_ERROR);
    }

    std::string key = FormatProgressKey(file_md5);

    bool ok = redisManager->Expire(key, seconds);
    if (!ok) {
        LOG_WARN(
            "[FileRepository] Failed to set expiration for md5: {} "
            "(key may not exist)",
            file_md5);
        return Result<void>::Error(ErrorCodes::REDIS_ERROR);
    }

    LOG_DEBUG(
        "[FileRepository] Set expiration {} seconds for md5: {}",
        seconds,
        file_md5);

    return Result<void>::OK();
}

// ============================================================================
// 下载断点管理实现
// ============================================================================

std::string FileRepository::FormatDownloadProgressKey(
    const std::string& file_name, const std::string& session_id) {
    return "download_progress:" + file_name + ":" + session_id;
}

Result<void> FileRepository::SaveDownloadProgress(
    const std::string& file_name, const std::string& session_id,
    int64_t downloaded_bytes) {

    auto redisManager = RedisManager::getInstance();
    if (!redisManager) {
        LOG_ERROR("[FileRepository] RedisManager instance is null");
        return Result<void>::Error(ErrorCodes::REDIS_ERROR);
    }

    std::string key       = FormatDownloadProgressKey(file_name, session_id);
    int         timestamp = GetCurrentTimestamp();

    auto hset_file_name = redisManager->HSet(key, "file_name", file_name);
    if (!hset_file_name) {
        LOG_ERROR(
            "[FileRepository] Failed to set file_name for: {}", file_name);
        return Result<void>::Error(ErrorCodes::REDIS_ERROR);
    }

    auto hset_session_id = redisManager->HSet(key, "session_id", session_id);
    if (!hset_session_id) {
        LOG_ERROR(
            "[FileRepository] Failed to set session_id for: {}", session_id);
        return Result<void>::Error(ErrorCodes::REDIS_ERROR);
    }

    auto hset_downloaded = redisManager->HSet(
        key, "downloaded_bytes", std::to_string(downloaded_bytes));
    if (!hset_downloaded) {
        LOG_ERROR(
            "[FileRepository] Failed to set downloaded_bytes for: {}",
            downloaded_bytes);
        return Result<void>::Error(ErrorCodes::REDIS_ERROR);
    }

    auto hset_updated_at
        = redisManager->HSet(key, "updated_at", std::to_string(timestamp));
    if (!hset_updated_at) {
        LOG_WARN(
            "[FileRepository] Failed to set updated_at for: {}", file_name);
    }

    redisManager->Expire(key, DEFAULT_EXPIRE_SECONDS);

    LOG_INFO(
        "[FileRepository] Saved download progress: file={}, session={}, "
        "bytes={}",
        file_name,
        session_id,
        downloaded_bytes);

    return Result<void>::OK();
}

Result<DownloadProgress> FileRepository::GetDownloadProgress(
    const std::string& file_name, const std::string& session_id) {

    auto redisManager = RedisManager::getInstance();
    if (!redisManager) {
        LOG_ERROR("[FileRepository] RedisManager instance is null");
        return Result<DownloadProgress>::Error(ErrorCodes::REDIS_ERROR);
    }

    std::string key = FormatDownloadProgressKey(file_name, session_id);

    std::string retrieved_file_name = redisManager->HGet(key, "file_name");
    if (retrieved_file_name.empty()) {
        LOG_DEBUG(
            "[FileRepository] Download progress not found for: {}:{}",
            file_name,
            session_id);
        return Result<DownloadProgress>::Error(ErrorCodes::REDIS_ERROR);
    }

    DownloadProgress progress;
    progress.file_name  = retrieved_file_name;
    progress.session_id = session_id;

    auto downloaded_bytes_str = redisManager->HGet(key, "downloaded_bytes");
    if (downloaded_bytes_str.empty()) {
        LOG_ERROR("[FileRepository] downloaded_bytes not found for: {}", key);
        return Result<DownloadProgress>::Error(ErrorCodes::REDIS_ERROR);
    }
    progress.downloaded_bytes = std::atoll(downloaded_bytes_str.c_str());

    auto updated_at_str = redisManager->HGet(key, "updated_at");
    if (!updated_at_str.empty()) {
        progress.updated_at = std::atoi(updated_at_str.c_str());
    }

    return Result<DownloadProgress>::OK(progress);
}

Result<void> FileRepository::UpdateDownloadProgress(
    const std::string& file_name, const std::string& session_id,
    int64_t downloaded_bytes) {

    auto redisManager = RedisManager::getInstance();
    if (!redisManager) {
        LOG_ERROR("[FileRepository] RedisManager instance is null");
        return Result<void>::Error(ErrorCodes::REDIS_ERROR);
    }

    std::string key = FormatDownloadProgressKey(file_name, session_id);

    auto hset_downloaded = redisManager->HSet(
        key, "downloaded_bytes", std::to_string(downloaded_bytes));
    if (!hset_downloaded) {
        LOG_ERROR(
            "[FileRepository] Failed to update downloaded_bytes for: {}:{}",
            file_name,
            session_id);
        return Result<void>::Error(ErrorCodes::REDIS_ERROR);
    }

    int  timestamp = GetCurrentTimestamp();
    auto hset_updated_at
        = redisManager->HSet(key, "updated_at", std::to_string(timestamp));
    if (!hset_updated_at) {
        LOG_WARN(
            "[FileRepository] Failed to update updated_at for::{}",
            file_name,
            session_id);
    }

    LOG_DEBUG(
        "[FileRepository] Updated download progress: file={}, session={}, "
        "bytes={}",
        file_name,
        session_id,
        downloaded_bytes);

    return Result<void>::OK();
}

Result<void> FileRepository::DeleteDownloadProgress(
    const std::string& file_name, const std::string& session_id) {

    auto redisManager = RedisManager::getInstance();
    if (!redisManager) {
        LOG_ERROR("[FileRepository] RedisManager instance is null");
        return Result<void>::Error(ErrorCodes::REDIS_ERROR);
    }

    std::string key = FormatDownloadProgressKey(file_name, session_id);

    auto del_result = redisManager->Del(key);

    if (!del_result) {
        LOG_WARN(
            "[FileRepository] Failed to delete download progress for: {}:{}",
            file_name,
            session_id);
    }

    LOG_INFO(
        "[FileRepository] Deleted download progress: file={}, session={}",
        file_name,
        session_id);

    return Result<void>::OK();
}

Result<bool> FileRepository::ExistsDownloadProgress(
    const std::string& file_name, const std::string& session_id) {

    auto redisManager = RedisManager::getInstance();
    if (!redisManager) {
        LOG_ERROR("[FileRepository] RedisManager instance is null");
        return Result<bool>::Error(ErrorCodes::REDIS_ERROR);
    }

    std::string key    = FormatDownloadProgressKey(file_name, session_id);
    bool        exists = redisManager->ExistsKey(key);

    return Result<bool>::OK(exists);
}

Result<void> FileRepository::SetDownloadExpire(
    const std::string& file_name, const std::string& session_id, int seconds) {

    auto redisManager = RedisManager::getInstance();
    if (!redisManager) {
        LOG_ERROR("[FileRepository] RedisManager instance is null");
        return Result<void>::Error(ErrorCodes::REDIS_ERROR);
    }

    std::string key = FormatDownloadProgressKey(file_name, session_id);

    bool ok = redisManager->Expire(key, seconds);
    if (!ok) {
        LOG_WARN(
            "[FileRepository] Failed to set expiration for download: {}:{} "
            "(key may not exist)",
            file_name,
            session_id);
        return Result<void>::Error(ErrorCodes::REDIS_ERROR);
    }

    LOG_DEBUG(
        "[FileRepository] Set download expiration {} seconds for {}:{}",
        seconds,
        file_name,
        session_id);

    return Result<void>::OK();
}

Result<std::vector<ChunkInfo>> FileRepository::GetUploadedChunks(
    const std::string& file_md5, int block_size) {

    auto redisManager = RedisManager::getInstance();
    if (!redisManager) {
        LOG_ERROR("[FileRepository] RedisManager instance is null");
        return Result<std::vector<ChunkInfo>>::Error(ErrorCodes::REDIS_ERROR);
    }

    std::string key        = FormatBlockKey(file_md5);
    auto        all_blocks = redisManager->HGetAll(key);

    if (all_blocks.empty()) {
        return Result<std::vector<ChunkInfo>>::OK(std::vector<ChunkInfo>{});
    }

    std::vector<ChunkInfo> chunks;
    for (const auto& [field, value] : all_blocks) {
        if (field.find("block_") == 0) {
            int   index  = std::atoi(field.substr(6).c_str());
            int64 offset = index * block_size;
            chunks.push_back({index, value, offset});
        }
    }

    std::sort(
        chunks.begin(),
        chunks.end(),
        [](const ChunkInfo& a, const ChunkInfo& b) {
            return a.chunk_index < b.chunk_index;
        });

    LOG_DEBUG(
        "[FileRepository] Retrieved {} uploaded chunks for md5: {}",
        chunks.size(),
        file_md5);

    return Result<std::vector<ChunkInfo>>::OK(chunks);
}

// ============================================================================
// 块校验点管理实现
// ============================================================================

Result<void> FileRepository::SaveBlockCheckpoint(
    const std::string& file_md5, int block_index,
    const std::string& block_md5) {

    auto redisManager = RedisManager::getInstance();
    if (!redisManager) {
        LOG_ERROR("[FileRepository] RedisManager instance is null");
        return Result<void>::Error(ErrorCodes::REDIS_ERROR);
    }

    std::string key   = FormatBlockKey(file_md5);
    std::string field = FormatBlockFieldKey(block_index);

    auto hset_result = redisManager->HSet(key, field, block_md5);
    if (!hset_result) {
        LOG_ERROR(
            "[FileRepository] Failed to save block checkpoint: md5={}, "
            "block={}",
            file_md5,
            block_index);
        return Result<void>::Error(ErrorCodes::REDIS_ERROR);
    }

    // 设置块校验点的过期时间
    redisManager->Expire(key, BLOCK_CHECKPOINT_EXPIRE_SECONDS);

    LOG_DEBUG(
        "[FileRepository] Saved block checkpoint: md5={}, block={}, md5={}",
        file_md5,
        block_index,
        block_md5);

    return Result<void>::OK();
}

Result<std::string> FileRepository::GetBlockCheckpoint(
    const std::string& file_md5, int block_index) {

    auto redisManager = RedisManager::getInstance();
    if (!redisManager) {
        LOG_ERROR("[FileRepository] RedisManager instance is null");
        return Result<std::string>::Error(ErrorCodes::REDIS_ERROR);
    }

    std::string key   = FormatBlockKey(file_md5);
    std::string field = FormatBlockFieldKey(block_index);

    std::string block_md5 = redisManager->HGet(key, field);
    if (block_md5.empty()) {
        LOG_DEBUG(
            "[FileRepository] Block checkpoint not found: md5={}, block={}",
            file_md5,
            block_index);
        return Result<std::string>::Error(ErrorCodes::REDIS_ERROR);
    }

    LOG_DEBUG(
        "[FileRepository] Retrieved block checkpoint: md5={}, block={}, md5={}",
        file_md5,
        block_index,
        block_md5);

    return Result<std::string>::OK(block_md5);
}

Result<int> FileRepository::VerifyUploadBlocks(
    const std::string& file_md5, int reported_offset, int block_size) {

    auto redisManager = RedisManager::getInstance();
    if (!redisManager) {
        LOG_ERROR("[FileRepository] RedisManager instance is null");
        return Result<int>::Error(ErrorCodes::REDIS_ERROR);
    }

    std::string key        = FormatBlockKey(file_md5);
    auto        all_blocks = redisManager->HGetAll(key);

    if (all_blocks.empty()) {
        LOG_DEBUG(
            "[FileRepository] No block checkpoints found for md5: {}",
            file_md5);
        return Result<int>::OK(0);
    }

    int expected_block_index = reported_offset / block_size;
    int completed_count      = 0;

    for (int i = 0; i < expected_block_index; ++i) {
        std::string field = FormatBlockFieldKey(i);
        if (all_blocks.find(field) != all_blocks.end()) {
            completed_count++;
        }
    }

    LOG_DEBUG(
        "[FileRepository] Verified {} blocks for md5={}, offset={}",
        completed_count,
        file_md5,
        reported_offset);

    return Result<int>::OK(completed_count);
}

Result<void> FileRepository::DeleteBlockCheckpoints(
    const std::string& file_md5) {

    auto redisManager = RedisManager::getInstance();
    if (!redisManager) {
        LOG_ERROR("[FileRepository] RedisManager instance is null");
        return Result<void>::Error(ErrorCodes::REDIS_ERROR);
    }

    std::string key = FormatBlockKey(file_md5);

    auto del_result = redisManager->Del(key);
    if (!del_result) {
        LOG_WARN(
            "[FileRepository] Failed to delete block checkpoints for md5: {} "
            "(key may not exist)",
            file_md5);
    }

    LOG_INFO(
        "[FileRepository] Deleted block checkpoints for md5: {}", file_md5);

    return Result<void>::OK();
}

Result<int> FileRepository::CountCompletedBlocks(
    const std::string& file_md5, int block_size) {

    auto redisManager = RedisManager::getInstance();
    if (!redisManager) {
        LOG_ERROR("[FileRepository] RedisManager instance is null");
        return Result<int>::Error(ErrorCodes::REDIS_ERROR);
    }

    std::string key        = FormatBlockKey(file_md5);
    auto        all_blocks = redisManager->HGetAll(key);

    if (all_blocks.empty()) {
        return Result<int>::OK(0);
    }

    int count = 0;
    for (const auto& [field, value] : all_blocks) {
        if (field.find("block_") == 0) {
            count++;
        }
    }

    LOG_DEBUG(
        "[FileRepository] Counted {} completed blocks for md5: {}",
        count,
        file_md5);

    return Result<int>::OK(count);
}
