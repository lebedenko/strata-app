#pragma once

#include <QString>
#include <cstdint>

namespace strata::decoder {

struct DecodedKey {
    QString primaryLabel;
    QString secondaryLabel;
    QString tooltip;
    QString category; // "alpha", "mod", "layer", "nav", "media", "symbol", "misc"
};

class KeycodeDecoder {
public:
    static DecodedKey decode(const QString &behavior, uint32_t param1, uint32_t param2);

    static QString hidUsageToLabel(uint32_t code);
    static QString modifierToLabel(uint32_t modCode);
};

} // namespace strata::decoder
