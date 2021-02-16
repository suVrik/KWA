#include "core/endian_utils.h"

namespace kw::EndianUtils {

inline uint16_t swap_bytes(uint16_t value) {
    return (value << 8) | (value >> 8);
}

inline int16_t swap_bytes(int16_t value) {
    return static_cast<int16_t>(swap_bytes(static_cast<uint16_t>(value)));
}

inline uint32_t swap_bytes(uint32_t value) {
    return (value << 24) | ((value << 8) & 0xFF0000U) | ((value >> 8) & 0xFF00U) | (value >> 24);
}

inline int32_t swap_bytes(int32_t value) {
    return static_cast<int32_t>(swap_bytes(static_cast<uint32_t>(value)));
}

inline uint64_t swap_bytes(uint64_t value) {
    return (static_cast<uint64_t>(swap_bytes(static_cast<uint32_t>(value))) << 32) |
            static_cast<uint64_t>(swap_bytes(static_cast<uint32_t>(value >> 32)));
}

inline int64_t swap_bytes(int64_t value) {
    return static_cast<int64_t>(swap_bytes(static_cast<uint64_t>(value)));
}

inline float swap_bytes(float value) {
    union {
        float a;
        uint32_t b;
    } x = { value };

    x.b = swap_bytes(x.b);

    return x.a;
}

inline double swap_bytes(double value) {
    union {
        double a;
        uint64_t b;
    } x = { value };

    x.b = swap_bytes(x.b);

    return x.a;
}

#ifdef KW_BIG_ENDIAN

uint16_t swap_le(uint16_t value) {
    return swap_bytes(value);
}

int16_t swap_le(int16_t value) {
    return swap_bytes(value);
}

uint32_t swap_le(uint32_t value) {
    return swap_bytes(value);
}

int32_t swap_le(int32_t value) {
    return swap_bytes(value);
}

uint64_t swap_le(uint64_t value) {
    return swap_bytes(value);
}

int64_t swap_le(int64_t value) {
    return swap_bytes(value);
}

float swap_le(float value) {
    return swap_bytes(value);
}

double swap_le(double value) {
    return swap_bytes(value);
}

uint16_t swap_be(uint16_t value) {
    return value;
}

int16_t swap_be(int16_t value) {
    return value;
}

uint32_t swap_be(uint32_t value) {
    return value;
}

int32_t swap_be(int32_t value) {
    return value;
}

uint64_t swap_be(uint64_t value) {
    return value;
}

int64_t swap_be(int64_t value) {
    return value;
}

float swap_be(float value) {
    return value;
}

double swap_be(double value) {
    return value;
}

#else

uint16_t swap_le(uint16_t value) {
    return value;
}

int16_t swap_le(int16_t value) {
    return value;
}

uint32_t swap_le(uint32_t value) {
    return value;
}

int32_t swap_le(int32_t value) {
    return value;
}

uint64_t swap_le(uint64_t value) {
    return value;
}

int64_t swap_le(int64_t value) {
    return value;
}

float swap_le(float value) {
    return value;
}

double swap_le(double value) {
    return value;
}

uint16_t swap_be(uint16_t value) {
    return swap_bytes(value);
}

int16_t swap_be(int16_t value) {
    return swap_bytes(value);
}

uint32_t swap_be(uint32_t value) {
    return swap_bytes(value);
}

int32_t swap_be(int32_t value) {
    return swap_bytes(value);
}

uint64_t swap_be(uint64_t value) {
    return swap_bytes(value);
}

int64_t swap_be(int64_t value) {
    return swap_bytes(value);
}

float swap_be(float value) {
    return swap_bytes(value);
}

double swap_be(double value) {
    return swap_bytes(value);
}

#endif

} // namespace kw::EndianUtils
