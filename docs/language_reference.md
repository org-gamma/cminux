# cminux Language Reference

**Version:** 1.0.0

This document defines every construct in the cminux language. It is exhaustive
and intentionally boring. If a construct is not listed here, it is not part
of cminux — it is either C++ (which passes through unchanged) or it does not
exist.

cminux is a strict subset of C++'s capabilities with a different surface
syntax for nine constructs. Everything outside those nine constructs is C++
and behaves exactly as C++ behaves. This document only covers the nine.

---

## Table of contents

1. [Lexical structure](#1-lexical-structure)
2. [Keywords](#2-keywords)
3. [Source files](#3-source-files)
4. [Import](#4-import)
5. [Functions](#5-functions)
6. [Return](#6-return)
7. [Variables](#7-variables)
8. [Nil](#8-nil)
9. [Output functions](#9-output-functions)
10. [Everything else](#10-everything-else)
11. [Grammar summary](#11-grammar-summary)

---

## 1. Lexical structure

### 1.1 Encoding

cminux source files are UTF-8. The tokenizer reads bytes; multi-byte
characters outside string literals are rejected as unexpected characters.
Inside string literals, any byte is passed through unchanged.

### 1.2 Whitespace

Spaces, tabs, and carriage returns are whitespace. They separate tokens and
are otherwise ignored. Newlines are **not** ignored — they are preserved as
tokens so that line numbers in the emitted C++ match the source. See
[Internals: Newlines](#newlines).

### 1.3 Comments

Two forms, both identical to C++:

```
// line comment to end of line
/* block comment, may span lines */
```

Block comments must be terminated. An unterminated block comment is an error
at the line and column of the opening `/*`.

Comments are removed during tokenization. They do not appear in the token
stream and do not affect the output.

### 1.4 Identifiers

An identifier begins with a letter (`a`–`z`, `A`–`Z`) or underscore (`_`),
followed by zero or more letters, digits (`0`–`9`), or underscores.

```
foo
_bar
x1
MAX_VALUE
```

Identifiers that are not keywords are emitted to the C++ output unchanged.

### 1.5 String literals

A string literal begins and ends with a double quote:

```
"hello"
""
"with \"escaped\" quotes"
"backslash: \\"
```

The only recognized escape sequences are `\"` (a double quote) and `\\`
(a backslash). Any other backslash sequence — `\n`, `\t`, `\0`, `\x41` — is
**preserved verbatim**, meaning the two characters `\` and `n` are kept as-is
in the string token. When the token is re-emitted to C++, the C++ compiler
interprets them the way C++ always does. This round-trips losslessly: you
write `"\n"` in cminux, you get a newline in the C++ compiler's view.

String literals may not span lines. An unterminated string is an error at
the line and column of the opening quote.

There are no raw strings (`R"(...)"`), no character literals (`'a'`), and
no user-defined literals (`123_km`). Character literals are not supported as
cminux tokens — if you need `'a'`, write it inside a string and index it, or
use C++ directly.

### 1.6 Numbers

An integer is a run of decimal digits. A decimal is a run of digits, a
period, and a run of digits. There are no hex literals, no binary literals,
no octal literals, no exponent notation, and no type suffixes.

```
0
42
1000000
3.14
0.5
```

A number token is emitted to C++ unchanged. `3.14` is a `double`; `42` is an
`int`. If you want `3.14f` you cannot write it in cminux — write a `float`
variable and assign `3.14` to it, and let C++ narrow it.

### 1.7 Punctuation

cminux recognizes the following punctuation tokens. Longer tokens must match
before shorter ones, so `==` is recognized before `=`.

```
...                                        3-character
<<= >>=                                    3-character
== != <= >= && || -> :: << >> += -= *= /= %= &= |= ^= ++ --    2-character
( ) { } [ ] ; , : . + - * / % = ! < > & | ^ ~ ?                 1-character
```

Every punctuation token is emitted to C++ unchanged. cminux does not
interpret punctuation, only recognizes it so the tokenizer can separate it
from identifiers and numbers.

---

## 2. Keywords

The following words are reserved. They cannot be used as identifiers.

```
import  func  render  print  println
let  var  nil
if  else  while  for  return
int  long  float  double  bool  char  string  void
true  false
```

The last three groups (`if else while for return`, the type names, and
`true false`) are **pass-through keywords**: they are recognized by the
tokenizer so they cannot be shadowed as identifiers, but the rewriter
does not modify them. They behave exactly as in C++.

The first three groups are **translated keywords**: each has a specific
rewrite rule. See the corresponding section below.

Adding a keyword is a breaking change: any cminux source that used the word
as an identifier stops compiling. The keyword list is frozen at v1.0.0.

---

## 3. Source files

### 3.1 Extension

cminux source files use the `.cmx` extension by convention. The transpiler
does not require it — you can name a file anything and pass it to `cminux`
— but tooling, syntax highlighting, and the test harness all assume `.cmx`.

### 3.2 Structure

A cminux source file is a sequence of top-level items. There are no
constraints on ordering. A typical file has:

```
import ["header1.hpp", "header2.hpp"]

func f() -> int {
    ...
}

func main() -> int {
    ...
}
```

Imports appear at the top by convention but may appear anywhere at top
level. Function definitions may appear in any order. There is no `main`
requirement in the language — but the C++ linker requires one, so a program
meant to produce an executable must define `main`.

### 3.3 No declarations without bodies

cminux v1.0.0 does not support function forward declarations (a `func` with
no body, ending in `;`). This is a deliberate restriction: cminux programs
are single-file, and if you need a forward declaration, you are using C++
directly.

Writing `func foo() -> int;` is an error:

```
error: function declaration without body is not supported
```

---

## 4. Import

### 4.1 Syntax

```
import [ "path1.hpp" ]
import [ "path1.hpp", "path2.hpp" ]
import [ "path1.hpp", "path2.hpp", "path3.hpp" ]
```

The brackets are required. The list must contain at least one string
literal. Items are separated by commas. A trailing semicolon after `]` is
optional but recommended.

### 4.2 Semantics

`import [...]` expands to one `#include` line per string, in order, on
consecutive output lines. The paths are emitted as written — cminux does not
resolve them, does not check whether the files exist, and does not modify
them.

```
import ["outfn.hpp", "math.hpp"]
```

becomes:

```cpp
#include "outfn.hpp"
#include "math.hpp"
```

### 4.3 Examples

Valid:

```
import ["outfn.hpp"]
import ["a.hpp", "b.hpp", "c.hpp"]
import ["outfn.hpp"];
import [
    "a.hpp",
    "b.hpp"
]
```

Invalid:

```
import "outfn.hpp"              error: expected '[' after 'import'
import []                       error: expected string literal in import list
import [outfn.hpp]              error: expected string literal in import list
import ["a.hpp" "b.hpp"]        error: expected ',' or ']' in import list
import ["a.hpp", ]              error: expected string literal in import list
```

### 4.4 Relationship to `#include`

cminux does not support `#include`. Use `import [...]` instead. There is no
`import <>` form for system headers — use the quoted form and let the C++
compiler resolve it, or pass `-I` on the cminux command line.

---

## 5. Functions

### 5.1 Syntax

```
func name(params) -> ReturnType {
    body
}
```

or, for a function that returns nothing:

```
func name(params) {
    body
}
```

The `func` keyword, the parameter list in parentheses, and the body in
braces are all required. The return type after `->` is optional; if absent,
the function returns `void`.

The parameter list may be empty, may contain zero or more C++ parameter
declarations, and must have balanced parentheses. cminux does not interpret
parameter syntax — it copies it verbatim. This means parameters must be
valid C++ parameter syntax.

### 5.2 The `-> ReturnType` rewrite

When `-> ReturnType` is present, the return type is moved from after the
arrow to before the function name:

```
func add(int a, int b) -> int {
    render a + b;
}
```

becomes:

```cpp
int add(int a, int b) {
    return a + b;
}
```

The tokens between `->` and the body's `{` are copied verbatim as the
return type. Any C++ return type syntax works: `int`, `void`, `std::string`,
`MyClass*`, `const char*`, `std::vector<int>`, and so on.

When `-> ReturnType` is absent, `void` is synthesized as the return type:

```
func hello() {
    println("hi");
}
```

becomes:

```cpp
void hello() {
    println("hi");
}
```

### 5.3 Return type with templates

The rewriter scans tokens between `->` and the body's `{`. Template syntax
in a return type — `std::vector<int>` — is preserved because `->` opens the
scan and `{` closes it, and angle brackets in between are just tokens.

```
func make() -> std::vector<int> {
    render {};
}
```

becomes:

```cpp
std::vector<int> make() {
    return {};
}
```

### 5.4 The function name

The name after `func` must be an identifier. It is emitted unchanged before
the parameter list.

Reserved keywords cannot be function names. Writing `func if() -> int` is an
error at the `if` token: `expected function name after 'func'`.

### 5.5 Examples

Valid:

```
func f() -> int { render 0; }
func f() { }
func f(int a) -> int { render a; }
func f(int a, int b) -> int { render a + b; }
func f(std::string s) -> void { println(s); }
func f(int a) -> const char* { render "x"; }
```

Invalid:

```
func () -> int { }               error: expected function name after 'func'
func f( -> int { }               error: unterminated parameter list
func f() -> { }                  error: expected return type after '->'
func f() -> int;                 error: function declaration without body is not supported
func 123() -> int { }            error: expected function name after 'func'
```

### 5.6 Overloading, namespaces, and linkage

cminux has no namespaces, no `extern "C"`, no `static` functions, and no
`constexpr` functions. If you need them, write them in C++ and call them
from cminux.

Function overloading works, but only because C++ does the overloading —
cminux copies the parameters verbatim and lets the C++ compiler resolve the
call.

---

## 6. Return

### 6.1 Syntax

```
render expression ;
```

`render` is a statement terminator. It replaces C++'s `return`.

### 6.2 Semantics

`render` is replaced by `return`, token for token. Nothing else about the
statement changes. It may return a value (`render 0;`) or nothing
(`render;`).

```
render 0;
```

becomes:

```cpp
return 0;
```

```
render;
```

becomes:

```cpp
return;
```

### 6.3 Examples

Valid:

```
render 0;
render a + b;
render f(x);
render "hello";
render;
```

Invalid:

```
render                      error: (none — `render` followed by `;` is a bare return)
render 0                    error: (none at cminux level; C++ will report the missing `;`)
```

Note that cminux does not require `render` to have a value. `render;` inside
a `void` function is valid. `render 0;` inside a `void` function is a C++
error reported by the C++ compiler, on the line where `render 0;` appeared.

### 6.4 Why not `return`?

`return` is a C++ keyword and a valid identifier in cminux. You can write
`int return = 5;` in cminux and it will compile, because cminux does not
reserve the word `return`. This is a historical accident — cminux v0.1
needed *some* keyword to mark returns and `render` was chosen to avoid
collision. It has stayed.

---

## 7. Variables

### 7.1 Two forms

```
let name = expression ;
var name = expression ;
```

`let` declares an immutable variable (translated to `const auto`).
`var` declares a mutable variable (translated to `auto`).

Type inference is required in both forms. cminux v1.0.0 has no explicit
type annotation syntax. If you need an explicit type, write a cast:

```
var x = 5;                      auto x = 5;
let y = compute();              const auto y = compute();
var z = (double)42;             auto z = (double)42;
```

### 7.2 Semantics

`let` expands to two tokens: `const` and `auto`.

```
let greeting = "hello";
```

becomes:

```cpp
const auto greeting = "hello";
```

`var` expands to one token: `auto`.

```
var count = 0;
```

becomes:

```cpp
auto count = 0;
```

### 7.3 Examples

Valid:

```
let x = 5;
var y = 10;
let s = "hello";
var f = 3.14;
let p = nil;
var v = std::vector<int>{};
```

Invalid:

```
let x;                          error: (none at cminux level; C++ will report)
let x = ;                       error: (none at cminux level; C++ will report)
var = 5;                        error: (none at cminux level; C++ will report)
```

### 7.4 Scope

`let` and `var` follow C++ scoping rules, because they expand to `auto` in
whatever scope they appear in. A `let` inside an `if` block goes out of
scope at the end of the block, same as C++.

### 7.5 Reassignment

`var` variables may be reassigned. `let` variables may not — the C++ compiler
enforces this via the `const` keyword.

```
let x = 5;
x = 10;                         // C++ error: assignment to const variable
```

This is not a cminux error. It is a C++ error, reported by the C++ compiler,
on the line where `x = 10;` appears.

### 7.6 No type annotations

cminux v1.0.0 has no syntax for explicit type annotations. There is no
equivalent of C++'s `int x = 5;` in cminux source. If you need a specific
type, cast the initializer:

```
var x = (long)5;                // auto x = (long)5; — x is long
```

### 7.7 No multi-declaration

cminux does not support declaring multiple variables in one statement:

```
var x = 1, y = 2;               // NOT valid cminux
```

Write two separate statements:

```
var x = 1;
var y = 2;
```

---

## 8. Nil

### 8.1 Syntax

`nil` is a literal. It may appear anywhere a C++ expression may appear.

```
var p = nil;
if p == nil { ... }
f(nil);
```

### 8.2 Semantics

`nil` is replaced by `nullptr`.

```
var p = nil;
```

becomes:

```cpp
auto p = nullptr;
```

### 8.3 Examples

Valid:

```
var p = nil;
if p == nil { }
if nil == p { }
```

Invalid:

```
nil = 5;                        // C++ error: nullptr is not assignable
var nil = 5;                    // error: nil is a keyword, cannot be an identifier
```

### 8.4 `nil` has type `std::nullptr_t`

Because `nil` expands to `nullptr`, it has the type `std::nullptr_t`, not
`void*`. This matters for overload resolution: `f(nil)` calls `f(std::nullptr_t)`,
not `f(int)` or `f(void*)`.

If you want a null pointer of a specific type, use a cast:

```
var p = (int*)nil;              // auto p = (int*)nullptr;
```

---

## 9. Output functions

### 9.1 `print` and `println`

`print` and `println` are not keywords in cminux. They are functions
declared in `outfn.hpp`, brought into scope by the prologue that the
emitter writes at the top of every generated file.

```
print("hello");
println("hello");
print(42);
println(3.14);
```

The prologue is:

```cpp
#include "outfn.hpp"
using cmx::print;
using cmx::println;
```

This means `print(x)` resolves to `cmx::print(x)`, with `cmx::print`
overloaded for `int`, `long`, `long long`, unsigned variants, `float`,
`double`, `bool`, `char`, `const char*`, `std::string`, and
`std::string_view`.

### 9.2 Optional import

`import ["outfn.hpp"]` is not required to use `print` and `println` — the
prologue already includes it. But writing it explicitly is harmless and
documents your dependency. The `import` and the prologue both emit
`#include "outfn.hpp"`, and `outfn.hpp` has include guards, so the second
include is a no-op.

### 9.3 What `print` and `println` do

`print(x)` writes `x` to `stdout` with no trailing newline, then flushes.
`println(x)` writes `x` followed by a newline, then flushes. `println()`
with no argument writes only a newline.

For full details, see `docs/output-library.md`.

### 9.4 What they do not do

`print` and `println` do not format numbers — they write them the way
`std::cout` writes them. If you want width, precision, or hexadecimal, use
`printf`:

```
printf("%x\n", 255);
```

`printf` is also declared in `outfn.hpp` under `namespace cmx`, and the
prologue's `using` declarations bring it into scope alongside `print` and
`println`.

### 9.5 Overloading

Because `print` and `println` are C++ functions with C++ overloads, you can
add your own overloads by defining them in a header and importing that
header:

```
import ["my_print.hpp"]
```

```cpp
// my_print.hpp
#include "outfn.hpp"
namespace cmx {
    template<typename T>
    void print(const std::vector<T>& v) {
        std::cout << "[";
        for (size_t i = 0; i < v.size(); ++i) {
            if (i) std::cout << ", ";
            print(v[i]);
        }
        std::cout << "]";
    }
}
```

Then `print(some_vector)` works, because overload resolution picks your
template.

---

## 10. Everything else

Everything not described in sections 4–9 is C++ as-is. This includes, but is
not limited to:

- `if`, `else`, `while`, `for`, `switch`, `case`, `break`, `continue`, `goto`
- `class`, `struct`, `union`, `enum`, `namespace`, `using`
- `template`, `typename`, `concept`, `requires`
- `try`, `catch`, `throw`
- `new`, `delete`
- `static`, `extern`, `inline`, `constexpr`, `consteval`, `consteval`
- Type names: `int`, `long`, `float`, `double`, `bool`, `char`, `void`
- Literals: `true`, `false`, `0`, `42`, `3.14`, `'a'`, `"str"`
- Operators: `+`, `-`, `*`, `/`, `%`, `==`, `!=`, `<`, `>`, `&&`, `||`, `!`, etc.
- Everything in the C++ standard library

cminux does not modify, check, or interact with any of it. What you write is
what the C++ compiler sees.

### 10.1 Two important caveats

**First**, the C++ keywords `if`, `while`, `for`, `return`, `class`, and
similar are **not** reserved by cminux. You can write `var if = 5;` and it
will compile as `auto if = 5;` — and then fail at C++ compile time, because
`if` is a C++ keyword. cminux does not protect you from this. If you want a
variable named `if`, you cannot have one, but you *can* write the declaration
and get the error from the C++ compiler instead of from cminux.

**Second**, the cminux keywords `func`, `render`, `import`, `let`, `var`, and
`nil` **are** reserved by cminux, but they are not reserved by C++. You
cannot use them as identifiers in cminux, but if you write C++ headers that
use them, cminux does not care. The one exception is if a cminux keyword
appears in a context where the rewriter expects it not to — for example,
`func` at top level is always a function definition. If you have a C++
header that defines a type called `func`, you cannot refer to it from
cminux source.

---

## 11. Grammar summary

The complete grammar of cminux v1.0.0, in a compact form. Terminals are
quoted. Non-terminals are lowercase. Everything in `[…]*` is zero or more.

```
source-file     ::= item*
item            ::= import-stmt
                  | function-def
                  | cpp-statement

import-stmt     ::= "import" "[" string ("," string)* "]" ";"?

function-def    ::= "func" ident "(" params ")" ("->" type)? "{" body "}"

params          ::= balanced-tokens until ')'
type            ::= tokens until '{'
body            ::= tokens until '}'  (balanced)

cpp-statement   ::= any token sequence

let-stmt        ::= "let" ident "=" expr ";"
var-stmt        ::= "var" ident "=" expr ";"

render-stmt     ::= "render" expr? ";"

string          ::= '"' (any char except '"' or '\' | '\"' | '\\')* '"'
ident           ::= [A-Za-z_][A-Za-z0-9_]*
```

The `params`, `type`, and `body` rules are intentionally loose: cminux does
not parse them, it only copies them. This is why cminux source can contain
arbitrarily complex C++ in a function body — cminux does not look inside.

---

## Appendix A: What cminux v1.0.0 does not have

For clarity, a list of things cminux does **not** have, and why:

- **Classes.** Use C++ classes. They work as-is.
- **Templates.** Use C++ templates. They work as-is.
- **Namespaces.** Use C++ namespaces. They work as-is.
- **Modules.** Use `import ["file.hpp"]`; the C++ compiler sees `#include`.
- **Type annotations.** Use casts: `var x = (long)5;`.
- **`constexpr`.** Use C++ `constexpr` in a header, call it from cminux.
- **Lambdas.** Use C++ lambdas. They work as-is.
- **Exceptions.** Use C++ exceptions. They work as-is.
- **Concurrency.** Use C++ threads, mutexes, atomics. They work as-is.
- **Standard library.** Use C++ standard library. It works as-is.
- **Package manager.** Use CMake, vcpkg, Conan, or fetch headers manually.
- **Build system.** Use CMake, Make, or compile each `.cmx` file individually.

If you want one of these, use C++. cminux does not add them, does not
abstract them, does not hide them. It removes nine pieces of ceremony and
gets out of the way.

---

## Appendix B: The nine rules

The complete rewrite table, for reference. This is the entire language.

| cminux                          | emits                                       |
|---------------------------------|---------------------------------------------|
| `import ["a.hpp", "b.hpp"]`     | `#include "a.hpp"` / `#include "b.hpp"`     |
| `func f() -> T { ... }`         | `T f() { ... }`                             |
| `func f() { ... }`              | `void f() { ... }`                          |
| `render e;`                     | `return e;`                                 |
| `nil`                           | `nullptr`                                   |
| `let x = e;`                    | `const auto x = e;`                         |
| `var x = e;`                    | `auto x = e;`                               |
| `print(...)`                    | `cmx::print(...)` (via prologue)            |
| `println(...)`                  | `cmx::println(...)` (via prologue)          |

Nine rules. One language. Everything else is C++.