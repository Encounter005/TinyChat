#ifndef GLOBAL_H
#define GLOBAL_H

#include <QWidget>
#include <functional>
#include <memory>
#include <QObject>
#include <QMap>
#include <QJsonObject>
#include "QStyle"

/*
* @brief repolish 刷新qss样式
*/
extern std::function<void(QWidget*)> repolish;

enum ReqId{
    ID_GET_VARIFY_CODE = 1001, //获取验证码
    ID_REG_UESR = 1002, //注册用户
};

enum ErrorCodes {
    SUCCESS = 0,
    ERROR_JSON = 1, // Json解析错误
    ERROR_NETWORK = 2, // 网络错误
};

enum Modules {
    REGISTERMOD = 0,
};

extern QString gate_url_prefix;



#endif // GLOBAL_H
