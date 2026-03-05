#ifndef MESSAGESYNCPROTOCOL_H
#define MESSAGESYNCPROTOCOL_H

#include <QJsonObject>

inline QJsonObject BuildHistoryRequest(int uid, int days, int limit) {
    QJsonObject obj;
    obj["uid"] = uid;
    obj["days"] = days;
    obj["limit"] = limit;
    return obj;
}

#endif // MESSAGESYNCPROTOCOL_H
