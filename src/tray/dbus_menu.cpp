#include "tray/dbus_menu.hpp"

#include <QDBusConnection>
#include <QDBusMetaType>
#include <QLoggingCategory>

namespace strata::tray {

Q_LOGGING_CATEGORY(lcDBusMenu, "strata.tray.dbusmenu")

QDBusArgument &operator<<(QDBusArgument &arg, const DBusMenuItem &item) {
    arg.beginStructure();
    arg << item.id;
    arg << item.properties;
    arg.beginArray(qMetaTypeId<QDBusVariant>());
    for (const auto &child : item.children) {
        arg << child;
    }
    arg.endArray();
    arg.endStructure();
    return arg;
}

const QDBusArgument &operator>>(const QDBusArgument &arg, DBusMenuItem &item) {
    arg.beginStructure();
    arg >> item.id;
    arg >> item.properties;
    arg.beginArray();
    item.children.clear();
    while (!arg.atEnd()) {
        QDBusVariant child;
        arg >> child;
        item.children.append(child);
    }
    arg.endArray();
    arg.endStructure();
    return arg;
}

DBusMenuService::DBusMenuService(QObject *parent)
    : QObject(parent) {
    qDBusRegisterMetaType<DBusMenuItem>();
}

bool DBusMenuService::AboutToShow(int id) {
    Q_UNUSED(id);
    return false;
}

void DBusMenuService::GetLayout(int parentId, int recursionDepth, const QStringList &propertyNames,
                                const QDBusMessage &message) {
    Q_UNUSED(parentId);
    Q_UNUSED(recursionDepth);
    Q_UNUSED(propertyNames);

    auto reply = message.createReply();
    const uint revision = 1;

    DBusMenuItem rootItem;
    rootItem.id = 0;
    rootItem.properties[QStringLiteral("children-display")] = QStringLiteral("submenu");

    auto appendItem = [&](int id, const QString &label,
                          const QString &type = QStringLiteral("standard")) {
        DBusMenuItem childItem;
        childItem.id = id;
        childItem.properties[QStringLiteral("enabled")] = true;
        childItem.properties[QStringLiteral("visible")] = true;
        childItem.properties[QStringLiteral("type")] = type;
        if (!label.isEmpty()) {
            childItem.properties[QStringLiteral("label")] = label;
        }

        rootItem.children.append(QDBusVariant(QVariant::fromValue(childItem)));
    };

    appendItem(1, tr("Open Visualizer"));
    appendItem(2, QString{}, QStringLiteral("separator"));
    appendItem(3, tr("Refresh Keymap"));
    appendItem(4, tr("Clear Cache"));
    appendItem(5, QString{}, QStringLiteral("separator"));
    appendItem(6, tr("Quit"));

    reply << revision << QVariant::fromValue(rootItem);
    QDBusConnection::sessionBus().send(reply);
}

void DBusMenuService::Event(int id, const QString &eventId, const QDBusVariant &data,
                            uint timestamp) {
    Q_UNUSED(data);
    Q_UNUSED(timestamp);

    qCInfo(lcDBusMenu) << "DBusMenu Event received:" << id << eventId;

    if (eventId == QLatin1String("clicked")) {
        switch (id) {
        case 1:
            emit openVisualizerRequested();
            break;
        case 3:
            emit refreshKeymapRequested();
            break;
        case 4:
            emit clearCacheRequested();
            break;
        case 6:
            emit quitRequested();
            break;
        default:
            break;
        }
    }
}

} // namespace strata::tray
