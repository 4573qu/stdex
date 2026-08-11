//Last Modified At 2026/08/02
//@Version 1.4.0.0
#ifndef _STDEX_CRYPTO_LZ77_H_
#define _STDEX_CRYPTO_LZ77_H_ 1

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

namespace stdex {

namespace crypto {

struct lz77_token {
	using byte=uint8_t;
	uint16_t distance;
	uint16_t length;
	byte literal;
};

template <std::size_t _MaxChain,bool _Lazy,std::size_t _MinMatch,std::size_t _MaxMatch,std::size_t _WindowSize,std::size_t _HashBits,std::size_t _HashShift,std::size_t _GoodLength=_MaxMatch,std::size_t _NiceLength=_MaxMatch,std::size_t _MaxLazy=_MaxMatch>
class lz77_base {
	static_assert(_MinMatch>=1 && _MinMatch<=8,"Min match cannot be negative than 1 or positive than 8.");
	static_assert(_MaxMatch>=_MinMatch,"Max match cannot be negative than min match.");
	static_assert(_WindowSize>0,"Window size must be positive than 0.");
	static_assert(_HashBits>0  && _HashBits<64,"Hash bits must be positive than 0 and negative than 64.");
	static_assert(_HashShift>=1 && _HashShift<=8,"Hash shift cannot be negative than 1 or positive than 8.");

public:
	using byte=uint8_t;
	using token=lz77_token;

private:
	static constexpr std::size_t hash_size_=std::size_t(1)<<_HashBits; // Hash table size (zlib uses 32K)

	std::vector<int64_t> head_; // hash -> latest absolute position
	std::vector<int64_t> prev_; // previous absolute position with same hash
	int lazy_len_=0;
	int lazy_dist_=0;
	// total input bytes fed so far (absolute position)
	std::size_t total_pos_=0;

	static uint64_t hash_func(uint64_t val) noexcept {
		return ((val*11400714819323198485ull)>>(64-_HashBits))&(hash_size_-1);
	}

	static uint64_t load3(const byte* data,std::size_t pos) noexcept {
		if constexpr (_MinMatch==3) {
			return (static_cast<uint64_t>(data[pos])<<16)|(static_cast<uint64_t>(data[pos+1])<<8)|(static_cast<uint64_t>(data[pos+2]));
		}
		uint64_t result=0;
		std::memcpy(&result,data+pos,_MinMatch);
		//for (int i=0;i<_MinMatch;i++) result|=(static_cast<uint64_t>(data[pos+i])<<(8*(_MinMatch-1-i)));
		return result;
	}

	int64_t insert_hash(std::size_t pos,uint64_t h) noexcept {
		int64_t prev_head=head_[h];
		head_[h]=static_cast<int64_t>(pos);
		prev_[pos%_WindowSize]=prev_head;
		return prev_head;
	}

	static std::size_t match_length(const byte* a,const byte* b,std::size_t limit) noexcept {
		std::size_t len=0;
		while (len+8<=limit) {
			uint64_t va=0,vb=0;
			std::memcpy(&va,a+len,8);
			std::memcpy(&vb,b+len,8);
			if (va!=vb) {
				while (len<limit && a[len]==b[len]) len++;
				return len;
			}
			len+=8;
		}
		while (len<limit && a[len]==b[len]) len++;
		return len;
	}

	std::pair<int,int> find_match(const byte* data,std::size_t size,std::size_t pos,uint64_t h,int current_lazy_len=0) noexcept {
		int best_len=0,best_dist=0;
		const std::size_t max_match=std::min(static_cast<std::size_t>(_MaxMatch),size-pos);
		if (max_match<_MinMatch) return {0,0};
		const std::size_t min_pos=pos>_WindowSize?pos-_WindowSize:0;
		int effective_chain=static_cast<int>(_MaxChain);
		if constexpr (_Lazy) {
			if (current_lazy_len>=static_cast<int>(_GoodLength)) {
				effective_chain>>=2;
				effective_chain=std::max(effective_chain,1);
			}
		}
		int64_t match_pos=head_[h];
		int chain=0;
		while (match_pos>=0 && static_cast<std::size_t>(match_pos)>=min_pos && chain++<effective_chain) {
			const std::size_t abs_match=static_cast<std::size_t>(match_pos);
			if (abs_match>=pos) {
				match_pos=prev_[abs_match%_WindowSize];
				continue;
			}
			if (best_len>0 && data[abs_match+best_len]!=data[pos+best_len]) {
				match_pos=prev_[abs_match%_WindowSize];
				continue;
			}
			if (data[abs_match]==data[pos]) {
				std::size_t len=match_length(data+abs_match,data+pos,max_match);
				if (static_cast<int>(len)>best_len) {
					best_len=static_cast<int>(len);
					best_dist=static_cast<int>(pos-abs_match);
					if (best_len>=static_cast<int>(_NiceLength) || static_cast<std::size_t>(best_len)>=max_match) break;
				}
			}
			match_pos=prev_[abs_match%_WindowSize];
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

	void rebase(std::size_t delta) {
		if (delta==0) return;
		if (delta%_WindowSize!=0) throw std::invalid_argument("delta must be a multiple of the window size");
		const int64_t shift=static_cast<int64_t>(delta);
		for (auto& it:head_) it=it>=shift?it-shift:-1;
		for (auto& it:prev_) it=it>=shift?it-shift:-1;
		lazy_len_=0;
		lazy_dist_=0;
	}

	static constexpr std::size_t window_size() noexcept { return _WindowSize; }
	static constexpr std::size_t min_match() noexcept { return _MinMatch; }
	static constexpr std::size_t max_match() noexcept { return _MaxMatch; }

	std::vector<token> encode(const byte* data,std::size_t size) {
		reset();
		return encode_chunk(data,size,true);
	}

	std::vector<token> encode(const std::vector<byte>& input) {
		return encode(input.data(),input.size());
	}

	std::vector<token> encode_chunk(const byte* data,std::size_t size,bool is_final=false,std::size_t start=0) {
		std::vector<token> tokens;
		if (size<=start) {
			if (is_final && lazy_len_>=static_cast<int>(_MinMatch)) {
				tokens.push_back({static_cast<uint16_t>(lazy_dist_),static_cast<uint16_t>(lazy_len_),0});
				lazy_len_=0;
				lazy_dist_=0;
			}
			return tokens;
		}
		std::size_t position=start;
		const std::size_t matchable_end=size>_MinMatch?size-(_MinMatch-1):0;
		static constexpr int too_far_=4096;
		while (position<matchable_end) {
			uint64_t hash=hash_func(load3(data,position));
			int best_len=0,best_dist=0;
			bool do_search=true;
			if constexpr (_Lazy) {
				if (lazy_len_>=static_cast<int>(_MaxLazy)) do_search=false;
			}
			if (do_search) {
				auto found=find_match(data,size,position,hash,lazy_len_);
				best_len=found.first;
				best_dist=found.second;
				if (best_len==static_cast<int>(_MinMatch) && best_dist>too_far_) {
					best_len=0;
					best_dist=0;
				}
			}
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
						for (std::size_t s=1;s<skip;s++) {
							if (position+_MinMatch<=size && position+s<matchable_end) {
								uint64_t sh=hash_func(load3(data,position+s));
								insert_hash(position+s,sh);
							}
						}
						position+=skip;
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
		total_pos_+=size-start;
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

template <std::size_t _MaxMatch=258,std::size_t _WindowSize=32768,std::size_t _HashBits=15,std::size_t _HashShift=5,std::size_t _GoodLength=_MaxMatch,std::size_t _NiceLength=_MaxMatch>
using lz77=lz77_base<64,false,3,_MaxMatch,_WindowSize,_HashBits,_HashShift,_GoodLength,_NiceLength>;
template <std::size_t _MaxMatch=258,std::size_t _WindowSize=32768,std::size_t _HashBits=15,std::size_t _HashShift=5,std::size_t _GoodLength=_MaxMatch,std::size_t _NiceLength=_MaxMatch>
using lz77_lazy=lz77_base<128,true,3,_MaxMatch,_WindowSize,_HashBits,_HashShift,_GoodLength,_NiceLength>;
template <std::size_t _MaxMatch=258,std::size_t _WindowSize=32768,std::size_t _HashBits=15,std::size_t _HashShift=5,std::size_t _GoodLength=_MaxMatch,std::size_t _NiceLength=_MaxMatch>
using lz77_fast=lz77_base<8,false,3,_MaxMatch,_WindowSize,_HashBits,_HashShift,_GoodLength,_NiceLength>;

}

}
#endif