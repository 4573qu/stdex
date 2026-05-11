//Last Modified At 2026/05/12
//@Version 2.0.0.0
#ifndef _STDEX_BITWISE_BIT_READER_H_
#define _STDEX_BITWISE_BIT_READER_H_ 1

#include <climits>
#include <cstddef>
#include <cstdint>
#include <istream>
#include <stdexcept>
#include <type_traits>
#include <vector>

#include "bit_iterator.h"//At Least 1.0.0.1
#include "bits.h"//At Least 1.1
#include "endianness.h"//At Least 1.0.0.1

namespace stdex {

namespace bitwise {

class bit_reader {
public:
	using iterator=bit_iterator<const uint8_t>;

private:
	std::vector<uint8_t> owned_;
	const uint8_t* data_=nullptr;
	std::size_t bit_size_=0;
	std::size_t bit_pos_=0;
	bit_order order_=is_little_endian()?BO_LSBYTE:BO_MSBYTE;
	uint64_t bit_buf_=0;
	int bits_in_buf_=0;
	std::size_t byte_pos_=0;

	void fill_buffer() {
		while (bits_in_buf_<=56 && byte_pos_<(bit_size_+CHAR_BIT-1)/CHAR_BIT) {
			uint8_t next_byte=data_[byte_pos_++];
			if (order_==BO_MSBIT) next_byte=reverse_bits(next_byte);
			else if (order_==BO_LSBIT) {
				std::size_t valid_bits=CHAR_BIT;
				if (byte_pos_==(bit_size_+CHAR_BIT-1)/CHAR_BIT) {
					valid_bits=bit_size_ %CHAR_BIT;
					if (valid_bits==0) valid_bits=CHAR_BIT;
				}
				next_byte=reverse_bits(next_byte,valid_bits);
			}
			if (order_==BO_LSBYTE || order_==BO_LSBIT) bit_buf_|=static_cast<uint64_t>(next_byte)<<bits_in_buf_;
			else bit_buf_=(bit_buf_<<CHAR_BIT)|next_byte;
			bits_in_buf_+=CHAR_BIT;
		}
	}

public:
	bit_reader()=default;
	bit_reader(const void* ptr,std::size_t byte_size,bit_order order=is_little_endian()?BO_LSBYTE:BO_MSBYTE) : data_(reinterpret_cast<const uint8_t*>(ptr)) , bit_size_(byte_size*CHAR_BIT) , order_(order) { }
	explicit bit_reader(std::vector<uint8_t>&& buf,bit_order order=is_little_endian()?BO_LSBYTE:BO_MSBYTE) : owned_(std::move(buf)) , order_(order) {
		data_=owned_.data();
		bit_size_=owned_.size()*CHAR_BIT;
	}
	explicit bit_reader(const std::vector<uint8_t>& buf,bit_order order=is_little_endian()?BO_LSBYTE:BO_MSBYTE,bool copy=false) : order_(order) {
		if (copy) {
			owned_=buf;
			data_=owned_.data();
		} else data_=buf.data();
		bit_size_=buf.size()*CHAR_BIT;
	}
	template <typename _It,typename=std::enable_if_t<!std::is_integral_v<_It>>>
	bit_reader(_It first,_It last,bit_order order=is_little_endian()?BO_LSBYTE:BO_MSBYTE) : owned_(first,last) , order_(order) {
		data_=owned_.data();
		bit_size_=owned_.size()*CHAR_BIT;
	}

	[[nodiscard]]
	bool eof() const noexcept { return bit_pos_>=bit_size_; }
	[[nodiscard]]
	std::size_t tell_bits() const noexcept { return bit_pos_; }
	[[nodiscard]]
	std::size_t size_bits() const noexcept { return bit_size_; }

	void seek_bits(std::size_t pos) {
		if (pos>bit_size_) throw std::out_of_range("Seek bits out of range");
		bit_pos_=pos;
		byte_pos_=bit_pos_/CHAR_BIT;
		bit_buf_=0;
		bits_in_buf_=0;
		std::size_t remainder=bit_pos_%CHAR_BIT;
		if (remainder>0) {
			fill_buffer();
			if (order_==BO_LSBYTE || order_==BO_LSBIT) bit_buf_>>=remainder;
			bits_in_buf_-=static_cast<int>(remainder);
		}
	}

	void byte_align() noexcept {
		bit_pos_=(bit_pos_+CHAR_BIT-1u) & ~std::size_t(CHAR_BIT-1);
		if (bit_pos_>bit_size_) bit_pos_=bit_size_;
		byte_pos_=bit_pos_/CHAR_BIT;
		bit_buf_=0;
		bits_in_buf_=0;
	}

	template <typename _Tp>
	_Tp read_bits(std::size_t nbits) {
		static_assert(std::is_integral_v<_Tp> && std::is_unsigned_v<_Tp>,"_Tp must be unsigned integral.");

		if (nbits==0 || nbits>sizeof(_Tp)*CHAR_BIT) throw std::invalid_argument("invalid bit count");
		if (bits_in_buf_<static_cast<int>(nbits)) {
			fill_buffer();
			if (bits_in_buf_<static_cast<int>(nbits)) throw std::runtime_error("unexpected EOF");
		}
		_Tp result = 0;
		if (order_==BO_LSBYTE || order_==BO_LSBIT) {
			result=static_cast<_Tp>(bit_buf_&((1ULL<<nbits)-1));
			bit_buf_>>=nbits;
		} else result=static_cast<_Tp>((bit_buf_>>(bits_in_buf_-nbits))&((1ULL<<nbits)-1));
		bits_in_buf_-=static_cast<int>(nbits);
		bit_pos_+=nbits;
		return result;
	}

	uint8_t  read_u8()  { return read_bits<uint8_t>(8);  }
	uint16_t read_u16() { return read_bits<uint16_t>(16); }
	uint32_t read_u32() { return read_bits<uint32_t>(32); }
	uint64_t read_u64() { return read_bits<uint64_t>(64); }

	template <typename _Tp=uint32_t>
	_Tp peek_bits(std::size_t nbits) {
		if (bits_in_buf_<static_cast<int>(nbits)) {
			fill_buffer();
			if (bits_in_buf_<static_cast<int>(nbits)) throw std::runtime_error("unexpected EOF");
		}
		if (order_==BO_LSBYTE || order_==BO_LSBIT) return static_cast<_Tp>(bit_buf_&((1ULL<<nbits)-1));
		else return static_cast<_Tp>((bit_buf_>>(bits_in_buf_-nbits))&((1ULL<<nbits)-1));
	}

	void drop_bits(std::size_t nbits) {
		if (bits_in_buf_<static_cast<int>(nbits)) throw std::runtime_error("drop exceeds buffer");
		if (order_==BO_LSBYTE || order_==BO_LSBIT) bit_buf_>>=nbits;
		bits_in_buf_-=static_cast<int>(nbits);
		bit_pos_+=nbits;
	}

	std::size_t bit_pos() { return bit_pos_; }
	bit_order& bit_order() noexcept { return order_; }
};

}

}

#endif