#pragma once

#include <QAbstractListModel>
#include <QVector>

#include "dbus/strata_dbus_client.hpp"
#include "decoder/keycode_decoder.hpp"

namespace strata::model {

struct KeyItem {
    int position{0};
    QString behavior;
    QString primaryLabel;
    QString secondaryLabel;
    QString tooltip;
    QString category;
    uint32_t param1{0};
    uint32_t param2{0};
};

struct SensorItem {
    int sensorIndex{0};
    QString behavior;
    QString cwLabel;
    QString ccwLabel;
    QString tooltip;
    QString category;
};

class KeymapModel : public QAbstractListModel {
    Q_OBJECT

    Q_PROPERTY(int currentLayer READ currentLayer WRITE setCurrentLayer NOTIFY currentLayerChanged)
    Q_PROPERTY(int totalKeys READ totalKeys NOTIFY totalKeysChanged)
    Q_PROPERTY(bool isLoading READ isLoading NOTIFY isLoadingChanged)

public:
    enum KeyRoles {
        PositionRole = Qt::UserRole + 1,
        BehaviorRole,
        PrimaryLabelRole,
        SecondaryLabelRole,
        TooltipRole,
        CategoryRole,
        Param1Role,
        Param2Role,
    };

    explicit KeymapModel(dbus::StrataDBusClient *client, QObject *parent = nullptr);

    [[nodiscard]] int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    [[nodiscard]] QVariant data(const QModelIndex &index,
                                int role = Qt::DisplayRole) const override;
    [[nodiscard]] QHash<int, QByteArray> roleNames() const override;

    [[nodiscard]] int currentLayer() const noexcept { return currentLayer_; }
    void setCurrentLayer(int layer);

    [[nodiscard]] int totalKeys() const noexcept { return static_cast<int>(keys_.size()); }
    [[nodiscard]] bool isLoading() const noexcept { return isLoading_; }

    Q_INVOKABLE QVariantMap getKeyData(int position) const;
    Q_INVOKABLE QVariantMap getSensorData(int sensorIndex) const;
    Q_INVOKABLE void reload();

signals:
    void currentLayerChanged();
    void totalKeysChanged();
    void isLoadingChanged();
    void sensorDataChanged();

private slots:
    void onLayerBindingsLoaded(int layer, int count);
    void onKeymapLoaded(const QString &buildId, const QString &source, uint layerCount);

private:
    void populateKeys(int layer);

    dbus::StrataDBusClient *client_{nullptr};
    int currentLayer_{0};
    bool isLoading_{false};
    QVector<KeyItem> keys_;
    QVector<SensorItem> sensors_;
};

} // namespace strata::model
