#include "datapaths.h"

#include <QCoreApplication>
#include <QDir>

QString DataPaths::EnsureDir(const QString& absPath) {
    QDir dir;
    dir.mkpath(absPath);
    return absPath;
}

QString DataPaths::QtClientRootDir() {
    QDir current(QCoreApplication::applicationDirPath());
    while (true) {
        if (current.exists("QTClient.pro")) {
            return current.absolutePath();
        }
        if (current.exists("QTClient/QTClient.pro")) {
            return current.absoluteFilePath("QTClient");
        }
        if (!current.cdUp()) {
            break;
        }
    }
    return QCoreApplication::applicationDirPath();
}

QString DataPaths::DataRootDir() {
    const QString root = QtClientRootDir() + QDir::separator() + "data";
    return EnsureDir(root);
}

QString DataPaths::AvatarDir() {
    return EnsureDir(DataRootDir() + QDir::separator() + "avatars");
}

QString DataPaths::AvatarDirForUid(int uid) {
    return EnsureDir(AvatarDir() + QDir::separator() + QString::number(uid));
}

QString DataPaths::AvatarPath(int uid, const QString& iconName) {
    return AvatarDirForUid(uid) + QDir::separator() + iconName;
}

QString DataPaths::ChatDir() {
    return EnsureDir(DataRootDir() + QDir::separator() + "chat");
}

QString DataPaths::ChatCacheDbPath() {
    return ChatDir() + QDir::separator() + "chat_cache.db";
}

QString DataPaths::FilesDir() {
    return EnsureDir(DataRootDir() + QDir::separator() + "files");
}

QString DataPaths::ImagesDir() {
    return EnsureDir(DataRootDir() + QDir::separator() + "images");
}
