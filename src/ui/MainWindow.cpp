#include "ui/MainWindow.h"

#include "app/AppContext.h"
#include "core/DeviceManager.h"
#include "core/TransferManager.h"
#include "ui/DeviceListPage.h"
#include "ui/HomePage.h"
#include "ui/SettingsPage.h"
#include "ui/TransferPage.h"

#include <QHBoxLayout>
#include <QListWidget>
#include <QStackedWidget>
#include <QWidget>

MainWindow::MainWindow(AppContext *context, QWidget *parent)
    : QMainWindow(parent)
    , m_context(context)
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
    m_homePage = new HomePage(m_pages);
    m_deviceListPage = new DeviceListPage(m_pages);
    m_transferPage = new TransferPage(m_pages);
    m_settingsPage = new SettingsPage(m_pages);

    m_pages->addWidget(m_homePage);
    m_pages->addWidget(m_deviceListPage);
    m_pages->addWidget(m_transferPage);
    m_pages->addWidget(m_settingsPage);

    layout->addWidget(m_navigation);
    layout->addWidget(m_pages, 1);
    setCentralWidget(central);

    connect(m_navigation, &QListWidget::currentRowChanged, m_pages, &QStackedWidget::setCurrentIndex);
    m_navigation->setCurrentRow(0);

    if (m_context) {
        m_homePage->setDeviceName(m_context->config().deviceName);
        m_settingsPage->setConfig(m_context->config());

        connect(m_context->deviceManager(), &DeviceManager::devicesChanged,
            m_deviceListPage, &DeviceListPage::setDevices);
        connect(m_context->deviceManager(), &DeviceManager::devicesChanged,
            m_homePage, [this](const QList<DeviceInfo> &devices) {
                m_homePage->setOnlineDeviceCount(devices.size());
            });
        connect(m_context->transferManager(), &TransferManager::tasksChanged,
            m_transferPage, &TransferPage::setTasks);
        connect(m_deviceListPage, &DeviceListPage::sendFilesRequested,
            m_context, &AppContext::sendFilesToDevice);
    }
}
