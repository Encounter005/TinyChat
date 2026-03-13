#include "onlyofficeviewerdialog.h"

#include <QDesktopServices>
#include <QHBoxLayout>
#include <QPushButton>
#include <QVBoxLayout>
#include <QtWebEngineWidgets/QWebEngineView>

OnlyOfficeViewerDialog::OnlyOfficeViewerDialog(QWidget *parent)
    : QDialog(parent)
    , m_view(new QWebEngineView(this))
{
    setWindowTitle(tr("OnlyOffice 在线编辑"));
    resize(1200, 800);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(8, 8, 8, 8);
    layout->setSpacing(6);

    auto *toolbar = new QHBoxLayout();
    toolbar->setContentsMargins(0, 0, 0, 0);

    auto *open_external_btn = new QPushButton(tr("浏览器打开"), this);
    auto *reload_btn = new QPushButton(tr("刷新"), this);

    connect(open_external_btn, &QPushButton::clicked, this, [this]() {
        const QUrl current = m_view->url();
        if (current.isValid()) {
            QDesktopServices::openUrl(current);
        }
    });
    connect(reload_btn, &QPushButton::clicked, m_view, &QWebEngineView::reload);

    toolbar->addWidget(open_external_btn);
    toolbar->addWidget(reload_btn);
    toolbar->addStretch(1);

    layout->addLayout(toolbar);
    layout->addWidget(m_view, 1);
}

void OnlyOfficeViewerDialog::loadUrl(const QUrl &url)
{
    m_view->setUrl(url);
}
