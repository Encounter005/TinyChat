#include "messagesynccoordinator.h"
#include "usermanager.h"

MessageSyncCoordinator::MessageSyncCoordinator() : _localHasData(false) {}

bool MessageSyncCoordinator::BootStrap(int ownerUid)
{
    if(!_db.OpenForOwner(ownerUid)) {
        _localHasData = false;
        emit sig_request_history(ownerUid, 7, 50);
        return false;
    }

    _repo.reset(new MessageCacheRepository(_db));
    auto msgs = _repo->LoadByOwner(ownerUid);
    _localHasData = !msgs.empty();
    if(_localHasData) {
        UserManager::getInstance()->AppendMessagesForOwner(ownerUid, msgs);
        return true;
    }

    emit sig_request_history(ownerUid, 7, 50);
    return false;

}

void MessageSyncCoordinator::ApplyHistory(int ownerUid, const std::vector<std::shared_ptr<TextChatData> > &msgs)
{
    if(!_repo) {
        if(!_db.OpenForOwner(ownerUid)) {
            return;
        }
        _repo.reset(new MessageCacheRepository(_db));
    }

    qint64 now = QDateTime::currentMSecsSinceEpoch();
    for(const auto& msg : msgs) {
        _repo->SaveOne(ownerUid, *msg, now++);
    }

    UserManager::getInstance()->AppendMessagesForOwner(ownerUid, msgs);
}
