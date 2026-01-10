#ifndef TESTCONTROLLER_H_
#define TESTCONTROLLER_H_

#include <memory>
#include "core/LogicSystem.h"
#include "core/HttpConnection.h"
class LogicSystem;
class HttpConnection;


class TestController {
public:
    static void Register(LogicSystem& logic);
private:
    static void GetTest(std::shared_ptr<HttpConnection> connection);
};

#endif // TESTCONTROLLER_H_
