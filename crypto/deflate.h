//Last Modified At 2026/05/10
//@Version 1.0.0.0
#ifndef _STDEX_CRYPTO_DEFLATE_H_
#define _STDEX_CRYPTO_DEFLATE_H_ 1

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <functional>
#include <iterator>
#include <memory>
#include <numeric>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>


#include "../bitwise/bit_reader.h"
#include "../bitwise/bit_writer.h"
#include "../bitwise/bits.h"
#include "../crypto/lz77.h"
#include "../integrity/adler.h"
#include "../integrity/crc.h"
#include "../structure/huffman.h"

#if __has_include("../macros/cpp_version.h")
#include "../macros/cpp_version.h"
#endif

#ifndef _STDEX_CPP20_VERSION
#define _STDEX_CPP20_VERSION 202002L
#endif

#if __cplusplus>=_STDEX_CPP20_VERSION
#include <span>
#include <concepts>
#endif

namespace stdex {

namespace crypto {

enum deflate_stream_format {
	DSF_RAW,
	DSF_ZLIB,
	DSF_GZIP,
};

enum deflate_block_type {
	DBT_STORED=0,
	DBT_FIXED,
	DBT_DYNAMIC,
	DBT_AUTOMATIC,
};

enum deflate_level {
	DL_NONE=0,
	DL_FASTEST=1,
	DL_FAST=3,
	DL_NORMAL=6,
	DL_BEST=9,
};

struct gzip_info {
	std::string filename;
	std::string comment;
	uint32_t mtime=0;
	uint8_t os=0xFF;
	bool is_text=false;
	bool has_crc16=false;
};

class deflate_compressor;
class deflate_decompressor;

class deflate {

	template <typename _It,typename _Tp,typename=void>
	struct is_output_iter : std::false_type {};

	template <typename _It,typename _Tp>
	struct is_output_iter<_It,_Tp,std::void_t<decltype(*std::declval<_It&>()=std::declval<_Tp>()),decltype(++std::declval<_It&>())>> : std::true_type {};

#if __cplusplus>=_STDEX_CPP20_VERSION
	template <typename _It>
	concept byte_output_iterator=std::output_iterator<_It,uint8_t>;
#endif

	struct canonical_tree {
		std::vector<uint16_t> table;
		std::vector<uint8_t> length;
		int maxbits=0;
		std::vector<std::vector<uint16_t>> codes_bl;
		std::vector<std::vector<int>> syms_bl;

		bool empty() const noexcept {
			return length.empty() || maxbits==0;
		}

		int decode(bitwise::bit_reader& br) const {
			int code=0,len=1;
			for (;len<=maxbits;len++) {
				code|=br.read_bits<uint8_t>(1)<<(len-1);
				const auto& codes=codes_bl[len];
				const auto& syms=syms_bl[len];
				for (std::size_t i=0;i<codes.size();i++) {
					if (codes[i]==(uint16_t)code) return syms[i];
				}
			}
			throw std::runtime_error("invalid huffman code");
		}
	};

	static constexpr std::pair<int,int> length_codes_[29]={
		{3,0},{4,0},{5,0},{6,0},{7,0},{8,0},{9,0},{10,0},
		{11,1},{13,1},{15,1},{17,1},
		{19,2},{23,2},{27,2},{31,2},
		{35,3},{43,3},{51,3},{59,3},
		{67,4},{83,4},{99,4},{115,4},
		{131,5},{163,5},{195,5},{227,5},{258,0}
	};

	static constexpr std::pair<int,int> dist_codes_[30]={
		{1,0},{2,0},{3,0},{4,0},
		{5,1},{7,1},{9,2},{13,2},
		{17,3},{25,3},{33,4},{49,4},
		{65,5},{97,5},{129,6},{193,6},
		{257,7},{385,7},{513,8},{769,8},
		{1025,9},{1537,9},{2049,10},{3073,10},
		{4097,11},{6145,11},{8193,12},{12289,12},
		{16385,13},{24577,13}
	};

	static constexpr int order_[19]={16,17,18,0,8,7,9,6,10,5,11,4,12,3,13,2,14,1,15};

	static void build_canonical_table(canonical_tree& h) {
		int maxbits=0;
		for (uint8_t it:h.length) {
			if (it>maxbits) maxbits=it;
		}
		h.maxbits=maxbits;
		if (maxbits==0) {
			h.codes_bl.clear();
			h.syms_bl.clear();
			return;
		}
		std::vector<int> bl_count(maxbits+1,0);
		for (uint8_t it:h.length) {
			if (it) bl_count[it]++;
		}
		std::vector<int> next_code(maxbits+1,0);
		int code=0;
		for (int bits=1;bits<=maxbits;bits++) {
			code=(code+bl_count[bits-1])<<1;
			next_code[bits]=code;
		}
		h.table.assign(h.length.size(),0);
		h.codes_bl.assign(maxbits+1,{});
		h.syms_bl.assign(maxbits+1,{});
		for (std::size_t n=0;n<h.length.size();n++) {
			int len=h.length[n];
			if (len!=0) {
				int val=next_code[len]++;
				int rev=0;
				for (int i=0;i<len;i++) rev=(rev<<1)|((val>>i)&1);
				h.table[n]=static_cast<uint16_t>(rev);
				h.codes_bl[len].push_back(h.table[n]);
				h.syms_bl[len].push_back(static_cast<int>(n));
			}
		}
	}

	static canonical_tree build_fixed_lit() {
		canonical_tree h;
		h.length.resize(288,0);
		for (int i=0;i<=143;i++) h.length[i]=8;
		for (int i=144;i<=255;i++) h.length[i]=9;
		for (int i=256;i<=279;i++) h.length[i]=7;
		for (int i=280;i<=287;i++) h.length[i]=8;
		build_canonical_table(h);
		return h;
	}

	static canonical_tree build_fixed_dist() {
		canonical_tree h;
		h.length.assign(32,5);
		build_canonical_table(h);
		return h;
	}

	static canonical_tree build_dynamic_tree(const uint32_t* freq,int n) {
		canonical_tree result;
		result.length.assign(n,0);
		std::vector<uint32_t> freqs;
		std::vector<int> symbols;
		freqs.reserve(n);
		symbols.reserve(n);
		for (int i=0;i<n;i++) {
			if (freq[i]>0) {
				freqs.push_back(freq[i]);
				symbols.push_back(i);
			}
		}
		if (freqs.empty()) return result;
		if (freqs.size()==1) {
			result.length[symbols[0]]=1;
			build_canonical_table(result);
			return result;
		}
		structure::huffman<int,uint32_t> huff;
		huff.build_length_limited(symbols,freqs,14);
		huff.for_each([&](int s,uint32_t,std::size_t d){
			result.length[s]=static_cast<uint8_t>(d);
		});
		build_canonical_table(result);
		return result;
	}

	static std::pair<uint16_t,uint16_t> get_litlen(std::size_t index) noexcept {
		if (index<=143) return {static_cast<uint16_t>(0b00110000+index),8};
		if (index<=255) return {static_cast<uint16_t>(0b110010000+index-144),9};
		if (index<=279) return {static_cast<uint16_t>(0b0000000+index-256),7};
		if (index<=287) return {static_cast<uint16_t>(0b11000000+index-280),8};
		return {0,0};
	}

	static std::pair<uint16_t,uint16_t> get_dist(std::size_t index) noexcept {
		return {static_cast<uint16_t>(index),5};
	}

	static int get_length_code(int len,int& extra_value,int& extra_bits) noexcept {
		for (int i=0;i<29;i++) {
			int next_base=(i+1<29)?length_codes_[i+1].first:259;
			if (len>=length_codes_[i].first && len<next_base) {
				extra_bits=length_codes_[i].second;
				extra_value=len-length_codes_[i].first;
				return 257+i;
			}
		}
		return 285;
	}

	static int get_dist_code(int dist,int& extra_value,int& extra_bits) noexcept {
		for (int i=0;i<30;i++) {
			int next_base=(i+1<30)?dist_codes_[i+1].first:dist_codes_[i].first*2;
			if (dist>=dist_codes_[i].first && dist<next_base) {
				extra_bits=dist_codes_[i].second;
				extra_value=dist-dist_codes_[i].first;
				return i;
			}
		}
		return 29;
	}

	static void write_dynamic_header(bitwise::bit_writer& bw,const std::vector<uint8_t>& litlen_len,const std::vector<uint8_t>& dist_len) {
		int hlit=286-1;
		while (hlit>=257 && litlen_len[hlit]==0) hlit--;
		int hdist=30-1;
		while (hdist>=1 && dist_len[hdist]==0) hdist--;
		int hlit_count=hlit+1-257;
		int hdist_count=hdist+1-1;
		std::vector<uint8_t> all_lens;
		all_lens.reserve((hlit+1)+(hdist+1));
		all_lens.insert(all_lens.end(),litlen_len.begin(),litlen_len.begin()+hlit+1);
		all_lens.insert(all_lens.end(),dist_len.begin(),dist_len.begin()+hdist+1);
		struct run_entry {
			uint8_t sym_;
			uint8_t extra_bits_;
			uint8_t extra_val_;
		};
		std::array<uint32_t,19> code_freq{};
		std::vector<run_entry> runs;
		runs.reserve(all_lens.size());
		auto flush_run=[&](uint8_t val,int count){
			if (val==0) {
				while (count>0) {
					if (count>=11) {
						int n=std::min(count,138);
						count-=n;
						runs.push_back({18,7,static_cast<uint8_t>(n-11)});
						code_freq[18]++;
					} else if (count>=3) {
						int n=std::min(count,10);
						count-=n;
						runs.push_back({17,3,static_cast<uint8_t>(n-3)});
						code_freq[17]++;
					} else {
						runs.push_back({val,0,0});
						code_freq[val]++;
						count--;
					}
				}
			} else {
				runs.push_back({val,0,0});
				code_freq[val]++;
				count--;
				while (count>0) {
					if (count>=3) {
						int n=std::min(count,6);
						runs.push_back({16,2,static_cast<uint8_t>(n-3)});
						code_freq[16]++;
						count-=n;
					} else {
						runs.push_back({val,0,0});
						code_freq[val]++;
						count--;
					}
				}
			}
		};
		
		uint8_t prev=all_lens[0];
		int len_run=1;
		for (std::size_t i=1;i<all_lens.size();i++) {
			if (all_lens[i]==prev) len_run++;
			else {
				flush_run(prev,len_run);
				prev=all_lens[i];
				len_run=1;
			}
		}
		flush_run(prev,len_run);
		canonical_tree cl=build_dynamic_tree(code_freq.data(),19);
		int hclen=19-1;
		while (hclen>=4 && cl.length[order_[hclen]]==0) hclen--;
		int hclen_count=hclen+1-4;
		bw.write_bits<uint8_t>(5,hlit_count);
		bw.write_bits<uint8_t>(5,hdist_count);
		bw.write_bits<uint8_t>(4,hclen_count);
		for (int i=0;i<hclen+1;i++) bw.write_bits<uint8_t>(3,cl.length[order_[i]]);
		for (const auto& it:runs) {
			bw.write_bits<uint16_t>(cl.length[it.sym_],cl.table[it.sym_]);
			if (it.sym_>=16) bw.write_bits<uint8_t>(it.extra_bits_,it.extra_val_);
		}
	}

	static void encode_stored_blocks(bitwise::bit_writer& bw,const uint8_t* data,std::size_t size,bool is_last_overall) {
		std::size_t pos=0;
		while (pos<size) {
			std::size_t blk=std::min<std::size_t>(65535u,size-pos);
			bool last=(pos+blk==size) && is_last_overall;
			bw.write_bits<uint8_t>(1,last?1:0);
			bw.write_bits<uint8_t>(2,0);
			bw.flush_bits();
			auto len=static_cast<uint16_t>(blk);
			auto nlen=static_cast<uint16_t>(~len);
			bw.write_u16(len);
			bw.write_u16(nlen);
			for (std::size_t i=0;i<blk;i++) bw.write_u8(data[pos+i]);
			pos+=blk;
		}
	}

	static void encode_fixed_block(bitwise::bit_writer& bw,const std::vector<lz77::token>& tokens,bool last) {
		bw.write_bits<uint8_t>(1,last?1:0);
		bw.write_bits<uint8_t>(2,1);
		for (const auto& it:tokens) {
			if (it.length==0) {
				auto [code,len]=get_litlen(it.literal);
				bw.write_bits<uint16_t>(len,bitwise::reverse_bits(code,len));
			} else {
				int ev=0,eb=0;
				int lc=get_length_code(it.length,ev,eb);
				auto [code,len]=get_litlen(lc);
				bw.write_bits<uint16_t>(len,bitwise::reverse_bits(code,len));
				if (eb>0) bw.write_bits<uint16_t>(eb,static_cast<uint64_t>(ev));
				int dv=0,db=0;
				int dc=get_dist_code(it.distance,dv,db);
				auto [dcode,dlen]=get_dist(dc);
				bw.write_bits<uint16_t>(dlen,bitwise::reverse_bits(dcode,dlen));
				if (db>0) bw.write_bits<uint16_t>(db,static_cast<uint64_t>(dv));
			}
		}
		auto [ec,el]=get_litlen(256);
		bw.write_bits<uint16_t>(el,bitwise::reverse_bits(ec,el));
		bw.flush_bits();
	}

	static void encode_dynamic_block(bitwise::bit_writer& bw,const std::vector<lz77::token>& tokens,bool last) {
		bw.write_bits<uint8_t>(1,last?1:0);
		bw.write_bits<uint8_t>(2,2);
		std::array<uint32_t,286> litlen_freq{};
		std::array<uint32_t,30> dist_freq{};
		for (const auto& it:tokens) {
			if (it.length==0) litlen_freq[it.literal]++;
			else {
				int ev=0,eb=0;
				litlen_freq[get_length_code(it.length,ev,eb)]++;
				int dv=0,db=0;
				dist_freq[get_dist_code(it.distance,dv,db)]++;
			}
		}
		litlen_freq[256]=1;
		if (std::all_of(dist_freq.begin(),dist_freq.end(),[](uint32_t f){ return f==0; })) dist_freq[0]=1;
		canonical_tree ll=build_dynamic_tree(litlen_freq.data(),286);
		canonical_tree dt=build_dynamic_tree(dist_freq.data(),30);
		write_dynamic_header(bw,ll.length,dt.length);
		for (const auto& it:tokens) {
			if (it.length==0) bw.write_bits(ll.length[it.literal],ll.table[it.literal]);
			else {
				int ev=0,eb=0;
				int lc=get_length_code(it.length,ev,eb);
				bw.write_bits(ll.length[lc],ll.table[lc]);
				if (eb>0) bw.write_bits<uint16_t>(eb,static_cast<uint64_t>(ev));
				int dv=0,db=0;
				int dc=get_dist_code(it.distance,dv,db);
				bw.write_bits(dt.length[dc],dt.table[dc]);
				if (db>0) bw.write_bits<uint16_t>(db,static_cast<uint64_t>(dv));
			}
		}
		bw.write_bits(ll.length[256],ll.table[256]);
		bw.flush_bits();
	}

	static bool decode_block(bitwise::bit_reader& br,std::vector<uint8_t>& out,const uint8_t* raw_buf,std::size_t raw_size,std::size_t base_offset) {
		bool last=br.read_bits<uint32_t>(1)!=0;
		int btype=br.read_bits<uint32_t>(2);
		if (btype==0) {
			br.byte_align();
			std::size_t byte_pos=base_offset+(br.bit_pos()/8);
			if (byte_pos+4>raw_size) throw std::out_of_range("stored block overflow");
			uint16_t len=static_cast<uint16_t>(raw_buf[byte_pos])|static_cast<uint16_t>(raw_buf[byte_pos+1]<<8);
			uint16_t nlen=static_cast<uint16_t>(raw_buf[byte_pos+2])|static_cast<uint16_t>(raw_buf[byte_pos+3]<<8);
			if (static_cast<uint16_t>(len^0xFFFF)!=nlen) throw std::runtime_error("stored block nlen mismatch");
			byte_pos+=4;
			if (byte_pos+len>raw_size) throw std::out_of_range("stored block data overflow");
			out.insert(out.end(),raw_buf+byte_pos,raw_buf+byte_pos+len);
			br.bit_pos()=(byte_pos+len-base_offset)*8;
			return last;
		}
		canonical_tree litlen,dist;
		if (btype==1) {
			litlen=build_fixed_lit();
			dist=build_fixed_dist();
		} else if (btype==2) {
			int hlit=br.read_bits<uint32_t>(5)+257;
			int hdist=br.read_bits<uint32_t>(5)+1;
			int hclen=br.read_bits<uint32_t>(4)+4;
			std::vector<uint8_t> code_len(19,0);
			for (int i=0;i<hclen;i++) code_len[order_[i]]=br.read_bits<uint32_t>(3);
			canonical_tree cl;
			cl.length=code_len;
			build_canonical_table(cl);
			std::vector<uint8_t> ll_len;
			ll_len.reserve(hlit+hdist);
			int total=hlit+hdist;
			while (static_cast<int>(ll_len.size())<total) {
				int sym=cl.decode(br);
				if (sym<=15) ll_len.push_back(static_cast<uint8_t>(sym));
				else if (sym==16) {
					int rep=3+br.read_bits<uint32_t>(2);
					uint8_t prev=ll_len.empty()?0:ll_len.back();
					while (rep--) ll_len.push_back(prev);
				} else if (sym==17) {
					int rep=3+br.read_bits<uint32_t>(3);
					while (rep--) ll_len.push_back(0);
				} else if (sym==18) {
					int rep=11+br.read_bits<uint32_t>(7);
					while (rep--) ll_len.push_back(0);
				}
			}
			if (static_cast<int>(ll_len.size())!=total) throw std::runtime_error("bad dynamic code length sequence");
			litlen.length.assign(ll_len.begin(),ll_len.begin()+hlit);
			dist.length.assign(ll_len.begin()+hlit,ll_len.end());
			build_canonical_table(litlen);
			build_canonical_table(dist);
		} else throw std::runtime_error("reserved BTYPE");
		while (true) {
			int sym=litlen.decode(br);
			if (sym<256) {
				out.push_back(static_cast<uint8_t>(sym));
				continue;
			}
			if (sym==256) {
				br.byte_align();
				break;
			}
			if (sym>285) throw std::runtime_error("bad length symbol");
			int mlen=length_codes_[sym-257].first;
			if (length_codes_[sym-257].second) mlen+=br.read_bits<uint32_t>(length_codes_[sym-257].second);
			int dsym=dist.decode(br);
			if (dsym>29) throw std::runtime_error("bad distance symbol");
			int mdist=dist_codes_[dsym].first;
			if (dist_codes_[dsym].second) mdist+=br.read_bits<uint32_t>(dist_codes_[dsym].second);
			if (static_cast<std::size_t>(mdist)>out.size()) throw std::out_of_range("distance too far back");
			std::size_t start=out.size()-mdist;
			for (int i=0;i<mlen;i++) out.push_back(out[start+(i%mdist)]);
		}
		return last;
	}

	static void write_gzip_header(bitwise::bit_writer& bw,const gzip_info& info,deflate_level level) {
		bw.write_u8(0x1F);
		bw.write_u8(0x8B);
		bw.write_u8(0x08); // CM=8 deflate
		uint8_t flg=0;
		if (info.is_text) flg|=0x01;
		if (info.has_crc16) flg|=0x02;
		if (!info.filename.empty()) flg|=0x08;
		if (!info.comment.empty()) flg|=0x10;
		bw.write_u8(flg);
		// MTIME little-endian
		bw.write_u8(static_cast<uint8_t>(info.mtime&0xFF));
		bw.write_u8(static_cast<uint8_t>((info.mtime>>8)&0xFF));
		bw.write_u8(static_cast<uint8_t>((info.mtime>>16)&0xFF));
		bw.write_u8(static_cast<uint8_t>((info.mtime>>24)&0xFF));
		uint8_t xfl=0;
		if (level==DL_BEST) xfl=0x02;
		else if (level==DL_FASTEST) xfl=0x04;
		bw.write_u8(xfl);
		bw.write_u8(info.os);
		if (!info.filename.empty()) {
			for (char c:info.filename) bw.write_u8(static_cast<uint8_t>(c));
			bw.write_u8(0);
		}
		if (!info.comment.empty()) {
			for (char c:info.comment) bw.write_u8(static_cast<uint8_t>(c));
			bw.write_u8(0);
		}
	}

	static std::size_t read_gzip_header(const uint8_t* data,std::size_t size,gzip_info& info) {
		if (size<10) throw std::runtime_error("gzip stream too short");
		if (data[0]!=0x1F || data[1]!=0x8B) throw std::invalid_argument("not a gzip stream");
		if (data[2]!=8) throw std::invalid_argument("unsupported gzip CM");
		uint8_t flg=data[3];
		info.mtime=static_cast<uint32_t>(data[4])|(static_cast<uint32_t>(data[5])<<8)|(static_cast<uint32_t>(data[6])<<16)|(static_cast<uint32_t>(data[7])<<24);
		info.os=data[9];
		info.is_text=(flg&0x01)!=0;
		info.has_crc16=(flg&0x02)!=0;
		std::size_t offset=10;
		if (flg&0x04) { // FEXTRA
			if (offset+2>size) throw std::out_of_range("gzip FEXTRA truncated");
			uint16_t xlen=static_cast<uint16_t>(data[offset])|(static_cast<uint16_t>(data[offset+1])<<8);
			offset+=2;
			if (offset+xlen>size) throw std::out_of_range("gzip FEXTRA data truncated");
			offset+=xlen;
		}
		if (flg&0x08) { // FNAME
			while (offset<size && data[offset]!=0) {
				info.filename.push_back(static_cast<char>(data[offset]));
				offset++;
			}
			if (offset<size) offset++;
		}
		if (flg&0x10) { // FCOMMENT
			while (offset<size && data[offset]!=0) {
				info.comment.push_back(static_cast<char>(data[offset]));
				offset++;
			}
			if (offset<size) offset++;
		}
		if (flg&0x02) { // FHCRC
			if (offset+2>size) throw std::out_of_range("gzip FHCRC truncated");
			offset+=2;
		}
		if (offset>=size) throw std::out_of_range("gzip header overflows data");
		return offset;
	}

	static block_type resolve_block_type(deflate_block_type bt,deflate_level lv) noexcept {
		if (bt!=DBT_AUTOMATIC) return bt;
		if (lv==DL_NONE) return DBT_STORED;
		if (lv<=DL_FAST) return DBT_FIXED;
		return DBT_DYNAMIC;
	}

	friend class deflate_compressor;
	friend class deflate_decompressor;

public:
	static std::size_t compress_bound(std::size_t input_size,deflate_stream_format fmt=DSF_ZLIB) noexcept {
		std::size_t blocks=(input_size+65534)/65535;
		std::size_t result=input_size+blocks*5+1;
		if (fmt==DSF_ZLIB) result+=6;
		if (fmt==DSF_GZIP) result+=18;
		return result;
	}

	static bool is_valid_zlib_header(const uint8_t* data,std::size_t size) noexcept {
		if (size<2) return false;
		uint8_t cmf=data[0],flg=data[1];
		if ((cmf&0x0F)!=8) return false;
		return ((static_cast<unsigned>(cmf)<<8)+flg)%31==0;
	}

	static bool is_valid_zlib_header(const std::vector<uint8_t>& data) noexcept {
		return is_valid_zlib_header(data.data(),data.size());
	}

	static bool is_valid_gzip_header(const uint8_t* data,std::size_t size) noexcept {
		if (size<10) return false;
		return data[0]==0x1F && data[1]==0x8B && data[2]==0x08;
	}

	static bool is_valid_gzip_header(const std::vector<uint8_t>& data) noexcept {
		return is_valid_gzip_header(data.data(),data.size());
	}

	static deflate_stream_format detect_format(const uint8_t* data,std::size_t size) noexcept {
		if (is_valid_gzip_header(data,size)) return DSF_GZIP;
		if (is_valid_zlib_header(data,size)) return DSF_ZLIB;
		return DSF_RAW;
	}

	static stream_format detect_format(const std::vector<uint8_t>& data) noexcept {
		return detect_format(data.data(),data.size());
	}

	static std::vector<uint8_t> compress(const uint8_t* data,std::size_t size,deflate_level level=DL_NORMAL,deflate_stream_format fmt=DSF_ZLIB,deflate_block_type btype=DBT_AUTOMATIC) {
		bitwise::bit_writer bw(bitwise::BO_LSBYTE);
		if (fmt==DSF_ZLIB) {
			bw.write_u8(0x78);
			bw.write_u8(0x9C);
		} else if (fmt==DSF_GZIP) {
			gzip_info info;
			write_gzip_header(bw,info,level);
		}
		block_type effective=resolve_block_type(btype,level);
		if (effective==DBT_STORED) encode_stored_blocks(bw,data,size,true);
		else if (effective==DBT_FIXED) {
			lz77 lz;
			std::vector<uint8_t> tmp(data,data+size);
			auto tokens=lz.encode(tmp);
			encode_fixed_block(bw,tokens,true);
		} else {
			lz77 lz;
			std::vector<uint8_t> tmp(data,data+size);
			auto tokens=lz.encode(tmp);
			encode_dynamic_block(bw,tokens,true);
		}
		if (fmt==DSF_ZLIB) {
			uint32_t ad=integrity::adler32::calculate(data,size);
			bw.write_u8(static_cast<uint8_t>(ad>>24));
			bw.write_u8(static_cast<uint8_t>((ad>>16)&0xFF));
			bw.write_u8(static_cast<uint8_t>((ad>>8)&0xFF));
			bw.write_u8(static_cast<uint8_t>(ad&0xFF));
		} else if (fmt==DSF_GZIP) {
			integrity::crc32 crc_calc;
			crc_calc.update(data,size);
			uint32_t crc_val=crc_calc.checksum();
			uint32_t isize=static_cast<uint32_t>(size&0xFFFFFFFFu);
			bw.write_u8(static_cast<uint8_t>(crc_val&0xFF));
			bw.write_u8(static_cast<uint8_t>((crc_val>>8)&0xFF));
			bw.write_u8(static_cast<uint8_t>((crc_val>>16)&0xFF));
			bw.write_u8(static_cast<uint8_t>((crc_val>>24)&0xFF));
			bw.write_u8(static_cast<uint8_t>(isize&0xFF));
			bw.write_u8(static_cast<uint8_t>((isize>>8)&0xFF));
			bw.write_u8(static_cast<uint8_t>((isize>>16)&0xFF));
			bw.write_u8(static_cast<uint8_t>((isize>>24)&0xFF));
		}
		return bw.buffer();
	}

	static std::vector<uint8_t> compress(const std::vector<uint8_t>& data,deflate_level level=DL_NORMAL,deflate_stream_format fmt=DSF_ZLIB,deflate_block_type btype=DBT_AUTOMATIC) {
		return compress(data.data(),data.size(),level,fmt,btype);
	}

	static std::vector<uint8_t> compress(std::string_view sv,deflate_level level=DL_NORMAL,deflate_stream_format fmt=DSF_ZLIB,deflate_block_type btype=DBT_AUTOMATIC) {
		return compress(reinterpret_cast<const uint8_t*>(sv.data()),sv.size(),level,fmt,btype);
	}

#if __cplusplus>=_STDEX_CPP20_VERSION
	static std::vector<uint8_t> compress(std::span<const uint8_t> data,deflate_level level=DL_NORMAL,deflate_stream_format fmt=DSF_ZLIB,deflate_block_type btype=DBT_AUTOMATIC) {
		return compress(data.data(),data.size(),level,fmt,btype);
	}

	static std::vector<uint8_t> compress(std::span<const std::byte> data,deflate_level level=DL_NORMAL,deflate_stream_format fmt=DSF_ZLIB,deflate_block_type btype=DBT_AUTOMATIC) {
		return compress(reinterpret_cast<const uint8_t*>(data.data()),data.size(),level,fmt,btype);
	}
#endif

	static std::vector<uint8_t> compress(const uint8_t* data,std::size_t size,const gzip_info& info,deflate_level level=DL_NORMAL,deflate_block_type btype=DBT_AUTOMATIC) {
		bitwise::bit_writer bw(bitwise::BO_LSBYTE);
		write_gzip_header(bw,info,level);
		block_type effective=resolve_block_type(btype,level);
		if (effective==DBT_STORED) encode_stored_blocks(bw,data,size,true);
		else {
			lz77 lz;
			auto tokens=lz.encode_chunk(data,size,true);
			if (effective==DBT_FIXED) encode_fixed_block(bw,tokens,true);
			else encode_dynamic_block(bw,tokens,true);
		}
		integrity::crc32 crc_calc;
		crc_calc.update(data,size);
		uint32_t crc_val=crc_calc.checksum();
		uint32_t isize=static_cast<uint32_t>(size&0xFFFFFFFFu);
		bw.write_u8(static_cast<uint8_t>(crc_val&0xFF));
		bw.write_u8(static_cast<uint8_t>((crc_val>>8)&0xFF));
		bw.write_u8(static_cast<uint8_t>((crc_val>>16)&0xFF));
		bw.write_u8(static_cast<uint8_t>((crc_val>>24)&0xFF));
		bw.write_u8(static_cast<uint8_t>(isize&0xFF));
		bw.write_u8(static_cast<uint8_t>((isize>>8)&0xFF));
		bw.write_u8(static_cast<uint8_t>((isize>>16)&0xFF));
		bw.write_u8(static_cast<uint8_t>((isize>>24)&0xFF));
		return bw.buffer();
	}

	static std::vector<uint8_t> compress(const std::vector<uint8_t>& data,const gzip_info& info,deflate_level level=DL_NORMAL,deflate_block_type btype=DBT_AUTOMATIC) {
		return compress(data.data(),data.size(),info,level,btype);
	}

	template <typename _It,typename=std::enable_if_t<is_output_iter<_It,uint8_t>::value>>
	static _It encode_to(_It out,const uint8_t* data,std::size_t size,deflate_level level=DL_NORMAL,deflate_stream_format fmt=DSF_ZLIB,deflate_block_type btype=DBT_AUTOMATIC) {
		auto buf=compress(data,size,level,fmt,btype);
		return std::copy(buf.begin(),buf.end(),out);
	}

	template <typename _It,typename=std::enable_if_t<is_output_iter<_It,uint8_t>::value>>
	static _It encode_to(_It out,const std::vector<uint8_t>& data,deflate_level level=DL_NORMAL,deflate_stream_format fmt=DSF_ZLIB,deflate_block_type btype=DBT_AUTOMATIC) {
		return encode_to(out,data.data(),data.size(),level,fmt,btype);
	}

	template <typename _It,typename=std::enable_if_t<is_output_iter<_It,uint8_t>::value>>
	static _It encode_to(_It out,std::string_view sv,deflate_level level=DL_NORMAL,deflate_stream_format fmt=DSF_ZLIB,deflate_block_type btype=DBT_AUTOMATIC) {
		return encode_to(out,reinterpret_cast<const uint8_t*>(sv.data()),sv.size(),level,fmt,btype);
	}

#if __cplusplus>=_STDEX_CPP20_VERSION
	template <byte_output_iterator _It>
	static _It encode_to(_It out,std::span<const uint8_t> data,deflate_level level=DL_NORMAL,deflate_stream_format fmt=DSF_ZLIB,deflate_block_type btype=DBT_AUTOMATIC) {
		return encode_to(out,data.data(),data.size(),level,fmt,btype);
	}

	template <byte_output_iterator _It>
	static _It encode_to(_It out,std::span<const std::byte> data,deflate_level level=DL_NORMAL,deflate_stream_format fmt=DSF_ZLIB,deflate_block_type btype=DBT_AUTOMATIC) {
		return encode_to(out,reinterpret_cast<const uint8_t*>(data.data()),data.size(),level,fmt,btype);
	}
#endif

	static std::vector<uint8_t> decompress(const uint8_t* data,std::size_t size,deflate_stream_format fmt=DSF_ZLIB) {
		std::size_t offset=0;
		if (fmt==DSF_ZLIB) {
			if (size<2) throw std::runtime_error("zlib stream too short");
			uint8_t cmf=data[0],flg=data[1];
			if ((cmf&0x0F)!=8) throw std::invalid_argument("not a deflate stream");
			if (((static_cast<unsigned>(cmf)<<8)+flg)%31!=0) throw std::runtime_error("zlib FCHECK failed");
			offset=2;
		} else if (fmt==DSF_GZIP) {
			gzip_info info;
			offset=read_gzip_header(data,size,info);
		}
		bitwise::bit_reader br(data+offset,size-offset,bitwise::BO_LSBYTE);
		std::vector<uint8_t> out;
		out.reserve(size*3);
		bool last=false;
		while (!last) last=decode_block(br,out,data,size,offset);
		if (fmt==DSF_ZLIB) {
			if (size<6) throw std::runtime_error("zlib too short for adler32");
			std::size_t ap=size-4;
			uint32_t expect=(static_cast<uint32_t>(data[ap])<<24)|(static_cast<uint32_t>(data[ap+1])<<16)|(static_cast<uint32_t>(data[ap+2])<<8)|static_cast<uint32_t>(data[ap+3]);
			uint32_t actual=integrity::adler32::calculate(out.data(),out.size());
			if (actual!=expect) throw std::runtime_error("adler32 mismatch");
		} else if (fmt==DSF_GZIP) {
			if (size<8) throw std::runtime_error("gzip trailer truncated");
			std::size_t tp=size-8;
			uint32_t expect_crc=static_cast<uint32_t>(data[tp])|(static_cast<uint32_t>(data[tp+1])<<8)|(static_cast<uint32_t>(data[tp+2])<<16)|(static_cast<uint32_t>(data[tp+3])<<24);
			uint32_t expect_size=static_cast<uint32_t>(data[tp+4])|(static_cast<uint32_t>(data[tp+5])<<8)|(static_cast<uint32_t>(data[tp+6])<<16)|(static_cast<uint32_t>(data[tp+7])<<24);
			integrity::crc32 crc_calc;
			crc_calc.update(out.data(),out.size());
			if (crc_calc.checksum()!=expect_crc) throw std::runtime_error("gzip crc32 mismatch");
			if (static_cast<uint32_t>(out.size()&0xFFFFFFFFu)!=expect_size) throw std::runtime_error("gzip isize mismatch");
		}
		return out;
	}

	static std::vector<uint8_t> decompress(const std::vector<uint8_t>& data,deflate_stream_format fmt=DSF_ZLIB) {
		return decompress(data.data(),data.size(),fmt);
	}

	static std::vector<uint8_t> decompress(std::string_view sv,deflate_stream_format fmt=DSF_ZLIB) {
		return decompress(reinterpret_cast<const uint8_t*>(sv.data()),sv.size(),fmt);
	}

#if __cplusplus>=_STDEX_CPP20_VERSION
	static std::vector<uint8_t> decompress(std::span<const uint8_t> data,deflate_stream_format fmt=DSF_ZLIB) {
		return decompress(data.data(),data.size(),fmt);
	}

	static std::vector<uint8_t> decompress(std::span<const std::byte> data,deflate_stream_format fmt=DSF_ZLIB) {
		return decompress(reinterpret_cast<const uint8_t*>(data.data()),data.size(),fmt);
	}
#endif

	static std::vector<uint8_t> decompress(const uint8_t* data,std::size_t size,gzip_info& info) {
		std::size_t offset=read_gzip_header(data,size,info);
		bitwise::bit_reader br(data+offset,size-offset,bitwise::BO_LSBYTE);
		std::vector<uint8_t> out;
		out.reserve(size*3);
		bool last=false;
		while (!last) last=decode_block(br,out,data,size,offset);
		if (size<8) throw std::runtime_error("gzip trailer truncated");
		std::size_t tp=size-8;
		uint32_t expect_crc=static_cast<uint32_t>(data[tp])|(static_cast<uint32_t>(data[tp+1])<<8)|(static_cast<uint32_t>(data[tp+2])<<16)|(static_cast<uint32_t>(data[tp+3])<<24);
		uint32_t expect_size=static_cast<uint32_t>(data[tp+4])|(static_cast<uint32_t>(data[tp+5])<<8)|(static_cast<uint32_t>(data[tp+6])<<16)|(static_cast<uint32_t>(data[tp+7])<<24);
		integrity::crc32 crc_calc;
		crc_calc.update(out.data(),out.size());
		if (crc_calc.checksum()!=expect_crc) throw std::runtime_error("gzip crc32 mismatch");
		if (static_cast<uint32_t>(out.size()&0xFFFFFFFFu)!=expect_size) throw std::runtime_error("gzip isize mismatch");
		return out;
	}

	static std::vector<uint8_t> decompress(const std::vector<uint8_t>& data,gzip_info& info) {
		return decompress(data.data(),data.size(),info);
	}

	template <typename _It,typename=std::enable_if_t<is_output_iter<_It,uint8_t>::value>>
	static _It decode_to(_It out,const uint8_t* data,std::size_t size,deflate_stream_format fmt=DSF_ZLIB) {
		auto buf=decompress(data,size,fmt);
		return std::copy(buf.begin(),buf.end(),out);
	}

	template <typename _It,typename=std::enable_if_t<is_output_iter<_It,uint8_t>::value>>
	static _It decode_to(_It out,const std::vector<uint8_t>& data,deflate_stream_format fmt=DSF_ZLIB) {
		return decode_to(out,data.data(),data.size(),fmt);
	}

	template <typename _It,typename=std::enable_if_t<is_output_iter<_It,uint8_t>::value>>
	static _It decode_to(_It out,std::string_view sv,deflate_stream_format fmt=DSF_ZLIB) {
		return decode_to(out,reinterpret_cast<const uint8_t*>(sv.data()),sv.size(),fmt);
	}

#if __cplusplus>=_STDEX_CPP20_VERSION
	template <byte_output_iterator _It>
	static _It decode_to(_It out,std::span<const uint8_t> data,deflate_stream_format fmt=DSF_ZLIB) {
		return decode_to(out,data.data(),data.size(),fmt);
	}

	template <byte_output_iterator _It>
	static _It decode_to(_It out,std::span<const std::byte> data,deflate_stream_format fmt=DSF_ZLIB) {
		return decode_to(out,reinterpret_cast<const uint8_t*>(data.data()),data.size(),fmt);
	}
#endif
};

class deflate_compressor {
	deflate_level level_;
	deflate_stream_format fmt_;
	deflate_block_type btype_;
	gzip_info gzip_info_;

	bitwise::bit_writer bw_;
	lz77 lz_;

	std::vector<lz77::token> pending_tokens_;
	std::size_t pending_bytes_=0;
	std::size_t block_threshold_=65536;

	integrity::adler32 adler_state_;
	integrity::crc32 crc_state_;
	uint32_t total_input_bytes_=0;

	bool header_written_=false;
	bool finished_=false;

	deflate_block_type effective_btype_;

	void write_header() {
		if (fmt_==DSF_ZLIB) {
			bw_.write_u8(0x78);
			bw_.write_u8(0x9C);
		} else if (fmt_==DSF_GZIP) deflate::write_gzip_header(bw_,gzip_info_,level_);
		header_written_=true;
	}

	void flush_block(bool last) {
		if (pending_tokens_.empty() && !last) return;
		if (effective_btype_==DBT_STORED) deflate::encode_dynamic_block(bw_,pending_tokens_,last);
		else if (effective_btype_==DBT_FIXED) deflate::encode_fixed_block(bw_,pending_tokens_,last);
		else deflate::encode_dynamic_block(bw_,pending_tokens_,last);
		pending_tokens_.clear();
		pending_bytes_=0;
	}

	std::vector<uint8_t> take_output() {
		auto buf=bw_.buffer();
		bw_=bitwise::bit_writer(bitwise::BO_LSBYTE);
		return buf;
	}

public:
	explicit deflate_compressor(deflate_level level=DL_NORMAL,deflate_stream_format fmt=DSF_ZLIB,deflate_block_type btype=DBT_AUTOMATIC,std::size_t block_threshold=65536) : level_(level) , fmt_(fmt) , btype_(btype), bw_(bitwise::BO_LSBYTE) , block_threshold_(block_threshold) , effective_btype_(deflate::resolve_block_type(btype,level)) { }
	explicit deflate_compressor(const gzip_info& info,deflate_level level=DL_NORMAL,deflate_block_type btype=DBT_AUTOMATIC,std::size_t block_threshold=65536) : level_(level) , fmt_(DSF_GZIP) , btype_(btype) , gzip_info_(info) , bw_(bitwise::BO_LSBYTE) , block_threshold_(block_threshold) , effective_btype_(deflate::resolve_block_type(btype,level)) { }

	void set_block_threshold(std::size_t n) noexcept { block_threshold_=n; }
	std::size_t block_threshold() const noexcept { return block_threshold_; }

	bool is_finished() const noexcept { return finished_; }

	std::vector<uint8_t> feed(const uint8_t* data,std::size_t size) {
		if (finished_) throw std::logic_error("already finished");
		if (!header_written_) write_header();
		adler_state_.update(data,size);
		crc_state_.update(data,size);
		total_input_bytes_+=static_cast<uint32_t>(size&0xFFFFFFFFu);
		auto new_tokens=lz_.encode_chunk(data,size,false);
		pending_tokens_.insert(pending_tokens_.end(),new_tokens.begin(),new_tokens.end());
		pending_bytes_+=size;
		std::vector<uint8_t> output;
		while (pending_bytes_>=block_threshold_) {
			flush_block(false);
			auto chunk=take_output();
			output.insert(output.end(),chunk.begin(),chunk.end());
		}
		return output;
	}

	std::vector<uint8_t> feed(const std::vector<uint8_t>& data) {
		return feed(data.data(),data.size());
	}

	std::vector<uint8_t> feed(std::string_view sv) {
		return feed(reinterpret_cast<const uint8_t*>(sv.data()),sv.size());
	}

#if __cplusplus>=_STDEX_CPP20_VERSION
	std::vector<uint8_t> feed(std::span<const uint8_t> data) {
		return feed(data.data(),data.size());
	}
	std::vector<uint8_t> feed(std::span<const std::byte> data) {
		return feed(reinterpret_cast<const uint8_t*>(data.data()),data.size());
	}
#endif

	std::vector<uint8_t> flush() {
		if (finished_) throw std::logic_error("already finished");
		if (!header_written_) write_header();
		if (!pending_tokens_.empty()) flush_block(false);
		return take_output();
	}

	std::vector<uint8_t> finish() {
		if (finished_) throw std::logic_error("already finished");
		if (!header_written_) write_header();
		auto tail_tokens=lz_.encode_chunk(nullptr,0,true);
		pending_tokens_.insert(pending_tokens_.end(),tail_tokens.begin(),tail_tokens.end());
		flush_block(true);
		if (fmt_==DSF_ZLIB) {
			uint32_t ad=adler_state_.checksum();
			bw_.write_u8(static_cast<uint8_t>(ad>>24));
			bw_.write_u8(static_cast<uint8_t>((ad>>16)&0xFF));
			bw_.write_u8(static_cast<uint8_t>((ad>>8)&0xFF));
			bw_.write_u8(static_cast<uint8_t>(ad&0xFF));
		} else if (fmt_==DSF_GZIP) {
			uint32_t crc_val=crc_state_.checksum();
			uint32_t isize=total_input_bytes_;
			bw_.write_u8(static_cast<uint8_t>(crc_val&0xFF));
			bw_.write_u8(static_cast<uint8_t>((crc_val>>8)&0xFF));
			bw_.write_u8(static_cast<uint8_t>((crc_val>>16)&0xFF));
			bw_.write_u8(static_cast<uint8_t>((crc_val>>24)&0xFF));
			bw_.write_u8(static_cast<uint8_t>(isize&0xFF));
			bw_.write_u8(static_cast<uint8_t>((isize>>8)&0xFF));
			bw_.write_u8(static_cast<uint8_t>((isize>>16)&0xFF));
			bw_.write_u8(static_cast<uint8_t>((isize>>24)&0xFF));
		}
		finished_=true;
		return take_output();
	}

	void reset() {
		lz_.reset();
		bw_=bitwise::bit_writer(bitwise::BO_LSBYTE);
		pending_tokens_.clear();
		pending_bytes_=0;
		adler_state_.reset();
		crc_state_.reset();
		total_input_bytes_=0;
		header_written_=false;
		finished_=false;
	}

	void reset(deflate_level level,deflate_stream_format fmt,deflate_block_type btype=DBT_AUTOMATIC) {
		level_=level;
		fmt_=fmt;
		btype_=btype;
		effective_btype_=deflate::resolve_block_type(btype,level);
		reset();
	}
};

class deflate_decompressor {
	deflate_stream_format fmt_;
	std::vector<uint8_t> in_buf_;
	std::vector<uint8_t> out_buf_;

	integrity::adler32 adler_state_;
	integrity::crc32 crc_state_;

	bool header_parsed_=false;
	std::size_t data_offset_=0;
	bool done_=false;
	gzip_info gzip_info_;

	bool try_parse_header() {
		if (fmt_==DSF_RAW) {
			data_offset_=0;
			header_parsed_=true;
			return true;
		}
		if (fmt_==DSF_ZLIB) {
			if (in_buf_.size()<2) return false;
			uint8_t cmf=in_buf_[0],flg=in_buf_[1];
			if ((cmf&0x0F)!=8) throw std::runtime_error("not a deflate stream");
			if (((static_cast<unsigned>(cmf)<<8)+flg)%31!=0) throw std::runtime_error("zlib FCHECK failed");
			data_offset_=2;
			header_parsed_=true;
			return true;
		}
		if (fmt_==DSF_GZIP) {
			if (in_buf_.size()<10) return false;
			try {
				data_offset_=deflate::read_gzip_header(in_buf_.data(),in_buf_.size(),gzip_info_);
				header_parsed_=true;
				return true;
			} catch (const std::exception&) {
				if (in_buf_.size()>1024) throw;
				return false;
			}
		}
		return false;
	}

	void try_decompress() {
		if (!header_parsed_ && !try_parse_header()) return;
		if (done_) return;
		const uint8_t* raw=in_buf_.data();
		std::size_t raw_size=in_buf_.size();
		if (raw_size<=data_offset_+1) return;
		bitwise::bit_reader br(raw+data_offset_,raw_size-data_offset_,bitwise::BO_LSBYTE);
		std::size_t decoded_start=out_buf_.size();
		bool last=false;
		try {
			while (!last) last=deflate::decode_block(br,out_buf_,raw,raw_size,data_offset_);
		} catch (const std::runtime_error&) {
			out_buf_.resize(decoded_start);
			return;
		}
		if (out_buf_.size()>decoded_start) {
			adler_state_.update(out_buf_.data()+decoded_start,out_buf_.size()-decoded_start);
			crc_state_.update(out_buf_.data()+decoded_start,out_buf_.size()-decoded_start);
		}
		if (last) {
			done_=true;
			if (fmt_==DSF_ZLIB) {
				if (in_buf_.size()<6) throw std::runtime_error("zlib too short for adler32");
				std::size_t ap=in_buf_.size()-4;
				uint32_t expect=(static_cast<uint32_t>(in_buf_[ap])<<24)|(static_cast<uint32_t>(in_buf_[ap+1])<<16)|(static_cast<uint32_t>(in_buf_[ap+2])<<8)|static_cast<uint32_t>(in_buf_[ap+3]);
				if (adler_state_.checksum()!=expect) throw std::runtime_error("adler32 mismatch");
			} else if (fmt_==DSF_GZIP) {
				if (in_buf_.size()<8) throw std::runtime_error("gzip trailer truncated");
				std::size_t tp=in_buf_.size()-8;
				uint32_t expect_crc=static_cast<uint32_t>(in_buf_[tp])|(static_cast<uint32_t>(in_buf_[tp+1])<<8)|(static_cast<uint32_t>(in_buf_[tp+2])<<16)|(static_cast<uint32_t>(in_buf_[tp+3])<<24);
				uint32_t expect_isize=static_cast<uint32_t>(in_buf_[tp+4])|(static_cast<uint32_t>(in_buf_[tp+5])<<8)|(static_cast<uint32_t>(in_buf_[tp+6])<<16)|(static_cast<uint32_t>(in_buf_[tp+7])<<24);
				if (crc_state_.checksum()!=expect_crc) throw std::runtime_error("gzip crc32 mismatch");
				if (static_cast<uint32_t>(out_buf_.size()&0xFFFFFFFFu)!=expect_isize) throw std::runtime_error("gzip isize mismatch");
			}
		}
	}

public:
	explicit deflate_decompressor(deflate_stream_format fmt=DSF_ZLIB) : fmt_(fmt) { }

	void feed(const uint8_t* data,std::size_t size) {
		if (done_) throw std::logic_error("stream already done");
		in_buf_.insert(in_buf_.end(),data,data+size);
		try_decompress();
	}

	void feed(const std::vector<uint8_t>& data) {
		feed(data.data(),data.size());
	}

	void feed(std::string_view sv) {
		feed(reinterpret_cast<const uint8_t*>(sv.data()),sv.size());
	}

#if __cplusplus>=_STDEX_CPP20_VERSION
	void feed(std::span<const uint8_t> data) {
		feed(data.data(),data.size());
	}
	void feed(std::span<const std::byte> data) {
		feed(reinterpret_cast<const uint8_t*>(data.data()),data.size());
	}
#endif

	[[nodiscard]]
	std::vector<uint8_t> output() {
		std::vector<uint8_t> result=std::move(out_buf_);
		out_buf_.clear();
		return result;
	}

	[[nodiscard]]
	const std::vector<uint8_t>& peek() const noexcept {
		return out_buf_;
	}

	[[nodiscard]]
	bool done() const noexcept { return done_; }

	[[nodiscard]]
	std::size_t available() const noexcept { return out_buf_.size(); }

	[[nodiscard]]
	const gzip_info& info() const noexcept { return gzip_info_; }

	void reset(deflate_stream_format fmt) {
		fmt_=fmt;
		in_buf_.clear();
		out_buf_.clear();
		adler_state_.reset();
		crc_state_.reset();
		header_parsed_=false;
		data_offset_=0;
		done_=false;
		gzip_info_={};
	}

	void reset() {
		reset(fmt_);
	}
};

inline std::vector<uint8_t> deflate_compress(const std::vector<uint8_t>& data,deflate_level level=DL_NORMAL,deflate_stream_format fmt=DSF_ZLIB,deflate_block_type btype=DBT_AUTOMATIC) {
	return deflate::compress(data,level,fmt,btype);
}

inline std::vector<uint8_t> deflate_compress(const uint8_t* data,std::size_t size,deflate_level level=DL_NORMAL,deflate_stream_format fmt=DSF_ZLIB,deflate_block_type btype=DBT_AUTOMATIC) {
	return deflate::compress(data,size,level,fmt,btype);
}

inline std::vector<uint8_t> deflate_compress(std::string_view sv,deflate_level level=DL_NORMAL,deflate_stream_format fmt=DSF_ZLIB,deflate_block_type btype=DBT_AUTOMATIC) {
	return deflate::compress(sv,level,fmt,btype);
}

inline std::vector<uint8_t> deflate_decompress(const std::vector<uint8_t>& data,deflate_stream_format fmt=DSF_ZLIB) {
	return deflate::decompress(data,fmt);
}

inline std::vector<uint8_t> deflate_decompress(const uint8_t* data,std::size_t size,deflate_stream_format fmt=DSF_ZLIB) {
	return deflate::decompress(data,size,fmt);
}

inline std::vector<uint8_t> deflate_decompress(std::string_view sv,deflate_stream_format fmt=DSF_ZLIB) {
	return deflate::decompress(sv,fmt);
}

}

}

#endif