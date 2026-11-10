// lib/mathfn.hpp
// cminux math utilities — abs, min, max, clamp, lerp, sign, sqr.

#ifndef CMINUX_MATHFN_HPP
#define CMINUX_MATHFN_HPP

#include <algorithm>
#include <cmath>
#include <type_traits>

namespace cmx {

// ---------------------------------------------------------------------------
// abs / min / max
//
// All three are brought in from the standard library with `using`. This is
// better than writing wrappers: std::abs, std::min, and std::max already
// handle every numeric type cminux cares about, and they participate in
// overload resolution the way C++ programmers expect.
//
// A separate abs_value template with SFINAE dispatch was attempted earlier
// and removed. It was more code than the problem warranted, and it exposed
// a function (abs_value) that nobody calls instead of the function (abs)
// that everybody calls.
// ---------------------------------------------------------------------------

using std::abs;
using std::min;
using std::max;

// ---------------------------------------------------------------------------
// clamp
// ---------------------------------------------------------------------------

template<typename T>
inline T clamp(T x, T lo, T hi) {
    return x < lo ? lo : (x > hi ? hi : x);
}

// ---------------------------------------------------------------------------
// lerp
// ---------------------------------------------------------------------------

template<typename T, typename U>
inline T lerp(T a, T b, U t) {
    return a + (b - a) * t;
}

// ---------------------------------------------------------------------------
// sign / sqr
// ---------------------------------------------------------------------------

template<typename T>
inline int sign(T x) {
    if (x < T{}) return -1;
    if (x > T{}) return +1;
    return 0;
}

template<typename T>
inline T sqr(T x) {
    return x * x;
}

} // namespace cmx

using cmx::abs;
using cmx::min;
using cmx::max;
using cmx::clamp;
using cmx::lerp;
using cmx::sign;
using cmx::sqr;

#endif // CMINUX_MATHFN_HPP