#include <cstddef>
#include <cstdint>
#include <cstring>

#include "decoder/keycode_decoder.hpp"

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    if (size < 8)
        return 0;
    uint32_t first = 0;
    uint32_t second = 0;
    std::memcpy(&first, data, 4);
    std::memcpy(&second, data + 4, 4);
    const auto behavior = QString::fromUtf8(reinterpret_cast<const char *>(data + 8),
                                            static_cast<qsizetype>(size - 8));
    (void)strata::decoder::KeycodeDecoder::decode(behavior, first, second);
    (void)strata::decoder::KeycodeDecoder::decodeSensor(behavior, first, second);
    return 0;
}
