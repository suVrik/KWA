#pragma once

#include "core/utils/endian_utils.h"

#include <fstream>
#include <optional>
#include <type_traits>

namespace kw {

class BinaryReader {
public:
    BinaryReader() = default;
    explicit BinaryReader(const char* path);

    bool read(void* data, size_t size);

    template <typename T>
    bool read_le(T* output, size_t count = 1);

    template <typename T>
    std::optional<T> read_le();

    template <typename T>
    bool read_be(T* output, size_t count = 1);

    template <typename T>
    std::optional<T> read_be();

    operator bool() const;

private:
    std::ifstream m_stream;
};

} // namespace kw

#include "core/io/binary_reader_impl.h"
