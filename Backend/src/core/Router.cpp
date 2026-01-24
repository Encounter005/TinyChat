#include "Router.h"
#include "controller/TestController.h"
#include "controller/AuthController.h"
#include "controller/UserController.h"

void Router::RegisterRoutes(LogicSystem &logic) {
    TestController::Register(logic);
    AuthController::Register(logic);
    UserController::Register(logic);
    UserController::ResetPwd(logic);
    UserController::Login(logic);
}
