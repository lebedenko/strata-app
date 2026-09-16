#include <cassert>
#include <cstdlib>
#include <iostream>

#include "decoder/keycode_decoder.hpp"

#define TEST_ASSERT(cond)                                                                          \
    do {                                                                                           \
        if (!(cond)) {                                                                             \
            std::cerr << "ASSERTION FAILED: " #cond << " at " << __FILE__ << ":" << __LINE__       \
                      << "\n";                                                                     \
            std::abort();                                                                          \
        }                                                                                          \
    } while (0)

using namespace strata::decoder;

void test_alphas() {
    auto kQ = KeycodeDecoder::decode("key_press", 0x00070014, 0);
    TEST_ASSERT(kQ.primaryLabel == "Q");
    TEST_ASSERT(kQ.secondaryLabel.isEmpty());
    TEST_ASSERT(kQ.category == "alpha");

    auto kA = KeycodeDecoder::decode("key_press", 0x00070004, 0);
    TEST_ASSERT(kA.primaryLabel == "A");

    std::cout << "[PASS] Alpha key decoding\n";
}

void test_home_row_mods() {
    // LGUI + A
    auto kHrm = KeycodeDecoder::decode("home_row_mo", 0x000700e3, 0x00070004);
    TEST_ASSERT(kHrm.primaryLabel == "A");
    TEST_ASSERT(kHrm.secondaryLabel == "GUI");
    TEST_ASSERT(kHrm.category == "mod");

    // LSHIFT + F
    auto kHrmShift = KeycodeDecoder::decode("hml", 0x000700e1, 0x00070009);
    TEST_ASSERT(kHrmShift.primaryLabel == "F");
    TEST_ASSERT(kHrmShift.secondaryLabel == "SHIFT");
    TEST_ASSERT(kHrmShift.category == "mod");

    std::cout << "[PASS] Home-row mod decoding\n";
}

void test_layer_taps() {
    // MO(1) + SPACE (0x2C)
    auto kLt = KeycodeDecoder::decode("layer_tap", 1, 0x0007002c);
    TEST_ASSERT(kLt.primaryLabel == "SPACE");
    TEST_ASSERT(kLt.secondaryLabel == "MO(1)");
    TEST_ASSERT(kLt.category == "layer");

    std::cout << "[PASS] Layer tap decoding\n";
}

void test_special_keys() {
    auto kTrans = KeycodeDecoder::decode("transparent", 0, 0);
    TEST_ASSERT(kTrans.primaryLabel == "▽");

    auto kMute = KeycodeDecoder::decode("key_press", 0x000c00e2, 0);
    TEST_ASSERT(kMute.primaryLabel == "MUTE");
    TEST_ASSERT(kMute.category == "misc");

    auto kCapsWd = KeycodeDecoder::decode("caps_word", 0, 0);
    TEST_ASSERT(kCapsWd.primaryLabel == "CAPS");

    std::cout << "[PASS] Special key decoding\n";
}

int main() {
    std::cout << "Running decoder tests...\n";
    test_alphas();
    test_home_row_mods();
    test_layer_taps();
    test_special_keys();
    std::cout << "All decoder tests passed successfully!\n";
    return 0;
}
