#ifndef CLICKEDBTN_H
#define CLICKEDBTN_H

#include <QQuickItem>
#include <QPushButton>

class ClickedBtn : public QPushButton
{
    Q_OBJECT
public:
    ClickedBtn(QWidget* parent = nullptr);
    ~ClickedBtn();
    void SetState(QString normal, QString hovern, QString press);
protected:
    virtual void enterEvent(QEvent* event) override;
    virtual void leaveEvent(QEvent* event) override;
    virtual void mousePressEvent(QMouseEvent* event) override;
    virtual void mouseReleaseEvent(QMouseEvent* event) override;
private:
    QString _normal;
    QString _hover;
    QString _press;

};

#endif // CLICKEDBTN_H
