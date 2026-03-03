#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "login.h"
#include "register.h"
#include "resetdialog.h"
#include "chatdialog.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT
    enum UIStatus{
        LOGIN_UI,
        REGISTER_UI,
        RESET_UI,
        CHAT_UI
    };



public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
private:
    void offlineLogin();

private:
    Ui::MainWindow *ui;
    Login *_loginDialog;
    Register *_registerDialog;
    ResetDialog *_reset_dig;
    ChatDialog *_chat_dlg;
    UIStatus _ui_status;

public slots:
    void SlotSwitchReg();
    void SlotSwitchLogin();
    void SlotSwitchLogin2();
    void SlotSwitchReset();
    void SlotSwitchChat();
    void SlotOffline();
    void SlotConnectionLost();
};

#endif // MAINWINDOW_H
