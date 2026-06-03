#pragma once

#include <QWidget>

class QLabel;

class HomePage : public QWidget
{
    Q_OBJECT

public:
    explicit HomePage(QWidget *parent = nullptr);

    void setDeviceName(const QString &name);
    void setOnlineDeviceCount(int count);

private:
    QLabel *m_deviceNameLabel = nullptr;
    QLabel *m_statusLabel = nullptr;
    QLabel *m_onlineCountLabel = nullptr;
};
