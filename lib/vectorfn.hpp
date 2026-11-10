// lib/vectorfn.hpp
// cminux vector utilities — printing, range construction, summation.
//
// This header operates on std::vector<T>. It does not define a Vec<T>
// type. cminux uses the C++ standard library's vector, and this header
// adds the small set of conveniences that are tedious to write by hand.

#ifndef CMINUX_VECTORFN_HPP
#define CMINUX_VECTORFN_HPP

#include "outfn.hpp"
#include <vector>
#include <numeric>
#include <cstddef>

namespace cmx {

// ---------------------------------------------------------------------------
// print / println — std::vector<T>
//
// Prints "[a, b, c]". Elements are printed via the print overloads in
// outfn.hpp, so any vector whose element type is printable is printable.
// Nested vectors work by recursion: vector<vector<int>> prints as
// "[[1, 2], [3, 4]]".
//
// Requires outfn.hpp to be included first. It is, at the top.
// ---------------------------------------------------------------------------

template<typename T>
inline void print(const std::vector<T>& v) {
    std::cout << "[";
    for (std::size_t i = 0; i < v.size(); ++i) {
        if (i > 0) std::cout << ", ";
        print(v[i]);
    }
    std::cout << "]";
    std::cout.flush();
}

template<typename T>
inline void println(const std::vector<T>& v) {
    print(v);
    std::cout << '\n';
    std::cout.flush();
}

// ---------------------------------------------------------------------------
// range
//
//   range(n)          -> [0, 1, 2, ..., n-1]
//   range(start, end) -> [start, start+1, ..., end-1]
//
// Both return std::vector<int>. If end <= start, the result is empty.
// Negative n in range(n) produces an empty vector, not an error; this
// matches how Python's range behaves with a negative argument, and it
// avoids surprising crashes in loops that compute the bound.
// ---------------------------------------------------------------------------

inline std::vector<int> range(int n) {
    if (n <= 0) return {};
    std::vector<int> v;
    v.reserve(static_cast<std::size_t>(n));
    for (int i = 0; i < n; ++i) v.push_back(i);
    return v;
}

inline std::vector<int> range(int start, int end) {
    if (end <= start) return {};
    std::vector<int> v;
    v.reserve(static_cast<std::size_t>(end - start));
    for (int i = start; i < end; ++i) v.push_back(i);
    return v;
}

// ---------------------------------------------------------------------------
// sum
//
//   sum(v) -> the sum of v's elements
//
// The return type is T. For an empty vector, the sum is T{}, which is
// 0 for numeric types. For a vector of a type that does not support
// operator+, this is a compile error at the call site, not at the
// declaration; that is the same behavior std::accumulate has.
// ---------------------------------------------------------------------------

template<typename T>
inline T sum(const std::vector<T>& v) {
    return std::accumulate(v.begin(), v.end(), T{});
}

} // namespace cmx

using cmx::range;
using cmx::sum;

#endif // CMINUX_VECTORFN_HPP