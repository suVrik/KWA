#include "core/utils/parser_utils.h"
#include "core/utils/filesystem_utils.h"

namespace kw {

Parser::Parser(MemoryResource& memory_resource)
    : m_data(memory_resource)
    , m_end(nullptr)
    , m_position(nullptr)
{
}

Parser::Parser(MemoryResource& memory_resource, const char* relative_path)
    : m_data(FilesystemUtils::read_file(memory_resource, relative_path))
    , m_end(m_data.data() + m_data.size())
    , m_position(m_data.data())
{
}

Parser::Parser(MemoryResource& memory_resource, const char* relative_path, size_t max_size)
    : m_data(FilesystemUtils::read_file(memory_resource, relative_path, max_size))
    , m_end(m_data.data() + m_data.size())
    , m_position(m_data.data())
{
}

uint8_t* Parser::read(size_t size) {
    if (m_position + size <= m_end) {
        uint8_t* result = m_position;
        m_position += size;
        return result;
    }
    return nullptr;
}

} // namespace kw
