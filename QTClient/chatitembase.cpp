#include "chatitembase.h"
#include "bubbleframe.h"
#include <QFont>
#include <QFontMetrics>
#include <QVBoxLayout>

ChatItemBase::ChatItemBase(ChatRole role, QWidget *parent)
    : QWidget(parent), m_role(role) {
    m_pNameLabel = new QLabel();
    m_pNameLabel->setObjectName("chat_user_name");
    QFont font("Microsoft YaHei");
    font.setPointSize(9);
    m_pNameLabel->setFont(font);
    m_pNameLabel->setFixedHeight(20);
    m_pIconLabel = new QLabel();
    m_pIconLabel->setScaledContents(true);
    m_pIconLabel->setFixedSize(42, 42);
    m_pBubble          = new QWidget();
    auto *bubbleLayout = new QVBoxLayout(m_pBubble);
    bubbleLayout->setContentsMargins(0, 0, 0, 0);
    bubbleLayout->setSpacing(4);

    m_pBubbleContent = new QWidget(m_pBubble);
    bubbleLayout->addWidget(m_pBubbleContent);

    m_pTimeLabel = new QLabel(m_pBubble);
    m_pTimeLabel->setObjectName("chat_time_label");
    QFont timeFont("Microsoft YaHei");
    timeFont.setPointSize(9);
    m_pTimeLabel->setFont(timeFont);
    m_pTimeLabel->setStyleSheet("color: #9fb3db;");
    m_pTimeLabel->setAlignment(
        m_role == ChatRole::SELF ? Qt::AlignRight : Qt::AlignLeft);
    const int timeWidth = QFontMetrics(timeFont).horizontalAdvance(
                              QStringLiteral("0000-00-00 00:00"))
                          + 16;
    m_pTimeLabel->setMinimumWidth(timeWidth);
    bubbleLayout->addWidget(
        m_pTimeLabel,
        0,
        m_role == ChatRole::SELF ? Qt::AlignRight : Qt::AlignLeft);

    QGridLayout *pGLayout = new QGridLayout();
    pGLayout->setVerticalSpacing(3);
    pGLayout->setHorizontalSpacing(3);
    pGLayout->setMargin(3);
    QSpacerItem *pSpacer
        = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);
    if (m_role == ChatRole::SELF) {
        m_pNameLabel->setContentsMargins(0, 0, 8, 0);
        m_pNameLabel->setAlignment(Qt::AlignRight);
        pGLayout->addWidget(m_pNameLabel, 0, 1, 1, 1);
        pGLayout->addWidget(m_pIconLabel, 0, 2, 2, 1, Qt::AlignTop);
        pGLayout->addItem(pSpacer, 1, 0, 1, 1);
        pGLayout->addWidget(m_pBubble, 1, 1, 1, 1, Qt::AlignRight);
        pGLayout->setColumnStretch(0, 2);
        pGLayout->setColumnStretch(1, 3);
    } else {
        m_pNameLabel->setContentsMargins(8, 0, 0, 0);
        m_pNameLabel->setAlignment(Qt::AlignLeft);
        pGLayout->addWidget(m_pIconLabel, 0, 0, 2, 1, Qt::AlignTop);
        pGLayout->addWidget(m_pNameLabel, 0, 1, 1, 1);
        pGLayout->addWidget(m_pBubble, 1, 1, 1, 1, Qt::AlignLeft);
        pGLayout->addItem(pSpacer, 2, 2, 1, 1);
        pGLayout->setColumnStretch(1, 3);
        pGLayout->setColumnStretch(2, 2);
    }
    this->setLayout(pGLayout);
}

void ChatItemBase::setUserName(const QString &name) {
    m_pNameLabel->setText(name);
}

void ChatItemBase::setUserIcon(const QPixmap &icon) {
    m_pIconLabel->setPixmap(icon);
}

void ChatItemBase::setWidget(QWidget *w) {
    auto *bubbleLayout = qobject_cast<QVBoxLayout *>(m_pBubble->layout());
    if (bubbleLayout == nullptr) {
        return;
    }

    bubbleLayout->replaceWidget(m_pBubbleContent, w);
    bubbleLayout->setAlignment(
        w, m_role == ChatRole::SELF ? Qt::AlignRight : Qt::AlignLeft);
    delete m_pBubbleContent;
    m_pBubbleContent = w;

    if (auto *bubble_frame = qobject_cast<BubbleFrame *>(w)) {
        syncBubbleTimeLabel(bubble_frame);
        connect(
            bubble_frame,
            &BubbleFrame::timeTextChanged,
            m_pTimeLabel,
            &QLabel::setText);
    } else {
        m_pTimeLabel->clear();
    }
}

void ChatItemBase::syncBubbleTimeLabel(BubbleFrame *bubble_frame) {
    if (bubble_frame == nullptr) {
        m_pTimeLabel->clear();
        return;
    }
    m_pTimeLabel->setText(bubble_frame->timeText());
}
