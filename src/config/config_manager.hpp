#pragma once

#include <QObject>
#include <QString>
#include <filesystem>

namespace strata::config {

struct AppConfig {
    bool startMinimized{false};
    bool showTrayIcon{true};
    bool notificationsEnabled{true};
    bool notifyLayerChanges{true};
    int notificationTimeoutMs{1500};
    int windowWidth{980};
    int windowHeight{520};
    bool showKeyPositions{false};
};

class ConfigManager : public QObject {
    Q_OBJECT

    Q_PROPERTY(bool startMinimized READ startMinimized WRITE setStartMinimized NOTIFY configChanged)
    Q_PROPERTY(bool showTrayIcon READ showTrayIcon WRITE setShowTrayIcon NOTIFY configChanged)
    Q_PROPERTY(bool notificationsEnabled READ notificationsEnabled WRITE setNotificationsEnabled
                   NOTIFY configChanged)
    Q_PROPERTY(bool notifyLayerChanges READ notifyLayerChanges WRITE setNotifyLayerChanges NOTIFY
                   configChanged)
    Q_PROPERTY(int notificationTimeoutMs READ notificationTimeoutMs WRITE setNotificationTimeoutMs
                   NOTIFY configChanged)
    Q_PROPERTY(int windowWidth READ windowWidth WRITE setWindowWidth NOTIFY configChanged)
    Q_PROPERTY(int windowHeight READ windowHeight WRITE setWindowHeight NOTIFY configChanged)
    Q_PROPERTY(
        bool showKeyPositions READ showKeyPositions WRITE setShowKeyPositions NOTIFY configChanged)

public:
    explicit ConfigManager(QObject *parent = nullptr);

    bool load();
    bool save();

    [[nodiscard]] const AppConfig &config() const noexcept { return config_; }

    [[nodiscard]] bool startMinimized() const noexcept { return config_.startMinimized; }
    void setStartMinimized(bool val);

    [[nodiscard]] bool showTrayIcon() const noexcept { return config_.showTrayIcon; }
    void setShowTrayIcon(bool val);

    [[nodiscard]] bool notificationsEnabled() const noexcept {
        return config_.notificationsEnabled;
    }
    void setNotificationsEnabled(bool val);

    [[nodiscard]] bool notifyLayerChanges() const noexcept { return config_.notifyLayerChanges; }
    void setNotifyLayerChanges(bool val);

    [[nodiscard]] int notificationTimeoutMs() const noexcept {
        return config_.notificationTimeoutMs;
    }
    void setNotificationTimeoutMs(int val);

    [[nodiscard]] int windowWidth() const noexcept { return config_.windowWidth; }
    void setWindowWidth(int val);

    [[nodiscard]] int windowHeight() const noexcept { return config_.windowHeight; }
    void setWindowHeight(int val);

    [[nodiscard]] bool showKeyPositions() const noexcept { return config_.showKeyPositions; }
    void setShowKeyPositions(bool val);

signals:
    void configChanged();

private:
    AppConfig config_;
    std::filesystem::path configPath_;

    static std::filesystem::path resolveConfigPath();
};

} // namespace strata::config
