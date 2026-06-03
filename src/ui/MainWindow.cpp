#include "ui/MainWindow.h"

#include "ui/DeviceListPage.h"
#include "ui/HomePage.h"
#include "ui/SettingsPage.h"
#include "ui/TransferPage.h"

#include <QHBoxLayout>
#include <QListWidget>
#include <QStackedWidget>
#include <QWidget>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle(QStringLiteral("AnDrop"));

    auto *central = new QWidget(this);
    auto *layout = new QHBoxLayout(central);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    m_navigation = new QListWidget(central);
    m_navigation->setFixedWidth(160);
    m_navigation->addItem(QStringLiteral("首页"));
    m_navigation->addItem(QStringLiteral("设备"));
    m_navigation->addItem(QStringLiteral("传输"));
    m_navigation->addItem(QStringLiteral("设置"));

    m_pages = new QStackedWidget(central);
    m_pages->addWidget(new HomePage(m_pages));
    m_pages->addWidget(new DeviceListPage(m_pages));
    m_pages->addWidget(new TransferPage(m_pages));
    m_pages->addWidget(new SettingsPage(m_pages));

    layout->addWidget(m_navigation);
    layout->addWidget(m_pages, 1);
    setCentralWidget(central);

    connect(m_navigation, &QListWidget::currentRowChanged, m_pages, &QStackedWidget::setCurrentIndex);
    m_navigation->setCurrentRow(0);
}
