//Last Modified At 2025/09/13
//@Version 1.0.0.0
#ifndef _STD4573_BITWISE_BIT_ITERATOR_H_
#define _STD4573_BITWISE_BIT_ITERATOR_H_ 1

#include <cstddef>
#include <cstdint>
#include <iterator>
#include <type_traits>

#include "bit_ref.h"//At Least 1.0

namespace stdex {

namespace bitwise {

template <typename _Tp>
class bit_iterator {
	_Tp* data_;
	int index_;

public:
#define _STDEX_BIT_ITERATOR_SIZE (sizeof(_Tp)*8)
	bit_iterator() noexcept : data_(nullptr) , index_(0) {}
	bit_iterator(_Tp& data,int index_=0) noexcept : data_(&data) , index(index_) {
		while (index_<0) {
			index_+=_STDEX_BIT_ITERATOR_SIZE;
			data_--;
		}
		while (index_>=_STDEX_BIT_ITERATOR_SIZE) {
			index_-=_STDEX_BIT_ITERATOR_SIZE;
			data_++;
		}
	}

	bit_ref operator *() const noexcept {
		return bit_ref(*data_,index_);
	}

	bit_iterator& operator ++() noexcept {
		if (++index_>=_STDEX_BIT_ITERATOR_SIZE) {
			index_=0;
			data_++;
		}
		return *this;
	}
	bit_iterator operator ++(int) noexcept {
		bit_iterator temp=*this;
		++(*this);
		return temp;
	}
	bit_iterator& operator --() noexcept {
		if (!index_) {
			index_=_STDEX_BIT_ITERATOR_SIZE-1;
			data_--;
		} else index_--;
		return *this;
	}
	bit_iterator operator --(int) noexcept {
		bit_iterator temp=*this;
		--(*this);
		return temp;
	}
	bit_iterator& operator +=(std::ptrdiff_t n) noexcept {
		if (n<0) return *this-=(-n);
		std::ptrdiff_t total_bits=index_+n;
		data_+=total_bits/_STDEX_BIT_ITERATOR_SIZE;
		index+=total_bits%_STDEX_BIT_ITERATOR_SIZE;
		return *this;
	}
	bit_iterator& operator -=(std::ptrdiff_t n) noexcept {
		if (n<0) return *this+=(-n);
		std::ptrdiff_t total_bits=index_-n;
		if (total_bits<0) {
			data_-=(-total_bits+_STDEX_BIT_ITERATOR_SIZE-1)/_STDEX_BIT_ITERATOR_SIZE;
			total_bits=(-total_bits)%_STDEX_BIT_ITERATOR_SIZE;
			if (total_bits) index_=_STDEX_BIT_ITERATOR_SIZE-total_bits;
			else index_=0;
		} else index_=total_bits;
		return *this;
	}
	bit_iterator operator +(std::ptrdiff_t n) const noexcept {
		return bit_iterator(*this)+=n;
	}
	bit_iterator operator -(std::ptrdiff_t n) const noexcept {
		return bit_iterator(*this)-=n;
	}
	std::ptrdiff_t operator -(const bit_iterator& other) const noexcept {
		return (data_-other.data_)*_STDEX_BIT_ITERATOR_SIZE+(index_-other.index_);
	}
	bool operator ==(const bit_iterator& other) const noexcept {
		return data_==other.data_ && index_==other.index_;
	}
	bool operator !=(const bit_iterator& other) const noexcept {
		return !(*this==other);
	}
	bool operator <(const bit_iterator& other) const noexcept {
		if (data_!=other.data_) return data_<other.data_;
		return index_<other.index_;
	}
	bool operator <=(const bit_iterator& other) const noexcept {
		return !(other<*this);
	}
	bool operator >(const bit_iterator& other) const noexcept {
		return other<*this;
	}
	bool operator >=(const bit_iterator& other) const noexcept {
		return !(*this<other);
	}
#undef _STDEX_BIT_ITERATOR_SIZE
};

}

}

#endif