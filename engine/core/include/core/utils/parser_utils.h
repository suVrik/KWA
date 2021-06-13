#pragma once

#include "core/utils/endian_utils.h"

#include <fstream>
#include <optional>
#include <type_traits>

namespace kw {

class Reader {
public:
    Reader() = default;
    explicit Reader(const char* path);

    bool read(void* data, size_t size);

    template <typename T>
    bool read_le(T* output, size_t count = 1) {
        if (read(output, sizeof(T) * count)) {
            for (size_t i = 0; i < count; i++) {
                if constexpr (std::is_enum_v<T>) {
                    // Enum types don't require custom endian swap function defined.
                    output[i] = static_cast<T>(EndianUtils::swap_le(static_cast<std::underlying_type_t<T>>(output[i])));
                } else {
                    // Custom structures require custom endian swap function defined.
                    output[i] = EndianUtils::swap_le(output[i]);
                }
            }
            return true;
        }
        return false;
    }

    template <typename T>
    std::optional<T> read_le() {
        T result;
        if (!read_le(&result, 1)) {
            return std::nullopt;
        }
        return result;
    }

    template <typename T>
    bool read_be(T* output, size_t count = 1) {
        if (read(output, sizeof(T) * count)) {
            for (size_t i = 0; i < count; i++) {
                if constexpr (std::is_enum_v<T>) {
                    // Enum types don't require custom endian swap function defined.
                    result[i] = static_cast<T>(EndianUtils::swap_be(static_cast<std::underlying_type_t<T>>(result[i])));
                } else {
                    // Custom structures require custom endian swap function defined.
                    result[i] = EndianUtils::swap_be(result[i]);
                }
            }
            return true;
        }
        return false;
    }

    template <typename T>
    std::optional<T> read_be() {
        T result;
        if (!read_be(&result, 1)) {
            return std::nullopt;
        }
        return result;
    }

    operator bool() const {
        return static_cast<bool>(m_stream);
    }

private:
    std::ifstream m_stream;
};

class Writer {
public:
    Writer() = default;
    explicit Writer(const char* path);

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
