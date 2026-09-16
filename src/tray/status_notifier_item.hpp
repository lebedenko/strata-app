#pragma once

#include <QDBusArgument>
#include <QDBusConnection>
#include <QDBusObjectPath>
#include <QIcon>
#include <QList>
#include <QObject>
#include <QString>

class QDBusServiceWatcher;

namespace strata::tray {

class DBusMenuService;

struct SniIconPixel {
    int width{0};
    int height{0};
    QByteArray data;
};
using SniIconPixmapList = QList<SniIconPixel>;

QDBusArgument &operator<<(QDBusArgument &arg, const SniIconPixel &pix);
const QDBusArgument &operator>>(const QDBusArgument &arg, SniIconPixel &pix);

QDBusArgument &operator<<(QDBusArgument &arg, const SniIconPixmapList &list);
const QDBusArgument &operator>>(const QDBusArgument &arg, SniIconPixmapList &list);

struct SniToolTip {
    QString icon_name;
    SniIconPixmapList icon_pixmap;
    QString title;
    QString description;
};

QDBusArgument &operator<<(QDBusArgument &arg, const SniToolTip &tip);
const QDBusArgument &operator>>(const QDBusArgument &arg, SniToolTip &tip);

class StatusNotifierItem : public QObject {
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.StatusNotifierItem")

    Q_PROPERTY(QString Category READ category)
    Q_PROPERTY(QString Id READ id)
    Q_PROPERTY(QString Title READ title)
    Q_PROPERTY(QString Status READ status)
    Q_PROPERTY(int WindowId READ windowId)
    Q_PROPERTY(QString IconThemePath READ iconThemePath)
    Q_PROPERTY(bool ItemIsMenu READ itemIsMenu)
    Q_PROPERTY(QDBusObjectPath Menu READ menu)
    Q_PROPERTY(QString IconName READ iconName)
    Q_PROPERTY(SniIconPixmapList IconPixmap READ iconPixmap)
    Q_PROPERTY(QString OverlayIconName READ overlayIconName)
    Q_PROPERTY(SniIconPixmapList OverlayIconPixmap READ overlayIconPixmap)
    Q_PROPERTY(QString AttentionIconName READ attentionIconName)
    Q_PROPERTY(SniIconPixmapList AttentionIconPixmap READ attentionIconPixmap)
    Q_PROPERTY(QString AttentionMovieName READ attentionMovieName)
    Q_PROPERTY(SniToolTip ToolTip READ toolTip)

public:
    explicit StatusNotifierItem(QObject *parent = nullptr);
    ~StatusNotifierItem() override;

    bool registerItem();
    void unregisterItem();
    [[nodiscard]] bool isRegistered() const { return registered_; }

    [[nodiscard]] QString category() const { return QStringLiteral("Hardware"); }
    [[nodiscard]] QString id() const { return QStringLiteral("strata"); }
    [[nodiscard]] QString title() const { return title_; }
    [[nodiscard]] QString status() const { return status_; }
    [[nodiscard]] int windowId() const { return 0; }
    [[nodiscard]] QString iconThemePath() const;
    [[nodiscard]] bool itemIsMenu() const { return false; }
    [[nodiscard]] QDBusObjectPath menu() const {
        return QDBusObjectPath(QStringLiteral("/MenuBar"));
    }
    [[nodiscard]] QString iconName() const { return QStringLiteral("strata"); }
    [[nodiscard]] SniIconPixmapList iconPixmap() const { return iconPixmaps_; }
    [[nodiscard]] QString overlayIconName() const { return {}; }
    [[nodiscard]] SniIconPixmapList overlayIconPixmap() const { return {}; }
    [[nodiscard]] QString attentionIconName() const { return {}; }
    [[nodiscard]] SniIconPixmapList attentionIconPixmap() const { return {}; }
    [[nodiscard]] QString attentionMovieName() const { return {}; }
    [[nodiscard]] SniToolTip toolTip() const { return toolTip_; }

    [[nodiscard]] DBusMenuService *menuService() const { return menuService_; }

    void setTitle(const QString &title);
    void setStatus(const QString &status);
    void setToolTip(const QString &title, const QString &description);
    void setIcon(const QIcon &icon);

public slots:
    void Activate(int x, int y);
    void SecondaryActivate(int x, int y);
    void ContextMenu(int x, int y);
    void Scroll(int delta, const QString &orientation);

signals:
    // D-Bus signals conforming to org.kde.StatusNotifierItem
    void NewTitle();
    void NewIcon();
    void NewAttentionIcon();
    void NewOverlayIcon();
    void NewMenu();
    void NewToolTip();
    void NewStatus(const QString &status);

    // C++ signals
    void activateRequested(int x, int y);
    void secondaryActivateRequested(int x, int y);
    void contextMenuRequested(int x, int y);
    void scrollRequested(int delta, const QString &orientation);

private slots:
    void onWatcherRegistered();

private:
    void registerWithWatcher();
    void renderPixmaps(const QIcon &icon);

    QString title_{QStringLiteral("Strata")};
    QString status_{QStringLiteral("Active")};
    SniToolTip toolTip_;
    SniIconPixmapList iconPixmaps_;
    QString serviceName_;
    bool registered_{false};
    QDBusServiceWatcher *watcherTracker_{nullptr};
    DBusMenuService *menuService_{nullptr};
};

} // namespace strata::tray

Q_DECLARE_METATYPE(strata::tray::SniIconPixel)
Q_DECLARE_METATYPE(strata::tray::SniIconPixmapList)
Q_DECLARE_METATYPE(strata::tray::SniToolTip)
