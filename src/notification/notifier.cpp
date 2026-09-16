#include "notification/notifier.hpp"

#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusMessage>
#include <QDBusReply>
#include <QStringList>
#include <QVariantMap>

namespace strata::notification {

Notifier::Notifier(dbus::StrataDBusClient *client, config::ConfigManager *configMgr,
                   QObject *parent)
    : QObject(parent)
    , client_(client)
    , configMgr_(configMgr) {
    if (client_) {
        connect(client_, &dbus::StrataDBusClient::activeLayerChanged, this,
                &Notifier::onLayerChanged);
        connect(client_, &dbus::StrataDBusClient::deviceConnected, this,
                &Notifier::onDeviceConnected);
        connect(client_, &dbus::StrataDBusClient::deviceDisconnected, this,
                &Notifier::onDeviceDisconnected);
    }
}

void Notifier::notify(const QString &summary, const QString &body, int timeoutMs) {
    if (!configMgr_ || !configMgr_->notificationsEnabled()) {
        return;
    }

    QDBusInterface notifyIface("org.freedesktop.Notifications", "/org/freedesktop/Notifications",
                               "org.freedesktop.Notifications", QDBusConnection::sessionBus());
    if (!notifyIface.isValid()) {
        return;
    }

    QVariantMap hints;
    hints["urgency"] = static_cast<uchar>(1);
    hints["category"] = "device";

    int timeout = (timeoutMs >= 0) ? timeoutMs : configMgr_->notificationTimeoutMs();

    QList<QVariant> args;
    args << "Strata" << lastNotificationId_ << "strata" << summary << body << QStringList() << hints
         << timeout;

    QDBusMessage reply = notifyIface.callWithArgumentList(QDBus::AutoDetect, "Notify", args);
    if (reply.type() == QDBusMessage::ReplyMessage && !reply.arguments().isEmpty()) {
        lastNotificationId_ = reply.arguments().at(0).toUInt();
    }
}

void Notifier::onLayerChanged(int index, const QString &name, uint mask) {
    Q_UNUSED(mask);
    if (!configMgr_ || !configMgr_->notifyLayerChanges()) {
        return;
    }

    notify(tr("Layer: %1").arg(name), tr("Layer %1 active").arg(index),
           configMgr_->notificationTimeoutMs());
}

void Notifier::onDeviceConnected(const QString &name, const QString &node, const QString &buildId) {
    Q_UNUSED(node);
    Q_UNUSED(buildId);
    notify(tr("Keyboard Connected"), tr("%1 is now active").arg(name), 3000);
}

void Notifier::onDeviceDisconnected(const QString &node) {
    Q_UNUSED(node);
    notify(tr("Keyboard Disconnected"), tr("Device was detached"), 3000);
}

} // namespace strata::notification
