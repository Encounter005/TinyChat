#ifndef FILEUPLOADWINDOW_H
#define FILEUPLOADWINDOW_H

#include <QMainWindow>
#include <grpcpp/grpcpp.h>
#include <grpcpp/client_context.h>
#include <grpcpp/support/status.h>
#include <QSettings>
#include <QFile>
#include <QFileInfo>
#include "file.grpc.pb.h"


namespace Ui {
class FileUploadWindow;
}

class FileUploadWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit FileUploadWindow(QWidget *parent = nullptr);
    ~FileUploadWindow();

private slots:
    void on_select_button_clicked();
    void on_upload_button_clicked();
    void on_cancel_button_clicked();

private:
    std::string calculateFileMD5(const std::string& filename);
    void showSuccess(const QString& message);
    void showError(const QString& message);
    void uploadFile();
    void updateProgress(int value);
    void postError(const QString& msg);
    void postSuccess(const QString& msg);


private:
    Ui::FileUploadWindow *ui;
    QString _file_name;
    QString _server_host;
    QString _server_port;
    std::unique_ptr<FileService::FileTransport::Stub> _stub;
    std::atomic<bool> _is_uploading;
    std::shared_ptr<grpc::ClientContext> _context;
};

#endif // FILEUPLOADWINDOW_H
