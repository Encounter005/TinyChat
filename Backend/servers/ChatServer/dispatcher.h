#ifndef DISPATCHER_H_
#define DISPATCHER_H_

#include "common/const.h"
#include <cstdint>
#include <functional>
#include <memory>
#include <unordered_map>

class Session;
struct Message;
using MsgHandler = std::function<void(std::shared_ptr<Session>, const Message&)>;

class Dispatcher {
public:
    Dispatcher() = default;
    void Register(uint16_t msg_id, MsgHandler handler);
    void Register(MsgId msg_id, MsgHandler handler);
    void Dispatch(std::shared_ptr<Session> session, const Message&msg);
private:
    std::unordered_map<uint16_t, MsgHandler> _handlers;
};


#endif   // DISPATCHER_H_
