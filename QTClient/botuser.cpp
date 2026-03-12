#include "botuser.h"
#include "qcoreapplication.h"
#include "qdir.h"



QString BotAvatarPath() {
    return QCoreApplication::applicationDirPath() + QDir::separator() + "img"
           + QDir::separator() + BOT_ICON;
}

std::shared_ptr<UserInfo> BuildBotUserInfo() {
    return std::make_shared<UserInfo>(BOT_UID, BOT_NAME, BOT_NAME, BOT_ICON, 0);
}
