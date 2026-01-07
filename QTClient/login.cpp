#include "login.h"
#include "ui_login.h"
#include <QPushButton>
#include <QLineEdit>

Login::Login(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::Login)
{
    ui->setupUi(this);
    ui->password_edit->setEchoMode(QLineEdit::Password);
    connect(ui->register_button, &QPushButton::clicked, this, &Login::switchToRegister);
}

Login::~Login()
{
    delete ui;
}
