//Last Modified At 2025/09/14
//@Version 1.0.0.1
#ifndef _STD4573_BITWISE_DYNAMIC_BITSET_H_
#define _STD4573_BITWISE_DYNAMIC_BITSET_H_ 1

#include <algorithm>
#include <climits>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <vector>

#include "bit_ref.h"//At Least 1.0

namespace stdex {

namespace bitwise {

class dynamic_bitset {
	std::vector<uint64_t> blocks_;
	std::size_t num_bits_;

	static std::size_t blocks_needed(std::size_t num_bits) noexcept {
		return (num_bits+bpblock_-1)/bpblock_;
	}
	static std::size_t block_index(std::size_t pos) noexcept {
		return pos/bpblock_;
	}
	static int bit_index(std::size_t pos) noexcept {
		return pos/bpblock_;
	}
	void check_range(std::size_t pos) const {
		if (pos>=num_bits_) throw std::out_of_range("Position out of range");
	}

public:
#define _STDEX_DYNAMIC_BITSET_BLOCK_POS blocks_[block_index(pos)]
#define _STDEX_DYNAMIC_BITSET_BLOCK_VALUE (uint64_t(1)<<bit_index(pos))
	static constexpr int bpblock_=sizeof(uint64_t)*CHAR_BIT;//bits_per_block

	dynamic_bitset() : num_bits_(0) { }
	explicit dynamic_bitset(std::size_t num_bits,bool value=false) : blocks_(blocks_needed(num_bits),value?~uint64_t(0):0) , num_bits_(num_bits) { }

	void set(std::size_t pos,bool value=true) {
		check_range(pos);
		if (value) _STDEX_DYNAMIC_BITSET_BLOCK_POS|=_STDEX_DYNAMIC_BITSET_BLOCK_VALUE;
		else _STDEX_DYNAMIC_BITSET_BLOCK_POS&=~_STDEX_DYNAMIC_BITSET_BLOCK_VALUE;
	}
	void reset(std::size_t pos) {
		set(pos,false);
	}
	void flip(std::size_t pos) {
		check_range(pos);
		_STDEX_DYNAMIC_BITSET_BLOCK_POS^=_STDEX_DYNAMIC_BITSET_BLOCK_VALUE;
	}
	bool test(std::size_t pos) const {
		check_range(pos);
		return _STDEX_DYNAMIC_BITSET_BLOCK_POS&_STDEX_DYNAMIC_BITSET_BLOCK_VALUE;
	}
	bit_ref operator [](std::size_t pos) {
		check_range(pos);
		return bit_ref(reinterpret_cast<uint8_t&>(_STDEX_DYNAMIC_BITSET_BLOCK_POS),bit_index(pos));
	}
	std::size_t size() const noexcept {
		return num_bits_;
	}
	void resize(std::size_t new_size,bool value=false) {
		if (new_size>num_bits_) {
			std::size_t new_blocks_needed=blocks_needed(new_size);
			if (new_blocks_needed>blocks_.size()) blocks_.resize(new_blocks_needed,value?~uint64_t(0):0);
			if (value) {
				for (std::size_t i=num_bits_;i<new_size;i++) set(i,true);
			}
		}
		num_bits_=new_size;
	}
	dynamic_bitset& operator &=(const dynamic_bitset& other) {
		std::size_t min_blocks=std::min(blocks_.size(),other.blocks_.size());
		for (std::size_t i=0;i<min_blocks;i++) blocks_[i]&=other.blocks_[i];
		for (std::size_t i=min_blocks;i<blocks_.size();i++) blocks_[i]=0;
		return *this;
	}
	dynamic_bitset& operator |=(const dynamic_bitset& other) {
		std::size_t min_blocks=std::min(blocks_.size(),other.blocks_.size());
		for (std::size_t i=0;i<min_blocks;i++) blocks_[i]|=other.blocks_[i];
		return *this;
	}
	dynamic_bitset& operator ^=(const dynamic_bitset& other) {
		std::size_t min_blocks=std::min(blocks_.size(),other.blocks_.size());
		for (std::size_t i=0;i<min_blocks;i++) blocks_[i]^=other.blocks_[i];
		return *this;
	}
	dynamic_bitset& operator ~() {
		for (std::size_t i=0;i<blocks_.size();i++) blocks_[i]=~blocks_[i];
		return *this;
	}
#undef _STDEX_DYNAMIC_BITSET_BLOCK_VALUE
#undef _STDEX_DYNAMIC_BITSET_BLOCK_POS
};

}

}

#endif