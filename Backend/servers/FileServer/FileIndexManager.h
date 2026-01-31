#ifndef FILEINDEXMANAGER_H_
#define FILEINDEXMANAGER_H_

#include "common/singleton.h"
#include "infra/RedisManager.h"

class FileIndexManager : public SingleTon<FileIndexManager> {
    friend class SingleTon<FileIndexManager>;

public:
    // @brief 将文件索引添加到 Redis
    // @original_name: "gdb-1.md"
    // @full_path_on_disk: "./uploads/aecfb..._gdb-1.md"
    void add_file(
        const std::string& original_name, const std::string& full_path_on_disk);

    // @brief 从redis中查找文件的完整路径
    // @return 如果找到，返回完整路径，没有返回空字符串
    std::string find_path(const std::string& original_name);


    // 在服务器启动时，从磁盘扫描并构建/恢复索引
    void build_index_from_disk(const std::string& directory_path);

private:
    FileIndexManager();
    std::string format_key(const std::string& original_name);

private:
    std::string _redis_key_prefix;
};

#endif   // FILEINDEXMANAGER_H_
