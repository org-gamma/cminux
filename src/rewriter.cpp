// src/rewriter.cpp
// The one and only rewrite pass in cminux.
//
// Structure: walk the input tokens once. Most rules are pure substitutions
// on the current token. Five rules perform a forward scan:
//
//   func      balanced-paren scan of the parameter list, optional `->` scan
//   if        balanced scan of the condition, with or without parens
//   while     balanced scan of the condition, with or without parens
//   for-in    balanced scan of the container expression
//   loop      no scan; emits `while (true)` and lets the body copy
//
// Every rule is type-agnostic: the rewriter copies tokens without
// interpreting them. It never parses expressions, never resolves names,
// never checks types.
//
// New in this version:
//   - and / or / not as aliases for && / || / !
//   - loop { ... } as shorthand for while (true) { ... }
//   - if and while without parentheses
//   - for x in v { ... } as shorthand for the C++ range-for
//
// All additions are local and type-agnostic. The design rules from 1.0.0
// hold.

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

// ---------------------------------------------------------------------------
// import [ "a", "b" ]  ->  #include "a"\n#include "b"
// ---------------------------------------------------------------------------

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

// ---------------------------------------------------------------------------
// func <name> ( <params> ) [ -> <type> ] <body>
// ---------------------------------------------------------------------------

bool try_func(Cursor& c, std::vector<Token>& out, RewriteError& err) {
    const Token& kw = c.cur();
    c.advance(); // func

    if (!c.cur().is(TokKind::Ident)) {
        err.message = "expected function name after 'func'";
        err.line = kw.line; err.col = kw.col;
        return false;
    }
    Token name_tok = c.cur();
    c.advance();

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

    std::vector<Token> return_type;
    bool has_arrow = false;
    if (c.cur().is(TokKind::Punct, "->")) {
        has_arrow = true;
        c.advance();
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

    if (!has_arrow) {
        out.emplace_back(TokKind::Ident, "void", kw.line, kw.col);
        out.emplace_back(TokKind::Punct, " ", kw.line, kw.col);
    } else {
        for (const auto& t : return_type) out.push_back(t);
        out.emplace_back(TokKind::Punct, " ", kw.line, kw.col);
    }

    out.push_back(name_tok);
    for (const auto& t : params) out.push_back(t);

    return true;
}

// ---------------------------------------------------------------------------
// if <cond> <body>   ->  if ( <cond> ) <body>
//
// If the token immediately after `if` is `(`, the existing parens are
// preserved and only the body is copied. Otherwise the condition is
// wrapped in synthetic parens.
//
// The scan for the condition ends at the first `{` at depth 0. Parens,
// brackets, and braces inside the condition (function calls, initializer
// lists, etc.) are tracked so that a `{` inside a nested block does not
// prematurely end the scan. A `;` at depth 0 means the user wrote a
// statement without a body, which is a C++ error — but it is not a
// cminux error, so we copy it through and let the C++ compiler report it.
// ---------------------------------------------------------------------------

bool try_if(Cursor& c, std::vector<Token>& out, RewriteError& err) {
    (void)err;
    const Token& kw = c.cur();       // `if`
    c.advance();

    bool has_paren = c.cur().is(TokKind::Punct, "(");

    emit_ident(out, kw, "if");

    if (has_paren) {
        // Copy the existing parenthesized condition. Balanced scan.
        int depth = 0;
        while (!c.at_end()) {
            const Token& t = c.cur();
            out.push_back(t);
            if (t.is(TokKind::Punct, "(")) ++depth;
            else if (t.is(TokKind::Punct, ")")) {
                --depth;
                c.advance();
                if (depth == 0) break;
                continue;
            }
            c.advance();
        }
        return true;
    }

    // No parens: wrap the condition.
    emit_punct(out, kw, "(");
    int paren = 0, brace = 0, bracket = 0;
    while (!c.at_end()) {
        const Token& t = c.cur();
        if (t.is(TokKind::Punct, "{")) {
            if (paren == 0 && brace == 0 && bracket == 0) break;
            ++brace;
        } else if (t.is(TokKind::Punct, "}")) {
            if (brace > 0) --brace;
        } else if (t.is(TokKind::Punct, "(")) ++paren;
        else if (t.is(TokKind::Punct, ")")) { if (paren > 0) --paren; }
        else if (t.is(TokKind::Punct, "[")) ++bracket;
        else if (t.is(TokKind::Punct, "]")) { if (bracket > 0) --bracket; }

        if (t.is(TokKind::Punct, ";") && paren == 0 && brace == 0 && bracket == 0) {
            // No body; copy through and let C++ report the error.
            break;
        }

        out.push_back(t);
        c.advance();
    }
    emit_punct(out, kw, ")");
    return true;
}

// ---------------------------------------------------------------------------
// while <cond> <body>  ->  while ( <cond> ) <body>
//
// Same structure as try_if. The scan and wrapping logic are identical;
// the only difference is the keyword that is emitted.
// ---------------------------------------------------------------------------

bool try_while(Cursor& c, std::vector<Token>& out, RewriteError& err) {
    (void)err;
    const Token& kw = c.cur();       // `while`
    c.advance();

    bool has_paren = c.cur().is(TokKind::Punct, "(");

    emit_ident(out, kw, "while");

    if (has_paren) {
        int depth = 0;
        while (!c.at_end()) {
            const Token& t = c.cur();
            out.push_back(t);
            if (t.is(TokKind::Punct, "(")) ++depth;
            else if (t.is(TokKind::Punct, ")")) {
                --depth;
                c.advance();
                if (depth == 0) break;
                continue;
            }
            c.advance();
        }
        return true;
    }

    emit_punct(out, kw, "(");
    int paren = 0, brace = 0, bracket = 0;
    while (!c.at_end()) {
        const Token& t = c.cur();
        if (t.is(TokKind::Punct, "{")) {
            if (paren == 0 && brace == 0 && bracket == 0) break;
            ++brace;
        } else if (t.is(TokKind::Punct, "}")) {
            if (brace > 0) --brace;
        } else if (t.is(TokKind::Punct, "(")) ++paren;
        else if (t.is(TokKind::Punct, ")")) { if (paren > 0) --paren; }
        else if (t.is(TokKind::Punct, "[")) ++bracket;
        else if (t.is(TokKind::Punct, "]")) { if (bracket > 0) --bracket; }

        if (t.is(TokKind::Punct, ";") && paren == 0 && brace == 0 && bracket == 0) {
            break;
        }

        out.push_back(t);
        c.advance();
    }
    emit_punct(out, kw, ")");
    return true;
}

// ---------------------------------------------------------------------------
// loop <body>  ->  while ( true ) <body>
//
// `loop` is a fixed substitution. It takes no condition; the body follows
// immediately. Break out with `break` (which passes through as a C++
// keyword). There is no `loop <cond>` form; use `while` for that.
// ---------------------------------------------------------------------------

void emit_loop(Cursor& c, std::vector<Token>& out) {
    const Token& kw = c.cur();
    c.advance();
    emit_ident(out, kw, "while");
    emit_punct(out, kw, "(");
    emit_ident(out, kw, "true");
    emit_punct(out, kw, ")");
}

// ---------------------------------------------------------------------------
// for <var> in <expr> <body>  ->  for ( auto& <var> : <expr> ) <body>
//
// The for-in form scans from `in` to the opening `{` of the body, wrapping
// the loop variable and the container expression in the C++ range-for
// form. The loop variable is emitted by reference (`auto&`), so
// modifications inside the loop affect the container. To iterate by
// value, write the C++ range-for directly: `for (auto x : v)`.
//
// The container expression is copied verbatim with balanced-paren and
// balanced-bracket tracking. A `;` at depth 0 means the user wrote a
// C-style for, which cminux does not rewrite; the `for` keyword is
// preserved and the rest of the statement is copied as-is. That is
// intentional: cminux does not have a C-style for, but it does not
// break existing C++ for loops that appear in .cmx source either.
// ---------------------------------------------------------------------------

bool try_for(Cursor& c, std::vector<Token>& out, RewriteError& err) {
    const Token& kw = c.cur();       // `for`
    c.advance();

    // Look ahead: is this `for x in ...` or a C-style `for (...)`?
    // The for-in form has an identifier followed by the `in` keyword
    // before any `(` or `;`. The C-style form starts with `(`.
    if (!c.cur().is(TokKind::Ident) || !c.peek(1).is(TokKind::Keyword, "in")) {
        // C-style or malformed. Emit `for` and let the rest pass through.
        emit_ident(out, kw, "for");
        return true;
    }

    Token var = c.cur();
    c.advance();                     // var
    c.advance();                     // in

    // Scan the container expression until the opening `{` of the body.
    std::vector<Token> container;
    int paren = 0, bracket = 0;
    while (!c.at_end()) {
        const Token& t = c.cur();
        if (t.is(TokKind::Punct, "{") && paren == 0 && bracket == 0) break;
        if (t.is(TokKind::Punct, ";") && paren == 0 && bracket == 0) {
            err.message = "expected '{' after for-in container expression";
            err.line = t.line; err.col = t.col;
            return false;
        }
        if (t.is(TokKind::Punct, "(")) ++paren;
        else if (t.is(TokKind::Punct, ")")) { if (paren > 0) --paren; }
        else if (t.is(TokKind::Punct, "[")) ++bracket;
        else if (t.is(TokKind::Punct, "]")) { if (bracket > 0) --bracket; }

        container.push_back(t);
        c.advance();
    }

    if (container.empty()) {
        err.message = "expected container expression after 'in'";
        err.line = kw.line; err.col = kw.col;
        return false;
    }

    // Emit: for ( auto& <var> : <container> )
    emit_ident(out, kw, "for");
    emit_punct(out, kw, "(");
    emit_ident(out, kw, "auto");
    emit_punct(out, kw, "&");
    out.emplace_back(TokKind::Punct, " ", kw.line, kw.col);
    out.push_back(var);
    emit_punct(out, kw, " ");
    emit_punct(out, kw, ":");
    emit_punct(out, kw, " ");
    for (const auto& t : container) out.push_back(t);
    emit_punct(out, kw, ")");

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

        if (t.is(TokKind::Keyword, "if")) {
            if (!try_if(c, out, err)) return false;
            continue;
        }

        if (t.is(TokKind::Keyword, "while")) {
            if (!try_while(c, out, err)) return false;
            continue;
        }

        if (t.is(TokKind::Keyword, "for")) {
            if (!try_for(c, out, err)) return false;
            continue;
        }

        if (t.is(TokKind::Keyword, "loop")) {
            emit_loop(c, out);
            continue;
        }

        if (t.is(TokKind::Keyword, "render")) {
            emit_ident(out, t, "return");
            c.advance();
            continue;
        }

        if (t.is(TokKind::Keyword, "and")) {
            emit_punct(out, t, "&&");
            c.advance();
            continue;
        }

        if (t.is(TokKind::Keyword, "or")) {
            emit_punct(out, t, "||");
            c.advance();
            continue;
        }

        if (t.is(TokKind::Keyword, "not")) {
            emit_punct(out, t, "!");
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