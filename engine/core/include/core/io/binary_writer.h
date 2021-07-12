#pragma once

#include "core/utils/endian_utils.h"

#include <fstream>
#include <type_traits>

namespace kw {

class BinaryWriter {
public:
    BinaryWriter() = default;
    explicit BinaryWriter(const char* path);

    bool write(const void* data, size_t size);

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

    operator bool() const {
        return static_cast<bool>(m_stream);
    }

private:
    std::ofstream m_stream;
};

} // namespace kw
