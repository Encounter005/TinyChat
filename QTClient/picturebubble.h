#ifndef PICTUREBUBBLE_H
#define PICTUREBUBBLE_H

#include "bubbleframe.h"
#include <QHBoxLayout>
#include <QPixmap>

class QLabel;
class QProgressBar;

class PictureBubble : public BubbleFrame
{
    Q_OBJECT
public:
    PictureBubble(const QPixmap &picture, ChatRole role, QWidget *parent = nullptr);
    void setUploadProgress(int progress);
    void setUploadDone();
    void setUploadFailed();
    void setPicture(const QPixmap &picture);

private:
    QLabel *m_pictureLabel;
    QProgressBar *m_progressBar;
};

#endif // PICTUREBUBBLE_H
