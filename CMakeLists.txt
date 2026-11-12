cmake_minimum_required(VERSION 3.25)
project(
    strata
    VERSION 1.0.0
    LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(ENABLE_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" OFF)
if(ENABLE_WARNINGS_AS_ERRORS)
    add_compile_options(-Werror)
endif()

option(ENABLE_COVERAGE "Enable LLVM coverage" OFF)
option(ENABLE_FUZZING "Build decoder libFuzzer harness" OFF)

if(ENABLE_COVERAGE OR ENABLE_FUZZING)
    if(NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        message(FATAL_ERROR "Coverage and fuzzing require Clang")
    endif()
endif()
if(ENABLE_COVERAGE AND (ENABLE_SANITIZERS OR ENABLE_FUZZING))
    message(FATAL_ERROR "Coverage must use an isolated build")
endif()
if(ENABLE_COVERAGE)
    add_compile_options(-fprofile-instr-generate -fcoverage-mapping)
    add_link_options(-fprofile-instr-generate)
endif()

option(ENABLE_SANITIZERS "Enable Address and Undefined sanitizers" OFF)

if(ENABLE_SANITIZERS OR ENABLE_FUZZING)
    add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer)
    add_link_options(-fsanitize=address,undefined)
endif()

add_compile_options(-Wall -Wextra -Wpedantic -Wconversion -Wshadow -Wnon-virtual-dtor)

find_package(
    Qt6 6.5 REQUIRED
    COMPONENTS Core
               Gui
               Quick
               Qml
               Widgets
               DBus
               Svg)

qt_policy(SET QTP0001 NEW)
qt_policy(SET QTP0004 NEW)

find_package(PkgConfig REQUIRED)
pkg_check_modules(TOML REQUIRED IMPORTED_TARGET tomlplusplus)

qt_standard_project_setup()

qt_add_executable(
    strata
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
    src/notification/notifier.cpp)

qt_add_qml_module(
    strata
    URI
    Strata
    VERSION
    1.0
    QML_FILES
    qml/Main.qml
    qml/components/KeyCap.qml
    qml/components/RotaryKnob.qml
    qml/components/SplitKeyboard.qml
    qml/components/MacropadKeyboard.qml
    qml/components/LayerTabBar.qml
    qml/components/StatusBar.qml
    RESOURCES
    desktop/strata.svg
    resources/layouts/eyelash_corne.json
    resources/layouts/voyager.json
    resources/layouts/twindial25.json)

target_include_directories(strata PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_link_libraries(
    strata
    PRIVATE Qt6::Core
            Qt6::Gui
            Qt6::Quick
            Qt6::Qml
            Qt6::Widgets
            Qt6::DBus
            Qt6::Svg
            PkgConfig::TOML)

# Testing
include(CTest)

add_executable(test_decoder tests/test_decoder.cpp src/decoder/keycode_decoder.cpp)
target_include_directories(test_decoder PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(test_decoder PRIVATE Qt6::Core)
add_test(NAME test_decoder COMMAND test_decoder)

add_executable(test_config tests/test_config.cpp src/config/config_manager.cpp)
target_include_directories(test_config PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(test_config PRIVATE Qt6::Core PkgConfig::TOML)
add_test(NAME test_config COMMAND test_config)

add_executable(test_layout_model tests/test_layout_model.cpp src/model/keymap_model.cpp
                                 src/decoder/keycode_decoder.cpp src/dbus/strata_dbus_client.cpp)
target_include_directories(test_layout_model PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(test_layout_model PRIVATE Qt6::Core Qt6::DBus)
add_test(NAME test_layout_model COMMAND test_layout_model)

find_program(DBUS_RUN_SESSION dbus-run-session REQUIRED)
add_executable(
    test_lifecycle
    tests/test_lifecycle.cpp
    src/dbus/strata_dbus_client.cpp
    src/model/keymap_model.cpp
    src/decoder/keycode_decoder.cpp
    src/config/config_manager.cpp
    src/tray/tray_manager.cpp
    src/tray/status_notifier_item.cpp
    src/tray/dbus_menu.cpp)
target_include_directories(test_lifecycle PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(test_lifecycle PRIVATE Qt6::Widgets Qt6::DBus Qt6::Quick Qt6::Qml
                                             PkgConfig::TOML)
add_test(NAME test_lifecycle COMMAND ${DBUS_RUN_SESSION} -- $<TARGET_FILE:test_lifecycle>)
set_tests_properties(
    test_lifecycle PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen" WORKING_DIRECTORY
                              ${CMAKE_CURRENT_SOURCE_DIR} TIMEOUT 60)

# Installation
include(GNUInstallDirs)

install(TARGETS strata DESTINATION ${CMAKE_INSTALL_BINDIR})

install(FILES desktop/io.github.lebedenko.Strata.desktop
        DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)

install(FILES desktop/strata.svg DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps)

install(FILES resources/layouts/eyelash_corne.json resources/layouts/voyager.json
              resources/layouts/twindial25.json DESTINATION ${CMAKE_INSTALL_DATADIR}/strata/layouts)

# Tests locate fixtures independently of the selected build directory.
target_compile_definitions(test_layout_model
                           PRIVATE STRATA_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
set_tests_properties(test_decoder test_config test_layout_model test_lifecycle
                     PROPERTIES TIMEOUT 60 WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")

# Qt imported tool targets select the configured Qt installation.
string(CONCAT qt_tools_json "{\"qmllint\":\"$<TARGET_FILE:Qt6::qmllint>\","
              "\"qmlformat\":\"$<TARGET_FILE:Qt6::qmlformat>\"}\n")
file(
    GENERATE
    OUTPUT "${CMAKE_BINARY_DIR}/qt-tools.json"
    CONTENT "${qt_tools_json}")

if(ENABLE_FUZZING)
    add_executable(fuzz_decoder fuzz/decoder.cpp src/decoder/keycode_decoder.cpp)
    target_include_directories(fuzz_decoder PRIVATE src)
    target_link_libraries(fuzz_decoder PRIVATE Qt6::Core)
    target_compile_options(fuzz_decoder PRIVATE -fsanitize=fuzzer)
    target_link_options(fuzz_decoder PRIVATE -fsanitize=fuzzer)
endif()
