//Last Modified At 2025/10/30
//@Version 1.0.0.0
#ifndef _STDEX_INTEGRITY_CRC_H_
#define _STDEX_INTEGRITY_CRC_H_ 1

#include <array>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <type_traits>

#if __has_include("../macros/cpp_version.h")
#include "../macros/cpp_version.h"//At Least 1.0
#endif

#ifndef _STDEX_CPP20_VERSION
#define _STDEX_CPP20_VERSION 202002L
#endif

#if __cplusplus>=_STDEX_CPP20_VERSION
#include <numeric>
#include <span>
#endif

namespace stdex {

namespace integrity {

template <std::size_t _Bits,typename _PolyType,_PolyType _Polynomial,_PolyType _InitialValue=0,_PolyType _FinalXorValue=0,bool _ReflectInput=false,bool _ReflectOutput=false>
class crc {
public:
	static_assert(_Bits>0 && _Bits<=64,"Bits must be between 1 and 64.");
	static_assert(std::is_unsigned_v<_PolyType>,"Polynomial type must be unsigned.");

	using value_type=_PolyType;

private:
	value_type value_;

	static constexpr value_type reflect(value_type x) noexcept {
		value_type reflection=0;
		for (int i=0;i<_Bits;i++) {
			if (x&(static_cast<value_type>(1)<<i)) reflection|=static_cast<value_type>(1)<<((_Bits-1)-i);
		}
		return reflection;
	}
	void process_byte(uint8_t byte) noexcept {
		const auto idx=[byte,this]{
			if constexpr (reflect_input_) {
				return static_cast<uint8_t>(byte);
			} else {
				return static_cast<uint8_t>(byte)^(value_>>(_Bits-8));
			}
		}();
		if constexpr (reflect_input_) {
			value_=(value_>>8)^table[idx];
		} else {
			value_=(value_<<8)^table[idx];
		}
	}

public:
	static constexpr std::size_t bits_=_Bits;
	static constexpr value_type polynomial_=_Polynomial;
	static constexpr value_type initial_value_=_InitialValue;
	static constexpr value_type final_xor_value_=_FinalXorValue;
	static constexpr bool reflect_input_=_ReflectInput;
	static constexpr bool reflect_output_=_ReflectOutput;

	static constexpr auto generate_table() noexcept {
		std::array<value_type,256> table{};
		for (int i=0;i<256;i++) {
			value_type crc=static_cast<value_type>(i);
			if constexpr (reflect_input) {
				for (int j=0;j<8;j++) {
					if (crc&1) crc=(crc>>1)^polynomial;
					else crc>>=1;
				}
			} else {
				crc<<=(_Bits-8);
				for (int j=0;j<8;j++) {
					if (crc&(static_cast<value_type>(1)<<(_Bits-1))) crc=(crc<<1)^polynomial;
					else crc<<=1;
				}
			}
			table[i]=crc;
		}
		return table;
	}
	static constexpr auto table_=generate_table();

	crc() noexcept : value_(initial_value) {}
#if __cplusplus>=_STDEX_CPP20_VERSION
	void update(std::span<const std::byte> data) noexcept {
		for (auto it:data) {
			const auto idx=[it]{
				if constexpr (reflect_input_) {
					return static_cast<uint8_t>(it);
				} else {
					return static_cast<uint8_t>(it)^(value_>>(_Bits-8));
				}
			}();
			if constexpr (reflect_input_) {
				value_=(value_>>8)^table[idx];
			} else {
				value_=(value_<<8)^table[idx];
			}
		}
	}
#endif
	void update(const void* data,std::size_t length) noexcept {
		const uint8_t* bytes=static_cast<const uint8_t*>(data);
		for (std::size_t i=0;i<length;i++) process_byte(bytes[i]);
	}
	template <typename _Iterator>
	void update(_Iterator begin,_Iterator end) noexcept {
		for (auto it=begin;it!=end;it++) process_byte(static_cast<uint8_t>(*it));
	}
	template <typename _Container>
	auto update(const _Container& container) noexcept->decltype(std::data(container),std::size(container),void()) {
		update(std::data(container),std::size(container));
	}

	[[nodiscard]]
	value_type value() const noexcept {
		return value_;
	}
	[[nodiscard]]
	value_type checksum() const noexcept {
		auto result=value_;
		if constexpr (reflect_input_!=reflect_output_) {
			result=reflect(result);
		}
		return result^final_xor_value_;
	}
#if __cplusplus>=_STDEX_CPP20_VERSION
	static value_type calculate(std::span<const std::byte> data) noexcept {
		crc calculator;
		calculator.update(data);
		return calculator.checksum();
	}
#endif
	static value_type calculate(const void* data,std::size_t length) noexcept {
		crc calculator;
		calculator.update(data,length);
		return calculator.checksum();
	}
	template <typename _Container>
        static auto calculate(const _Container& container) noexcept->decltype(std::data(container),std::size(container),value_type()) {
		return calculate(std::data(container),std::size(container));
	}
	void reset() noexcept {
		value_=initial_value;
	}
};

using crc32=crc<32,uint32_t,0xEDB88320,0xFFFFFFFF,0xFFFFFFFF,true,true>; // IEEE 802.3
using crc32c=crc<32,uint32_t,0x82F63B78,0xFFFFFFFF,0xFFFFFFFF,true,true>; // iSCSI, SCTP
using crc16_ccitt=crc<16,uint16_t, 0x1021,0xFFFF,0x0000, false,false>; // X.25, V.41

}

}

#endif