#include "core/utils/parser_utils.h"
#include "core/debug/assert.h"

namespace kw {

Reader::Reader(const char* path)
    : m_stream(path, std::ios::binary)
{
}

bool Reader::read(void* output, size_t size) {
    KW_ASSERT(output != nullptr);
    return static_cast<bool>(m_stream.read(static_cast<char*>(output), size));
}

Writer::Writer(const char* path)
    : m_stream(path, std::ios::binary)
{
}

bool Writer::write(const void* data, size_t size) {
    KW_ASSERT(data != nullptr);
    return static_cast<bool>(m_stream.write(reinterpret_cast<const char*>(data), size));
}

} // namespace kw
