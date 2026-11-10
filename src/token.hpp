// src/token.hpp
// The complete token vocabulary of cminux v0.1.
//
// A token is either a keyword, an identifier, a string literal, a number,
// or a punctuation mark. There are no other token kinds. If a new construct
// in cminux source requires a new token kind, add it here and to the
// tokenizer; do not invent a new data structure in the rewriter.

#ifndef CMINUX_TOKEN_HPP
#define CMINUX_TOKEN_HPP

#include <string>
#include <cstdint>

namespace cmx {

enum class TokKind : uint8_t {
    End,        // end of input
    Newline,    // preserved so line structure survives the rewrite
    Keyword,    // one of the reserved words in keywords() below
    Ident,      // [A-Za-z_][A-Za-z0-9_]*
    String,     // "..." — the value excludes the surrounding quotes
    Number,     // 123, 4.5
    Punct,      // any operator or punctuation run
};

struct Token {
    TokKind     kind;
    std::string text;   // for String, text is the raw content without quotes
    int         line;   // 1-based line number of the token's start
    int         col;    // 1-based column of the token's start

    Token() : kind(TokKind::End), line(1), col(1) {}
    Token(TokKind k, std::string t, int ln, int cl)
        : kind(k), text(std::move(t)), line(ln), col(cl) {}

    bool is(TokKind k) const { return kind == k; }
    bool is(TokKind k, const std::string& s) const {
        return kind == k && text == s;
    }
};

// The keyword set. Every reserved word in cminux v0.1.
// Adding to this list is a breaking change for cminux source files that
// used the word as an identifier. Do not extend it lightly.
inline bool is_cminux_keyword(const std::string& s) {
    return s == "import"
        || s == "func"
        || s == "render"
        || s == "print"
        || s == "println"
        || s == "if"
        || s == "else"
        || s == "while"
        || s == "for"
        || s == "return"
        || s == "int"
        || s == "long"
        || s == "float"
        || s == "double"
        || s == "bool"
        || s == "char"
        || s == "string"
        || s == "void"
        || s == "true"
        || s == "false"
        || s == "nil"
        || s == "let"
        || s == "var";
}

} // namespace cmx

#endif // CMINUX_TOKEN_HPP