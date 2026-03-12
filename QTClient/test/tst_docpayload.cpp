#include <QApplication>
#include <QtTest>

#include "docpayload.h"
#include "filebubble.h"
#include "onlyofficeurl.h"

class DocPayloadTest : public QObject {
    Q_OBJECT
private slots:
    void buildAndParseKeepsDocFlag();
    void parseOldPayloadDefaultsToNonDoc();
    void officeExtensionCheck();
    void buildOnlyOfficeUrl();
};

void DocPayloadTest::buildAndParseKeepsDocFlag() {
    const QString payload
        = BuildFilePayload("r1.bin", "demo.docx", 123, true);
    FilePayloadInfo info;
    QVERIFY(ParseFilePayload(payload, &info));
    QCOMPARE(info.remote_name, QString("r1.bin"));
    QCOMPARE(info.display_name, QString("demo.docx"));
    QCOMPARE(info.size, static_cast<qint64>(123));
    QVERIFY(info.is_doc);
}

void DocPayloadTest::parseOldPayloadDefaultsToNonDoc() {
    const QString old_payload
        = QString("[file]{\"remote\":\"r2\",\"name\":\"a.txt\","
                  "\"size\":5}");
    FilePayloadInfo info;
    QVERIFY(ParseFilePayload(old_payload, &info));
    QCOMPARE(info.display_name, QString("a.txt"));
    QVERIFY(!info.is_doc);
}

void DocPayloadTest::officeExtensionCheck() {
    QVERIFY(IsOfficeDocument("/tmp/a.docx"));
    QVERIFY(!IsOfficeDocument("/tmp/a.txt"));
}

void DocPayloadTest::buildOnlyOfficeUrl() {
    const QUrl url = BuildOnlyOfficeUrlFromParts(
        "192.168.5.3", 3480, "/", "file_1.docx");
    QCOMPARE(url.scheme(), QString("http"));
    QCOMPARE(url.host(), QString("192.168.5.3"));
    QCOMPARE(url.port(), 3480);
    QCOMPARE(url.path(), QString("/"));
    QCOMPARE(url.query(), QString("file=file_1.docx"));
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
