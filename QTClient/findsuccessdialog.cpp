#include "findsuccessdialog.h"
#include "ui_findsuccessdialog.h"
#include "applyfriend.h"
#include <QDir>
#include <QPixmap>

FindSuccessDialog::FindSuccessDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::FindSuccessDialog)
{
    ui->setupUi(this);
    setWindowTitle("添加");
    // 隐藏对话框标题栏
    setWindowFlags(windowFlags() | Qt::FramelessWindowHint);
    this->setObjectName("FindSuccessDialog");

    QString app_path = QCoreApplication::applicationDirPath();
    QString pix_path = QDir::toNativeSeparators(app_path + QDir::separator() + "img" + QDir::separator() + "head_1.jpg");
    qDebug() << "pix path is: " << pix_path;
    QPixmap head_pix(pix_path);
    head_pix = head_pix.scaled(ui->head_label->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
    ui->head_label->setPixmap(head_pix);
    ui->add_friend_btn->SetState("normal", "hover", "pres");
    this->setModal(true);

}

FindSuccessDialog::~FindSuccessDialog()
{
    delete ui;
}

void FindSuccessDialog::SetSearchInfo(std::shared_ptr<SearchInfo> si)
{
    ui->name_label->setText(si->_name);
    _si = si;
}

void FindSuccessDialog::on_head_label_clicked()
{

}


void FindSuccessDialog::on_add_friend_btn_clicked()
{
    this->hide();
    auto applyFriend = new ApplyFriend(_parent);
    applyFriend->SetSearchInfo(_si);
    applyFriend->setModal(true);
    applyFriend->show();
}

