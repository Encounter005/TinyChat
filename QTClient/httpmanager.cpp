#include "httpmanager.h"

void HttpManager::PostHttpReq(QUrl url, QJsonObject json, ReqId req_id, Modules mod)
{
    //Create a http post request
    // 1. create the body
    QByteArray data = QJsonDocument(json).toJson();
    // 2. create header
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setHeader(QNetworkRequest::ContentLengthHeader, QByteArray::number(data.length()));

    // send the post and handle the result
    auto self = shared_from_this();
    QNetworkReply *reply = _manager.post(request, data);
    // set the slot and signal to wait for sending completition
    QObject::connect(reply, &QNetworkReply::finished, [reply, self, req_id, mod](){
        // handle errors
        if(reply->error() != QNetworkReply::NoError) {
            qDebug() << reply->errorString();
            //send the signal to notify the network process is completed
            emit self->sig_http_finish(req_id, "", ErrorCodes::ERROR_NETWORK, mod);
            reply->deleteLater();
            return;
        }

        // if no errors
        QString res = reply->readAll();
        qDebug() << "res is: " << res;
        emit self ->sig_http_finish(req_id, res, ErrorCodes::SUCCESS, mod);
        reply->deleteLater();
        return;
    });
}

void HttpManager::slot_http_finish(ReqId id, QString res, ErrorCodes error_code, Modules mod)
{
    switch(mod) {
    case Modules::REGISTERMOD:
        // send the signal to notify the module that http response complete
        emit sig_reg_mod_finish(id, res, error_code);
        break;
    case Modules::RESETMOD:
        emit sig_reset_mod_finish(id, res, error_code);
        break;
    case Modules::LOGINMOD:
        emit sig_login_mod_finish(id, res, error_code);
        break;
    }
}
HttpManager::HttpManager() {
    QObject::connect(this, &HttpManager::sig_http_finish, this, &HttpManager::slot_http_finish);
}
