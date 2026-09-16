#include "model/keymap_model.hpp"

#include <QJsonArray>
#include <QJsonObject>
#include <algorithm>

namespace strata::model {

KeymapModel::KeymapModel(dbus::StrataDBusClient *client, QObject *parent)
    : QAbstractListModel(parent)
    , client_(client) {
    if (client_) {
        connect(client_, &dbus::StrataDBusClient::selectedLayerChanged, this,
                [this]() { setCurrentLayer(client_->selectedLayerIndex()); });

        connect(client_, &dbus::StrataDBusClient::layerBindingsLoaded, this,
                &KeymapModel::onLayerBindingsLoaded);

        connect(client_, &dbus::StrataDBusClient::keymapLoaded, this, &KeymapModel::onKeymapLoaded);

        currentLayer_ = client_->selectedLayerIndex();
        populateKeys(currentLayer_);
    }
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
    if (!client_)
        return;

    isLoading_ = true;
    emit isLoadingChanged();

    QJsonObject keymapObj = client_->fetchKeymapJson(layer);
    QVector<KeyItem> newKeys;

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

                    auto decoded =
                        decoder::KeycodeDecoder::decode(item.behavior, item.param1, item.param2);
                    item.primaryLabel = decoded.primaryLabel;
                    item.secondaryLabel = decoded.secondaryLabel;
                    item.tooltip = decoded.tooltip;
                    item.category = decoded.category;

                    newKeys.append(item);
                }
            }
        }
    }

    // Sort keys by position
    std::sort(newKeys.begin(), newKeys.end(),
              [](const KeyItem &a, const KeyItem &b) { return a.position < b.position; });

    // If no keys parsed yet, create 48 placeholder keys
    if (newKeys.isEmpty()) {
        for (int i = 0; i < 48; ++i) {
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
    endResetModel();

    isLoading_ = false;
    emit isLoadingChanged();
    emit totalKeysChanged();
}

} // namespace strata::model
