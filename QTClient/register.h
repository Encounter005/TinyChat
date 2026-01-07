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

private:
    Ui::Register *ui;
    QMap<ReqId, std::function<void(const QJsonObject&)>> _handlers;
};

#endif // REGISTER_H
