//Last Modified At 2025/11/12
//@Version 1.1.0.0
#ifndef _STDEX_CRYPTO_LZ77_H_
#define _STDEX_CRYPTO_LZ77_H_ 1

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

namespace stdex {

namespace crypto {

template <std::size_t _MaxChain,bool _Lazy,std::size_t _MinMatch,std::size_t _MaxMatch,std::size_t _WindowSize,std::size_t _HashBits,std::size_t _HashShift>
class lz77_base {
	static_assert(_MinMatch>=1 && _MinMatch<=8,"Min match cannot be negative than 1 or positive than 8.");
	static_assert(_MaxMatch>=_MinMatch,"Max match cannot be negative than min match.");
	static_assert(_WindowSize>0,"Window size must be positive than 0.");
	static_assert(_HashBits>0  && _HashBits<64,"Hash bits must be positive than 0 and negative than 64.");
	static_assert(_HashShift>=1 && _HashShift<=8,"Hash shift cannot be negative than 1 or positive than 8.");

public:
	using byte=uint8_t;

	struct token {
		uint16_t distance_;
		uint16_t length_;
		byte literal_;
	};

private:
	static constexpr std::size_t hash_size_=1<<_HashBits; // Hash table size (zlib uses 32K)

	std::vector<int> head_; // hash -> latest position
	std::vector<int> prev_; // previous position with same hash
	int hash_head_=-1;

	static uint64_t hash_func(uint64_t val) noexcept {
		return ((val*2654435761u)>>(64-_HashBits))&(hash_size_-1);
	}
	static uint64_t load3(const std::vector<byte>& data,std::size_t pos) noexcept {
		if (_MinMatch==3) return (static_cast<uint64_t>(data[pos])<<16)|(static_cast<uint64_t>(data[pos+1])<<8)|(static_cast<uint64_t>(data[pos+2]));
		uint64_t result=0;
		for (int i=0;i<_MinMatch;i++) result|=(static_cast<uint64_t>(data[pos+i])<<(8*(_MinMatch-1-i)));
		return result;
	}

public:
	explicit lz77_base() : head_(hash_size_,-1) , prev_(_WindowSize,-1) { }

	std::vector<token> encode(const std::vector<byte>& input) {
		std::vector<token> tokens;
		if (input.empty()) return tokens;
		std::size_t input_size=input.size();
		uint64_t hash=0;
		std::size_t position=0;
		int prev_match_len=0,prev_match_dist=0;
		while (position+_MinMatch<input_size) {
			hash=hash_func(load3(input,position));
			/*
				if (position==0) hash=hash_func(load3(input,0));
				else hash=((hash<<_HashShift)^input[position+_MinMatch-1])&(hash_size_-1);
			*/
			//#define UPDATE_HASH(h, c) (h = (((h) << HASH_SHIFT) ^ (c)) & HASH_MASK)
			int best_len=0;
			int best_dist=0;
			int match=head_[hash];
			head_[hash]=static_cast<int>(position);
			prev_[position%_WindowSize]=match;
			int chain_count=0;
			while (match>=0 && chain_count++<static_cast<int>(_MaxChain)) {
				int dist=static_cast<int>(position-match);
				if (dist>static_cast<int>(_WindowSize)) break;
				int len=0;
				while (len<static_cast<int>(_MaxMatch) && position+len<input_size && input[match+len]==input[position+len]) len++;
				if (len>best_len) {
					best_len=len;
					best_dist=dist;
					if (len>=static_cast<int>(_MaxMatch)) break;
				}
				match=prev_[match%_WindowSize];
			}
			if (_Lazy && prev_match_len!=0 && best_len<=prev_match_len) {
				tokens.push_back({(uint16_t)prev_match_dist,(uint16_t)prev_match_len,0});
				position+=prev_match_len-1;
				prev_match_len=prev_match_dist=0;
				continue;
			}
			if (best_len>=_MinMatch) {
				/*if (best_len>=_MinMatch) {
					if (_Lazy && !lazy_pending) {
						lazy_pending=true;
						lazy_len=best_len;
						lazy_dist=best_dist;
						position++;
						continue;
					}
					if (_Lazy && lazy_pending) {
						if (best_len>lazy_len) {
							lazy_len=best_len;
							lazy_dist=best_dist;
							continue;
						} else {
							tokens.push_back({(uint16_t)lazy_dist,(uint16_t)lazy_len,0});
							lazy_pending=false;
							position+=lazy_len-1; 
							continue;
						}
					}
				}
				// literal fallback...
				*/
				if (_Lazy) {
					prev_match_len=best_len;
					prev_match_dist=best_dist;
					position++;
					continue;
				} else {
					tokens.push_back({(uint16_t)best_dist,(uint16_t)best_len,0});
					position+=best_len;
				}
			} else {
				tokens.push_back({0,0,input[position]});
				position++;
			}
		}
		while (position<input_size) tokens.push_back({0,0,input[position++]});
		return tokens;
	}
	std::vector<byte> decode(const std::vector<token>& tokens) {
		std::vector<byte> result;
		result.reserve(tokens.size()*2);
		for (auto& it:tokens) {
			if (it.length_==0) result.push_back(it.literal_);
			else {
				std::size_t start=result.size()-it.distance_;
				for (std::size_t i=0;i<it.length_;i++) result.push_back(result[start+i]);
			}
		}
		return result;
	}
	void reset() {
		std::fill(head_.begin(),head_.end(),-1);
		std::fill(prev_.begin(),prev_.end(),-1);
	}
};

template <std::size_t _MaxMatch=258,std::size_t _WindowSize=32768,std::size_t _HashBits=15,std::size_t _HashShift=5>
using lz77=lz77_base<64,false,3,_MaxMatch,_WindowSize,_HashBits,_HashShift>;
//using lz77_fast=lz77_base<16,false>;
//using lz77_highratio=lz77_base<256,true>;

}

}
#endif