#ifndef DOCPAYLOAD_H
#define DOCPAYLOAD_H

#include <QString>
#include <QtGlobal>

struct FilePayloadInfo {
    QString remote_name;
    QString display_name;
    qint64  size = 0;
    bool    is_doc = false;
};

QString BuildFilePayload(const QString &remote_name,
                         const QString &display_name,
                         qint64 file_size,
                         bool is_doc);

bool ParseFilePayload(const QString &content, FilePayloadInfo *out);
bool IsOfficeDocument(const QString &path_or_name);

#endif
