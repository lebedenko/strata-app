#pragma once

#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusMessage>
#include <QDBusObjectPath>
#include <QJsonObject>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariantList>
#include <QVariantMap>
#include <memory>

namespace strata::dbus {

class StrataDBusClient : public QObject {
    Q_OBJECT

    Q_PROPERTY(bool connected READ isConnected NOTIFY statusChanged)
    Q_PROPERTY(QString deviceId READ deviceId NOTIFY statusChanged)
    Q_PROPERTY(QString deviceName READ deviceName NOTIFY statusChanged)
    Q_PROPERTY(QString deviceNode READ deviceNode NOTIFY statusChanged)
    Q_PROPERTY(QString deviceType READ deviceType NOTIFY statusChanged)
    Q_PROPERTY(QStringList capabilities READ capabilities NOTIFY statusChanged)
    Q_PROPERTY(QString buildId READ buildId NOTIFY statusChanged)
    Q_PROPERTY(int activeLayerIndex READ activeLayerIndex NOTIFY activeLayerChanged)
    Q_PROPERTY(QString activeLayerName READ activeLayerName NOTIFY activeLayerChanged)
    Q_PROPERTY(uint activeLayerMask READ activeLayerMask NOTIFY activeLayerChanged)
    Q_PROPERTY(int layersCount READ layersCount NOTIFY statusChanged)
    Q_PROPERTY(bool isCached READ isCached NOTIFY statusChanged)
    Q_PROPERTY(QVariantList layers READ layers NOTIFY layersChanged)
    Q_PROPERTY(int selectedLayerIndex READ selectedLayerIndex WRITE setSelectedLayerIndex NOTIFY
                   selectedLayerChanged)
    Q_PROPERTY(QVariantList availableDevices READ availableDevices NOTIFY devicesChanged)
    Q_PROPERTY(QString activeDevicePath READ activeDevicePath NOTIFY activeDeviceChanged)

public:
    explicit StrataDBusClient(QObject *parent = nullptr);

    [[nodiscard]] bool isConnected() const noexcept { return connected_; }
    [[nodiscard]] QString deviceId() const noexcept { return deviceId_; }
    [[nodiscard]] QString deviceName() const noexcept { return deviceName_; }
    [[nodiscard]] QString deviceNode() const noexcept { return deviceNode_; }
    [[nodiscard]] QString deviceType() const noexcept { return deviceType_; }
    [[nodiscard]] QStringList capabilities() const noexcept { return capabilities_; }
    [[nodiscard]] QString buildId() const noexcept { return buildId_; }
    [[nodiscard]] int activeLayerIndex() const noexcept { return activeLayerIndex_; }
    [[nodiscard]] QString activeLayerName() const noexcept { return activeLayerName_; }
    [[nodiscard]] uint activeLayerMask() const noexcept { return activeLayerMask_; }
    [[nodiscard]] int layersCount() const noexcept { return layersCount_; }
    [[nodiscard]] bool isCached() const noexcept { return isCached_; }
    [[nodiscard]] const QVariantList &layers() const noexcept { return layers_; }
    [[nodiscard]] int selectedLayerIndex() const noexcept { return selectedLayerIndex_; }
    void setSelectedLayerIndex(int index);

    [[nodiscard]] const QVariantList &availableDevices() const noexcept {
        return availableDevices_;
    }
    [[nodiscard]] QString activeDevicePath() const noexcept { return activeDevicePath_; }

    Q_INVOKABLE void refreshDevices();
    Q_INVOKABLE bool selectDevice(const QString &deviceId);

    Q_INVOKABLE void refreshStatus();
    Q_INVOKABLE void refreshLayers();
    Q_INVOKABLE void refreshKeymap();
    Q_INVOKABLE void clearCache();
    Q_INVOKABLE QJsonObject fetchKeymapJson(int layer, bool forceRefresh = false);

    // Hardware write commands
    Q_INVOKABLE bool setLayer(int layer, bool lock = true);
    Q_INVOKABLE bool setRgbControl(bool enable);
    Q_INVOKABLE bool setRgbAll(int r, int g, int b);
    Q_INVOKABLE bool updateBrightness(bool increase);

signals:
    void statusChanged();
    void activeLayerChanged(int index, const QString &name, uint mask);
    void layersChanged();
    void selectedLayerChanged();
    void keymapLoaded(const QString &buildId, const QString &source, uint layerCount);
    void layerBindingsLoaded(int layer, int count);
    void deviceConnected(const QString &name, const QString &node, const QString &buildId);
    void deviceDisconnected(const QString &node);
    void devicesChanged();
    void activeDeviceChanged(const QString &deviceId, const QString &path);

private slots:
    void onLayerChangedSignal(uint index, const QString &name, uint mask, const QString &buildId,
                              const QDBusMessage &msg);
    void onKeymapLoadedSignal(const QString &buildId, const QString &source, uint layerCount,
                              const QDBusMessage &msg);
    void onLayerBindingsLoadedSignal(uint layer, uint count, const QDBusMessage &msg);

    void onDeviceAddedSignal(const QDBusObjectPath &path, const QString &deviceId);
    void onDeviceRemovedSignal(const QDBusObjectPath &path, const QString &deviceId);
    void onActiveDeviceChangedSignal(const QDBusObjectPath &path, const QString &deviceId);

private:
    void setupDBusConnections();
    std::unique_ptr<QDBusInterface> makeDeviceInterface(const QString &interfaceName);
    void parseStatusJson(const QString &jsonStr);
    void parseLayersJson(const QString &jsonStr);

    bool connected_{false};
    QString deviceId_;
    QString deviceName_;
    QString deviceNode_;
    QString deviceType_;
    QStringList capabilities_;
    QString buildId_;
    int activeLayerIndex_{0};
    QString activeLayerName_{"unknown"};
    uint activeLayerMask_{0};
    int layersCount_{0};
    bool isCached_{false};
    QVariantList layers_;
    int selectedLayerIndex_{0};

    QVariantList availableDevices_;
    QString activeDevicePath_;
};

} // namespace strata::dbus
