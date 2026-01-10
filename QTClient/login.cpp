#include "login.h"
#include "ui_login.h"
#include <QPushButton>
#include <QLineEdit>
#include <QLabel>


Login::Login(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::Login)
{
    ui->setupUi(this);
    connect(ui->register_button, &QPushButton::clicked, this, &Login::switchToRegister);
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

void Login::slot_forget_pwd()
{
    qDebug()<<"slot forget pwd";
    emit switchReset();

}




