#include "tcpmanager.h"
#include "global.h"
#include "qapplication.h"
#include "qdatetime.h"
#include "qjsondocument.h"
#include "qsettings.h"
#include "qtimer.h"
#include "userdata.h"
#include "usermanager.h"
#include <QDir>
#include <QJsonDocument>

void TcpManager::handleMsg(ReqId id, int len, QByteArray data) {
    auto find_iter = _handlers.find(id);
    if (find_iter == _handlers.end()) {
        qDebug() << "not found id [" << id << "] to handle";
        return;
    }
    find_iter.value()(id, len, data);
}

void TcpManager::initHandlers() {
    _handlers.insert(
        ReqId::ID_CHAT_LOGIN_RSP, [this](ReqId id, int len, QByteArray data) {
            qDebug() << "handle id is: " << id << "data is" << data;

            updateLastResponseTime();
            QJsonDocument jsonDoc = QJsonDocument::fromJson(data);
            if (jsonDoc.isNull()) {
                qDebug() << "Failed to create QJsonDocument";
                return;
            }

            QJsonObject jsonObj = jsonDoc.object();
            if (!jsonObj.contains("error")) {
                int err = ErrorCodes::ERROR_JSON;
                qDebug() << "Login Failed, err is Json Parse err " << err;
                emit sig_login_failed(ErrorCodes::ERROR_JSON);
                return;
            }

            auto uid  = jsonObj["uid"].toInt();
            auto name = jsonObj["name"].toString();
            auto nick = jsonObj["nick"].toString();
            auto icon = jsonObj["icon"].toString();
            auto sex  = jsonObj["sex"].toInt();
            auto user_info
                = std::make_shared<UserInfo>(uid, name, nick, icon, sex);

            qDebug() << "receive user info is: " << jsonObj;
            UserManager::getInstance()->SetUserInfo(user_info);
            UserManager::getInstance()->SetToken(jsonObj["token"].toString());
            if (jsonObj.contains("apply_list")) {
                UserManager::getInstance()->AppendApplyList(
                    jsonObj["apply_list"].toArray());
            }
            // 添加好友列表
            if (jsonObj.contains("friend_list")) {
                qDebug("Find Friend List");
                UserManager::getInstance()->AppendFriendList(
                    jsonObj["friend_list"].toArray());
            }

            const bool hasLocalHistory = _sync.BootStrap(uid);
            if (hasLocalHistory) {
                emit sig_switch_chat_dialog();
            } else {
                _wait_history_rsp = true;
            }
        });

    _handlers.insert(
        ReqId::ID_SEARCH_USER_RSP, [this](ReqId id, int len, QByteArray data) {
            Q_UNUSED(len);
            qDebug() << "handle id is " << id << " data is " << data;
            updateLastResponseTime();
            // 将QByteArray转换为QJsonDocument
            QJsonDocument jsonDoc = QJsonDocument::fromJson(data);
            // 检查转换是否成功
            if (jsonDoc.isNull()) {
                qDebug() << "Failed to create QJsonDocument.";
                return;
            }
            QJsonObject jsonObj = jsonDoc.object();

            if (!jsonObj.contains("error")) {
                auto error = ErrorCodes::ERROR_JSON;
                qDebug() << "Search Failed, err is Json Parse Err" << error;
                emit sig_user_search(nullptr);
                return;
            }
            auto error = jsonObj["error"].toInt();
            if (error != ErrorCodes::SUCCESS) {
                qDebug() << "Search Failed, err is " << error;
                emit sig_user_search(nullptr);
                return;
            }
            auto search_info = std::make_shared<SearchInfo>(
                jsonObj["uid"].toInt(),
                jsonObj["name"].toString(),
                jsonObj["nick"].toString(),
                jsonObj["desc"].toString(),
                jsonObj["icon"].toString(),
                jsonObj["sex"].toInt());
            emit sig_user_search(search_info);
        });
    _handlers.insert(
        ID_NOTIFY_ADD_FRIEND_REQ, [this](ReqId id, int len, QByteArray data) {
            Q_UNUSED(len);
            qDebug() << "handle id is " << id << " data is " << data;
            updateLastResponseTime();
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
            int     from_uid   = jsonObj["applyuid"].toInt();
            QString name       = jsonObj["name"].toString();
            QString desc       = jsonObj["desc"].toString();
            QString icon       = jsonObj["icon"].toString();
            QString nick       = jsonObj["nick"].toString();
            int     sex        = jsonObj["sex"].toInt();
            auto    apply_info = std::make_shared<AddFriendApply>(
                from_uid, name, desc, icon, nick, sex);
            emit sig_friend_apply(apply_info);
        });

    _handlers.insert(
        ID_AUTH_FRIEND_RSP, [this](ReqId id, int len, QByteArray data) {
            Q_UNUSED(len);
            qDebug() << "handle id is " << id << " data is " << data;
            updateLastResponseTime();
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
            auto sex  = jsonObj["sex"].toInt();
            auto uid  = jsonObj["uid"].toInt();
            auto rsp  = std::make_shared<AuthRsp>(uid, name, nick, icon, sex);
            emit sig_auth_rsp(rsp);
            qDebug() << "Auth Friend Success ";
        });
    _handlers.insert(
        ID_NOTIFY_AUTH_FRIEND_REQ, [this](ReqId id, int len, QByteArray data) {
            Q_UNUSED(len);
            qDebug() << "handle id is " << id << " data is " << data;
            updateLastResponseTime();
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
            int     from_uid = jsonObj["fromuid"].toInt();
            QString name     = jsonObj["name"].toString();
            QString nick     = jsonObj["nick"].toString();
            QString icon     = jsonObj["icon"].toString();
            int     sex      = jsonObj["sex"].toInt();
            auto    auth_info
                = std::make_shared<AuthInfo>(from_uid, name, nick, icon, sex);
            emit sig_add_auth_friend(auth_info);
        });
    _handlers.insert(
        ID_TEXT_CHAT_MSG_RSP, [this](ReqId id, int len, QByteArray data) {
            Q_UNUSED(len);
            qDebug() << "handle id is " << id << " data is " << data;
            updateLastResponseTime();
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
            qDebug() << "Receive Text Chat Rsp Success ";
            // ui设置送达等标记 todo...
        });
    _handlers.insert(
        ID_NOTIFY_TEXT_CHAT_MSG_REQ,
        [this](ReqId id, int len, QByteArray data) {
            Q_UNUSED(len);
            qDebug() << "handle id is " << id << " data is " << data;
            updateLastResponseTime();
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
                qDebug() << "Notify Chat Msg Failed, err is Json Parse Err"
                         << err;
                return;
            }
            int err = jsonObj["error"].toInt();
            if (err != ErrorCodes::SUCCESS) {
                qDebug() << "Notify Chat Msg Failed, err is " << err;
                return;
            }
            qDebug() << "Receive Text Chat Notify Success ";
            qint64 fallback_ts = jsonObj["timestamp"].toVariant().toLongLong();
            const QJsonArray arr = jsonObj["text_array"].toArray();
            qDebug() << "[chat-notify] from=" << jsonObj["fromuid"].toInt()
                     << "to=" << jsonObj["touid"].toInt()
                     << "items=" << arr.size()
                     << "fallback_ts=" << fallback_ts;
            auto msg_ptr = std::make_shared<TextChatMsg>(
                jsonObj["fromuid"].toInt(),
                jsonObj["touid"].toInt(),
                arr,
                fallback_ts);
            emit sig_text_chat_msg(msg_ptr);
        });

    _handlers.insert(
        ID_NOTIFY_OFF_LINE_REQ, [this](ReqId id, int len, QByteArray data) {
            Q_UNUSED(len);
            qDebug() << "handle id is " << id << " data is " << data;
            updateLastResponseTime();
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
                qDebug() << "Kick Msg Failed, err is Json Parse Err" << err;
                return;
            }
            int err = jsonObj["error"].toInt();
            if (err != ErrorCodes::SUCCESS) {
                qDebug() << "Kick Msg Failed, err is " << err;
                return;
            }
            emit sig_offline();
            stopHeartbeat();
        });

    _handlers.insert(
        ID_HEARTBEAT_RSP, [this](ReqId id, int len, QByteArray data) {
            Q_UNUSED(len);
            qDebug() << "Heartbeat response received";

            // Update last response time
            updateLastResponseTime();
        });

    // Handle heartbeat probe request from server
    _handlers.insert(
        ID_HEART_BEAT_REQ, [this](ReqId id, int len, QByteArray data) {
            Q_UNUSED(len);
            qDebug() << "Heartbeat request received";

            // Parse the message to check if it's a probe from server
            QJsonDocument jsonDoc = QJsonDocument::fromJson(data);
            if (jsonDoc.isNull()) {
                qDebug() << "Failed to parse heartbeat request";
                return;
            }

            QJsonObject jsonObj = jsonDoc.object();

            // Update last response time (we received something from server)
            updateLastResponseTime();

            // If this is a probe from server, send heartbeat response
            // immediately
            if (jsonObj.contains("probe") && jsonObj["probe"].toBool()) {
                qDebug()
                    << "Probe received from server, sending heartbeat response";
                sendHeartbeat();
            }
        });


    _handlers.insert(
        ID_PULL_HISTORY_MSG_RSP, [this](ReqId id, int len, QByteArray data) {
            Q_UNUSED(id);
            Q_UNUSED(len);
            updateLastResponseTime();
            QJsonDocument jsonDoc = QJsonDocument::fromJson(data);
            if (jsonDoc.isNull() || !jsonDoc.isObject()) {
                if (_wait_history_rsp) {
                    _wait_history_rsp = false;
                    emit sig_switch_chat_dialog();
                }
                return;
            }
            QJsonObject jsonObj = jsonDoc.object();
            if (jsonObj["error"].toInt() != ErrorCodes::SUCCESS) {
                if (_wait_history_rsp) {
                    _wait_history_rsp = false;
                    emit sig_switch_chat_dialog();
                }
                return;
            }
            const int  uid      = jsonObj["uid"].toInt();
            QJsonArray messages = jsonObj["messages"].toArray();
            std::vector<std::shared_ptr<TextChatData>> allMsgs;
            for (const auto& m : messages) {
                QJsonObject msgObj    = m.toObject();
                int         fromuid   = msgObj["fromuid"].toInt();
                int         touid     = msgObj["touid"].toInt();
                qint64      root_ts   = msgObj["timestamp"].toVariant().toLongLong();
                QJsonArray  textArray = msgObj["text_array"].toArray();
                for (const auto& t : textArray) {
                    QJsonObject tObj = t.toObject();
                    qint64      msg_ts
                        = tObj["timestamp"].toVariant().toLongLong();
                    if (msg_ts <= 0) {
                        msg_ts = root_ts;
                    }
                    if (msg_ts <= 0) {
                        const QString msgid = tObj["msgid"].toString();
                        if (msgid.startsWith("msg_")) {
                            const int second = msgid.indexOf('_', 4);
                            if (second > 4) {
                                bool ok = false;
                                msg_ts = msgid.mid(4, second - 4).toLongLong(&ok);
                                if (!ok) {
                                    msg_ts = 0;
                                }
                            }
                        }
                    }
                    if (msg_ts <= 0) {
                        msg_ts = QDateTime::currentSecsSinceEpoch();
                    }
                    allMsgs.push_back(
                        std::make_shared<TextChatData>(
                            tObj["msgid"].toString(),
                            tObj["content"].toString(),
                            fromuid,
                            touid,
                            msg_ts));
                }
            }
            _sync.ApplyHistory(uid, allMsgs);
            if (_wait_history_rsp) {
                _wait_history_rsp = false;
                emit sig_switch_chat_dialog();
            }
        });

    _handlers.insert(
        ID_NOTIFY_USER_ICON_REQ, [this](ReqId id, int len, QByteArray data) {
            QJsonDocument doc = QJsonDocument::fromJson(data);
            if (!doc.isObject()) return;
            auto obj = doc.object();

            int     uid  = obj.value("uid").toInt();
            QString icon = obj.value("icon").toString();
            if (uid <= 0 || icon.isEmpty()) return;

            emit sig_friend_icon_updated(uid, icon);
        });
}

void TcpManager::CloseConnection() {
    // _socket.close();
    stopHeartbeat();

    // Close the socket
    if (_socket.state() != QAbstractSocket::UnconnectedState) {
        _socket.disconnectFromHost();
        _socket.waitForDisconnected(1000);   // Wait up to 1 second
    }

    qDebug() << "Connection closed";
}

TcpManager::TcpManager()
    : _host("")
    , _recv_pending(false)
    , _message_id(0)
    , _message_len(0)
    , _heartbeat_timer(nullptr)
    , _timeout_timer(nullptr) {
    initHandlers();

    QString app_path = QCoreApplication::applicationDirPath();
    QString fileName = "config.ini";
    QString config_path
        = QDir::toNativeSeparators(app_path + QDir::separator() + fileName);
    QSettings settings(config_path, QSettings::IniFormat);
    _heartbeat_interval
        = settings.value("ChatServer/heartbeat_interval").toInt();
    _is_heartbeat_enabled
        = settings.value("ChatServer/heartbeat_enabled").toBool();
    _connection_timeout
        = settings.value("ChatServer/connection_timeout").toInt();

    QObject::connect(&_socket, &QTcpSocket::connected, [&]() {
        qDebug() << "Connected to the server";
        emit sig_con_success(true);
    });

    QObject::connect(&_socket, &QTcpSocket::readyRead, [this]() {
        _buffer.append(_socket.readAll());
        while (true) {
            const int HEADER_SIZE = 6;
            if (_buffer.size() < HEADER_SIZE) {
                break;
            }

            QDataStream headStream(_buffer);
            headStream.setByteOrder(QDataStream::BigEndian);

            quint32 message_len;
            quint16 message_id;
            headStream >> message_len >> message_id;

            if (_buffer.size() < static_cast<int>(HEADER_SIZE + message_len)) {
                break;
            }
            QByteArray messageBody = _buffer.mid(HEADER_SIZE, message_len);
            _buffer                = _buffer.mid(HEADER_SIZE + message_len);
            qDebug() << "msg id is " << message_id
                     << " msg len is: " << message_len
                     << "msg body is: " << _buffer;
            handleMsg(ReqId(message_id), message_len, messageBody);
        }
    });
    QObject::connect(
        &_socket,
        QOverload<QAbstractSocket::SocketError>::of(&QTcpSocket::errorOccurred),
        [&](QAbstractSocket::SocketError socketError) {
            Q_UNUSED(socketError)
            qDebug() << "Error:" << _socket.errorString();
        });

    QObject::connect(&_socket, &QTcpSocket::disconnected, [&]() {
        qDebug() << "Disconnected from the sever";
        stopHeartbeat();
    });

    QObject::connect(
        this, &TcpManager::sig_send_data, this, &TcpManager::slot_send_data);
    QObject::connect(
        &_sync,
        &MessageSyncCoordinator::sig_request_history,
        this,
        [this](int uid, int days, int limit) {
            QJsonObject   obj = BuildHistoryRequest(uid, days, limit);
            QJsonDocument doc(obj);
            emit          sig_send_data(
                ReqId::ID_PULL_HISTORY_MSG_REQ,
                QString::fromUtf8(doc.toJson(QJsonDocument::Compact)));
        });
}

void TcpManager::startHeartbeat() {
    if (!_is_heartbeat_enabled) {
        qDebug() << "Heartbeat is disabled";
        return;
    }

    // Stop existing timers if any
    stopHeartbeat();

    // Initialize heartbeat timer
    _heartbeat_timer = new QTimer(this);
    connect(
        _heartbeat_timer, &QTimer::timeout, this, &TcpManager::sendHeartbeat);
    _heartbeat_timer->start(
        _heartbeat_interval * 1000);   // Convert to milliseconds

    // Initialize timeout checker timer
    _timeout_timer = new QTimer(this);
    connect(_timeout_timer, &QTimer::timeout, this, &TcpManager::checkTimeout);
    _timeout_timer->start(5000);   // Check every 5 seconds

    // Initialize last response time to now
    _last_response_time = QDateTime::currentDateTime();

    qDebug() << "Heartbeat started - interval:" << _heartbeat_interval
             << "s, timeout:" << _connection_timeout << "s";
}

void TcpManager::stopHeartbeat() {
    if (_heartbeat_timer) {
        _heartbeat_timer->stop();
        delete _heartbeat_timer;
        _heartbeat_timer = nullptr;
    }

    if (_timeout_timer) {
        _timeout_timer->stop();
        delete _timeout_timer;
        _timeout_timer = nullptr;
    }

    qDebug() << "Heartbeat stopped";
}

void TcpManager::sendHeartbeat() {
    if (_socket.state() != QAbstractSocket::ConnectedState) {
        qDebug() << "Cannot send heartbeat - socket not connected";
        return;
    }

    // Construct heartbeat request JSON
    QJsonObject jsonObj;
    jsonObj["error"]     = 0;
    jsonObj["timestamp"] = QDateTime::currentSecsSinceEpoch();

    QJsonDocument jsonDoc(jsonObj);
    QString       jsonString
        = QString::fromUtf8(jsonDoc.toJson(QJsonDocument::Compact));

    // Send heartbeat packet
    emit sig_send_data(ReqId::ID_HEART_BEAT_REQ, jsonString);
    qDebug() << "Heartbeat sent";
}

void TcpManager::updateLastResponseTime() {
    _last_response_time = QDateTime::currentDateTime();
}

void TcpManager::checkTimeout() {
    QDateTime now                         = QDateTime::currentDateTime();
    qint64    seconds_since_last_response = _last_response_time.secsTo(now);

    if (seconds_since_last_response >= _connection_timeout) {
        qWarning() << "Connection timeout detected! Last response:"
                   << seconds_since_last_response << "seconds ago";

        // Stop heartbeat timers
        stopHeartbeat();

        // Emit connection lost signal
        emit sig_connection_lost();
    }
}

void TcpManager::slot_tcp_connect(ServerInfo si) {
    qDebug() << "receive tcp connect signal";
    qDebug() << "Connecting to server...";
    _host = si.Host;
    _port = static_cast<uint16_t>(si.Port.toUInt());
    _socket.connectToHost(_host, _port);
}

void TcpManager::slot_send_data(ReqId reqid, QString data) {
    uint16_t   id   = static_cast<uint16_t>(reqid);
    QByteArray body = data.toUtf8();
    quint32    len  = static_cast<quint32>(body.size());

    QByteArray  block;
    QDataStream out(&block, QIODevice::WriteOnly);
    out.setByteOrder(QDataStream::BigEndian);
    out << len << id;
    block.append(body);
    _socket.write(block);
    qDebug() << "send body_len =" << len << "msg_id =" << id
             << "chars =" << data.size() << "bytes =" << body.size()
             << "body =" << body;
}
