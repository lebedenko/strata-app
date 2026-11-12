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

    if (client_) {
        connect(client_, &dbus::StrataDBusClient::statusChanged, this, &TrayManager::updateTooltip);
        connect(client_, &dbus::StrataDBusClient::activeLayerChanged, this,
                &TrayManager::updateTooltip);
    }

    if (configMgr_) {
        connect(configMgr_, &config::ConfigManager::configChanged, this, [this]() {
            if (configMgr_->showTrayIcon())
                show();
            else
                hide();
        });
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

TrayManager::~TrayManager() {
    delete menu_;
}

bool TrayManager::canHideToTray() const {
    return configMgr_ && configMgr_->showTrayIcon() && sni_ && sni_->isRegistered();
}

void TrayManager::show() {
    if (sni_)
        sni_->registerItem();
    emit canHideToTrayChanged();
}

void TrayManager::hide() {
    if (sni_)
        sni_->unregisterItem();
    emit canHideToTrayChanged();
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
}

} // namespace strata::tray
