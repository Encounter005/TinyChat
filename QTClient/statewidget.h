#ifndef STATEWIDGET_H
#define STATEWIDGET_H

#include <QObject>
#include <QWidget>
#include <QPaintEvent>
#include "clickedlabel.h"

class StateWidget : public QLabel
{
    Q_OBJECT
public:
    explicit StateWidget(QWidget* parent = nullptr);
    void SetState(QString normal = "",
                  QString hover = "",
                  QString press = "",
                  QString select = "",
                  QString select_hover = "",
                  QString select_press ="");
    ClickLbState GetCurrentState();
    void ClearState();
    void SetSelected(bool bselected);
    void AddRedPoint();
    void ShowRedPoint(bool);
protected:
    void painEvent(QPaintEvent* event);
    virtual void mousePressEvent(QMouseEvent* event) override;
    virtual void mouseReleaseEvent(QMouseEvent* event) override;
    virtual void enterEvent(QEvent* event) override;
    virtual void leaveEvent(QEvent* event) override;
private:
    QString _normal;
    QString _normal_hover;
    QString _normal_press;
    QString _selected;
    QString _selected_hover;
    QString _selected_press;
    ClickLbState _curstate;
    QLabel* _red_point;
    QLabel* _label;
signals:
    void clicked();
public slots:


};

#endif // STATEWIDGET_H
