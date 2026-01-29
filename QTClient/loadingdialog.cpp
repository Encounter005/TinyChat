#include "loadingdialog.h"
#include "ui_loadingdialog.h"
#include <QMovie>

LoadingDialog::LoadingDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::LoadingDialog)
{
    ui->setupUi(this);
    this->setWindowFlags(Qt::CustomizeWindowHint | Qt::FramelessWindowHint);
    // 设置背景透明
    this->setAttribute(Qt::WA_TranslucentBackground);
    // 获取屏幕尺寸
    this->setFixedSize(parent->size()); // 设置对话框尺寸

    QMovie* movie = new QMovie("./img/loading.gif");
    ui->loading_label->setMovie(movie);
    movie->start();
}

LoadingDialog::~LoadingDialog()
{
    delete ui;
}
