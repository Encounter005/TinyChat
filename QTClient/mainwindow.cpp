#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    _loginDialog = new Login(this);
    setCentralWidget(_loginDialog);
    // _loginDialog->show();
    //取消边框
    _loginDialog->setWindowFlags(Qt::CustomizeWindowHint | Qt::FramelessWindowHint);

    connect(_loginDialog, &Login::switchToRegister, this, &MainWindow::SlotSwitchReg);
    _registerDialog = new Register(this);
    _registerDialog->setWindowFlags(Qt::CustomizeWindowHint | Qt::FramelessWindowHint);
    _registerDialog->hide();

}

void MainWindow::SlotSwitchReg() {
    setCentralWidget(_registerDialog);
    _loginDialog->hide();
    _registerDialog->show();
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
