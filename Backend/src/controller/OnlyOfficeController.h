#ifndef ONLYOFFICECONTROLLER_H_
#define ONLYOFFICECONTROLLER_H_

#include "core/HttpConnection.h"
#include "core/LogicSystem.h"
#include <memory>

class LogicSystem;
class HttpConnection;

class OnlyOfficeController {
public:
    static void Register(LogicSystem& logic);

private:
    static void OpenEditor(std::shared_ptr<HttpConnection> connection);
    static void DownloadFile(std::shared_ptr<HttpConnection> connection);
    static void Callback(std::shared_ptr<HttpConnection> connection);
};

#endif
