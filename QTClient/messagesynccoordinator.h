#ifndef MESSAGESYNCCOORDINATOR_H
#define MESSAGESYNCCOORDINATOR_H

#include <QObject>
#include "messagecachedb.h"
#include "messagecacherepository.h"
#include "messagesyncprotocol.h"
#include "userdata.h"

class MessageSyncCoordinator : public QObject {

    Q_OBJECT
public:
    MessageSyncCoordinator();
    bool BootStrap(int ownerUid);
    void ApplyHistory(int ownerUid, const std::vector<std::shared_ptr<TextChatData>>& msgs);

    // test
    void SetLocalHasData(bool hasData) {
        _localHasData = hasData;
    }

    bool ShouldRequestRemote() const {
        return !_localHasData;
    }
signals:
    void sig_request_history(int ownerUid, int days, int limit);
private:
    bool _localHasData;
    MessageCacheDb _db;
    std::unique_ptr<MessageCacheRepository> _repo;

};

#endif // MESSAGESYNCCOORDINATOR_H
