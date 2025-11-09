//Last Modified At 2025/11/05
//@Version 1.1.0.0
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
	const uint8_t* data_=nullptr;
	std::size_t bit_size_=0;
	std::size_t bit_pos_=0;
	bit_order order_=is_little_endian()?BO_LSBYTE:BO_MSBYTE;

public:
	bit_reader()=default;
	bit_reader(const void* ptr,std::size_t byte_size,bit_order order=is_little_endian()?BO_LSBYTE:BO_MSBYTE) : data_(reinterpret_cast<const uint8_t*>(ptr)) , bit_size_(byte_size*CHAR_BIT) , order_(order) { }
	explicit bit_reader(const std::vector<uint8_t>& buf,bit_order order=is_little_endian()?BO_LSBYTE:BO_MSBYTE) : data_(buf.data()) , bit_size_(buf.size()*CHAR_BIT) , order_(order) { }
	template <typename _It,typename=std::enable_if_t<!std::is_integral_v<_It>>>
	bit_reader(_It first,_It last,bit_order order=is_little_endian()?BO_LSBYTE:BO_MSBYTE) : order_(order) {
		std::vector<uint8_t> temp(first,last);
		data_=temp.data();
		bit_size_=temp.size()*CHAR_BIT;
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
	}

	void byte_align() noexcept {
		bit_pos_=(bit_pos_+CHAR_BIT-1u) & ~std::size_t(CHAR_BIT-1);
	}

	template <typename _Tp>
	_Tp read_bits(std::size_t nbits) {
		static_assert(std::is_integral_v<_Tp> && std::is_unsigned_v<_Tp>,"_Tp must be unsigned integral.");

		if (nbits==0 || nbits>sizeof(_Tp)*CHAR_BIT) throw std::invalid_argument("Invalid bit count");
		if (bit_pos_+nbits>bit_size_) throw std::runtime_error("Unexpected EOF in bit_reader");
		_Tp result=0;
		if (order_ == BO_LSBIT || order_ == BO_MSBIT) {
			std::size_t start_bit=bit_pos_;
			std::size_t start_byte=start_bit/CHAR_BIT;
			std::size_t start_off =start_bit%CHAR_BIT;
			std::size_t needed_bit_end=start_off+nbits;
			std::size_t total_bytes=(needed_bit_end+CHAR_BIT-1)/CHAR_BIT;
			uint8_t temp[sizeof(_Tp)+2]{};
			for (std::size_t j=0;j<total_bytes;j++) temp[j]=reverse_bits(data_[start_byte+j]);
			for (std::size_t i=0;i<nbits;i++) {
				std::size_t abs_bit=start_off+i;
				std::size_t byte_offset=abs_bit/CHAR_BIT;
				std::size_t bit_offset=abs_bit%CHAR_BIT;
				bool bit;
				if (order_==BO_LSBIT) bit=(temp[byte_offset]>>bit_offset)&1u;
				else bit=(temp[byte_offset]>>(CHAR_BIT-1-bit_offset))&1u;
				if (order_==BO_LSBIT) result|=static_cast<_Tp>(bit)<<i;
				else result=static_cast<_Tp>((result<<1)|static_cast<_Tp>(bit));
			}
			bit_pos_+=nbits;
			return result;
		}
		iterator it(data_[bit_pos_/CHAR_BIT],static_cast<int>(bit_pos_%CHAR_BIT));
		for (std::size_t i=0;i<nbits;i++,it++,bit_pos_++) {
			bool bit=static_cast<bool>(*it);
			if (order_==BO_MSBYTE) result=static_cast<_Tp>((result<<1)|bit);
			else result=static_cast<_Tp>(result|(static_cast<_Tp>(bit)<<i));
		}
		return result;
	}

	uint8_t  read_u8()  { return read_bits<uint8_t>(8);  }
	uint16_t read_u16() { return read_bits<uint16_t>(16); }
	uint32_t read_u32() { return read_bits<uint32_t>(32); }
	uint64_t read_u64() { return read_bits<uint64_t>(64); }

	template <typename _Tp=uint32_t>
	_Tp peek_bits(std::size_t nbits) {
		auto saved=bit_pos_;
		_Tp val=read_bits<_Tp>(nbits);
		bit_pos_=saved;
		return val;
	}

	std::size_t& bit_pos() { return bit_pos_; }
	bit_order& bit_order() noexcept { return order_; }
};

}

}

#endif