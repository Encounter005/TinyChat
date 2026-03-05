#ifndef MESSAGECACHEDB_H
#define MESSAGECACHEDB_H

#include <QObject>
#include <QSqlDatabase>

class MessageCacheDb
{
public:
    MessageCacheDb();
    bool OpenForOwner(int owneruid);
    QSqlDatabase Connection() const;
private:
    bool EnsureSchema();
    QSqlDatabase _db;
};

#endif // MESSAGECACHEDB_H

