cmake_minimum_required(VERSION 3.25)
project(strata VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(ENABLE_SANITIZERS "Enable Address and Undefined sanitizers" OFF)

if(ENABLE_SANITIZERS)
    add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer)
    add_link_options(-fsanitize=address,undefined)
endif()

add_compile_options(
    -Wall
    -Wextra
    -Wpedantic
    -Wconversion
    -Wshadow
    -Wnon-virtual-dtor
)

find_package(Qt6 6.5 REQUIRED COMPONENTS
    Core
    Gui
    Quick
    Qml
    Widgets
    DBus
    Svg
)

qt_policy(SET QTP0001 NEW)
qt_policy(SET QTP0004 NEW)

find_package(PkgConfig REQUIRED)
pkg_check_modules(TOML REQUIRED IMPORTED_TARGET tomlplusplus)

qt_standard_project_setup()

qt_add_executable(strata
    src/main.cpp
    src/config/config_manager.hpp
    src/config/config_manager.cpp
    src/decoder/keycode_decoder.hpp
    src/decoder/keycode_decoder.cpp
    src/dbus/strata_dbus_client.hpp
    src/dbus/strata_dbus_client.cpp
    src/model/keymap_model.hpp
    src/model/keymap_model.cpp
    src/tray/tray_manager.hpp
    src/tray/tray_manager.cpp
    src/tray/status_notifier_item.hpp
    src/tray/status_notifier_item.cpp
    src/tray/dbus_menu.hpp
    src/tray/dbus_menu.cpp
    src/notification/notifier.hpp
    src/notification/notifier.cpp
)

qt_add_qml_module(strata
    URI Strata
    VERSION 1.0
    QML_FILES
        qml/Main.qml
        qml/components/KeyCap.qml
        qml/components/RotaryKnob.qml
        qml/components/SplitKeyboard.qml
        qml/components/LayerTabBar.qml
        qml/components/StatusBar.qml
    RESOURCES
        desktop/strata.svg
)

target_include_directories(strata PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(strata PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Quick
    Qt6::Qml
    Qt6::Widgets
    Qt6::DBus
    Qt6::Svg
    PkgConfig::TOML
)

# Testing
enable_testing()

add_executable(test_decoder
    tests/test_decoder.cpp
    src/decoder/keycode_decoder.cpp
)
target_include_directories(test_decoder PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)
target_link_libraries(test_decoder PRIVATE
    Qt6::Core
)
add_test(NAME test_decoder COMMAND test_decoder)

add_executable(test_config
    tests/test_config.cpp
    src/config/config_manager.cpp
)
target_include_directories(test_config PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)
target_link_libraries(test_config PRIVATE
    Qt6::Core
    PkgConfig::TOML
)
add_test(NAME test_config COMMAND test_config)

# Installation
include(GNUInstallDirs)

install(TARGETS strata
    DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(FILES desktop/org.freedesktop.Strata.desktop
    DESTINATION ${CMAKE_INSTALL_DATADIR}/applications
)

install(FILES desktop/strata.svg
    DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps
)
