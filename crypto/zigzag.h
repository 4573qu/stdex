//Last Modified At 2025/10/24
//@Version 1.0.0.0
#ifndef _STDEX_CRYPTO_ZIGZAG_H_
#define _STDEX_CRYPTO_ZIGZAG_H_ 1

#include <cstddef>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <type_traits>

namespace stdex {

namespace crypto {

namespace zigzag {

template <typename _Tp>
std::make_unsigned_t<_Tp> encode(_Tp value) {
	static_assert(std::is_integral_v<_Tp> && !std::is_same_v<_Tp,bool>,"_Tp must be an integral type.");
	return (static_cast<std::make_unsigned_t<_Tp>>(value)<<1)^static_cast<std::make_unsigned_t<_Tp>(-(value<0));
}

template <typename _Tp>
_Tp decode(std::make_unsigned_t<_Tp> value) {
	static_assert(std::is_integral_v<_Tp> && !std::is_same_v<_Tp,bool>,"_Tp must be an integral type.");
	return static_cast<_Tp>((value>>1)^(-(value&1)));
}

}

}

}

#endif