// lib/outfn.hpp
// cminux output library — the only runtime dependency of cminux programs.
//
// This header is included by every transpiled .cpp file. It is intentionally
// small. Every function here is callable directly from cminux source with
// no wrapping.
//
// Design rule: if you want a new output behavior in cminux, add an overload
// here. Never add a special case to the transpiler for a function that could
// live in this library.

#ifndef CMINUX_OUTFN_HPP
#define CMINUX_OUTFN_HPP

#include <cstdio>
#include <string>
#include <string_view>
#include <iostream>

namespace cmx {

// ---------------------------------------------------------------------------
// print / println — string overloads
// ---------------------------------------------------------------------------

inline void print(std::string_view s) {
    std::cout << s;
    std::cout.flush();
}

inline void print(const std::string& s) {
    print(std::string_view(s));
}

inline void print(const char* s) {
    print(std::string_view(s));
}

inline void println() {
    std::cout << '\n';
    std::cout.flush();
}

inline void println(std::string_view s) {
    std::cout << s << '\n';
    std::cout.flush();
}

inline void println(const std::string& s) {
    println(std::string_view(s));
}

inline void println(const char* s) {
    println(std::string_view(s));
}

// ---------------------------------------------------------------------------
// print / println — scalar overloads
// ---------------------------------------------------------------------------

inline void print(int v)                { std::cout << v; std::cout.flush(); }
inline void print(long v)               { std::cout << v; std::cout.flush(); }
inline void print(long long v)          { std::cout << v; std::cout.flush(); }
inline void print(unsigned v)           { std::cout << v; std::cout.flush(); }
inline void print(unsigned long v)      { std::cout << v; std::cout.flush(); }
inline void print(unsigned long long v) { std::cout << v; std::cout.flush(); }
inline void print(float v)              { std::cout << v; std::cout.flush(); }
inline void print(double v)             { std::cout << v; std::cout.flush(); }
inline void print(bool v)               { std::cout << (v ? "true" : "false"); std::cout.flush(); }
inline void print(char v)               { std::cout << v; std::cout.flush(); }

inline void println(int v)                { std::cout << v << '\n'; std::cout.flush(); }
inline void println(long v)               { std::cout << v << '\n'; std::cout.flush(); }
inline void println(long long v)          { std::cout << v << '\n'; std::cout.flush(); }
inline void println(unsigned v)           { std::cout << v << '\n'; std::cout.flush(); }
inline void println(unsigned long v)      { std::cout << v << '\n'; std::cout.flush(); }
inline void println(unsigned long long v) { std::cout << v << '\n'; std::cout.flush(); }
inline void println(float v)              { std::cout << v << '\n'; std::cout.flush(); }
inline void println(double v)             { std::cout << v << '\n'; std::cout.flush(); }
inline void println(bool v)               { std::cout << (v ? "true" : "false") << '\n'; std::cout.flush(); }
inline void println(char v)               { std::cout << v << '\n'; std::cout.flush(); }

// ---------------------------------------------------------------------------
// printf — forwarded to the C library, but namespaced so cminux programs
// cannot accidentally call the global one through shadowing.
// ---------------------------------------------------------------------------

inline int printf(const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    int n = std::vprintf(fmt, ap);
    va_end(ap);
    std::fflush(stdout);
    return n;
}

} // namespace cmx

// ---------------------------------------------------------------------------
// cminux programs do not write `cmx::` — the transpiler emits a `using`
// directive so that `print("hi")` resolves to `cmx::print`.
// ---------------------------------------------------------------------------

using cmx::print;
using cmx::println;

#endif // CMINUX_OUTFN_HPP