#include <QtTest>
#include "messagesyncprotocol.h"

class MessageSyncProtocolTest : public QObject {
    Q_OBJECT
private slots:
    void buildHistoryRequestPayload();
    
};

void MessageSyncProtocolTest::buildHistoryRequestPayload() {
    const int uid = 1061;
    const int days = 7;
    const int limit = 50;
    QJsonObject obj = BuildHistoryRequest(uid, days, limit);
    QCOMPARE(obj["uid"].toInt(), uid);
    QCOMPARE(obj["days"].toInt(), days);
    QCOMPARE(obj["limit"].toInt(), limit);
}

QTEST_MAIN(MessageSyncProtocolTest)

#include "tst_messagesyncprotocol.moc"
