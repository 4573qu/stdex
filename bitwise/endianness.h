//Last Modified At 2025/09/13
//@Version 1.0.0.0
#ifndef _STD4573_BITWISE_ENDIANNESS_H_
#define _STD4573_BITWISE_ENDIANNESS_H_ 1

#include <cstddef>
#include <cstdint>

namespace stdex {

namespace bitwise {

namespace endianness {
	constexpr uint32_t test_value=0x45732026;
	constexpr uint8_t first_byte=static_cast<const uint8_t&>(test_value);
}

constexpr bool is_little_endian() noexcept {
	return endianness::first_byte==0x26;
}

constexpr bool is_big_endian() noexcept {
	return endianness::first_byte==0x45;
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