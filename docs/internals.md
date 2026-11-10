# cminux Internals

**Version:** 1.0.0

This document describes how cminux works internally, for anyone modifying
the transpiler, adding a tenth rewrite rule, or debugging an issue in the
pipeline. It is not needed to use cminux — see `docs/language-reference.md`
for that.

Every invariant described here is load-bearing. Breaking an invariant means
breaking a guarantee that some later stage relies on. If you find yourself
wanting to break one, read the "Why this matters" note carefully and
consider whether the change belongs in cminux at all.

---

## Table of contents

1. [Source layout](#1-source-layout)
2. [The pipeline](#2-the-pipeline)
3. [Data structures](#3-data-structures)
4. [Token stream invariants](#4-token-stream-invariants)
5. [The tokenizer](#5-the-tokenizer)
6. [The rewriter](#6-the-rewriter)
7. [The emitter](#7-the-emitter)
8. [The compiler driver](#8-the-compiler-driver)
9. [The top-level driver](#9-the-top-level-driver)
10. [Adding a new rule](#10-adding-a-new-rule)
11. [Adding a new keyword](#11-adding-a-new-keyword)
12. [Testing](#12-testing)
13. [Things cminux deliberately does not do](#13-things-cminux-deliberately-does-not-do)

---

## 1. Source layout

```
src/
├── main.cpp          CLI entry point, argument parsing
├── token.hpp         Token struct, TokKind enum, keyword list
├── tokenizer.hpp     tokenize() declaration, TokenizeError
├── tokenizer.cpp     hand-written scanner
├── rewriter.hpp      rewrite() declaration, RewriteError
├── rewriter.cpp      all rewrite rules
├── emitter.hpp       emit_cpp() declaration, EmitOptions
├── emitter.cpp       prologue + token printer
├── compiler.hpp      compile_to_exe() declaration, CompileOptions
├── compiler.cpp      shell-out to C++ compiler
├── driver.hpp        DriverOptions struct, run() declaration
└── driver.cpp        pipeline orchestration
```

Each header exposes one function and one struct. Each `.cpp` implements one
stage. No stage depends on any other stage's `.cpp` — only its `.hpp`. The
dependency graph is a straight line:

```
main.cpp → driver.hpp → tokenizer.hpp → token.hpp
              ↓             ↓
         rewriter.hpp ← tokenizer.hpp
              ↓
          emitter.hpp ← token.hpp
              ↓
         compiler.hpp
```

`token.hpp` is the only header included by more than one stage. It defines
`Token` and `TokKind`, which every stage passes around.

The library is separate:

```
lib/
├── outfn.hpp         the entire output library
└── src/
    └── outfn.cpp     anchor translation unit
```

The library is compiled as a static library (`cminux_outfn`) and linked
into the cminux executable. It is also the header that every transpiled
`.cpp` includes via the prologue.

---

## 2. The pipeline

The pipeline is a sequence of function calls, each taking the previous
stage's output and producing the next stage's input:

```cpp
// driver.cpp, in cmx::run()
bool ok;
std::string source = read_file(opts.input_path, ok);
if (!ok) return 1;

std::vector<Token> toks;
TokenizeError terr;
if (!tokenize(source, opts.input_path, toks, terr)) return 1;

std::vector<Token> rewritten;
RewriteError rerr;
if (!rewrite(toks, rewritten, rerr)) return 1;

EmitOptions eopts;
eopts.source_filename = opts.input_path;
std::string cpp = emit_cpp(rewritten, eopts);

write_file(temp_cpp_path(opts.input_path), cpp);

CompileOptions copts;
// ... fill in ...
std::string cerr_msg;
if (!compile_to_exe(copts, cerr_msg)) return 1;

return 0;
```

Each stage is pure: it takes its input, returns its output, and does not
modify global state. `tokenize` and `rewrite` fill an error struct on
failure. `emit_cpp` cannot fail. `compile_to_exe` returns a bool.

The driver owns the temp file lifecycle. `read_file` and `write_file` are
local to `driver.cpp` — they are not exposed in any header, because no
other stage needs to read or write files.

---

## 3. Data structures

Two structs and one enum make up the entire data model.

### 3.1 `Token`

```cpp
struct Token {
    TokKind     kind;
    std::string text;
    int         line;
    int         col;
};
```

`kind` determines how `text` should be interpreted:

- `End`: text is empty. Marks end of stream.
- `Newline`: text is `"\n"`. Marks a line boundary.
- `Keyword`: text is one of the reserved words in `is_cminux_keyword`.
- `Ident`: text is a valid identifier.
- `String`: text is the string content with quotes stripped and `\"` / `\\`
  resolved. Other backslash sequences are preserved verbatim.
- `Number`: text is a decimal integer or decimal.
- `Punct`: text is one of the punctuation tokens.

`line` and `col` are 1-based and indicate the position of the first
character of the token in the source.

### 3.2 `TokKind`

```cpp
enum class TokKind : uint8_t {
    End,
    Newline,
    Keyword,
    Ident,
    String,
    Number,
    Punct,
};
```

Seven kinds. This is the complete vocabulary. Adding a kind is a
significant change — it means the rewriter, the emitter, and every stage
that inspects `kind` must be updated. In v1.0.0, there is no case where
adding a kind would be the right solution; new syntax is expressed as
new rewrite rules over existing kinds.

### 3.3 Error structs

```cpp
struct TokenizeError {
    std::string message;
    int line;
    int col;
};

struct RewriteError {
    std::string message;
    int line;
    int col;
};
```

Both have identical shape. They are separate types because they are
produced by different stages and, in principle, could diverge. In practice
they are the same three fields.

The `message` is a human-readable error, printed to stderr by the driver.
It does not include the file name — that is added by `report_error` in
`driver.cpp`.

---

## 4. Token stream invariants

The tokenizer produces a stream that satisfies five invariants. Every
subsequent stage relies on all five. If you modify the tokenizer, you must
maintain them.

### 4.1 The last token is `End`

`tokenize` always appends a `TokKind::End` token at the end. The `End`
token's `line` and `col` are the position where the input ended.

**Why this matters:** Every scan loop terminates on `End`, so the loops do
not need to check for end-of-vector separately.

### 4.2 Every token has a valid line and col

Line and col are 1-based. The first token of the file is at line 1, col 1.
The `End` token has the line and col of the character after the last
character.

**Why this matters:** Error messages reference these fields. If they are
wrong, all error messages are wrong.

### 4.3 Newline tokens separate lines

A `TokKind::Newline` token appears after the last non-`Newline` token of
every source line. This includes the last line if it ends with a newline
(which is normal) or does not (in which case no trailing `Newline` is
appended for the last line, but `End` follows immediately).

**Why this matters:** The emitter prints `Newline` tokens as actual `\n`
characters, so line structure is preserved from source to output. Without
this invariant, the emitter would have to reconstruct line structure from
`line` fields, which is error-prone.

### 4.4 String literals have their quotes stripped

The `text` of a `String` token does not include the surrounding `"`. It
also has `\"` resolved to `"` and `\\` resolved to `\`. All other backslash
sequences are preserved.

**Why this matters:** The emitter must re-quote and re-escape strings when
printing them. The rule for re-quoting is: wrap in `"`, escape `"` and `\`.
The rule for re-escaping is: any backslash sequence other than `\"` and `\\`
is written as-is, so `\n` is written as `\n`, which the C++ compiler sees
as a newline. This round-trip is lossless.

### 4.5 No comments, no whitespace other than newlines

Spaces, tabs, and `\r` are consumed by the tokenizer and produce no tokens.
Line and block comments are consumed and produce no tokens (except the
`Newline` tokens for line breaks inside block comments).

**Why this matters:** The rewriter and emitter do not need to handle
whitespace or comments. Every token is either meaningful or a `Newline`.

---

## 5. The tokenizer

The tokenizer is a single pass over the source, in `src/tokenizer.cpp`.
It is about 250 lines.

### 5.1 Structure

```cpp
bool tokenize(const std::string& source, const std::string& filename,
              std::vector<Token>& out, TokenizeError& err);
```

The function does not take a filename for anything other than logging —
in v1.0.0, the filename is unused inside the tokenizer. The driver passes
it so future diagnostics could mention it, but all errors are reported by
the driver with the filename from `opts.input_path`. The tokenizer only
fills `err.line` and `err.col`; the driver adds the filename.

### 5.2 The cursor

The tokenizer maintains a `Cursor` over the source:

```cpp
struct Cursor {
    const std::string& src;
    size_t i;
    int line;
    int col;

    char peek() const;
    char peek_next() const;
    char advance();
    bool eof() const;
};
```

`advance()` is the only way to move forward. It updates `line` and `col`
correctly on `\n`. All scanning is done via `peek`, `peek_next`, and
`advance` — there is no direct index access outside `Cursor`.

**Why this matters:** `line` and `col` are updated in one place. If you
find yourself computing line and col from `i` elsewhere, you are duplicating
logic that should live in `advance`.

### 5.3 The main loop

```cpp
while (!c.eof()) {
    char ch = c.peek();
    // dispatch on ch:
    //   whitespace        -> advance, continue
    //   '\n'              -> emit Newline, advance
    //   '/' + '/'         -> skip to end of line
    //   '/' + '*'         -> skip block comment
    //   '"'               -> scan string
    //   ident start       -> scan ident or keyword
    //   digit             -> scan number
    //   punctuation       -> try match_punct, else single char
    //   anything else     -> error
}
```

Each case either advances the cursor and produces a token, or advances the
cursor and produces nothing (for whitespace and comments), or errors out.

### 5.4 String scanning

```cpp
if (ch == '"') {
    int ln = c.line, cl = c.col;
    c.advance();  // opening quote
    std::string value;
    while (!c.eof()) {
        char s = c.advance();
        if (s == '"') break;              // closing quote
        if (s == '\\') {
            char e = c.advance();
            if (e == '"' || e == '\\') value.push_back(e);
            else { value.push_back('\\'); value.push_back(e); }
        } else if (s == '\n') {
            // error: unterminated string
        } else {
            value.push_back(s);
        }
    }
    out.emplace_back(TokKind::String, std::move(value), ln, cl);
}
```

The rule for escapes is: `\"` becomes `"`, `\\` becomes `\`, everything else
is preserved as `\` followed by the escaped character. This is what makes
the round-trip through the emitter lossless.

### 5.5 Punctuation matching

`kPuncts` is a list of multi-character punctuation tokens, ordered longest
first. `match_punct` scans this list against the current position and
returns the first match. Single-character punctuation is handled by the
fallthrough: if `match_punct` returns null and the character is a
recognized single-character punct, it is emitted as a one-character `Punct`
token.

**Why longest first:** So `==` is matched before `=`, `->` before `-`, `<<=`
before `<<` before `<`. If you add a punctuation token, insert it in the
correct position in `kPuncts`.

### 5.6 Errors

Three errors: unterminated string, unterminated block comment, unexpected
character. See `docs/error-messages.md` section 2.

---

## 6. The rewriter

The rewriter is a single pass over the token stream, in `src/rewriter.cpp`.
It is about 350 lines.

### 6.1 Structure

```cpp
bool rewrite(const std::vector<Token>& in,
             std::vector<Token>& out,
             RewriteError& err);
```

Same shape as `tokenize`. It reads `in`, writes `out`, fills `err` on
failure.

### 6.2 The main loop

```cpp
Cursor c(in);
while (!c.at_end()) {
    const Token& t = c.cur();

    if (t.is(TokKind::Keyword, "import")) { try_import(c, out, err); continue; }
    if (t.is(TokKind::Keyword, "func"))   { try_func(c, out, err);   continue; }
    if (t.is(TokKind::Keyword, "render")) { /* emit return, advance */ continue; }
    if (t.is(TokKind::Keyword, "nil"))    { /* emit nullptr, advance */ continue; }
    if (t.is(TokKind::Keyword, "let"))    { /* emit const auto, advance */ continue; }
    if (t.is(TokKind::Keyword, "var"))    { /* emit auto, advance */ continue; }

    out.push_back(t);
    c.advance();
}
```

Every rule is a `case` in this dispatch. Two rules (`import` and `func`)
are function calls because they scan. Four rules (`render`, `nil`, `let`,
`var`) are inline substitutions. The rest of the tokens (`print`,
`println`, everything not a cminux keyword) are copied verbatim.

### 6.3 The cursor

```cpp
struct Cursor {
    const std::vector<Token>& toks;
    size_t i;
    const Token& cur() const;
    const Token& peek(size_t n) const;
    bool at_end() const;
    void advance();
};
```

Similar to the tokenizer's cursor, but over a token vector. `peek` returns
`End` for out-of-bounds indices, so rules can peek ahead without checking.

**Why `peek` returns `End` for OOB:** So `c.peek(1).is(...)` never crashes.
If a rule scans ahead and runs off the end, it sees `End` and can decide
what to do. In practice, all rules that scan ahead treat `End` as an error
condition.

### 6.4 Rule: `import`

`try_import` is called when `c.cur()` is `Keyword("import")`. It:

1. Consumes `import`.
2. Requires `[`.
3. Scans string literals separated by commas until `]`.
4. Optionally consumes a trailing `;`.
5. For each path, emits `#include "path"` and a `Newline`.

The token sequence for each path is:

```
Punct("#include") Punct("\"") Ident(path) Punct("\"") Newline
```

The `"` tokens are emitted as `Punct` so the emitter prints them as-is
(not as a string literal that would be re-quoted). The path is emitted as
`Ident` so the emitter prints it verbatim.

**Why `Punct("#include")` and not a new token kind:** Because `#include` is
just a sequence of characters that the emitter prints. It has no semantic
meaning to any later stage. A new token kind would require updates to every
stage that switches on `kind`, for no benefit.

**Why the path is `Ident` and not `String`:** A `String` token would be
re-quoted by the emitter, adding an extra set of quotes: `""foo.hpp""`.
Treating the path as `Ident` prints it verbatim, which is what we want.

### 6.5 Rule: `func`

`try_func` is the most complex rule. It:

1. Consumes `func`.
2. Reads the function name (must be `Ident`).
3. Scans the parameter list with balanced parentheses, collecting tokens
   including the outer `(` and `)`.
4. If `->` follows, scans tokens until `{` (the body), collecting the
   return type.
5. Emits: return type (or synthetic `void`), spacer, name, parameter list.
6. Returns to the main loop; `c.cur()` is now `{` (the body).

The spacer token is `Punct(" ")`. See section 7.4.

**Balanced-parenthesis scan:**

```cpp
int depth = 0;
while (!c.at_end()) {
    const Token& t = c.cur();
    params.push_back(t);
    if (t.is(TokKind::Punct, "(")) ++depth;
    else if (t.is(TokKind::Punct, ")")) {
        --depth;
        if (depth == 0) { c.advance(); break; }
    }
    c.advance();
}
```

The first token is `(` (which increments depth to 1). The scan ends when
depth returns to 0 after a `)`. If `End` is reached before depth returns to
0, the function reports `unterminated parameter list`.

**Return-type scan:**

```cpp
while (!c.at_end()) {
    const Token& t = c.cur();
    if (t.is(TokKind::Punct, "{")) break;
    if (t.is(TokKind::Punct, ";")) { /* error: declaration without body */ }
    return_type.push_back(t);
    c.advance();
}
```

Scans until `{` (body starts) or `;` (error — forward declaration). Every
token in between is part of the return type.

### 6.6 Other rules

`render`, `nil`, `let`, and `var` are all simple:

```cpp
if (t.is(TokKind::Keyword, "render")) {
    out.emplace_back(TokKind::Punct, "return", t.line, t.col);
    c.advance();
    continue;
}
```

Each replaces one token with one or two tokens, preserving `line` and `col`
so subsequent error messages have the right location.

### 6.7 Errors

Eight errors, all about `import` or `func` syntax. See
`docs/error-messages.md` section 3.

---

## 7. The emitter

The emitter is a single pass over the rewritten token stream, in
`src/emitter.cpp`. It is about 150 lines.

### 7.1 Structure

```cpp
std::string emit_cpp(const std::vector<Token>& tokens,
                     const EmitOptions& opts);
```

Takes tokens, returns a string. No errors, because the emitter cannot fail.

### 7.2 Prologue

The first thing emitted is the prologue, exactly as in
`docs/translation-rules.md` section 2:

```cpp
out << "// generated by cminux v1.0.0 — do not edit\n";
out << "// source: " << opts.source_filename << "\n";
out << "#include \"outfn.hpp\"\n";
out << "using cmx::print;\n";
out << "using cmx::println;\n";
out << "\n";
if (opts.emit_line_directive) {
    out << "#line 1 \"" << opts.source_filename << "\"\n";
}
```

The `#line` directive is critical for error mapping. Without it, C++
compiler errors would point at the generated file, not the source.

### 7.3 The token loop

```cpp
const Token* prev = nullptr;
bool at_line_start = true;
for (const auto& t : tokens) {
    if (t.kind == TokKind::End) break;
    if (t.kind == TokKind::Newline) { out << '\n'; prev = nullptr; at_line_start = true; continue; }
    if (is_spacer(t)) { out << ' '; prev = nullptr; at_line_start = false; continue; }
    if (!at_line_start && prev && needs_space(*prev, t)) out << ' ';
    if (t.kind == TokKind::String) { /* print with quotes and escapes */ }
    else { out << t.text; }
    prev = &t;
    at_line_start = false;
}
```

Three cases: `End` (stop), `Newline` (print `\n`), spacer (print a space).
Everything else goes through the spacing rules.

### 7.4 Spacing rules

`needs_space(prev, cur)` decides whether to insert a space between two
tokens. The rules, in order:

1. No space after `#include` (handled specially — always a space, so the
   directive is printed correctly)
2. No space before `,`, `;`, `)`, `]`, `}`
3. No space after `(`, `[`, `{`
4. No space around `.` or `->`
5. No space around `::`
6. Space between two word tokens (`int main`, `return 0`)
7. Space between a word token and a string (`print "hi"` — rare but valid)
8. No space otherwise

**Why not just "join with spaces":** Because `int main()` would become
`int main ( )` with spaces, which is valid C++ but ugly. And `x.y` would
become `x . y`, which is valid but also ugly. The spacing rules give
readable output.

### 7.5 The spacer token

`Punct(" ")` is a special token emitted by the rewriter in rule 2/3. The
emitter prints it as a single space and resets `prev` to `nullptr`, so the
next token is printed with no preceding space (per rule 8). This is how
`int*foo` becomes `int* foo` (well, `int*` and then `foo` — the space comes
from the spacer, not from `needs_space`).

**Why not just let `needs_space` handle it:** Because `needs_space` sees
`Punct("*")` followed by `Ident("foo")` and would apply rule 8 (no space),
producing `*foo`. In the context `int* foo`, we want the space after `*`.
The spacer is the escape hatch: the rewriter knows the context and can
insert a space wherever it is needed.

### 7.6 Line structure

The emitter prints `Newline` tokens as actual `\n` characters. Since the
token stream preserves newlines from the source, the output line structure
matches the input line structure (plus prologue lines and import-line
shifts).

**Why not just emit `\n` after every statement:** Because cminux does not
parse statements. It does not know where statements end. It only knows
where lines end, which the tokenizer tracks.

---

## 8. The compiler driver

The compiler driver is `src/compiler.cpp`. It is about 100 lines.

### 8.1 Structure

```cpp
bool compile_to_exe(const CompileOptions& opts, std::string& err);
std::string detect_compiler();
```

`compile_to_exe` builds a command string and calls `std::system`. It returns
true if the compiler exited with status 0, false otherwise.

### 8.2 Compiler selection

```cpp
std::string detect_compiler() {
    if (const char* env = std::getenv("CMX_CC")) {
        if (env[0] != '\0') return env;
    }
#if defined(_WIN32)
    return "cl";
#elif defined(__APPLE__)
    return "clang++";
#else
    return "g++";
#endif
}
```

The priority is: `CMX_CC` environment variable, then a platform default.
Users can also pass `--cc <name>` to override, which is stored in
`opts.compiler` and takes precedence over `detect_compiler`.

### 8.3 Command construction

```cpp
std::ostringstream cmd;
cmd << cc;
cmd << ' ' << shell_quote(opts.cpp_source);
for (const auto& inc : opts.include_dirs) cmd << " -I" << shell_quote(inc);
for (const auto& flag : opts.extra_flags) cmd << ' ' << flag;
cmd << " -o " << shell_quote(opts.output_exe);
cmd << " -std=c++17";
```

Paths are shell-quoted. Extra flags are passed verbatim (the user is
responsible for their own quoting). The `-std=c++17` is added
unconditionally — cminux targets C++17 and does not support older
standards.

### 8.4 Shell quoting

```cpp
#if defined(_WIN32)
    // wrap in double quotes, escape embedded "
#else
    // wrap in single quotes, escape embedded '
#endif
```

Windows `cmd.exe` and POSIX shells have different quoting rules. The
`shell_quote` function handles both.

**Limitation:** Paths with embedded newlines or other exotic characters
are not handled. This is fine for normal use; do not pass such paths.

### 8.5 Why `std::system`

Because it is portable and simple. A more robust implementation would use
`CreateProcess` on Windows and `posix_spawn` on POSIX, avoiding the shell
entirely. That is a future improvement, not required for v1.0.0.

**Security note:** `std::system` runs the command through a shell. If
`opts.cpp_source` or `opts.output_exe` come from untrusted input, shell
injection is possible. In cminux, these come from the user's own command
line, so they are not more dangerous than what the user typed. Do not use
cminux to compile untrusted input without sandboxing.

### 8.6 The library path

The driver, not the compiler driver, adds the cminux library directory to
the include path:

```cpp
if (const char* libdir = std::getenv("CMX_LIB_DIR")) {
    if (libdir[0] != '\0') copts.include_dirs.push_back(libdir);
} else {
    copts.include_dirs.push_back(CMX_LIB_DIR_DEFAULT);
}
```

`CMX_LIB_DIR_DEFAULT` is defined by CMake at compile time:

```cmake
target_compile_definitions(cminux PRIVATE
    CMX_LIB_DIR_DEFAULT="${CMAKE_CURRENT_SOURCE_DIR}/lib"
)
```

This means the built cminux executable knows where its library was at build
time. If the user moves the executable without moving the library, they
must set `CMX_LIB_DIR`.

---

## 9. The top-level driver

The top-level driver is `src/driver.cpp`. It is about 150 lines.

### 9.1 Structure

```cpp
int run(const DriverOptions& opts);
```

Returns the process exit code: 0 on success, 1 on failure.

### 9.2 Stages

`run` executes the pipeline in order:

1. Read source file.
2. Tokenize (report errors, return 1 on failure).
3. If `opts.dump_tokens`, print token stream to stderr.
4. Rewrite (report errors, return 1 on failure).
5. Emit.
6. Write temp `.cpp` next to input.
7. Compile (report errors, return 1 on failure).
8. Remove temp file unless `opts.keep_cpp`.
9. Return 0.

### 9.3 Temp file naming

```cpp
std::string temp_cpp_path(const std::string& input_path) {
    // input: dir/main.cmx  ->  dir/main.cmx.cpp
}
```

The temp file is named `<stem>.cmx.cpp` and lives next to the input. This
means relative `#include` paths in the input resolve from the input's
directory, not from the cwd, which is what users expect.

**Why not `/tmp`:** Because users want to be able to `--keep` the file and
inspect it. Putting it in `/tmp` makes that harder and breaks relative
imports.

### 9.4 Output executable naming

```cpp
std::string derive_exe_path(const std::string& input_path) {
#if defined(_WIN32)
    return dir + stem + ".exe";
#else
    return dir + stem;
#endif
}
```

If `opts.output_exe` is empty, the executable is named after the input.
Windows gets `.exe`; Unix gets no extension.

### 9.5 Error reporting

```cpp
void report_error(const std::string& file, int line, int col, const std::string& msg) {
    std::cerr << file << ":" << line << ":" << col << ": error: " << msg << "\n";
}
```

Single function for all cminux-native errors. The format is
`file:line:col: error: message`.

C++ compiler errors are not passed through this function. They appear on
stderr exactly as the C++ compiler emits them.

---

## 10. Adding a new rule

To add a tenth rule, you need:

1. A keyword in `is_cminux_keyword` (in `token.hpp`).
2. A `case` in the main loop of `rewrite` (in `rewriter.cpp`).
3. If the rule scans (like `func`), a helper function (like `try_func`).
4. A test in `tests/`.
5. An update to `docs/language-reference.md` and this document.

### 10.1 Example: adding `fn` as an alias for `func`

Not that cminux needs this (it does not), but as an example of the process.

**Step 1:** Add `fn` to `is_cminux_keyword`:

```cpp
return s == "import"
    || s == "func"
    || s == "fn"        // NEW
    || s == "render"
    // ...
```

**Step 2:** Add a case in `rewrite`:

```cpp
if (t.is(TokKind::Keyword, "fn")) {
    if (!try_func(c, out, err)) return false;
    continue;
}
```

**Step 3:** No change to `try_func` — it already handles `func`. But wait:
`try_func` starts with `c.advance()` to consume `func`, and the error
message "expected function name after 'func'" would now be wrong for `fn`.
So `try_func` needs a parameter:

```cpp
bool try_func(Cursor& c, std::vector<Token>& out, RewriteError& err,
              const std::string& keyword);
```

And the error message becomes `"expected function name after '" + keyword +
"'"`.

**Step 4:** Add tests:

```
tests/fn_alias.cmx
tests/fn_alias.expected.cpp
```

**Step 5:** Update docs.

### 10.2 What not to add

Do not add a rule that requires semantic information. If your rule needs to
know what a type is, or what a name refers to, or what a value equals, it is
a compiler feature, not a cminux feature. Stop.

Do not add a rule that requires more than a fixed lookahead. The `func`
rule's balanced-parenthesis scan is the most complex lookahead cminux has.
Any new rule that requires more is a red flag.

Do not add a rule that interacts with another rule. If the output of your
rule depends on whether some other rule ran first, the rules are not
independent, and the pipeline's "single pass" invariant is broken.

Do not add a rule that cannot be expressed as a token substitution. Every
cminux rule is a local token transformation. A rule that requires
restructuring the token stream (moving whole blocks, reordering statements,
inserting braces) is not a cminux rule.

---

## 11. Adding a new keyword

Keywords are reserved. Adding one is a breaking change: any cminux source
that used the word as an identifier stops compiling.

If you are adding a keyword for a new rule, the process is:

1. Add the word to `is_cminux_keyword` in `token.hpp`.
2. Add the corresponding case in `rewrite`.
3. Update every test that used the word as an identifier (there should be
   none, but check).
4. Update `docs/language-reference.md` section 2.

If you are adding a keyword to reserve it (without a rule), you still need
all four steps except 2. Reserved keywords with no rule are "pass-through":
the tokenizer marks them `Keyword`, the rewriter copies them verbatim, the
emitter prints them. The C++ compiler sees them as identifiers, which is
what you want if the keyword is a C++ type name (`int`, `long`).

---

## 12. Testing

### 12.1 The test harness

`tests/run.bat` (Windows) or `tests/run.sh` (Unix) runs each `.cmx` file in
`tests/` through the pipeline and compares:

- The emitted `.cpp` against `<name>.expected.cpp` (if present).
- The program's stdout against `<name>.expected.stdout` (if present).
- The program's exit code against `<name>.expected.exit` (if present).

### 12.2 Adding a test

1. Create `tests/mytest.cmx` with the input.
2. Create `tests/mytest.expected.cpp` with the expected output (run
   `cminux tests/mytest.cmx --keep` and copy the result).
3. Create `tests/mytest.expected.stdout` with the expected stdout (if the
   program prints anything).
4. Create `tests/mytest.expected.exit` with the expected exit code (usually
   `0`).
5. Run `tests\run.bat` or `tests/run.sh` to verify.

### 12.3 What to test

Every rule should have at least one test:

- `import` with one path: `import_multi.cmx` (or add a single-path test)
- `import` with multiple paths: `import_multi.cmx`
- `func` with return type: `explicit_return_type.cmx`
- `func` without return type: (add a test)
- `render` with value: `hello.cmx`
- `render` without value: (add a test)
- `nil` in an expression: `nil_and_var.cmx`
- `let`: `nil_and_var.cmx`
- `var`: `nil_and_var.cmx`
- `print`: `hello.cmx`
- `println`: `hello.cmx`

Add error tests too. For example, `tests/error_unterminated_string.cmx`
with a broken string literal, and check that cminux reports the error. The
test harness does not currently compare stderr, so error tests need a
different mechanism (or a future test harness extension).

### 12.4 Testing edge cases

Edge cases worth testing when you add a rule:

- Empty input
- Input with no trailing newline
- Input with CRLF line endings
- Input with comments (line and block)
- Input with strings containing `\"` and `\\`
- Input with deeply nested parentheses
- Input with a function whose return type is a template type

---

## 13. Things cminux deliberately does not do

For the record, and to discourage well-meaning changes that would break
the design:

**cminux does not parse C++.** It tokenizes and rewrites. It does not
understand expressions, statements, types, or declarations. This is what
keeps it small.

**cminux does not have an AST.** It has a token stream. There is no parse
tree, no semantic graph, no IR. This is what keeps it fast.

**cminux does not do name resolution.** It does not know what `main` is,
what `int` is, or what `print` is. It knows only the token text.

**cminux does not do type checking.** It does not know that `x` is an
`int`. It does not know that `print` takes a string. It only knows that
`print` is an identifier that must be copied verbatim.

**cminux does not optimize.** It emits tokens. The C++ compiler optimizes.

**cminux does not manage dependencies.** It compiles one file. Linking is
the user's job.

**cminux does not generate code.** It emits source. The C++ compiler
generates code.

**cminux does not have a runtime.** The library is a header with `print`,
`println`, and `printf`. That is the entire runtime.

**cminux does not have a standard library.** The C++ standard library is
the standard library.

**cminux does not have a package manager.** Use CMake, vcpkg, Conan, or
fetch headers manually.

**cminux does not have a build system.** Use CMake, Make, or compile each
`.cmx` file individually.

**cminux does not have a language server.** Not yet. If you want one, add
`tools/cminux-lsp/` and depend only on `token.hpp` and `tokenizer.hpp`.

**cminux does not have a formatter.** Not yet. If you want one, add
`tools/cminux-fmt/` and depend only on `token.hpp`, `tokenizer.hpp`, and
`rewriter.hpp`. (Do not depend on `emitter.hpp` — the formatter should
produce cminux source, not C++ source.)

If you find yourself wanting to add one of these, that is fine — but add it
as a *separate tool* in `tools/`, not as a stage in the compiler. The
compiler stays small.

---

## Appendix A: The complete pipeline, in one diagram

```
              ┌──────────────┐
              │  source.cmx  │
              └──────┬───────┘
                     │
                     ▼
              ┌──────────────┐
              │  tokenizer   │  → TokenizeError
              └──────┬───────┘
                     │  vector<Token>
                     ▼
              ┌──────────────┐
              │   rewriter   │  → RewriteError
              └──────┬───────┘
                     │  vector<Token> (rewritten)
                     ▼
              ┌──────────────┐
              │   emitter    │  → string (C++ source)
              └──────┬───────┘
                     │
                     ▼
              ┌──────────────┐
              │  source.cmx.cpp │
              └──────┬───────┘
                     │
                     ▼
              ┌──────────────┐
              │  cl.exe /    │
              │  g++ / clang++│
              └──────┬───────┘
                     │
                     ▼
              ┌──────────────┐
              │  source.exe  │
              └──────────────┘
```

Every arrow between boxes is a function call in `driver.cpp`. There are no
hidden stages, no plugins, no hooks, no callbacks. The pipeline is six
steps and you can read them in `run()` in order.

---

## Appendix B: File reference

| File | Lines | Purpose |
|------|-------|---------|
| `src/main.cpp` | ~120 | CLI parsing, calls `cmx::run` |
| `src/token.hpp` | ~80 | Token, TokKind, keyword list |
| `src/tokenizer.hpp` | ~40 | `tokenize` declaration |
| `src/tokenizer.cpp` | ~260 | Scanner |
| `src/rewriter.hpp` | ~35 | `rewrite` declaration |
| `src/rewriter.cpp` | ~340 | Rewrite rules |
| `src/emitter.hpp` | ~30 | `emit_cpp` declaration |
| `src/emitter.cpp` | ~140 | Token printer |
| `src/compiler.hpp` | ~30 | `compile_to_exe` declaration |
| `src/compiler.cpp` | ~100 | Shell-out |
| `src/driver.hpp` | ~30 | `run` declaration |
| `src/driver.cpp` | ~150 | Pipeline orchestration |
| `lib/outfn.hpp` | ~130 | Output library |
| `lib/src/outfn.cpp` | ~15 | Anchor |

Total cminux-specific code: about 1,500 lines. That is the entire transpiler.

---

## Appendix C: Constants

These values are referenced in multiple places and should not be changed
without updating every reference.

- `CMINUX_OUTFN_HPP` — include guard in `outfn.hpp`
- `CMX_LIB_DIR_DEFAULT` — compile-time default for `outfn.hpp` location
- `CMX_CC` — environment variable for compiler selection
- `CMX_LIB_DIR` — environment variable for library location
- `cminux 1.0.0` — version string, appears in `--version`, the prologue,
  and the README
- `#line 1 "<source>"` — line directive format, exact spacing matters
- `Punct(" ")` — the spacer token, exact text matters
- `"generated by cminux v1.0.0 — do not edit"` — prologue first line, exact
  text matters (the em-dash is U+2014)