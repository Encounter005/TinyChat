#include "chat_time.h"

#include <QDateTime>

QString FormatChatTimeText(qint64 timestamp_secs) {
    if (timestamp_secs <= 0) {
        timestamp_secs = QDateTime::currentSecsSinceEpoch();
    }
    return QDateTime::fromSecsSinceEpoch(timestamp_secs)
        .toString("yyyy-MM-dd hh:mm");
}
