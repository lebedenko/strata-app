#include "config/config_manager.hpp"

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <toml++/toml.hpp>

namespace strata::config {

namespace fs = std::filesystem;

ConfigManager::ConfigManager(QObject *parent)
    : QObject(parent)
    , configPath_(resolveConfigPath()) {
    load();
}

fs::path ConfigManager::resolveConfigPath() {
    if (const char *xdg = std::getenv("XDG_CONFIG_HOME"); xdg && *xdg) {
        return fs::path(xdg) / "strata" / "config.toml";
    }
    if (const char *home = std::getenv("HOME"); home && *home) {
        return fs::path(home) / ".config" / "strata" / "config.toml";
    }
    return fs::temp_directory_path() / "strata" / "config.toml";
}

bool ConfigManager::load() {
    if (!fs::exists(configPath_)) {
        save(); // Save defaults if no config exists yet
        return true;
    }

    try {
        toml::table tbl = toml::parse_file(configPath_.string());

        if (auto general = tbl["general"].as_table()) {
            config_.startMinimized = (*general)["start_minimized"].value_or(config_.startMinimized);
            config_.showTrayIcon = (*general)["show_tray_icon"].value_or(config_.showTrayIcon);
        }

        if (auto notif = tbl["notifications"].as_table()) {
            config_.notificationsEnabled =
                (*notif)["enabled"].value_or(config_.notificationsEnabled);
            config_.notifyLayerChanges =
                (*notif)["show_layer_changes"].value_or(config_.notifyLayerChanges);
            config_.notificationTimeoutMs =
                (*notif)["timeout_ms"].value_or(config_.notificationTimeoutMs);
        }

        if (auto vis = tbl["visualizer"].as_table()) {
            config_.windowWidth = (*vis)["window_width"].value_or(config_.windowWidth);
            config_.windowHeight = (*vis)["window_height"].value_or(config_.windowHeight);
            config_.showKeyPositions =
                (*vis)["show_key_positions"].value_or(config_.showKeyPositions);
        }

        emit configChanged();
        return true;
    } catch (const toml::parse_error &err) {
        std::cerr << "Error parsing config " << configPath_ << ": " << err.description() << "\n";
        return false;
    }
}

bool ConfigManager::save() {
    std::error_code ec;
    fs::create_directories(configPath_.parent_path(), ec);
    if (ec) {
        std::cerr << "Failed to create directory " << configPath_.parent_path() << ": "
                  << ec.message() << "\n";
        return false;
    }

    toml::table tbl{
        {"general", toml::table{{"start_minimized", config_.startMinimized},
                                {"show_tray_icon", config_.showTrayIcon}}},
        {"notifications", toml::table{{"enabled", config_.notificationsEnabled},
                                      {"show_layer_changes", config_.notifyLayerChanges},
                                      {"timeout_ms", config_.notificationTimeoutMs}}},
        {"visualizer", toml::table{{"window_width", config_.windowWidth},
                                   {"window_height", config_.windowHeight},
                                   {"show_key_positions", config_.showKeyPositions}}},
    };

    std::ofstream out(configPath_);
    if (!out.is_open()) {
        std::cerr << "Failed to open config file for writing: " << configPath_ << "\n";
        return false;
    }

    out << tbl << "\n";
    return true;
}

void ConfigManager::setStartMinimized(bool val) {
    if (config_.startMinimized != val) {
        config_.startMinimized = val;
        emit configChanged();
        save();
    }
}

void ConfigManager::setShowTrayIcon(bool val) {
    if (config_.showTrayIcon != val) {
        config_.showTrayIcon = val;
        emit configChanged();
        save();
    }
}

void ConfigManager::setNotificationsEnabled(bool val) {
    if (config_.notificationsEnabled != val) {
        config_.notificationsEnabled = val;
        emit configChanged();
        save();
    }
}

void ConfigManager::setNotifyLayerChanges(bool val) {
    if (config_.notifyLayerChanges != val) {
        config_.notifyLayerChanges = val;
        emit configChanged();
        save();
    }
}

void ConfigManager::setNotificationTimeoutMs(int val) {
    if (config_.notificationTimeoutMs != val) {
        config_.notificationTimeoutMs = val;
        emit configChanged();
        save();
    }
}

void ConfigManager::setWindowWidth(int val) {
    if (config_.windowWidth != val) {
        config_.windowWidth = val;
        emit configChanged();
        save();
    }
}

void ConfigManager::setWindowHeight(int val) {
    if (config_.windowHeight != val) {
        config_.windowHeight = val;
        emit configChanged();
        save();
    }
}

void ConfigManager::setShowKeyPositions(bool val) {
    if (config_.showKeyPositions != val) {
        config_.showKeyPositions = val;
        emit configChanged();
        save();
    }
}

} // namespace strata::config
