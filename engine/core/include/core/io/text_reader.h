#pragma once

#include "core/containers/string.h"
#include "core/containers/unique_ptr.h"
#include "core/error.h"
#include "core/memory/memory_resource.h"

#include <fstream>
#include <type_traits>

namespace kw {

// Allows to parse and tokenize a text file. Designed primarily for Backus–Naur form.
template <typename Child>
class TextParser {
protected:
    class Token {
    public:
        virtual ~Token() = default;

        // No-op by default.
        virtual void init(const char* begin, const char* end);

        // Previous token in the same hieratchy.
        UniquePtr<Token> previous;

        // The last child token.
        UniquePtr<Token> last;
    };

    // Throw error if failed to read the file.
    TextParser(MemoryResource& memory_resource, const char* relative_path);

    // Check whether the next symbol is `c`. Advance stream on success.
    bool parse(char c);

    // Check whether the next symbols are `string`. Advance stream on success.
    bool parse(const char* string);

    // Execute the given function. Undo stream and tokens on failure.
    bool parse(bool (Child::*func)());

    // x ::= arg args...
    template <typename Arg, typename... Args>
    bool parse(Arg arg, Args... args);

    // x ::= arg x | ""
    template <typename Arg>
    bool parse_recursive(Arg arg);

    // x ::= arg args... x | ""
    template <typename Arg, typename... Args>
    bool parse_recursive(Arg arg, Args... args);

    // Check whether the next symbol is any of `string`. Advance stream on success.
    bool parse_any_of(const char* string);

    // Check whether the next symbol is not in `string`. Advance stream on success.
    bool parse_any_but(const char* string);

    // `parse`, but with token enqueuing on success. Tokens within `parse` will be children of a newly created token.
    template <typename T, typename... Args>
    bool token(Args... args);

    // Return the last top level token.
    Token* get_last() const;

private:
    class RootToken;

    void pop_until(Token* token);

    MemoryResource& m_memory_resource;
    String m_data;
    const char* m_current;
    UniquePtr<Token> m_token;
};

} // namespace kw

#include "core/io/impl/text_reader_impl.h"
