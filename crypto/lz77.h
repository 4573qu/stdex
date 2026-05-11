//Last Modified At 2026/05/11
//@Version 1.2.1.0
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
		uint16_t distance;
		uint16_t length;
		byte literal;
	};

private:
	static constexpr std::size_t hash_size_=std::size_t(1)<<_HashBits; // Hash table size (zlib uses 32K)

	std::vector<int64_t> head_; // hash -> latest position
	std::vector<int64_t> prev_; // previous position with same hash
	int lazy_len_=0;
	int lazy_dist_=0;
	// total input bytes fed so far (absolute position)
	std::size_t total_pos_=0;

	static uint64_t hash_func(uint64_t val) noexcept {
		return ((val*11400714819323198485ull)>>(64-_HashBits))&(hash_size_-1);
	}

	static uint64_t load3(const byte* data,std::size_t pos) noexcept {
		if (_MinMatch==3) return (static_cast<uint64_t>(data[pos])<<16)|(static_cast<uint64_t>(data[pos+1])<<8)|(static_cast<uint64_t>(data[pos+2]));
		uint64_t result=0;
		for (int i=0;i<_MinMatch;i++) result|=(static_cast<uint64_t>(data[pos+i])<<(8*(_MinMatch-1-i)));
		return result;
	}

	int insert_hash(std::size_t pos,uint64_t h) noexcept {
		int64_t prev_head=head_[h];
		head_[h]=pos%_WindowSize;
		prev_[pos%_WindowSize]=prev_head;
		return prev_head;
	}

	std::pair<int,int> find_match(const byte* data,std::size_t size,std::size_t pos,uint64_t h) noexcept {
		int best_len=0,best_dist=0;
		int match_pos=head_[h];
		int chain=0;
		while (match_pos>=0 && chain++<static_cast<int>(_MaxChain)) {
			std::size_t abs_match;
			std::size_t cur_slot=pos%_WindowSize;
			std::size_t mat_slot=static_cast<std::size_t>(match_pos);
			if (mat_slot<=cur_slot) abs_match=pos-(cur_slot-mat_slot);
			else abs_match=pos-(_WindowSize-mat_slot+cur_slot);
			int dist=static_cast<int>(pos-abs_match);
			if (pos<static_cast<std::size_t>(dist)) {
				match_pos=prev_[mat_slot];
				continue; 
			}
			if (dist<=0 || dist>static_cast<int>(_WindowSize)) break;
			if (best_len>0) {
				if (pos+best_len>=size || data[abs_match+best_len]!=data[pos+best_len]) {
					match_pos=prev_[mat_slot];
					continue;
				}
			}
			if (data[abs_match]!= data[pos]) {
				match_pos=prev_[mat_slot];
				continue;
			}
			int len=1;
			while (len<static_cast<int>(_MaxMatch) && pos+static_cast<std::size_t>(len)<size && data[abs_match+len]==data[pos+len]) len++;
			if (len>best_len) {
				best_len=len;
				best_dist=dist;
				if (len>=static_cast<int>(_MaxMatch)) break;
			}
			match_pos=prev_[mat_slot];
		}
		return {best_len,best_dist};
	}


public:
	explicit lz77_base() : head_(hash_size_,-1) , prev_(_WindowSize,-1) { }

	void reset() noexcept {
		std::fill(head_.begin(),head_.end(),-1);
		std::fill(prev_.begin(),prev_.end(),-1);
		lazy_len_=0;
		lazy_dist_=0;
		total_pos_=0;
	}

	std::vector<token> encode(const std::vector<byte>& input) {
		reset();
		return encode_chunk(input.data(),input.size(),true);
	}

	std::vector<token> encode_chunk(const byte* data,std::size_t size,bool is_final=false) {
		std::vector<token> tokens;
		if (size==0) {
			if (is_final && lazy_len_>=static_cast<int>(_MinMatch)) {
				tokens.push_back({static_cast<uint16_t>(lazy_dist_),static_cast<uint16_t>(lazy_len_),0});
				lazy_len_=0;
				lazy_dist_=0;
			}
			return tokens;
		}
		std::size_t position=0;
		const std::size_t matchable_end=size>_MinMatch?size-(_MinMatch-1):0;
		while (position<matchable_end) {
			uint64_t hash=hash_func(load3(data,position));
			auto [best_len,best_dist]=find_match(data,size,position,hash);
			insert_hash(position,hash);
			if constexpr (_Lazy) {
				if (lazy_len_>=static_cast<int>(_MinMatch)) {
					if (best_len>lazy_len_) {
						tokens.push_back({0,0,data[position-1]});
						lazy_len_=best_len;
						lazy_dist_=best_dist;
						position++;
					} else {
						tokens.push_back({static_cast<uint16_t>(lazy_dist_),static_cast<uint16_t>(lazy_len_),0});
						std::size_t skip=static_cast<std::size_t>(lazy_len_)-1;
						for (std::size_t s=0;s<skip && position<matchable_end;s++,position++) {
							if (position+_MinMatch<=size) {
								uint64_t sh=hash_func(load3(data,position));
								insert_hash(position,sh);
							}
						}
						lazy_len_=0;
						lazy_dist_=0;
					}
				} else {
					if (best_len>=static_cast<int>(_MinMatch)) {
						lazy_len_=best_len;
						lazy_dist_=best_dist;
						position++;
					} else {
						tokens.push_back({0,0,data[position]});
						position++;
					}
				}
			} else {
				if (best_len>=static_cast<int>(_MinMatch)) {
					tokens.push_back({static_cast<uint16_t>(best_dist),static_cast<uint16_t>(best_len),0});
					for (std::size_t s=1;s<static_cast<std::size_t>(best_len) && position+s+_MinMatch<=size;s++) {
						uint64_t sh=hash_func(load3(data,position+s));
						insert_hash(position+s,sh);
					}
					position+=static_cast<std::size_t>(best_len);
				} else {
					tokens.push_back({0,0,data[position]});
					position++;
				}
			}
		}
		if constexpr (_Lazy) {
			if (lazy_len_>=static_cast<int>(_MinMatch)) {
				if (lazy_dist_>0) {
					tokens.push_back({static_cast<uint16_t>(lazy_dist_),static_cast<uint16_t>(lazy_len_),0});
					position+=static_cast<std::size_t>(lazy_len_)-1;
					lazy_len_=0;
					lazy_dist_=0;
				}
			}
		}
		while (position<size) {
			tokens.push_back({0,0,data[position]});
			position++;
		}
		total_pos_+=size;
		return tokens;
	}

	std::vector<byte> decode(const std::vector<token>& tokens) {
		std::vector<byte> result;
		result.reserve(tokens.size()*2);
		for (auto& it:tokens) {
			if (it.length==0) result.push_back(it.literal);
			else {
				std::size_t start=result.size()-it.distance;
				for (std::size_t i=0;i<it.length;i++) result.push_back(result[start+i]);
			}
		}
		return result;
	}

	std::size_t total_input() const noexcept { return total_pos_; }
};

template <std::size_t _MaxMatch=258,std::size_t _WindowSize=32768,std::size_t _HashBits=15,std::size_t _HashShift=5>
using lz77=lz77_base<64,false,3,_MaxMatch,_WindowSize,_HashBits,_HashShift>;
template <std::size_t _MaxMatch=258,std::size_t _WindowSize=32768,std::size_t _HashBits=15,std::size_t _HashShift=5>
using lz77_lazy=lz77_base<128,true,3,_MaxMatch,_WindowSize,_HashBits,_HashShift>;
template <std::size_t _MaxMatch=258,std::size_t _WindowSize=32768,std::size_t _HashBits=15,std::size_t _HashShift=5>
using lz77_fast=lz77_base<8,false,3,_MaxMatch,_WindowSize,_HashBits,_HashShift>;

}

}
#endif