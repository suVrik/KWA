#pragma once

#include "core/io/text_reader.h"

namespace kw {

template <typename Child>
void TextParser<Child>::Token::init(const char* begin, const char* end) {
    // No-op.
}

template <typename Child>
class TextParser<Child>::RootToken : public TextParser<Child>::Token {
public:
};

template <typename Child>
TextParser<Child>::TextParser(MemoryResource& memory_resource, const char* relative_path)
    : m_memory_resource(memory_resource)
    , m_data(memory_resource)
    , m_token(static_pointer_cast<Token>(allocate_unique<RootToken>(memory_resource)))
{
    std::ifstream file(relative_path, std::ios::binary | std::ios::ate);

    KW_ERROR(
        file,
        "Failed to open text file \"%s\".", relative_path
    );

    std::streamsize size = file.tellg();

    KW_ERROR(
        size >= 0,
        "Failed to query text file size \"%s\".", relative_path
    );

    file.seekg(0, std::ios::beg);

    m_data.resize(size);

    KW_ERROR(
        file.read(m_data.data(), size),
        "Failed to read text file \"%s\".", relative_path
    );

    m_current = m_data.data();
}

template <typename Child>
bool TextParser<Child>::parse(char c) {
    if (*m_current == c) {
        m_current++;
        return true;
    }
    return false;
}

template <typename Child>
bool TextParser<Child>::parse(const char* string) {
    const char* temp_current = m_current;
    while (*string != '\0') {
        if (*m_current != *(string++)) {
            m_current = temp_current;
            return false;
        }
        m_current++;
    }
    return true;
}

template <typename Child>
bool TextParser<Child>::parse(bool (Child::*func)()) {
    const char* temp_current = m_current;
    Token* temp_last = m_token->last.get();
    if (!(static_cast<Child*>(this)->*func)()) {
        m_current = temp_current;
        pop_until(temp_last);
        return false;
    }
    return true;
}

template <typename Child>
template <typename Arg, typename... Args>
bool TextParser<Child>::parse(Arg arg, Args... args) {
    const char* temp_current = m_current;
    Token* temp_last = m_token->last.get();
    if (!parse(arg) || !parse(args...)) {
        m_current = temp_current;
        pop_until(temp_last);
        return false;
    }
    return true;
}

template <typename Child>
template <typename Arg>
bool TextParser<Child>::parse_recursive(Arg arg) {
    while (parse(arg));
    return true;
}

template <typename Child>
template <typename Arg, typename... Args>
bool TextParser<Child>::parse_recursive(Arg arg, Args... args) {
    while (parse(arg) && parse(args...));
    return true;
}

template <typename Child>
bool TextParser<Child>::parse_any_of(const char* string) {
    while (*string != '\0') {
        if (parse(*(string++))) {
            return true;
        }
    }
    return false;
}

template <typename Child>
bool TextParser<Child>::parse_any_but(const char* string) {
    do {
        if (*m_current == *string) {
            return false;
        }
    } while (*(string++) != '\0');
    m_current++;
    return true;
}

template <typename Child>
template <typename T, typename... Args>
bool TextParser<Child>::token(Args... args) {
    const char* old_current = m_current;
    UniquePtr<Token> old_token = std::move(m_token);

    m_token = static_pointer_cast<Token>(allocate_unique<T>(m_memory_resource));

    bool result = parse(args...);
    if (result) {
        m_token->init(old_current, m_current);
        m_token->previous = std::move(old_token->last);
        old_token->last = std::move(m_token);
    }

    m_token = std::move(old_token);

    return result;
}

template <typename Child>
typename TextParser<Child>::Token* TextParser<Child>::get_last() const {
    return m_token->last.get();
}

template <typename Child>
void TextParser<Child>::pop_until(Token* token) {
    while (m_token->last.get() != token) {
        UniquePtr<Token> old_last = std::move(m_token->last);
        m_token->last = std::move(old_last->previous);
    }
}

} // namespace kw
