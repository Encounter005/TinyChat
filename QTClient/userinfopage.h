#ifndef USERINFOPAGE_H
#define USERINFOPAGE_H

#include <QWidget>
#include <QLineEdit>

namespace Ui {
class UserInfoPage;
}

class UserInfoPage : public QWidget
{
    Q_OBJECT

public:
    explicit UserInfoPage(QWidget *parent = nullptr);
    ~UserInfoPage();
    bool eventFilter(QObject *watched, QEvent *event) override;

private slots:
    void on_up_btn_clicked();

private:
    void animateInputHeight(QLineEdit *edit, int target_height);
    void playPageIntroAnimation();
    Ui::UserInfoPage *ui;
};

#endif // USERINFOPAGE_H
