#include "tray/dbus_menu.hpp"

#include <QDBusConnection>
#include <QLoggingCategory>

namespace strata::tray {

Q_LOGGING_CATEGORY(lcDBusMenu, "strata.tray.dbusmenu")

DBusMenuService::DBusMenuService(QObject *parent)
    : QObject(parent) {}

bool DBusMenuService::AboutToShow(int id) {
    Q_UNUSED(id);
    return false;
}

void DBusMenuService::GetLayout(int parentId, int recursionDepth,
                                const QStringList &propertyNames,
                                const QDBusMessage &message) {
    Q_UNUSED(parentId);
    Q_UNUSED(recursionDepth);
    Q_UNUSED(propertyNames);

    auto reply = message.createReply();
    const uint revision = 1;

    // Root layout struct: (id, properties, children)
    QDBusArgument rootArg;
    rootArg.beginStructure();
    rootArg << 0; // Root id is 0

    QVariantMap rootProps;
    rootProps[QStringLiteral("children-display")] = QStringLiteral("submenu");
    rootArg << rootProps;

    rootArg.beginArray(qMetaTypeId<QDBusVariant>());

    auto appendItem = [&](int id, const QString &label,
                          const QString &type = QStringLiteral("standard")) {
        QDBusArgument childArg;
        childArg.beginStructure();
        childArg << id;

        QVariantMap props;
        props[QStringLiteral("enabled")] = true;
        props[QStringLiteral("visible")] = true;
        props[QStringLiteral("type")] = type;
        if (!label.isEmpty()) {
            props[QStringLiteral("label")] = label;
        }

        childArg << props;
        childArg.beginArray(qMetaTypeId<QDBusVariant>());
        childArg.endArray();
        childArg.endStructure();

        rootArg << QDBusVariant(QVariant::fromValue(childArg));
    };

    appendItem(1, tr("Open Visualizer"));
    appendItem(2, QString{}, QStringLiteral("separator"));
    appendItem(3, tr("Refresh Keymap"));
    appendItem(4, tr("Clear Cache"));
    appendItem(5, QString{}, QStringLiteral("separator"));
    appendItem(6, tr("Quit"));

    rootArg.endArray();
    rootArg.endStructure();

    reply << revision << QVariant::fromValue(rootArg);
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
