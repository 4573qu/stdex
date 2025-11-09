//Last Modified At 2025/11/08
//@Version 1.1.0.0
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
	bit_order order_=is_little_endian()?BO_LSBYTE:BO_MSBYTE;
	uint8_t current_byte_=0;
	std::size_t curr_byte_pos_=0;

	template <typename _Tp>
	void write_bits_msb(std::size_t nbits,_Tp value) {
		const _Tp high_bit_mask=static_cast<_Tp>(1)<<(nbits-1);
		for (std::size_t i=0;i<nbits;i++) {
			bool bit=(value&high_bit_mask)!=0;
			value<<=1;
			current_byte_=(current_byte_<<1)|static_cast<uint8_t>(bit);
			curr_byte_pos_++;
			bit_pos_++;
			if (curr_byte_pos_==CHAR_BIT) {
				buffer_.push_back(current_byte_);
				current_byte_=0;
				curr_byte_pos_=0;
			}
		}
	}
	template <typename _Tp>
	void write_bits_lsb(std::size_t nbits,_Tp value) {
		for (std::size_t i=0;i<nbits;i++) {
			bool bit=(value&1)!=0;
			value>>=1;
			current_byte_|=static_cast<uint8_t>(bit)<<curr_byte_pos_;
			curr_byte_pos_++;
			bit_pos_++;
			if (curr_byte_pos_==CHAR_BIT) {
				buffer_.push_back(current_byte_);
				current_byte_=0;
				curr_byte_pos_=0;
			}
		}
	}
	template <typename _Tp>
	void write_bits_msbit(std::size_t nbits,_Tp value) {
		const _Tp high_bit_mask=static_cast<_Tp>(1)<<(nbits-1);
		for (std::size_t i=0;i<nbits;i++) {
			bool bit=(value&high_bit_mask)!=0;
			value<<=1;
			current_byte_=(current_byte_<<1)|static_cast<uint8_t>(bit);
			curr_byte_pos_++;
			bit_pos_++;
			if (curr_byte_pos_==CHAR_BIT) {
				buffer_.push_back(reverse_bits(current_byte_));
				current_byte_=0;
				curr_byte_pos_=0;
			}
		}
	}
	template <typename _Tp>
	void write_bits_lsbit(std::size_t nbits,_Tp value) {
		for (std::size_t i=0;i<nbits;i++) {
			bool bit=(value&1)!=0;
			value>>=1;
			current_byte_|=static_cast<uint8_t>(bit)<<curr_byte_pos_;
			curr_byte_pos_++;
			bit_pos_++;
			if (curr_byte_pos_==CHAR_BIT) {
				buffer_.push_back(reverse_bits(current_byte_));
				current_byte_=0;
				curr_byte_pos_=0;
			}
		}
	}

public:
	bit_writer() = default;
	explicit bit_writer(bit_order order) : order_(order) { }
	explicit bit_writer(std::size_t capacity,bit_order order=is_little_endian()?BO_LSBYTE:BO_MSBYTE) : order_(order) {
		buffer_.reserve(capacity);
	}

	[[nodiscard]]
	std::size_t tell_bits() const noexcept { return bit_pos_; }
	[[nodiscard]]
	std::size_t size_bits() const noexcept { return buffer_.size()*CHAR_BIT+curr_byte_pos_; }
	[[nodiscard]]
	std::size_t size_bytes() const noexcept { 
		return buffer_.size()+(curr_byte_pos_>0?1:0);
	}

	[[nodiscard]]
	bool is_aligned() const noexcept { return curr_byte_pos_==0; }

	void byte_align() {
		if (curr_byte_pos_>0) {
			int n=curr_byte_pos_;
			for (int i=0;i<CHAR_BIT-n;i++) write_bits<uint8_t>(1,0);
		}
	}
	void flush_bits() {
		byte_align();
	}
	template <typename _Tp>
	void write_bits(std::size_t nbits,_Tp value) {
		static_assert(std::is_integral_v<_Tp> && std::is_unsigned_v<_Tp>,"_Tp must be unsigned integral.");

		if (nbits==0 || nbits>sizeof(_Tp)*CHAR_BIT) return;//throw std::invalid_argument("Invalid bit count");
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
		const uint8_t* byte_data=reinterpret_cast<const uint8_t*>(data);
		buffer_.insert(buffer_.end(),byte_data,byte_data+byte_count);
		bit_pos_+=byte_count*CHAR_BIT;
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
		current_byte_=0;
		curr_byte_pos_=0;
		bit_pos_=0;
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