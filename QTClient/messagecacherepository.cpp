#include "messagecacherepository.h"
#include "qsqlquery.h"

namespace {
qint64 NormalizeUnixTs(qint64 ts)
{
    if (ts <= 0) {
        return 0;
    }
    if (ts >= 1000000000000LL) {
        return ts / 1000;
    }
    return ts;
}
}


MessageCacheRepository::MessageCacheRepository(MessageCacheDb &db) : _db(db)
{

}

std::vector<std::shared_ptr<TextChatData> > MessageCacheRepository::LoadByOwner(int ownerUid)
{
    std::vector<std::shared_ptr<TextChatData>> result;
    QSqlQuery query(_db.Connection());
    query.prepare("SELECT msg_id, content, from_uid, to_uid, msg_ts FROM chat_message_cache "
                  "WHERE owner_uid = ? ORDER BY msg_ts ASC");
    query.addBindValue(ownerUid);
    if(!query.exec()) {
        return result;
    }

    while(query.next()) {
        const QString msgId = query.value(0).toString();
        const QString content = query.value(1).toString();
        const int fromUid = query.value(2).toInt();
        const int toUid = query.value(3).toInt();
        const qint64 ts = NormalizeUnixTs(query.value(4).toLongLong());
        result.push_back(
            std::make_shared<TextChatData>(msgId, content, fromUid, toUid, ts));
    }
    return result;

}

bool MessageCacheRepository::SaveOne(int ownerUid, const TextChatData &msg, qint64 ts)
{
    const int peerUid = ResolvePeerUid(ownerUid, msg);
    QSqlQuery query(_db.Connection());
    query.prepare("INSERT OR IGNORE INTO chat_message_cache"
                  "(owner_uid, peer_uid, msg_id, from_uid, to_uid, content, msg_ts, source)"
                  "VALUES(?, ?, ?, ?, ?, ?, ?, ?)");
    query.addBindValue(ownerUid);
    query.addBindValue(peerUid);
    query.addBindValue(msg._msg_id);
    query.addBindValue(msg._from_uid);
    query.addBindValue(msg._to_uid);
    query.addBindValue(msg._msg_content);
    query.addBindValue(NormalizeUnixTs(ts));
    query.addBindValue(0);
    return query.exec();

}

int MessageCacheRepository::ResolvePeerUid(int owneruid, const TextChatData &msg) const
{
    return (msg._from_uid == owneruid) ? msg._to_uid : msg._from_uid;
}
