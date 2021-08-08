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
    bool write_le(T* values, size_t count);

    template <typename T, typename U>
    bool write_le(const U& uvalue);

    template <typename T>
    bool write_be(T* values, size_t count);

    template <typename T, typename U>
    bool write_be(const U& uvalue);

    operator bool() const;

private:
    std::ofstream m_stream;
};

} // namespace kw

#include "core/io/binary_writer_impl.h"
