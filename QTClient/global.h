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
extern std::function<QString(QString)> xorString;
extern std::function<QString(QString)> toMD5;
extern QString gate_url_prefix;

enum ReqId{
    ID_GET_VARIFY_CODE = 1001, //获取验证码
    ID_REG_UESR = 1002, //注册用户
    ID_RESET_PWD = 1003, //注册用户
};

enum ErrorCodes {
    SUCCESS = 0,
    ERROR_JSON = 1, // Json解析错误
    ERROR_NETWORK = 2, // 网络错误
};

enum Modules {
    REGISTERMOD = 0,
    RESETMOD = 1,
};

enum TipErr {
    TIP_SUCCESS = 0,
    TIP_EMAIL_ERR = 1,
    TIP_PWD_ERR = 2,
    TIP_CONFIRM_ERR = 3,
    TIP_PWD_CONFIRM_ERR = 4,
    TIP_VARIFY_ERR = 5,
    TIP_USER_ERR = 6,

};

// 点击状态
enum ClickLbState{
    Normal = 0,
    Selected = 1
};


#endif // GLOBAL_H
