#ifndef MESSAGEPERSISTENCEREPOSITORY_H_
#define MESSAGEPERSISTENCEREPOSITORY_H_

#include "common/result.h"
#include <vector>
class MessagePersistenceRepository {
public:
    static Result<void> SaveChatMessage(
        int from_uid, int to_uid, const std::string& msg_json);
    static Result<std::vector<std::string>> GetMessagesFromCache(int from_uid, int to_uid, int count);
    static Result<void> RemovePersistedMessages(int from_uid, int to_uid, int count);
    static Result<void> BatchInsertToMySQL(const std::string& table_name, const std::vector<std::string>& messages);

    static Result<std::vector<std::pair<int, int>>> GetAllChatQueues();
    static int GetChatMessageTable(int from_uid, int to_uid);
    static std::string GetChatMessageTableName(int from_uid, int to_uid);
private:
    static const std::string CHAT_MSG_PREFIX;
    static const std::string CHAT_META_PREFIX;
};


#endif   // MESSAGEPERSISTENCEREPOSITORY_H_
