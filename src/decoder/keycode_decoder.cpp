#include "decoder/keycode_decoder.hpp"

namespace strata::decoder {

namespace {

// ZMK modifier flags (dt-bindings/zmk/modifiers.h)
constexpr uint8_t MOD_LCTL = 0x01;
constexpr uint8_t MOD_LSFT = 0x02;
constexpr uint8_t MOD_LALT = 0x04;
constexpr uint8_t MOD_LGUI = 0x08;
constexpr uint8_t MOD_RCTL = 0x10;
constexpr uint8_t MOD_RSFT = 0x20;
constexpr uint8_t MOD_RALT = 0x40;
constexpr uint8_t MOD_RGUI = 0x80;

uint8_t extractModifierMask(uint32_t code) {
    uint8_t explicitMods = static_cast<uint8_t>((code >> 24) & 0xFF);
    uint32_t usage = code & 0xFFFF;
    uint8_t usageMod = 0;

    if (usage >= 0xE0 && usage <= 0xE7) {
        usageMod = static_cast<uint8_t>(1 << (usage - 0xE0));
    } else if ((code >> 8) == 0 && code != 0) {
        // Pure modifier bitmask (e.g. MOD_LCTL | MOD_LSFT)
        return static_cast<uint8_t>(code & 0xFF);
    }

    return explicitMods | usageMod;
}

} // namespace

QString KeycodeDecoder::modifierToLabel(uint32_t modCode) {
    uint8_t mask = extractModifierMask(modCode);
    bool hasCtrl = (mask & (MOD_LCTL | MOD_RCTL)) != 0;
    bool hasShift = (mask & (MOD_LSFT | MOD_RSFT)) != 0;
    bool hasAlt = (mask & (MOD_LALT | MOD_RALT)) != 0;
    bool hasGui = (mask & (MOD_LGUI | MOD_RGUI)) != 0;

    // Emulated Hyper key: Ctrl + Shift + Alt + GUI
    if (hasCtrl && hasShift && hasAlt && hasGui) {
        return "HYPER";
    }

    // Emulated Meh key: Ctrl + Shift + Alt (without GUI)
    if (hasCtrl && hasShift && hasAlt && !hasGui) {
        return "MEH";
    }

    // Common two-modifier combinations
    if (hasCtrl && hasAlt && !hasShift && !hasGui)
        return "C+A";
    if (hasCtrl && hasShift && !hasAlt && !hasGui)
        return "C+S";
    if (hasAlt && hasShift && !hasCtrl && !hasGui)
        return "A+S";
    if (hasGui && hasCtrl && !hasShift && !hasAlt)
        return "G+C";
    if (hasGui && hasAlt && !hasShift && !hasCtrl)
        return "G+A";
    if (hasGui && hasShift && !hasCtrl && !hasAlt)
        return "G+S";

    // Single modifiers
    if (hasCtrl)
        return "CTRL";
    if (hasShift)
        return "SHIFT";
    if (hasAlt)
        return "ALT";
    if (hasGui)
        return "GUI";

    return "MOD";
}

QString KeycodeDecoder::hidUsageToLabel(uint32_t code) {
    bool isShifted = (code & 0x02000000) != 0;
    uint32_t page = (code >> 16) & 0xFF;
    uint32_t usage = code & 0xFFFF;

    // Consumer page
    if (page == 0x0C) {
        switch (usage) {
        case 0xE2:
            return "MUTE";
        case 0xE9:
            return "VOL+";
        case 0xEA:
            return "VOL-";
        case 0xB5:
            return "NEXT";
        case 0xB6:
            return "PREV";
        case 0xCD:
            return "PLAY";
        default:
            return QString("MEDIA(0x%1)").arg(usage, 0, 16);
        }
    }

    if (isShifted) {
        switch (usage) {
        case 0x1E:
            return "!";
        case 0x1F:
            return "@";
        case 0x20:
            return "#";
        case 0x21:
            return "$";
        case 0x22:
            return "%";
        case 0x23:
            return "^";
        case 0x24:
            return "&";
        case 0x25:
            return "*";
        case 0x26:
            return "(";
        case 0x27:
            return ")";
        case 0x2D:
            return "_";
        case 0x2E:
            return "+";
        case 0x2F:
            return "{";
        case 0x30:
            return "}";
        case 0x31:
            return "|";
        case 0x33:
            return ":";
        case 0x34:
            return "\"";
        case 0x35:
            return "~";
        case 0x36:
            return "<";
        case 0x37:
            return ">";
        case 0x38:
            return "?";
        default:
            break;
        }
    }

    // Standard Keyboard / Keypad Page
    if (usage >= 0x04 && usage <= 0x1D) {
        return QString(QChar('A' + static_cast<char>(usage - 0x04)));
    }

    switch (usage) {
    case 0x1E:
        return "1";
    case 0x1F:
        return "2";
    case 0x20:
        return "3";
    case 0x21:
        return "4";
    case 0x22:
        return "5";
    case 0x23:
        return "6";
    case 0x24:
        return "7";
    case 0x25:
        return "8";
    case 0x26:
        return "9";
    case 0x27:
        return "0";
    case 0x28:
        return "ENTER";
    case 0x29:
        return "ESC";
    case 0x2A:
        return "BSPC";
    case 0x2B:
        return "TAB";
    case 0x2C:
        return "SPACE";
    case 0x2D:
        return "-";
    case 0x2E:
        return "=";
    case 0x2F:
        return "[";
    case 0x30:
        return "]";
    case 0x31:
        return "\\";
    case 0x33:
        return ";";
    case 0x34:
        return "'";
    case 0x35:
        return "`";
    case 0x36:
        return ",";
    case 0x37:
        return ".";
    case 0x38:
        return "/";
    case 0x39:
        return "CAPS";
    case 0x3A:
        return "F1";
    case 0x3B:
        return "F2";
    case 0x3C:
        return "F3";
    case 0x3D:
        return "F4";
    case 0x3E:
        return "F5";
    case 0x3F:
        return "F6";
    case 0x40:
        return "F7";
    case 0x41:
        return "F8";
    case 0x42:
        return "F9";
    case 0x43:
        return "F10";
    case 0x44:
        return "F11";
    case 0x45:
        return "F12";
    case 0x46:
        return "PSCRN";
    case 0x47:
        return "SCLK";
    case 0x48:
        return "PAUSE";
    case 0x49:
        return "INS";
    case 0x4A:
        return "HOME";
    case 0x4B:
        return "PGUP";
    case 0x4C:
        return "DEL";
    case 0x4D:
        return "END";
    case 0x4E:
        return "PGDN";
    case 0x4F:
        return "RIGHT";
    case 0x50:
        return "LEFT";
    case 0x51:
        return "DOWN";
    case 0x52:
        return "UP";
    case 0xE0:
    case 0xE1:
    case 0xE2:
    case 0xE3:
    case 0xE4:
    case 0xE5:
    case 0xE6:
    case 0xE7: {
        uint8_t modMask = extractModifierMask(code);
        bool hasCtrl = (modMask & (MOD_LCTL | MOD_RCTL)) != 0;
        bool hasShift = (modMask & (MOD_LSFT | MOD_RSFT)) != 0;
        bool hasAlt = (modMask & (MOD_LALT | MOD_RALT)) != 0;
        bool hasGui = (modMask & (MOD_LGUI | MOD_RGUI)) != 0;

        if (hasCtrl && hasShift && hasAlt && hasGui) {
            return "HYPER";
        }
        if (hasCtrl && hasShift && hasAlt && !hasGui) {
            return "MEH";
        }

        switch (usage) {
        case 0xE0:
            return "LCTRL";
        case 0xE1:
            return "LSHIFT";
        case 0xE2:
            return "LALT";
        case 0xE3:
            return "LGUI";
        case 0xE4:
            return "RCTRL";
        case 0xE5:
            return "RSHIFT";
        case 0xE6:
            return "RALT";
        case 0xE7:
            return "RGUI";
        default:
            return QString("0x%1").arg(code, 0, 16);
        }
    }
    default:
        if (code == 0)
            return "";
        {
            uint8_t modMask = extractModifierMask(code);
            if (modMask != 0) {
                bool hasCtrl = (modMask & (MOD_LCTL | MOD_RCTL)) != 0;
                bool hasShift = (modMask & (MOD_LSFT | MOD_RSFT)) != 0;
                bool hasAlt = (modMask & (MOD_LALT | MOD_RALT)) != 0;
                bool hasGui = (modMask & (MOD_LGUI | MOD_RGUI)) != 0;
                if (hasCtrl && hasShift && hasAlt && hasGui)
                    return "HYPER";
                if (hasCtrl && hasShift && hasAlt && !hasGui)
                    return "MEH";
            }
        }
        return QString("0x%1").arg(code, 0, 16);
    }
    return QString("0x%1").arg(code, 0, 16);
}

DecodedKey KeycodeDecoder::decode(const QString &behavior, uint32_t param1, uint32_t param2) {
    DecodedKey k;

    if (behavior == "key_press" || behavior == "kp") {
        k.primaryLabel = hidUsageToLabel(param1);
        if (k.primaryLabel == "HYPER" || k.primaryLabel == "MEH" || k.primaryLabel == "LCTRL" ||
            k.primaryLabel == "RCTRL" || k.primaryLabel == "LSHIFT" || k.primaryLabel == "RSHIFT" ||
            k.primaryLabel == "LALT" || k.primaryLabel == "RALT" || k.primaryLabel == "LGUI" ||
            k.primaryLabel == "RGUI") {
            k.category = "mod";
        } else if (k.primaryLabel.length() == 1 && k.primaryLabel[0].isLetter()) {
            k.category = "alpha";
        } else {
            k.category = "misc";
        }

        if (k.primaryLabel == "HYPER") {
            k.tooltip = "Key Press: HYPER (Ctrl+Shift+Alt+Gui)";
        } else if (k.primaryLabel == "MEH") {
            k.tooltip = "Key Press: MEH (Ctrl+Shift+Alt)";
        } else {
            k.tooltip = QString("Key Press: %1").arg(k.primaryLabel);
        }
        return k;
    }

    if (behavior == "home_row_mo" || behavior == "hml" || behavior == "hmr" || behavior == "mt") {
        k.primaryLabel = hidUsageToLabel(param2);
        k.secondaryLabel = modifierToLabel(param1);
        k.category = "mod";
        if (k.secondaryLabel == "HYPER") {
            k.tooltip =
                QString("Home-Row Mod: HYPER [Ctrl+Shift+Alt+Gui] (Tap: %1)").arg(k.primaryLabel);
        } else if (k.secondaryLabel == "MEH") {
            k.tooltip = QString("Home-Row Mod: MEH [Ctrl+Shift+Alt] (Tap: %1)").arg(k.primaryLabel);
        } else {
            k.tooltip = QString("Home-Row Mod: %1 (Tap: %2)").arg(k.secondaryLabel, k.primaryLabel);
        }
        return k;
    }

    if (behavior == "layer_tap" || behavior == "lt") {
        k.primaryLabel = hidUsageToLabel(param2);
        k.secondaryLabel = QString("MO(%1)").arg(param1);
        k.category = "layer";
        k.tooltip = QString("Layer Tap: %1 (Hold: Layer %2)").arg(k.primaryLabel).arg(param1);
        return k;
    }

    if (behavior == "to_layer" || behavior == "to") {
        k.primaryLabel = QString("TO(%1)").arg(param1);
        k.category = "layer";
        k.tooltip = QString("Switch to Layer %1").arg(param1);
        return k;
    }

    if (behavior == "momentary_layer" || behavior == "mo") {
        k.primaryLabel = QString("MO(%1)").arg(param1);
        k.category = "layer";
        k.tooltip = QString("Momentary Layer %1").arg(param1);
        return k;
    }

    if (behavior == "toggle_layer" || behavior == "tog") {
        k.primaryLabel = QString("TOG(%1)").arg(param1);
        k.category = "layer";
        k.tooltip = QString("Toggle Layer %1").arg(param1);
        return k;
    }

    if (behavior == "transparent" || behavior == "trans") {
        k.primaryLabel = "▽";
        k.category = "misc";
        k.tooltip = "Transparent (Pass-through)";
        return k;
    }

    if (behavior == "caps_word") {
        k.primaryLabel = "CAPS";
        k.category = "misc";
        k.tooltip = "Caps Word (Smart Caps)";
        return k;
    }

    if (behavior == "mouse_move") {
        k.primaryLabel = "MOVE";
        k.category = "nav";
        k.tooltip = "Mouse Cursor Move";
        return k;
    }

    if (behavior == "mouse_key_p") {
        k.primaryLabel = "CLICK";
        k.category = "nav";
        k.tooltip = "Mouse Click";
        return k;
    }

    if (behavior == "none") {
        k.primaryLabel = "---";
        k.category = "misc";
        k.tooltip = "Disabled";
        return k;
    }

    // Fallback for custom or unknown behaviors
    k.primaryLabel = behavior.toUpper();
    k.category = "misc";
    k.tooltip = QString("%1(0x%2, 0x%3)").arg(behavior).arg(param1, 0, 16).arg(param2, 0, 16);
    return k;
}

DecodedSensor KeycodeDecoder::decodeSensor(const QString &behavior, uint32_t param1,
                                           uint32_t param2) {
    DecodedSensor s;
    s.behavior = behavior;

    if (behavior == "inc_dec_kp") {
        s.cwLabel = hidUsageToLabel(param1);
        s.ccwLabel = hidUsageToLabel(param2);
        s.category = "media";
        s.tooltip = QString("Encoder: CW → %1, CCW → %2").arg(s.cwLabel, s.ccwLabel);
        return s;
    }

    if (behavior == "scroll_encoder" || behavior.contains("scroll")) {
        s.cwLabel = "SCRL DN";
        s.ccwLabel = "SCRL UP";
        s.category = "nav";
        s.tooltip = "Encoder: CW → Scroll Down, CCW → Scroll Up";
        return s;
    }

    if (behavior == "rgb_encoder" || behavior.contains("rgb")) {
        s.cwLabel = "RGB BRI";
        s.ccwLabel = "RGB BRD";
        s.category = "misc";
        s.tooltip = "Encoder: CW → RGB Brightness +, CCW → RGB Brightness -";
        return s;
    }

    if (param1 != 0 || param2 != 0) {
        s.cwLabel = hidUsageToLabel(param1);
        s.ccwLabel = hidUsageToLabel(param2);
    } else {
        s.cwLabel = "CW";
        s.ccwLabel = "CCW";
    }
    s.category = "misc";
    s.tooltip = QString("Encoder %1").arg(behavior);
    return s;
}

} // namespace strata::decoder
