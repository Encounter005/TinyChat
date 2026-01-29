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
    int _uid;
    QString _token;
    QMap<ReqId, std::function<void(const QJsonObject&)>> _handlers;
    void showTip(QString str, bool st);
    void AddTipErr(TipErr te, QString tips);
    void DelTipErr(TipErr te);
    bool checkEmailValid();
    bool checkPassValid();
    void initHttpHandlers();
    bool enableBtn(bool);
private slots:
    void slot_forget_pwd();
    void slot_login_mod_finish(ReqId id, QString res, ErrorCodes err);
    void slot_login_failed(ErrorCodes err);
    void on_login_button_clicked();
    void slot_tcp_connection_finish(bool success);

signals:
    void switchToRegister();
    void switchReset();
    void sig_connect_tcp(ServerInfo);
};

#endif // LOGIN_H
