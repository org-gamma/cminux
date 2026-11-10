# cminux Output Library Reference

**Version:** 1.0.0

The cminux output library is a single header, `lib/outfn.hpp`, plus an
anchor translation unit, `lib/src/outfn.cpp`. It provides `print`, `println`,
and `printf` for cminux programs. That is the entire library.

This document covers every function the library provides, every overload,
and the exact behavior of each. It also covers how to add new overloads
without modifying the library itself.

---

## Table of contents

1. [Overview](#1-overview)
2. [The header](#2-the-header)
3. [print](#3-print)
4. [println](#4-println)
5. [printf](#5-printf)
6. [What the library does not provide](#6-what-the-library-does-not-provide)
7. [Adding your own overloads](#7-adding-your-own-overloads)
8. [Type deduction and overload resolution](#8-type-deduction-and-overload-resolution)
9. [Output buffering and flushing](#9-output-buffering-and-flushing)
10. [Thread safety](#10-thread-safety)
11. [The .cpp file](#11-the-cpp-file)

---

## 1. Overview

The library's job is to make this work in cminux source:

```
println("hello");
print(42);
print(3.14);
println();
```

Without the library, every cminux program would have to write
`std::cout << "hello" << std::endl;` — verbose, and unlike any other modern
language. The library provides short names for the two operations every
program needs: write a value, write a value followed by a newline.

The library is deliberately small. There is no `Vec`, no `String`, no `Map`,
no `Result`, no `Option`, no `Path`, no `File`. If you want those, use the
C++ standard library — it is fully available, because cminux emits C++ and
the C++ standard library is part of C++.

The library is not a framework, not a runtime, and not a replacement for
`<iostream>`. It is a thin veneer over `<iostream>` and `<cstdio>` that
provides three functions.

---

## 2. The header

The library is declared in `lib/outfn.hpp`. The file is organized as
follows:

```cpp
#ifndef CMINUX_OUTFN_HPP
#define CMINUX_OUTFN_HPP

#include <cstdio>
#include <string>
#include <string_view>
#include <iostream>

namespace cmx {

    // print overloads (section 3)
    // println overloads (section 4)
    // printf (section 5)

}

using cmx::print;
using cmx::println;

#endif
```

Everything is inside `namespace cmx`. The `using` declarations at the bottom
bring `print` and `println` into the global namespace, so cminux source can
write `print(x)` without `cmx::print(x)`.

The prologue emitted by the transpiler also does `using cmx::print;` and
`using cmx::println;`. This is redundant with the `using` declarations in
the header, but harmless — C++ allows multiple `using` declarations for the
same name.

### Include guards

`CMINUX_OUTFN_HPP` prevents multiple inclusion. Every header that might be
included more than once — which is all of them — needs an include guard, and
this one has one. Do not remove it.

### Dependencies

The header depends only on `<cstdio>`, `<string>`, `<string_view>`, and
`<iostream>`. `<cstdio>` is for `printf`. `<string>` and `<string_view>` are
for the string overloads. `<iostream>` is for `std::cout`.

If MSVC complains about `va_list` in the `printf` forwarder, add
`#include <cstdarg>` at the top of the header. This is a known quirk of
MSVC's `<cstdio>` on some versions.

---

## 3. print

### 3.1 Signatures

```cpp
inline void print(std::string_view s);
inline void print(const std::string& s);
inline void print(const char* s);
inline void print(int v);
inline void print(long v);
inline void print(long long v);
inline void print(unsigned v);
inline void print(unsigned long v);
inline void print(unsigned long long v);
inline void print(float v);
inline void print(double v);
inline void print(bool v);
inline void print(char v);
```

### 3.2 Behavior

`print(x)` writes `x` to `stdout` with no trailing newline, then flushes.

For strings, the characters of the string are written verbatim. No escaping,
no quoting, no formatting.

For integers, the decimal representation is written. Negative numbers are
written with a leading `-`. There is no width padding, no grouping
separators, no sign prefix for non-negative numbers.

For floating-point values, the default `std::cout` formatting is used:
shortest representation that round-trips to the same value, up to the
default precision of 6 significant digits. For `3.14` this prints `3.14`;
for `1.0/3.0` this prints `0.333333`. If you want more precision, use
`printf("%.17g\n", x)`.

For `bool`, `true` is written as the four characters `true` and `false` as
the five characters `false`. This differs from `std::cout`'s default, which
prints `1` and `0`. The library chooses readable output over `std::cout`
defaults; if you want `1` and `0`, cast to `int` before printing:
`print((int)some_bool)`.

For `char`, the character itself is written, not its numeric value. To write
the numeric value, cast to `int`.

### 3.3 Overload resolution

The overloads are selected by C++ overload resolution on the argument type.
If you pass an `int`, `print(int)` is chosen. If you pass a `double`,
`print(double)` is chosen. If you pass a `std::string`, the
`print(const std::string&)` overload is chosen. If you pass a string
literal, `print(const char*)` is chosen.

If you pass a type not covered by an overload — for example,
`std::vector<int>` — the call fails to compile with `no matching function
for call to 'print'`. See section 7 for how to add your own overloads.

### 3.4 Examples

```
print("hello");             // hello
print(42);                  // 42
print(-17);                 // -17
print(3.14);                // 3.14
print(0.1 + 0.2);           // 0.3
print(true);                // true
print(false);               // false
print('x');                 // x
print("");                  // (nothing)
```

### 3.5 No variadic form

`print` has no variadic form. `print("x =", x)` is not valid. Write two
calls:

```
print("x = ");
println(x);
```

or use `printf`:

```
printf("x = %d\n", x);
```

---

## 4. println

### 4.1 Signatures

```cpp
inline void println();
inline void println(std::string_view s);
inline void println(const std::string& s);
inline void println(const char* s);
inline void println(int v);
inline void println(long v);
inline void println(long long v);
inline void println(unsigned v);
inline void println(unsigned long v);
inline void println(unsigned long long v);
inline void println(float v);
inline void println(double v);
inline void println(bool v);
inline void println(char v);
```

### 4.2 Behavior

`println(x)` is `print(x)` followed by a newline. For every overload,
`println(x)` is exactly equivalent to `print(x); print('\n');` — except that
it flushes once at the end rather than twice, which matters only for
performance and not for correctness.

`println()` with no arguments writes only a newline.

### 4.3 Examples

```
println("hello");           // hello\n
println(42);                // 42\n
println(3.14);              // 3.14\n
println(true);              // true\n
println();                  // \n
```

### 4.4 No variadic form

Same as `print`: no variadic form. `println("x =", x)` is not valid.

---

## 5. printf

### 5.1 Signature

```cpp
inline int printf(const char* fmt, ...);
```

### 5.2 Behavior

`printf` forwards to `std::vprintf`. It writes formatted output to `stdout`
according to `fmt`, then flushes `stdout`, then returns the number of
characters written (or a negative value on error).

The behavior is identical to C's `printf`, because that is exactly what it
is. All format specifiers work: `%d`, `%u`, `%x`, `%s`, `%f`, `%e`, `%g`,
`%c`, `%p`, `%%`, and width and precision modifiers.

### 5.3 Namespacing

`printf` is declared inside `namespace cmx`. The prologue does not bring
`printf` into scope (unlike `print` and `println`), so you must either write
`cmx::printf(...)` or add `using cmx::printf;` yourself.

Alternatively, C's `printf` is available via `<cstdio>` without qualification,
because `std::printf` in `<cstdio>` is also exposed as `::printf` on most
implementations. If you `import ["<cstdio>"]` — wait, cminux does not
support `import <>`. Use `import ["cstdio"]` if your compiler resolves it,
or just call `cmx::printf` directly.

The cleanest option in v1.0.0 is:

```
import ["outfn.hpp"]
func main() -> int {
    cmx::printf("%d\n", 42);
    render 0;
}
```

### 5.4 Why not just `printf`?

The library namespaces `printf` under `cmx` to avoid a subtle hazard:
`std::cout` and `printf` do not always agree about the order of their
output when both are used in the same program, because they use separate
buffers. If you mix `print("x")` (which uses `std::cout`) with
`printf("y\n")` (which uses `stdout` via `std::vprintf`), you may see `y`
before `x` even though `print("x")` came first.

The library mitigates this by flushing `std::cout` after every `print` and
`println`, and by flushing `stdout` after every `printf`. This makes the
interleaving correct, but at the cost of a flush on every call. For
performance-critical output, use `printf` for everything, or use
`std::cout` directly (in C++ interop code) for everything.

### 5.5 Examples

```
cmx::printf("%d\n", 42);                // 42\n
cmx::printf("%s = %d\n", "x", 5);       // x = 5\n
cmx::printf("%.2f\n", 3.14159);         // 3.14\n
cmx::printf("%x\n", 255);               // ff\n
cmx::printf("%5d|\n", 42);              //    42|\n
cmx::printf("%-5d|\n", 42);             // 42   |\n
```

---

## 6. What the library does not provide

The library does **not** provide:

- Containers (`Vec`, `Map`, `Set`, `Queue`, `Stack`) — use `std::vector`,
  `std::unordered_map`, `std::set`, `std::queue`, `std::stack`
- Strings (`String`) — use `std::string`
- Files (`File`) — use `std::fstream` or `std::ifstream` / `std::ofstream`
- Paths (`Path`) — use `std::filesystem::path` (C++17)
- Errors (`Result`, `Option`) — use `std::optional`, `std::variant`, or
  exceptions
- Threads (`Thread`) — use `std::thread`
- Time (`Duration`, `Instant`) — use `std::chrono`
- Random (`Rng`) — use `<random>`
- Formatting (`format`) — use `printf`, `std::ostringstream`, or
  `std::format` (C++20)
- Input (`read`, `readline`) — use `std::cin` or `std::getline`
- Logging — use `printf` or `std::cerr`
- Assertions — use `<cassert>` or write your own

Every one of these has an excellent C++ equivalent. The library exists only
because `print` and `println` are so common that making every cminux program
write `std::cout << ... << std::endl;` would defeat the purpose of the
language. Nothing else crosses that bar.

### 6.1 Why not `read`?

Input is harder to get right than output, and every library that provides
`read` also provides a buffering model, an error model, and an EOF model —
three design decisions that would need to be made, documented, and frozen.
For v1.0.0, the answer is `std::cin`:

```cpp
int x;
std::cin >> x;
```

If you want a `read` function, write it in your own header and import it.
The library will not grow one; the C++ standard library already has one.

---

## 7. Adding your own overloads

Because `print` and `println` are ordinary C++ functions, not keywords, you
can extend them without modifying `lib/outfn.hpp`. Define your overloads in
a header, import that header, and the overloads are available.

### 7.1 Example: printing a `std::vector`

```cpp
// my_print.hpp
#ifndef MY_PRINT_HPP
#define MY_PRINT_HPP

#include "outfn.hpp"
#include <vector>

namespace cmx {

template<typename T>
void print(const std::vector<T>& v) {
    std::cout << "[";
    for (size_t i = 0; i < v.size(); ++i) {
        if (i > 0) std::cout << ", ";
        print(v[i]);
    }
    std::cout << "]";
}

template<typename T>
void println(const std::vector<T>& v) {
    print(v);
    std::cout << "\n";
    std::cout.flush();
}

}

#endif
```

Then in cminux source:

```
import ["outfn.hpp", "my_print.hpp"]

func main() -> int {
    var v = std::vector<int>{1, 2, 3};
    println(v);                    // [1, 2, 3]
    render 0;
}
```

The overload is chosen by C++ overload resolution: `std::vector<int>` does
not match any built-in overload, so the template is selected, and the
template's body calls `print` recursively (which resolves to `print(int)` for
each element).

### 7.2 Example: printing a custom type

```cpp
// my_print.hpp
#ifndef MY_PRINT_HPP
#define MY_PRINT_HPP

#include "outfn.hpp"

struct Point { int x; int y; };

namespace cmx {

inline void print(const Point& p) {
    std::cout << "(" << p.x << ", " << p.y << ")";
}

inline void println(const Point& p) {
    print(p);
    std::cout << "\n";
    std::cout.flush();
}

}

#endif
```

```
import ["outfn.hpp", "my_print.hpp"]

func main() -> int {
    let p = Point{1, 2};
    println(p);                    // (1, 2)
    render 0;
}
```

### 7.3 The rule

If you find yourself wanting a new overload, add it to a header you own.
Do not modify `lib/outfn.hpp`. The library is frozen at v1.0.0; user
overloads live in user headers.

This keeps the library small and the transpiler simple. The transpiler does
not need to know about your overloads — they resolve via C++ overload
resolution, same as the built-in ones.

---

## 8. Type deduction and overload resolution

Because `print` and `println` are C++ functions, they participate in C++
overload resolution. This has consequences you should know about.

### 8.1 `print(0)` prints `0`, not `nullptr`

`0` is an `int`, so `print(int)` is selected. If you want to print a null
pointer, pass `nullptr` or `nil`:

```
print(nil);                    // this is print(std::nullptr_t) — see 8.3
```

### 8.2 `print(x)` where `x` is `auto`

If `x` is declared with `auto` (or `var` in cminux), its type is deduced from
the initializer, and `print(x)` selects the corresponding overload.

```
var x = 5;                     // x is int
print(x);                      // print(int)
```

```
var x = 5.0;                   // x is double
print(x);                      // print(double)
```

```
var x = "hello";               // x is const char*
print(x);                      // print(const char*)
```

```
var x = std::string("hello");  // x is std::string
print(x);                      // print(const std::string&)
```

### 8.3 `print(nil)`

`nil` expands to `nullptr`, which has type `std::nullptr_t`. There is no
`print(std::nullptr_t)` overload, so this is a compile error:
`no matching function for call to 'print'`. To print a null pointer, cast it
to `void*` first:

```
print((void*)nil);             // (some implementation-defined output)
```

### 8.4 `print((char)65)`

If you cast to `char`, `print(char)` is selected and the character `A` is
written. If you want the number `65`, do not cast:

```
print(65);                     // 65
print((char)65);               // A
```

### 8.5 Ambiguous conversions

If the argument can convert to multiple overload types with equal rank —
which is rare but possible with user-defined types — the call is ambiguous
and fails to compile. This is a C++ rule, not a cminux rule. The fix is to
cast the argument to the exact overload you want:

```
print((double)some_int);       // forces print(double)
```

---

## 9. Output buffering and flushing

Every `print`, `println`, and `printf` call in the library flushes `stdout`
at the end of the call. This has two consequences.

### 9.1 Output is always visible

If a cminux program crashes, all output up to the crash point has been
written to the console. There is no partially buffered output that is lost
in a crash. This is intentional and matches what users expect from modern
languages.

### 9.2 Output is slow

A flush on every call means a system call on every call. For a program that
prints 1,000,000 lines in a loop, this can be orders of magnitude slower
than buffered output. If you are writing performance-sensitive output code,
bypass the library and use `std::cout` directly in a C++ interop header, or
write a batched output helper in your own header.

### 9.3 Interleaving with `printf`

Because `print` flushes `std::cout` and `printf` flushes `stdout`, and
`std::cout` and `stdout` share the same file descriptor when `sync_with_stdio`
is true (the default), output from the two functions interleaves in call
order. If you disable `sync_with_stdio`, they do not interleave correctly,
and you will see garbled output. Do not disable `sync_with_stdio` if you
call both `print` and `printf`.

---

## 10. Thread safety

The library is not thread-safe by itself. `std::cout` has its own internal
locking, so a single call to `print` from a single thread is atomic with
respect to itself. But a sequence like `print("x = "); println(x);` from
multiple threads can interleave, producing output like `x = 1x = 2`.

If you need thread-safe output, protect it yourself:

```cpp
// safe_print.hpp
#ifndef SAFE_PRINT_HPP
#define SAFE_PRINT_HPP

#include "outfn.hpp"
#include <mutex>

namespace cmx {
inline std::mutex& print_mutex() {
    static std::mutex m;
    return m;
}

template<typename T>
void safe_println(const T& v) {
    std::lock_guard<std::mutex> lock(print_mutex());
    println(v);
}
}

#endif
```

Then `safe_println(x)` writes a whole line atomically.

---

## 11. The .cpp file

The library has one translation unit, `lib/src/outfn.cpp`. It is:

```cpp
#include "../outfn.hpp"

namespace cmx {

// Reserved for future non-inline implementation.

}
```

That is the entire file. It is empty except for the include and the
namespace. Its only job is to give the library a real object file so that
CMake can link against it explicitly.

Everything in `outfn.hpp` is `inline`, which means the compiler does not
need a `.cpp` file to use it. But a library without a `.cpp` file is awkward
to build — CMake wants at least one source file per target — and having a
real `.cpp` makes it easy to add non-inline state later (a buffering layer,
a mutex, a redirection mechanism for tests) without restructuring.

For v1.0.0, the `.cpp` is a placeholder. If you add non-inline state to the
library — which you should not, but if you do — it lives here.

### 11.1 Why an anchor file at all?

Three reasons.

**First**, CMake. A `STATIC` library needs at least one source file. An
empty `.cpp` is the idiomatic way to make an otherwise header-only library
buildable as a library.

**Second**, extensibility. When you inevitably want to add buffering or
locking to the library (see sections 9 and 10), you will need a place to put
the non-inline state. The `.cpp` file is that place, and it exists from day
one.

**Third**, linking semantics. Some build configurations (static linking,
link-time optimization) behave differently with header-only libraries than
with libraries that have a translation unit. Having a `.cpp` file makes the
library's linkage explicit and predictable.

### 11.2 What not to put in the .cpp

Do not put the implementations of `print` and `println` in the `.cpp`. They
are `inline` in the header, and they should stay `inline`. Moving them to
the `.cpp` would require changing the `inline` keyword to `extern` and
adding explicit instantiation or linkage directives, and there is no benefit
— the functions are tiny and the compiler inlines them at the call site
anyway.

Do not put user-specific code in the `.cpp`. If a user wants to add an
overload of `print`, they add it to their own header, not to
`lib/outfn.cpp`.

Do not put test code in the `.cpp`. Tests live in `tests/`.

---

## Appendix A: complete API summary

```cpp
namespace cmx {

// print — write a value with no trailing newline, then flush
void print(std::string_view);
void print(const std::string&);
void print(const char*);
void print(int);
void print(long);
void print(long long);
void print(unsigned);
void print(unsigned long);
void print(unsigned long long);
void print(float);
void print(double);
void print(bool);
void print(char);

// println — write a value followed by '\n', then flush
void println();
void println(std::string_view);
void println(const std::string&);
void println(const char*);
void println(int);
void println(long);
void println(long long);
void println(unsigned);
void println(unsigned long);
void println(unsigned long long);
void println(float);
void println(double);
void println(bool);
void println(char);

// printf — formatted output, forwarded to std::vprintf
int printf(const char* fmt, ...);

}
```

All functions are `inline`. `print` and `println` are also brought into the
global namespace via `using` declarations at the bottom of `outfn.hpp`.

---

## Appendix B: examples

### B.1 Hello, world

```
import ["outfn.hpp"]
func main() -> int {
    println("hello, world");
    render 0;
}
```

Output: `hello, world\n`

### B.2 All supported types

```
import ["outfn.hpp"]
func main() -> int {
    print("string: ");       println("hello");
    print("int: ");          println(42);
    print("long: ");         println(42L);
    print("long long: ");    println(42LL);
    print("unsigned: ");     println(42U);
    print("float: ");        println(3.14f);
    print("double: ");       println(3.14);
    print("bool: ");         println(true);
    print("char: ");         println('x');
    render 0;
}
```

### B.3 Formatted output with printf

```
import ["outfn.hpp"]
func main() -> int {
    cmx::printf("%d + %d = %d\n", 2, 3, 2 + 3);
    cmx::printf("%.3f\n", 3.14159);
    cmx::printf("%-10s|%10s|\n", "left", "right");
    render 0;
}
```

Output:

```
2 + 3 = 5
3.142
left      |     right|
```

### B.4 Custom overload

```
import ["outfn.hpp", "my_print.hpp"]
func main() -> int {
    var v = std::vector<int>{1, 2, 3};
    println(v);                // [1, 2, 3]
    render 0;
}
```

---

## Appendix C: the library's design rules

For anyone considering a change to the library, three rules have guided
every decision and should guide future ones.

**Rule 1: The library exists only to remove ceremony from programs that
would otherwise be ugly in C++.**

`print("hello")` is cleaner than `std::cout << "hello";`. `println("hello")`
is cleaner than `std::cout << "hello" << std::endl;`. Those two functions
justify the library. Nothing else does. If a proposed addition can be
expressed as a clean line of C++ — which is true of almost everything — it
does not belong in the library.

**Rule 2: The library adds no semantics.**

`print(x)` writes `x`. `println(x)` writes `x` and a newline. `printf` does
what C's `printf` does. There is no formatting mini-language, no color codes,
no buffering strategy, no log levels, no timestamp insertion, no redirection
hooks. If you want any of these, write them in your own header. The library
is a set of thin wrappers over `std::cout` and `std::vprintf`, and it should
stay that way.

**Rule 3: If the library cannot express something cleanly with C++ overload
resolution, it does not express it at all.**

`print` and `println` use C++ overload resolution for everything. There is
no template magic, no `if constexpr`, no SFINAE, no type traits. Adding a
new overload means adding a new function signature, with a body that does
one obvious thing. If a proposed feature would require the library to
inspect the type of its argument at compile time beyond what overload
resolution already does, it is out of scope.

These three rules have kept the library under 200 lines since v0.1. Breaking
any of them would make the library grow without bound, and would require the
transpiler to care about function semantics, which it does not. The library
is small on purpose, and it should stay small on purpose.