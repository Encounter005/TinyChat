#ifndef MESSAGECACHEREPOSITORY_H
#define MESSAGECACHEREPOSITORY_H

#include "messagecachedb.h"
#include "userdata.h"
#include <vector>

class MessageCacheRepository
{
public:
    explicit MessageCacheRepository(MessageCacheDb& db);
    std::vector<std::shared_ptr<TextChatData>> LoadByOwner(int ownerUid);
    bool SaveOne(int ownerUid, const TextChatData& msg, qint64 ts);
private:
    int ResolvePeerUid(int owneruid, const TextChatData& msg) const;
    MessageCacheDb& _db;
};

#endif // MESSAGECACHEREPOSITORY_H
