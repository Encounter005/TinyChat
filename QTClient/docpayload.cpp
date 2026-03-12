#include "docpayload.h"
#include "qfileinfo.h"
#include "qjsondocument.h"
#include "qjsonobject.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QFileInfo>
#include <QSet>

namespace {
    const QString kFilePayLoadPrefix = "[file]";
    QSet<QString> BuildOfficeExtSet() {
        return QSet<QString>{"doc", "docx", "xls", "xlsx", "ppt", "pptx"};
    }
}

QString BuildFilePayload(const QString &remote_name, const QString &display_name, qint64 file_size, bool is_doc) {
    QJsonObject obj;
    obj["remote"] = remote_name;
    obj["name"] = display_name;
    obj["size"] = file_size;
    obj["doc"] = is_doc;
    obj["ext"] = QFileInfo(display_name).suffix().toLower();

    return kFilePayLoadPrefix + QString::fromUtf8(QJsonDocument(obj).toJson(QJsonDocument::Compact));
}

bool ParseFilePayload(const QString &content, FilePayloadInfo *out) {
    if(!content.startsWith(kFilePayLoadPrefix)) return false;

    const QString json = content.mid(kFilePayLoadPrefix.length());
    const QJsonDocument doc = QJsonDocument::fromJson(json.toUtf8());
    if(!doc.isObject()) return false;

    const QJsonObject obj = doc.object();
    if(out) {
        out->remote_name = obj.value("remote").toString();
        out->display_name = obj.value("name").toString();
        out->size = obj.value("size").toVariant().toLongLong();
        out->is_doc = obj.value("doc").toBool(false);
    }

    return true;
}


bool IsOfficeDocument(const QString& path_or_name) {
    const QString ext = QFileInfo(path_or_name).suffix().toLower();
    static const QSet<QString> exts = BuildOfficeExtSet();
    return exts.contains(ext);
}
