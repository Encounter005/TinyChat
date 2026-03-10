#include "textbubble.h"
#include <QFontMetricsF>
#include <QDebug>
#include <QFont>
#include "global.h"
#include <QTimer>
#include <QTextDocument>
#include <QTextBlock>
#include <QTextLayout>
#include <QTextOption>
#include <QStyle>
#include <QtMath>
#include <QFont>

TextBubble::TextBubble(ChatRole role, const QString &text, QWidget *parent)
    :BubbleFrame(role, parent)
{
    m_pTextEdit = new QTextEdit();
    m_pTextEdit->setReadOnly(true);
    m_pTextEdit->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_pTextEdit->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_pTextEdit->setFrameShape(QFrame::NoFrame);
    m_pTextEdit->setContentsMargins(0, 0, 0, 0);
    m_pTextEdit->setLineWrapMode(QTextEdit::WidgetWidth);
    m_pTextEdit->setWordWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
    m_pTextEdit->document()->setDocumentMargin(2);
    m_pTextEdit->installEventFilter(this);
    QFont font("Maple Mono NF");
    font.setPointSize(12);
    m_pTextEdit->setFont(font);
    setPlainText(text);
    setWidget(m_pTextEdit);
    initStyleSheet(role);
}

bool TextBubble::eventFilter(QObject *o, QEvent *e)
{
    if(m_pTextEdit == o && e->type() == QEvent::Paint)
    {
        const int currentMax = currentMaxTextWidth();
        if (currentMax != m_lastMaxTextWidth) {
            relayoutForWidth(currentMax);
        }
        adjustTextHeight(); //PaintEvent中设置
    }
    return BubbleFrame::eventFilter(o, e);
}

void TextBubble::setPlainText(const QString &text)
{
    m_pTextEdit->setPlainText(text);
    relayoutForWidth(currentMaxTextWidth());
}

int TextBubble::currentMaxTextWidth() const {
    constexpr int kFallbackMax = 300;
    constexpr int kMinMax = 180;
    constexpr int kHardMax = 420;

    const QWidget* p = parentWidget();
    if (!p) {
        return kFallbackMax;
    }

    int available = p->width();
    if (available <= 0) {
        return kFallbackMax;
    }

    // 给头像、边距和气泡三角预留空间，避免撑满整行
    available -= 150;
    int dynamicMax = static_cast<int>(available * 0.62);
    dynamicMax = qBound(kMinMax, dynamicMax, kHardMax);
    return dynamicMax;
}

void TextBubble::relayoutForWidth(int maxTextWidth) {
    QTextDocument *doc = m_pTextEdit->document();
    doc->setTextWidth(maxTextWidth);
    doc->adjustSize();

    const qreal docMargin = doc->documentMargin();
    int contentWidth = qCeil(doc->idealWidth());
    contentWidth = qMin(contentWidth, maxTextWidth);
    contentWidth = qMax(contentWidth, 24);

    const int editWidth = contentWidth + qCeil(docMargin * 2.0) + 2;
    m_pTextEdit->setFixedWidth(editWidth);

    const int marginLeft = this->layout()->contentsMargins().left();
    const int marginRight = this->layout()->contentsMargins().right();
    setMaximumWidth(editWidth + marginLeft + marginRight);

    m_lastMaxTextWidth = maxTextWidth;
    adjustTextHeight();
}

void TextBubble::adjustTextHeight()
{
    const qreal docMargin = m_pTextEdit->document()->documentMargin();
    QTextDocument *doc = m_pTextEdit->document();
    doc->adjustSize();

    const int textHeight = qCeil(doc->size().height());
    const int editHeight = textHeight + qCeil(docMargin * 2.0) + 2;
    m_pTextEdit->setFixedHeight(editHeight);

    const QMargins margins = this->layout()->contentsMargins();
    setFixedHeight(editHeight + margins.top() + margins.bottom());
}

void TextBubble::initStyleSheet(ChatRole role)
{
    m_pTextEdit->setObjectName("chat_bubble_text");
    if (role == ChatRole::SELF) {
        m_pTextEdit->setProperty("bubble_role", "self");
    } else {
        m_pTextEdit->setProperty("bubble_role", "other");
    }
    m_pTextEdit->style()->unpolish(m_pTextEdit);
    m_pTextEdit->style()->polish(m_pTextEdit);
}
