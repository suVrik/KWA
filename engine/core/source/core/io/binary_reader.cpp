#include "core/io/binary_reader.h"
#include "core/debug/assert.h"

namespace kw {

BinaryReader::BinaryReader(const char* path)
    : m_stream(path, std::ios::binary)
{
}

bool BinaryReader::read(void* output, size_t size) {
    KW_ASSERT(output != nullptr || size == 0);
    return static_cast<bool>(m_stream.read(static_cast<char*>(output), size));
}

BinaryReader::operator bool() const {
    return static_cast<bool>(m_stream);
}

} // namespace kw
