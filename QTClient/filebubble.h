#ifndef FILEBUBBLE_H
#define FILEBUBBLE_H

#include "bubbleframe.h"

class QLabel;
class QToolButton;
class QProgressBar;
class QMouseEvent;

class FileBubble : public BubbleFrame
{
    Q_OBJECT
public:
    FileBubble(const QString &file_name, ChatRole role, QWidget *parent = nullptr);

    void setSelectable(bool selectable);
    bool isSelected() const;
    void setReceived(bool received);
    void setTransferProgress(int progress);
    void setTransferDone();
    void setTransferFailed();
    void setIconPath(const QString& path);
    QString iconPath() const;
    

signals:
    void sig_clicked();
    void sig_select_changed(bool selected);

protected:
    void mousePressEvent(QMouseEvent *event) override;

private:
    QLabel *m_nameLabel;
    QLabel *m_iconLabel;
    QToolButton *m_selectBtn;
    QProgressBar *m_progressBar;
    QString m_iconPath;
};

#endif // FILEBUBBLE_H
