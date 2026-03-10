#ifndef CHATPAGE_H
#define CHATPAGE_H

#include "userdata.h"
#include "fileuploadwindow.h"
#include <QWidget>
#include <QMap>
#include <QPaintEvent>
#include <QSet>
#include <functional>

namespace Ui {
class ChatPage;
}

class PictureBubble;
class FileBubble;

class ChatPage : public QWidget
{
    Q_OBJECT

public:
    explicit ChatPage(QWidget *parent = nullptr);
    ~ChatPage();
    virtual void paintEvent(QPaintEvent* event) override;
    void SetUserInfo(std::shared_ptr<UserInfo> user_info);
    void AppendChatMsg(std::shared_ptr<TextChatData> msg);
private:
    static bool IsImagePayload(const QString &content, QString *remote_name = nullptr);
    static bool IsFilePayload(
        const QString &content,
        QString *remote_name = nullptr,
        QString *display_name = nullptr,
        qint64 *file_size = nullptr);
    void UploadImageAsync(
        const QString &local_path,
        const QString &remote_name,
        const std::function<void(int)> &on_progress,
        const std::function<void()> &on_success,
        const std::function<void(const QString &)> &on_error);
    void UploadFileAsync(
        const QString &local_path,
        const QString &remote_name,
        const std::function<void(int)> &on_progress,
        const std::function<void()> &on_success,
        const std::function<void(const QString &)> &on_error);
    void DownloadImageAsync(
        const QString &remote_name,
        const std::function<void(const QPixmap &)> &on_success,
        const std::function<void(const QString &)> &on_error);
    void DownloadFileAsync(
        const QString &remote_name,
        const QString &save_path,
        const std::function<void(int)> &on_progress,
        const std::function<void()> &on_success,
        const std::function<void(const QString &)> &on_error);
    static QString BuildImagePayload(const QString &remote_name);
    static QString BuildFilePayload(
        const QString &remote_name,
        const QString &display_name,
        qint64 file_size);

    Ui::ChatPage *ui;
    std::shared_ptr<UserInfo> _user_info;
    QMap<QString, QWidget*> _bubble_map;
    QMap<QString, QString> _image_msgid_to_remote;
    QMap<QString, FileBubble*> _pending_file_bubbles;
    QMap<QString, QString> _pending_file_remote;
    QMap<QString, QString> _pending_file_name;
    QSet<QString> _selected_file_msgids;
    FileUploadWindow* _fileWindow{nullptr};
signals:
    void sig_append_send_chat_msg(std::shared_ptr<TextChatData>);
private slots:
    void on_recv_btn_clicked();
    void on_send_btn_clicked();
    void slot_switch_to_upload();
    void slot_pick_image();

};

#endif // CHATPAGE_H
