#pragma once

#include "core/containers/vector.h"
#include "core/utils/endian_utils.h"

#include <type_traits>

namespace kw {

class Parser {
public:
    Parser(MemoryResource& memory_resource);
    Parser(MemoryResource& memory_resource, const char* relative_path);
    Parser(MemoryResource& memory_resource, const char* relative_path, size_t max_size);

    // Returns nullptr if out of bounds.
    uint8_t* read(size_t size);

    template <typename T>
    T* read_le(size_t count = 1) {
        T* result = reinterpret_cast<T*>(read(sizeof(T) * count));
        if (result != nullptr) {
            for (size_t i = 0; i < count; i++) {
                if constexpr (std::is_enum_v<T>) {
                    // Enum types don't require custom endian swap function defined.
                    result[i] = static_cast<T>(EndianUtils::swap_le(static_cast<std::underlying_type_t<T>>(result[i])));
                } else {
                    // Custom structures require custom endian swap function defined.
                    result[i] = EndianUtils::swap_le(result[i]);
                }
            }
        }
        return result;
    }

    template <typename T>
    T* read_be(size_t count = 1) {
        T* result = reinterpret_cast<T*>(read(sizeof(T) * count));
        if (result != nullptr) {
            for (size_t i = 0; i < count; i++) {
                if constexpr (std::is_enum_v<T>) {
                    // Enum types don't require custom endian swap function defined.
                    result[i] = static_cast<T>(EndianUtils::swap_be(static_cast<std::underlying_type_t<T>>(result[i])));
                } else {
                    // Custom structures require custom endian swap function defined.
                    result[i] = EndianUtils::swap_be(result[i]);
                }
            }
        }
        return result;
    }

    bool is_eof() const {
        return m_position == m_end;
    }

private:
    Vector<uint8_t> m_data;
    uint8_t* m_end;
    uint8_t* m_position;
};

} // namespace kw
