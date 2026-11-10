# cminux Error Messages

**Version:** 1.0.0

This document lists every error message cminux can emit, what triggers each
one, and how to fix it. It is organized by pipeline stage: tokenizer errors
first, then rewriter errors, then errors from the C++ compiler.

Every cminux-native error has the form:

```
<file>:<line>:<col>: error: <message>
```

For example:

```
hello.cmx:3:14: error: unterminated string literal
```

The file, line, and column point at the exact location in your `.cmx`
source. Clicking the message in most editors jumps to that location.

Errors from the C++ compiler are forwarded verbatim, but because of the
`#line` directives cminux emits, the line numbers in those errors point at
your `.cmx` file, not the generated `.cpp` file.

---

## Table of contents

1. [How errors work](#1-how-errors-work)
2. [Tokenizer errors](#2-tokenizer-errors)
3. [Rewriter errors](#3-rewriter-errors)
4. [C++ compiler errors](#4-c-compiler-errors)
5. [Error format reference](#5-error-format-reference)
6. [Common problems and fixes](#6-common-problems-and-fixes)
7. [What to do when you cannot find your error](#7-what-to-do-when-you-cannot-find-your-error)

---

## 1. How errors work

cminux has two stages that can produce cminux-native errors: the tokenizer
and the rewriter. After both succeed, the emitted C++ is passed to the C++
compiler, which can also produce errors.

### 1.1 Errors vs. warnings

cminux does not emit warnings. Every diagnostic is either an error (the
transpilation stops and no executable is produced) or nothing.

The C++ compiler may emit warnings. Those are not cminux errors — they come
from the C++ compiler, are about C++ code (the generated C++ code, which
is semantically equivalent to your cminux code), and cminux does not
interfere with them. If you want to silence a C++ warning, use `-C
-Wno-<warning>` on the cminux command line, which forwards the flag to the
C++ compiler.

### 1.2 Error propagation

When the tokenizer or rewriter reports an error, cminux:

1. Prints the error to `stderr`.
2. Deletes the temporary `.cmx.cpp` file if it was created.
3. Exits with code 1.

The C++ compiler is never invoked on invalid input. There is no partial
output, no best-effort compilation, no "compile what we can." cminux either
transpiles the whole file or none of it.

### 1.3 When multiple errors exist

The tokenizer and rewriter stop at the first error they encounter. If a
`.cmx` file has three syntax errors, cminux reports the first one only.
This is a limitation: it means you fix errors one at a time and re-run.

The C++ compiler is more forgiving — it typically reports several errors per
run — but its error recovery is its own, not cminux's.

---

## 2. Tokenizer errors

The tokenizer produces three kinds of errors.

### 2.1 `unterminated string literal`

**Message:**

```
error: unterminated string literal
```

**Trigger:** A string literal opened with `"` is not closed before the end
of the line.

```
println("hello);
```

**Cause:** The closing `"` is missing. Strings cannot span lines in cminux.

**Fix:** Add the closing `"`:

```
println("hello");
```

**Line and column:** The line and column of the opening `"`.

**Note:** If the string appears to be closed but you still get this error,
look for an escaped quote inside the string that is not properly escaped:

```
println("she said \"hi\"");
```

The above is correct — `\"` is an escaped quote, and the string content is
`she said "hi"`.

```
println("she said "hi"");
```

The above is a common mistake: the inner quotes are not escaped, so the
string ends at the first inner quote, and the rest of the line is parsed as
code, which fails with a different error.

### 2.2 `unterminated block comment`

**Message:**

```
error: unterminated block comment
```

**Trigger:** A block comment opened with `/*` is not closed with `*/`.

```
/* this comment never ends
println("hello");
```

**Cause:** The closing `*/` is missing.

**Fix:** Add `*/`:

```
/* this comment ends here */
println("hello");
```

**Line and column:** The line and column of the opening `/*`.

**Note:** Block comments do not nest in cminux (same as C++). `/* /* */`
is a complete block comment — the inner `/*` is just text. If you want
nested block comments, use line comments (`//`) for the inner levels.

### 2.3 `unexpected character`

**Message:**

```
error: unexpected character '<char>'
```

**Trigger:** A character that is not whitespace, not part of a comment, not
part of a string literal, not part of an identifier, not part of a number,
and not a recognized punctuation character.

**Cause:** Usually one of the following:

- A Unicode character outside a string literal: `→`, `☃`, non-breaking space
- A character literal, which cminux does not support: `'a'`
- A character that is not in the C++ source character set

**Example:**

```
var x = 'a';
```

This produces `unexpected character '''` (the single quote is not a valid
cminux token).

**Fix:** Depending on the cause:

- For a character literal, write the character inside a string instead, or
  use C++ interop: `var x = "a"[0];`
- For a Unicode character outside a string, move it inside a string, or
  remove it
- For a non-breaking space accidentally inserted by an editor, replace it
  with a regular space

**Line and column:** The line and column of the offending character.

**Note:** The error message shows only the character itself, not its code
point. If the character is not visible (a non-breaking space, a zero-width
joiner, a byte-order mark), the message may look like `unexpected character
' '` where the space inside the quotes is not a regular space. This is rare
but worth knowing about when debugging invisible characters.

---

## 3. Rewriter errors

The rewriter produces seven kinds of errors, all related to `import` or
`func` syntax.

### 3.1 `expected '[' after 'import'`

**Message:**

```
error: expected '[' after 'import'
```

**Trigger:** The word `import` is followed by something other than `[`.

```
import "outfn.hpp"
```

**Cause:** Missing `[`. The import list must be bracketed, even with a
single entry.

**Fix:** Add the brackets:

```
import ["outfn.hpp"]
```

**Line and column:** The line and column of `import`.

### 3.2 `expected string literal in import list`

**Message:**

```
error: expected string literal in import list
```

**Trigger:** Inside `import [...]`, a token that is not a string literal
appears where a path is expected.

```
import [outfn]
```

or:

```
import ["a.hpp", ]
```

or:

```
import []
```

**Cause:** The path is not quoted, or a trailing comma is left before `]`,
or the list is empty.

**Fix:** Make sure every entry is a quoted string, and remove any trailing
comma:

```
import ["a.hpp", "b.hpp"]
```

**Line and column:** The line and column of the offending token. For an
empty list, the `]` token.

**Note:** For an empty list `import []`, cminux reports the error at the
`]` because it is scanning for a string literal and finds `]` instead. If
you want an empty import list (which is pointless but legal in some
languages), cminux does not allow it.

### 3.3 `expected ',' or ']' in import list`

**Message:**

```
error: expected ',' or ']' in import list
```

**Trigger:** After a string literal in an import list, the next token is
neither `,` nor `]`.

```
import ["a.hpp" "b.hpp"]
```

**Cause:** Missing comma between two paths.

**Fix:** Add the comma:

```
import ["a.hpp", "b.hpp"]
```

**Line and column:** The line and column of the offending token (the
second `"b.hpp"` in the example above).

### 3.4 `expected function name after 'func'`

**Message:**

```
error: expected function name after 'func'
```

**Trigger:** `func` is followed by a token that is not an identifier.

```
func () -> int { }
```

or:

```
func if() -> int { }
```

or:

```
func 123() -> int { }
```

**Cause:** Missing function name, or a reserved keyword used as the name, or
a number used as the name.

**Fix:** Use a valid identifier that is not a cminux keyword:

```
func main() -> int { }
```

**Line and column:** The line and column of `func`.

### 3.5 `expected '(' after function name`

**Message:**

```
error: expected '(' after function name
```

**Trigger:** After `func <name>`, the next token is not `(`.

```
func main -> int { }
```

**Cause:** Missing open parenthesis before the parameter list.

**Fix:** Add the parentheses (even for an empty parameter list):

```
func main() -> int { }
```

**Line and column:** The line and column of the offending token (the `->`
in the example above).

### 3.6 `unterminated parameter list`

**Message:**

```
error: unterminated parameter list
```

**Trigger:** The rewriter scans for the matching `)` of the parameter list
and reaches the end of the file without finding it.

```
func main(int a, int b {
    render 0;
}
```

**Cause:** Missing close parenthesis. Usually a typo, often the result of a
missing `)` in a nested call within a default argument.

**Fix:** Add the `)`:

```
func main(int a, int b) {
    render 0;
}
```

**Line and column:** The line and column of `func`. The error message
points at the start of the function because the missing `)` could be
anywhere inside the parameter list, and the start of the function is the
most useful anchor point.

**Note:** If the source has balanced parentheses but the rewriter still
reports this error, it means a `)` is missing somewhere in a
parameter-list expression, such as a default argument:

```
func f(int a = foo(1, 2) -> int { }
```

Here the `foo(1, 2)` is balanced, but the parameter list as a whole is
missing its close parenthesis. Add it:

```
func f(int a = foo(1, 2)) -> int { }
```

### 3.7 `expected return type after '->'`

**Message:**

```
error: expected return type after '->'
```

**Trigger:** `->` appears in a function signature, but the next token is
`{` (immediately the body) with no type in between.

```
func main() -> { }
```

**Cause:** The return type was omitted after `->`, or the arrow was written
when a `void` return was intended.

**Fix:** Either provide a return type:

```
func main() -> int { }
```

or remove the arrow entirely:

```
func main() { }
```

**Line and column:** The line and column of `func`.

### 3.8 `function declaration without body is not supported`

**Message:**

```
error: function declaration without body is not supported
```

**Trigger:** The rewriter scans for the body's opening `{` and finds `;`
first.

```
func foo() -> int;
```

**Cause:** A function forward declaration. cminux v1.0.0 does not support
forward declarations.

**Fix:** Either add a body:

```
func foo() -> int {
    render 0;
}
```

or, if you need a forward declaration, write it in C++ in an imported
header:

```cpp
// my_header.hpp
int foo();
```

and import the header from cminux.

**Line and column:** The line and column of the `;`.

---

## 4. C++ compiler errors

After cminux successfully tokenizes, rewrites, and emits, the generated
`.cpp` file is passed to the C++ compiler. Any errors from that compiler
are C++ errors — syntax errors, type errors, missing includes, linker
errors, and so on.

cminux does not translate, filter, or reformat C++ compiler errors. They
appear exactly as the C++ compiler emits them, with one modification: the
`#line` directives in the generated `.cpp` cause the C++ compiler to report
line numbers that point at your `.cmx` source, not the generated file.

### 4.1 Reading a C++ error in cminux context

If the C++ compiler reports:

```
hello.cmx:7:5: error: use of undeclared identifier 'pritn'
```

Then line 7, column 5 of your `.cmx` file has a call to `pritn`, which is a
typo for `print`. The C++ compiler sees the call (because the `#line`
directive makes it report the source location), and you see the error at
the location in your source.

### 4.2 Common C++ errors you will encounter

These are not cminux errors — they are C++ errors on the generated code.
But because cminux users will encounter them often, here is a brief guide.

**`no matching function for call to 'print'`**

You passed a type that `outfn.hpp` does not have an overload for. See
`docs/output-library.md` section 7 for how to add your own overload.

**`use of undeclared identifier 'X'`**

You used a name that has not been declared. Common causes:

- Missing `import` for the header that declares `X`
- Typo in the name
- Using a name before its declaration (cminux v1.0.0 does not support
  forward references in the same file for variables, only for functions)

**`expected ';' after expression`**

You wrote an expression where C++ expects a statement terminator. Most
commonly, this is a missing `;` at the end of a line:

```
var x = 5
println(x);
```

Fix by adding the semicolon:

```
var x = 5;
println(x);
```

**`redefinition of 'X'`**

You declared the same name twice in the same scope. cminux does not
detect this; C++ does.

```
func main() -> int {
    var x = 5;
    var x = 10;
    render 0;
}
```

Fix by renaming one of them:

```
func main() -> int {
    var x = 5;
    var y = 10;
    render 0;
}
```

**`'auto' cannot be used as a function return type` or similar**

You wrote something that C++ cannot deduce. This usually means your cminux
source has an expression whose type is ambiguous. Add an explicit cast:

```
func main() -> int {
    var x = (int){};      // instead of: var x = {};
    render 0;
}
```

**`undefined reference to 'main'`**

You have no `main` function. Every executable needs one. If your cminux
source has only helper functions, add:

```
func main() -> int {
    render 0;
}
```

### 4.3 Linker errors

Linker errors are reported after all C++ files have been compiled. They
typically mean:

- A function is declared but not defined (you promised it in a header but
  did not provide the `.cpp`)
- A header is included but its corresponding `.cpp` is not linked
- The wrong version of a library is linked

These are not cminux-specific. They are the same linker errors you would
see compiling the equivalent C++ program.

### 4.4 Warnings

The C++ compiler may emit warnings. cminux does not suppress them. If a
warning is worth silencing, forward a `-Wno-<warning>` flag:

```
cminux main.cmx -C -Wno-unused-variable
```

If a warning is about your cminux code, fix the code. If a warning is about
the generated `.cpp` prologue or `outfn.hpp`, it is a bug in cminux or the
library — please report it.

---

## 5. Error format reference

### 5.1 cminux-native errors

```
<file>:<line>:<column>: error: <message>
```

Example:

```
hello.cmx:3:14: error: unterminated string literal
```

- `<file>` is the path as given on the command line, verbatim.
- `<line>` is 1-based.
- `<column>` is 1-based.
- `<message>` is one of the messages in sections 2 and 3.

### 5.2 C++ compiler errors

```
<file>:<line>:<column>: error: <message>
```

or, for some compilers:

```
<file>(<line>): error: <message>
```

or:

```
<file>:<line>: error: <message>
```

The exact format depends on the C++ compiler (MSVC, GCC, and Clang each
have their own conventions). Because of the `#line` directive, `<file>`
and `<line>` point at your `.cmx` source.

### 5.3 Errors that are not line-numbered

Two things can produce output that does not have the `<file>:<line>:<col>`
form:

**Command-line errors.** `cminux: expected exactly one input file`,
`cminux: unknown option '-x'`, and similar — these are about the cminux
invocation, not about a source file. They have the form:

```
cminux: <message>
```

**Linker errors.** Linker errors usually name the symbols or object files
involved, not a source line. They may look like:

```
undefined reference to `foo()'
```

These are not cminux's errors; they come from the linker.

---

## 6. Common problems and fixes

This section collects user-facing problems and their solutions, in case you
encounter a symptom and want to know what it means.

### 6.1 "My program compiles but crashes at runtime"

cminux did its job — it translated and compiled. The crash is a runtime bug
in your program, not in the translation. Debug the program as you would
debug any C++ program: with a debugger, with `printf` debugging, with
`-g` flags.

```
cminux main.cmx -C -g -C -fsanitize=address
```

This forwards `-g` and `-fsanitize=address` to the C++ compiler, which adds
debug symbols and AddressSanitizer. If the crash is a memory error,
AddressSanitizer will pinpoint it.

### 6.2 "I get an error but there is nothing wrong with my source"

If the error message names a line you think is fine, check:

- **Line endings.** Windows CRLF (`\r\n`) vs. Unix LF (`\n`) should not
  matter — the tokenizer treats `\r` as whitespace — but a lone `\r` inside
  a string literal is preserved verbatim and may cause visible artifacts.
- **Tab vs. space.** Tabs and spaces are both whitespace. Neither should
  cause a syntax error.
- **Invisible characters.** Copy-pasting from a web page can introduce
  non-breaking spaces (U+00A0), which look like spaces but are not. The
  tokenizer rejects them with `unexpected character`. If the character in
  the error message looks like a space but the error says it is unexpected,
  this is likely the cause.

### 6.3 "The C++ compiler says the file is hello.cmx, but I did not write hello.cmx"

You probably did — you invoked cminux on it:

```
cminux hello.cmx -o hello.exe
```

The `#line` directive in the generated `.cpp` tells the C++ compiler the
source is `hello.cmx`, not `hello.cmx.cpp`. This is intentional. The C++
compiler is reporting errors at the correct location in *your* source.

If you want the C++ compiler to report errors at `.cpp` line numbers
instead, add `-C -P` or remove the `#line` directive from the emitter.
This is not recommended for normal use.

### 6.4 "My import does not resolve"

cminux does not resolve `#include` paths — the C++ compiler does. If your
import does not resolve:

- The path must be relative to the generated `.cpp` file, not the cminux
  binary
- The generated `.cpp` is next to the input `.cmx`, so relative paths
  resolve from the input's directory
- For paths elsewhere, use `-I <dir>` on the cminux command line, which
  forwards `-I` to the C++ compiler

### 6.5 "The prologue's `#include "outfn.hpp"` fails"

The prologue includes `outfn.hpp` from the cminux library directory. If
this fails:

- Make sure the CMake build placed `outfn.hpp` in `lib/`
- If you moved cminux to a different machine without `lib/`, set
  `CMX_LIB_DIR=<path-to-lib>` before invoking cminux
- If you invoke cminux from a script that changes directory, the
  compile-time `CMX_LIB_DIR_DEFAULT` still points at the original build
  directory; override with `CMX_LIB_DIR`

### 6.6 "The tests fail with an unexplained diff"

If `tests\run.bat` reports a diff, the actual output is shown alongside
the expected output. Common causes:

- Line endings: Windows CRLF vs. Unix LF in the expected files. The test
  harness uses `fc /b` on Windows (byte-for-byte) and `diff -u` on Unix,
  so line-ending mismatches are caught. Make sure the expected files are
  saved with the platform's line endings.
- Trailing newline: whether a file ends with a `\n` is significant. The
  test harness compares bytes, so a missing or extra trailing newline is a
  failure.
- Actually different output: if the actual output is genuinely different
  from the expected, the emitter or rewriter has changed since the expected
  file was written. Update the expected file, or fix the change.

---

## 7. What to do when you cannot find your error

If you encounter an error that does not match any in this document:

**First**, check whether it is a cminux error or a C++ compiler error. The
format is the same, but the message text is the giveaway:

- Messages with words like `expected`, `unterminated`, `unexpected`, and
  `import list`, `parameter list`, `function name`, `return type` are
  cminux errors. They are listed in sections 2 and 3.
- Messages about types, overloads, declarations, and definitions are C++
  errors. They come from the C++ compiler, and the compiler's own
  documentation is authoritative.

**Second**, if it is a cminux error and not in this document, it is a bug.
Report it with:

- The `.cmx` source that triggers it (reduced to the smallest file that
  reproduces)
- The exact error message
- Your cminux version (`cminux --version`)

**Third**, if it is a C++ error, the C++ compiler's documentation is the
right reference. Consult:

- Your compiler's manual (MSVC, GCC, or Clang)
- The C++ reference (en.cppreference.com)
- The C++ standard, if you want to know the exact rule

cminux does not modify C++ error messages, so the C++ compiler's
documentation applies without modification.

---

## Appendix: complete error list

Every cminux-native error message, one line each.

```
unterminated string literal
unterminated block comment
unexpected character '<char>'
expected '[' after 'import'
expected string literal in import list
expected ',' or ']' in import list
expected function name after 'func'
expected '(' after function name
unterminated parameter list
expected return type after '->'
function declaration without body is not supported
```

That is the complete list. Twelve errors. If cminux emits anything else,
it is either a C++ compiler error or a bug.

Command-line errors (not tied to a source file):

```
cminux: cannot open '<path>'
cminux: cannot write '<path>'
cminux: expected exactly one input file
cminux: unknown option '<option>'
cminux: <option> requires an argument
cminux: compiler exited with status <n>
```

These are about the invocation, not the source. They do not have line or
column information because there is no source location to point at.