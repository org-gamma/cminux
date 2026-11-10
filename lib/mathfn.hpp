// lib/mathfn.hpp
// cminux math utilities — abs, min, max, clamp, lerp, sign, sqr.
//
// These are wrappers over <algorithm> and a few arithmetic identities.
// They do not define a new numeric type. cminux uses int, long, float,
// double, and the C++ standard library for everything else.

#ifndef CMINUX_MATHFN_HPP
#define CMINUX_MATHFN_HPP

#include <algorithm>
#include <type_traits>

namespace cmx {

// ---------------------------------------------------------------------------
// abs
//
//   abs(x) -> |x|
//
// For unsigned types, abs is the identity. For signed integers, the
// result of abs(INT_MIN) is implementation-defined in C++ (it overflows
// because -INT_MIN cannot be represented); use std::llabs or a wider
// type if you need to handle that case. For floating point, abs is
// std::abs.
//
// This function is unqualified so that cminux source can write abs(x)
// instead of std::abs(x). The using declaration below brings the
// standard library's abs into scope for the fallback case (unsigned
// and floating point), and the overloads here take precedence for
// signed integer types where we want to avoid the std::abs template
// ambiguity that sometimes bites in C++.
// ---------------------------------------------------------------------------

using std::abs;

template<typename T>
inline typename std::enable_if<std::is_integral<T>::value && std::is_signed<T>::value, T>::type
abs_value(T x) {
    return x < 0 ? -x : x;
}

template<typename T>
inline typename std::enable_if<std::is_integral<T>::value && std::is_unsigned<T>::value, T>::type
abs_value(T x) {
    return x;
}

template<typename T>
inline typename std::enable_if<std::is_floating_point<T>::value, T>::type
abs_value(T x) {
    return x < 0 ? -x : x;
}

// ---------------------------------------------------------------------------
// min / max
//
//   min(a, b) -> the smaller of a and b
//   max(a, b) -> the larger of a and b
//
// Both arguments must have the same type. If they do not, the C++
// compiler reports an error at the call site, which is the right
// behavior — silent promotion would hide bugs.
// ---------------------------------------------------------------------------

using std::min;
using std::max;

// ---------------------------------------------------------------------------
// clamp
//
//   clamp(x, lo, hi) -> x if lo <= x <= hi, else lo or hi
//
// If lo > hi, the behavior is undefined (same as std::clamp). Callers
// are expected to pass lo <= hi. This is the standard library's
// behavior and we do not change it; a defensive check would hide bugs
// in caller code that passes swapped bounds.
// ---------------------------------------------------------------------------

template<typename T>
inline T clamp(T x, T lo, T hi) {
    return x < lo ? lo : (x > hi ? hi : x);
}

// ---------------------------------------------------------------------------
// lerp
//
//   lerp(a, b, t) -> a + (b - a) * t
//
// Linear interpolation from a to b by t. When t is 0, the result is a.
// When t is 1, the result is b. When t is outside [0, 1], the result
// is extrapolated; this matches most graphics and animation APIs and
// is the reason there is no clamping here.
//
// The type of t determines the return type. For integer a and b, pass
// a floating-point t or the fractional part will be truncated. This is
// the one place where the templated signature can surprise a caller,
// so it is documented here rather than discovered at runtime.
// ---------------------------------------------------------------------------

template<typename T, typename U>
inline T lerp(T a, T b, U t) {
    return a + (b - a) * t;
}

// ---------------------------------------------------------------------------
// sign
//
//   sign(x) -> -1 if x < 0, 0 if x == 0, +1 if x > 0
//
// The return type is int, not T. This is deliberate: sign is used to
// branch, and an int branch is unambiguous. If you need a sign as the
// same type as the input (for arithmetic), use sign(x) cast to T, or
// use (x > 0) - (x < 0) inline.
// ---------------------------------------------------------------------------

template<typename T>
inline int sign(T x) {
    if (x < T{}) return -1;
    if (x > T{}) return +1;
    return 0;
}

// ---------------------------------------------------------------------------
// sqr
//
//   sqr(x) -> x * x
//
// The name is short because it is used often in hot code. The return
// type is T, so sqr(3) is int 9 and sqr(3.0) is double 9.0.
// ---------------------------------------------------------------------------

template<typename T>
inline T sqr(T x) {
    return x * x;
}

} // namespace cmx

using cmx::clamp;
using cmx::lerp;
using cmx::sign;
using cmx::sqr;

#endif // CMINUX_MATHFN_HPP