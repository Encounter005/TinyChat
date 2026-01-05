#include "login.h"
#include "ui_login.h"
#include <QPushButton>

Login::Login(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::Login)
{
    ui->setupUi(this);
    connect(ui->register_button, &QPushButton::clicked, this, &Login::switchToRegister);
}

Login::~Login()
{
    delete ui;
}
