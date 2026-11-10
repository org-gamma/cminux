// lib/stringfn.hpp
// cminux string utilities — split, join, trim, case, prefix/suffix, replace.
//
// All functions operate on std::string. None of them defines a new string
// type. If you want a string method that is not here, write it in your own
// header and import it; this library covers the common cases and no more.

#ifndef CMINUX_STRINGFN_HPP
#define CMINUX_STRINGFN_HPP

#include "outfn.hpp"
#include <string>
#include <vector>
#include <cctype>
#include <algorithm>

namespace cmx {

// ---------------------------------------------------------------------------
// trim
//
//   trim(s)       -> s with leading and trailing whitespace removed
//   trim_left(s)  -> s with leading whitespace removed
//   trim_right(s) -> s with trailing whitespace removed
//
// Whitespace is defined by std::isspace: space, tab, newline, carriage
// return, form feed, vertical tab. The returned string is a copy; the
// argument is not modified.
// ---------------------------------------------------------------------------

inline std::string trim_left(const std::string& s) {
    std::size_t i = 0;
    while (i < s.size() && std::isspace(static_cast<unsigned char>(s[i]))) ++i;
    return s.substr(i);
}

inline std::string trim_right(const std::string& s) {
    std::size_t i = s.size();
    while (i > 0 && std::isspace(static_cast<unsigned char>(s[i - 1]))) --i;
    return s.substr(0, i);
}

inline std::string trim(const std::string& s) {
    return trim_left(trim_right(s));
}

// ---------------------------------------------------------------------------
// split
//
//   split(s, delim) -> vector<string>
//
// Splits s on every occurrence of the character delim. Empty segments
// between consecutive delimiters are preserved: split("a,,b", ',') is
// {"a", "", "b"}. This matches Python's str.split with a single-char
// separator and avoids the common surprise of silently dropping empties.
//
// The delimiter is a single char, not a string. If you need to split on
// a multi-character sequence, split on the first character and post-
// process; adding a substring overload is easy but not needed yet.
// ---------------------------------------------------------------------------

inline std::vector<std::string> split(const std::string& s, char delim) {
    std::vector<std::string> out;
    std::string current;
    for (char c : s) {
        if (c == delim) {
            out.push_back(current);
            current.clear();
        } else {
            current.push_back(c);
        }
    }
    out.push_back(current);
    return out;
}

// ---------------------------------------------------------------------------
// join
//
//   join(v, sep) -> string
//
// Concatenates every element of v, inserting sep between consecutive
// elements. join({}, ",") is "". join({"a"}, ",") is "a".
// join({"a", "b", "c"}, ", ") is "a, b, c".
// ---------------------------------------------------------------------------

inline std::string join(const std::vector<std::string>& v, const std::string& sep) {
    std::string out;
    for (std::size_t i = 0; i < v.size(); ++i) {
        if (i > 0) out += sep;
        out += v[i];
    }
    return out;
}

// ---------------------------------------------------------------------------
// starts_with / ends_with
//
//   starts_with(s, prefix) -> bool
//   ends_with(s, suffix)   -> bool
//
// Empty prefix/suffix always matches (returns true), which is the
// standard behavior in std::string::starts_with (C++20) and in most
// string libraries.
// ---------------------------------------------------------------------------

inline bool starts_with(const std::string& s, const std::string& prefix) {
    if (prefix.size() > s.size()) return false;
    return std::equal(prefix.begin(), prefix.end(), s.begin());
}

inline bool ends_with(const std::string& s, const std::string& suffix) {
    if (suffix.size() > s.size()) return false;
    return std::equal(suffix.rbegin(), suffix.rend(), s.rbegin());
}

// ---------------------------------------------------------------------------
// to_upper / to_lower
//
//   to_upper(s) -> s with every ASCII letter uppercased
//   to_lower(s) -> s with every ASCII letter lowercased
//
// The static_cast<unsigned char> in the call to std::toupper / tolower
// is required: passing a negative char (which is possible for non-ASCII
// bytes) is undefined behavior in the C library. This is the standard
// idiom and the reason the cast is written out rather than hidden.
//
// Only ASCII letters are converted. Non-ASCII UTF-8 sequences are left
// untouched, because proper Unicode case folding requires <locale> or a
// third-party library, and cminux does not take that dependency.
// ---------------------------------------------------------------------------

inline std::string to_upper(const std::string& s) {
    std::string out = s;
    for (char& c : out) {
        c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    }
    return out;
}

inline std::string to_lower(const std::string& s) {
    std::string out = s;
    for (char& c : out) {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    return out;
}

// ---------------------------------------------------------------------------
// replace
//
//   replace(s, from, to) -> string
//
// Replaces every non-overlapping occurrence of the substring `from` with
// the substring `to`. If `from` is empty, returns s unchanged (matching
// Python's str.replace and avoiding an infinite loop).
//
// Left-to-right scan. After replacing an occurrence, scanning continues
// after the inserted `to` text, not inside it. So replace("aaa", "aa", "b")
// is "ba", not "ab" and not "bb".
// ---------------------------------------------------------------------------

inline std::string replace(const std::string& s,
                           const std::string& from,
                           const std::string& to) {
    if (from.empty()) return s;
    std::string out;
    std::size_t i = 0;
    while (i < s.size()) {
        if (s.compare(i, from.size(), from) == 0) {
            out += to;
            i += from.size();
        } else {
            out.push_back(s[i]);
            ++i;
        }
    }
    return out;
}

// ---------------------------------------------------------------------------
// repeat
//
//   repeat(s, n) -> string
//
// Returns s concatenated n times. repeat(s, 0) is "". repeat(s, 1) is s.
// Negative n returns "" (same convention as Python's "abc" * -1, and
// the same as range() in vectorfn).
// ---------------------------------------------------------------------------

inline std::string repeat(const std::string& s, int n) {
    if (n <= 0) return {};
    std::string out;
    out.reserve(s.size() * static_cast<std::size_t>(n));
    for (int i = 0; i < n; ++i) out += s;
    return out;
}

} // namespace cmx

#endif // CMINUX_STRINGFN_HPP