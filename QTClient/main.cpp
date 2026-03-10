#include "mainwindow.h"
#include "global.h"
#include "datapaths.h"
#include <QApplication>
#include <QSettings>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QString>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QFile qss("./style/stylesheet.qss");
    qDebug("Ready to read qss file");
    if(qss.open(QFile::ReadOnly)) {
        qDebug("Open success!");
        QString style = QLatin1String(qss.readAll());
        a.setStyleSheet (style);
        qss.close();
    } else {
        qDebug("Open failed!");
    }

    QString app_path = QCoreApplication::applicationDirPath();
    QString fileName = "config.ini";
    QString config_path = QDir::toNativeSeparators(app_path + QDir::separator() + fileName);

    QSettings settings(config_path, QSettings::IniFormat);
    QString gate_host = settings.value("GateServer/host").toString();
    QString gate_port = settings.value("GateServer/port").toString();
    gate_url_prefix = "http://" + gate_host + ":" + gate_port;
    qDebug() << gate_url_prefix;

    // 初始化本地数据目录（头像/聊天库/文件/图片）
    DataPaths::DataRootDir();
    DataPaths::AvatarDir();
    DataPaths::ChatDir();
    DataPaths::FilesDir();
    DataPaths::ImagesDir();

    MainWindow w;
    w.show();
    return a.exec();
}
