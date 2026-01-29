#include <QTextEdit>
#include "bubbleframe.h"
#include <QHBoxLayout>

class TextBubble : public BubbleFrame
{
    Q_OBJECT
public:
    TextBubble(ChatRole role, const QString &text, QWidget *parent = nullptr);
protected:
    bool eventFilter(QObject *o, QEvent *e);
private:
    void adjustTextHeight();
    void setPlainText(const QString &text);
    void initStyleSheet();
private:
    QTextEdit *m_pTextEdit;
};
