//Last Modified At 2026/05/10
//@Version 1.0.0.0
#ifndef _STDEX_INTEGRITY_ADLER_H_
#define _STDEX_INTEGRITY_ADLER_H_ 1

#include <cstddef>
#include <cstdint>
#include <iterator>
#include <type_traits>

#if __has_include("../macros/cpp_version.h")
#include "../macros/cpp_version.h"
#endif

#ifndef _STDEX_CPP20_VERSION
#define _STDEX_CPP20_VERSION 202002L
#endif

#if __cplusplus>=_STDEX_CPP20_VERSION
#include <span>
#endif

namespace stdex {

namespace integrity {

namespace adler {

constexpr bool is_prime(uint64_t n) noexcept {
	if (n<2) return false;
	if (n==2) return true;
	if ((n&1)==0) return false;
	for (uint64_t i=3;i*i<=n;i+=2) {
		if (n%i==0) return false;
	}
	return true;
}

constexpr uint64_t largest_prime_below(uint64_t limit) noexcept {
	if (limit<=2) return 0;
	uint64_t candidate=limit-1;
	while (candidate>=2) {
		if (is_prime(candidate)) return candidate;
		candidate--;
	}
	return 0;
}

template <std::size_t _Bits,typename=void>
struct type_for_bits {
	using type=uint64_t;
};

template <std::size_t _Bits>
struct type_for_bits<_Bits,std::enable_if_t<(_Bits<=8)>> {
	using type=uint8_t;
};

template <std::size_t _Bits>
struct type_for_bits<_Bits,std::enable_if_t<(_Bits>8 && _Bits<=16)>> {
	using type=uint16_t;
};
template <std::size_t _Bits>
struct type_for_bits<_Bits,std::enable_if_t<(_Bits>16 && _Bits<=32)>> {
	using type=uint32_t;
};
template <std::size_t _Bits>
struct type_for_bits<_Bits,std::enable_if_t<(_Bits>32 && _Bits<=64)>> {
	using type=uint64_t;
};

template <std::size_t _Bits,uint64_t _Modulus=largest_prime_below(uint64_t(1)<<(_Bits/2))>
class adler {
	static_assert(_Bits>=8 && _Bits<=64,"_Bits must be in [8,64].");
	static_assert(_Bits%2==0,"_Bits must be even.");
	static_assert(_Modulus>=2,"_Modulus must be at least 2.");

public:
	using value_type=typename type_for_bits<_Bits>::type;

	static constexpr std::size_t bits=_Bits;
	static constexpr uint64_t modulus=_Modulus;
	static constexpr std::size_t half_bits=_Bits/2;

private:
	uint64_t a_=1;
	uint64_t b_=0;

	void process_byte(uint8_t byte) noexcept {
		a_=(a_+byte)%_Modulus;
		b_=(b_+a_)%_Modulus;
	}

public:
	adler() noexcept=default;

	void update(const void* data,std::size_t length) noexcept {
		const uint8_t* bytes=static_cast<const uint8_t*>(data);
		for (std::size_t i=0;i<length;i++) process_byte(bytes[i]);
	}

	template <typename _It>
	void update(_It begin,_It end) noexcept {
		for (auto it=begin;it!=end;it++) process_byte(static_cast<uint8_t>(*it));
	}

	template <typename _Container>
	auto update(const _Container& container) noexcept->decltype(std::data(container),std::size(container),void()) {
		update(std::data(container),std::size(container));
	}

#if __cplusplus>=_STDEX_CPP20_VERSION
	void update(std::span<const std::byte> data) noexcept {
		update(data.data(),data.size());
	}
	void update(std::span<const uint8_t> data) noexcept {
		update(data.data(),data.size());
	}
#endif

	[[nodiscard]]
	value_type value_a() const noexcept {
		return static_cast<value_type>(a_);
	}

	[[nodiscard]]
	value_type value_b() const noexcept {
		return static_cast<value_type>(b_);
	}

	[[nodiscard]]
	value_type value() const noexcept {
		return static_cast<value_type>((b_<<half_bits)|a_);
	}

	[[nodiscard]]
	value_type checksum() const noexcept {
		return value();
	}

	void reset() noexcept {
		a_=1;
		b_=0;
	}

	static value_type calculate(const void* data,std::size_t length) noexcept {
		adler calc;
		calc.update(data,length);
		return calc.checksum();
	}

	template <typename _Container>
	static auto calculate(const _Container& container) noexcept->decltype(std::data(container),std::size(container),value_type()) {
		return calculate(std::data(container),std::size(container));
	}

	template <typename _Iterator>
	static value_type calculate(_Iterator begin,_Iterator end) noexcept {
		adler calc;
		calc.update(begin,end);
		return calc.checksum();
	}

#if __cplusplus>=_STDEX_CPP20_VERSION
	static value_type calculate(std::span<const std::byte> data) noexcept {
		return calculate(data.data(),data.size());
	}
	static value_type calculate(std::span<const uint8_t> data) noexcept {
		return calculate(data.data(),data.size());
	}
#endif
};

}

using adler16=adler::adler<16>;
using adler32=adler::adler<32>;
using adler64=adler::adler<64>;

}

}

#endif