#include "core/io/markdown_reader.h"
#include "core/debug/assert.h"

#include <algorithm>

namespace kw {

// 
// <letter>           ::= "A" | "B" | "C" | "D" | "E" | "F" | "G" | "H" | "I" | "J" | "K" | "L" | "M" |
//                        "N" | "O" | "P" | "Q" | "R" | "S" | "T" | "U" | "V" | "W" | "X" | "Y" | "Z" |
//                        "a" | "b" | "c" | "d" | "e" | "f" | "g" | "h" | "i" | "j" | "k" | "l" | "m" |
//                        "n" | "o" | "p" | "q" | "r" | "s" | "t" | "u" | "v" | "w" | "x" | "y" | "z"
// 
// <non-zero-digit>   ::= "1" | "2" | "3" | "4" | "5" | "6" | "7" | "8" | "9"
// 
// <digit>            ::= "0" | <non-zero-digit>
// 
// <space>            ::= " " | "\t" | "\n" | "\v" | "\f" | "\r"
// 
// <opt-digits>       ::= <digit> <opt-digits> | ""
// 
// <opt-spaces>       ::= <space> <opt-spaces> | ""
// 
// <opt_minus>        ::= "-" | ""
// 
// <spaces>           ::= <space> <opt-spaces>
// 
// <int-number>       ::= "0" | <non-zero-digit> <opt-digits>
// 
// <real-number>      ::= <int-number> "." <digit> <opt-digits>
// 
// <number>           ::= <opt_minus> <int-number> | <opt_minus> <real-number>
// 
// <escape-char>      ::= "\"" | "\\" | "t" | "n" | "v" | "f" | "r"
// 
// <string-char>      ::= Any character but not double quote, backslash or line terminator | "\\" <escape-char>
// 
// <opt-string-chars> ::= <string-char> <opt-string-chars> | ""
// 
// <string>           ::= "\"" <opt-string-chars> "\""
// 
// <bool>             ::= "true" | "false"
// 
// <key-char>         ::= <letter> | "_" | <digit>
// 
// <opt-key-chars>    ::= <key-char> <opt-key-chars> | ""
// 
// <key-start-char>   ::= <letter> | "_"
// 
// <key>              ::= <key-start-char> <opt-key-chars>
// 
// <value>            ::= <number> | <string> | <bool> | <object> | <array>
// 
// <key-value>        ::= <key> <opt-spaces> ":" <opt-spaces> <value>
// 
// <opt-key-values>   ::= <key-value> <spaces> <opt-key-values> | <key-value> | ""
// 
// <object>           ::= "{" <opt-spaces> <opt-key-values> <opt-spaces> "}"
// 
// <opt-values>       ::= <value> <spaces> <opt-values> | <value> | ""
// 
// <array>            ::= "[" <opt-spaces> <opt-values> <opt-spaces> "]"
// 
// <syntax>           ::= <value>
// 

void MarkdownReader::NumberToken::init(const char* begin, const char* end) {
    value = std::atof(begin);
}

MarkdownType MarkdownReader::NumberToken::get_type() const {
    return MarkdownType::NUMBER;
}

void MarkdownReader::StringToken::init(const char* begin_, const char* end_) {
    if (*begin_ == '"') {
        // String literal.
        value = StringView(begin_ + 1, end_ - begin_ - 2);
    } else {
        // Object key.
        value = StringView(begin_, end_ - begin_);
    }
}

MarkdownType MarkdownReader::StringToken::get_type() const {
    return MarkdownType::STRING;
}

void MarkdownReader::BooleanToken::init(const char* begin, const char* end) {
    value = std::strncmp(begin, "true", 4) == 0;
}

MarkdownType MarkdownReader::BooleanToken::get_type() const {
    return MarkdownType::BOOLEAN;
}

MarkdownType MarkdownReader::ObjectToken::get_type() const {
    return MarkdownType::OBJECT;
}

MarkdownType MarkdownReader::ArrayToken::get_type() const {
    return MarkdownType::ARRAY;
}

MarkdownReader::MarkdownReader(MemoryResource& memory_resource, const char* relative_path)
    : TextParser(memory_resource, relative_path)
    , m_memory_resource(memory_resource)
    , m_root(nullptr)
{
    KW_ERROR(
        parse_recursive(&MarkdownReader::opt_spaces, &MarkdownReader::value),
        "Failed to parse markdown file \"%s\".", relative_path
    );

    //
    // Calculate the number of elements in root node.
    //

    size_t element_count = 0;

    Token* token = get_last();
    while (token != nullptr) {
        token = token->previous.get();
        element_count++;
    }

    //
    // Construct the root node.
    //

    Vector<UniquePtr<MarkdownNode>> elements(memory_resource);
    elements.reserve(element_count);

    token = get_last();
    while (token != nullptr) {
        elements.push_back(build_node_from_token(token));
        token = token->previous.get();
    }

    // Tokens are stored in reverse order.
    std::reverse(elements.begin(), elements.end());

    m_root = allocate_unique<ArrayNode>(m_memory_resource, std::move(elements));
}

MarkdownNode& MarkdownReader::operator[](size_t index) const {
    return (*m_root)[index];
}

size_t MarkdownReader::get_size() const {
    return m_root->get_size();
}

bool MarkdownReader::letter() {
    return parse_any_of("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz");
}

bool MarkdownReader::non_zero_digit() {
    return parse_any_of("123456789");
}

bool MarkdownReader::digit() {
    return parse('0') || parse(&MarkdownReader::non_zero_digit);
}

bool MarkdownReader::space() {
    return parse_any_of(" \t\n\v\f\r");
}

bool MarkdownReader::opt_digits() {
    return parse_recursive(&MarkdownReader::digit);
}

bool MarkdownReader::opt_spaces() {
    return parse_recursive(&MarkdownReader::space);
}

bool MarkdownReader::opt_minus() {
    return parse('-') || true;
}

bool MarkdownReader::spaces() {
    return parse(&MarkdownReader::space, &MarkdownReader::opt_spaces);
}

bool MarkdownReader::real_number() {
    return parse(&MarkdownReader::int_number, '.', &MarkdownReader::digit, &MarkdownReader::opt_digits);
}

bool MarkdownReader::int_number() {
    return parse('0') || parse(&MarkdownReader::non_zero_digit, &MarkdownReader::opt_digits);
}

bool MarkdownReader::number() {
    return parse(&MarkdownReader::opt_minus, &MarkdownReader::real_number) ||
           parse(&MarkdownReader::opt_minus, &MarkdownReader::int_number);
}

bool MarkdownReader::escape_char() {
    return parse_any_of("\"\\tnvfr");
}

bool MarkdownReader::string_char() {
    return parse_any_but("\"\\\n\r") || parse('\\', &MarkdownReader::escape_char);
}

bool MarkdownReader::opt_string_chars() {
    return parse_recursive(&MarkdownReader::string_char);
}

bool MarkdownReader::string() {
    return parse('"', &MarkdownReader::opt_string_chars, '"');
}

bool MarkdownReader::boolean() {
    return parse("true") || parse("false");
}

bool MarkdownReader::key_char() {
    return parse(&MarkdownReader::letter) || parse('_') || parse(&MarkdownReader::digit);
}

bool MarkdownReader::opt_key_chars() {
    return parse_recursive(&MarkdownReader::key_char);
}

bool MarkdownReader::key_start_char() {
    return parse(&MarkdownReader::letter) || parse('_');
}

bool MarkdownReader::key() {
    return token<StringToken>(&MarkdownReader::key_start_char, &MarkdownReader::opt_key_chars);
}

bool MarkdownReader::value() {
    return token<NumberToken>(&MarkdownReader::number)   ||
           token<StringToken>(&MarkdownReader::string)   ||
           token<BooleanToken>(&MarkdownReader::boolean) ||
           token<ObjectToken>(&MarkdownReader::object)   ||
           token<ArrayToken>(&MarkdownReader::array);
}

bool MarkdownReader::key_value() {
    return parse(&MarkdownReader::key, &MarkdownReader::opt_spaces, ':', &MarkdownReader::opt_spaces, &MarkdownReader::value);
}

bool MarkdownReader::opt_space_separated_key_values() {
    return parse_recursive(&MarkdownReader::spaces, &MarkdownReader::key_value);
}

bool MarkdownReader::opt_key_values() {
    return parse(&MarkdownReader::key_value, &MarkdownReader::opt_space_separated_key_values) || true;
}

bool MarkdownReader::opt_space_separated_values() {
    return parse_recursive(&MarkdownReader::spaces, &MarkdownReader::value);
}

bool MarkdownReader::opt_values() {
    return parse(&MarkdownReader::value, &MarkdownReader::opt_space_separated_values) || true;
}

bool MarkdownReader::object() {
    return parse('{', &MarkdownReader::opt_spaces, &MarkdownReader::opt_key_values, &MarkdownReader::opt_spaces, '}');
}

bool MarkdownReader::array() {
    return parse('[', &MarkdownReader::opt_spaces, &MarkdownReader::opt_values, &MarkdownReader::opt_spaces, ']');
}

UniquePtr<MarkdownNode> MarkdownReader::build_node_from_token(Token* token) {
    KW_ASSERT(token != nullptr, "Invalid token.");

    switch (token->get_type()) {
    case MarkdownType::NUMBER:
        return static_pointer_cast<MarkdownNode>(allocate_unique<NumberNode>(m_memory_resource, static_cast<NumberToken*>(token)->value));
    case MarkdownType::STRING:
        return static_pointer_cast<MarkdownNode>(allocate_unique<StringNode>(m_memory_resource, String(static_cast<StringToken*>(token)->value, m_memory_resource)));
    case MarkdownType::BOOLEAN:
        return static_pointer_cast<MarkdownNode>(allocate_unique<BooleanNode>(m_memory_resource, static_cast<BooleanToken*>(token)->value));
    case MarkdownType::OBJECT: {
        Map<String, UniquePtr<MarkdownNode>, ObjectNode::TransparentLess> elements(m_memory_resource);

        Token* value_token = token->last.get();
        while (value_token != nullptr) {
            // Tokens are stored in reverse order.
            Token* key_token = value_token->previous.get();
            KW_ASSERT(key_token != nullptr, "Invalid object key token.");
            KW_ASSERT(key_token->get_type() == MarkdownType::STRING, "Invalid object key token.");

            auto [_, success] = elements.emplace(String(static_cast<StringToken*>(key_token)->value, m_memory_resource), build_node_from_token(value_token));
            KW_ASSERT(success, "Object key already exists.");

            value_token = key_token->previous.get();
        }

        return static_pointer_cast<MarkdownNode>(allocate_unique<ObjectNode>(m_memory_resource, std::move(elements)));
    }
    case MarkdownType::ARRAY: {
        //
        // Calculate the number of elements in the node.
        //

        size_t element_count = 0;

        Token* element_token = token->last.get();
        while (element_token != nullptr) {
            element_token = element_token->previous.get();
            element_count++;
        }

        //
        // Construct the node.
        //

        Vector<UniquePtr<MarkdownNode>> elements(m_memory_resource);
        elements.reserve(element_count);

        element_token = token->last.get();
        while (element_token != nullptr) {
            elements.push_back(build_node_from_token(element_token));
            element_token = element_token->previous.get();
        }

        // Tokens are stored in reverse order.
        std::reverse(elements.begin(), elements.end());

        return static_pointer_cast<MarkdownNode>(allocate_unique<ArrayNode>(m_memory_resource, std::move(elements)));
    }
    default:
        KW_ASSERT(false, "Invalid token type.");
        return UniquePtr<MarkdownNode>();
    }
}

} // namespace kw
