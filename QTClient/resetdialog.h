#ifndef RESETDIALOG_H
#define RESETDIALOG_H

#include <QDialog>
#include "global.h"

namespace Ui {
class ResetDialog;
}

class ResetDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ResetDialog(QWidget *parent = nullptr);
    ~ResetDialog();
    void initHttpHandlers();
    void showTip(QString str, bool st);
public slots:
    void slot_reset_mod_finish(ReqId id, QString res, ErrorCodes error_code);

private slots:
    void on_varify_code_btn_clicked();

    void on_confirm_btn_clicked();

    void on_return_btn_clicked();

private:

    void AddTipErr(TipErr te, QString tips);
    void DelTipErr(TipErr te);
    bool checkUserValid();
    bool checkEmailValid();
    bool checkPassValid();
    bool checkVarifyValid();
    Ui::ResetDialog *ui;
    QMap<ReqId, std::function<void(const QJsonObject&)>> _handlers;
    QMap<TipErr, QString> _tip_errs;
signals:
    void sigSwitchLogin();
};




#endif // RESETDIALOG_H
