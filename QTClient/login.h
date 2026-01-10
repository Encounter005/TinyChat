#ifndef LOGIN_H
#define LOGIN_H

#include <QDialog>
#include "global.h"

namespace Ui {
class Login;
}

class Login : public QDialog
{
    Q_OBJECT

public:
    explicit Login(QWidget *parent = nullptr);
    ~Login();

private:
    Ui::Login *ui;
    QMap<TipErr, QString> _tip_errs;
    void showTip(QString str, bool st);
    void AddTipErr(TipErr te, QString tips);
    void DelTipErr(TipErr te);
    bool checkEmailValid();
    bool checkPassValid();
private slots:
    void slot_forget_pwd();

signals:
    void switchToRegister();
    void switchReset();
};

#endif // LOGIN_H
