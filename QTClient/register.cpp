#include "register.h"
#include "ui_register.h"
#include "clickedlabel.h"
#include <QLineEdit>
#include <QCryptographicHash>

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
    //clear tips
    ui->error_tips->clear();

    connect(ui->user_edit,&QLineEdit::editingFinished,this,[this](){
        checkUserValid();
    });
    connect(ui->email_edit, &QLineEdit::editingFinished, this, [this](){
        checkEmailValid();
    });
    connect(ui->pass_edit, &QLineEdit::editingFinished, this, [this](){
        checkPassValid();
    });
    connect(ui->verify_edit, &QLineEdit::editingFinished, this, [this](){
        checkVarifyValid();
    });



    //设置浮动显示手形状
    ui->pass_visible->setCursor(Qt::PointingHandCursor);
    ui->confirm_visible->setCursor(Qt::PointingHandCursor);
    ui->pass_visible->SetState("unvisible","unvisible_hover","","visible",
                               "visible_hover","");
    ui->confirm_visible->SetState("unvisible","unvisible_hover","","visible",
                                  "visible_hover","");
    //连接点击事件
    connect(ui->pass_visible, &ClickedLabel::clicked, this, [this]() {
        auto state = ui->pass_visible->GetCurState();
        if(state == ClickLbState::Normal){
            ui->pass_edit->setEchoMode(QLineEdit::Password);
        }else{
            ui->pass_edit->setEchoMode(QLineEdit::Normal);
        }
        qDebug() << "Label was clicked!";
    });
    connect(ui->confirm_visible, &ClickedLabel::clicked, this, [this]() {
        auto state = ui->confirm_visible->GetCurState();
        if(state == ClickLbState::Normal){
            ui->confirm_edit->setEchoMode(QLineEdit::Password);
        }else{
            ui->confirm_edit->setEchoMode(QLineEdit::Normal);
        }
        qDebug() << "Label was clicked!";

    });
    // 创建定时器
    _countdown_timer = new QTimer(this);
    // 连接信号和槽
    connect(_countdown_timer, &QTimer::timeout, [this](){
        if(_countdown==0){
            _countdown_timer->stop();
            emit sigSwitchLogin();
            return;
        }
        _countdown--;
        auto str = QString("注册成功，%1 s后返回登录").arg(_countdown);
        ui->tip_lab1->setText(str);});


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
        auto data = jsonObj["data"].toString();
        showTip(tr("验证码已发送到邮箱，注意查收"), true);
        qDebug() << "email is: " << data;
        qDebug() << "json obj is: " << jsonObj;
    });

    _handlers.insert(ReqId::ID_REG_UESR, [this](QJsonObject jsonObj){
        int error = jsonObj["error"].toInt();
        if(error != ErrorCodes::SUCCESS) {
            showTip(tr("用户参数错误"), false);
            return;
        }
        auto data = jsonObj["data"].toString();
        showTip(tr("用户注册成功"), true);
        qDebug() << "data is: " << data;
        qDebug() << "json obj is: " << jsonObj;
        ChangeTipPage();
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


void Register::on_confirm_button_clicked()
{

    bool valid = checkUserValid();
    if(!valid){
        return;
    }
    valid = checkEmailValid();
    if(!valid){
        return;
    }
    valid = checkPassValid();
    if(!valid){
        return;
    }
    valid = checkVarifyValid();
    if(!valid){
        return;
    }

    QJsonObject jsonObj;
    jsonObj["user"] = ui->user_edit->text();
    jsonObj["email"] = ui->email_edit->text();
    jsonObj["passwd"] =  toMD5(ui->pass_edit->text());
    jsonObj["varifycode"] = ui->verify_edit->text();
    jsonObj["confirm"] = toMD5(ui->confirm_edit->text());
    HttpManager::getInstance()->PostHttpReq(QUrl(gate_url_prefix+"/user_register"),
                                            jsonObj, ReqId::ID_REG_UESR, Modules::REGISTERMOD);
}


void Register::AddTipErr(TipErr te, QString tips)
{
    _tip_errs[te] = tips;
    showTip(tips, false);
}

void Register::DelTipErr(TipErr te)
{
    _tip_errs.remove(te);
    if(_tip_errs.empty()){
        ui->error_tips->clear();
        return;
    }
    showTip(_tip_errs.first(), false);
}

bool Register::checkUserValid()
{
    if(ui->user_edit->text() == ""){
        AddTipErr(TipErr::TIP_USER_ERR, tr("用户名不能为空"));
        return false;
    }
    DelTipErr(TipErr::TIP_USER_ERR);
    return true;
}

bool Register::checkEmailValid()
{
    //验证邮箱的地址正则表达式
    auto email = ui->email_edit->text();
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

bool Register::checkPassValid()
{
    auto pass = ui->pass_edit->text();
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

bool Register::checkVarifyValid()
{
    auto pass = ui->verify_edit->text();
    if(pass.isEmpty()){
        AddTipErr(TipErr::TIP_VARIFY_ERR, tr("验证码不能为空"));
        return false;
    }
    DelTipErr(TipErr::TIP_VARIFY_ERR);
    return true;
}

void Register::ChangeTipPage()
{
    _countdown_timer->stop();
    ui->stackedWidget->setCurrentWidget(ui->page_2);
    // 启动定时器，设置间隔为1000毫秒（1秒）
    _countdown_timer->start(1000);
}

void Register::on_return_to_login_btn_clicked()
{
    _countdown_timer->stop();
    emit sigSwitchLogin();
}


void Register::on_cancel_button_clicked()
{
    _countdown_timer->stop();
    emit sigSwitchLogin();
}

