//Last Modified At 2025/10/23
//@Version 1.0.0.0
#ifndef _STDEX_CRYPTO_VARINT_H_
#define _STDEX_CRYPTO_VARINT_H_ 1

#include <cstddef>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

namespace stdex {

namespace crypto {

namespace varint {

template <typename _Tp>
std::vector<uint8_t> encode(_Tp value) {
	static_assert(std::is_integral_v<_Tp> && !std::is_same_v<_Tp,bool>,"_Tp must be an integral type.");
	std::vector<uint8_t> result;
	std::make_unsigned_t<_Tp> uvalue=static_cast<std::make_unsigned_t<_Tp>>(value);
	while (uvalue>=0x80) {
		result.push_back(static_cast<uint8_t>(uvalue|0x80));
		uvalue>>=7;
	}
	result.push_back(static_cast<uint8_t>(uvalue);
	return result;
}

template <typename _Tp>
std::pair<_Tp,size_t> decode_stream(const uint8_t* data,size_t size) {
	static_assert(std::is_integral_v<_Tp> && !std::is_same_v<_Tp,bool>,"_Tp must be an integral type.");
	_Tp result=0;
	int shift=0;
	std::size_t bytes=0;
	for (std::size_t i=0;i<size;i++) {
		if (shift>=std::numeric_limits<T>::digits) throw std::overflow_error("Varint too large for target type");
		uint8_t byte=data[i];
		result|=static_cast<_Tp>(byte&0x7F)<<shift;
		shift+=7;
		bytes++;
		if (!(byte&0x80)) break;
		if (i==size-1 && (byte&0x80)) throw std::invalid_argument("Incomplete varint sequence");
	}
	return {result,bytes};
}

template <typename _Tp>
_Tp decode(const uint8_t* data,size_t size) {
	return decode_stream(data,size).first;
}

template <typename _Tp>
_Tp decode(std::vector<uint8_t>& data) {
	return decode<_Tp>(data.data(),data.size());
}

}

}

}

#endif