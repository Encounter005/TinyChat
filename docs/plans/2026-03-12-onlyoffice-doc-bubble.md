# OnlyOffice 文档气泡 Implementation Plan

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** 在 QT 客户端对常见 Office 文档显示文档气泡（使用 `img/document.png`），接收端点击可选择“在线编辑/下载”，在线编辑使用 GateServer 提供的 URL 并在默认浏览器打开。

**Architecture:** 前端通过文件消息 payload 增加 `doc` 标记；`ChatPage` 根据标记切换气泡图标和点击逻辑。OnlyOffice 编辑 URL 由 GateServer 生成，客户端只负责拼接并打开。

**Tech Stack:** Qt Widgets (C++17), QtTest, QNetwork/URL, QDesktopServices.

---

## 说明（先读）
1. 这是纯客户端改动，不需要改后端。
2. 我在 worktree 基线检查时发现 backend 测试配置失败（缺少 `test_db.cpp`），这不影响 Qt 客户端改动。
3. GateServer 需要提供可直接打开的编辑 URL，例如：`http://<host>:<port>/onlyoffice/editor?file=<remote_name>`。

---

### Task 1: 新增文档 payload 工具 + 测试（TDD）

**Files:**
- Create: `QTClient/docpayload.h`
- Create: `QTClient/docpayload.cpp`
- Create: `QTClient/test/tst_docpayload.cpp`
- Create: `QTClient/test/docpayload.pro`

**Step 1: 写失败测试（DocPayload + FileBubble）**

创建 `QTClient/test/tst_docpayload.cpp`：

```cpp
#include <QApplication>
#include <QtTest>

#include "docpayload.h"
#include "filebubble.h"

class DocPayloadTest : public QObject {
    Q_OBJECT
private slots:
    void buildAndParseKeepsDocFlag();
    void parseOldPayloadDefaultsToNonDoc();
    void officeExtensionCheck();
};

void DocPayloadTest::buildAndParseKeepsDocFlag() {
    const QString payload = BuildFilePayload("r1.bin", "demo.docx", 123, true);
    FilePayloadInfo info;
    QVERIFY(ParseFilePayload(payload, &info));
    QCOMPARE(info.remote_name, QString("r1.bin"));
    QCOMPARE(info.display_name, QString("demo.docx"));
    QCOMPARE(info.size, static_cast<qint64>(123));
    QVERIFY(info.is_doc);
}

void DocPayloadTest::parseOldPayloadDefaultsToNonDoc() {
    const QString old_payload = QString("[file]{\"remote\":\"r2\",\"name\":\"a.txt\",\"size\":5}");
    FilePayloadInfo info;
    QVERIFY(ParseFilePayload(old_payload, &info));
    QCOMPARE(info.display_name, QString("a.txt"));
    QVERIFY(!info.is_doc);
}

void DocPayloadTest::officeExtensionCheck() {
    QVERIFY(IsOfficeDocument("/tmp/a.docx"));
    QVERIFY(!IsOfficeDocument("/tmp/a.txt"));
}

class FileBubbleTest : public QObject {
    Q_OBJECT
private slots:
    void setIconPathStoresValue();
};

void FileBubbleTest::setIconPathStoresValue() {
    FileBubble bubble("demo", ChatRole::SELF, nullptr);
    bubble.setIconPath(":/img/document.png");
    QCOMPARE(bubble.iconPath(), QString(":/img/document.png"));
}

int main(int argc, char **argv) {
    QApplication app(argc, argv);
    int status = 0;
    DocPayloadTest tc1;
    status |= QTest::qExec(&tc1, argc, argv);
    FileBubbleTest tc2;
    status |= QTest::qExec(&tc2, argc, argv);
    return status;
}

#include "tst_docpayload.moc"
```

创建 `QTClient/test/docpayload.pro`：

```pro
QT += core testlib widgets
CONFIG += c++17 console testcase
TEMPLATE = app
TARGET = tst_docpayload

INCLUDEPATH += ..

SOURCES += \
    tst_docpayload.cpp \
    ../docpayload.cpp \
    ../filebubble.cpp \
    ../bubbleframe.cpp

HEADERS += \
    ../docpayload.h \
    ../filebubble.h \
    ../bubbleframe.h \
    ../global.h
```

**Step 2: 运行测试，确认失败（RED）**

```bash
qmake QTClient/test/docpayload.pro -o QTClient/test/Makefile.docpayload
make -C QTClient/test -f Makefile.docpayload -j$(nproc)
./QTClient/test/tst_docpayload
```

预期：编译失败（因为 `docpayload.h/cpp` 和 `FileBubble::setIconPath/iconPath` 还不存在）。

**Step 3: 写最小实现（GREEN）**

创建 `QTClient/docpayload.h`：

```cpp
#ifndef DOCPAYLOAD_H
#define DOCPAYLOAD_H

#include <QString>

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
```

创建 `QTClient/docpayload.cpp`：

```cpp
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
}

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
           + QString::fromUtf8(QJsonDocument(obj).toJson(QJsonDocument::Compact));
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
        out->display_name = obj.value("name").toString();
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
```

**Step 4: 运行测试，确认通过（GREEN）**

```bash
qmake QTClient/test/docpayload.pro -o QTClient/test/Makefile.docpayload
make -C QTClient/test -f Makefile.docpayload -j$(nproc)
./QTClient/test/tst_docpayload
```

预期：全部 PASS。

---

### Task 2: FileBubble 支持替换图标（TDD 已覆盖）

**Files:**
- Modify: `QTClient/filebubble.h`
- Modify: `QTClient/filebubble.cpp`

**Step 1: 修改头文件**

在 `QTClient/filebubble.h` 增加：

```cpp
public:
    void setIconPath(const QString &path);
    QString iconPath() const;

private:
    QString m_iconPath;
```

**Step 2: 修改实现**

在 `QTClient/filebubble.cpp` 中：

1. 构造时初始化 `m_iconPath` 并用它设置图标：

```cpp
    , m_iconPath(":/img/file.png")
```

```cpp
    m_iconLabel->setPixmap(QPixmap(m_iconPath));
```

2. 添加实现：

```cpp
void FileBubble::setIconPath(const QString &path) {
    m_iconPath = path;
    if (m_iconLabel) {
        m_iconLabel->setPixmap(QPixmap(m_iconPath));
    }
}

QString FileBubble::iconPath() const {
    return m_iconPath;
}
```

**Step 3: 运行 `tst_docpayload`，确认仍然通过**

```bash
qmake QTClient/test/docpayload.pro -o QTClient/test/Makefile.docpayload
make -C QTClient/test -f Makefile.docpayload -j$(nproc)
./QTClient/test/tst_docpayload
```

---

### Task 3: 资源中加入 document 图标

**Files:**
- Modify: `QTClient/resources.qrc`

**Step 1: 添加资源条目**

在 `<qresource prefix="/">` 中新增：

```xml
<file>img/document.png</file>
```

**Step 2: 运行 `tst_docpayload`，确认仍通过**

```bash
qmake QTClient/test/docpayload.pro -o QTClient/test/Makefile.docpayload
make -C QTClient/test -f Makefile.docpayload -j$(nproc)
./QTClient/test/tst_docpayload
```

---

### Task 4: ChatPage 使用文档 payload + 气泡逻辑

**Files:**
- Modify: `QTClient/chatpage.cpp`

**Step 1: 更新 include**

在顶部增加：

```cpp
#include "docpayload.h"
#include <QDesktopServices>
#include <QUrlQuery>
```

**Step 2: 替换旧的 BuildFilePayload/IsFilePayload**

删除 `ChatPage::BuildFilePayload` 和 `ChatPage::IsFilePayload` 的实现，改为调用 `BuildFilePayload` / `ParseFilePayload`。

在 `AppendChatMsg` 中：

```cpp
FilePayloadInfo file_info;
const bool is_file = ParseFilePayload(msg->_msg_content, &file_info);
const bool is_doc = is_file && (file_info.is_doc || IsOfficeDocument(file_info.display_name));
```

并用 `file_info.remote_name / display_name / size` 代替原变量。

**Step 3: 发送端文件分支：设置 doc 标记 + 图标**

在 `type == "file"` 分支中加入：

```cpp
const bool is_doc = IsOfficeDocument(local_path);
...
if (file_bubble && is_doc) {
    file_bubble->setIconPath(":/img/document.png");
}
...
const QString payload = BuildFilePayload(remote_name, display_name, file_size, is_doc);
```

**Step 4: 接收端点击 doc 气泡：弹窗选择在线编辑或下载**

在接收端 `is_file` 分支中：

```cpp
const bool is_doc = file_info.is_doc || IsOfficeDocument(file_info.display_name);
if (is_doc) {
    file_bubble->setIconPath(":/img/document.png");
}
```

替换点击逻辑：

```cpp
connect(
    file_bubble,
    &FileBubble::sig_clicked,
    this,
    [this, msg_id, file_bubble, file_info]() {
        if (!file_bubble) {
            return;
        }
        const bool is_doc = file_info.is_doc || IsOfficeDocument(file_info.display_name);
        if (is_doc) {
            QMessageBox box(this);
            box.setWindowTitle(tr("文档操作"));
            box.setText(tr("请选择操作"));
            QPushButton *btn_edit = box.addButton(tr("在线编辑"), QMessageBox::AcceptRole);
            QPushButton *btn_download = box.addButton(tr("下载"), QMessageBox::DestructiveRole);
            box.addButton(tr("取消"), QMessageBox::RejectRole);
            box.exec();

            if (box.clickedButton() == btn_edit) {
                const QUrl url = BuildOnlyOfficeUrl(file_info.remote_name);
                if (!QDesktopServices::openUrl(url)) {
                    QMessageBox::warning(this, tr("打开失败"), tr("无法打开浏览器"));
                }
                return;
            }
            if (box.clickedButton() != btn_download) {
                return;
            }
        } else {
            const auto ret = QMessageBox::question(
                this, tr("接收文件"), tr("是否接收该文件？"),
                QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);
            if (ret != QMessageBox::Yes) {
                return;
            }
        }
        const QString save_path = DataPaths::FilesDir()
                                  + QDir::separator()
                                  + file_info.remote_name;
        file_bubble->setTransferProgress(0);
        DownloadFileAsync(
            file_info.remote_name,
            save_path,
            [file_bubble](int progress) {
                if (file_bubble) {
                    file_bubble->setTransferProgress(progress);
                }
            },
            [this, msg_id, file_bubble]() {
                if (file_bubble) {
                    file_bubble->setTransferDone();
                    file_bubble->setReceived(true);
                }
                _selected_file_msgids.remove(msg_id);
            },
            [file_bubble](const QString &) {
                if (file_bubble) {
                    file_bubble->setTransferFailed();
                }
            });
    });
```

**Step 5: 添加 OnlyOffice URL 拼接函数**

在 `chatpage.cpp` 顶部 `namespace { ... }` 中添加：

```cpp
QUrl BuildOnlyOfficeUrl(const QString &remote_name) {
    QString app_path = QCoreApplication::applicationDirPath();
    QString config_path = QDir::toNativeSeparators(
        app_path + QDir::separator() + "config.ini");
    QSettings settings(config_path, QSettings::IniFormat);
    const QString host = settings.value("GateServer/host").toString();
    const int port = settings.value("GateServer/port").toInt();

    QUrl url;
    url.setScheme("http");
    url.setHost(host);
    if (port > 0) {
        url.setPort(port);
    }
    url.setPath("/onlyoffice/editor");
    QUrlQuery query;
    query.addQueryItem("file", remote_name);
    url.setQuery(query);
    return url;
}
```

**Step 6: 发送文件对话框加文档过滤**

在 `slot_switch_to_upload` 中把过滤器改为：

```cpp
tr("Documents (*.doc *.docx *.xls *.xlsx *.ppt *.pptx);;All Files (*.*)")
```

**Step 7: 运行测试（回归）**

```bash
qmake QTClient/test/docpayload.pro -o QTClient/test/Makefile.docpayload
make -C QTClient/test -f Makefile.docpayload -j$(nproc)
./QTClient/test/tst_docpayload
```

---

### Task 5: 手动验证（最小）

1. 发送一个 `docx` 文件：应显示 `document.png` 图标。
2. 接收端点击气泡：弹出“在线编辑/下载”。
3. 选择“在线编辑”：默认浏览器打开 `GateServer` URL。
4. 选择“下载”：正常下载到 `DataPaths::FilesDir()`。

---

## 变更清单
**新增/修改的文件**
- `QTClient/docpayload.h`
- `QTClient/docpayload.cpp`
- `QTClient/filebubble.h`
- `QTClient/filebubble.cpp`
- `QTClient/chatpage.cpp`
- `QTClient/resources.qrc`
- `QTClient/test/docpayload.pro`
- `QTClient/test/tst_docpayload.cpp`

---

## 常见问题
1. **点击在线编辑打不开**：确认 `config.ini` 的 `GateServer` host/port 正确，且服务提供 `/onlyoffice/editor` 路径。
2. **图标不显示**：检查 `resources.qrc` 是否加入 `img/document.png`，并重新构建客户端。
3. **测试编译失败**：确保 `QT += widgets` 已加入 `docpayload.pro`。
