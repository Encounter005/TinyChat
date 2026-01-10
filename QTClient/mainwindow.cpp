#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    _loginDialog = new Login(this);
    setCentralWidget(_loginDialog);
    //取消边框
    _loginDialog->setWindowFlags(Qt::CustomizeWindowHint | Qt::FramelessWindowHint);
    // _loginDialog->show();
    connect(_loginDialog, &Login::switchToRegister, this, &MainWindow::SlotSwitchReg);
    connect(_loginDialog, &Login::switchReset, this, &MainWindow::SlotSwitchReset);

}

void MainWindow::SlotSwitchReg() {
    _registerDialog = new Register(this);
    _registerDialog->setWindowFlags(Qt::CustomizeWindowHint | Qt::FramelessWindowHint);
    _registerDialog->hide();
    setCentralWidget(_registerDialog);
    connect(_registerDialog, &Register::sigSwitchLogin, this, &MainWindow::SlotSwitchLogin);
    _loginDialog->hide();
    _registerDialog->show();

}

void MainWindow::SlotSwitchLogin()
{
    // _registerDialog->hide();
    // _loginDialog->show();
    //创建一个CentralWidget, 并将其设置为MainWindow的中心部件
    _loginDialog = new Login(this);
    _loginDialog->setWindowFlags(Qt::CustomizeWindowHint|Qt::FramelessWindowHint);
    setCentralWidget(_loginDialog);
    _registerDialog->hide();
    _loginDialog->show();
    //连接登录界面注册信号
    connect(_loginDialog, &Login::switchToRegister, this, &MainWindow::SlotSwitchReg);
    //连接登录界面忘记密码信号
    connect(_loginDialog, &Login::switchReset, this, &MainWindow::SlotSwitchReset);
}

void MainWindow::SlotSwitchLogin2()
{
    //创建一个CentralWidget, 并将其设置为MainWindow的中心部件
    _loginDialog = new Login(this);
    _loginDialog->setWindowFlags(Qt::CustomizeWindowHint|Qt::FramelessWindowHint);
    setCentralWidget(_loginDialog);
    _reset_dig->hide();
    _loginDialog->show();
    //连接登录界面忘记密码信号
    connect(_loginDialog, &Login::switchReset, this, &MainWindow::SlotSwitchReset);
    //连接登录界面注册信号
    connect(_loginDialog, &Login::switchToRegister, this, &MainWindow::SlotSwitchReg);}

void MainWindow::SlotSwitchReset()
{

    _reset_dig = new ResetDialog(this);
    _reset_dig->setWindowFlags(Qt::CustomizeWindowHint | Qt::FramelessWindowHint);
    _reset_dig->hide();
    setCentralWidget(_reset_dig);
    connect(_reset_dig, &ResetDialog::sigSwitchLogin, this, &MainWindow::SlotSwitchLogin2);
    _loginDialog->hide();
    _reset_dig->show();
}




MainWindow::~MainWindow()
{
    // if(_loginDialog) {
    //     delete _loginDialog;
    //     _loginDialog = nullptr;
    // }


    // if(_registerDialog) {
    //     delete _registerDialog;
    //     _registerDialog = nullptr;
    // }
    delete ui;
}
