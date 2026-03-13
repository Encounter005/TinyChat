#include <QTextBrowser>
#include "bubbleframe.h"
#include <QHBoxLayout>
#include <QPoint>

class TextBubble : public BubbleFrame
{
    Q_OBJECT
public:
    enum class ContentFormat {
        PlainText,
        Markdown,
    };

    TextBubble(
        ChatRole role,
        const QString &text,
        ContentFormat format = ContentFormat::PlainText,
        QWidget *parent = nullptr);
protected:
    bool eventFilter(QObject *o, QEvent *e);
private:
    void adjustTextHeight();
    void setContent(const QString &text);
    void updateHoverCursor(const QPoint &pos);
    void initStyleSheet(ChatRole role);
    int currentMaxTextWidth() const;
    void relayoutForWidth(int maxTextWidth);
private:
    QTextBrowser *m_pTextEdit;
    int m_lastMaxTextWidth = -1;
    ContentFormat m_contentFormat = ContentFormat::PlainText;
};
