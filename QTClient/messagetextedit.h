#ifndef MESSAGETEXTEDIT_H
#define MESSAGETEXTEDIT_H

#include "global.h"
#include <QApplication>
#include <QDrag>
#include <QFileIconProvider>
#include <QFileInfo>
#include <QInputMethodEvent>
#include <QMimeData>
#include <QMimeType>
#include <QMouseEvent>
#include <QObject>
#include <QPainter>
#include <QTextEdit>
#include <QVector>


class MessageTextEdit : public QTextEdit {
    Q_OBJECT
public:
    explicit MessageTextEdit(QWidget *parent = nullptr);

    ~MessageTextEdit();

    QVector<MsgInfo> getMsgList();

    void insertFileFromUrl(const QStringList &urls);
signals:
    void send();

protected:
    void dragEnterEvent(QDragEnterEvent *event);
    void dropEvent(QDropEvent *event);
    void inputMethodEvent(QInputMethodEvent *event) override;
    void keyPressEvent(QKeyEvent *e);

private:
    void insertImages(const QString &url);
    void insertTextFile(const QString &url);
    bool canInsertFromMimeData(const QMimeData *source) const;
    void insertFromMimeData(const QMimeData *source);

private:
    bool isImage(QString url);   // 判断文件是否为图片
    void insertMsgList(
        QVector<MsgInfo> &list, QString flag, QString text, QPixmap pix);

    QStringList getUrl(QString text);
    // 获取文件图标及大小信息，并转化成图片
    QPixmap getFileIconPixmap(const QString &url);
    QString getFileSize(qint64 size);   // 获取文件大小

private slots:
    void textEditChanged();

private:
    QVector<MsgInfo> mMsgList;
    QVector<MsgInfo> mGetMsgList;
    bool             m_hasActivePreedit = false;
};

#endif   // MESSAGETEXTEDIT_H
