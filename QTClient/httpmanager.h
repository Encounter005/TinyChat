#ifndef HTTPMANAGER_H
#define HTTPMANAGER_H
#include <QString>
#include <QUrl>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QObject>
#include <QJsonObject>
#include <QJsonDocument>
#include "singleton.h"
#include "global.h"

//CRTP
class HttpManager : public QObject, public SingleTon<HttpManager>, public std::enable_shared_from_this<HttpManager>
{
    Q_OBJECT
public:
    ~HttpManager() = default;
    void PostHttpReq(QUrl url, QJsonObject json, ReqId req_id, Modules mod);
public slots:
    void slot_http_finish(ReqId id, QString res, ErrorCodes error_code, Modules mod);

private:
    friend class SingleTon<HttpManager>;
    HttpManager();
    QNetworkAccessManager _manager;


signals:
    //通知其他模块网络初始化完成
    void sig_http_finish(ReqId id, QString res, ErrorCodes error_code, Modules mod);
    void sig_reg_mod_finish(ReqId id, QString res, ErrorCodes error_code);
    void sig_reset_mod_finish(ReqId id, QString res, ErrorCodes error_code);
};


#endif // HTTPMANAGER_H
