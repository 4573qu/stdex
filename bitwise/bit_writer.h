//Last Modified At 2026/03/14
//@Version 2.0.0.0
#ifndef _STDEX_BITWISE_BIT_WRITER_H_
#define _STDEX_BITWISE_BIT_WRITER_H_ 1

#include <climits>
#include <cstddef>
#include <cstdint>
#include <ostream>
#include <stdexcept>
#include <type_traits>
#include <vector>

#include "bit_iterator.h"//At Least 1.0.0.1
#include "bits.h"//At Least 1.1
#include "endianness.h"//At Least 1.0.0.1

namespace stdex {

namespace bitwise {

class bit_writer {
public:
	using iterator=bit_iterator<uint8_t>;

private:
	std::vector<uint8_t> buffer_;
	std::size_t bit_pos_=0;
	std::size_t bit_size_=0;
	bit_order order_=is_little_endian()?BO_LSBYTE:BO_MSBYTE;


	void ensure_bits(std::size_t bits) {
		std::size_t bytes=(bits+CHAR_BIT-1)/CHAR_BIT;
		if (buffer_.size()<bytes) buffer_.resize(bytes,0);
	}

	template <typename _Tp>
	void write_bits_msb(std::size_t nbits,_Tp value) {
		ensure_bits(bit_pos_+nbits);
		_Tp high_bit_mask=static_cast<_Tp>(1)<<(nbits-1);
		for (std::size_t i=0; i < nbits;i++) {
			bool bit=(value&high_bit_mask)!=0;
			value<<=1;
			write_bit_msb_layout(bit_pos_++,bit);
		}
		if (bit_pos_>bit_size_) bit_size_=bit_pos_;
	}
	template <typename _Tp>
	void write_bits_lsb(std::size_t nbits,_Tp value) {
		ensure_bits(bit_pos_+nbits);
		for (std::size_t i=0;i<nbits;i++) {
			bool bit=(value&1u)!=0;
			value>>=1;
			write_bit_lsb_layout(bit_pos_++,bit);
		}
		if (bit_pos_>bit_size_) bit_size_=bit_pos_;
	}
	template <typename _Tp>
	void write_bits_msbit(std::size_t nbits,_Tp value) {
		ensure_bits(bit_pos_+nbits);
		_Tp high_bit_mask=static_cast<_Tp>(1)<<(nbits-1);
		for (std::size_t i=0;i<nbits;i++) {
			bool bit=(value&high_bit_mask)!=0;
			value<<=1;
			write_bit_lsb_layout(bit_pos_++,bit);
		}
		if (bit_pos_>bit_size_) bit_size_=bit_pos_;
	}
	template <typename _Tp>
	void write_bits_lsbit(std::size_t nbits,_Tp value) {
		ensure_bits(bit_pos_+nbits);
		for (std::size_t i=0;i<nbits;i++) {
			bool bit=(value&1u)!=0;
			value>>=1;
			write_bit_msb_layout(bit_pos_++,bit);
		}
		if (bit_pos_>bit_size_) bit_size_=bit_pos_;
	}

public:
	bit_writer() = default;
	explicit bit_writer(bit_order order) : order_(order) { }
	explicit bit_writer(std::size_t capacity,bit_order order=is_little_endian()?BO_LSBYTE:BO_MSBYTE) : order_(order) {
		buffer_.reserve(capacity);
	}

	void seek_bits(std::size_t pos) {
		bit_pos_=pos;
	}
	[[nodiscard]]
	std::size_t tell_bits() const noexcept { return bit_pos_; }
	[[nodiscard]]
	std::size_t size_bits() const noexcept { return bit_size_; }
	[[nodiscard]]
	std::size_t size_bytes() const noexcept { 
		return (bit_size_+CHAR_BIT-1)/CHAR_BIT;
	}

	[[nodiscard]]
	bool is_aligned() const noexcept { return (bit_pos_%CHAR_BIT)==0; }

	void byte_align() {
		std::size_t aligned=(bit_pos_+CHAR_BIT-1)&~std::size_t(CHAR_BIT-1);
		if (aligned>bit_size_) {
			ensure_bits(aligned);
			bit_size_=aligned;
		}
		bit_pos_=aligned;
	}
	void flush_bits() {
		byte_align();
	}
	template <typename _Tp>
	void write_bits(std::size_t nbits,_Tp value) {
		static_assert(std::is_integral_v<_Tp> && std::is_unsigned_v<_Tp>,"_Tp must be unsigned integral.");

		if (nbits==0) return;
#ifndef _STDEX_IGNORE_BITWISE_BIT_WRITER_WARNINGS
		if (nbits>sizeof(_Tp)*CHAR_BIT) throw std::invalid_argument("Invalid bit count");
#else
		if (nbits>sizeof(_Tp)*CHAR_BIT) return;
#endif
		if (order_==BO_MSBYTE) write_bits_msb(nbits,value);
		else if (order_==BO_LSBYTE) write_bits_lsb(nbits,value);
		else if (order_==BO_MSBIT) write_bits_msbit(nbits,value);
		else write_bits_lsbit(nbits,value);
	}

	void write_u8(uint8_t value) { write_bits(8,value); }
	void write_u16(uint16_t value) { write_bits(16,value); }
	void write_u32(uint32_t value) { write_bits(32,value); }
	void write_u64(uint64_t value) { write_bits(64,value); }

	void write_bytes(const void* data,std::size_t byte_count) {
		if (byte_count==0) return;
		if (!is_aligned()) throw std::runtime_error("Cannot write bytes when not byte-aligned");
		std::size_t byte_pos=bit_pos_/CHAR_BIT;
		if (buffer_.size()<byte_pos+byte_count) buffer_.resize(byte_pos+byte_count,0);
		const uint8_t* src=reinterpret_cast<const uint8_t*>(data);
		for (std::size_t i=0;i<byte_count;i++) buffer_[byte_pos+i]=src[i];
		bit_pos_+=byte_count*CHAR_BIT;
		if (bit_pos_>bit_size_) bit_size_=bit_pos_;
	}
	template <typename _Tp>
	void write_bytes(const _Tp& value) {
		static_assert(std::is_standard_layout_v<_Tp>,"_Tp must be standard layout type");

		write_bytes(&value,sizeof(value));
	}
	void write_stream(std::ostream& os) {
		byte_align();
		if (!buffer_.empty()) os.write(reinterpret_cast<const char*>(buffer_.data()),buffer_.size());
	}

	void clear() noexcept {
		buffer_.clear();
		bit_pos_=0;
		bit_size_=0;
	}

	const std::vector<uint8_t>& buffer() const& noexcept {
		return buffer_;
	}
	std::vector<uint8_t> buffer() && noexcept {
		byte_align();
		return std::move(buffer_);
	}
	bit_order& bit_order() noexcept { return order_; }
};

}

}

#endif