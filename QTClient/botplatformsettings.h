#ifndef BOTPLATFORMSETTINGS_H
#define BOTPLATFORMSETTINGS_H

#include <QString>
#include <QStringList>

class BotPlatformSettings {
public:
    static QString     DefaultPlatform();
    static QStringList AvailablePlatforms();
    static QString     DisplayNameForPlatform(const QString& platform);
    static QString     LoadPlatformForBot();
    static void        SavePlatformForBot(const QString& platform);

private:
    static QString NormalizePlatform(const QString& platform);
    static QString SettingsPath();
};

#endif   // BOTPLATFORMSETTINGS_H
