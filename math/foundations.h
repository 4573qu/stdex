//Last Modified At 2026/06/04
//@Version 1.1.0.0
#ifndef _STDEX_MATH_FOUNDATIONS_H_
#define _STDEX_MATH_FOUNDATIONS_H_ 1

#include <cmath>
#include <string>

#if __has_include("../macros/cpp_version.h")
#include "../macros/cpp_version.h"//At Least 1.0
#endif

#ifndef _STDEX_CPP20_VERSION
#define _STDEX_CPP20_VERSION 202002L
#endif

#if __cplusplus>=_STDEX_CPP20_VERSION
#include <numbers>
#endif

namespace stdex {

namespace math {

#ifndef _STDEX_MATH_E
#if __cplusplus>=_STDEX_CPP20_VERSION
#define _STDEX_MATH_E std::numbers::e
#else
#ifdef _STDEX_MATH_USE_FAST_CONSTANT
#define _STDEX_MATH_E 2.71828182845904523536
#else
#define _STDEX_MATH_E std::exp(1.0)
#endif
#endif
#endif

#ifndef _STDEX_MATH_PI
#if __cplusplus>=_STDEX_CPP20_VERSION
#define _STDEX_MATH_PI std::numbers::pi
#else
#ifdef _STDEX_MATH_USE_FAST_CONSTANT
#define _STDEX_MATH_PI 3.14159265358979323846
#else
#define _STDEX_MATH_PI std::acos(-1.0)
#endif
#endif
#endif

template <typename _Tp>
struct base_unit_trait {
	static _Tp value() { return (_Tp)1; }
	static _Tp zero() { return (_Tp)0; }
	static _Tp neg_unit() { return (_Tp)-1; }
	static _Tp E() { return (_Tp)_STDEX_MATH_E; }
	static _Tp PI() { return (_Tp)_STDEX_MATH_PI; }
	static _Tp sin(_Tp num) { return (_Tp)std::sin(num); }
	static _Tp cos(_Tp num) { return (_Tp)std::cos(num); }
	static _Tp pow(_Tp base,_Tp exponent) { return (_Tp)std::pow(base,exponent); }
	static _Tp log(_Tp base,_Tp logarithm) { return (_Tp)std::log(logarithm)/(_Tp)std::log(base); }
	static _Tp sqrt(_Tp num) { return (_Tp)std::sqrt(num); }
	static _Tp abs(_Tp num) { return (_Tp)std::abs(num); }
	static std::string to_string(_Tp value) { return std::to_string(value); }
};

template <>
std::string base_unit_trait<char>::to_string(char value) {
	return std::string(1,value);
}

/*
Example of defining specific base_unit_trait:
template <>
struct base_unit_trait<YourSpecialType> {
	static YourSpecialType value() { return YourSpecialType::base_value(); }
};
*/
	
}

}

#endif