#include <QApplication>
#include <QDBusAbstractAdaptor>
#include <QDBusConnectionInterface>
#include <QDBusReply>
#include <QDir>
#include <QElapsedTimer>
#include <QProcess>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QThread>
#include <cstdlib>
#include <functional>
#include <iostream>

#include "config/config_manager.hpp"
#include "dbus/strata_dbus_client.hpp"
#include "model/keymap_model.hpp"
#include "tray/dbus_menu.hpp"
#include "tray/tray_manager.hpp"
#include "window.hpp"

static const QString service = "io.github.lebedenko.Strata";
static const QString managerPath = "/io/github/lebedenko/Strata/Manager";
static const QString base = "/io/github/lebedenko/Strata/";
#define CHECK(x)                                                                                   \
    do {                                                                                           \
        if (!(x)) {                                                                                \
            std::cerr << "Failed: " #x << ':' << __LINE__ << '\n';                                 \
            std::abort();                                                                          \
        }                                                                                          \
    } while (false)

class Device : public QObject {
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "io.github.lebedenko.Strata.Device1")
public:
    QString id;
    explicit Device(QString name)
        : id(std::move(name)) {}
public slots:
    QString GetStatus() {
        return QString(
                   R"({"id":"%1","name":"%1","connected":true,"type":"%1","node":"/dev/%1","build_id":"build-%1","layers_count":2,"active_layer":{"index":1,"name":"Nav","mask":2}})")
            .arg(id);
    }
};
class Keymap : public QDBusAbstractAdaptor {
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "io.github.lebedenko.Strata.Keymap1")
public:
    explicit Keymap(Device *d)
        : QDBusAbstractAdaptor(d)
        , device(d) {}
    Device *device;
public slots:
    QString GetLayers() { return R"([{"index":0,"name":"Base"},{"index":1,"name":"Nav"}])"; }
    QString GetKeymap(uint layer, bool) {
        return QString(
                   R"({"bindings":{"%1":[{"pos":0,"behavior":"&kp","primaryLabel":"%2","category":"key"}]}})")
            .arg(layer)
            .arg(device->id);
    }
};
class Manager : public QObject {
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "io.github.lebedenko.Strata.Manager1")
public:
    QList<QDBusObjectPath> devices;
    QDBusObjectPath active{"/"};
public slots:
    QList<QDBusObjectPath> GetDevices() { return devices; }
    QDBusObjectPath GetActiveDevice() { return active; }
    bool SetActiveDevice(const QString &id) {
        active = QDBusObjectPath(base + id);
        emit ActiveDeviceChanged(active, id);
        return true;
    }
    void Configure(int mode) {
        auto old = active;
        devices.clear();
        if (mode) {
            devices << QDBusObjectPath(base + "corne") << QDBusObjectPath(base + "voyager");
            active = devices.at(mode == 2 ? 1 : 0);
            QString id = active.path().section('/', -1);
            if (mode == 2) {
                emit ActiveDeviceChanged(active, id);
                emit DeviceAdded(active, id);
            } else {
                emit DeviceAdded(active, id);
                emit ActiveDeviceChanged(active, id);
            }
        } else {
            active = QDBusObjectPath("/");
            emit DeviceRemoved(old, old.path().section('/', -1));
            emit ActiveDeviceChanged(active, "");
        }
    }
signals:
    void DeviceAdded(const QDBusObjectPath &, const QString &);
    void DeviceRemoved(const QDBusObjectPath &, const QString &);
    void ActiveDeviceChanged(const QDBusObjectPath &, const QString &);
};
class Watcher : public QObject {
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.StatusNotifierWatcher")
public:
    int count = 0;
public slots:
    void RegisterStatusNotifierItem(const QString &) { ++count; }
    int Count() { return count; }
};
static bool until(const std::function<bool()> &predicate) {
    QElapsedTimer timer;
    timer.start();
    do {
        QCoreApplication::processEvents();
        if (predicate())
            return true;
        QThread::msleep(5);
    } while (timer.elapsed() < 5000);
    return false;
}
int main(int argc, char **argv) {
    QApplication app(argc, argv);
    auto bus = QDBusConnection::sessionBus();
    if (app.arguments().contains("--watcher")) {
        Watcher watcher;
        CHECK(bus.registerObject("/StatusNotifierWatcher", &watcher,
                                 QDBusConnection::ExportAllSlots));
        CHECK(bus.registerService("org.kde.StatusNotifierWatcher"));
        return app.exec();
    }
    if (app.arguments().contains("--daemon")) {
        Manager manager;
        Device corne("corne"), voyager("voyager");
        new Keymap(&corne);
        new Keymap(&voyager);
        CHECK(
            bus.registerObject(base + "corne", &corne,
                               QDBusConnection::ExportAllSlots | QDBusConnection::ExportAdaptors));
        CHECK(
            bus.registerObject(base + "voyager", &voyager,
                               QDBusConnection::ExportAllSlots | QDBusConnection::ExportAdaptors));
        CHECK(bus.registerObject(managerPath, &manager, QDBusConnection::ExportAllContents));
        manager.Configure(app.arguments().contains("--voyager") ? 2 : 1);
        auto registration = bus.interface()->registerService(
            service, QDBusConnectionInterface::ReplaceExistingService,
            QDBusConnectionInterface::AllowReplacement);
        CHECK(registration.isValid() &&
              registration.value() == QDBusConnectionInterface::ServiceRegistered);
        return app.exec();
    }
    strata::dbus::StrataDBusClient client;
    strata::model::KeymapModel model(&client);
    CHECK(!client.isConnected());
    QProcess daemon;
    auto start = [&]() {
        daemon.start(app.applicationFilePath(), {"--daemon"});
        CHECK(daemon.waitForStarted());
        CHECK(until([&]() { return bus.interface()->isServiceRegistered(service).value(); }));
    };
    auto configure = [&](int mode) {
        QDBusInterface manager(service, managerPath, "io.github.lebedenko.Strata.Manager1", bus);
        CHECK(manager.call("Configure", mode).type() == QDBusMessage::ReplyMessage);
    };
    auto verify = [&](const QString &id) {
        CHECK(until([&]() {
            return client.deviceId() == id && model.getKeyData(0)["primaryLabel"] == id;
        }));
        CHECK(client.isConnected());
        CHECK(client.deviceName() == id);
        CHECK(client.deviceNode() == "/dev/" + id);
        CHECK(client.buildId() == "build-" + id);
        CHECK(client.layers().size() == 2);
        CHECK(client.selectedLayerIndex() == 1);
        CHECK(model.currentLayer() == 1);
        CHECK(model.layoutId() == (id == "voyager" ? "voyager" : "eyelash_corne"));
    };
    auto cleared = [&]() {
        return !client.isConnected() && client.deviceId().isEmpty() &&
               client.activeDevicePath().isEmpty() && client.layers().isEmpty() &&
               client.buildId().isEmpty() && client.selectedLayerIndex() == 0 &&
               model.getKeyData(0)["primaryLabel"] == "0";
    };
    start();
    verify("corne");
    configure(1);
    verify("corne");
    configure(2);
    verify("voyager");
    CHECK(client.selectDevice("corne"));
    verify("corne");
    configure(0);
    CHECK(until(cleared));
    configure(2);
    verify("voyager");
    daemon.kill();
    CHECK(daemon.waitForFinished());
    CHECK(until(cleared));
    start();
    verify("corne");
    QProcess replacement;
    replacement.start(app.applicationFilePath(), {"--daemon", "--voyager"});
    CHECK(replacement.waitForStarted());
    verify("voyager");
    replacement.kill();
    CHECK(replacement.waitForFinished());
    CHECK(until(cleared));
    daemon.kill();
    CHECK(daemon.waitForFinished());
    start();
    verify("corne");

    strata::config::ConfigManager config;
    auto testWindow = [&](strata::tray::TrayManager &tray, bool canHide) {
        QQmlApplicationEngine engine;
        engine.rootContext()->setContextProperty("trayManager", &tray);
        engine.rootContext()->setContextProperty("configManager", &config);
        engine.rootContext()->setContextProperty("strataDBusClient", &client);
        engine.rootContext()->setContextProperty("strataKeymapModel", &model);
        engine.load(QUrl::fromLocalFile(QDir::current().filePath("qml/Main.qml")));
        CHECK(!engine.rootObjects().isEmpty());
        auto *window = qobject_cast<QQuickWindow *>(engine.rootObjects().first());
        CHECK(window);
        strata::configureWindow(window, &tray, true);
        CHECK(window->isVisible() == !canHide);
        window->show();
        CHECK(window->close() == !canHide);
        CHECK(!window->isVisible());
        if (canHide) {
            config.setShowTrayIcon(false);
            CHECK(window->isVisible());
            config.setShowTrayIcon(true);
        }
    };
    config.setShowTrayIcon(true);
    {
        QObject conflict;
        CHECK(bus.registerObject("/StatusNotifierItem", &conflict));
        strata::tray::TrayManager tray(&client, &config);
        CHECK(!tray.canHideToTray());
        testWindow(tray, false);
        bus.unregisterObject("/StatusNotifierItem");
    }
    {
        strata::tray::TrayManager tray(&client, &config);
        CHECK(tray.canHideToTray());
        testWindow(tray, true);
        QProcess watcher;
        for (int attempt = 0; attempt < 2; ++attempt) {
            watcher.start(app.applicationFilePath(), {"--watcher"});
            CHECK(watcher.waitForStarted());
            CHECK(until([&]() {
                QDBusInterface iface("org.kde.StatusNotifierWatcher", "/StatusNotifierWatcher",
                                     "org.kde.StatusNotifierWatcher", bus);
                QDBusReply<int> count = iface.call("Count");
                return count.isValid() && count.value() > 0;
            }));
            watcher.kill();
            CHECK(watcher.waitForFinished());
            CHECK(until([&]() {
                return !bus.interface()
                            ->isServiceRegistered("org.kde.StatusNotifierWatcher")
                            .value();
            }));
        }
        auto *sni = tray.findChild<strata::tray::StatusNotifierItem *>();
        CHECK(sni);
        CHECK(sni->toolTip().title.contains("corne"));
        int toggles = 0, quits = 0;
        QObject::connect(&tray, &strata::tray::TrayManager::toggleWindowRequested,
                         [&]() { ++toggles; });
        QObject::connect(&tray, &strata::tray::TrayManager::quitRequested, [&]() { ++quits; });
        sni->Activate(0, 0);
        sni->menuService()->Event(1, "clicked", QDBusVariant(), 0);
        sni->menuService()->Event(6, "clicked", QDBusVariant(), 0);
        CHECK(toggles == 2 && quits == 1);
        config.setShowTrayIcon(false);
        CHECK(!tray.canHideToTray());
        testWindow(tray, false);
        config.setShowTrayIcon(true);
        CHECK(tray.canHideToTray());
        configure(0);
        CHECK(until(cleared));
        CHECK(sni->toolTip().description == "Keyboard Disconnected");
    }
    daemon.kill();
    CHECK(daemon.waitForFinished());
    return 0;
}
#include "test_lifecycle.moc"
