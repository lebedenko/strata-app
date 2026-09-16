#include "tray/tray_manager.hpp"

#include <QAction>
#include <QIcon>

namespace strata::tray {

TrayManager::TrayManager(dbus::StrataDBusClient *client, config::ConfigManager *configMgr,
                         QObject *parent)
    : QObject(parent)
    , client_(client)
    , configMgr_(configMgr) {
    trayIcon_ = new QSystemTrayIcon(this);
    trayIcon_->setIcon(QIcon(":/qt/qml/Strata/desktop/strata.svg"));

    createMenu();

    connect(trayIcon_, &QSystemTrayIcon::activated, this, &TrayManager::onActivated);

    if (client_) {
        connect(client_, &dbus::StrataDBusClient::statusChanged, this, &TrayManager::updateTooltip);
        connect(client_, &dbus::StrataDBusClient::activeLayerChanged, this,
                &TrayManager::updateTooltip);
    }

    updateTooltip();

    if (configMgr_ && configMgr_->showTrayIcon()) {
        trayIcon_->show();
    }
}

void TrayManager::createMenu() {
    menu_ = new QMenu();

    auto *toggleAction = menu_->addAction(tr("Open Visualizer"));
    connect(toggleAction, &QAction::triggered, this, &TrayManager::toggleWindowRequested);

    menu_->addSeparator();

    auto *refreshAction = menu_->addAction(tr("Refresh Keymap"));
    connect(refreshAction, &QAction::triggered, this, [this]() {
        if (client_)
            client_->refreshKeymap();
    });

    auto *clearAction = menu_->addAction(tr("Clear Cache"));
    connect(clearAction, &QAction::triggered, this, [this]() {
        if (client_)
            client_->clearCache();
    });

    menu_->addSeparator();

    auto *quitAction = menu_->addAction(tr("Quit"));
    connect(quitAction, &QAction::triggered, this, &TrayManager::quitRequested);

    trayIcon_->setContextMenu(menu_);
}

void TrayManager::show() {
    if (trayIcon_)
        trayIcon_->show();
}

void TrayManager::hide() {
    if (trayIcon_)
        trayIcon_->hide();
}

void TrayManager::onActivated(QSystemTrayIcon::ActivationReason reason) {
    if (reason == QSystemTrayIcon::Trigger || reason == QSystemTrayIcon::DoubleClick) {
        emit toggleWindowRequested();
    }
}

void TrayManager::updateTooltip() {
    if (!trayIcon_ || !client_)
        return;

    if (client_->isConnected()) {
        trayIcon_->setToolTip(tr("%1\nActive Layer: %2 (Index: %3)")
                                  .arg(client_->deviceName())
                                  .arg(client_->activeLayerName())
                                  .arg(client_->activeLayerIndex()));
    } else {
        trayIcon_->setToolTip(tr("Strata: Keyboard Disconnected"));
    }
}

} // namespace strata::tray
