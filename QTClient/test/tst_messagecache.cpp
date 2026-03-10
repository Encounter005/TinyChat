#include <QtTest>
#include <QSqlQuery>
#include "messagecachedb.h"

class MessageCacheDbTest : public QObject {
    Q_OBJECT
private slots:
    void opensAndCreatesSchema();
};

void MessageCacheDbTest::opensAndCreatesSchema() {
    MessageCacheDb db;
    QVERIFY(db.OpenForOwner(1001));
    QSqlQuery query(db.Connection());
    QVERIFY(query.exec("SELECT name FROM sqlite_master WHERE type='table' AND name='chat_message_cache'"));
    QVERIFY(query.next());
    QCOMPARE(query.value(0).toString(), QString("chat_message_cache"));
}

QTEST_MAIN(MessageCacheDbTest)
#include "tst_messagecachedb.moc"

