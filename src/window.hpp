#pragma once

#include <QQuickWindow>

#include "tray/tray_manager.hpp"

namespace strata {
inline void configureWindow(QQuickWindow *window, tray::TrayManager *trayManager,
                            bool startMinimized) {
    if (startMinimized && trayManager->canHideToTray()) {
        window->hide();
    }

    QObject::connect(trayManager, &tray::TrayManager::canHideToTrayChanged, window,
                     [window, trayManager]() {
                         if (!trayManager->canHideToTray() && !window->isVisible())
                             window->showNormal();
                     });

    QObject::connect(trayManager, &tray::TrayManager::toggleWindowRequested, window, [window]() {
        if (window->isVisible()) {
            window->hide();
        } else {
            window->showNormal();
            window->raise();
            window->requestActivate();
        }
    });
}
} // namespace strata
