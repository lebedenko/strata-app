#include "dbus/strata_dbus_client.hpp"

#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusReply>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <iostream>

namespace strata::dbus {

static const QString ServiceName = "org.freedesktop.Strata";
static const QString ObjectPath = "/org/freedesktop/Strata/Device0";
static const QString InterfaceName = "org.freedesktop.Strata.Device1";

StrataDBusClient::StrataDBusClient(QObject *parent)
    : QObject(parent) {
    setupDBusConnections();
    refreshStatus();
    refreshLayers();
}

void StrataDBusClient::setupDBusConnections() {
    auto bus = QDBusConnection::sessionBus();

    bus.connect(ServiceName, ObjectPath, InterfaceName, "LayerChanged", this,
                SLOT(onLayerChangedSignal(uint, QString, uint, QString)));

    bus.connect(ServiceName, ObjectPath, InterfaceName, "DeviceConnected", this,
                SLOT(onDeviceConnectedSignal(QString, QString, QString)));

    bus.connect(ServiceName, ObjectPath, InterfaceName, "DeviceDisconnected", this,
                SLOT(onDeviceDisconnectedSignal(QString)));

    bus.connect(ServiceName, ObjectPath, InterfaceName, "KeymapLoaded", this,
                SLOT(onKeymapLoadedSignal(QString, QString, uint)));

    bus.connect(ServiceName, ObjectPath, InterfaceName, "LayerBindingsLoaded", this,
                SLOT(onLayerBindingsLoadedSignal(uint, uint)));
}

void StrataDBusClient::setSelectedLayerIndex(int index) {
    if (selectedLayerIndex_ != index) {
        selectedLayerIndex_ = index;
        emit selectedLayerChanged();
    }
}

void StrataDBusClient::refreshStatus() {
    QDBusInterface iface(ServiceName, ObjectPath, InterfaceName, QDBusConnection::sessionBus());
    if (!iface.isValid()) {
        connected_ = false;
        emit statusChanged();
        return;
    }

    QDBusReply<QString> reply = iface.call("GetStatus");
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
    deviceName_ = obj.value("name").toString();
    deviceNode_ = obj.value("node").toString();
    buildId_ = obj.value("build_id").toString();
    layersCount_ = obj.value("layers_count").toInt(0);
    isCached_ = obj.value("cached").toBool(false);

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
    QDBusInterface iface(ServiceName, ObjectPath, InterfaceName, QDBusConnection::sessionBus());
    if (!iface.isValid())
        return;

    QDBusReply<QString> reply = iface.call("GetLayers");
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
            map["index"] = lObj.value("index").toInt();
            map["id"] = lObj.value("id").toInt();
            map["name"] = lObj.value("name").toString();
            map["active"] = lObj.value("active").toBool();
            newList.append(map);
        }
    }

    layers_ = newList;
    emit layersChanged();
}

void StrataDBusClient::refreshKeymap() {
    QDBusInterface iface(ServiceName, ObjectPath, InterfaceName, QDBusConnection::sessionBus());
    if (iface.isValid()) {
        iface.call("RefreshKeymap");
    }
}

void StrataDBusClient::clearCache() {
    QDBusInterface iface(ServiceName, ObjectPath, InterfaceName, QDBusConnection::sessionBus());
    if (iface.isValid()) {
        iface.call("ClearCache");
        refreshStatus();
        refreshLayers();
    }
}

QJsonObject StrataDBusClient::fetchKeymapJson(int layer, bool forceRefresh) {
    QDBusInterface iface(ServiceName, ObjectPath, InterfaceName, QDBusConnection::sessionBus());
    if (!iface.isValid())
        return {};

    QDBusReply<QString> reply = iface.call("GetKeymap", static_cast<uint>(layer), forceRefresh);
    if (!reply.isValid())
        return {};

    QJsonDocument doc = QJsonDocument::fromJson(reply.value().toUtf8());
    return doc.isObject() ? doc.object() : QJsonObject{};
}

void StrataDBusClient::onLayerChangedSignal(uint index, const QString &name, uint mask,
                                            const QString &buildId) {
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

void StrataDBusClient::onDeviceConnectedSignal(const QString &name, const QString &node,
                                               const QString &buildId) {
    connected_ = true;
    deviceName_ = name;
    deviceNode_ = node;
    buildId_ = buildId;
    emit statusChanged();
    emit deviceConnected(name, node, buildId);
    refreshLayers();
}

void StrataDBusClient::onDeviceDisconnectedSignal(const QString &node) {
    connected_ = false;
    deviceNode_ = node;
    emit statusChanged();
    emit deviceDisconnected(node);
}

void StrataDBusClient::onKeymapLoadedSignal(const QString &buildId, const QString &source,
                                            uint layerCount) {
    buildId_ = buildId;
    layersCount_ = static_cast<int>(layerCount);
    isCached_ = (source == "cache");
    emit statusChanged();
    emit keymapLoaded(buildId, source, layerCount);
    refreshLayers();
}

void StrataDBusClient::onLayerBindingsLoadedSignal(uint layer, uint count) {
    emit layerBindingsLoaded(static_cast<int>(layer), static_cast<int>(count));
}

} // namespace strata::dbus
