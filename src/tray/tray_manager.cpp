#include "tray/tray_manager.hpp"

#include <QAction>
#include <QCursor>
#include <QIcon>
#include <QLoggingCategory>

#include "tray/dbus_menu.hpp"

namespace strata::tray {

Q_LOGGING_CATEGORY(lcTray, "strata.tray")

TrayManager::TrayManager(dbus::StrataDBusClient *client, config::ConfigManager *configMgr,
                         QObject *parent)
    : QObject(parent)
    , client_(client)
    , configMgr_(configMgr) {
    createMenu();

    // Direct StatusNotifierItem for Wayland / SNI-compliant panels (Holonight, Waybar, Plasma)
    sni_ = new StatusNotifierItem(this);
    connect(sni_, &StatusNotifierItem::activateRequested, this,
            [this](int /*x*/, int /*y*/) { emit toggleWindowRequested(); });
    connect(sni_, &StatusNotifierItem::contextMenuRequested, this,
            &TrayManager::onContextMenuRequested);

    if (sni_->menuService()) {
        connect(sni_->menuService(), &DBusMenuService::openVisualizerRequested, this,
                &TrayManager::toggleWindowRequested);
        connect(sni_->menuService(), &DBusMenuService::refreshKeymapRequested, this, [this]() {
            if (client_)
                client_->refreshKeymap();
        });
        connect(sni_->menuService(), &DBusMenuService::clearCacheRequested, this, [this]() {
            if (client_)
                client_->clearCache();
        });
        connect(sni_->menuService(), &DBusMenuService::quitRequested, this,
                &TrayManager::quitRequested);
    }

    // Fallback to QSystemTrayIcon if platform theme provides a native tray (e.g. X11 / XEmbed)
    if (QSystemTrayIcon::isSystemTrayAvailable()) {
        qCInfo(lcTray) << "QSystemTrayIcon is available on this platform";
        trayIcon_ = new QSystemTrayIcon(this);
        trayIcon_->setIcon(QIcon(QStringLiteral(":/qt/qml/Strata/desktop/strata.svg")));
        trayIcon_->setContextMenu(menu_);
        connect(trayIcon_, &QSystemTrayIcon::activated, this, &TrayManager::onActivated);
    } else {
        qCInfo(lcTray) << "QSystemTrayIcon is not available; relying on direct StatusNotifierItem";
    }

    if (client_) {
        connect(client_, &dbus::StrataDBusClient::statusChanged, this, &TrayManager::updateTooltip);
        connect(client_, &dbus::StrataDBusClient::activeLayerChanged, this,
                &TrayManager::updateTooltip);
    }

    updateTooltip();

    if (configMgr_ && configMgr_->showTrayIcon()) {
        show();
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
}

void TrayManager::show() {
    if (sni_) {
        sni_->registerItem();
    }
    if (trayIcon_) {
        trayIcon_->show();
    }
}

void TrayManager::hide() {
    if (sni_) {
        sni_->unregisterItem();
    }
    if (trayIcon_) {
        trayIcon_->hide();
    }
}

void TrayManager::onActivated(QSystemTrayIcon::ActivationReason reason) {
    if (reason == QSystemTrayIcon::Trigger || reason == QSystemTrayIcon::DoubleClick) {
        emit toggleWindowRequested();
    }
}

void TrayManager::onContextMenuRequested(int x, int y) {
    if (!menu_)
        return;

    QPoint pos(x, y);
    if (pos.isNull()) {
        pos = QCursor::pos();
    }
    menu_->popup(pos);
}

void TrayManager::updateTooltip() {
    QString title;
    QString desc;

    if (client_ && client_->isConnected()) {
        title = tr("Strata: %1").arg(client_->deviceName());
        desc = tr("Active Layer: %1 (Index: %2)")
                   .arg(client_->activeLayerName())
                   .arg(client_->activeLayerIndex());
    } else {
        title = tr("Strata");
        desc = tr("Keyboard Disconnected");
    }

    if (sni_) {
        sni_->setToolTip(title, desc);
    }

    if (trayIcon_) {
        trayIcon_->setToolTip(QStringLiteral("%1\n%2").arg(title, desc));
    }
}

} // namespace strata::tray
