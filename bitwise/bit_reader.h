//Last Modified At 2026/05/13
//@Version 2.1.0.0
#ifndef _STDEX_BITWISE_BIT_READER_H_
#define _STDEX_BITWISE_BIT_READER_H_ 1

#include <climits>
#include <cstddef>
#include <cstdint>
#include <istream>
#include <stdexcept>
#include <type_traits>
#include <vector>
#include <utility>

#include "bit_iterator.h"//At Least 1.0.0.1
#include "bits.h"//At Least 1.1
#include "endianness.h"//At Least 1.0.0.1

namespace stdex {

namespace bitwise {

class bit_reader_view;

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
		while (bits_in_buf_<=sizeof(bit_buf_)*CHAR_BIT-CHAR_BIT && byte_pos_<(bit_size_+CHAR_BIT-1)/CHAR_BIT) {
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

	friend class bit_reader_view;

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
		if (pos>bit_size_) throw std::out_of_range("seek_bits out of range");
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
		if (nbits==0 || nbits>sizeof(_Tp)*CHAR_BIT) throw std::invalid_argument("Invalid bit count");
		if (bit_pos_+nbits>bit_size_) throw std::runtime_error("Unexpected EOF");
		fill_buffer();
		if (bits_in_buf_>=static_cast<int>(nbits)) {
			_Tp result=0;
 			if (order_==BO_LSBYTE || order_==BO_LSBIT) {
				result=static_cast<_Tp>(bit_buf_&((1ULL<<nbits)-1));
				bit_buf_>>=nbits;
			} else result=static_cast<_Tp>((bit_buf_>>(bits_in_buf_-nbits))&((1ULL<<nbits)-1));
			bits_in_buf_-=static_cast<int>(nbits);
			bit_pos_+=nbits;
			return result;
		}
		_Tp result=0;
		int taken=0;
		if (bits_in_buf_>0) {
			if (order_==BO_LSBYTE || order_==BO_LSBIT) {
				result=static_cast<_Tp>(bit_buf_&((1ULL<<bits_in_buf_)-1));
				taken=bits_in_buf_;
			} else {
				result=static_cast<_Tp>(bit_buf_&((1ULL<<bits_in_buf_)-1));
				taken=bits_in_buf_;
			}
			bit_pos_+=bits_in_buf_;
			bits_in_buf_=0;
			bit_buf_=0;
		}
		int remaining_needed=static_cast<int>(nbits)-taken;
		while (remaining_needed>0) {
			fill_buffer();
			int can_take=std::min(remaining_needed,bits_in_buf_);
			if (can_take<=0) throw std::runtime_error("Unexpected EOF");
			if (order_==BO_LSBYTE || order_==BO_LSBIT) {
				_Tp chunk=static_cast<_Tp>(bit_buf_&((1ULL<<can_take)-1));
				result|=chunk<<taken;
				bit_buf_>>=can_take;
			} else {
				_Tp chunk=static_cast<_Tp>((bit_buf_>>(bits_in_buf_-can_take))&((1ULL<<can_take)-1));
				result=static_cast<_Tp>((result<<can_take)|chunk);
			}
			bits_in_buf_-=can_take;
			bit_pos_+=can_take;
			taken+=can_take;
			remaining_needed-=can_take;
		}
		return result;
	}

	void read_bytes(void* dst,std::size_t byte_count) {
		if (!is_aligned()) throw std::runtime_error("read_bytes requires byte alignment");
		std::size_t byte_pos=bit_pos_/CHAR_BIT;
		std::size_t avail=(bit_size_-bit_pos_)/CHAR_BIT;
		if (byte_count>avail) throw std::out_of_range("read_bytes overflow");
		std::memcpy(dst,data_+byte_pos,byte_count);
		seek_bits(bit_pos_+byte_count*CHAR_BIT);
	}

	uint8_t  read_u8()  { return read_bits<uint8_t>(8);  }
	uint16_t read_u16() { return read_bits<uint16_t>(16); }
	uint32_t read_u32() { return read_bits<uint32_t>(32); }
	uint64_t read_u64() { return read_bits<uint64_t>(64); }

	template <typename _Tp=uint32_t>
	_Tp peek_bits(std::size_t nbits) {
		fill_buffer();
		if (bits_in_buf_>=static_cast<int>(nbits)) {
			if (order_==BO_LSBYTE || order_==BO_LSBIT) return static_cast<_Tp>(bit_buf_&((1ULL<<nbits)-1));
			else return static_cast<_Tp>((bit_buf_>>(bits_in_buf_-nbits))&((1ULL<<nbits)-1));
		}
		uint64_t saved_buf=bit_buf_;
		int saved_avail=bits_in_buf_;
		std::size_t saved_bpos=byte_pos_;
		std::size_t saved_bitpos=bit_pos_;
		_Tp val=read_bits<_Tp>(nbits);
		bit_buf_=saved_buf;
		bits_in_buf_=saved_avail;
		byte_pos_=saved_bpos;
		bit_pos_=saved_bitpos;
		return val;
	}

	void drop_bits(std::size_t nbits) {
		if (bits_in_buf_<static_cast<int>(nbits)) throw std::runtime_error("drop_bits exceeds buffer");
		if (order_==BO_LSBYTE || order_==BO_LSBIT) bit_buf_>>=nbits;
		bits_in_buf_-=static_cast<int>(nbits);
		bit_pos_+=nbits;
	}

	[[nodiscard]]
	bool is_aligned() const noexcept { return (bit_pos_%CHAR_BIT)==0; }

	[[nodiscard]]
	std::size_t remaining_bits() const noexcept { return bit_size_-bit_pos_; }
	[[nodiscard]]
	std::size_t remaining_bytes() const noexcept {
		return (bit_size_-bit_pos_+CHAR_BIT-1)/CHAR_BIT;
	}
	[[nodiscard]]
	std::size_t size_bytes() const noexcept {
		return (bit_size_+CHAR_BIT-1)/CHAR_BIT;
	}

	void rewind() { seek_bits(0); }

	std::size_t bit_pos() { return bit_pos_; }
	bit_order& bit_order() noexcept { return order_; }
	const uint8_t* data() const noexcept { return data_; }

	bit_reader_view borrow_view();
};

class bit_reader_view {
	bit_reader& reader_;
	uint64_t buf_;
	int bits_in_buf_;
	std::size_t byte_pos_;
	const uint8_t* data_;
	std::size_t bend_;
	bool returned_=false;

	void swap_reader() {
		std::swap(buf_,reader_.bit_buf_);
		std::swap(bits_in_buf_,reader_.bits_in_buf_);
		std::swap(byte_pos_,reader_.byte_pos_);
	}

public:
	explicit bit_reader_view(bit_reader& br) : reader_(br) , buf_(br.bit_buf_) , bits_in_buf_(br.bits_in_buf_) , byte_pos_(br.byte_pos_) , data_(br.data_) , bend_((br.bit_size_+CHAR_BIT-1)/CHAR_BIT) {
		reader_.bit_buf_=0;
		reader_.bits_in_buf_=0;
	}
	~bit_reader_view() {
		if (!returned_) return_to_reader();
	}
	bit_reader_view(const bit_reader_view&)=delete;
	bit_reader_view& operator =(const bit_reader_view&)=delete;

	void return_to_reader() {
		reader_.bit_buf_=buf_;
		reader_.bits_in_buf_=bits_in_buf_;
		reader_.byte_pos_=byte_pos_;
		reader_.bit_pos_=byte_pos_*CHAR_BIT-bits_in_buf_;
		returned_=true;
	}

	void refill() {
		swap_reader();
		reader_.fill_buffer();
		swap_reader();
	}

	template <typename _Tp=uint32_t>
	_Tp peek(int n) {
		swap_reader();
		_Tp result=reader_.peek_bits<_Tp>(n);
		swap_reader();
		return result;
	}

	void consume(int n) {
		swap_reader();
		reader_.drop_bits(n);
		swap_reader();
	}

	std::size_t remaining_bits() const noexcept {
		return (bend_-byte_pos_)*CHAR_BIT+bits_in_buf_;
	}

	[[nodiscard]]
	bool eof() const noexcept { return remaining_bits()==0; }
};

inline bit_reader_view bit_reader::borrow_view() {
	return bit_reader_view(*this);
}

}

}

#endif