#pragma once

#include "protocol/DeviceInfo.h"

#include <QWidget>

class QListWidget;
class QPushButton;

class DeviceListPage : public QWidget
{
    Q_OBJECT

public:
    explicit DeviceListPage(QWidget *parent = nullptr);

public slots:
    void setDevices(const QList<DeviceInfo> &devices);

signals:
    void sendFilesRequested(DeviceInfo device, QStringList filePaths);

private:
    QListWidget *m_deviceList = nullptr;
    QPushButton *m_sendButton = nullptr;
    QList<DeviceInfo> m_devices;
};
