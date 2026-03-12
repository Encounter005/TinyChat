#ifndef BOTUSER_H
#define BOTUSER_H

#include <QObject>
#include <userdata.h>
#include <QString>
#include <memory>

constexpr int BOT_UID = -1;
const QString BOT_NAME = QStringLiteral("AI Bot");
const QString BOT_ICON = QStringLiteral("bot.jpg");

inline bool IsBotUid(int uid) {
    return uid == BOT_UID;
}

QString BotAvatarPath();
std::shared_ptr<UserInfo> BuildBotUserInfo();




#endif   // BOTUSER_H
