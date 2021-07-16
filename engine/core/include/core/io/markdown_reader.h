#pragma once

#include "core/io/markdown.h"
#include "core/io/text_reader.h"

namespace kw {

class MarkdownReader : public TextParser<MarkdownReader> {
public:
    MarkdownReader(MemoryResource& memory_resource, const char* relative_path);

    // Root node is an array. Throws when out of bounds.
    MarkdownNode& operator[](size_t index) const;
    size_t get_size() const;

private:
    class NumberToken : public Token {
    public:
        void init(const char* begin, const char* end) override;

        double value = 0.0;
    };

    class StringToken : public Token {
    public:
        void init(const char* begin, const char* end) override;

        StringView value;
    };

    class BooleanToken : public Token {
    public:
        void init(const char* begin, const char* end) override;

        bool value = false;
    };

    class ObjectToken : public Token {
    public:
    };

    class ArrayToken : public Token {
    public:
    };

    bool letter();
    bool non_zero_digit();
    bool digit();
    bool space();
    bool opt_digits();
    bool opt_spaces();
    bool opt_minus();
    bool spaces();
    bool real_number();
    bool int_number();
    bool number();
    bool escape_char();
    bool string_char();
    bool opt_string_chars();
    bool string();
    bool boolean();
    bool key_char();
    bool opt_key_chars();
    bool key_start_char();
    bool key();
    bool object();
    bool array();
    bool value();
    bool key_value();
    bool opt_space_separated_key_values();
    bool opt_key_values();
    bool opt_space_separated_values();
    bool opt_values();

    UniquePtr<MarkdownNode> build_node_from_token(Token* token);

    MemoryResource& m_memory_resource;
    UniquePtr<ArrayNode> m_root;
};

} // namespace kw
