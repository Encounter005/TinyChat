#ifndef BUBBLEFRAME_H
#define BUBBLEFRAME_H

#include <QFrame>
#include "global.h"
#include <QHBoxLayout>
class BubbleFrame : public QFrame
{
    Q_OBJECT
public:
    BubbleFrame(ChatRole role, QWidget *parent = nullptr);
    void setMargin(int margin);
    //inline int margin(){return margin;}
    void setWidget(QWidget *w);
protected:
    void paintEvent(QPaintEvent *e);
private:
    QHBoxLayout *m_pHLayout;
    ChatRole m_role;
    int      m_margin;
};



#endif // BUBBLEFRAME_H
