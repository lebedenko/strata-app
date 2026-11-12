#include <QApplication>
#include <QDBusAbstractAdaptor>
#include <QDBusConnectionInterface>
#include <QDBusReply>
#include <QDir>
#include <QElapsedTimer>
#include <QObject>
#include <QProcess>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QTemporaryDir>
#include <QThread>
#include <QTimer>
#include <csignal>
#include <cstdlib>
#include <functional>
#include <iostream>

#include "config/config_manager.hpp"
#include "dbus/strata_dbus_client.hpp"
#include "model/keymap_model.hpp"
#include "tray/dbus_menu.hpp"
#include "tray/tray_manager.hpp"
#include "window.hpp"

namespace {
volatile std::sig_atomic_t stopRequested = 0;
void requestStop(int /*signal*/) {
    stopRequested = 1;
}
} // namespace

constexpr QLatin1StringView service("io.github.lebedenko.Strata");
constexpr QLatin1StringView managerPath("/io/github/lebedenko/Strata/Manager");
constexpr QLatin1StringView base("/io/github/lebedenko/Strata/");
#define CHECK(x)                                                                                   \
    do {                                                                                           \
        const bool passed = (x);                                                                   \
        if (!passed) {                                                                             \
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
    // These const slots are exported test D-Bus methods.
    [[nodiscard]] QString GetStatus() const { // clazy:exclude=const-signal-or-slot
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
    // These const slots are exported test D-Bus methods.
    [[nodiscard]] QString GetKeymap(uint layer, // clazy:exclude=const-signal-or-slot
                                    bool /*unused*/) const {
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
    // These const slots are exported test D-Bus methods.
    // Qt MOC requires value returns for exported D-Bus slots.
    // cppcheck-suppress returnByReference
    [[nodiscard]] QList<QDBusObjectPath> GetDevices() const { // clazy:exclude=const-signal-or-slot
        return devices;
    }
    // These const slots are exported test D-Bus methods.
    [[nodiscard]] QDBusObjectPath GetActiveDevice() const { // clazy:exclude=const-signal-or-slot
        return active;
    }
    bool SetActiveDevice(const QString &id) {
        active = QDBusObjectPath(base + id);
        emit ActiveDeviceChanged(active, id);
        return true;
    }
    void Configure(int mode) {
        auto old = active;
        devices.clear();
        if (mode != 0) {
            devices << QDBusObjectPath(base + QStringLiteral("corne"))
                    << QDBusObjectPath(base + QStringLiteral("voyager"));
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
    void DeviceAdded(const QDBusObjectPath & /*_t1*/, const QString & /*_t2*/);
    void DeviceRemoved(const QDBusObjectPath & /*_t1*/, const QString & /*_t2*/);
    void ActiveDeviceChanged(const QDBusObjectPath & /*_t1*/, const QString & /*_t2*/);
};
class Watcher : public QObject {
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.StatusNotifierWatcher")
public:
    int count = 0;
public slots:
    void RegisterStatusNotifierItem(const QString & /*unused*/) { ++count; }
    // These const slots are exported test D-Bus methods.
    [[nodiscard]] int Count() const { // clazy:exclude=const-signal-or-slot
        return count;
    }
};
static bool until(const std::function<bool()> &predicate) {
    QElapsedTimer timer;
    timer.start();
    do {
        QCoreApplication::processEvents();
        if (predicate()) {
            return true;
        }
        QThread::msleep(5);
    } while (timer.elapsed() < (qEnvironmentVariableIsSet("STRATA_INSTRUMENTED") ? 120000 : 5000));
    return false;
}
static void stopProcess(QProcess &process) {
    const bool instrumented = qEnvironmentVariableIsSet("STRATA_INSTRUMENTED");
    if (instrumented) {
        process.terminate();
    } else {
        process.kill();
    }
    CHECK(process.waitForFinished(instrumented ? 120000 : 30000));
    if (instrumented) {
        CHECK(process.exitStatus() == QProcess::NormalExit);
        CHECK(process.exitCode() == 0);
    }
}
int main(int argc, char **argv) {
    QTemporaryDir configuration;
    CHECK(configuration.isValid());
    qputenv("XDG_CONFIG_HOME", configuration.path().toUtf8());
    QApplication app(argc, argv);
    QTimer terminationPoll;
    if (qEnvironmentVariableIsSet("STRATA_INSTRUMENTED")) {
        std::signal(SIGTERM, requestStop);
        QObject::connect(&terminationPoll, &QTimer::timeout, &app, [] {
            if (stopRequested != 0) {
                QCoreApplication::quit();
            }
        });
        terminationPoll.start(25);
    }
    auto bus = QDBusConnection::sessionBus();
    if (QApplication::arguments().contains("--watcher")) {
        Watcher watcher;
        CHECK(bus.registerObject("/StatusNotifierWatcher", &watcher,
                                 QDBusConnection::ExportAllSlots));
        CHECK(bus.registerService("org.kde.StatusNotifierWatcher"));
        return QApplication::exec();
    }
    if (QApplication::arguments().contains("--daemon")) {
        Manager manager;
        Device corne("corne");
        Device voyager("voyager");
        new Keymap(&corne);
        new Keymap(&voyager);
        CHECK(
            bus.registerObject(base + QStringLiteral("corne"), &corne,
                               QDBusConnection::ExportAllSlots | QDBusConnection::ExportAdaptors));
        CHECK(
            bus.registerObject(base + QStringLiteral("voyager"), &voyager,
                               QDBusConnection::ExportAllSlots | QDBusConnection::ExportAdaptors));
        CHECK(bus.registerObject(managerPath, &manager, QDBusConnection::ExportAllContents));
        manager.Configure(QApplication::arguments().contains("--voyager") ? 2 : 1);
        auto registration = bus.interface()->registerService(
            service, QDBusConnectionInterface::ReplaceExistingService,
            QDBusConnectionInterface::AllowReplacement);
        CHECK(registration.isValid() &&
              registration.value() == QDBusConnectionInterface::ServiceRegistered);
        return QApplication::exec();
    }
    strata::dbus::StrataDBusClient client;
    strata::model::KeymapModel model(&client);
    CHECK(!client.isConnected());
    QProcess daemon;
    daemon.setProcessChannelMode(QProcess::ForwardedChannels);
    auto start = [&] {
        daemon.start(QApplication::applicationFilePath(), {"--daemon"});
        CHECK(daemon.waitForStarted(qEnvironmentVariableIsSet("STRATA_INSTRUMENTED") ? 120000
                                                                                     : 30000));
        CHECK(until([&]() { return bus.interface()->isServiceRegistered(service).value(); }));
    };
    auto configure = [&](int mode) {
        QDBusInterface manager(service, managerPath, "io.github.lebedenko.Strata.Manager1", bus);
        CHECK(manager.call("Configure", mode).type() == QDBusMessage::ReplyMessage);
    };
    auto verify = [&](const QString &id) {
        CHECK(until([&]() {
            return client.deviceId() == id && model.getKeyData(0).value("primaryLabel") == id;
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
    auto cleared = [&] {
        return !client.isConnected() && client.deviceId().isEmpty() &&
               client.activeDevicePath().isEmpty() && client.layers().isEmpty() &&
               client.buildId().isEmpty() && client.selectedLayerIndex() == 0 &&
               model.getKeyData(0).value("primaryLabel") == "0";
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
    stopProcess(daemon);
    CHECK(until(cleared));
    start();
    verify("corne");
    QProcess replacement;
    replacement.setProcessChannelMode(QProcess::ForwardedChannels);
    replacement.start(QApplication::applicationFilePath(), {"--daemon", "--voyager"});
    CHECK(replacement.waitForStarted(qEnvironmentVariableIsSet("STRATA_INSTRUMENTED") ? 120000
                                                                                      : 30000));
    verify("voyager");
    stopProcess(replacement);
    CHECK(until(cleared));
    stopProcess(daemon);
    start();
    verify("corne");

    strata::config::ConfigManager config;
    auto testWindow = [&](strata::tray::TrayManager &tray, bool canHide) {
        QQmlApplicationEngine engine;
        engine.setInitialProperties({{"trayManager", QVariant::fromValue(&tray)},
                                     {"configManager", QVariant::fromValue(&config)},
                                     {"strataDBusClient", QVariant::fromValue(&client)},
                                     {"strataKeymapModel", QVariant::fromValue(&model)}});
        engine.load(QUrl::fromLocalFile(QDir::current().filePath("qml/Main.qml")));
        CHECK(!engine.rootObjects().isEmpty());
        auto *window = qobject_cast<QQuickWindow *>(engine.rootObjects().constFirst());
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
        watcher.setProcessChannelMode(QProcess::ForwardedChannels);
        for (int attempt = 0; attempt < 2; ++attempt) {
            watcher.start(QApplication::applicationFilePath(), {"--watcher"});
            CHECK(watcher.waitForStarted(qEnvironmentVariableIsSet("STRATA_INSTRUMENTED") ? 120000
                                                                                          : 30000));
            CHECK(until([&]() {
                QDBusInterface iface("org.kde.StatusNotifierWatcher", "/StatusNotifierWatcher",
                                     "org.kde.StatusNotifierWatcher", bus);
                QDBusReply<int> count = iface.call("Count");
                return count.isValid() && count.value() > 0;
            }));
            stopProcess(watcher);
            CHECK(until([&]() {
                return !bus.interface()
                            ->isServiceRegistered("org.kde.StatusNotifierWatcher")
                            .value();
            }));
        }
        auto *sni = tray.findChild<strata::tray::StatusNotifierItem *>();
        CHECK(sni);
        CHECK(sni->toolTip().title.contains("corne"));
        int toggles = 0;
        int quits = 0;
        QObject::connect(&tray, &strata::tray::TrayManager::toggleWindowRequested,
                         [&] { ++toggles; });
        QObject::connect(&tray, &strata::tray::TrayManager::quitRequested, [&] { ++quits; });
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
    stopProcess(daemon);
    return 0;
}
#include "test_lifecycle.moc"
