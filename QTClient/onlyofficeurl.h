#ifndef ONLYOFFICEURL_H
#define ONLYOFFICEURL_H

#include <QUrl>
#include <QString>

QUrl BuildOnlyOfficeUrlFromParts(const QString &host,
                                 int port,
                                 const QString &path,
                                 const QString &remote_name);

#endif
