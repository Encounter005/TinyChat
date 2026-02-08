#include "FileIndexManager.h"
#include "infra/ConfigManager.h"
#include "infra/LogManager.h"
#include "infra/RedisManager.h"
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

FileIndexManager::FileIndexManager() {
    _redis_key_prefix = "file:path:";
}

std::string FileIndexManager::format_key(const std::string& original_name) {
    return _redis_key_prefix + original_name;
}

void FileIndexManager::add_file(
    const std::string& original_name, const std::string& full_path_disk) {
    auto        redisManager = RedisManager::getInstance();
    std::string key          = format_key(original_name);
    if (!redisManager->Set(key, full_path_disk)) {
        LOG_ERROR("[FileIndex] Failed to set key: {}", key);
    }
}

std::string FileIndexManager::find_path(const std::string& original_name) {
    auto        redisManager = RedisManager::getInstance();
    std::string key          = format_key(original_name);
    std::string value        = "";
    redisManager->Get(key, value);
    return value;
}


void FileIndexManager::build_index_from_disk(
    const std::string& directory_path) {
    LOG_INFO(
        "[FileIndex] Starting to build index from disk directory: {}",
        directory_path);
    int count = 0;
    try {
        for (const auto& entry : fs::directory_iterator(directory_path)) {
            if (entry.is_regular_file()) {
                std::string disk_filename = entry.path().filename().string();
                size_t      pos           = disk_filename.find("_");
                if (pos != std::string::npos && pos > 0) {
                    std::string original_name = disk_filename.substr(pos + 1);
                    std::string path = directory_path + entry.path().filename().string();
                    add_file(original_name, path);
                    ++count;
                }
            }
        }
    } catch (std::exception& e) {
        LOG_ERROR(
            "[FileIndex] Failed to scan directory '{}': {}",
            directory_path,
            e.what());
    }
    LOG_INFO("[FileIndex] Index build complete. Indexed {} files.", count);
}
