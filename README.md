# Strata Desktop Companion Application

**Strata** is the modern Wayland-native desktop companion application and live layer visualizer for ergonomic mechanical keyboards (ZMK Eyelash Corne, QMK ZSA Voyager).

Built with **C++23**, **Qt 6**, and **QML**, communicating over the user session bus with the [`stratad`](https://github.com/lebedenko/strata) daemon.

---

## Features

- **Live Layer Visualizer**: Real-time layer switching indicator and interactive keycap map highlighting active and secondary behaviors (Home-Row Mods, Layer Taps, Media, Navigation).
- **Authentic Split Layout**: Column-staggered geometry for Eyelash Corne (6×3 split + 3 thumb keys per side + center 5-way navigation cluster).
- **Wayland Native & Dark/Light Adaptive**: Automatically adapts to system dark or light palette using standard Qt theming (no hardcoded styles).
- **StatusNotifierItem Tray Icon**: Seamlessly integrates into Waybar, Sway, Hyprland, and KDE status trays.
- **Desktop Notifications**: Instant, unobtrusive toast notifications on layer change and device attach/detach via `org.freedesktop.Notifications`.
- **User Configuration**: Settings persisted in `~/.config/strata/config.toml` via `tomlplusplus`.
- **Minimized Autostart**: Supports launching with `--minimized` on user login.

---

## Build & Installation

### Requirements
- Arch Linux / Linux with Wayland
- C++23 compliant compiler (GCC 14+ / Clang 18+)
- CMake 3.25+, Ninja
- Qt 6 (Core, Gui, Quick, Qml, Widgets, DBus, Svg)
- `tomlplusplus`
- `task` (Taskfile runner)

### Tasks
```bash
# 1. Build release binaries
task build

# 2. Run unit test suite
task test

# 3. Install binary and desktop launcher
task install

# 4. Launch visualizer
strata

# Or start directly to tray
strata --minimized
```

---

## Configuration (`~/.config/strata/config.toml`)

```toml
[general]
start_minimized = false
show_tray_icon = true

[notifications]
enabled = true
show_layer_changes = true
timeout_ms = 1500

[visualizer]
window_width = 980
window_height = 520
show_key_positions = false
```
