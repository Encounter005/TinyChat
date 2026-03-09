#include "fileuploadwindow.h"
#include "qdebug.h"
#include "ui_fileuploadwindow.h"
#include "file.grpc.pb.h"
#include <QFileDialog>
#include <grpcpp/grpcpp.h>
#include <grpcpp/client_context.h>
#include <grpcpp/support/status.h>
#include <QSettings>
#include <QFile>
#include <QThread>
#include <QFileInfo>
#include <QCryptographicHash>
#include <vector>

FileUploadWindow::FileUploadWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::FileUploadWindow), _is_uploading(false)
{
    ui->setupUi(this);
    this->setWindowTitle("文件传输");
    ui->progressBar->hide();

    // 读取配置文件
    QString app_path = QCoreApplication::applicationDirPath();
    QString config_path = QDir::toNativeSeparators(app_path + QDir::separator() + "config.ini");

    QSettings settings(config_path, QSettings::IniFormat);
    _server_host = settings.value("FileServer/host").toString();
    _server_port = settings.value("FileServer/port").toString();
    QString url = _server_host + ":" + _server_port;
}

FileUploadWindow::~FileUploadWindow()
{
    delete ui;
}

void FileUploadWindow::on_select_button_clicked()
{
    // 打开文件对话框
    QString fileName = QFileDialog::getOpenFileName(nullptr,
                                                    "Open File",
                                                    "",
                                                    "All Files (*);;Text Files (*.txt);;Image Files (*.png *.jpg);;PDF Files (*.pdf)");

    if (!fileName.isEmpty()) {
        qDebug() << "Selected file:" << fileName;
        _file_name = fileName;

        QFileInfo fileInfo(_file_name); // 创建 QFileInfo 对象
        ui->filename_label->setText(fileInfo.fileName());
        ui->progressBar->setRange(0,100);
        ui->progressBar->setValue(0);
    } else {
        qDebug() << "No file selected.";
    }
}


void FileUploadWindow::on_upload_button_clicked()
{
    if (_is_uploading) {
        return;
    }

    if (_file_name.isEmpty()) {
        postError("请先选择文件");
        return;
    }

    _is_uploading = true;
    ui->progressBar->show();
    ui->tip_label->clear();

    // 使用工作线程执行上传
    auto* t = QThread::create([this]() {
        uploadFile();
    });
    connect(t, &QThread::finished, t, &QObject::deleteLater);
    t->start();
}



void FileUploadWindow::on_cancel_button_clicked()
{
    _is_uploading = false;

    auto ctx = _context;  // 拷贝 shared_ptr
    if (ctx) {
        ctx->TryCancel();
    }
    this->hide();
}

std::string FileUploadWindow::calculateFileMD5(const std::string &filename)
{
    // 将 std::string 转换为 QString 以便使用 QFile
    QString qFilename = QString::fromStdString(filename);
    QFile file(qFilename);

    if (!file.open(QIODevice::ReadOnly)) {
        qDebug() << "Cannot open file for hashing:" << qFilename;
        return "";
    }

    // 创建 MD5 哈希对象
    QCryptographicHash hash(QCryptographicHash::Md5);

    // 分块读取文件并更新哈希值
    // 即使是 GB 级别的文件，这种方式也只占用很小的固定内存（这里是 1MB）
    const qint64 bufferSize = 1024 * 1024;
    while (!file.atEnd()) {
        QByteArray buffer = file.read(bufferSize);
        hash.addData(buffer);
    }

    file.close();

    // 获取结果并转换为十六进制字符串
    // result() 返回 QByteArray，toHex() 转换为十六进制，最后转为 std::string
    return hash.result().toHex().toStdString();
}

void FileUploadWindow::showSuccess(const QString &message)
{
    ui->tip_label->setText(message);
    ui->tip_label->setStyleSheet("color: #6ed66f; font-size: 14px; font-family: \"Maple Mono NF\";");
    ui->progressBar->hide();
    _is_uploading = false;
}

void FileUploadWindow::showError(const QString &message)
{
    ui->tip_label->setText(message);
    ui->tip_label->setStyleSheet("color: #e65046; font-size: 14px; font-family: \"Maple Mono NF\";");
    ui->progressBar->hide();
    _is_uploading = false;
}

void FileUploadWindow::uploadFile()
{
    auto channel = grpc::CreateChannel(
        (_server_host + ":" + _server_port).toStdString(),
        grpc::InsecureChannelCredentials());

    auto stub = FileService::FileTransport::NewStub(channel);

    QFile file(_file_name);
    if (!file.open(QIODevice::ReadOnly)) {
        postError("无法打开文件");
        return;
    }

    QFileInfo fileInfo(_file_name);
    qint64 file_size = fileInfo.size();

    if (file_size == 0) {
        postError("文件为空");
        return;
    }

    _context = std::make_shared<grpc::ClientContext>();
    FileService::UploadResponse response;
    auto writer = stub->UploadFile(_context.get(), &response);

    if (!writer) {
        postError("创建上传失败");
        return;
    }

    // 发送元数据
    // 创建 UploadRequest 对象
    FileService::UploadRequest request;

    // 设置元数据
    FileService::FileMeta* meta = request.mutable_meta();
    meta->set_file_name(fileInfo.fileName().toStdString());
    meta->set_total_size(file_size);
    meta->set_file_md5(calculateFileMD5(_file_name.toStdString()));

    // 写入元数据
    if (!writer->Write(request)) {
        postError("发送元数据失败");
        return;
    }
    qDebug() << "开始上传文件:" << fileInfo.fileName();

    // 分块上传
    const qint64 CHUNK_SIZE = 1024 * 1024 * 2; // 2MB
    std::vector<char> buffer(CHUNK_SIZE);
    qint64 total_sent = 0;

    bool write_ok = true;
    while (!file.atEnd() && _is_uploading) {
        qint64 bytes_to_read = qMin<qint64>(CHUNK_SIZE, file.size() - file.pos());
        qint64 bytes_read = file.read(buffer.data(), bytes_to_read);

        if (bytes_read > 0) {
            FileService::UploadRequest request;
            request.set_chunk(buffer.data(), bytes_read);

            if (!writer->Write(request)) {
                postError("发送数据失败");
                write_ok = false;
                break;
            }

            total_sent += bytes_read;
            int progress = static_cast<int>((total_sent * 100) / file_size);
            updateProgress(progress);
        }
    }

    file.close();

    // 发送结束标记
    writer->WritesDone();

    // 接收响应
    grpc::Status status = writer->Finish();
    if(!write_ok) {
        postError("发送数据失败");
        return;
    }

    if (!status.ok()) {
        // 检查网络错误
        if (status.error_code() == grpc::StatusCode::UNAVAILABLE) {
            postError("无法连接到服务器");
        } else {
            postError(QString(status.error_message().c_str()));
        }
        _is_uploading = false;
        return;
    }

    if (response.success()) {
        postSuccess("文件传输成功");
    } else {
        postError("文件传输失败：" + QString::fromStdString(response.message()));
    }

    _is_uploading = false;
}

void FileUploadWindow::updateProgress(int value)
{
    QMetaObject::invokeMethod(this, [this, value]() {
        ui->progressBar->setValue(value);
    }, Qt::QueuedConnection);
}

void FileUploadWindow::postError(const QString &msg)
{
    QMetaObject::invokeMethod(this, [this, msg]() {
        showError(msg);
    }, Qt::QueuedConnection);
}

void FileUploadWindow::postSuccess(const QString &msg)
{
    QMetaObject::invokeMethod(this, [this, msg]() {
        showSuccess(msg);
    }, Qt::QueuedConnection);
}

