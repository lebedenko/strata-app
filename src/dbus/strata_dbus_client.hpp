#pragma once

#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusMessage>
#include <QJsonObject>
#include <QObject>
#include <QString>
#include <QVariantList>
#include <QVariantMap>

namespace strata::dbus {

class StrataDBusClient : public QObject {
    Q_OBJECT

    Q_PROPERTY(bool connected READ isConnected NOTIFY statusChanged)
    Q_PROPERTY(QString deviceName READ deviceName NOTIFY statusChanged)
    Q_PROPERTY(QString deviceNode READ deviceNode NOTIFY statusChanged)
    Q_PROPERTY(QString buildId READ buildId NOTIFY statusChanged)
    Q_PROPERTY(int activeLayerIndex READ activeLayerIndex NOTIFY activeLayerChanged)
    Q_PROPERTY(QString activeLayerName READ activeLayerName NOTIFY activeLayerChanged)
    Q_PROPERTY(uint activeLayerMask READ activeLayerMask NOTIFY activeLayerChanged)
    Q_PROPERTY(int layersCount READ layersCount NOTIFY statusChanged)
    Q_PROPERTY(bool isCached READ isCached NOTIFY statusChanged)
    Q_PROPERTY(QVariantList layers READ layers NOTIFY layersChanged)
    Q_PROPERTY(int selectedLayerIndex READ selectedLayerIndex WRITE setSelectedLayerIndex NOTIFY
                   selectedLayerChanged)

public:
    explicit StrataDBusClient(QObject *parent = nullptr);

    [[nodiscard]] bool isConnected() const noexcept { return connected_; }
    [[nodiscard]] QString deviceName() const noexcept { return deviceName_; }
    [[nodiscard]] QString deviceNode() const noexcept { return deviceNode_; }
    [[nodiscard]] QString buildId() const noexcept { return buildId_; }
    [[nodiscard]] int activeLayerIndex() const noexcept { return activeLayerIndex_; }
    [[nodiscard]] QString activeLayerName() const noexcept { return activeLayerName_; }
    [[nodiscard]] uint activeLayerMask() const noexcept { return activeLayerMask_; }
    [[nodiscard]] int layersCount() const noexcept { return layersCount_; }
    [[nodiscard]] bool isCached() const noexcept { return isCached_; }
    [[nodiscard]] const QVariantList &layers() const noexcept { return layers_; }
    [[nodiscard]] int selectedLayerIndex() const noexcept { return selectedLayerIndex_; }
    void setSelectedLayerIndex(int index);

    Q_INVOKABLE void refreshStatus();
    Q_INVOKABLE void refreshLayers();
    Q_INVOKABLE void refreshKeymap();
    Q_INVOKABLE void clearCache();
    Q_INVOKABLE QJsonObject fetchKeymapJson(int layer, bool forceRefresh = false);

signals:
    void statusChanged();
    void activeLayerChanged(int index, const QString &name, uint mask);
    void layersChanged();
    void selectedLayerChanged();
    void keymapLoaded(const QString &buildId, const QString &source, uint layerCount);
    void layerBindingsLoaded(int layer, int count);
    void deviceConnected(const QString &name, const QString &node, const QString &buildId);
    void deviceDisconnected(const QString &node);

private slots:
    void onLayerChangedSignal(uint index, const QString &name, uint mask, const QString &buildId);
    void onDeviceConnectedSignal(const QString &name, const QString &node, const QString &buildId);
    void onDeviceDisconnectedSignal(const QString &node);
    void onKeymapLoadedSignal(const QString &buildId, const QString &source, uint layerCount);
    void onLayerBindingsLoadedSignal(uint layer, uint count);

private:
    void setupDBusConnections();
    void parseStatusJson(const QString &jsonStr);
    void parseLayersJson(const QString &jsonStr);

    bool connected_{false};
    QString deviceName_;
    QString deviceNode_;
    QString buildId_;
    int activeLayerIndex_{0};
    QString activeLayerName_{"unknown"};
    uint activeLayerMask_{0};
    int layersCount_{0};
    bool isCached_{false};
    QVariantList layers_;
    int selectedLayerIndex_{0};
};

} // namespace strata::dbus
