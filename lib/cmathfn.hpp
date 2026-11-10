// lib/cmathfn.hpp
// cminux math library — trigonometric, exponential, logarithmic, and
// rounding functions, plus pi and e.
//
// Every function is a wrapper over <cmath>. None of them does anything
// the standard library does not already do. The value of the header is
// that cminux source can write sin(x) instead of std::sin(x), and that
// pi and e are available by name.

#ifndef CMINUX_CMATHFN_HPP
#define CMINUX_CMATHFN_HPP

#include <cmath>

namespace cmx {

// ---------------------------------------------------------------------------
// Constants
//
// pi and e are constexpr double. They are accurate to 17 significant
// digits, which is the full precision of an IEEE-754 double. If you
// need float precision, cast: (float)pi.
// ---------------------------------------------------------------------------

constexpr double pi = 3.14159265358979323846;
constexpr double e  = 2.71828182845904523536;

// ---------------------------------------------------------------------------
// Trigonometric functions
//
// All take and return double. Angles are in radians, not degrees. To
// convert: radians = degrees * pi / 180.
//
//   sin, cos, tan          — primary
//   asin, acos, atan       — inverse (atan of a single argument; for
//                            atan2 use std::atan2 directly)
// ---------------------------------------------------------------------------

inline double sin(double x)  { return std::sin(x); }
inline double cos(double x)  { return std::cos(x); }
inline double tan(double x)  { return std::tan(x); }
inline double asin(double x) { return std::asin(x); }
inline double acos(double x) { return std::acos(x); }
inline double atan(double x) { return std::atan(x); }

// ---------------------------------------------------------------------------
// Exponential and logarithmic functions
//
//   sqrt, pow, exp
//   log    — natural logarithm (base e)
//   log10  — base-10 logarithm
//
// pow takes two doubles. For pow(x, 2), prefer sqr(x) from mathfn.hpp;
// it is clearer and does not call into the math library.
// ---------------------------------------------------------------------------

inline double sqrt(double x)  { return std::sqrt(x); }
inline double pow(double x, double y) { return std::pow(x, y); }
inline double exp(double x)   { return std::exp(x); }
inline double log(double x)   { return std::log(x); }
inline double log10(double x) { return std::log10(x); }

// ---------------------------------------------------------------------------
// Rounding functions
//
//   floor, ceil, round, fmod
//
// floor(x)  — largest integer <= x, as double
// ceil(x)   — smallest integer >= x, as double
// round(x)  — nearest integer, halfway cases away from zero, as double
// fmod(x,y) — floating-point remainder of x/y
//
// Note: these return double, not int. If you want an int, cast:
// (int)floor(x). This matches <cmath> and is the right default; casting
// inside these wrappers would hide overflow.
// ---------------------------------------------------------------------------

inline double floor(double x)  { return std::floor(x); }
inline double ceil(double x)   { return std::ceil(x); }
inline double round(double x)  { return std::round(x); }
inline double fmod(double x, double y) { return std::fmod(x, y); }

} // namespace cmx

#endif // CMINUX_CMATHFN_HPP