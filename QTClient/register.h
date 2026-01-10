#ifndef REGISTER_H
#define REGISTER_H

#include <QDialog>
#include "global.h"
#include "httpmanager.h"
#include <QRegularExpression>

namespace Ui {
class Register;
}

class Register : public QDialog
{
    Q_OBJECT

public:
    explicit Register(QWidget *parent = nullptr);
    ~Register();
    void showTip(QString str);
    void showTip(QString str, bool st);
    void initHttpHandlers();
private slots:
    void on_get_code_button_clicked();
    void slot_reg_mod_finish(ReqId id, QString res, ErrorCodes error_code);

    void on_confirm_button_clicked();
    void on_return_to_login_btn_clicked();

    void on_cancel_button_clicked();

signals:

    void sigSwitchLogin();

private:

    void AddTipErr(TipErr te, QString tips);
    void DelTipErr(TipErr te);
    bool checkUserValid();
    bool checkEmailValid();
    bool checkPassValid();
    bool checkVarifyValid();
    void ChangeTipPage();
    Ui::Register *ui;
    QMap<ReqId, std::function<void(const QJsonObject&)>> _handlers;
    QMap<TipErr, QString> _tip_errs;
    QTimer* _countdown_timer;
    int _countdown;
};

#endif // REGISTER_H
