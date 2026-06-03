#pragma once

#include <QMainWindow>

class AppContext;
class DeviceListPage;
class HomePage;
class QListWidget;
class QStackedWidget;
class SettingsPage;
class TransferPage;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(AppContext *context, QWidget *parent = nullptr);

private:
    AppContext *m_context = nullptr;
    HomePage *m_homePage = nullptr;
    DeviceListPage *m_deviceListPage = nullptr;
    TransferPage *m_transferPage = nullptr;
    SettingsPage *m_settingsPage = nullptr;
    QListWidget *m_navigation = nullptr;
    QStackedWidget *m_pages = nullptr;
};
