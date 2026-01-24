#include "dispatcher.h"
#include "session.h"
#include "Messsage.h"
#include "infra/LogManager.h"
#include <memory>

void Dispatcher::Register(uint16_t msg_id, MsgHandler handler) {
    _handlers.emplace(msg_id, std::move(handler));
}

void Dispatcher::Register(MsgId msg_id, MsgHandler handler) {
    Register(static_cast<uint16_t>(msg_id), std::move(handler));
}

void Dispatcher::Dispatch(std::shared_ptr<Session> session, const Message& msg) {
    auto it = _handlers.find(msg.msg_id);
    if(it != _handlers.end()) {
        it->second(session, msg);
    } else {
        LOG_ERROR("[Session] no method to handle this message, msgid is: {}", msg.msg_id);
    }
}
