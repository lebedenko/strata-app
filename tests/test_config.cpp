#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>

#include "config/config_manager.hpp"

#define TEST_ASSERT(cond)                                                                          \
    do {                                                                                           \
        if (!(cond)) {                                                                             \
            std::cerr << "ASSERTION FAILED: " #cond << " at " << __FILE__ << ":" << __LINE__       \
                      << "\n";                                                                     \
            std::abort();                                                                          \
        }                                                                                          \
    } while (0)

namespace fs = std::filesystem;

int main() {
    std::cout << "Running config tests...\n";

    fs::path tempConfigDir = fs::temp_directory_path() / "strata_test_config";
    fs::remove_all(tempConfigDir);
    fs::create_directories(tempConfigDir);

    setenv("XDG_CONFIG_HOME", tempConfigDir.c_str(), 1);

    {
        strata::config::ConfigManager mgr;
        TEST_ASSERT(!mgr.startMinimized());
        TEST_ASSERT(mgr.showTrayIcon());
        TEST_ASSERT(mgr.notificationsEnabled());
        TEST_ASSERT(mgr.windowWidth() == 980);
        TEST_ASSERT(mgr.windowHeight() == 520);

        // Modify values
        mgr.setStartMinimized(true);
        mgr.setWindowWidth(1200);
        mgr.setWindowHeight(600);
        mgr.setShowKeyPositions(true);

        TEST_ASSERT(mgr.save());
        std::cout << "[PASS] Default values & save config\n";
    }

    {
        // Reload in new instance
        strata::config::ConfigManager mgr2;
        TEST_ASSERT(mgr2.startMinimized());
        TEST_ASSERT(mgr2.windowWidth() == 1200);
        TEST_ASSERT(mgr2.windowHeight() == 600);
        TEST_ASSERT(mgr2.showKeyPositions());
        std::cout << "[PASS] Reload modified config\n";
    }

    fs::remove_all(tempConfigDir);
    std::cout << "All config tests passed successfully!\n";
    return 0;
}
