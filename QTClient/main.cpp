#include "mainwindow.h"

#include <QApplication>
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
    MainWindow w;
    w.show();
    return a.exec();
}
