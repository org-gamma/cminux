// lib/cmathfn.hpp
// cminux math library — trigonometric, exponential, logarithmic, and
// rounding functions, plus pi and e.

#ifndef CMINUX_CMATHFN_HPP
#define CMINUX_CMATHFN_HPP

#include <cmath>

namespace cmx {

constexpr double pi = 3.14159265358979323846;
constexpr double e  = 2.71828182845904523536;

inline double sin(double x)  { return std::sin(x); }
inline double cos(double x)  { return std::cos(x); }
inline double tan(double x)  { return std::tan(x); }
inline double asin(double x) { return std::asin(x); }
inline double acos(double x) { return std::acos(x); }
inline double atan(double x) { return std::atan(x); }

inline double sqrt(double x)  { return std::sqrt(x); }
inline double pow(double x, double y) { return std::pow(x, y); }
inline double exp(double x)   { return std::exp(x); }
inline double log(double x)   { return std::log(x); }
inline double log10(double x) { return std::log10(x); }

inline double floor(double x)  { return std::floor(x); }
inline double ceil(double x)   { return std::ceil(x); }
inline double round(double x)  { return std::round(x); }
inline double fmod(double x, double y) { return std::fmod(x, y); }

} // namespace cmx

using cmx::pi;
using cmx::e;
using cmx::sin;
using cmx::cos;
using cmx::tan;
using cmx::asin;
using cmx::acos;
using cmx::atan;
using cmx::sqrt;
using cmx::pow;
using cmx::exp;
using cmx::log;
using cmx::log10;
using cmx::floor;
using cmx::ceil;
using cmx::round;
using cmx::fmod;

#endif // CMINUX_CMATHFN_HPP