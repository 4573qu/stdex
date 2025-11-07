//Last Modified At 2025/11/08
//@Version 1.0.0.0
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

template <std::size_t _MaxChain,bool _Lazy>
class lz77_base {
public:
	using byte=uint8_t;

	struct token {
		uint16_t distance_;
		uint16_t length_;
		byte literal_;
	};

private:
	static constexpr std::size_t window_size_=32768;   // 32KB sliding window
	static constexpr std::size_t hash_size_=1<<15; // Hash table size (zlib uses 32K)
	static constexpr std::size_t min_match_=3;
	static constexpr std::size_t max_match_=258;
	static constexpr std::size_t hash_shift_=5;       // For 3-byte rolling hash

	std::vector<int> head_; // hash -> latest position
	std::vector<int> prev_; // previous position with same hash
	int hash_head_=-1;

	static uint32_t hash_func(uint32_t val) noexcept {
		return ((val*2654435761u)>>(32-15))&(hash_size_-1);
	}

public:
	explicit lz77_base() : head_(hash_size_,-1) , prev_(window_size_,-1) { }

	std::vector<token> encode(const std::vector<byte>& input) {
		std::vector<Token> tokens;
		if (input.empty()) return tokens;
		std::size_t input_size=input.size();
		uint32_t hash=0;
		std::size_t position=0;
		auto load3=[&](std::size_t pos)->uint32_t{
			return (static_cast<uint32_t>(input[pos])<<16)|(static_cast<uint32_t>(input[pos+1])<<8)|(static_cast<uint32_t>(input[pos+2]));
		};
		int prev_match_len=0,prev_match_dist=0;
		while (position+min_match_<input_size) {
			hash=hash_func(load3(position));
			int best_len=0;
			int best_dist=0;
			int match=head_[hash];
			head_[hash]=static_cast<int>(position);
			prev_[position%window_size_]=match;
			int chain_count=0;
			while (match>=0 && chain_count++<_MaxChain) {
				int dist=static_cast<int>(position-match);
				if (dist>window_size_) break;
				int len=0;
				while (len<max_match_ && position+len<input_size && input[match+len]==input[position+len]) len++;
				if (len>best_len) {
					best_len=len;
					best_dist=dist;
					if (len>=max_match_) break;
				}
				match=prev_[match%window_size_];
			}
			if (_Lazy && prev_match_len!=0 && best_len<=prev_match_len) {
				tokens_.push_back({(uint16_t)prev_match_dist,(uint16_t)prev_match_len,0});
				pos+=prev_match_len;
				prev_match_len=prev_match_dist=0;
				continue;
			}
			if (best_len>=min_match_) {
				if (_Lazy) {
					prev_match_len=best_len;
					prev_match_dist=best_dist;
					position++;
					continue;
				} else {
					tokens.push_back({static_cast<uint16_t>(best_dist),static_cast<uint16_t>(best_len),0});
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
		return output;
	}
};

using lz77=lz77_base<64,false>;
using lz77_fast=lz77_base<16,false>;
using lz77_highratio=lz77_base<256,true>;

}

}
#endif