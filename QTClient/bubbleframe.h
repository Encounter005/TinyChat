#ifndef BUBBLEFRAME_H
#define BUBBLEFRAME_H

#include "global.h"
#include <QFrame>
#include <QHBoxLayout>
#include <QString>

class BubbleFrame : public QFrame {
    Q_OBJECT
public:
    BubbleFrame(ChatRole role, QWidget *parent = nullptr);
    void    setMargin(int margin);
    void    setTimeText(const QString &time_text);
    QString timeText() const;
    void    setMuted(bool muted);
    void    setHighlighted(bool highlighted);
    // inline int margin(){return margin;}
    void setWidget(QWidget *w);
signals:
    void timeTextChanged(const QString &time_text);

protected:
    void paintEvent(QPaintEvent *e);

private:
    QString      m_timeText;
    QHBoxLayout *m_pHLayout;
    ChatRole     m_role;
    int          m_margin;
    bool         m_muted;
    bool         m_highlighted;
};



#endif   // BUBBLEFRAME_H
