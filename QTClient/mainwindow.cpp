#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "tcpmanager.h"
#include <QMessageBox>

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
    connect(TcpManager::getInstance().get(), &TcpManager::sig_switch_chat_dialog, this, &MainWindow::SlotSwitchChat);
    connect(TcpManager::getInstance().get(), &TcpManager::sig_offline, this, &MainWindow::SlotOffline);
    connect(TcpManager::getInstance().get(), &TcpManager::sig_connection_lost, this, &MainWindow::SlotConnectionLost);
    // test
    //emit TcpManager::getInstance()->sig_switch_chat_dialog();
}

void MainWindow::SlotSwitchReg() {
    _registerDialog = new Register(this);
    _registerDialog->setWindowFlags(Qt::CustomizeWindowHint | Qt::FramelessWindowHint);
    _registerDialog->hide();
    setCentralWidget(_registerDialog);
    connect(_registerDialog, &Register::sigSwitchLogin, this, &MainWindow::SlotSwitchLogin);
    _loginDialog->hide();
    _registerDialog->show();
    _ui_status = REGISTER_UI;

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
    _ui_status = LOGIN_UI;
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
    connect(_loginDialog, &Login::switchToRegister, this, &MainWindow::SlotSwitchReg);
    _ui_status = LOGIN_UI;
}

void MainWindow::SlotSwitchReset()
{

    _reset_dig = new ResetDialog(this);
    _reset_dig->setWindowFlags(Qt::CustomizeWindowHint | Qt::FramelessWindowHint);
    _reset_dig->hide();
    setCentralWidget(_reset_dig);
    connect(_reset_dig, &ResetDialog::sigSwitchLogin, this, &MainWindow::SlotSwitchLogin2);
    _loginDialog->hide();
    _reset_dig->show();
    _ui_status = RESET_UI;
}

void MainWindow::SlotSwitchChat()
{
    _chat_dlg = new ChatDialog();
    _chat_dlg->setWindowFlags(Qt::CustomizeWindowHint | Qt::FramelessWindowHint);
    setCentralWidget(_chat_dlg);
    _chat_dlg->show();
    _loginDialog->hide();
    this->setMinimumSize(QSize(1050, 900));
    this->setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
    _ui_status = CHAT_UI;
}

void MainWindow::SlotOffline()
{

    // 使用静态方法直接弹出一个信息框
    QMessageBox::information(this, "下线提示", "同账号异地登录，该终端下线！");
    TcpManager::getInstance()->CloseConnection();
    offlineLogin();
}

void MainWindow::SlotConnectionLost()
{
    QMessageBox::information(this, "连接断开", "与服务器连接已断开，请重新登录！");
    TcpManager::getInstance()->CloseConnection();
    offlineLogin();
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

void MainWindow::offlineLogin()
{
    if(_ui_status == LOGIN_UI){
        return;
    }
    //创建一个CentralWidget, 并将其设置为MainWindow的中心部件
    _loginDialog = new Login(this);
    _loginDialog->setWindowFlags(Qt::CustomizeWindowHint|Qt::FramelessWindowHint);
    setCentralWidget(_loginDialog);

    _chat_dlg->hide();
    this->setMaximumSize(300,500);
    this->setMinimumSize(300,500);
    this->resize(300,500);
    _loginDialog->show();
    //连接登录界面注册信号
    connect(_loginDialog, &Login::switchToRegister, this, &MainWindow::SlotSwitchReg);
    //连接登录界面忘记密码信号
    connect(_loginDialog, &Login::switchReset, this, &MainWindow::SlotSwitchReset);
    _ui_status = LOGIN_UI;

}
