#include "tcpmanager.h"
#include "usermanager.h"
#include <QJsonDocument>

void TcpManager::handleMsg(ReqId id, int len, QByteArray data)
{
    auto find_iter =  _handlers.find(id);
    if(find_iter == _handlers.end()){
        qDebug()<< "not found id ["<< id << "] to handle";
        return ;
    }
    find_iter.value()(id,len,data);
}

void TcpManager::initHandlers()
{
    _handlers.insert(ReqId::ID_CHAT_LOGIN_RSP, [this](ReqId id, int len, QByteArray data){
        qDebug() << "handle id is: " << id << "data is" << data;

        QJsonDocument jsonDoc = QJsonDocument::fromJson(data);
        if(jsonDoc.isNull()) {
            qDebug() << "Failed to create QJsonDocument";
            return;
        }

        QJsonObject jsonObj = jsonDoc.object();
        if(!jsonObj.contains("error")) {
            int err = ErrorCodes::ERROR_JSON;
            qDebug() << "Login Failed, err is Json Parse err " << err;
            emit sig_login_failed(ErrorCodes::ERROR_JSON);
            return;
        }

        auto uid = jsonObj["uid"].toInt();
        auto name = jsonObj["name"].toString();
        auto nick = jsonObj["nick"].toString();
        auto icon = jsonObj["icon"].toString();
        auto sex = jsonObj["sex"].toInt();
        auto user_info = std::make_shared<UserInfo>(uid, name, nick, icon, sex);

        qDebug() << "receive user info is: " << jsonObj;
        UserManager::getInstance()->SetUserInfo(user_info);
        UserManager::getInstance()->SetToken(jsonObj["token"].toString());
        if(jsonObj.contains("apply_list")){
            UserManager::getInstance()->AppendApplyList(jsonObj["apply_list"].toArray());
        }
        //添加好友列表
        if (jsonObj.contains("friend_list")) {
            qDebug("Find Friend List");
            UserManager::getInstance()->AppendFriendList(jsonObj["friend_list"].toArray());
        }
        // emit chatDialog
        emit sig_switch_chat_dialog();
    });

    _handlers.insert(ReqId::ID_SEARCH_USER_RSP,[this](ReqId id, int len , QByteArray data){
        Q_UNUSED(len);
        qDebug()<< "handle id is "<< id << " data is " << data;
        // 将QByteArray转换为QJsonDocument
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data);
        // 检查转换是否成功
        if(jsonDoc.isNull()){
            qDebug() << "Failed to create QJsonDocument.";
            return;
        }
        QJsonObject jsonObj = jsonDoc.object();

        if(!jsonObj.contains("error")){
            auto error = ErrorCodes::ERROR_JSON;
            qDebug() << "Search Failed, err is Json Parse Err" << error ;
            emit sig_user_search(nullptr);
            return;
        }
        auto error = jsonObj["error"].toInt();
        if(error != ErrorCodes::SUCCESS){
            qDebug() << "Search Failed, err is " << error ;
            emit sig_user_search(nullptr);
            return;
        }
        auto search_info = std::make_shared<SearchInfo>(jsonObj["uid"].toInt(),
                                                        jsonObj["name"].toString(), jsonObj["nick"].toString(),
                                                        jsonObj["desc"].toString(), jsonObj["icon"].toString(),jsonObj["sex"].toInt());
        emit sig_user_search(search_info);
    });
    _handlers.insert(ID_NOTIFY_ADD_FRIEND_REQ, [this](ReqId id, int len, QByteArray data) {
        Q_UNUSED(len);
        qDebug() << "handle id is " << id << " data is " << data;
        // 将QByteArray转换为QJsonDocument
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data);
        // 检查转换是否成功
        if (jsonDoc.isNull()) {
            qDebug() << "Failed to create QJsonDocument.";
            return;
        }
        QJsonObject jsonObj = jsonDoc.object();
        if (!jsonObj.contains("error")) {
            int err = ErrorCodes::ERROR_JSON;
            qDebug() << "Login Failed, err is Json Parse Err" << err;
            emit sig_user_search(nullptr);
            return;
        }
        auto err = jsonObj["error"].toString();
        if (err != "success") {
            qDebug() << "Login Failed, err is " << err;
            emit sig_user_search(nullptr);
            return;
        }
        int from_uid = jsonObj["applyuid"].toInt();
        QString name = jsonObj["name"].toString();
        QString desc = jsonObj["desc"].toString();
        QString icon = jsonObj["icon"].toString();
        QString nick = jsonObj["nick"].toString();
        int sex = jsonObj["sex"].toInt();
        auto apply_info = std::make_shared<AddFriendApply>(
            from_uid, name, desc,
            icon, nick, sex);
        emit sig_friend_apply(apply_info);
    });

    _handlers.insert(ID_AUTH_FRIEND_RSP, [this](ReqId id, int len, QByteArray data) {
        Q_UNUSED(len);
        qDebug() << "handle id is " << id << " data is " << data;
        // 将QByteArray转换为QJsonDocument
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data);
        // 检查转换是否成功
        if (jsonDoc.isNull()) {
            qDebug() << "Failed to create QJsonDocument.";
            return;
        }
        QJsonObject jsonObj = jsonDoc.object();
        if (!jsonObj.contains("error")) {
            int err = ErrorCodes::ERROR_JSON;
            qDebug() << "Auth Friend Failed, err is Json Parse Err" << err;
            return;
        }
        int err = jsonObj["error"].toInt();
        if (err != ErrorCodes::SUCCESS) {
            qDebug() << "Auth Friend Failed, err is " << err;
            return;
        }
        auto name = jsonObj["name"].toString();
        auto nick = jsonObj["nick"].toString();
        auto icon = jsonObj["icon"].toString();
        auto sex = jsonObj["sex"].toInt();
        auto uid = jsonObj["uid"].toInt();
        auto rsp = std::make_shared<AuthRsp>(uid, name, nick, icon, sex);
        emit sig_auth_rsp(rsp);
        qDebug() << "Auth Friend Success " ;
    });
    _handlers.insert(ID_NOTIFY_AUTH_FRIEND_REQ, [this](ReqId id, int len, QByteArray data) {
        Q_UNUSED(len);
        qDebug() << "handle id is " << id << " data is " << data;
        // 将QByteArray转换为QJsonDocument
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data);
        // 检查转换是否成功
        if (jsonDoc.isNull()) {
            qDebug() << "Failed to create QJsonDocument.";
            return;
        }
        QJsonObject jsonObj = jsonDoc.object();
        if (!jsonObj.contains("error")) {
            int err = ErrorCodes::ERROR_JSON;
            qDebug() << "Auth Friend Failed, err is " << err;
            return;
        }
        int err = jsonObj["error"].toInt();
        if (err != ErrorCodes::SUCCESS) {
            qDebug() << "Auth Friend Failed, err is " << err;
            return;
        }
        int from_uid = jsonObj["fromuid"].toInt();
        QString name = jsonObj["name"].toString();
        QString nick = jsonObj["nick"].toString();
        QString icon = jsonObj["icon"].toString();
        int sex = jsonObj["sex"].toInt();
        auto auth_info = std::make_shared<AuthInfo>(from_uid,name,
                                                    nick, icon, sex);
        emit sig_add_auth_friend(auth_info);
    });
    _handlers.insert(ID_TEXT_CHAT_MSG_RSP, [this](ReqId id, int len, QByteArray data) {
        Q_UNUSED(len);
        qDebug() << "handle id is " << id << " data is " << data;
        // 将QByteArray转换为QJsonDocument
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data);
        // 检查转换是否成功
        if (jsonDoc.isNull()) {
            qDebug() << "Failed to create QJsonDocument.";
            return;
        }
        QJsonObject jsonObj = jsonDoc.object();
        if (!jsonObj.contains("error")) {
            int err = ErrorCodes::ERROR_JSON;
            qDebug() << "Chat Msg Rsp Failed, err is Json Parse Err" << err;
            return;
        }
        int err = jsonObj["error"].toInt();
        if (err != ErrorCodes::SUCCESS) {
            qDebug() << "Chat Msg Rsp Failed, err is " << err;
            return;
        }
        qDebug() << "Receive Text Chat Rsp Success " ;
        //ui设置送达等标记 todo...
    });
    _handlers.insert(ID_NOTIFY_TEXT_CHAT_MSG_REQ, [this](ReqId id, int len, QByteArray data) {
        Q_UNUSED(len);
        qDebug() << "handle id is " << id << " data is " << data;
        // 将QByteArray转换为QJsonDocument
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data);
        // 检查转换是否成功
        if (jsonDoc.isNull()) {
            qDebug() << "Failed to create QJsonDocument.";
            return;
        }
        QJsonObject jsonObj = jsonDoc.object();
        if (!jsonObj.contains("error")) {
            int err = ErrorCodes::ERROR_JSON;
            qDebug() << "Notify Chat Msg Failed, err is Json Parse Err" << err;
            return;
        }
        int err = jsonObj["error"].toInt();
        if (err != ErrorCodes::SUCCESS) {
            qDebug() << "Notify Chat Msg Failed, err is " << err;
            return;
        }
        qDebug() << "Receive Text Chat Notify Success " ;
        auto msg_ptr = std::make_shared<TextChatMsg>(jsonObj["fromuid"].toInt(),
                                                     jsonObj["touid"].toInt(),jsonObj["text_array"].toArray());
        emit sig_text_chat_msg(msg_ptr);
    });
}

TcpManager::TcpManager() : _host(""), _recv_pending(false), _message_id(0), _message_len(0) {
    initHandlers();
    QObject::connect(&_socket, &QTcpSocket::connected, [&](){
        qDebug() << "Connected to the server";
        emit sig_con_success(true);
    });
    QObject::connect(&_socket, &QTcpSocket::readyRead, [this](){
        _buffer.append(_socket.readAll());
        while(true) {
            const int HEADER_SIZE = 6;
            if(_buffer.size() < HEADER_SIZE) {
                break;
            }

            QDataStream headStream(_buffer);
            headStream.setByteOrder(QDataStream::BigEndian);

            quint32 message_len;
            quint16 message_id;
            headStream >> message_len >> message_id;

            if(_buffer.size() < static_cast<int>(HEADER_SIZE + message_len)) {
                break;
            }
            QByteArray messageBody = _buffer.mid(HEADER_SIZE, message_len);
            _buffer = _buffer.mid(HEADER_SIZE + message_len);
            qDebug() << "msg id is " << message_id << " msg len is: " << message_len << "msg body is: " << _buffer;
            handleMsg(ReqId(message_id), message_len, messageBody);

        }
    });
    QObject::connect(&_socket,
                     QOverload<QAbstractSocket::SocketError>::of(&QTcpSocket::errorOccurred),
                     [&](QAbstractSocket::SocketError socketError) {
          Q_UNUSED(socketError)
          qDebug() << "Error:" << _socket.errorString(); });

    QObject::connect(&_socket, &QTcpSocket::disconnected, [&](){
        qDebug() << "Disconnected from the sever";
    });

    QObject::connect(this, &TcpManager::sig_send_data, this, &TcpManager::slot_send_data);
}

void TcpManager::slot_tcp_connect(ServerInfo si)
{
    qDebug() << "receive tcp connect signal";
    qDebug() << "Connecting to server...";
    _host = si.Host;
    _port = static_cast<uint16_t>(si.Port.toUInt());
    _socket.connectToHost(_host, _port);
}

void TcpManager::slot_send_data(ReqId reqid, QString data)
{
    uint16_t id = static_cast<uint16_t>(reqid);
    QByteArray body = data.toUtf8();
    quint32 len = static_cast<quint32>(data.size());

    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly);
    out.setByteOrder(QDataStream::BigEndian);
    out << len << id;
    block.append(body);
    _socket.write(block);
    qDebug() << "send body_len =" << len
             << "msg_id =" << id
             << "body =" << body;
}
