#include "bubbleframe.h"
#include <QDateTime>
#include <QPainter>
#include <QDebug>

const int WIDTH_SANJIAO  = 8;  //三角宽
const int TIME_AREA_HEIGHT = 20;
const int TIME_TEXT_PADDING = 8;

BubbleFrame::BubbleFrame(ChatRole role, QWidget *parent)
    :QFrame(parent)
    ,m_timeText(QDateTime::currentDateTime().toString("hh:mm"))
    ,m_pHLayout(nullptr)
    ,m_role(role)
    ,m_margin(3)
    ,m_muted(false)
    ,m_highlighted(false)
{
    m_pHLayout = new QHBoxLayout();
    if(m_role == ChatRole::SELF)
        m_pHLayout->setContentsMargins(
            m_margin,
            m_margin,
            WIDTH_SANJIAO + m_margin,
            m_margin + TIME_AREA_HEIGHT);
    else
        m_pHLayout->setContentsMargins(
            WIDTH_SANJIAO + m_margin,
            m_margin,
            m_margin,
            m_margin + TIME_AREA_HEIGHT);

    this->setLayout(m_pHLayout);
}

void BubbleFrame::setMargin(int margin)
{
    Q_UNUSED(margin);
    //m_margin = margin;
}

void BubbleFrame::setWidget(QWidget *w)
{
    if(m_pHLayout->count() > 0)
        return ;
    else{
        m_pHLayout->addWidget(w);
    }
}

void BubbleFrame::setTimeText(const QString &time_text)
{
    m_timeText = time_text;
    update();
}

void BubbleFrame::setMuted(bool muted)
{
    m_muted = muted;
    update();
}

void BubbleFrame::setHighlighted(bool highlighted)
{
    m_highlighted = highlighted;
    update();
}

void BubbleFrame::paintEvent(QPaintEvent *e)
{
    QPainter painter(this);
    painter.setPen(Qt::NoPen);

    const int bubble_height = qMax(1, this->height() - TIME_AREA_HEIGHT);

    if(m_role == ChatRole::OTHER)
    {
        //画气泡
        QColor bk_color = m_muted ? QColor("#3b4e73") : QColor("#162746");
        painter.setBrush(QBrush(bk_color));
        QRect bk_rect = QRect(WIDTH_SANJIAO, 0, this->width()-WIDTH_SANJIAO, bubble_height);
        painter.drawRoundedRect(bk_rect,5,5);
        //画小三角
        QPointF points[3] = {
            QPointF(bk_rect.x(), 12),
            QPointF(bk_rect.x(), 10+WIDTH_SANJIAO +2),
            QPointF(bk_rect.x()-WIDTH_SANJIAO, 10+WIDTH_SANJIAO-WIDTH_SANJIAO/2),
        };
        painter.drawPolygon(points, 3);

        if (m_highlighted) {
            painter.setBrush(Qt::NoBrush);
            painter.setPen(QPen(QColor("#f9e2af"), 2));
            painter.drawRoundedRect(bk_rect.adjusted(1, 1, -1, -1), 5, 5);
            painter.setPen(Qt::NoPen);
        }
    }
    else
    {
        QColor bk_color = m_muted ? QColor("#4769a8") : QColor("#4f86ff");
        painter.setBrush(QBrush(bk_color));
        //画气泡
        QRect bk_rect = QRect(0, 0, this->width()-WIDTH_SANJIAO, bubble_height);
        painter.drawRoundedRect(bk_rect,5,5);
        //画三角
        QPointF points[3] = {
            QPointF(bk_rect.x()+bk_rect.width(), 12),
            QPointF(bk_rect.x()+bk_rect.width(), 12+WIDTH_SANJIAO +2),
            QPointF(bk_rect.x()+bk_rect.width()+WIDTH_SANJIAO, 10+WIDTH_SANJIAO-WIDTH_SANJIAO/2),
        };
        painter.drawPolygon(points, 3);

        if (m_highlighted) {
            painter.setBrush(Qt::NoBrush);
            painter.setPen(QPen(QColor("#f9e2af"), 2));
            painter.drawRoundedRect(bk_rect.adjusted(1, 1, -1, -1), 5, 5);
            painter.setPen(Qt::NoPen);
        }

    }

    painter.setPen(m_muted ? QColor("#7d8eb0") : QColor("#9fb3db"));
    QFont time_font = painter.font();
    time_font.setPointSize(9);
    painter.setFont(time_font);

    QRect time_rect(0, bubble_height, this->width(), TIME_AREA_HEIGHT);
    if (m_role == ChatRole::OTHER) {
        painter.drawText(
            time_rect.adjusted(TIME_TEXT_PADDING, 0, 0, 0),
            Qt::AlignLeft | Qt::AlignVCenter,
            m_timeText);
    } else {
        painter.drawText(
            time_rect.adjusted(0, 0, -WIDTH_SANJIAO - TIME_TEXT_PADDING, 0),
            Qt::AlignRight | Qt::AlignVCenter,
            m_timeText);
    }

    Q_UNUSED(e);
}
