//Last Modified At 2026/05/08
//@Version 1.1.0.0
#ifndef _STDEX_BITWISE_ENDIANNESS_H_
#define _STDEX_BITWISE_ENDIANNESS_H_ 1

#include <cstddef>
#include <cstdint>

#if __has_include("../macros/cpp_version.h")
#include "../macros/cpp_version.h"//At Least 1.0
#endif

#ifndef _STDEX_CPP20_VERSION
#define _STDEX_CPP20_VERSION 202002L
#endif

#if __cplusplus>=_STDEX_CPP20_VERSION
#include <bits>
#endif

namespace stdex {

namespace bitwise {

namespace endianness {
	constexpr uint32_t test_value=0x57DA7C17;//STD AT C17
	constexpr uint8_t first_byte=static_cast<const uint8_t&>(test_value);
}

constexpr bool is_little_endian() noexcept {
#if defined(__BYTE_ORDER__) && __BYTE_ORDER__==__ORDER_LITTLE_ENDIAN__
	return true;
#elif defined(__BYTE_ORDER__) && __BYTE_ORDER__==__ORDER_BIG_ENDIAN__
	return false;
#elif __cplusplus>=_STDEX_CPP20_VERSION
	return std::endian::native==std::endian::little;
#else
	return endianness::first_byte==0x17;
#endif
}

constexpr bool is_big_endian() noexcept {
#if defined(__BYTE_ORDER__) && __BYTE_ORDER__==__ORDER_BIG_ENDIAN__
	return true;
#elif defined(__BYTE_ORDER__) && __BYTE_ORDER__==__ORDER_LITTLE_ENDIAN__
	return false;
#elif __cplusplus>=_STDEX_CPP20_VERSION
	return std::endian::native==std::endian::big;
#else
	return endianness::first_byte==0x57;
#endif
}

template <typename _Tp>
_Tp reverse_bytes(_Tp value) noexcept {
	static_assert(std::is_unsigned_v<_Tp>,"_Tp must be an unsigned type.");
	_Tp result=0;
	for (std::size_t i=0;i<sizeof(_Tp);i++) result|=((value>>(i*8))&0xFF)<<((sizeof(_Tp)-1-i)*8);
	return result;
}

template <typename _Tp>
_Tp to_little_endian(_Tp value) noexcept {
	static_assert(std::is_unsigned_v<_Tp>,"_Tp must be an unsigned type.");
	if constexpr (is_little_endian()) {
		return value;
	} else {
		return reverse_bytes(value);
	}
}

template <typename _Tp>
_Tp to_big_endian(_Tp value) noexcept {
	static_assert(std::is_unsigned_v<_Tp>,"_Tp must be an unsigned type.");
	if constexpr (is_little_endian()) {
		return reverse_bytes(value);
	} else {
		return value;
	}
}

}

}

#endif