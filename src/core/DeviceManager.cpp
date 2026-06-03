#include "core/DeviceManager.h"

#include <algorithm>
#include <utility>

DeviceManager::DeviceManager(QString localDeviceId, QObject *parent)
    : QObject(parent)
    , m_localDeviceId(std::move(localDeviceId))
{
}

QList<DeviceInfo> DeviceManager::devices() const
{
    QList<DeviceInfo> result = m_devices.values();
    std::sort(result.begin(), result.end(), [](const DeviceInfo &left, const DeviceInfo &right) {
        return left.deviceName.localeAwareCompare(right.deviceName) < 0;
    });
    return result;
}

void DeviceManager::handleAnnounce(const DeviceInfo &device)
{
    if (device.deviceId.isEmpty() || device.deviceId == m_localDeviceId) {
        return;
    }

    // IP 地址可能因为 DHCP 或多网卡变化，设备身份必须始终以 deviceId 为准。
    const bool changed = !m_devices.contains(device.deviceId) || m_devices.value(device.deviceId).deviceName != device.deviceName ||
        m_devices.value(device.deviceId).platform != device.platform ||
        m_devices.value(device.deviceId).ipAddress != device.ipAddress ||
        m_devices.value(device.deviceId).tcpPort != device.tcpPort ||
        m_devices.value(device.deviceId).lastSeen != device.lastSeen;

    m_devices.insert(device.deviceId, device);

    if (changed) {
        emitChanged();
    }
}

void DeviceManager::handleBye(const QString &deviceId)
{
    if (m_devices.remove(deviceId) > 0) {
        emitChanged();
    }
}

void DeviceManager::removeExpired(const QDateTime &now, int timeoutSeconds)
{
    bool changed = false;

    const QList<QString> ids = m_devices.keys();
    for (const QString &id : ids) {
        const DeviceInfo device = m_devices.value(id);
        if (device.lastSeen.secsTo(now) > timeoutSeconds) {
            m_devices.remove(id);
            changed = true;
        }
    }

    if (changed) {
        emitChanged();
    }
}

void DeviceManager::emitChanged()
{
    emit devicesChanged(devices());
}
