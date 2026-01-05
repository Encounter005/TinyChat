#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    _loginDialog = new Login();
    setCentralWidget(_loginDialog);
    _loginDialog->show();

    connect(_loginDialog, &Login::switchToRegister, this, &MainWindow::SlotSwitchReg);
    _registerDialog = new Register();

}

void MainWindow::SlotSwitchReg() {
    setCentralWidget(_registerDialog);
    _loginDialog->hide();
    _registerDialog->show();
}


MainWindow::~MainWindow()
{
    if(_loginDialog) {
        delete _loginDialog;
        _loginDialog = nullptr;
    }


    if(_registerDialog) {
        delete _registerDialog;
        _registerDialog = nullptr;
    }
    delete ui;
}
