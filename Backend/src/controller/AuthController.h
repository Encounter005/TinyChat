#ifndef AUTHCONTROLLER_H_
#define AUTHCONTROLLER_H_

#include "core/HttpConnection.h"
#include "core/LogicSystem.h"
#include <memory.h>

class LogicSystem;
class HttpConnection;

class AuthController {
public:
    static void Register(LogicSystem& logic);
private:
    static void GetVarifyCode(std::shared_ptr<HttpConnection> connection);

};

#endif // AUTHCONTROLLER_H_
