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

void test_sensors() {
    auto sVol = KeycodeDecoder::decodeSensor("inc_dec_kp", 0x000c00e9, 0x000c00ea);
    TEST_ASSERT(sVol.cwLabel == "VOL+");
    TEST_ASSERT(sVol.ccwLabel == "VOL-");
    TEST_ASSERT(sVol.category == "media");

    auto sScroll = KeycodeDecoder::decodeSensor("scroll_encoder", 0, 0);
    TEST_ASSERT(sScroll.cwLabel == "SCRL DN");
    TEST_ASSERT(sScroll.ccwLabel == "SCRL UP");
    TEST_ASSERT(sScroll.category == "nav");

    auto sRgb = KeycodeDecoder::decodeSensor("rgb_encoder", 0, 0);
    TEST_ASSERT(sRgb.cwLabel == "RGB BRI");
    TEST_ASSERT(sRgb.ccwLabel == "RGB BRD");

    std::cout << "[PASS] Sensor decoding\n";
}

void test_hyper_and_meh() {
    // Standalone HYPER keypress: &kp LS(LC(LA(LGUI))) -> 0x070700e3
    auto kHyper = KeycodeDecoder::decode("key_press", 0x070700e3, 0);
    TEST_ASSERT(kHyper.primaryLabel == "HYPER");
    TEST_ASSERT(kHyper.secondaryLabel.isEmpty());
    TEST_ASSERT(kHyper.category == "mod");

    // Standalone MEH keypress: &kp LS(LC(LALT)) -> 0x030700e2
    auto kMeh = KeycodeDecoder::decode("key_press", 0x030700e2, 0);
    TEST_ASSERT(kMeh.primaryLabel == "MEH");
    TEST_ASSERT(kMeh.secondaryLabel.isEmpty());
    TEST_ASSERT(kMeh.category == "mod");

    // Home-row mod MEH + G: &hml LS(LC(LALT)) G -> p1: 0x030700e2, p2: 0x0007000a
    auto kHmlMeh = KeycodeDecoder::decode("hml", 0x030700e2, 0x0007000a);
    TEST_ASSERT(kHmlMeh.primaryLabel == "G");
    TEST_ASSERT(kHmlMeh.secondaryLabel == "MEH");
    TEST_ASSERT(kHmlMeh.category == "mod");

    // Home-row mod MEH + H: &hmr LS(LC(LALT)) H -> p1: 0x030700e2, p2: 0x0007000b
    auto kHmrMeh = KeycodeDecoder::decode("hmr", 0x030700e2, 0x0007000b);
    TEST_ASSERT(kHmrMeh.primaryLabel == "H");
    TEST_ASSERT(kHmrMeh.secondaryLabel == "MEH");
    TEST_ASSERT(kHmrMeh.category == "mod");

    // Pure bitmask MEH and HYPER (MOD_LCTL | MOD_LSFT | MOD_LALT = 0x07, + MOD_LGUI = 0x0F)
    TEST_ASSERT(KeycodeDecoder::modifierToLabel(0x07) == "MEH");
    TEST_ASSERT(KeycodeDecoder::modifierToLabel(0x0F) == "HYPER");

    // Single modifiers via keypress
    auto kLgui = KeycodeDecoder::decode("key_press", 0x000700e3, 0);
    TEST_ASSERT(kLgui.primaryLabel == "LGUI");
    TEST_ASSERT(kLgui.category == "mod");

    std::cout << "[PASS] Hyper and Meh decoding\n";
}

int main() {
    std::cout << "Running decoder tests...\n";
    test_alphas();
    test_home_row_mods();
    test_layer_taps();
    test_special_keys();
    test_sensors();
    test_hyper_and_meh();
    std::cout << "All decoder tests passed successfully!\n";
    return 0;
}
