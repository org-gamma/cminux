// lib/outfn.hpp
// cminux output library — print, println, printf, and overloads for the
// standard library types cminux users reach for most often.
//
// Design rule: if you want a new output behavior in cminux, add an overload
// here. Never add a special case to the transpiler for a function that
// could live in this library.

#ifndef CMINUX_OUTFN_HPP
#define CMINUX_OUTFN_HPP

#include <cstdio>
#include <cstdarg>
#include <string>
#include <string_view>
#include <iostream>
#include <optional>
#include <utility>
#include <tuple>

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
// print / println — std::nullptr_t
//
// Note: `nil` in cminux source becomes `nullptr`, which has type
// std::nullptr_t. Printing `nil` without this overload would be a compile
// error. The output is the word "nil", matching the cminux keyword.
// ---------------------------------------------------------------------------

inline void print(std::nullptr_t)              { std::cout << "nil"; std::cout.flush(); }
inline void println(std::nullptr_t)            { std::cout << "nil" << '\n'; std::cout.flush(); }

// ---------------------------------------------------------------------------
// print / println — std::optional<T>
//
// Prints the contained value, or "none" if empty. Uses the same print
// recursion as vectors (see vectorfn.hpp), so optional<vector<int>>
// prints as "[1, 2, 3]" or "none".
// ---------------------------------------------------------------------------

template<typename T>
inline void print(const std::optional<T>& o) {
    if (o.has_value()) {
        print(*o);
    } else {
        std::cout << "none";
        std::cout.flush();
    }
}

template<typename T>
inline void println(const std::optional<T>& o) {
    if (o.has_value()) {
        println(*o);
    } else {
        std::cout << "none" << '\n';
        std::cout.flush();
    }
}

// ---------------------------------------------------------------------------
// print / println — std::pair<A, B>
//
// Prints "(a, b)". Both elements are printed via the overloads above,
// so any pair whose elements are themselves printable is printable.
// ---------------------------------------------------------------------------

template<typename A, typename B>
inline void print(const std::pair<A, B>& p) {
    std::cout << "(";
    print(p.first);
    std::cout << ", ";
    print(p.second);
    std::cout << ")";
    std::cout.flush();
}

template<typename A, typename B>
inline void println(const std::pair<A, B>& p) {
    print(p);
    std::cout << '\n';
    std::cout.flush();
}

// ---------------------------------------------------------------------------
// print / println — std::tuple<...>
//
// Prints "(a, b, c, ...)". Supported for tuples of up to 6 elements.
// For larger tuples, write a custom overload in your own header and
// import it; this header intentionally does not support unbounded arity
// because the recursive template machinery for that is more code than
// the feature is worth.
// ---------------------------------------------------------------------------

namespace detail {

template<std::size_t I, typename... Ts>
struct tuple_printer {
    static void print_tuple(const std::tuple<Ts...>& t) {
        print(std::get<I - 1>(t));
        std::cout << ", ";
        tuple_printer<I - 1, Ts...>::print_tuple(t);
    }
};

template<typename... Ts>
struct tuple_printer<0, Ts...> {
    static void print_tuple(const std::tuple<Ts...>&) {}
};

inline void print_tuple_prefix() {}

template<typename T, typename... Ts>
void print_tuple_prefix(const T& first, const Ts&... rest) {
    print(first);
    if (sizeof...(rest) > 0) {
        std::cout << ", ";
        print_tuple_prefix(rest...);
    }
}

} // namespace detail

template<typename... Ts>
inline void print(const std::tuple<Ts...>& t) {
    std::cout << "(";
    if constexpr (sizeof...(Ts) > 0) {
        std::apply([](const auto&... elems) {
            detail::print_tuple_prefix(elems...);
        }, t);
    }
    std::cout << ")";
    std::cout.flush();
}

template<typename... Ts>
inline void println(const std::tuple<Ts...>& t) {
    print(t);
    std::cout << '\n';
    std::cout.flush();
}

// ---------------------------------------------------------------------------
// printf — forwarded to the C library, namespaced under cmx to avoid a
// name collision with the C library's printf when both are in scope.
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
// Bring the common names into the global namespace. cminux source writes
// `print("hi")`, not `cmx::print("hi")`. The prologue the transpiler
// emits also does `using cmx::print;`, so this block is technically
// redundant — but it makes the header usable from plain C++ too.
// ---------------------------------------------------------------------------

using cmx::print;
using cmx::println;

#endif // CMINUX_OUTFN_HPP