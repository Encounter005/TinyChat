#include "onlyofficeurl.h"

#include <QUrlQuery>

QUrl BuildOnlyOfficeUrlFromParts(const QString &host,
                                 int port,
                                 const QString &path,
                                 const QString &remote_name)
{
    QUrl url;
    url.setScheme("http");
    url.setHost(host);
    if (port > 0) {
        url.setPort(port);
    }
    const QString safe_path = path.isEmpty() ? QStringLiteral("/") : path;
    url.setPath(safe_path);
    QUrlQuery query;
    query.addQueryItem("file", remote_name);
    url.setQuery(query);
    return url;
}
