// src/tokenizer.cpp
// Hand-written scanner for cminux v0.1.
//
// The scanner is a single pass over the source, producing tokens as it goes.
// It never backtracks and never buffers more than one token ahead.

#include "tokenizer.hpp"
#include <cctype>
#include <cstdint>

namespace cmx {

namespace {

struct Cursor {
    const std::string& src;
    size_t i = 0;
    int line = 1;
    int col = 1;

    explicit Cursor(const std::string& s) : src(s) {}

    bool eof() const { return i >= src.size(); }
    char peek() const { return eof() ? '\0' : src[i]; }
    char peek_next() const { return (i + 1 < src.size()) ? src[i + 1] : '\0'; }

    char advance() {
        char c = src[i++];
        if (c == '\n') { ++line; col = 1; }
        else           { ++col; }
        return c;
    }
};

bool is_ident_start(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}
bool is_ident_cont(char c) {
    return is_ident_start(c) || (c >= '0' && c <= '9');
}
bool is_digit(char c) {
    return c >= '0' && c <= '9';
}

// All multi-character punctuation cminux recognizes. Longer entries must
// come before their prefixes, so we test them longest-first.
const char* const kPuncts[] = {
    // 3-char
    "...",
    "<<=", ">>=",
    // 2-char
    "==", "!=", "<=", ">=", "&&", "||", "->", "::", "<<", ">>",
    "+=", "-=", "*=", "/=", "%=", "&=", "|=", "^=", "++", "--",
    // 1-char (anything not matched above, printed as a single char)
};

const char* match_punct(const std::string& s, size_t i) {
    for (const char* p : kPuncts) {
        size_t n = std::char_traits<char>::length(p);
        if (i + n <= s.size() && s.compare(i, n, p) == 0) {
            return p;
        }
    }
    return nullptr;
}

} // namespace

bool tokenize(const std::string& source,
              const std::string& filename,
              std::vector<Token>& out,
              TokenizeError& err) {
    (void)filename;
    out.clear();
    Cursor c(source);

    while (!c.eof()) {
        char ch = c.peek();

        // Skip horizontal whitespace; vertical whitespace is a Newline token.
        if (ch == ' ' || ch == '\t' || ch == '\r') {
            c.advance();
            continue;
        }

        if (ch == '\n') {
            int ln = c.line, cl = c.col;
            c.advance();
            out.emplace_back(TokKind::Newline, "\n", ln, cl);
            continue;
        }

        // Line comment: // ... to end of line. The newline is still emitted.
        if (ch == '/' && c.peek_next() == '/') {
            while (!c.eof() && c.peek() != '\n') c.advance();
            continue;
        }

        // Block comment: /* ... */. Newlines inside are preserved so line
        // numbers downstream stay correct.
        if (ch == '/' && c.peek_next() == '*') {
            int start_line = c.line, start_col = c.col;
            c.advance(); c.advance();
            bool closed = false;
            while (!c.eof()) {
                if (c.peek() == '*' && c.peek_next() == '/') {
                    c.advance(); c.advance();
                    closed = true;
                    break;
                }
                if (c.peek() == '\n') {
                    int ln = c.line, cl = c.col;
                    c.advance();
                    out.emplace_back(TokKind::Newline, "\n", ln, cl);
                } else {
                    c.advance();
                }
            }
            if (!closed) {
                err.message = "unterminated block comment";
                err.line = start_line;
                err.col = start_col;
                return false;
            }
            continue;
        }

        // String literal
        if (ch == '"') {
            int ln = c.line, cl = c.col;
            c.advance(); // opening quote
            std::string value;
            bool closed = false;
            while (!c.eof()) {
                char s = c.advance();
                if (s == '"') { closed = true; break; }
                if (s == '\\') {
                    if (c.eof()) break;
                    char e = c.advance();
                    if (e == '"' || e == '\\') value.push_back(e);
                    else { value.push_back('\\'); value.push_back(e); }
                } else if (s == '\n') {
                    err.message = "unterminated string literal";
                    err.line = ln;
                    err.col = cl;
                    return false;
                } else {
                    value.push_back(s);
                }
            }
            if (!closed) {
                err.message = "unterminated string literal";
                err.line = ln;
                err.col = cl;
                return false;
            }
            out.emplace_back(TokKind::String, std::move(value), ln, cl);
            continue;
        }

        // Identifier or keyword
        if (is_ident_start(ch)) {
            int ln = c.line, cl = c.col;
            std::string word;
            while (!c.eof() && is_ident_cont(c.peek())) {
                word.push_back(c.advance());
            }
            TokKind k = is_cminux_keyword(word) ? TokKind::Keyword : TokKind::Ident;
            out.emplace_back(k, std::move(word), ln, cl);
            continue;
        }

        // Number (integer or decimal; no exponent, no hex, no suffix)
        if (is_digit(ch)) {
            int ln = c.line, cl = c.col;
            std::string num;
            while (!c.eof() && is_digit(c.peek())) num.push_back(c.advance());
            if (c.peek() == '.' && is_digit(c.peek_next())) {
                num.push_back(c.advance());
                while (!c.eof() && is_digit(c.peek())) num.push_back(c.advance());
            }
            out.emplace_back(TokKind::Number, std::move(num), ln, cl);
            continue;
        }

        // Punctuation — try longest match first
        if (const char* p = match_punct(source, c.i)) {
            int ln = c.line, cl = c.col;
            std::string tok = p;
            for (size_t k = 0; k < tok.size(); ++k) c.advance();
            out.emplace_back(TokKind::Punct, std::move(tok), ln, cl);
            continue;
        }

        // Single-character punctuation: everything else that isn't
        // whitespace, a comment, a string, an identifier, or a number.
        if (ch == '(' || ch == ')' || ch == '{' || ch == '}' ||
            ch == '[' || ch == ']' || ch == ';' || ch == ',' ||
            ch == ':' || ch == '.' || ch == '+' || ch == '-' ||
            ch == '*' || ch == '/' || ch == '%' || ch == '=' ||
            ch == '!' || ch == '<' || ch == '>' || ch == '&' ||
            ch == '|' || ch == '^' || ch == '~' || ch == '?') {
            int ln = c.line, cl = c.col;
            std::string tok(1, c.advance());
            out.emplace_back(TokKind::Punct, std::move(tok), ln, cl);
            continue;
        }

        // Anything else is a hard error. There is no "unknown character"
        // recovery; the user should never see this from valid cminux source.
        err.message = std::string("unexpected character '") + ch + "'";
        err.line = c.line;
        err.col = c.col;
        return false;
    }

    out.emplace_back(TokKind::End, "", c.line, c.col);
    return true;
}

} // namespace cmx