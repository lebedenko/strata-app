#pragma once

#include <QMenu>
#include <QObject>

#include "config/config_manager.hpp"
#include "dbus/strata_dbus_client.hpp"
#include "tray/status_notifier_item.hpp"

namespace strata::tray {

class TrayManager : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool canHideToTray READ canHideToTray NOTIFY canHideToTrayChanged)

public:
    TrayManager(dbus::StrataDBusClient *client, config::ConfigManager *configMgr,
                QObject *parent = nullptr);
    ~TrayManager() override;
    [[nodiscard]] bool canHideToTray() const;

    void show();
    void hide();

signals:
    void canHideToTrayChanged();
    void toggleWindowRequested();
    void quitRequested();

private slots:
    void onContextMenuRequested(int x, int y);
    void updateTooltip();

private:
    void createMenu();

    dbus::StrataDBusClient *client_{nullptr};
    config::ConfigManager *configMgr_{nullptr};
    StatusNotifierItem *sni_{nullptr};
    QMenu *menu_{nullptr};
};

} // namespace strata::tray
