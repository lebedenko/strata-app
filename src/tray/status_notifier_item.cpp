#include "tray/status_notifier_item.hpp"

#include <QCoreApplication>
#include <QDBusInterface>
#include <QDBusMetaType>
#include <QDBusServiceWatcher>
#include <QDir>
#include <QImage>
#include <QLoggingCategory>
#include <QPixmap>
#include <QRandomGenerator>

#include "tray/dbus_menu.hpp"

namespace strata::tray {

Q_LOGGING_CATEGORY(lcSni, "strata.tray.sni")

QDBusArgument &operator<<(QDBusArgument &arg, const SniIconPixel &pix) {
    arg.beginStructure();
    arg << pix.width;
    arg << pix.height;
    arg << pix.data;
    arg.endStructure();
    return arg;
}

const QDBusArgument &operator>>(const QDBusArgument &arg, SniIconPixel &pix) {
    arg.beginStructure();
    arg >> pix.width;
    arg >> pix.height;
    arg >> pix.data;
    arg.endStructure();
    return arg;
}

QDBusArgument &operator<<(QDBusArgument &arg, const SniIconPixmapList &list) {
    arg.beginArray(qMetaTypeId<SniIconPixel>());
    for (const auto &item : list) {
        arg << item;
    }
    arg.endArray();
    return arg;
}

const QDBusArgument &operator>>(const QDBusArgument &arg, SniIconPixmapList &list) {
    arg.beginArray();
    list.clear();
    while (!arg.atEnd()) {
        SniIconPixel item;
        arg >> item;
        list.append(item);
    }
    arg.endArray();
    return arg;
}

QDBusArgument &operator<<(QDBusArgument &arg, const SniToolTip &tip) {
    arg.beginStructure();
    arg << tip.icon_name;
    arg << tip.icon_pixmap;
    arg << tip.title;
    arg << tip.description;
    arg.endStructure();
    return arg;
}

const QDBusArgument &operator>>(const QDBusArgument &arg, SniToolTip &tip) {
    arg.beginStructure();
    arg >> tip.icon_name;
    arg >> tip.icon_pixmap;
    arg >> tip.title;
    arg >> tip.description;
    arg.endStructure();
    return arg;
}

static void registerTrayMetaTypes() {
    static bool registered = false;
    if (registered)
        return;
    qDBusRegisterMetaType<SniIconPixel>();
    qDBusRegisterMetaType<SniIconPixmapList>();
    qDBusRegisterMetaType<SniToolTip>();
    registered = true;
}

static SniIconPixel imageToSniPixel(const QImage &src) {
    QImage image = src.convertToFormat(QImage::Format_ARGB32);
    SniIconPixel pix;
    pix.width = image.width();
    pix.height = image.height();
    pix.data.resize(static_cast<qsizetype>(pix.width * pix.height * 4));

    auto *dest = reinterpret_cast<uchar *>(pix.data.data());
    for (int y = 0; y < image.height(); ++y) {
        const auto *scanline = reinterpret_cast<const QRgb *>(image.constScanLine(y));
        for (int x = 0; x < image.width(); ++x) {
            QRgb pixel = scanline[x];
            // Network byte order (big endian): Alpha, Red, Green, Blue
            *dest++ = static_cast<uchar>(qAlpha(pixel));
            *dest++ = static_cast<uchar>(qRed(pixel));
            *dest++ = static_cast<uchar>(qGreen(pixel));
            *dest++ = static_cast<uchar>(qBlue(pixel));
        }
    }
    return pix;
}

StatusNotifierItem::StatusNotifierItem(QObject *parent)
    : QObject(parent) {
    registerTrayMetaTypes();

    toolTip_.icon_name = QString{};
    toolTip_.title = QStringLiteral("Strata");
    toolTip_.description = QStringLiteral("Hardware Visualizer");

    renderPixmaps(QIcon(QStringLiteral(":/qt/qml/Strata/desktop/strata.svg")));

    menuService_ = new DBusMenuService(this);
}

StatusNotifierItem::~StatusNotifierItem() {
    unregisterItem();
}

void StatusNotifierItem::renderPixmaps(const QIcon &icon) {
    iconPixmaps_.clear();
    const QList<int> sizes = {16, 22, 24, 32, 48, 64};
    for (int size : sizes) {
        QPixmap pm = icon.pixmap(size, size);
        if (!pm.isNull()) {
            iconPixmaps_.append(imageToSniPixel(pm.toImage()));
        }
    }
}

QString StatusNotifierItem::iconThemePath() const {
    return QDir::homePath() + QStringLiteral("/.local/share/icons");
}

bool StatusNotifierItem::registerItem() {
    if (registered_)
        return true;

    auto bus = QDBusConnection::sessionBus();
    if (!bus.isConnected()) {
        qCWarning(lcSni) << "Session D-Bus is not connected; cannot register StatusNotifierItem";
        return false;
    }

    serviceName_ = QStringLiteral("org.kde.StatusNotifierItem-%1-1")
                       .arg(QCoreApplication::applicationPid());
    if (!bus.registerService(serviceName_)) {
        serviceName_ = QStringLiteral("org.kde.StatusNotifierItem-%1-%2")
                           .arg(QCoreApplication::applicationPid())
                           .arg(QRandomGenerator::global()->generate());
        if (!bus.registerService(serviceName_)) {
            qCWarning(lcSni) << "Failed to register D-Bus service" << serviceName_;
            return false;
        }
    }

    if (!bus.registerObject(QStringLiteral("/StatusNotifierItem"), this,
                            QDBusConnection::ExportAllContents)) {
        qCWarning(lcSni) << "Failed to export /StatusNotifierItem on D-Bus";
        bus.unregisterService(serviceName_);
        return false;
    }

    if (menuService_) {
        if (!bus.registerObject(QStringLiteral("/MenuBar"), menuService_,
                                QDBusConnection::ExportAllContents)) {
            qCWarning(lcSni) << "Failed to export /MenuBar on D-Bus";
        }
    }

    registered_ = true;

    if (!watcherTracker_) {
        watcherTracker_ = new QDBusServiceWatcher(
            QStringLiteral("org.kde.StatusNotifierWatcher"),
            bus,
            QDBusServiceWatcher::WatchForRegistration,
            this);
        connect(watcherTracker_, &QDBusServiceWatcher::serviceRegistered,
                this, &StatusNotifierItem::onWatcherRegistered);
    }

    registerWithWatcher();
    return true;
}

void StatusNotifierItem::unregisterItem() {
    if (!registered_)
        return;

    auto bus = QDBusConnection::sessionBus();
    bus.unregisterObject(QStringLiteral("/MenuBar"));
    bus.unregisterObject(QStringLiteral("/StatusNotifierItem"));
    bus.unregisterService(serviceName_);
    registered_ = false;
}

void StatusNotifierItem::registerWithWatcher() {
    auto bus = QDBusConnection::sessionBus();
    QDBusInterface watcher(
        QStringLiteral("org.kde.StatusNotifierWatcher"),
        QStringLiteral("/StatusNotifierWatcher"),
        QStringLiteral("org.kde.StatusNotifierWatcher"),
        bus);

    if (watcher.isValid()) {
        qCInfo(lcSni) << "Registering StatusNotifierItem" << serviceName_ << "with StatusNotifierWatcher";
        watcher.asyncCall(QStringLiteral("RegisterStatusNotifierItem"), serviceName_);
    } else {
        qCInfo(lcSni) << "StatusNotifierWatcher is not currently registered; will wait for it";
    }
}

void StatusNotifierItem::onWatcherRegistered() {
    qCInfo(lcSni) << "StatusNotifierWatcher registered on D-Bus; re-registering item";
    registerWithWatcher();
}

void StatusNotifierItem::setTitle(const QString &title) {
    if (title_ != title) {
        title_ = title;
        emit NewTitle();
    }
}

void StatusNotifierItem::setStatus(const QString &status) {
    if (status_ != status) {
        status_ = status;
        emit NewStatus(status_);
    }
}

void StatusNotifierItem::setToolTip(const QString &title, const QString &description) {
    if (toolTip_.title != title || toolTip_.description != description) {
        toolTip_.title = title;
        toolTip_.description = description;
        emit NewToolTip();
    }
}

void StatusNotifierItem::setIcon(const QIcon &icon) {
    renderPixmaps(icon);
    emit NewIcon();
}

void StatusNotifierItem::Activate(int x, int y) {
    qCInfo(lcSni) << "Activate received:" << x << y;
    emit activateRequested(x, y);
}

void StatusNotifierItem::SecondaryActivate(int x, int y) {
    qCInfo(lcSni) << "SecondaryActivate received:" << x << y;
    emit secondaryActivateRequested(x, y);
}

void StatusNotifierItem::ContextMenu(int x, int y) {
    qCInfo(lcSni) << "ContextMenu received:" << x << y;
    emit contextMenuRequested(x, y);
}

void StatusNotifierItem::Scroll(int delta, const QString &orientation) {
    qCInfo(lcSni) << "Scroll received:" << delta << orientation;
    emit scrollRequested(delta, orientation);
}

} // namespace strata::tray
