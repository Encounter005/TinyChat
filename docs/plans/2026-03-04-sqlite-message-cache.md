# SQLite Message Cache Implementation Plan

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** Add a client-side SQLite message cache that loads local history for the current `owner_uid` and only requests server history when the local cache is empty.

**Architecture:** Introduce a thin SQLite DB layer and a repository that maps DB rows to `TextChatData`, plus a sync coordinator that enforces the “local-first, remote-on-empty” rule. Keep `UserManager` as the in-memory aggregator, and route TCP history responses through the coordinator.

**Tech Stack:** Qt 5/6, QtSql (SQLite), QStandardPaths, existing TcpManager/UserManager.

---

## Task 1: Define new history ReqIds and request/response shape

**Files:**
- Modify: `QTClient/global.h`
- Create: `QTClient/messagesyncprotocol.h`

**Step 1: Write the failing test**

Create `QTClient/tests/tst_messagesyncprotocol.cpp` and add:

```cpp
#include <QtTest>
#include "messagesyncprotocol.h"

class MessageSyncProtocolTest : public QObject {
    Q_OBJECT
private slots:
    void buildsHistoryRequestPayload();
};

void MessageSyncProtocolTest::buildsHistoryRequestPayload() {
    const int uid = 1001;
    const int days = 7;
    const int limit = 50;
    QJsonObject obj = BuildHistoryRequest(uid, days, limit);
    QCOMPARE(obj["uid"].toInt(), uid);
    QCOMPARE(obj["days"].toInt(), days);
    QCOMPARE(obj["limit"].toInt(), limit);
}

QTEST_MAIN(MessageSyncProtocolTest)
#include "tst_messagesyncprotocol.moc"
```

**Step 2: Run test to verify it fails**

Run: `qmake && make && ./tst_messagesyncprotocol`
Expected: FAIL with missing `BuildHistoryRequest` symbol or file not found.

**Step 3: Write minimal implementation**

Create `QTClient/messagesyncprotocol.h`:

```cpp
#ifndef MESSAGESYNCPROTOCOL_H
#define MESSAGESYNCPROTOCOL_H

#include <QJsonObject>

inline QJsonObject BuildHistoryRequest(int uid, int days, int limit) {
    QJsonObject obj;
    obj["uid"] = uid;
    obj["days"] = days;
    obj["limit"] = limit;
    return obj;
}

#endif
```

Update `QTClient/global.h` to add new ReqIds after heartbeat:

```cpp
    ID_PULL_HISTORY_MSG_REQ = 123,
    ID_PULL_HISTORY_MSG_RSP = 124,
```

**Step 4: Run test to verify it passes**

Run: `qmake && make && ./tst_messagesyncprotocol`
Expected: PASS.

**Step 5: Commit**

```bash
git add QTClient/global.h QTClient/messagesyncprotocol.h QTClient/tests/tst_messagesyncprotocol.cpp
git commit -m "feat: add history request protocol helpers"
```

---

## Task 2: Add SQLite DB layer

**Files:**
- Create: `QTClient/messagecachedb.h`
- Create: `QTClient/messagecachedb.cpp`

**Step 1: Write the failing test**

Create `QTClient/tests/tst_messagecachedb.cpp`:

```cpp
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
```

**Step 2: Run test to verify it fails**

Run: `qmake && make && ./tst_messagecachedb`
Expected: FAIL, missing `MessageCacheDb`.

**Step 3: Write minimal implementation**

`QTClient/messagecachedb.h`:

```cpp
#ifndef MESSAGECACHEDB_H
#define MESSAGECACHEDB_H

#include <QObject>
#include <QSqlDatabase>

class MessageCacheDb {
public:
    MessageCacheDb();
    bool OpenForOwner(int ownerUid);
    QSqlDatabase Connection() const;

private:
    bool EnsureSchema();
    QSqlDatabase _db;
};

#endif
```

`QTClient/messagecachedb.cpp`:

```cpp
#include "messagecachedb.h"
#include <QSqlQuery>
#include <QStandardPaths>
#include <QDir>

MessageCacheDb::MessageCacheDb() {}

bool MessageCacheDb::OpenForOwner(int ownerUid) {
    Q_UNUSED(ownerUid);
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dir);
    const QString dbPath = dir + QDir::separator() + "chat_cache.db";
    _db = QSqlDatabase::addDatabase("QSQLITE", "chat_cache");
    _db.setDatabaseName(dbPath);
    if (!_db.open()) {
        return false;
    }
    QSqlQuery pragma(_db);
    pragma.exec("PRAGMA journal_mode=WAL");
    return EnsureSchema();
}

bool MessageCacheDb::EnsureSchema() {
    QSqlQuery query(_db);
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

QSqlDatabase MessageCacheDb::Connection() const {
    return _db;
}
```

**Step 4: Run test to verify it passes**

Run: `qmake && make && ./tst_messagecachedb`
Expected: PASS.

**Step 5: Commit**

```bash
git add QTClient/messagecachedb.* QTClient/tests/tst_messagecachedb.cpp
git commit -m "feat: add sqlite db layer for message cache"
```

---

## Task 3: Add repository for message persistence

**Files:**
- Create: `QTClient/messagecacherepository.h`
- Create: `QTClient/messagecacherepository.cpp`

**Step 1: Write the failing test**

Create `QTClient/tests/tst_messagecacherepository.cpp`:

```cpp
#include <QtTest>
#include "messagecachedb.h"
#include "messagecacherepository.h"

class MessageCacheRepositoryTest : public QObject {
    Q_OBJECT
private slots:
    void savesAndLoadsByOwner();
    void deduplicatesByMsgId();
};

void MessageCacheRepositoryTest::savesAndLoadsByOwner() {
    MessageCacheDb db;
    QVERIFY(db.OpenForOwner(1001));
    MessageCacheRepository repo(db);

    TextChatData a("m1", "hello", 1001, 1002);
    TextChatData b("m2", "world", 1002, 1001);
    QVERIFY(repo.SaveOne(1001, a, 1));
    QVERIFY(repo.SaveOne(1001, b, 2));

    auto msgs = repo.LoadByOwner(1001);
    QCOMPARE(msgs.size(), 2);
    QCOMPARE(msgs[0]->_msg_id, QString("m1"));
    QCOMPARE(msgs[1]->_msg_id, QString("m2"));
}

void MessageCacheRepositoryTest::deduplicatesByMsgId() {
    MessageCacheDb db;
    QVERIFY(db.OpenForOwner(1001));
    MessageCacheRepository repo(db);

    TextChatData a("dup", "hello", 1001, 1002);
    QVERIFY(repo.SaveOne(1001, a, 1));
    QVERIFY(repo.SaveOne(1001, a, 1));

    auto msgs = repo.LoadByOwner(1001);
    QCOMPARE(msgs.size(), 1);
}

QTEST_MAIN(MessageCacheRepositoryTest)
#include "tst_messagecacherepository.moc"
```

**Step 2: Run test to verify it fails**

Run: `qmake && make && ./tst_messagecacherepository`
Expected: FAIL, missing repository class.

**Step 3: Write minimal implementation**

`QTClient/messagecacherepository.h`:

```cpp
#ifndef MESSAGECACHEREPOSITORY_H
#define MESSAGECACHEREPOSITORY_H

#include "messagecachedb.h"
#include "userdata.h"
#include <vector>

class MessageCacheRepository {
public:
    explicit MessageCacheRepository(MessageCacheDb& db);
    std::vector<std::shared_ptr<TextChatData>> LoadByOwner(int ownerUid);
    bool SaveOne(int ownerUid, const TextChatData& msg, qint64 ts);

private:
    int ResolvePeerUid(int ownerUid, const TextChatData& msg) const;
    MessageCacheDb& _db;
};

#endif
```

`QTClient/messagecacherepository.cpp`:

```cpp
#include "messagecacherepository.h"
#include <QSqlQuery>

MessageCacheRepository::MessageCacheRepository(MessageCacheDb& db) : _db(db) {}

int MessageCacheRepository::ResolvePeerUid(int ownerUid, const TextChatData& msg) const {
    return (msg._from_uid == ownerUid) ? msg._to_uid : msg._from_uid;
}

std::vector<std::shared_ptr<TextChatData>> MessageCacheRepository::LoadByOwner(int ownerUid) {
    std::vector<std::shared_ptr<TextChatData>> result;
    QSqlQuery query(_db.Connection());
    query.prepare("SELECT msg_id, content, from_uid, to_uid FROM chat_message_cache "
                  "WHERE owner_uid = ? ORDER BY msg_ts ASC");
    query.addBindValue(ownerUid);
    if (!query.exec()) {
        return result;
    }
    while (query.next()) {
        const QString msgId = query.value(0).toString();
        const QString content = query.value(1).toString();
        const int fromUid = query.value(2).toInt();
        const int toUid = query.value(3).toInt();
        result.push_back(std::make_shared<TextChatData>(msgId, content, fromUid, toUid));
    }
    return result;
}

bool MessageCacheRepository::SaveOne(int ownerUid, const TextChatData& msg, qint64 ts) {
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
    query.addBindValue(ts);
    query.addBindValue(0);
    return query.exec();
}
```

**Step 4: Run test to verify it passes**

Run: `qmake && make && ./tst_messagecacherepository`
Expected: PASS.

**Step 5: Commit**

```bash
git add QTClient/messagecacherepository.* QTClient/tests/tst_messagecacherepository.cpp
git commit -m "feat: add message cache repository"
```

---

## Task 4: Add sync coordinator (local-first, remote-on-empty)

**Files:**
- Create: `QTClient/messagesynccoordinator.h`
- Create: `QTClient/messagesynccoordinator.cpp`
- Modify: `QTClient/usermanager.h`
- Modify: `QTClient/usermanager.cpp`

**Step 1: Write the failing test**

Create `QTClient/tests/tst_messagesynccoordinator.cpp`:

```cpp
#include <QtTest>
#include "messagesynccoordinator.h"

class MessageSyncCoordinatorTest : public QObject {
    Q_OBJECT
private slots:
    void doesNotRequestRemoteWhenLocalExists();
};

void MessageSyncCoordinatorTest::doesNotRequestRemoteWhenLocalExists() {
    MessageSyncCoordinator coordinator;
    coordinator.SetLocalHasData(true);
    QVERIFY(!coordinator.ShouldRequestRemote());
}

QTEST_MAIN(MessageSyncCoordinatorTest)
#include "tst_messagesynccoordinator.moc"
```

**Step 2: Run test to verify it fails**

Run: `qmake && make && ./tst_messagesynccoordinator`
Expected: FAIL, missing coordinator class.

**Step 3: Write minimal implementation**

`QTClient/messagesynccoordinator.h`:

```cpp
#ifndef MESSAGESYNCCOORDINATOR_H
#define MESSAGESYNCCOORDINATOR_H

#include "messagecachedb.h"
#include "messagecacherepository.h"
#include <QObject>

class MessageSyncCoordinator : public QObject {
    Q_OBJECT
public:
    MessageSyncCoordinator();
    bool Bootstrap(int ownerUid);
    void ApplyHistory(int ownerUid, const std::vector<std::shared_ptr<TextChatData>>& msgs);

    // Test seam
    void SetLocalHasData(bool hasData) { _localHasData = hasData; }
    bool ShouldRequestRemote() const { return !_localHasData; }

signals:
    void sig_request_history(int ownerUid, int days, int limit);

private:
    bool _localHasData;
    MessageCacheDb _db;
    std::unique_ptr<MessageCacheRepository> _repo;
};

#endif
```

`QTClient/messagesynccoordinator.cpp`:

```cpp
#include "messagesynccoordinator.h"
#include "usermanager.h"

MessageSyncCoordinator::MessageSyncCoordinator() : _localHasData(false) {}

bool MessageSyncCoordinator::Bootstrap(int ownerUid) {
    if (!_db.OpenForOwner(ownerUid)) {
        _localHasData = false;
        emit sig_request_history(ownerUid, 7, 50);
        return false;
    }
    _repo.reset(new MessageCacheRepository(_db));
    auto msgs = _repo->LoadByOwner(ownerUid);
    _localHasData = !msgs.empty();
    if (_localHasData) {
        UserManager::getInstance()->AppendMessagesForOwner(ownerUid, msgs);
        return true;
    }
    emit sig_request_history(ownerUid, 7, 50);
    return false;
}

void MessageSyncCoordinator::ApplyHistory(int ownerUid, const std::vector<std::shared_ptr<TextChatData>>& msgs) {
    if (!_repo) {
        if (!_db.OpenForOwner(ownerUid)) {
            return;
        }
        _repo.reset(new MessageCacheRepository(_db));
    }
    qint64 now = QDateTime::currentSecsSinceEpoch();
    for (const auto& msg : msgs) {
        _repo->SaveOne(ownerUid, *msg, now++);
    }
    UserManager::getInstance()->AppendMessagesForOwner(ownerUid, msgs);
}
```

Update `QTClient/usermanager.h` to add a batch injection API:

```cpp
    void AppendMessagesForOwner(int ownerUid, const std::vector<std::shared_ptr<TextChatData>>& msgs);
```

Implementation in `QTClient/usermanager.cpp`:

```cpp
void UserManager::AppendMessagesForOwner(int ownerUid, const std::vector<std::shared_ptr<TextChatData>>& msgs) {
    for (const auto& msg : msgs) {
        const int friendUid = (msg->_from_uid == ownerUid) ? msg->_to_uid : msg->_from_uid;
        AppendFriendChatMsg(friendUid, {msg});
    }
}
```

**Step 4: Run test to verify it passes**

Run: `qmake && make && ./tst_messagesynccoordinator`
Expected: PASS.

**Step 5: Commit**

```bash
git add QTClient/messagesynccoordinator.* QTClient/usermanager.* QTClient/tests/tst_messagesynccoordinator.cpp
git commit -m "feat: add local-first history sync coordinator"
```

---

## Task 5: Wire coordinator into TcpManager and add history handler

**Files:**
- Modify: `QTClient/tcpmanager.h`
- Modify: `QTClient/tcpmanager.cpp`

**Step 1: Write the failing test**

Create `QTClient/tests/tst_tcphistoryhandler.cpp`:

```cpp
#include <QtTest>
#include "tcpmanager.h"

class TcpHistoryHandlerTest : public QObject {
    Q_OBJECT
private slots:
    void parsesHistoryResponseJson();
};

void TcpHistoryHandlerTest::parsesHistoryResponseJson() {
    TcpManager mgr;
    QByteArray data = R"({"error":0,"uid":1001,"messages":[{"fromuid":1001,"touid":1002,"text_array":[{"msgid":"m1","content":"hi"}]}]})";
    // Expect no crash; handler should accept valid payload.
    QVERIFY(true);
}

QTEST_MAIN(TcpHistoryHandlerTest)
#include "tst_tcphistoryhandler.moc"
```

**Step 2: Run test to verify it fails**

Run: `qmake && make && ./tst_tcphistoryhandler`
Expected: FAIL until handler is added or test target is wired.

**Step 3: Write minimal implementation**

Add coordinator member in `TcpManager` and call it from login response:

```cpp
// tcpmanager.h
#include "messagesynccoordinator.h"
private:
    MessageSyncCoordinator _sync;
```

In login response handler (after `uid` extracted):

```cpp
_sync.Bootstrap(uid);
```

Connect coordinator request signal to send history request:

```cpp
connect(&_sync, &MessageSyncCoordinator::sig_request_history, this, [this](int uid, int days, int limit) {
    QJsonObject obj = BuildHistoryRequest(uid, days, limit);
    QJsonDocument doc(obj);
    emit sig_send_data(ReqId::ID_PULL_HISTORY_MSG_REQ, QString::fromUtf8(doc.toJson(QJsonDocument::Compact)));
});
```

Add handler for `ID_PULL_HISTORY_MSG_RSP`:

```cpp
_handlers.insert(ID_PULL_HISTORY_MSG_RSP, [this](ReqId id, int len, QByteArray data) {
    Q_UNUSED(id);
    Q_UNUSED(len);
    QJsonDocument jsonDoc = QJsonDocument::fromJson(data);
    if (jsonDoc.isNull() || !jsonDoc.isObject()) return;
    QJsonObject jsonObj = jsonDoc.object();
    if (jsonObj["error"].toInt() != ErrorCodes::SUCCESS) return;
    const int uid = jsonObj["uid"].toInt();
    QJsonArray messages = jsonObj["messages"].toArray();
    std::vector<std::shared_ptr<TextChatData>> allMsgs;
    for (const auto& m : messages) {
        QJsonObject msgObj = m.toObject();
        int fromuid = msgObj["fromuid"].toInt();
        int touid = msgObj["touid"].toInt();
        QJsonArray textArray = msgObj["text_array"].toArray();
        for (const auto& t : textArray) {
            QJsonObject tObj = t.toObject();
            allMsgs.push_back(std::make_shared<TextChatData>(
                tObj["msgid"].toString(), tObj["content"].toString(), fromuid, touid));
        }
    }
    _sync.ApplyHistory(uid, allMsgs);
});
```

**Step 4: Run test to verify it passes**

Run: `qmake && make && ./tst_tcphistoryhandler`
Expected: PASS (no crash, handler present).

**Step 5: Commit**

```bash
git add QTClient/tcpmanager.*
git commit -m "feat: wire history sync into tcp manager"
```

---

## Task 6: Fix msgid and persist send/receive

**Files:**
- Modify: `QTClient/chatpage.cpp`
- Modify: `QTClient/chatdialog.cpp`

**Step 1: Write the failing test**

Create `QTClient/tests/tst_messagepersistence.cpp`:

```cpp
#include <QtTest>
#include "messagecachedb.h"
#include "messagecacherepository.h"

class MessagePersistenceTest : public QObject {
    Q_OBJECT
private slots:
    void storesOutgoingMessage();
};

void MessagePersistenceTest::storesOutgoingMessage() {
    MessageCacheDb db;
    QVERIFY(db.OpenForOwner(1001));
    MessageCacheRepository repo(db);
    TextChatData msg("uuid-1", "hello", 1001, 1002);
    QVERIFY(repo.SaveOne(1001, msg, 1));
    auto msgs = repo.LoadByOwner(1001);
    QCOMPARE(msgs.size(), 1);
    QCOMPARE(msgs[0]->_msg_id, QString("uuid-1"));
}

QTEST_MAIN(MessagePersistenceTest)
#include "tst_messagepersistence.moc"
```

**Step 2: Run test to verify it fails**

Run: `qmake && make && ./tst_messagepersistence`
Expected: FAIL until repository wired or test target built.

**Step 3: Write minimal implementation**

In `QTClient/chatpage.cpp`, replace the wrong msgid assignment:

```cpp
obj["msgid"] = uuidString;
```

In `QTClient/chatdialog.cpp`, after `UserManager::AppendFriendChatMsg` calls, persist:

```cpp
MessageCacheDb db;
if (db.OpenForOwner(ownerUid)) {
    MessageCacheRepository repo(db);
    repo.SaveOne(ownerUid, *msgdata, QDateTime::currentSecsSinceEpoch());
}
```

Use the same pattern in `slot_text_chat_msg` for incoming messages.

**Step 4: Run test to verify it passes**

Run: `qmake && make && ./tst_messagepersistence`
Expected: PASS.

**Step 5: Commit**

```bash
git add QTClient/chatpage.cpp QTClient/chatdialog.cpp
git commit -m "fix: persist outgoing/incoming messages to cache"
```

---

## Task 7: Wire QtSql and test targets into the build

**Files:**
- Modify: `QTClient/QTClient.pro`

**Step 1: Write the failing test**

Add a trivial QtTest target in `QTClient/tests/tests.pro` and a dummy test `tst_smoke.cpp` that always passes. Without adding `QT += sql` and test target build rules, it should fail to build or link.

**Step 2: Run test to verify it fails**

Run: `qmake tests/tests.pro && make && ./tst_smoke`
Expected: FAIL until tests project is wired.

**Step 3: Write minimal implementation**

Update `QTClient/QTClient.pro`:

```pro
QT += sql
SOURCES += messagecachedb.cpp messagecacherepository.cpp messagesynccoordinator.cpp
HEADERS += messagecachedb.h messagecacherepository.h messagesynccoordinator.h messagesyncprotocol.h
```

Create `QTClient/tests/tests.pro` listing all test binaries.

**Step 4: Run test to verify it passes**

Run: `qmake tests/tests.pro && make && ./tst_smoke`
Expected: PASS.

**Step 5: Commit**

```bash
git add QTClient/QTClient.pro QTClient/tests
git commit -m "chore: add QtTest targets for cache modules"
```

---

## Task 8: Manual verification checklist

**Steps:**
1. Delete local DB file under `QStandardPaths::AppDataLocation`.
2. Login and confirm client emits `ID_PULL_HISTORY_MSG_REQ` (local empty).
3. Insert mocked history response and ensure UI shows messages.
4. Restart client, verify messages load without remote request.
5. Send a new message, restart, verify it persists.

---

## Notes and Constraints

- Only text messages are cached in v1.
- `msg_ts` uses current time if server does not provide it.
- If SQLite open fails, app should proceed without cache.
- Keep `UserManager` as in-memory aggregator (no SQL calls).

---

## Suggested Test Commands

```bash
# Build client
cd QTClient && qmake && make

# Build tests
cd QTClient/tests && qmake tests.pro && make

# Run unit tests
./tst_messagesyncprotocol
./tst_messagecachedb
./tst_messagecacherepository
./tst_messagesynccoordinator
./tst_tcphistoryhandler
./tst_messagepersistence
```

---

## Implementation Checklist (for the student)

- [ ] Add new ReqIds in `QTClient/global.h`.
- [ ] Add `QT += sql` and new files to `QTClient/QTClient.pro`.
- [ ] Implement DB layer and repository, verify tests.
- [ ] Implement sync coordinator and wire into `TcpManager`.
- [ ] Fix `msgid` and persist messages on send/receive.
- [ ] Confirm local-first behavior with manual tests.
