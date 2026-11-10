// src/rewriter.cpp
// The one and only rewrite pass in cminux v1.0.0.
//
// Structure: walk the input tokens once. Most rules are pure substitutions
// on the current token. One rule — `func` — performs a balanced-bracket
// scan to relocate a trailing return type (`-> T`) to the front of the
// declaration. Every rule remains type-agnostic: the rewriter copies type
// tokens without interpreting them.

#include "rewriter.hpp"
#include <cstring>

namespace cmx {

namespace {

struct Cursor {
    const std::vector<Token>& toks;
    size_t i = 0;
    explicit Cursor(const std::vector<Token>& t) : toks(t) {}

    const Token& cur() const { return toks[i]; }
    bool at_end() const { return cur().is(TokKind::End); }

    const Token& peek(size_t n) const {
        size_t j = i + n;
        if (j >= toks.size()) return toks.back();
        return toks[j];
    }

    void advance() { if (i + 1 < toks.size()) ++i; }
};

void emit_punct(std::vector<Token>& out, const Token& src, const std::string& text) {
    out.emplace_back(TokKind::Punct, text, src.line, src.col);
}

void emit_ident(std::vector<Token>& out, const Token& src, const std::string& text) {
    out.emplace_back(TokKind::Ident, text, src.line, src.col);
}

void emit_synthetic_include(std::vector<Token>& out, const Token& src, const std::string& path) {
    emit_punct(out, src, "#include");
    emit_punct(out, src, "\"");
    out.emplace_back(TokKind::Ident, path, src.line, src.col);
    emit_punct(out, src, "\"");
}

// import [ "a", "b" ]  ->  #include "a"\n#include "b"
bool try_import(Cursor& c, std::vector<Token>& out, RewriteError& err) {
    const Token& kw = c.cur();
    c.advance(); // import

    if (!c.cur().is(TokKind::Punct, "[")) {
        err.message = "expected '[' after 'import'";
        err.line = kw.line; err.col = kw.col;
        return false;
    }
    c.advance(); // [

    std::vector<std::string> paths;
    while (true) {
        if (c.cur().is(TokKind::Punct, "]")) { c.advance(); break; }
        if (!c.cur().is(TokKind::String)) {
            err.message = "expected string literal in import list";
            err.line = c.cur().line; err.col = c.cur().col;
            return false;
        }
        paths.push_back(c.cur().text);
        c.advance();
        if (c.cur().is(TokKind::Punct, ",")) { c.advance(); continue; }
        if (c.cur().is(TokKind::Punct, "]")) { c.advance(); break; }
        err.message = "expected ',' or ']' in import list";
        err.line = c.cur().line; err.col = c.cur().col;
        return false;
    }

    bool had_semi = c.cur().is(TokKind::Punct, ";");
    if (had_semi) c.advance();

    for (const auto& p : paths) {
        emit_synthetic_include(out, kw, p);
        out.emplace_back(TokKind::Newline, "\n", kw.line, kw.col);
    }
    if (!had_semi) {
        out.emplace_back(TokKind::Newline, "\n", kw.line, kw.col);
    }
    return true;
}

// func <name> ( <params> ) [ -> <type> ] <body>
//   ->   <type> <name> ( <params> ) <body>       (with ->)
//   ->   void   <name> ( <params> ) <body>       (without ->)
//
// This rule performs a balanced-parenthesis scan over the parameter list,
// then an optional return-type scan over the tokens after `->` up to the
// body's opening `{`. It copies tokens verbatim; it never interprets types.
//
// On malformed input (missing name, missing paren, missing `{`), it fills
// `err` and returns false. The driver aborts. We do not attempt recovery:
// the user should never see this from valid cminux source.
bool try_func(Cursor& c, std::vector<Token>& out, RewriteError& err) {
    const Token& kw = c.cur();       // the `func` keyword
    c.advance();                     // consume func

    // ---- 1. Function name ------------------------------------------------
    if (!c.cur().is(TokKind::Ident)) {
        err.message = "expected function name after 'func'";
        err.line = kw.line; err.col = kw.col;
        return false;
    }
    Token name_tok = c.cur();
    c.advance();

    // ---- 2. Parameter list — scan with balanced parens -------------------
    if (!c.cur().is(TokKind::Punct, "(")) {
        err.message = "expected '(' after function name";
        err.line = c.cur().line; err.col = c.cur().col;
        return false;
    }
    std::vector<Token> params;
    int depth = 0;
    bool got_close = false;
    while (!c.at_end()) {
        const Token& t = c.cur();
        params.push_back(t);
        if (t.is(TokKind::Punct, "(")) ++depth;
        else if (t.is(TokKind::Punct, ")")) {
            --depth;
            if (depth == 0) { got_close = true; c.advance(); break; }
        }
        c.advance();
    }
    if (!got_close) {
        err.message = "unterminated parameter list";
        err.line = kw.line; err.col = kw.col;
        return false;
    }

    // ---- 3. Optional `-> Type` ------------------------------------------
    std::vector<Token> return_type;
    bool has_arrow = false;
    if (c.cur().is(TokKind::Punct, "->")) {
        has_arrow = true;
        c.advance(); // ->
        // Read tokens until the body's opening `{`. We do not cross `;`
        // because that would mean a declaration without a body, which
        // cminux v1.0.0 does not have (no forward declarations).
        while (!c.at_end()) {
            const Token& t = c.cur();
            if (t.is(TokKind::Punct, "{")) break;
            if (t.is(TokKind::Punct, ";")) {
                err.message = "function declaration without body is not supported";
                err.line = t.line; err.col = t.col;
                return false;
            }
            return_type.push_back(t);
            c.advance();
        }
        if (return_type.empty()) {
            err.message = "expected return type after '->'";
            err.line = kw.line; err.col = kw.col;
            return false;
        }
    }

    // ---- 4. Emit: <type> <name> ( <params> ) ----------------------------
    // No arrow: synthesize `void` as the return type.
    if (!has_arrow) {
        out.emplace_back(TokKind::Ident, "void", kw.line, kw.col);
        out.emplace_back(TokKind::Punct, " ", kw.line, kw.col); // spacer
    } else {
        for (const auto& t : return_type) out.push_back(t);
        out.emplace_back(TokKind::Punct, " ", kw.line, kw.col); // spacer
    }

    out.push_back(name_tok);
    // The parameter vector already includes the opening `(` and closing `)`.
    // But we want no space between name and `(` — the emitter's needs_space
    // rule handles that (word followed by `(` -> no space).
    for (const auto& t : params) out.push_back(t);

    // The caller's main loop continues from c.cur(), which is now `{`
    // (or `;` on the no-body error path, already returned).
    return true;
}

} // namespace

bool rewrite(const std::vector<Token>& in,
             std::vector<Token>& out,
             RewriteError& err) {
    out.clear();
    Cursor c(in);

    while (!c.at_end()) {
        const Token& t = c.cur();

        if (t.is(TokKind::Keyword, "import")) {
            if (!try_import(c, out, err)) return false;
            continue;
        }

        if (t.is(TokKind::Keyword, "func")) {
            if (!try_func(c, out, err)) return false;
            continue;
        }

        if (t.is(TokKind::Keyword, "render")) {
            emit_ident(out, t, "return");
            c.advance();
            continue;
        }

        if (t.is(TokKind::Keyword, "nil")) {
            emit_ident(out, t, "nullptr");
            c.advance();
            continue;
        }

        if (t.is(TokKind::Keyword, "let")) {
            emit_ident(out, t, "const");
            emit_ident(out, t, "auto");
            c.advance();
            continue;
        }

        if (t.is(TokKind::Keyword, "var")) {
            emit_ident(out, t, "auto");
            c.advance();
            continue;
        }

        out.push_back(t);
        c.advance();
    }

    out.emplace_back(TokKind::End, "", in.empty() ? 1 : in.back().line,
                                      in.empty() ? 1 : in.back().col);
    return true;
}

} // namespace cmx