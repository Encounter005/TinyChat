#ifndef TCPMANAGER_H
#define TCPMANAGER_H

#include "singleton.h"
#include "global.h"
#include "userdata.h"
#include <QTcpSocket>
#include <QObject>

class TcpManager : public QObject, public SingleTon<TcpManager>,  public std::enable_shared_from_this<TcpManager>
{
    friend class SingleTon<TcpManager>;
    Q_OBJECT
public:
    ~TcpManager() = default;
    void handleMsg(ReqId id, int len, QByteArray data);
    void initHandlers();
private:
    TcpManager();
    QTcpSocket _socket;
    QString _host;
    uint16_t _port;
    QByteArray _buffer;
    bool _recv_pending;
    quint16 _message_id;
    quint32 _message_len;
    QMap<ReqId, std::function<void(ReqId id, int len, QByteArray data)>> _handlers;

public slots:
    void slot_tcp_connect(ServerInfo);
    void slot_send_data(ReqId, QString);
signals:
    void sig_con_success(bool bsuccess);
    void sig_send_data(ReqId, QString data);
    void sig_login_failed(ErrorCodes err);
    void sig_switch_chat_dialog();
    void sig_user_search(std::shared_ptr<SearchInfo>);
    void sig_auth_rsp(std::shared_ptr<AuthRsp>);
    void sig_friend_apply(std::shared_ptr<AddFriendApply>);
    void sig_add_auth_friend(std::shared_ptr<AuthInfo>);
    void sig_text_chat_msg(std::shared_ptr<TextChatMsg> msg);

};

#endif // TCPMANAGER_H
