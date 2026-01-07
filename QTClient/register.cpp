#include "register.h"
#include "ui_register.h"
#include <QLineEdit>

Register::Register(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::Register)
{
    ui->setupUi(this);
    ui->pass_edit->setEchoMode(QLineEdit::Password);
    ui->confirm_edit->setEchoMode(QLineEdit::Password);
    ui->error_tips->setProperty("state", "normal");
    this->initHttpHandlers();
    repolish(ui->error_tips);
    auto httpManager = HttpManager::getInstance();
    QObject::connect(httpManager.get(), &HttpManager::sig_reg_mod_finish, this, &Register::slot_reg_mod_finish);
}

Register::~Register()
{
    delete ui;
}

void Register::showTip(QString str)
{
    ui->error_tips->setText(str);
    ui->error_tips->setProperty("state", "error");
    qDebug("change the error_tip state to error");
    repolish(ui->error_tips);
}

void Register::showTip(QString str, bool st)
{
    if(!st) {
        ui->error_tips->setProperty("state", "error");
    } else {
        ui->error_tips->setProperty("state", "normal");
    }
    ui->error_tips->setText(str);
    repolish(ui->error_tips);
}

void Register::initHttpHandlers()
{
    _handlers.insert(ReqId::ID_GET_VARIFY_CODE, [this](QJsonObject jsonObj){
        int error = jsonObj["error"].toInt();
        if(error != ErrorCodes::SUCCESS) {
            showTip(tr("param error!"), false);
            return;
        }
        auto email = jsonObj["email"].toString();
        showTip(tr("验证码已发送到邮箱，注意查收"), true);
        qDebug() << "email is " << email;
    });
}


void Register::on_get_code_button_clicked()
{
    //验证邮箱的地址正则表达式
    auto email = ui->email_edit->text();
    // 邮箱地址的正则表达式
    QRegularExpression regex(R"((\w+)(\.|_)?(\w*)@(\w+)(\.(\w+))+)");
    bool match = regex.match(email).hasMatch(); // 执行正则表达式匹配
    if(match) {
        //发送http请求获取验证码
        QJsonObject json_obj;
        json_obj["email"] = email;
        HttpManager::getInstance()->PostHttpReq(QUrl( gate_url_prefix + "/get_varifycode"), json_obj, ReqId::ID_GET_VARIFY_CODE, Modules::REGISTERMOD);

    } else{
        //提示邮箱不正确
        showTip(tr("邮箱地址不正确"));
    }
}

void Register::slot_reg_mod_finish(ReqId id, QString res, ErrorCodes error_code)
{
    if(error_code != ErrorCodes::SUCCESS) {
        showTip(tr("Network request error!"));
        return;
    }
    // parse Json
    QJsonDocument jsonDoc = QJsonDocument::fromJson(res.toUtf8());
    if(jsonDoc.isEmpty() || !jsonDoc.isObject()) {
        showTip(tr("Json parsing error"), false);
        return;
    }

    auto it = _handlers.find(id);
    if(it != _handlers.end()) {
        if(it.value()) {
            it.value()(jsonDoc.object());
        } else {
            qWarning() << "Handler for ID " << id << " is empty!";
        }
    } else {
        qWarning() << "No handler registered for ID: " << id;
    }

    return;
}

