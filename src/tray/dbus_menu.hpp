#pragma once

#include <QDBusArgument>
#include <QDBusMessage>
#include <QDBusVariant>
#include <QObject>
#include <QStringList>

namespace strata::tray {

class DBusMenuService : public QObject {
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "com.canonical.dbusmenu")
    Q_CLASSINFO("D-Bus Introspection",
        "  <interface name=\"com.canonical.dbusmenu\">\n"
        "    <method name=\"GetLayout\">\n"
        "      <arg direction=\"in\" type=\"i\" name=\"parentId\"/>\n"
        "      <arg direction=\"in\" type=\"i\" name=\"recursionDepth\"/>\n"
        "      <arg direction=\"in\" type=\"as\" name=\"propertyNames\"/>\n"
        "      <arg direction=\"out\" type=\"u\" name=\"revision\"/>\n"
        "      <arg direction=\"out\" type=\"(ia{sv}av)\" name=\"layout\"/>\n"
        "    </method>\n"
        "    <method name=\"AboutToShow\">\n"
        "      <arg direction=\"in\" type=\"i\" name=\"id\"/>\n"
        "      <arg direction=\"out\" type=\"b\"/>\n"
        "    </method>\n"
        "    <method name=\"Event\">\n"
        "      <arg direction=\"in\" type=\"i\" name=\"id\"/>\n"
        "      <arg direction=\"in\" type=\"s\" name=\"eventId\"/>\n"
        "      <arg direction=\"in\" type=\"v\" name=\"data\"/>\n"
        "      <arg direction=\"in\" type=\"u\" name=\"timestamp\"/>\n"
        "    </method>\n"
        "    <property type=\"u\" access=\"read\" name=\"Version\"/>\n"
        "    <property type=\"s\" access=\"read\" name=\"TextDirection\"/>\n"
        "    <property type=\"s\" access=\"read\" name=\"Status\"/>\n"
        "    <property type=\"as\" access=\"read\" name=\"IconThemePath\"/>\n"
        "  </interface>\n")

    Q_PROPERTY(uint Version READ version)
    Q_PROPERTY(QString TextDirection READ textDirection)
    Q_PROPERTY(QString Status READ status)
    Q_PROPERTY(QStringList IconThemePath READ iconThemePath)

public:
    explicit DBusMenuService(QObject *parent = nullptr);
    ~DBusMenuService() override = default;

    [[nodiscard]] uint version() const { return 4; }
    [[nodiscard]] QString textDirection() const { return QStringLiteral("ltr"); }
    [[nodiscard]] QString status() const { return QStringLiteral("normal"); }
    [[nodiscard]] QStringList iconThemePath() const { return {}; }

public slots:
    void GetLayout(int parentId, int recursionDepth, const QStringList &propertyNames,
                   const QDBusMessage &message);
    bool AboutToShow(int id);
    void Event(int id, const QString &eventId, const QDBusVariant &data, uint timestamp);

signals:
    void LayoutUpdated(uint revision, int parentId);
    void ItemActivationRequested(int id, uint timestamp);

    void openVisualizerRequested();
    void refreshKeymapRequested();
    void clearCacheRequested();
    void quitRequested();
};

} // namespace strata::tray
