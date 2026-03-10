#ifndef BUBBLEFRAME_H
#define BUBBLEFRAME_H

#include <QFrame>
#include "global.h"
#include <QHBoxLayout>
#include <QString>

class BubbleFrame : public QFrame
{
    Q_OBJECT
public:
    BubbleFrame(ChatRole role, QWidget *parent = nullptr);
    void setMargin(int margin);
    void setTimeText(const QString &time_text);
    void setMuted(bool muted);
    void setHighlighted(bool highlighted);
    //inline int margin(){return margin;}
    void setWidget(QWidget *w);
protected:
    void paintEvent(QPaintEvent *e);
private:
    QString m_timeText;
    QHBoxLayout *m_pHLayout;
    ChatRole m_role;
    int      m_margin;
    bool     m_muted;
    bool     m_highlighted;
};



#endif // BUBBLEFRAME_H
