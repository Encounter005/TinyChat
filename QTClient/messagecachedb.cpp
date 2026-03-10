#include "messagecachedb.h"
#include "datapaths.h"
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSqlQuery>

namespace {
void MoveIfNeeded(const QString& src, const QString& dst) {
    if (src == dst) {
        return;
    }

    QFileInfo srcInfo(src);
    if (!srcInfo.exists()) {
        return;
    }

    QFileInfo dstInfo(dst);
    if (dstInfo.exists()) {
        return;
    }

    QDir().mkpath(QFileInfo(dst).absolutePath());
    QFile::rename(src, dst);
}

void MigrateLegacyChatCache() {
    const QString legacyDb = DataPaths::QtClientRootDir() + QDir::separator() + "chat_cache.db";
    const QString targetDb = DataPaths::ChatCacheDbPath();

    MoveIfNeeded(legacyDb, targetDb);
    MoveIfNeeded(legacyDb + "-wal", targetDb + "-wal");
    MoveIfNeeded(legacyDb + "-shm", targetDb + "-shm");
}
}

MessageCacheDb::MessageCacheDb() {}

bool MessageCacheDb::OpenForOwner(int owneruid)
{
    Q_UNUSED(owneruid);
    MigrateLegacyChatCache();
    const QString dbPath = DataPaths::ChatCacheDbPath();
    _db = QSqlDatabase::addDatabase("QSQLITE", "chat_cache");
    _db.setDatabaseName(dbPath);
    if(!_db.open()) {
        return false;
    }

    QSqlQuery pragma(_db);
    pragma.exec("PRAGMA journal_mode=WAL");
    return EnsureSchema();

}

QSqlDatabase MessageCacheDb::Connection() const
{
    return _db;
}

bool MessageCacheDb::EnsureSchema()
{
    QSqlQuery  query(_db);
    return query.exec(
               "CREATE TABLE IF NOT EXISTS chat_message_cache ("
               "owner_uid INTEGER NOT NULL,"
               "peer_uid INTEGER NOT NULL,"
               "msg_id TEXT NOT NULL,"
               "from_uid INTEGER NOT NULL,"
               "to_uid INTEGER NOT NULL,"
               "content TEXT NOT NULL,"
               "msg_ts INTEGER NOT NULL,"
               "source INTEGER NOT NULL,"
               "PRIMARY KEY(owner_uid, msg_id)"
               ")")
           && query.exec("CREATE INDEX IF NOT EXISTS idx_owner_ts ON chat_message_cache(owner_uid, msg_ts)")
           && query.exec("CREATE INDEX IF NOT EXISTS idx_owner_peer_ts ON chat_message_cache(owner_uid, peer_uid, msg_ts)");

}
