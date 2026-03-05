#include "messagecachedb.h"
#include <QCoreApplication>
#include <QDir>
#include <QSqlQuery>

namespace {
QString ResolveQtClientDirPath() {
    QDir current(QCoreApplication::applicationDirPath());
    while (true) {
        if (current.exists("QTClient.pro")) {
            return current.absolutePath();
        }

        if (current.exists("QTClient/QTClient.pro")) {
            return current.absoluteFilePath("QTClient");
        }

        if (!current.cdUp()) {
            break;
        }
    }

    return QCoreApplication::applicationDirPath();
}
}

MessageCacheDb::MessageCacheDb() {}

bool MessageCacheDb::OpenForOwner(int owneruid)
{
    Q_UNUSED(owneruid);
    const QString dir = ResolveQtClientDirPath();
    QDir().mkpath(dir);

    const QString dbPath = dir + QDir::separator() + "chat_cache.db";
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
