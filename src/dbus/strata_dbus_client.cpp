#include "dbus/strata_dbus_client.hpp"

#include <QDBusArgument>
#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusInterface>
#include <QDBusMetaType>
#include <QDBusReply>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <iostream>

namespace strata::dbus {

static const QString ModernService = "io.github.lebedenko.Strata";
static const QString ManagerPath = "/io/github/lebedenko/Strata/Manager";
static const QString ManagerInterface = "io.github.lebedenko.Strata.Manager1";
static const QString DeviceInterface = "io.github.lebedenko.Strata.Device1";
static const QString KeymapInterface = "io.github.lebedenko.Strata.Keymap1";

StrataDBusClient::StrataDBusClient(QObject *parent)
    : QObject(parent) {
    setupDBusConnections();
    refreshDevices();
    refreshStatus();
    refreshLayers();
}

void StrataDBusClient::setupDBusConnections() {
    auto bus = QDBusConnection::sessionBus();

    // 1. Manager signals
    bus.connect(ModernService, ManagerPath, ManagerInterface, "DeviceAdded", this,
                SLOT(onDeviceAddedSignal(QDBusObjectPath, QString)));
    bus.connect(ModernService, ManagerPath, ManagerInterface, "DeviceRemoved", this,
                SLOT(onDeviceRemovedSignal(QDBusObjectPath, QString)));
    bus.connect(ModernService, ManagerPath, ManagerInterface, "ActiveDeviceChanged", this,
                SLOT(onActiveDeviceChangedSignal(QDBusObjectPath, QString)));

    // 2. Device & Keymap signals across all device paths
    bus.connect(ModernService, QString(), DeviceInterface, "LayerChanged", this,
                SLOT(onLayerChangedSignal(uint, QString, uint, QString, QDBusMessage)));
    bus.connect(ModernService, QString(), KeymapInterface, "KeymapLoaded", this,
                SLOT(onKeymapLoadedSignal(QString, QString, uint, QDBusMessage)));
    bus.connect(ModernService, QString(), KeymapInterface, "LayerBindingsLoaded", this,
                SLOT(onLayerBindingsLoadedSignal(uint, uint, QDBusMessage)));
}

void StrataDBusClient::setSelectedLayerIndex(int index) {
    if (selectedLayerIndex_ != index) {
        selectedLayerIndex_ = index;
        emit selectedLayerChanged();
    }
}

void StrataDBusClient::refreshDevices() {
    auto bus = QDBusConnection::sessionBus();
    QDBusInterface mgrIface(ModernService, ManagerPath, ManagerInterface, bus);

    QVariantList newDevices;
    QString activePath;

    if (mgrIface.isValid()) {
        QDBusMessage replyMsg = mgrIface.call("GetDevices");
        QStringList devPaths;
        if (replyMsg.type() == QDBusMessage::ReplyMessage && !replyMsg.arguments().isEmpty()) {
            QVariant arg0 = replyMsg.arguments().at(0);
            if (arg0.userType() == qMetaTypeId<QDBusArgument>()) {
                const QDBusArgument arg = arg0.value<QDBusArgument>();
                if (arg.currentType() == QDBusArgument::ArrayType) {
                    arg.beginArray();
                    while (!arg.atEnd()) {
                        QDBusObjectPath p;
                        arg >> p;
                        devPaths.append(p.path());
                    }
                    arg.endArray();
                }
            } else if (arg0.canConvert<QStringList>()) {
                devPaths = arg0.toStringList();
            }
        }

        for (const auto &pathStr : devPaths) {
            QDBusInterface devIface(ModernService, pathStr, DeviceInterface, bus);
            if (devIface.isValid()) {
                QDBusReply<QString> stReply = devIface.call("GetStatus");
                if (stReply.isValid()) {
                    QJsonDocument doc = QJsonDocument::fromJson(stReply.value().toUtf8());
                    if (doc.isObject()) {
                        QJsonObject obj = doc.object();
                        QVariantMap dMap;
                        QString devId = obj.value("id").toString();
                        if (devId.isEmpty()) {
                            devId = pathStr.section('/', -1);
                        }
                        dMap["id"] = devId;
                        dMap["path"] = pathStr;
                        dMap["name"] = obj.value("name").toString(devId);
                        dMap["type"] = obj.value("type").toString();
                        dMap["node"] = obj.value("node").toString();
                        dMap["connected"] = obj.value("connected").toBool(true);
                        newDevices.append(dMap);
                    }
                }
            }
        }

        QDBusMessage actMsg = mgrIface.call("GetActiveDevice");
        if (actMsg.type() == QDBusMessage::ReplyMessage && !actMsg.arguments().isEmpty()) {
            QVariant v = actMsg.arguments().at(0);
            if (v.canConvert<QDBusObjectPath>()) {
                activePath = v.value<QDBusObjectPath>().path();
            } else {
                activePath = v.toString();
            }
        }
    }

    if (!activePath.isEmpty() && activePath != "/") {
        activeDevicePath_ = activePath;
        deviceId_ = activeDevicePath_.section('/', -1);
    }

    availableDevices_ = newDevices;
    emit devicesChanged();
}

bool StrataDBusClient::selectDevice(const QString &deviceId) {
    auto bus = QDBusConnection::sessionBus();
    QDBusInterface mgrIface(ModernService, ManagerPath, ManagerInterface, bus);
    if (mgrIface.isValid()) {
        QDBusReply<bool> reply = mgrIface.call("SetActiveDevice", deviceId);
        if (reply.isValid() && reply.value()) {
            refreshDevices();
            refreshStatus();
            refreshLayers();
            emit activeDeviceChanged(deviceId_, activeDevicePath_);
            return true;
        }
    }
    return false;
}

std::unique_ptr<QDBusInterface>
StrataDBusClient::makeDeviceInterface(const QString &interfaceName) {
    auto bus = QDBusConnection::sessionBus();
    QString path = activeDevicePath_.isEmpty() ? QStringLiteral("/") : activeDevicePath_;
    return std::make_unique<QDBusInterface>(ModernService, path, interfaceName, bus);
}

void StrataDBusClient::refreshStatus() {
    auto devIface = makeDeviceInterface(DeviceInterface);
    if (!devIface->isValid()) {
        connected_ = false;
        emit statusChanged();
        return;
    }

    QDBusReply<QString> reply = devIface->call("GetStatus");
    if (reply.isValid()) {
        parseStatusJson(reply.value());
    } else {
        connected_ = false;
        emit statusChanged();
    }
}

void StrataDBusClient::parseStatusJson(const QString &jsonStr) {
    QJsonDocument doc = QJsonDocument::fromJson(jsonStr.toUtf8());
    if (!doc.isObject())
        return;

    QJsonObject obj = doc.object();
    connected_ = obj.value("connected").toBool(false);
    deviceId_ = obj.value("id").toString();
    deviceName_ = obj.value("name").toString();
    deviceNode_ = obj.value("node").toString();
    deviceType_ = obj.value("type").toString();
    buildId_ = obj.value("build_id").toString();
    layersCount_ = obj.value("layers_count").toInt(0);
    isCached_ = obj.value("cached").toBool(false);

    capabilities_.clear();
    if (obj.contains("capabilities") && obj["capabilities"].isArray()) {
        for (const auto &c : obj["capabilities"].toArray()) {
            capabilities_.append(c.toString());
        }
    }

    if (deviceId_.isEmpty() && !activeDevicePath_.isEmpty()) {
        deviceId_ = activeDevicePath_.section('/', -1);
    }

    if (obj.contains("active_layer") && obj["active_layer"].isObject()) {
        QJsonObject active = obj["active_layer"].toObject();
        activeLayerIndex_ = active.value("index").toInt(0);
        activeLayerName_ = active.value("name").toString("unknown");
        activeLayerMask_ = static_cast<uint>(active.value("mask").toInteger(0));
        setSelectedLayerIndex(activeLayerIndex_);
    }

    emit statusChanged();
    emit activeLayerChanged(activeLayerIndex_, activeLayerName_, activeLayerMask_);
}

void StrataDBusClient::refreshLayers() {
    auto devIface = makeDeviceInterface(KeymapInterface);
    if (!devIface->isValid())
        return;

    QDBusReply<QString> reply = devIface->call("GetLayers");
    if (reply.isValid()) {
        parseLayersJson(reply.value());
    }
}

void StrataDBusClient::parseLayersJson(const QString &jsonStr) {
    QJsonDocument doc = QJsonDocument::fromJson(jsonStr.toUtf8());
    if (!doc.isArray())
        return;

    QVariantList newList;
    for (const auto &val : doc.array()) {
        if (val.isObject()) {
            QJsonObject lObj = val.toObject();
            QVariantMap map;
            int idx = lObj.value("index").toInt();
            map["index"] = idx;
            map["id"] = lObj.value("id").toInt();
            map["name"] = lObj.value("name").toString();
            map["active"] = (idx == activeLayerIndex_);
            newList.append(map);
        }
    }

    layers_ = newList;
    emit layersChanged();
}

void StrataDBusClient::refreshKeymap() {
    auto devIface = makeDeviceInterface(KeymapInterface);
    if (devIface->isValid()) {
        devIface->call("RefreshKeymap");
    }
}

void StrataDBusClient::clearCache() {
    auto devIface = makeDeviceInterface(KeymapInterface);
    if (devIface->isValid()) {
        devIface->call("ClearCache");
        refreshStatus();
        refreshLayers();
    }
}

QJsonObject StrataDBusClient::fetchKeymapJson(int layer, bool forceRefresh) {
    auto devIface = makeDeviceInterface(KeymapInterface);
    if (!devIface->isValid())
        return {};

    QDBusReply<QString> reply = devIface->call("GetKeymap", static_cast<uint>(layer), forceRefresh);
    if (!reply.isValid())
        return {};

    QJsonDocument doc = QJsonDocument::fromJson(reply.value().toUtf8());
    return doc.isObject() ? doc.object() : QJsonObject{};
}

bool StrataDBusClient::setLayer(int layer, bool lock) {
    auto devIface = makeDeviceInterface(DeviceInterface);
    if (devIface->isValid()) {
        QDBusReply<bool> reply = devIface->call("SetLayer", static_cast<uint>(layer), lock);
        return reply.isValid() && reply.value();
    }
    return false;
}

bool StrataDBusClient::setRgbControl(bool enable) {
    auto devIface = makeDeviceInterface(DeviceInterface);
    if (devIface->isValid()) {
        QDBusReply<bool> reply = devIface->call("SetRgbControl", enable);
        return reply.isValid() && reply.value();
    }
    return false;
}

bool StrataDBusClient::setRgbAll(int r, int g, int b) {
    auto devIface = makeDeviceInterface(DeviceInterface);
    if (devIface->isValid()) {
        QDBusReply<bool> reply = devIface->call("SetRgbAll", static_cast<uchar>(r),
                                                static_cast<uchar>(g), static_cast<uchar>(b));
        return reply.isValid() && reply.value();
    }
    return false;
}

bool StrataDBusClient::updateBrightness(bool increase) {
    auto devIface = makeDeviceInterface(DeviceInterface);
    if (devIface->isValid()) {
        QDBusReply<bool> reply = devIface->call("UpdateBrightness", increase);
        return reply.isValid() && reply.value();
    }
    return false;
}

void StrataDBusClient::onLayerChangedSignal(uint index, const QString &name, uint mask,
                                            const QString &buildId, const QDBusMessage &msg) {
    if (activeDevicePath_.isEmpty() || msg.path() != activeDevicePath_) {
        return;
    }

    activeLayerIndex_ = static_cast<int>(index);
    activeLayerName_ = name;
    activeLayerMask_ = mask;
    if (!buildId.isEmpty()) {
        buildId_ = buildId;
    }

    for (auto &layerVar : layers_) {
        QVariantMap map = layerVar.toMap();
        map["active"] = (map["index"].toInt() == activeLayerIndex_);
        layerVar = map;
    }

    emit activeLayerChanged(activeLayerIndex_, activeLayerName_, activeLayerMask_);
    emit layersChanged();

    setSelectedLayerIndex(activeLayerIndex_);
}

void StrataDBusClient::onKeymapLoadedSignal(const QString &buildId, const QString &source,
                                            uint layerCount, const QDBusMessage &msg) {
    if (activeDevicePath_.isEmpty() || msg.path() != activeDevicePath_) {
        return;
    }

    buildId_ = buildId;
    layersCount_ = static_cast<int>(layerCount);
    isCached_ = (source == "cache");
    emit statusChanged();
    emit keymapLoaded(buildId, source, layerCount);
    refreshLayers();
}

void StrataDBusClient::onLayerBindingsLoadedSignal(uint layer, uint count,
                                                   const QDBusMessage &msg) {
    if (activeDevicePath_.isEmpty() || msg.path() != activeDevicePath_) {
        return;
    }

    emit layerBindingsLoaded(static_cast<int>(layer), static_cast<int>(count));
}

void StrataDBusClient::onDeviceAddedSignal(const QDBusObjectPath &path, const QString &deviceId) {
    refreshDevices();
    emit deviceConnected(deviceId, path.path(), "");
}

void StrataDBusClient::onDeviceRemovedSignal(const QDBusObjectPath &path, const QString &deviceId) {
    Q_UNUSED(path);
    refreshDevices();
    emit deviceDisconnected(deviceId);
}

void StrataDBusClient::onActiveDeviceChangedSignal(const QDBusObjectPath &path,
                                                   const QString &deviceId) {
    if (activeDevicePath_ == path.path() && deviceId_ == deviceId) {
        return;
    }
    activeDevicePath_ = path.path();
    deviceId_ = deviceId;
    refreshStatus();
    refreshLayers();
    emit activeDeviceChanged(deviceId_, activeDevicePath_);
    emit devicesChanged();
}

} // namespace strata::dbus
