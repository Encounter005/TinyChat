#include "docpayload.h"

#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSet>

namespace {
const QString kFilePayloadPrefix = "[file]";

QSet<QString> BuildOfficeExtSet() {
    return QSet<QString>{"doc", "docx", "xls", "xlsx", "ppt", "pptx"};
}
}   // namespace

QString BuildFilePayload(const QString &remote_name,
                         const QString &display_name,
                         qint64 file_size,
                         bool is_doc) {
    QJsonObject obj;
    obj["remote"] = remote_name;
    obj["name"]   = display_name;
    obj["size"]   = static_cast<qint64>(file_size);
    obj["doc"]    = is_doc;
    obj["ext"]    = QFileInfo(display_name).suffix().toLower();
    return kFilePayloadPrefix
           + QString::fromUtf8(
               QJsonDocument(obj).toJson(QJsonDocument::Compact));
}

bool ParseFilePayload(const QString &content, FilePayloadInfo *out) {
    if (!content.startsWith(kFilePayloadPrefix)) {
        return false;
    }
    const QString json = content.mid(kFilePayloadPrefix.length());
    const QJsonDocument doc = QJsonDocument::fromJson(json.toUtf8());
    if (!doc.isObject()) {
        return false;
    }
    const QJsonObject obj = doc.object();
    if (out) {
        out->remote_name  = obj.value("remote").toString();
        if (out->remote_name.isEmpty()) {
            out->remote_name = obj.value("file").toString();
        }
        out->display_name = obj.value("name").toString();
        if (out->display_name.isEmpty()) {
            out->display_name = obj.value("filename").toString();
        }
        if (out->display_name.isEmpty()) {
            out->display_name = out->remote_name;
        }
        out->size         = obj.value("size").toVariant().toLongLong();
        out->is_doc       = obj.value("doc").toBool(false);
    }
    return true;
}

bool IsOfficeDocument(const QString &path_or_name) {
    const QString ext = QFileInfo(path_or_name).suffix().toLower();
    static const QSet<QString> exts = BuildOfficeExtSet();
    return exts.contains(ext);
}
