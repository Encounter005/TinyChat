#ifndef USERCONTROLLER_H_
#define USERCONTROLLER_H_

#include "core/HttpConnection.h"
#include "core/LogicSystem.h"
#include <memory>

class LogicSystem;
class HttpConnection;

class UserController {
public:
    static void Register(LogicSystem& logic);
    static void ResetPwd(LogicSystem& logic);
private:
    static void RegisterUser(std::shared_ptr<HttpConnection> connection);
    static void ResetUserPwd(std::shared_ptr<HttpConnection> connection);
};



#endif   // USERCONTROLLER_H_
