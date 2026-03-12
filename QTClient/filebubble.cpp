#include "filebubble.h"

#include <QCursor>
#include <QHBoxLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QPixmap>
#include <QProgressBar>
#include <QToolButton>
#include <QVBoxLayout>

FileBubble::FileBubble(const QString &file_name, ChatRole role, QWidget *parent)
    : BubbleFrame(role, parent)
    , m_nameLabel(nullptr)
    , m_iconLabel(nullptr)
    , m_selectBtn(nullptr)
    , m_progressBar(nullptr)
    , m_iconPath(":/img/file.png")
{
    QWidget *container = new QWidget(this);
    QVBoxLayout *vbox = new QVBoxLayout(container);
    vbox->setContentsMargins(0, 0, 0, 0);
    vbox->setSpacing(6);

    m_nameLabel = new QLabel(file_name, container);
    m_nameLabel->setWordWrap(true);
    m_nameLabel->setMaximumWidth(220);
    vbox->addWidget(m_nameLabel);

    QHBoxLayout *hbox = new QHBoxLayout();
    hbox->setContentsMargins(0, 0, 0, 0);
    hbox->setSpacing(8);

    m_iconLabel = new QLabel(container);
    m_iconLabel->setFixedSize(48, 48);
    m_iconLabel->setScaledContents(true);
    m_iconLabel->setPixmap(QPixmap(m_iconPath));
    hbox->addWidget(m_iconLabel);

    m_selectBtn = new QToolButton(container);
    m_selectBtn->setText(QStringLiteral("标记"));
    m_selectBtn->setCheckable(true);
    m_selectBtn->hide();
    hbox->addWidget(m_selectBtn, 0, Qt::AlignVCenter);
    hbox->addStretch(1);

    vbox->addLayout(hbox);

    m_progressBar = new QProgressBar(container);
    m_progressBar->setRange(0, 100);
    m_progressBar->setValue(0);
    m_progressBar->setTextVisible(true);
    m_progressBar->hide();
    vbox->addWidget(m_progressBar);

    setWidget(container);
    setCursor(Qt::PointingHandCursor);

    connect(m_selectBtn, &QToolButton::toggled, this, [this](bool checked) {
        setHighlighted(checked);
        emit sig_select_changed(checked);
    });

    int left = layout()->contentsMargins().left();
    int right = layout()->contentsMargins().right();
    int top = layout()->contentsMargins().top();
    int bottom = layout()->contentsMargins().bottom();
    setFixedSize(240 + left + right, 104 + top + bottom);
}

void FileBubble::setIconPath(const QString &path)
{
    m_iconPath = path;
    if (m_iconLabel) {
        m_iconLabel->setPixmap(QPixmap(m_iconPath));
    }
}

QString FileBubble::iconPath() const
{
    return m_iconPath;
}

void FileBubble::mousePressEvent(QMouseEvent *event)
{
    if (event && event->button() == Qt::LeftButton) {
        emit sig_clicked();
    }
    BubbleFrame::mousePressEvent(event);
}

void FileBubble::setSelectable(bool selectable)
{
    if (!m_selectBtn) {
        return;
    }
    m_selectBtn->setVisible(selectable);
    if (!selectable) {
        m_selectBtn->setChecked(false);
        setHighlighted(false);
    }
}

bool FileBubble::isSelected() const
{
    return m_selectBtn && m_selectBtn->isVisible() && m_selectBtn->isChecked();
}

void FileBubble::setReceived(bool received)
{
    setMuted(received);
    if (!m_selectBtn) {
        return;
    }
    if (received) {
        m_selectBtn->setVisible(false);
        m_selectBtn->setChecked(false);
        setHighlighted(false);
        m_selectBtn->setText(QStringLiteral("已接收"));
    } else {
        m_selectBtn->setText(QStringLiteral("标记"));
    }
}

void FileBubble::setTransferProgress(int progress)
{
    if (!m_progressBar) {
        return;
    }
    m_progressBar->show();
    m_progressBar->setFormat(QStringLiteral("%p%"));
    m_progressBar->setValue(qBound(0, progress, 100));
}

void FileBubble::setTransferDone()
{
    if (!m_progressBar) {
        return;
    }
    m_progressBar->setValue(100);
    m_progressBar->hide();
}

void FileBubble::setTransferFailed()
{
    if (!m_progressBar) {
        return;
    }
    m_progressBar->show();
    m_progressBar->setFormat(QStringLiteral("传输失败"));
}
