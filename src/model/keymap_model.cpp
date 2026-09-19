#include "model/keymap_model.hpp"

#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <algorithm>
#include <iostream>

namespace strata::model {

KeymapModel::KeymapModel(dbus::StrataDBusClient *client, QObject *parent)
    : QAbstractListModel(parent)
    , client_(client) {
    // Initial layout load
    loadLayout("eyelash_corne");

    if (client_) {
        connect(client_, &dbus::StrataDBusClient::selectedLayerChanged, this,
                [this]() { setCurrentLayer(client_->selectedLayerIndex()); });

        connect(client_, &dbus::StrataDBusClient::activeLayerChanged, this,
                [this](int index, const QString &, uint) { setCurrentLayer(index); });

        connect(client_, &dbus::StrataDBusClient::layerBindingsLoaded, this,
                &KeymapModel::onLayerBindingsLoaded);

        connect(client_, &dbus::StrataDBusClient::keymapLoaded, this, &KeymapModel::onKeymapLoaded);

        connect(client_, &dbus::StrataDBusClient::statusChanged, this,
                &KeymapModel::onDeviceStateChanged);

        connect(client_, &dbus::StrataDBusClient::activeDeviceChanged, this,
                &KeymapModel::onDeviceStateChanged);

        connect(client_, &dbus::StrataDBusClient::layersChanged, this,
                [this]() { populateKeys(currentLayer_); });

        connect(client_, &dbus::StrataDBusClient::deviceConnected, this,
                [this](const QString &, const QString &, const QString &) {
                    updateLayoutFromDevice();
                    populateKeys(currentLayer_);
                });

        updateLayoutFromDevice();
        currentLayer_ = client_->selectedLayerIndex();
    }
    populateKeys(currentLayer_);
}

int KeymapModel::rowCount(const QModelIndex &parent) const {
    if (parent.isValid())
        return 0;
    return static_cast<int>(keys_.size());
}

QVariant KeymapModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= keys_.size()) {
        return {};
    }

    const auto &item = keys_.at(index.row());
    switch (role) {
    case PositionRole:
        return item.position;
    case BehaviorRole:
        return item.behavior;
    case PrimaryLabelRole:
        return item.primaryLabel;
    case SecondaryLabelRole:
        return item.secondaryLabel;
    case TooltipRole:
        return item.tooltip;
    case CategoryRole:
        return item.category;
    case Param1Role:
        return item.param1;
    case Param2Role:
        return item.param2;
    default:
        return {};
    }
}

QHash<int, QByteArray> KeymapModel::roleNames() const {
    return {
        {PositionRole, "position"},
        {BehaviorRole, "behavior"},
        {PrimaryLabelRole, "primaryLabel"},
        {SecondaryLabelRole, "secondaryLabel"},
        {TooltipRole, "tooltip"},
        {CategoryRole, "category"},
        {Param1Role, "param1"},
        {Param2Role, "param2"},
    };
}

void KeymapModel::setCurrentLayer(int layer) {
    if (currentLayer_ != layer) {
        currentLayer_ = layer;
        emit currentLayerChanged();
        populateKeys(layer);
    }
}

void KeymapModel::setLayoutId(const QString &id) {
    if (layoutId_ != id) {
        loadLayout(id);
    }
}

bool KeymapModel::loadLayout(const QString &layoutId) {
    QStringList candidates = {
        QString(":/qt/qml/Strata/resources/layouts/%1.json").arg(layoutId),
        QString(QDir::homePath() + "/.config/strata/layouts/%1.json").arg(layoutId),
        QString("/usr/share/strata/layouts/%1.json").arg(layoutId),
        QString("resources/layouts/%1.json").arg(layoutId),
        QString("../resources/layouts/%1.json").arg(layoutId)};

    QByteArray fileData;
    for (const auto &path : candidates) {
        QFile file(path);
        if (file.open(QIODevice::ReadOnly)) {
            fileData = file.readAll();
            break;
        }
    }

    if (fileData.isEmpty()) {
        qWarning("Failed to load layout profile for '%s'", qPrintable(layoutId));
        return false;
    }

    QJsonDocument doc = QJsonDocument::fromJson(fileData);
    if (!doc.isObject()) {
        qWarning("Layout profile '%s' is not a valid JSON object", qPrintable(layoutId));
        return false;
    }

    layoutJson_ = doc.object();
    layoutData_ = layoutJson_.toVariantMap();
    layoutId_ = layoutId;

    emit layoutDataChanged();
    emit layoutIdChanged();
    populateKeys(currentLayer_);
    return true;
}

void KeymapModel::updateLayoutFromDevice() {
    if (!client_)
        return;

    QString name = client_->deviceName().toLower();
    QString id = client_->deviceId().toLower();
    QString type = client_->deviceType().toLower();

    QString targetLayout = "eyelash_corne";
    if (name.contains("voyager") || id.contains("voyager") || type.contains("voyager")) {
        targetLayout = "voyager";
    } else if (name.contains("twindial") || id.contains("twindial") || type.contains("twindial")) {
        targetLayout = "twindial25";
    }

    if (targetLayout != layoutId_) {
        loadLayout(targetLayout);
    }
}

void KeymapModel::onDeviceStateChanged() {
    updateLayoutFromDevice();
    populateKeys(currentLayer_);
}

void KeymapModel::reload() {
    populateKeys(currentLayer_);
}

QVariantMap KeymapModel::getKeyData(int position) const {
    for (const auto &item : keys_) {
        if (item.position == position) {
            return {{"primaryLabel", item.primaryLabel},
                    {"secondaryLabel", item.secondaryLabel},
                    {"tooltip", item.tooltip},
                    {"category", item.category}};
        }
    }
    return {{"primaryLabel", QString::number(position)},
            {"secondaryLabel", ""},
            {"tooltip", QString("Position %1").arg(position)},
            {"category", "misc"}};
}

QVariantMap KeymapModel::getSensorData(int sensorIndex, int pressPos) const {
    // 1. Determine press key position from layout if not specified
    if (pressPos < 0) {
        pressPos = 34; // default fallback
        if (layoutJson_.contains("centerControls") && layoutJson_["centerControls"].isArray()) {
            for (const auto &cVal : layoutJson_["centerControls"].toArray()) {
                if (cVal.isObject()) {
                    auto cObj = cVal.toObject();
                    if (cObj.value("type").toString() == "rotary_knob" &&
                        cObj.value("sensorIndex").toInt(0) == sensorIndex) {
                        pressPos = cObj.value("pressPos").toInt(34);
                        break;
                    }
                }
            }
        }
    }

    // 2. Get Press Key Data
    QVariantMap pressData = getKeyData(pressPos);
    QString pressLabel = pressData.value("primaryLabel", "PUSH").toString();
    QString pressCategory = pressData.value("category", "media").toString();
    QString pressTooltip = pressData.value("tooltip", "Push Switch").toString();

    // 3. Dynamic sensor data received from hardware/cache
    for (const auto &s : sensors_) {
        if (s.sensorIndex == sensorIndex) {
            return {{"hasData", true},
                    {"behavior", s.behavior},
                    {"cwLabel", s.cwLabel},
                    {"ccwLabel", s.ccwLabel},
                    {"tooltip", s.tooltip},
                    {"category", s.category},
                    {"pressLabel", pressLabel},
                    {"pressCategory", pressCategory},
                    {"pressTooltip", pressTooltip},
                    {"pressKeyPos", pressPos}};
        }
    }

    // 4. Declarative sensor defaults from layout profile
    if (layoutJson_.contains("sensorDefaults") && layoutJson_["sensorDefaults"].isObject()) {
        QJsonObject sDefaults = layoutJson_["sensorDefaults"].toObject();
        QString sIdxStr = QString::number(sensorIndex);
        if (sDefaults.contains(sIdxStr) && sDefaults[sIdxStr].isObject()) {
            QJsonObject layersMap = sDefaults[sIdxStr].toObject();
            QString layerStr = QString::number(currentLayer_);
            QJsonObject defObj;
            if (layersMap.contains(layerStr) && layersMap[layerStr].isObject()) {
                defObj = layersMap[layerStr].toObject();
            } else if (layersMap.contains("default") && layersMap["default"].isObject()) {
                defObj = layersMap["default"].toObject();
            }

            if (!defObj.isEmpty()) {
                return {{"hasData", true},
                        {"behavior", defObj.value("behavior").toString()},
                        {"cwLabel", defObj.value("cwLabel").toString("CW")},
                        {"ccwLabel", defObj.value("ccwLabel").toString("CCW")},
                        {"tooltip", defObj.value("tooltip").toString("Rotary Encoder")},
                        {"category", defObj.value("category").toString("media")},
                        {"pressLabel", pressLabel},
                        {"pressCategory", pressCategory},
                        {"pressTooltip", pressTooltip},
                        {"pressKeyPos", pressPos}};
            }
        }
    }

    return {{"hasData", false},
            {"behavior", ""},
            {"cwLabel", "CW"},
            {"ccwLabel", "CCW"},
            {"tooltip", "Rotary Encoder"},
            {"category", "misc"},
            {"pressLabel", pressLabel},
            {"pressCategory", pressCategory},
            {"pressTooltip", pressTooltip},
            {"pressKeyPos", pressPos}};
}

void KeymapModel::onLayerBindingsLoaded(int layer, int count) {
    Q_UNUSED(count);
    if (layer == currentLayer_) {
        populateKeys(layer);
    }
}

void KeymapModel::onKeymapLoaded(const QString &buildId, const QString &source, uint layerCount) {
    Q_UNUSED(buildId);
    Q_UNUSED(source);
    Q_UNUSED(layerCount);
    populateKeys(currentLayer_);
}

void KeymapModel::populateKeys(int layer) {
    isLoading_ = true;
    emit isLoadingChanged();

    QVector<KeyItem> newKeys;
    QVector<SensorItem> newSensors;

    QJsonObject keymapObj;
    if (client_) {
        keymapObj = client_->fetchKeymapJson(layer);
    }

    if (keymapObj.contains("bindings") && keymapObj["bindings"].isObject()) {
        QJsonObject bindingsObj = keymapObj["bindings"].toObject();
        QString layerKey = QString::number(layer);

        if (bindingsObj.contains(layerKey) && bindingsObj[layerKey].isArray()) {
            QJsonArray arr = bindingsObj[layerKey].toArray();
            for (const auto &val : arr) {
                if (val.isObject()) {
                    QJsonObject bObj = val.toObject();
                    KeyItem item;
                    item.position = bObj.value("pos").toInt();
                    item.behavior = bObj.value("behavior").toString();
                    item.param1 = static_cast<uint32_t>(bObj.value("param1").toInteger(0));
                    item.param2 = static_cast<uint32_t>(bObj.value("param2").toInteger(0));

                    if (bObj.contains("primaryLabel") || bObj.contains("category") ||
                        bObj.contains("tooltip")) {
                        item.primaryLabel = bObj.value("primaryLabel").toString();
                        item.secondaryLabel = bObj.value("secondaryLabel").toString();
                        item.tooltip = bObj.value("tooltip").toString();
                        item.category = bObj.value("category").toString("misc");
                    } else {
                        auto decoded = decoder::KeycodeDecoder::decode(item.behavior, item.param1,
                                                                       item.param2);
                        item.primaryLabel = decoded.primaryLabel;
                        item.secondaryLabel = decoded.secondaryLabel;
                        item.tooltip = decoded.tooltip;
                        item.category = decoded.category;
                    }

                    newKeys.append(item);
                }
            }
        }
    }

    if (keymapObj.contains("sensor_bindings") && keymapObj["sensor_bindings"].isObject()) {
        QJsonObject sBindingsObj = keymapObj["sensor_bindings"].toObject();
        QString layerKey = QString::number(layer);
        if (sBindingsObj.contains(layerKey) && sBindingsObj[layerKey].isArray()) {
            QJsonArray arr = sBindingsObj[layerKey].toArray();
            for (const auto &val : arr) {
                if (val.isObject()) {
                    QJsonObject sObj = val.toObject();
                    SensorItem item;
                    item.sensorIndex = sObj.value("sensor").toInt(0);
                    item.behavior = sObj.value("behavior").toString();
                    uint32_t p1 = static_cast<uint32_t>(sObj.value("param1").toInteger(0));
                    uint32_t p2 = static_cast<uint32_t>(sObj.value("param2").toInteger(0));
                    auto decoded = decoder::KeycodeDecoder::decodeSensor(item.behavior, p1, p2);
                    item.cwLabel = decoded.cwLabel;
                    item.ccwLabel = decoded.ccwLabel;
                    item.tooltip = decoded.tooltip;
                    item.category = decoded.category;
                    newSensors.append(item);
                }
            }
        }
    }

    // Sort keys by position
    std::sort(newKeys.begin(), newKeys.end(),
              [](const KeyItem &a, const KeyItem &b) { return a.position < b.position; });

    // If no keys parsed yet, create placeholder keys matching layout totalKeys
    if (newKeys.isEmpty()) {
        int total = layoutJson_.value("totalKeys").toInt(48);
        for (int i = 0; i < total; ++i) {
            KeyItem item;
            item.position = i;
            item.primaryLabel = QString::number(i);
            item.category = "misc";
            item.tooltip = QString("Key position %1").arg(i);
            newKeys.append(item);
        }
    }

    beginResetModel();
    keys_ = std::move(newKeys);
    sensors_ = std::move(newSensors);
    endResetModel();

    ++revision_;
    emit revisionChanged();
    isLoading_ = false;
    emit isLoadingChanged();
    emit totalKeysChanged();
    emit sensorDataChanged();
}

} // namespace strata::model
