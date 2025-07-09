//Last Modified At 2025/06/01
//@Version 1.0.0.1
#ifndef _STD4573_MATH_MATH_H_
#define _STD4573_MATH_MATH_H_ 1
#include <math.h>
#include <string>

namespace stdex {

namespace math {

template <typename _Tp>
struct base_unit_trait {
	static _Tp value() { return (_Tp)1; }
	static _Tp zero() { return (_Tp)0; }
	static _Tp neg_unit() { return (_Tp)-1; }
	static _Tp E() { return (_Tp)M_E; }
	static _Tp PI() { return (_Tp)M_PI; }
	static _Tp sin(_Tp num) { return (_Tp)std::sin(num); }
	static _Tp cos(_Tp num) { return (_Tp)std::cos(num); }
	static _Tp pow(_Tp base,_Tp exponent) { return (_Tp)std::pow(base,exponent); }
	static _Tp log(_Tp base,_Tp logarithm) { return (_Tp)std::log(logarithm)/(_Tp)std::log(base); }
	static _Tp sqrt(_Tp num) { return (_Tp)std::sqrt(num); }
	static _Tp abs(_Tp num) { return (_Tp)std::abs(num); }
	static string to_string(_Tp value) { return std::to_string(value); }
};

template <>
string base_unit_trait<char>::to_string(char value) {
	return string(1,value);
}

/*define specific base_unit_trait:
template <>
struct base_unit_trait<YourSpecialType> {
	static YourSpecialType value() { return YourSpecialType::base_value(); }
};
*/
	
}

}

#endif