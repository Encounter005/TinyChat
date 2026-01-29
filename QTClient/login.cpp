#include "login.h"
#include "httpmanager.h"
#include "tcpmanager.h"
#include "ui_login.h"
#include <QPushButton>
#include <QLineEdit>
#include <QLabel>


Login::Login(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::Login)
{
    ui->setupUi(this);
    initHttpHandlers();
    connect(ui->register_button, &QPushButton::clicked, this, &Login::switchToRegister);
    ui->password_edit->setEchoMode(QLineEdit::Password);
    ui->error_tips->setProperty("state", "normal");
    repolish(ui->error_tips);
    ui->error_tips->clear();

    ui->pass_visible->SetState("unvisible","unvisible_hover","","visible",
                               "visible_hover","");
    connect(ui->pass_visible, &ClickedLabel::clicked, this, [this](){
        auto state = ui->pass_visible->GetCurState();
        if(state == ClickLbState::Normal){
            ui->password_edit->setEchoMode(QLineEdit::Password);
        }else{
            ui->password_edit->setEchoMode(QLineEdit::Normal);
        }
        qDebug() << "Label was clicked!";

    });
    connect(ui->user_lineEdit, &QLineEdit::editingFinished, this, [this](){
        checkEmailValid();
    });
    connect(ui->password_edit, &QLineEdit::editingFinished, this, [this](){
        checkPassValid();
    });
    ui->forget_label->SetState("normal","hover","","selected","selected_hover","");
    ui->forget_label->setCursor(Qt::PointingHandCursor);
    connect(ui->forget_label, &ClickedLabel::clicked, this, &Login::slot_forget_pwd);
    connect(HttpManager::getInstance().get(), &HttpManager::sig_login_mod_finish, this, &Login::slot_login_mod_finish);
    connect(this, &Login::sig_connect_tcp, TcpManager::getInstance().get(), &TcpManager::slot_tcp_connect);
    connect(TcpManager::getInstance().get(), &TcpManager::sig_con_success, this, &Login::slot_tcp_connection_finish);
    connect(TcpManager::getInstance().get(), &TcpManager::sig_login_failed, this, &Login::slot_login_failed);
}

Login::~Login()
{
    delete ui;
}

void Login::showTip(QString str, bool st)
{
    if(!st) {
        ui->error_tips->setProperty("state", "error");
    } else {
        ui->error_tips->setProperty("state", "normal");
    }
    ui->error_tips->setText(str);
    repolish(ui->error_tips);
}

void Login::AddTipErr(TipErr te, QString tips)
{
    _tip_errs[te] = tips;
    showTip(tips, false);

}

void Login::DelTipErr(TipErr te)
{
    _tip_errs.remove(te);
    if(_tip_errs.empty()){
        ui->error_tips->clear();
        return;
    }
    showTip(_tip_errs.first(), false);
}

bool Login::checkEmailValid()
{
    //验证邮箱的地址正则表达式
    auto email = ui->user_lineEdit->text();
    // 邮箱地址的正则表达式
    QRegularExpression regex(R"((\w+)(\.|_)?(\w*)@(\w+)(\.(\w+))+)");
    bool match = regex.match(email).hasMatch(); // 执行正则表达式匹配
    if(!match){
        //提示邮箱不正确
        AddTipErr(TipErr::TIP_EMAIL_ERR, tr("邮箱地址不正确"));
        return false;
    }
    DelTipErr(TipErr::TIP_EMAIL_ERR);
    return true;
}

bool Login::checkPassValid()
{

    auto pass = ui->password_edit->text();
    if(pass.length() < 6 || pass.length()>15){
        //提示长度不准确
        AddTipErr(TipErr::TIP_PWD_ERR, tr("密码长度应为6~15"));
        return false;
    }
    // 创建一个正则表达式对象，按照上述密码要求
    // 这个正则表达式解释：
    // ^[a-zA-Z0-9!@#$%^&*]{6,15}$ 密码长度至少6，可以是字母、数字和特定的特殊字符
    QRegularExpression regExp("^[a-zA-Z0-9!@#$%^&*]{6,15}$");
    bool match = regExp.match(pass).hasMatch();
    if(!match){
        //提示字符非法
        AddTipErr(TipErr::TIP_PWD_ERR, tr("不能包含非法字符"));
        return false;;
    }
    DelTipErr(TipErr::TIP_PWD_ERR);
    return true;
}

void Login::initHttpHandlers()
{
    //注册获取登录回包逻辑
    _handlers.insert(ReqId::ID_LOGIN_USER, [this](QJsonObject jsonObj){
        int error = jsonObj["error"].toInt();
        if(error != ErrorCodes::SUCCESS){
            showTip(tr("参数错误"),false);
            return;
        }
        showTip(tr("登录成功"), true);

        auto data = jsonObj["data"].toObject();

        ServerInfo si;
        si.Uid = data["uid"].toInt();
        si.Host = data["host"].toString();
        si.Port = data["port"].toString();
        si.Token = data["token"].toString();
        _uid = si.Uid;
        _token = si.Token;

        qDebug()<< " uid is " << si.Uid <<" host is "
                 << si.Host << " Port is " << si.Port << " Token is " << si.Token;
        emit sig_connect_tcp(si);
    });
}

bool Login::enableBtn(bool enabled)
{
    ui->login_button->setEnabled(enabled);
    ui->login_button->setEnabled(enabled);
    return true;
}

void Login::slot_forget_pwd()
{
    qDebug()<<"slot forget pwd";
    emit switchReset();

}

void Login::slot_login_mod_finish(ReqId id, QString res, ErrorCodes err)
{
    qDebug() << "err is: " << err;
    if(err != ErrorCodes::SUCCESS){
        showTip(tr("网络请求错误"),false);
        return;
    }
    // 解析 JSON 字符串,res需转化为QByteArray
    QJsonDocument jsonDoc = QJsonDocument::fromJson(res.toUtf8());
    qDebug() << "jsonDoc is: "<< jsonDoc;
    //json解析错误
    if(jsonDoc.isNull()){
        showTip(tr("json解析错误"),false);
        return;
    }
    //json解析错误
    if(!jsonDoc.isObject()){
        showTip(tr("json解析错误"),false);
        return;
    }
    //调用对应的逻辑,根据id回调。
    _handlers[id](jsonDoc.object());
    return;
}

void Login::slot_login_failed(ErrorCodes err)
{
    QString result = QString("登录失败, err is %1").arg(err);
    showTip(result, false);
    enableBtn(true);
}

void Login::on_login_button_clicked()
{
    qDebug()<<"login btn clicked";
    if(checkEmailValid() == false){
        return;
    }
    if(checkPassValid() == false){
        return ;
    }

    auto email = ui->user_lineEdit->text();
    auto passwd = ui->password_edit->text();
    QJsonObject json_obj;
    json_obj["email"] = email;
    json_obj["passwd"] = toMD5(passwd);
    HttpManager::getInstance()->PostHttpReq(QUrl(gate_url_prefix + "/user_login"), json_obj, ReqId::ID_LOGIN_USER, Modules::LOGINMOD);
}

void Login::slot_tcp_connection_finish(bool success)
{
    if(success) {
        showTip(tr("聊天服务连接成功，正在登录..."), true);
        QJsonObject jsonObj;
        jsonObj["uid"] = _uid;
        jsonObj["token"] = _token;
        QJsonDocument doc(jsonObj);
        QString jsonString = doc.toJson(QJsonDocument::Indented);
        TcpManager::getInstance()->sig_send_data(ReqId::ID_CHAT_LOGIN_REQ, jsonString);
    } else {
        showTip(tr("网络异常"), false);
        enableBtn(true);
    }
}

