#pragma once

#include <QDBusInterface>
#include <QObject>

#include "config/config_manager.hpp"
#include "dbus/strata_dbus_client.hpp"

namespace strata::notification {

class Notifier : public QObject {
    Q_OBJECT

public:
    Notifier(dbus::StrataDBusClient *client, config::ConfigManager *configMgr,
             QObject *parent = nullptr);

public slots:
    void notify(const QString &summary, const QString &body, int timeoutMs = -1);

private slots:
    void onLayerChanged(int index, const QString &name, uint mask);
    void onDeviceConnected(const QString &name, const QString &node, const QString &buildId);
    void onDeviceDisconnected(const QString &node);

private:
    dbus::StrataDBusClient *client_{nullptr};
    config::ConfigManager *configMgr_{nullptr};
    uint32_t lastNotificationId_{0};
};

} // namespace strata::notification
