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
// split / join
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
// replace / repeat
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

inline std::string repeat(const std::string& s, int n) {
    if (n <= 0) return {};
    std::string out;
    out.reserve(s.size() * static_cast<std::size_t>(n));
    for (int i = 0; i < n; ++i) out += s;
    return out;
}

} // namespace cmx

using cmx::trim;
using cmx::trim_left;
using cmx::trim_right;
using cmx::split;
using cmx::join;
using cmx::starts_with;
using cmx::ends_with;
using cmx::to_upper;
using cmx::to_lower;
using cmx::replace;
using cmx::repeat;

#endif // CMINUX_STRINGFN_HPP