#ifndef DATAPATHS_H
#define DATAPATHS_H

#include <QString>

class DataPaths {
public:
    static QString QtClientRootDir();
    static QString DataRootDir();

    static QString AvatarDir();
    static QString AvatarDirForUid(int uid);
    static QString AvatarPath(int uid, const QString& iconName);

    static QString ChatDir();
    static QString ChatCacheDbPath();

    static QString FilesDir();
    static QString ImagesDir();

private:
    static QString EnsureDir(const QString& absPath);
};

#endif // DATAPATHS_H
