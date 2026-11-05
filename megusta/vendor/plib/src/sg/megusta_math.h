#pragma once
// MEGUSTA MOD MG-PLIB-003: preserve original C double math signatures.
#include <cmath>
inline double mg_plib_sqrt(double x) { return std::sqrt(x); }
inline double mg_plib_sin(double x) { return std::sin(x); }
inline double mg_plib_cos(double x) { return std::cos(x); }
inline double mg_plib_tan(double x) { return std::tan(x); }
inline double mg_plib_fabs(double x) { return std::fabs(x); }
inline double mg_plib_acos(double x) { return std::acos(x); }
inline double mg_plib_asin(double x) { return std::asin(x); }
inline double mg_plib_atan(double x) { return std::atan(x); }
inline double mg_plib_atan2(double x,double y) { return std::atan2(x,y); }
inline double mg_plib_pow(double x,double y) { return std::pow(x,y); }

// MG-PLIB-016: original engine adapters 004d9d10/004d9d20 execute FSIN/FCOS
// on a float input and return ST(0), without a double spill. The non-x86
// fallback remains an explicit cross-target numerical validation obligation.
inline long double mg_plib_engine_sin(float x) {
#if defined(__i386__)
  long double result; __asm__("fsin" : "=t"(result) : "0"(x)); return result;
#else
  return std::sin(static_cast<long double>(x));
#endif
}
inline long double mg_plib_engine_cos(float x) {
#if defined(__i386__)
  long double result; __asm__("fcos" : "=t"(result) : "0"(x)); return result;
#else
  return std::cos(static_cast<long double>(x));
#endif
}
// MG-PLIB-027: 004d9d30 executes FPTAN and discards its extra 1.0 result.
inline long double mg_plib_engine_tan(float x) {
#if defined(__i386__)
  long double result; __asm__("fptan; fstp %%st(0)" : "=t"(result) : "0"(x)); return result;
#else
  return std::tan(static_cast<long double>(x));
#endif
}
// MG-PLIB-033: 004d9d50 consumes a float and retains the FSQRT result.
inline long double mg_plib_engine_sqrt(float x) {
#if defined(__i386__)
  long double result; __asm__("fsqrt" : "=t"(result) : "0"(x)); return result;
#else
  return std::sqrt(static_cast<long double>(x));
#endif
}
