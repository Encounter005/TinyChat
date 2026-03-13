#ifndef ONLYOFFICEVIEWERDIALOG_H
#define ONLYOFFICEVIEWERDIALOG_H

#include <QDialog>
#include <QUrl>

class QWebEngineView;

class OnlyOfficeViewerDialog : public QDialog
{
    Q_OBJECT
public:
    explicit OnlyOfficeViewerDialog(QWidget *parent = nullptr);
    void loadUrl(const QUrl &url);

private:
    QWebEngineView *m_view;
};

#endif // ONLYOFFICEVIEWERDIALOG_H
