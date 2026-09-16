#pragma once

#include <QMenu>
#include <QObject>
#include <QSystemTrayIcon>

#include "config/config_manager.hpp"
#include "dbus/strata_dbus_client.hpp"
#include "tray/status_notifier_item.hpp"

namespace strata::tray {

class TrayManager : public QObject {
    Q_OBJECT

public:
    TrayManager(dbus::StrataDBusClient *client, config::ConfigManager *configMgr,
                QObject *parent = nullptr);
    ~TrayManager() override = default;

    void show();
    void hide();

signals:
    void toggleWindowRequested();
    void quitRequested();

private slots:
    void onActivated(QSystemTrayIcon::ActivationReason reason);
    void onContextMenuRequested(int x, int y);
    void updateTooltip();

private:
    void createMenu();

    dbus::StrataDBusClient *client_{nullptr};
    config::ConfigManager *configMgr_{nullptr};
    StatusNotifierItem *sni_{nullptr};
    QSystemTrayIcon *trayIcon_{nullptr};
    QMenu *menu_{nullptr};
};

} // namespace strata::tray
