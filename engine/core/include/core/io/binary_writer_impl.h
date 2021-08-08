#pragma once

#include "core/io/binary_writer.h"

namespace kw {

template <typename T>
bool write_le(T* values, size_t count) {
    for (size_t i = 0; i < count; i++) {
        if constexpr (std::is_enum_v<T>) {
            // Enum types don't require custom endian swap function defined.
            values[i] = static_cast<T>(EndianUtils::swap_le(static_cast<std::underlying_type_t<T>>(values[i])));
        } else {
            // Custom structures require custom endian swap function defined.
            values[i] = EndianUtils::swap_le(values[i]);
        }
    }
    return write(values, sizeof(T)* count);
}

template <typename T, typename U>
bool write_le(const U& uvalue) {
    T tvalue = static_cast<T>(uvalue);
    return write_le(&tvalue, 1);
}

template <typename T>
bool write_be(T* values, size_t count) {
    for (size_t i = 0; i < count; i++) {
        if constexpr (std::is_enum_v<T>) {
            // Enum types don't require custom endian swap function defined.
            values[i] = static_cast<T>(EndianUtils::swap_be(static_cast<std::underlying_type_t<T>>(values[i])));
        } else {
            // Custom structures require custom endian swap function defined.
            values[i] = EndianUtils::swap_be(values[i]);
        }
    }

    return write(values, sizeof(T)* count);
}

template <typename T, typename U>
bool write_be(const U& uvalue) {
    T tvalue = static_cast<T>(uvalue);
    return write_be(&tvalue, 1);
}

} // namespace kw
