#include "picturebubble.h"
#include <QLabel>
#include <QProgressBar>
#include <QVBoxLayout>


#define PIC_MAX_WIDTH 160
#define PIC_MAX_HEIGHT 90

PictureBubble::PictureBubble(const QPixmap &picture, ChatRole role, QWidget *parent)
    :BubbleFrame(role, parent)
{
    QWidget *container = new QWidget(this);
    QVBoxLayout *vbox = new QVBoxLayout(container);
    vbox->setContentsMargins(0, 0, 0, 0);
    vbox->setSpacing(6);

    m_pictureLabel = new QLabel(container);
    m_pictureLabel->setScaledContents(true);
    QPixmap pix = picture.scaled(QSize(PIC_MAX_WIDTH, PIC_MAX_HEIGHT), Qt::KeepAspectRatio);
    m_pictureLabel->setPixmap(pix);
    vbox->addWidget(m_pictureLabel, 0, Qt::AlignCenter);

    m_progressBar = new QProgressBar(container);
    m_progressBar->setRange(0, 100);
    m_progressBar->setValue(0);
    m_progressBar->setFixedWidth(pix.width());
    m_progressBar->setTextVisible(true);
    vbox->addWidget(m_progressBar, 0, Qt::AlignCenter);

    this->setWidget(container);

    int left_margin = this->layout()->contentsMargins().left();
    int right_margin = this->layout()->contentsMargins().right();
    int top_margin = this->layout()->contentsMargins().top();
    int bottom_margin = this->layout()->contentsMargins().bottom();
    const int progress_h = m_progressBar->sizeHint().height();
    setFixedSize(
        pix.width() + left_margin + right_margin,
        pix.height() + progress_h + 6 + top_margin + bottom_margin);
}

void PictureBubble::setUploadProgress(int progress)
{
    if (!m_progressBar) {
        return;
    }
    m_progressBar->show();
    m_progressBar->setValue(qBound(0, progress, 100));
}

void PictureBubble::setUploadDone()
{
    if (!m_progressBar) {
        return;
    }
    m_progressBar->setValue(100);
    m_progressBar->hide();
}

void PictureBubble::setUploadFailed()
{
    if (!m_progressBar) {
        return;
    }
    m_progressBar->show();
    m_progressBar->setFormat(QStringLiteral("上传失败"));
}

void PictureBubble::setPicture(const QPixmap &picture)
{
    if (!m_pictureLabel) {
        return;
    }
    QPixmap pix = picture.scaled(QSize(PIC_MAX_WIDTH, PIC_MAX_HEIGHT), Qt::KeepAspectRatio);
    m_pictureLabel->setPixmap(pix);
    if (m_progressBar) {
        m_progressBar->setFixedWidth(pix.width());
    }
}
