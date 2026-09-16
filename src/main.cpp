#include <QApplication>
#include <QIcon>
#include <QLoggingCategory>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickWindow>
#include <iostream>

#include "config/config_manager.hpp"
#include "dbus/strata_dbus_client.hpp"
#include "model/keymap_model.hpp"
#include "notification/notifier.hpp"
#include "tray/tray_manager.hpp"

using namespace strata;

int main(int argc, char *argv[]) {
    // Suppress internal Qt Wayland warning regarding missing X11 systemTrayWindowChanged signal
    QLoggingCategory::setFilterRules("qt.core.qobject.connect.warning=false\n");

    QApplication app(argc, argv);
    app.setApplicationName("Strata");
    app.setApplicationDisplayName("Strata");
    app.setOrganizationName("Strata");
    app.setWindowIcon(QIcon(":/qt/qml/Strata/desktop/strata.svg"));

    bool startMinimized = false;
    for (int i = 1; i < argc; ++i) {
        std::string_view arg = argv[i];
        if (arg == "--minimized") {
            startMinimized = true;
        } else if (arg == "-v" || arg == "--version") {
            std::cout << "Strata 1.0.0 (Qt " << QT_VERSION_STR << ")\n";
            return 0;
        } else if (arg == "-h" || arg == "--help") {
            std::cout << "Usage: strata [options]\n\n"
                      << "Options:\n"
                      << "  --minimized    Start minimized directly to the system tray\n"
                      << "  -v, --version  Display version information\n"
                      << "  -h, --help     Show this help message\n";
            return 0;
        }
    }

    config::ConfigManager configMgr;
    dbus::StrataDBusClient dbusClient;
    model::KeymapModel keymapModel(&dbusClient);
    notification::Notifier notifier(&dbusClient, &configMgr);
    tray::TrayManager trayManager(&dbusClient, &configMgr);

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("configManager", &configMgr);
    engine.rootContext()->setContextProperty("strataDBusClient", &dbusClient);
    engine.rootContext()->setContextProperty("strataKeymapModel", &keymapModel);

    QObject::connect(
        &engine, &QQmlApplicationEngine::objectCreationFailed, &app,
        []() { QCoreApplication::exit(-1); }, Qt::QueuedConnection);

    engine.loadFromModule("Strata", "Main");

    auto rootObjects = engine.rootObjects();
    if (!rootObjects.isEmpty()) {
        auto *window = qobject_cast<QQuickWindow *>(rootObjects.first());
        if (window) {
            if (startMinimized || configMgr.startMinimized()) {
                window->hide();
            }

            QObject::connect(&trayManager, &tray::TrayManager::toggleWindowRequested, [window]() {
                if (window->isVisible()) {
                    window->hide();
                } else {
                    window->show();
                    window->raise();
                    window->requestActivate();
                }
            });
        }
    }

    QObject::connect(&trayManager, &tray::TrayManager::quitRequested, &app,
                     &QCoreApplication::quit);

    return app.exec();
}
