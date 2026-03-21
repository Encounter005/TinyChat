#include "botplatformsettings.h"

#include "datapaths.h"

#include <QDir>
#include <QSettings>

QString BotPlatformSettings::DefaultPlatform() {
    return QStringLiteral("astrbot");
}

QStringList BotPlatformSettings::AvailablePlatforms() {
    return {QStringLiteral("astrbot"), QStringLiteral("vane")};
}

QString BotPlatformSettings::DisplayNameForPlatform(const QString& platform) {
    const QString normalized = NormalizePlatform(platform);
    if (normalized == QStringLiteral("vane")) {
        return QStringLiteral("Vane");
    }
    return QStringLiteral("AstrBot");
}

QString BotPlatformSettings::LoadPlatformForBot() {
    QSettings settings(SettingsPath(), QSettings::IniFormat);
    return NormalizePlatform(
        settings.value(QStringLiteral("bot/platform"), DefaultPlatform())
            .toString());
}

void BotPlatformSettings::SavePlatformForBot(const QString& platform) {
    QSettings settings(SettingsPath(), QSettings::IniFormat);
    settings.setValue(
        QStringLiteral("bot/platform"), NormalizePlatform(platform));
    settings.sync();
}

QString BotPlatformSettings::NormalizePlatform(const QString& platform) {
    const QString normalized = platform.trimmed().toLower();
    if (AvailablePlatforms().contains(normalized)) {
        return normalized;
    }
    return DefaultPlatform();
}

QString BotPlatformSettings::SettingsPath() {
    return DataPaths::DataRootDir() + QDir::separator()
           + QStringLiteral("bot_platform.ini");
}
