#ifndef CHATITEMBASE_H
#define CHATITEMBASE_H

#include "global.h"
#include <QGridLayout>
#include <QLabel>
#include <QWidget>
class BubbleFrame;

class ChatItemBase : public QWidget {
    Q_OBJECT
public:
    explicit ChatItemBase(ChatRole role, QWidget *parent = nullptr);
    void setUserName(const QString &name);
    void setUserIcon(const QPixmap &icon);
    void setWidget(QWidget *w);

private:
    void syncBubbleTimeLabel(BubbleFrame *bubble_frame);

    ChatRole m_role;
    QLabel  *m_pNameLabel;
    QLabel  *m_pIconLabel;
    QWidget *m_pBubble;
    QLabel  *m_pTimeLabel;
    QWidget *m_pBubbleContent;
};


#endif   // CHATITEMBASE_H
