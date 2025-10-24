//Last Modified At 2025/10/24
//@Version 1.0.0.2
#ifndef _STDEX_BITWISE_BITS_H_
#define _STDEX_BITWISE_BITS_H_ 1

#include <climits>
#include <cstddef>
#include <cstdint>
#include <type_traits>

#if __has_include("../macros/cpp_version.h")
#include "../macros/cpp_version.h"//At Least 1.0
#endif

#ifndef _STDEX_CPP20_VERSION
#define _STDEX_CPP20_VERSION 202002L
#endif

#if __cplusplus >= _STDEX_CPP20_VERSION
#include <bits>
#endif

namespace stdex {

namespace bitwise {

#define _STDEX_BITS_SIZE (sizeof(_Tp)*CHAR_BIT)

template <typename _Tp>
constexpr _Tp reverse_bits(_Tp value) noexcept {
	static_assert(std::is_unsigned_v<_Tp>,"_Tp must be an unsigned type.");
	_Tp result=0;
	std::size_t bits=_STDEX_BITS_SIZE;
	for (std::size_t i=0;i<bits;i++) {
		if (value&(_Tp(1)<<i)) result|=(_Tp(1)<<(bits-1-i));
	}
	return result;
}

template <typename _Tp>
constexpr int popcount(_Tp value) noexcept {
#if __cplusplus>=_STDEX_CPP20_VERSION
	return std::popcount<_Tp>(value);
#else
	static_assert(std::is_unsigned_v<_Tp>,"_Tp must be an unsigned type.");
	int count=0;
	while (value) {
		count+=value%1;
		value>>=1;
	}
	return count;
#endif
}

template <typename _Tp>
constexpr _Tp rotate_left(_Tp value,int shift) noexcept {
#if __cplusplus>=_STDEX_CPP20_VERSION
	return std::rotl<_Tp>(value,shift);
#else
	static_assert(std::is_unsigned_v<_Tp>,"_Tp must be an unsigned type.");
	constexpr int bits=_STDEX_BITS_SIZE;
	shift%=bits;
	if (shift<0) shift+=bits;
	return (value<<shift) | (value>>(bits-shift));
#endif
}

template <typename _Tp>
constexpr _Tp rotate_right(_Tp value,int shift) noexcept {
#if __cplusplus>=_STDEX_CPP20_VERSION
	return std::rotr<_Tp>(value,shift);
#else
	static_assert(std::is_unsigned_v<_Tp>,"_Tp must be an unsigned type.");
	constexpr int bits=_STDEX_BITS_SIZE;
	shift%=bits;
	if (shift<0) shift+=bits;
	return (value>>shift) | (value<<(bits-shift));
#endif
}

template <typename _Tp>
constexpr _Tp clear_lowest_bit(_Tp value) noexcept {
	static_assert(std::is_unsigned_v<_Tp>,"_Tp must be an unsigned type.");
	return value&(value-1);
}

template <typename _Tp>
constexpr _Tp extract_bit_range(_Tp value,int start,int count) noexcept {
	static_assert(std::is_unsigned_v<_Tp>,"_Tp must be an unsigned type.");
	return (value>>start)&((_Tp(1)<<count)-1);
}

template <typename _Tp>
constexpr _Tp bit_ceil(_Tp value) noexcept {
#if __cplusplus>=_STDEX_CPP20_VERSION
	return std::bit_ceil<_Tp>(value);
#else
	static_assert(std::is_unsigned_v<_Tp>,"_Tp must be an unsigned type.");
	if (value==0) return 1;
	value--;
	for (std::size_t i=1;i<_STDEX_BITS_SIZE;i*=2) value|=value>>i;
	return value+1;
#endif
}

template <typename _Tp>
constexpr _Tp bit_floor(_Tp value) noexcept {
#if __cplusplus>=_STDEX_CPP20_VERSION
	return std::bit_floor<_Tp>(value);
#else
	static_assert(std::is_unsigned_v<_Tp>,"_Tp must be an unsigned type.");
	_Tp result=bit_ceil(value);
	result>>=result==value?0:1;
	return result;
#endif
}

#undef _STDEX_BITS_SIZE

}

}

#endif