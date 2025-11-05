//Last Modified At 2025/11/06
//@Version 1.1.0.0
#ifndef _STDEX_TYPE_ZIPPED_H_
#define _STDEX_TYPE_ZIPPED_H_ 1

#include <algorithm>
#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <memory>
#include <numeric>
#include <queue>
#include <stdexcept>
#include <string>
#include <system_error>
#include <vector>

#include "../bitwise/bit_reader.h"//At Least 1.0
#include "../integrity/crc.h"//At Least 1.0

namespace stdex {

namespace type {

namespace zipped {

class deflate_compressor {
private:
	/*struct bit_reader {
		const uint8_t* data_;
		std::size_t size_;
		std::size_t bitpos_;
		bit_reader(const uint8_t* d,std::size_t s) : data_(d) , size_(s) , bitpos_(0) { }
		uint32_t read_bits(int n) {
			uint32_t val=0;
			for (int i=0;i<n;i++) {
				if (bitpos_/8>=size_) break;
				if (data_[bitpos_/8]&(1<<(bitpos_%8))) val|=1u<<i;
				bitpos_++;
			}
			return val;
		}
		void byte_align() { bitpos_=(bitpos_+7)&~7; }
	};*/
	struct huffman {
		std::vector<uint16_t> table_;
		std::vector<uint8_t> length_;
		int maxbits_;
		int decode(bitwise::bit_reader& br) const {
			int code=0,len=1;
			for (;len<=maxbits_;len++) {
				code|=br.read_bits<uint8_t>(1)<<(len-1);
				for (std::size_t i=0;i<length_.size();i++) {
					if (length_[i] == len && table_[i]==code) return (int)i;
				}
			}
			throw std::runtime_error("invalid huffman code");
		}
	};
	static huffman build_fixed_tree_LIT() {
		huffman h;
		h.maxbits_=9;
		h.table_.resize(16,0);
		h.length_.resize(288,0);
		for (int i=0;i<=143;i++) h.length_[i]=8;
		for (int i=144;i<=255;i++) h.length_[i]=9;
		for (int i=256;i<=279;i++) h.length_[i]=7;
		for (int i=280;i<=287;i++) h.length_[i]=8;
		build_canonical_table(h);
		return h;
	}
	static huffman build_fixed_tree_DIST() {
		huffman h;
		h.maxbits_=5;
		h.table_.resize(16,0);
		h.length_.resize(32,5);
		build_canonical_table(h);
		return h;
	}
	static void build_canonical_table(huffman& h) {
		int MAXBITS=0;
		for (uint8_t it:h.length_) {
			if (it>MAXBITS) MAXBITS=it;
		}
		h.maxbits_=MAXBITS;
		std::vector<int> bl_count(MAXBITS+1,0);
		for (uint8_t it:h.length_) {
			if (it) bl_count[it]++;
		}
		std::vector<int> next_code(MAXBITS+1,0);
		int code=0;
		for (int bits=1;bits<=MAXBITS;bits++) {
			code=(code+bl_count[bits-1])<<1;
			next_code[bits]=code;
		}
		h.table_.assign(h.length_.size(),0);
		for (std::size_t n=0;n<h.length_.size();n++) {
			int len=h.length_[n];
			if (len!=0) {
				int val=next_code[len]++;
				int rev=0;
				for (int i=0;i<len;i++) rev=(rev<<1)|((val>>i)&1);
				h.table_[n]=rev;
			}
		}
	}
	static void write_bits(std::vector<uint8_t>& out,uint32_t& bitbuf,int& bitcount,uint32_t val,int bits) {
		bitbuf|=(val<<bitcount);
		bitcount+=bits;
		while (bitcount>=8) {
			out.push_back(bitbuf&0xFF);
			bitbuf>>=8;
			bitcount-=8;
		}
	}
	static void flush_bits(std::vector<uint8_t>& out,uint32_t& bitbuf,int& bitcount) {
		while (bitcount>0) {
			out.push_back(bitbuf&0xFF);
			bitbuf>>=8;
			bitcount-=8;
		}
	}
	static uint32_t compute_adler32(const std::vector<uint8_t>& data) {
		uint32_t a=1,b=0;
		for (uint8_t it:data) {
			a=(a+it)%65521;
			b=(b+a)%65521;
		}
		return (b<<16)|a;
	}

public:
	static std::vector<uint8_t> compress(const std::vector<uint8_t>& data,int level=6,bool raw=false) {
		std::vector<uint8_t> compressed;
		uint8_t cmf=0x78;
		uint8_t flg=0x01;
		if (level>=6) flg=0x9C;
		else if (level>=3) flg=0x5E;
		if (!raw) {
			compressed.push_back(cmf);
			compressed.push_back(flg);
		}
		uint32_t bitbuf=0;
		int bitcount=0;
		write_bits(compressed,bitbuf,bitcount,1,1);
		write_bits(compressed,bitbuf,bitcount,1,2);
		for (uint8_t it:data) {
			uint16_t code;
			int len;
			if (it<=143) {
				code=0x30+it;
				len=8;
			} else {
				code=0x190+(it-144);
				len=9;
			}
			write_bits(compressed,bitbuf,bitcount,code,len);
		}
		write_bits(compressed,bitbuf,bitcount,0x00,7);
		flush_bits(compressed,bitbuf,bitcount);
		if (!raw) {
			uint32_t adler=compute_adler32(data);
			compressed.push_back(static_cast<uint8_t>((adler>>24)&0xFF));
			compressed.push_back(static_cast<uint8_t>((adler>>16)&0xFF));
			compressed.push_back(static_cast<uint8_t>((adler>>8)&0xFF));
			compressed.push_back(static_cast<uint8_t>(adler&0xFF));
		}
		return compressed;
	}
	static std::vector<uint8_t> decompress(const std::vector<uint8_t>& compressed,bool raw=false) {
		std::size_t offset=0;
		if (!raw && compressed.size()<2) throw std::runtime_error("too short");
		if (!raw) {
			uint8_t cmf=compressed[0],flg=compressed[1];
			if ((cmf&0x0F)!=8) throw std::runtime_error("not DEFLATE");
			if (((cmf<<8)+flg)%31!=0) throw std::runtime_error("zlib FCHECK");
			offset=2;
		}
		bitwise::bit_reader br(&compressed[offset],compressed.size()-offset,bitwise::bit_reader::BO_LSB);
		std::vector<uint8_t> decompressed;
		bool last=false;
		while (!last) {
			last=br.read_bits<uint32_t>(1);
			int btype=br.read_bits<uint32_t>(2);
			if (btype==0) {
				br.byte_align();
				std::size_t byte_pos=offset+(br.bitpos()/8);
				if (byte_pos+4>compressed.size()) throw std::runtime_error("stored overflow");
				uint16_t len=(uint16_t)compressed[byte_pos]|((uint16_t)compressed[byte_pos+1]<<8);
				uint16_t nlen=(uint16_t)compressed[byte_pos+2]|((uint16_t)compressed[byte_pos+3]<<8);
				if ((len^0xFFFF)!=nlen) throw std::runtime_error("stored nlen mismatch");
				byte_pos+=4;
				if (byte_pos+len>compressed.size()) throw std::runtime_error("stored beyond");
				decompressed.insert(decompressed.end(),&compressed[byte_pos],&compressed[byte_pos+len]);
				br.bitpos()=(byte_pos+len-offset)*8;
				continue;
			}
			huffman litlen,dist;
			if (btype==1) {
				litlen=build_fixed_tree_LIT();
				dist=build_fixed_tree_DIST();
			} else if (btype==2) {
				int HLIT=br.read_bits<uint32_t>(5)+257;
				int HDIST=br.read_bits<uint32_t>(5)+1;
				int HCLEN=br.read_bits<uint32_t>(4)+4;
				static const int order[19]={16,17,18,0,8,7,9,6,10,5,11,4,12,3,13,2,14,1,15};
				std::vector<uint8_t> code_len(19,0);
				for (int i=0;i<HCLEN;i++) code_len[order[i]]=br.read_bits<uint32_t>(3);		
				huffman code_tree;
				code_tree.length_=code_len;
				build_canonical_table(code_tree);
				std::vector<uint8_t> ll_len;
				int total=HLIT+HDIST;
				while ((int)ll_len.size()<total) {
					int sym=code_tree.decode(br);
					if (sym<=15) ll_len.push_back(sym);
					else if (sym==16) {
						int rep=3+br.read_bits<uint32_t>(2);
						uint8_t prev=ll_len.empty()?0:ll_len.back();
						while (rep--) ll_len.push_back(prev);
					} else if (sym==17) {
						int rep=3+br.read_bits<uint32_t>(3);
						while(rep--) ll_len.push_back(0);
					} else if (sym==18) {
						int rep=11+br.read_bits<uint32_t>(7);
						while(rep--) ll_len.push_back(0);
					}
				}
				litlen.length_.assign(ll_len.begin(),ll_len.begin()+HLIT);
				dist.length_.assign(ll_len.begin()+HLIT,ll_len.end());
				build_canonical_table(litlen);
				build_canonical_table(dist);
			} else throw std::runtime_error("Reserved BTYPE");
			static const int lens[29]={3,4,5,6,7,8,9,10,11,13,15,17,19,23,27,31,35,43,51,59,67,83,99,115,131,163,195,227,258};
			static const int lext[29]={0,0,0,0,0,0,0,0,1,1,1,1,2,2,2,2,3,3,3,3,4,4,4,4,5,5,5,5,0};
			static const int dstbase[30]={1,2,3,4,5,7,9,13,17,25,33,49,65,97,129,193,257,385,513,769,1025,1537,2049,3073,4097,6145,8193,12289,16385,24577};
			static const int dstext[30]={0,0,0,0,1,1,2,2,3,3,4,4,5,5,6,6,7,7,8,8,9,9,10,10,11,11,12,12,13,13};
			while (1) {
				int sym=litlen.decode(br);
				if (sym<256) {
					decompressed.push_back((uint8_t)sym);
					continue;
				}
				if (sym==256) {
					br.byte_align();
					break;
				}
				if (sym>285) throw std::runtime_error("bad length sym");
				int len=lens[sym-257];
				if (lext[sym-257]) len+=br.read_bits<uint32_t>(lext[sym-257]);
				int dist_sym=dist.decode(br);
				if (dist_sym>29) throw std::runtime_error("bad dist sym");
				int distv=dstbase[dist_sym];
				if (dstext[dist_sym]) distv+=br.read_bits<uint32_t>(dstext[dist_sym]);
				if ((std::size_t)distv>decompressed.size()) throw std::runtime_error("dist too far");
				std::size_t start=decompressed.size()-distv;
				for (int i=0;i<len;i++) decompressed.push_back(decompressed[start+i]);
			}
			if (last) break;
		}
		if (raw) return decompressed;
		if (compressed.size()<4) throw std::runtime_error("truncated adler");
		std::size_t adler_pos=compressed.size()-4;
		uint32_t expect=(compressed[adler_pos]<<24)|(compressed[adler_pos+1]<<16)|(compressed[adler_pos+2]<<8)|compressed[adler_pos+3];
		uint32_t actual=compute_adler32(decompressed);
		if (actual!=expect) throw std::runtime_error("adler error");
		return decompressed;
	}
};

class bzip2_compressor {
	static constexpr uint64_t block_magic_=0x314159265359ULL;
	static constexpr uint64_t end_magic_=0x177245385090ULL;
	struct bz2_block_header {
		uint32_t crc_;
		bool randomized_;
		uint32_t original_ptr_;
	};
	bool next_block(bitwise::bit_reader& br,bz2_block_header& header) {
		uint64_t marker=br.read_bits<uint64_t>(48);
		if (marker==end_magic_) return false;
		if (marker!=block_magic_) throw std::runtime_error("Bad block magic");
		header.crc_=read_bits<uint32_t>(32);
		header.randomised_=read_bits<uint8_t>(1);
		header.original_ptr_=read_bits<uint32_t>(24);
		return true;
	}
	static void read_used_bytes(bitwise::bit_reader& br,std::array<bool,256>& in_use) {
		in_use.fill(false);
		std::array<bool,16> in_use_group{};
		for (int i=0;i<16;i++) in_use_group[i]=br.read_bits<uint8_t>(1);
		for (int g=0;g<16;g++) {
			if (in_use_group[g]) {
				for (int i=0;i<16;i++) in_use[g*16+i]=bits.read_bits<uint8_t>(1);
			}
		}
	}
	struct group_table {
		int min_len_=0;
		int max_len_=0;
		std::vector<int> limit_;
		std::vector<int> base_;
		std::vector<int> perm_;
	};
	struct huffman_tables {
		int n_groups_{};
		int n_selectors_{};
		int alpha_size_{};
		std::vector<uint8_t> selectors_;
		std::vector<std::vector<uint8_t>> code_lengths_;
		std::vector<group_table> groups_;
	};

	inline huffman_tables read_tables(bitwise::bit_reader& br,const std::array<bool,256>& in_use) {
		huffman_tables ht;
		ht.n_groups_=br.read_bits<uint32_t>(3);
		if (ht.n_groups_<2 || bits.n_groups_>6) throw std::runtime_error("Invalid nGroups");
		ht.n_selectors_=br.read_bits<uint32_t>(15);
		ht.selectors_.resize(ht.n_selectors_);
		std::vector<uint8_t> mtf_list(ht.n_groups);
		std::iota(mtf_list.begin(),mtf_list.end(),0);
		for (int i=0;i<ht.n_selectors_;i++) {
			int cnt=0;
			while (br.read_bits<uint8_t>(1)) cnt++;
			uint8_t v=mtf_list[cnt++];
			ht.selectors_[i]=v;
			mtf_list.erase(mtf_list.begin()+cnt);
			mtf_list.insert(mtf_list.begin(),v);
		}
		int in_use_count=0;
		for(bool it:in_use) {
			if(it) in_use_count++;
		}
		ht.alpha_size_=in_use_count+2;
		ht.code_lengths.assign(ht.n_groups_,std::vector<uint8_t>(ht.alpha_size_));
		for (int t=0;t<ht.n_groups;t++) {
			int curr=br.read_bits<uint32_t>(5);
			for (int i=0;i<ht.alpha_size;i++) {
				while (br.read_bits<uint8_t>(1)) curr+=br.read_bits<uint8_t>(1)?-1:1;
				ht.code_lengths_[t][i]=static_cast<uint8_t>(curr);
			}
		}
		ht.groups_.resize(ht.n_groups_);
		for (int g=0;g<ht.n_groups_;g++) {
			auto& len=ht.code_lengths_[g];
			auto& G=ht.groups_[g];
			int min_len=32,max_len=0;
			for (uint8_t it:len) {
				if (it>0) {
					min_len=std::min(min_len,(int)it);
					max_len=std::max(max_len,(int)it);
				}
			}
			G.min_len_=min_len;
			G.max_len_=max_len;
			G.limit_.assign(max_len+2,0);
			G.base_.assign(max_len+2,0);
			for (int l=min_len;l<=max_len;l++) {
				for (int i=0;i<ht.alpha_size_;i++) {
					if (len[i]==l) G.perm_.push_back(i);
				}
			}
			int vec=0;
			for (int l=min_len;l<=max_len;l++) {
				int n=std::count(len.begin(),len.end(),l);
				vec=(vec+n)<<1;
				G.limit_[l]=vec-1;
				G.base_[l+1]=vec;
			}
		}
		return ht;
	}
	static int alphabet_size(const std::array<bool,256>& in_use) {
		int n=0;
		for(bool it:in_use) {
			if(it) n++;
		}
		return n+2;
	}
	static int decode_symbol(bitwise::bit_reader& bits,const group_table& H) {
		int length=H.min_len_;
		int code=bits.read_bits<int>(length);
		while (length<=H.max_len_ && code>H.limit_[length]) {
			length++;
			code=(code<<1) | bits.read_bits<uint8_t>(1);
		}
		if (length>H.max_len_) throw std::runtime_error("Huffman decode overflow");
		int index=code-H.base_[length];
		if (index<0 || index>=(int)H.perm_.size()) throw std::runtime_error("Huffman decode index out of range");
		return H.perm_[index];
	}
	static std::vector<uint8_t> decode_huffman_data(bitwise::bit_reader& bits,const huffman_tables& ht,const std::array<bool,256>& in_use) {
		const int alpha_size=alphabet_size(in_use);
		const int group_switch=50;
		const std::size_t buf_limit=100000 * 9;
		std::vector<uint8_t> result;
		result.reserve(buf_limit);
		int sel_index=0,group_pos=0;
		group_table const* H=&ht.groups_[ht.selectors_[0]];
		int run_count=0;
		int repeat_count=0;
		while (1) {
			if (group_pos==0) {
				H=&ht.groups_[ht.selectors_[sel_index++]];
				group_pos=group_switch;
			}
			group_pos--;
			int next_sym=decode_symbol(br,*H);
			if (next_sym==0 || next_sym==1) {
				repeat_count=(repeat_count<<1)|next_sym;
				continue;
			}
			if (repeat_count>0) {
				int reps=repeat_count+1;
				repeat_count=0;
				uint8_t value=result.back();
				result.insert(result.end(),reps,value);
				continue;
			}
			if (next_sym-2==alpha_size-1) break;
			result.push_back(static_cast<uint8_t>(next_sym-2));
		}
		return result;
	}
	static std::vector<uint8_t> inverse_bwt(const std::vector<uint8_t>& last,uint32_t original_ptr) {
		const std::size_t n=last.size();
		std::array<int,256> freq{};
		for (uint8_t it:last) freq[it]++;
		std::array<int,256> cum{};
		int sum=0;
		for (int i=0;i<256;i++) {
			cum[i]=sum;
			sum+=freq[i];
		}
		std::vector<int> next(n);
		std::array<int,256> occ{};
		for (std::size_t i=0;i<n;i++) {
			uint8_t c=last[i];
			next[cum[c]+occ[c]++]=i;
		}
		std::vector<uint8_t> result(n);
		std::size_t j=original_ptr;
		for (std::size_t i=0;i<n;i++) {
			j=next[j];
			result[i]=last[j];
		}
		return result;
	}
	static std::vector<uint8_t> undo_rle2(const std::vector<uint8_t>& data) {
		std::vector<uint8_t> result;
		result.reserve(data.size()*2);
		std::size_t i=0;
		while (i<data.size()) {
			uint8_t v=data[i];
			if (i+4<data.size() && data[i+1]==v && data[i+2]==v && data[i+3]==v) {
				uint8_t count=data[i+4];
				result.insert(result.end(),count+4,v);
				i+=5;
			} else {
				result.push_back(v);
				i++;
			}
		}
		return out;
	}

	/*static void serialize_huffman_tree(const std::shared_ptr<huffman_node>& node,std::vector<bool>& bit_stream) {
		if (node->is_leaf()) {
			bit_stream.push_back(true);
			for (int i=0;i<8;i++) bit_stream.push_back((node->symbol_>>i)&1);
		} else {
			bit_stream.push_back(false);
			serialize_huffman_tree(node->left_,bit_stream);
			serialize_huffman_tree(node->right_,bit_stream);
		}
	}
	static std::shared_ptr<huffman_node> deserialize_huffman_tree(const std::vector<bool>& bit_stream,std::size_t& bit_pos) {
		if (bit_pos>=bit_stream.size()) return nullptr;
		bool is_leaf=bit_stream[bit_pos++];
		if (is_leaf) {
			uint8_t symbol=0;
			for (int i=0;i<8 && bit_pos<bit_stream.size();i++) {
				if (bit_stream[bit_pos++]) symbol|=(1<<i);
			}
			return std::make_shared<huffman_node>(symbol,0);
		} else {
			auto left=deserialize_huffman_tree(bit_stream,bit_pos);
			auto right=deserialize_huffman_tree(bit_stream,bit_pos);
			if (!left || !right) return nullptr;
			return std::make_shared<huffman_node>(left,right);
		}
	}*/
	static std::vector<uint8_t> inverse_bwt(const std::vector<uint8_t>& data) {
		if (data.empty()) return data;
		if (data.size()<4) throw std::runtime_error("Invalid BWT data!");
		std::size_t original_index=(static_cast<std::size_t>(data[data.size()-4])<<24)|(static_cast<std::size_t>(data[data.size()-3])<<16)|(static_cast<std::size_t>(data[data.size()-2])<<8)|static_cast<std::size_t>(data[data.size()-1]);
		std::vector<uint8_t> bwt_data(data.begin(),data.end()-4);
		std::vector<std::pair<uint8_t,std::size_t>> table;
		table.reserve(bwt_data.size());
		for (std::size_t i=0;i<bwt_data.size();i++) table.emplace_back(bwt_data[i],i);
		std::stable_sort(table.begin(),table.end(),[](const std::pair<uint8_t,std::size_t>& a,const std::pair<uint8_t,std::size_t>& b){
			return a.first<b.first;
		});
		std::vector<uint8_t> result;
		result.reserve(bwt_data.size());
		std::size_t current_index=original_index;
		for (std::size_t i=0;i<bwt_data.size();i++) {
			current_index=table[current_index].second;
			result.push_back(bwt_data[current_index]);
		}
		return result;
	}
	static std::vector<uint8_t> inverse_mtf(const std::vector<uint8_t>& data) {
		std::vector<uint8_t> dictionary(256);
		for (int i=0;i<256;i++) dictionary[i]=static_cast<uint8_t>(i);
		std::vector<uint8_t> result;
		result.reserve(data.size());
		for (uint8_t it:data) {
			uint8_t ch=dictionary[it];
			result.push_back(ch);
			for (int i=it;i>0;i--) dictionary[i]=dictionary[i-1];
 			dictionary[0]=ch;
		}
		return result;
	}
	static std::vector<uint8_t> inverse_rle(const std::vector<uint8_t>& data) {
		std::vector<uint8_t> result;
		result.reserve(data.size()*2);
		std::size_t i=0;
		while (i<data.size()) {
			if (i+4<data.size() && data[i]==data[i+1] && data[i]==data[i+2] && data[i]==data[i+3]) {
				uint8_t value=data[i];
				uint8_t count=data[i+4]+4;
				for (int j=0;j<count;j++) result.push_back(value);
				i+=5;
			} else {
				result.push_back(data[i]);
				i++;
			}
		}
		return result;
	}
	static std::vector<uint8_t> apply_bwt(const std::vector<uint8_t>& data) {
		if (data.empty()) return data;
		std::vector<std::vector<uint8_t>> rotations;
		rotations.reserve(data.size());
		for (std::size_t i=0;i<data.size();i++) {
			std::vector<uint8_t> rotation(data.size());
			for (std::size_t j=0;j<data.size();j++) rotation[j]=data[(i+j)%data.size()];
			rotations.push_back(std::move(rotation));
		}
		std::sort(rotations.begin(),rotations.end());
		std::vector<uint8_t> result;
		//result.reserve(data.size());
		//for (const auto& it:rotations) result.push_back(it.back());
		result.reserve(data.size()+4);
		std::size_t original_index=0;
		for (std::size_t i=0;i<rotations.size();i++) {
			result.push_back(rotations[i].back());
			if (rotations[i]==data) original_index=i;
		}
		result.push_back(static_cast<uint8_t>((original_index>>24)&0xFF));
		result.push_back(static_cast<uint8_t>((original_index>>16)&0xFF));
		result.push_back(static_cast<uint8_t>((original_index>>8)&0xFF));
		result.push_back(static_cast<uint8_t>(original_index&0xFF));
		return result;
	}
	static std::vector<uint8_t> apply_mtf(const std::vector<uint8_t>& data) {
		std::vector<uint8_t> dictionary(256);
		for (int i=0;i<256;i++) dictionary[i]=static_cast<uint8_t>(i);
		std::vector<uint8_t> result;
		result.reserve(data.size());
		for (uint8_t it:data) {
			auto jt=std::find(dictionary.begin(),dictionary.end(),it);
			std::size_t index=std::distance(dictionary.begin(),jt);
			result.push_back(static_cast<uint8_t>(index));
			if (index>0) {
				uint8_t temp=dictionary[index];
				for (std::size_t i=index;i>0;i--) dictionary[i]=dictionary[i-1];
				dictionary[0]=temp;
			}
		}
		return result;
	}
	static std::vector<uint8_t> apply_rle(const std::vector<uint8_t>& data) {
		std::vector<uint8_t> result;
		result.reserve(data.size()*2);
		std::size_t i=0;
		while (i<data.size()) {
			uint8_t current=data[i];
			std::size_t count=1;
			while (i+count<data.size() && data[i+count]==current && count<255) count++;
			if (count>=4) {
				result.push_back(current);
				result.push_back(current);
				result.push_back(current);
				result.push_back(current);
				result.push_back(static_cast<uint8_t>(count-4));
				i+=count;
			} else {
				result.push_back(current);
				i++;
			}
		}
		return result;
	}
	static std::vector<uint8_t> huffman_decode(const std::string& bit_stream,const std::shared_ptr<huffman_node>& root,std::size_t data_size) {
		std::vector<uint8_t> result;
		result.reserve(data_size);
		auto current_node=root;
		for (char it:bit_stream) {
			if (it=='0') current_node=current_node->left_;
			else current_node=current_node->right_;
			if (current_node->is_leaf()) {
				result.push_back(static_cast<uint8_t>(current_node->symbol_));
				current_node=root;
				if (result.size()>=data_size) break;
			}
		}
		return result;
	}
	static std::vector<uint8_t> huffman_decode(const std::vector<bool>& bit_stream,const std::shared_ptr<huffman_node>& root,std::size_t data_size) {
		std::vector<uint8_t> result;
		result.reserve(data_size);
		auto current_node=root;
		for (std::size_t i=0;i<bit_stream.size() && result.size()<data_size;i++) {
			if (bit_stream[i]) current_node=current_node->right_;
			else current_node=current_node->left_;
			if (current_node->is_leaf()) {
				result.push_back(static_cast<uint8_t>(current_node->symbol_));
				current_node=root;
			}
		}
		return result;
	}
	static std::vector<uint8_t> bits_to_bytes(const std::vector<bool>& bit_stream) {
		std::vector<uint8_t> bytes;
		bytes.reserve((bit_stream.size()+7)/8);
		for (std::size_t i=0;i<bit_stream.size();i+=8) {
			uint8_t byte=0;
			for (int j=0;j<8 && i+j<bit_stream.size();j++) {
				if (bit_stream[i+j]) byte|=(1<<j);
			}
			bytes.push_back(byte);
		}
		return bytes;
	}
	static std::vector<bool> bytes_to_bits(const std::vector<uint8_t>& bytes,std::size_t bit_count) {
		std::vector<bool> bit_stream;
		bit_stream.reserve(bit_count);
		for (std::size_t i=0;i<bytes.size() && bit_stream.size()<bit_count;i++) {
			for (int j=0;j<8 && bit_stream.size()<bit_count;j++) bit_stream.push_back((bytes[i]>>j)&1);
		}
		return bit_stream;
	}

public:
	static std::vector<uint8_t> compress(const std::vector<uint8_t>& data,int level=6) {
		if (data.empty()) return {};
		std::vector<uint8_t> compressed;
		compressed.push_back('B');
		compressed.push_back('Z');
		compressed.push_back('h');
		compressed.push_back(static_cast<uint8_t>('0'+std::min(9,std::max(1,level))));
		std::vector<uint8_t> transformed=data;
		transformed=apply_bwt(transformed);
		transformed=apply_mtf(transformed);
		transformed=apply_rle(transformed);
		std::vector<int> frequencies(256,0);
		for (uint8_t it:transformed) frequencies[it]++;
		auto huffman_tree=build_huffman_tree(frequencies);
		std::vector<std::string> huffman_codes(256);
		build_huffman_codes(huffman_tree,"",huffman_codes);
		std::vector<bool> tree_bit_stream;
		serialize_huffman_tree(huffman_tree,tree_bit_stream);
		std::vector<bool> data_bit_stream;
		for (uint8_t it:transformed) {
			const std::string& code=huffman_codes[it];
			for (char jt:code) data_bit_stream.push_back(jt=='1');
		}
		std::vector<bool> complete_bit_stream;
		uint32_t tree_bit_count=tree_bit_stream.size();
		for (int i=0;i<32;i++) complete_bit_stream.push_back((tree_bit_count>>i)&1);
		complete_bit_stream.insert(complete_bit_stream.end(),tree_bit_stream.begin(),tree_bit_stream.end());
		uint32_t data_bit_count=data_bit_stream.size();
		for (int i=0;i<32;i++) complete_bit_stream.push_back((data_bit_count>>i)&1);
		complete_bit_stream.insert(complete_bit_stream.end(),data_bit_stream.begin(),data_bit_stream.end());
		std::vector<uint8_t> compressed_data=bits_to_bytes(complete_bit_stream);
		compressed.insert(compressed.end(),compressed_data.begin(),compressed_data.end());
		uint32_t crc=integrity::crc32::calculate(data);
		for (int i=3;i>=0;i--) compressed.push_back((crc>>(i*8))&0xFF);
		uint32_t original_size=(uint32_t)data.size();
		for (int i=3;i>=0;i--) compressed.push_back((original_size>>(i*8))&0xFF);
		return compressed;
	}
	static std::vector<uint8_t> decompress(const std::vector<uint8_t>& compressed) {
		bitwise::bit_reader br(compressed,bitwise::bit_reader::BO_MSB);
		char header[4];
		if (compressed.size()<4) throw std::runtime_error("Invalid BZIP2 header");
		header[0]=br.read_u8();
		header[1]=br.read_u8();
		header[2]=br.read_u8();
		header[3]=br.read_u8();
		if (header[0]!='B' || header[1]!='Z' || header[2]!='h' || header[3]<'1' || header[3]>'9') throw std::runtime_error("Invalid BZIP2 header");
		//int level=hdr[3]-'0';
		std::vector<uint8_t> result;
		uint32_t stream_crc=0;
		while (1) {
			bz2_block_header blk{};
			if (!next_block(br,blk)) break;
			std::array<bool,256> in_use;
			read_used_bytes(br,in_use);
			huffman_tables hts=read_huffman_tables(br,in_use);
			auto last_column=decode_huffman_data(br,hts,in_use);
			auto bwt_undo=inverse_bwt(last_column,blk.original_ptr_);
			auto expanded =undo_rle2(bwt_undo);
			uint32_t computed_crc=integrity::crc32.calculate(expanded);
			if (computed_crc!=blk.crc_) throw std::runtime_error("Block CRC mismatch");
			stream_crc=((stream_crc<<1)|(stream_crc>>31))^computed_crc;//stream_crc^=computed_crc;
			result.insert(result.end(),expanded.begin(),expanded.end());
		}
		uint32_t stored_stream_crc=br.read_u32();
		if (stored_stream_crc!=stream_crc) throw std::runtime_error("Stream CRC mismatch");
		return result;
	}
};

#pragma pack(push,1)
struct local_file_header {
	uint32_t signature_;
	uint16_t version_needed_;
	uint16_t flags_;
	uint16_t compression_method_;
	uint16_t last_mod_time_;
	uint16_t last_mod_date_;
	uint32_t crc32_;
	uint32_t compressed_size_;
	uint32_t uncompressed_size_;
	uint16_t file_name_length_;
	uint16_t extra_field_length_;
	static constexpr uint32_t expected_signature_=0x04034B50;
	bool valid() const noexcept {
		return signature_==expected_signature_;
	}
};

struct central_directory_header {
	uint32_t signature_;
	uint16_t version_made_by_;
	uint16_t version_needed_;
	uint16_t flags_;
	uint16_t compression_method_;
	uint16_t last_mod_time_;
	uint16_t last_mod_date_;
	uint32_t crc32_;
	uint32_t compressed_size_;
	uint32_t uncompressed_size_;
	uint16_t file_name_length_;
	uint16_t extra_field_length_;
	uint16_t file_comment_length_;
	uint16_t disk_number_start_;
	uint16_t internal_attributes_;
	uint32_t external_attributes_;
	uint32_t local_header_offset_;
	static constexpr uint32_t expected_signature_=0x02014B50;
	bool valid() const noexcept {
		return signature_==expected_signature_;
	}
};

struct end_of_central_directory {
	uint32_t signature_;
	uint16_t disk_number_;
	uint16_t disk_start_;
	uint16_t num_entries_on_disk_;
	uint16_t total_entries_;
	uint32_t central_dir_size_;
	uint32_t central_dir_offset_;
	uint16_t comment_length_;
	static constexpr uint32_t expected_signature_=0x06054B50;
	bool valid() const noexcept {
		return signature_==expected_signature_;
	}
};

struct data_descriptor {
	uint32_t signature_;
	uint32_t crc32_;
	uint32_t compressed_size_;
	uint32_t uncompressed_size_;
	static constexpr uint32_t expected_signature_=0x08074B50;
};

struct zip64_end_of_central_directory {
	uint32_t signature_;
	uint64_t size_of_record_;
	uint16_t version_made_by_;
	uint16_t version_needed_;
	uint32_t disk_number_;
	uint32_t disk_number_start_;
	uint64_t num_entries_on_disk_;
	uint64_t total_entries_;
	uint64_t central_dir_size_;
	uint64_t central_dir_offset_;
	static constexpr uint32_t expected_signature_=0x06064B50;
	bool valid() const noexcept { return signature_==expected_signature_; }
};

struct zip64_end_of_central_directory_locator {
	uint32_t signature_;
	uint32_t disk_with_eocd_;
	uint64_t offset_of_eocd_;
	uint32_t total_disks_;
	static constexpr uint32_t expected_signature_=0x07064B50;
	bool valid() const noexcept { return signature_==expected_signature_; }
};
#pragma pack(pop)

enum compression_method : uint16_t {
	CM_STORED=0,
	//CM_SHRUNK=1,
	//CM_REDUCED_1=2,
	//CM_REDUCED_2=3,
	//CM_REDUCED_3=4,
	//CM_REDUCED_4=5,
	//CM_IMPLODED=6,
	//CM_RESERVED_FOR_TOKENIZING=7,
	CM_DEFLATED=8,
	//CM_ENHANCED_DEFLATED=9,
	//CM_PKWARE_DCL_IMPLODED=10,
	CM_BZIP2=12,
	//CM_LZMA=14,
	//CM_IBM_TERSE=18,
	//CM_IBM_LZ77=19,
	//CM_ZSTD=93,
	//CM_XZ=95,
	//CM_JPEG_RECOMPRESSION=96,
	//CM_WAV_PACK=97,
	//CM_PPMD=98,
};

enum compression_level : int {
	CL_NONE=0,
	CL_FAST=1,
	CL_NORMAL=6,
	CL_MAXIMUM=9,
};

class file_info {
public:
	using string_type=std::string;
	using byte_vector=std::vector<uint8_t>;
	using path_type=std::filesystem::path;

private:
	string_type filename_;
	path_type filepath_;
	compression_method method_=CM_STORED;
	uint64_t compressed_size_=0;
	uint64_t uncompressed_size_=0;
	uint32_t crc32_=0;
	uint64_t local_header_offset_=0;
	bool is_directory_=false;
	std::chrono::system_clock::time_point last_write_time_=std::chrono::system_clock::now();
	byte_vector data_;
	std::string comment_;
	uint16_t internal_attributes_=0;
	uint32_t external_attributes_=0;
	byte_vector extra_field_;

	void update_filepath() {
		filepath_=std::filesystem::u8path(filename_);
		is_directory_=!filename_.empty() && (filename_.back()=='/' || filename_.back()=='\\');
	}

public:
	file_info()=default;
	file_info(string_type filename) : filename_(std::move(filename)) {
		update_filepath();
	}

	const string_type& filename() const noexcept { return filename_; }
	const path_type& filepath() const noexcept { return filepath_; }
	compression_method method() const noexcept { return method_; }
	uint64_t compressed_size() const noexcept { return compressed_size_; }
	uint64_t uncompressed_size() const noexcept { return uncompressed_size_; }
	uint32_t crc32() const noexcept { return crc32_; }
	uint64_t local_header_offset() const noexcept { return local_header_offset_; }
	bool is_directory() const noexcept { return is_directory_; }
	const byte_vector& data() const noexcept { return data_; }
	const std::string& comment() const noexcept { return comment_; }
	uint16_t internal_attributes() const noexcept { return internal_attributes_; }
	uint32_t external_attributes() const noexcept { return external_attributes_; }	
	const byte_vector& extra_field() const noexcept { return extra_field_; }
	std::chrono::system_clock::time_point last_write_time() const {
		return last_write_time_;
	}
	void calculate_crc32() {
		crc32_=integrity::crc32::calculate(data_);
	}
	void set_directory(bool d) {
		is_directory_=d;
	}
	void filename(string_type name) { 
		filename_=std::move(name);
		update_filepath();
	}
	
	void method(compression_method m) { method_=m; }
	void compressed_size(uint64_t size) { compressed_size_=size; }
	void uncompressed_size(uint64_t size) { uncompressed_size_=size; }
	void crc32(uint32_t crc) { crc32_=crc; }
	void local_header_offset(uint64_t offset) { local_header_offset_=offset; }
	void last_write_time(std::chrono::system_clock::time_point time) { 
		last_write_time_=time; 
	}
	void data(byte_vector d) { 
		data_=std::move(d);
		uncompressed_size_=data_.size();
	}
	void comment(std::string c) { comment_=std::move(c); }
	void internal_attributes(uint16_t attr) { internal_attributes_=attr; }
	void external_attributes(uint32_t attr) { external_attributes_=attr; }
	void extra_field(byte_vector extra) { extra_field_ = std::move(extra); }

	void set_dos_time(uint16_t time,uint16_t date) {
		int year=((date>>9)&0x7F)+1980;
		int month=(date>>5)&0x0F;
		int day=date&0x1F;
		int hour=(time>>11)&0x1F;
		int minute=(time>>5)&0x3F;
		int second=(time&0x1F)*2;
		std::tm tm={};
		tm.tm_year=year-1900;
		tm.tm_mon=month-1;
		tm.tm_mday=day;
		tm.tm_hour=hour;
		tm.tm_min=minute;
		tm.tm_sec=second;
		auto time_t=std::mktime(&tm);
		last_write_time_=std::chrono::system_clock::from_time_t(time_t);
	}
	std::pair<uint16_t,uint16_t> to_dos_time() const {
		auto time_t=std::chrono::system_clock::to_time_t(last_write_time_);
		std::tm tm=*std::localtime(&time_t);
		uint16_t time=((tm.tm_hour&0x1F)<<11)|((tm.tm_min&0x3F)<<5)|((tm.tm_sec/2)&0x1F);
		uint16_t date=(((tm.tm_year+1900-1980)&0x7F)<<9)|(((tm.tm_mon+1)&0x0F)<<5)|(tm.tm_mday&0x1F);
		return {time,date};
	}
	bool verify() const {
		if (data_.empty()) return true;
		return crc32_==integrity::crc32::calculate(data_);
	}

	void compress_data(compression_level level=CL_NORMAL) {
		if (method_==CM_STORED || data_.empty()) {
			compressed_size_=uncompressed_size_;
			calculate_crc32();
			return;
		}
		uint32_t orcc=integrity::crc32::calculate(data_);
		std::vector<uint8_t> compressed;
		bool compression_successful=false;
		switch (method_) {
			case CM_DEFLATED: {
				compressed=deflate_compressor::compress(data_,static_cast<int>(level),true);
				compression_successful=compressed.size()<data_.size();
				break;
			}
			case CM_BZIP2: {
				compressed=bzip2_compressor::compress(data_,static_cast<int>(level));
				compression_successful=compressed.size()<data_.size();
				break;
			}
			default: {
				method_=CM_STORED;
				compressed_size_=uncompressed_size_;
				crc32_=orcc;
				return;
			}
		}
		crc32_=orcc;
		if (compression_successful) {
			data_=std::move(compressed);
			compressed_size_=data_.size();
		} else {
			method_=CM_STORED;
			compressed_size_=uncompressed_size_;
		}
	}
	void decompress_data() {
		if (method_==CM_STORED || data_.empty()) return;
		switch (method_) {
			case CM_DEFLATED: {
				try {
					data_=deflate_compressor::decompress(data_,true);
				} catch (const std::exception& e) {
					throw std::runtime_error("Deflated decompression failed:"+e.what());
				}
				break;
			}
			case CM_BZIP2: {
				try {
					data_=bzip2_compressor::decompress(data_);
				} catch (const std::exception& e) {
					throw std::runtime_error("BZIP2 decompression failed:"+e.what());
				}
				break;
			}
			default: throw std::runtime_error("Unsupported compression method for decompression!");
		}
		uncompressed_size_=data_.size();
		method_=CM_STORED;
	}
};

class archive {
public:
	using value_type=file_info;
	using reference=value_type&;
	using const_reference=const value_type&;
	using iterator=typename std::vector<value_type>::iterator;
	using const_iterator=typename std::vector<value_type>::const_iterator;
	using size_type=typename std::vector<value_type>::size_type;

private:
	struct central_direction_view {
		uint64_t offset,size,entries;
	};
	
	std::vector<value_type> files_;
	std::vector<uint8_t> data_;
	std::string archive_comment_;
	
	static void append_le16(std::vector<uint8_t>& buf,uint16_t v) {
		buf.push_back(v&0xFF);
		buf.push_back((v>>8)&0xFF);
	}
	static void append_le32(std::vector<uint8_t>& buf,uint32_t v) {
		buf.push_back(v&0xFF);
		buf.push_back((v>>8)&0xFF);
		buf.push_back((v>>16)&0xFF);
		buf.push_back((v>>24)&0xFF);
	}
	static void append_le64(std::vector<uint8_t>& buf,uint64_t v) {
		buf.push_back(v&0xFF);
		buf.push_back((v>>8)&0xFF);
		buf.push_back((v>>16)&0xFF);
		buf.push_back((v>>24)&0xFF);
		buf.push_back((v>>32)&0xFF);
		buf.push_back((v>>40)&0xFF);
		buf.push_back((v>>48)&0xFF);
		buf.push_back((v>>56)&0xFF);
	}

	std::vector<uint8_t> build_zip_data() {
		std::vector<uint8_t> zip_data;
		std::vector<std::pair<std::size_t,uint64_t>> local_header_offsets;
		for (std::size_t i=0;i<files_.size();i++) {
			auto& file=files_[i];
			uint64_t local_header_offset=zip_data.size();
			local_header_offsets.emplace_back(i,local_header_offset);
			local_file_header lfh{};
			lfh.signature_=local_file_header::expected_signature_;
			lfh.version_needed_=45;
			lfh.flags_=0;
			if (!std::all_of(file.filename().begin(),file.filename().end(),[](unsigned char c){ return c < 128; })) lfh.flags_|=(1<<11);
			lfh.compression_method_=static_cast<uint16_t>(file.method());
			if (file.method()==CM_DEFLATED && !file.data().empty()) {
				if (file.data().size()==file.uncompressed_size()) lfh.compression_method_=CM_STORED;
			}
			auto [dos_time,dos_date]=file.to_dos_time();
			lfh.last_mod_time_=dos_time;
			lfh.last_mod_date_=dos_date;
			lfh.crc32_=file.crc32();
			std::vector<uint8_t> extra=file.extra_field();
			bool use_zip64=false;
			if (file.compressed_size()>=0xFFFFFFFFULL || file.uncompressed_size()>=0xFFFFFFFFULL || local_header_offset>=0xFFFFFFFFULL) {
				use_zip64=true;
				append_le16(extra,0x0001);
				std::size_t data_len=16;
				if (local_header_offset>=0xFFFFFFFFULL) data_len+=8;
				append_le16(extra,static_cast<uint16_t>(data_len));
				append_le64(extra,file.uncompressed_size());
				append_le64(extra,file.compressed_size());
				if (local_header_offset>=0xFFFFFFFFULL) append_le64(extra,local_header_offset);
				lfh.compressed_size_=0xFFFFFFFF;
				lfh.uncompressed_size_=0xFFFFFFFF;
			} else {
				lfh.compressed_size_=static_cast<uint32_t>(file.compressed_size());
				lfh.uncompressed_size_=static_cast<uint32_t>(file.uncompressed_size());
			}
			lfh.extra_field_length_=static_cast<uint16_t>(extra.size());
			lfh.file_name_length_=static_cast<uint16_t>(file.filename().size());
			const uint8_t* lfh_ptr=reinterpret_cast<const uint8_t*>(&lfh);
			zip_data.insert(zip_data.end(),lfh_ptr,lfh_ptr+sizeof(lfh));
			const uint8_t* filename_ptr=reinterpret_cast<const uint8_t*>(file.filename().data());
			zip_data.insert(zip_data.end(),filename_ptr,filename_ptr+file.filename().size());
			zip_data.insert(zip_data.end(),extra.begin(),extra.end());
			if (!file.is_directory()) zip_data.insert(zip_data.end(), file.data().begin(), file.data().end());
		}
		uint64_t central_dir_start=zip_data.size();
		for (const auto& [index,local_offset]:local_header_offsets) {
			const auto& file=files_[index];
			central_directory_header cdh{};
			cdh.signature_=central_directory_header::expected_signature_;
			cdh.version_made_by_=45;
			cdh.version_needed_=45;
			cdh.flags_=0;
			if (!std::all_of(file.filename().begin(),file.filename().end(),[](unsigned char c){ return c < 128; })) cdh.flags_|=(1<<11);
			cdh.compression_method_=static_cast<uint16_t>(file.method());
			auto [dos_time,dos_date]=file.to_dos_time();
			cdh.last_mod_time_=dos_time;
			cdh.last_mod_date_=dos_date;
			cdh.crc32_=file.crc32();
			cdh.file_name_length_=static_cast<uint16_t>(file.filename().size());
			std::vector<uint8_t> extra = file.extra_field();
			if (file.compressed_size()>=0xFFFFFFFFULL || file.uncompressed_size()>=0xFFFFFFFFULL || local_offset>=0xFFFFFFFFULL) {
				append_le16(extra,0x0001);
				std::size_t data_len=16;
				if (local_offset>=0xFFFFFFFFULL) data_len+=8;
				append_le16(extra,static_cast<uint16_t>(data_len));
				append_le64(extra,file.uncompressed_size());
				append_le64(extra,file.compressed_size());
				if (local_offset>=0xFFFFFFFFULL) append_le64(extra,local_offset);
				cdh.uncompressed_size_=0xFFFFFFFF;
				cdh.compressed_size_=0xFFFFFFFF;
				cdh.local_header_offset_=0xFFFFFFFF;
			} else {
				cdh.uncompressed_size_=static_cast<uint32_t>(file.uncompressed_size());
				cdh.compressed_size_=static_cast<uint32_t>(file.compressed_size());
				cdh.local_header_offset_=static_cast<uint32_t>(local_offset);
			}
			cdh.extra_field_length_ = static_cast<uint16_t>(extra.size());
			cdh.file_comment_length_=static_cast<uint16_t>(file.comment().size());
			cdh.disk_number_start_=0;
			cdh.internal_attributes_=file.internal_attributes();
			cdh.external_attributes_=file.external_attributes();
			const uint8_t* cdh_ptr=reinterpret_cast<const uint8_t*>(&cdh);
			zip_data.insert(zip_data.end(),cdh_ptr,cdh_ptr+sizeof(cdh));
			const uint8_t* filename_ptr=reinterpret_cast<const uint8_t*>(file.filename().data());
			zip_data.insert(zip_data.end(),filename_ptr,filename_ptr+file.filename().size());
			zip_data.insert(zip_data.end(),file.extra_field().begin(),file.extra_field().end());
			if (!file.comment().empty()) {
				const uint8_t* comment_ptr=reinterpret_cast<const uint8_t*>(file.comment().data());
				zip_data.insert(zip_data.end(),comment_ptr,comment_ptr+file.comment().size());
			}
		}
		uint64_t central_dir_size=zip_data.size()-central_dir_start;
		bool need_zip64=central_dir_start>=0xFFFFFFFFULL || central_dir_size>=0xFFFFFFFFULL || files_.size()>=0xFFFF;
		if (need_zip64) {
			append_le32(zip_data,0x06064B50);
			append_le64(zip_data,44);
			append_le16(zip_data,45);
			append_le16(zip_data,45);
			append_le32(zip_data,0); 
			append_le32(zip_data,0);
			append_le64(zip_data,files_.size());
			append_le64(zip_data,files_.size());
			append_le64(zip_data,central_dir_size);
			append_le64(zip_data,central_dir_start);
			append_le32(zip_data,0x07064B50);
			append_le32(zip_data,0);
			append_le64(zip_data,zip_data.size()-central_dir_size-56);
			append_le32(zip_data,1);
		}
		end_of_central_directory eocd{};
		eocd.signature_=end_of_central_directory::expected_signature_;
		eocd.disk_number_=0;
		eocd.disk_start_=0;
		eocd.num_entries_on_disk_=static_cast<uint16_t>(files_.size());
	    eocd.total_entries_=static_cast<uint16_t>(std::min<uint64_t>(files_.size(),0xFFFFULL));
		eocd.central_dir_size_=static_cast<uint32_t>(std::min<uint64_t>(central_dir_size,0xFFFFFFFFULL));
		eocd.central_dir_offset_=static_cast<uint32_t>(std::min<uint64_t>(central_dir_start,0xFFFFFFFFULL));
		eocd.comment_length_=static_cast<uint16_t>(archive_comment_.size());
		const uint8_t* eocd_ptr=reinterpret_cast<const uint8_t*>(&eocd);
		zip_data.insert(zip_data.end(),eocd_ptr,eocd_ptr+sizeof(eocd));
		if (!archive_comment_.empty()) {
			const uint8_t* comment_ptr=reinterpret_cast<const uint8_t*>(archive_comment_.data());
			zip_data.insert(zip_data.end(),comment_ptr,comment_ptr+archive_comment_.size());
		}
		return zip_data;
	}
	static uint16_t read_le16(const uint8_t* p) {
		return (uint16_t)p[0]|((uint16_t)p[1]<<8);
	}
	static uint32_t read_le32(const uint8_t* p) {
		return (uint32_t)p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);
	}
	static uint64_t read_le64(const uint8_t* p) {
		return (uint64_t)p[0]|((uint64_t)p[1]<<8)|((uint64_t)p[2]<<16)|((uint64_t)p[3]<<24)|((uint64_t)p[4]<<32)|((uint64_t)p[5]<<40)|((uint64_t)p[6]<<48)|((uint64_t)p[7]<<56);
	}
	bool parse_central_directory(const char* data,std::size_t size,const end_of_central_directory* eocd) {
		data_.assign(reinterpret_cast<const uint8_t*>(data),reinterpret_cast<const uint8_t*>(data)+size);
		if (eocd->central_dir_offset_>=size) return false;
		const char* cd_start=data+eocd->central_dir_offset_;
		const char* cd_end=cd_start+eocd->central_dir_size_;
		if (cd_end>data+size) return false;
		const char* ptr=cd_start;
		for (uint16_t i=0;i<eocd->total_entries_ && ptr+sizeof(central_directory_header)<=cd_end;i++) {
			const auto* cdh=reinterpret_cast<const central_directory_header*>(ptr);
			if (!cdh->valid()) return false;
			ptr+=sizeof(central_directory_header);
			if (ptr+cdh->file_name_length_>cd_end) return false;
			file_info file(std::string(ptr,cdh->file_name_length_));
			ptr+=cdh->file_name_length_;
			if (ptr+cdh->extra_field_length_>cd_end) return false;
			file.extra_field(std::vector<uint8_t>(reinterpret_cast<const uint8_t*>(ptr),reinterpret_cast<const uint8_t*>(ptr)+cdh->extra_field_length_));
			const auto& extra=file.extra_field();
			uint64_t zip64_uncompressed=0;
			uint64_t zip64_compressed=0;
			uint64_t zip64_offset=0;
			std::size_t pos=0;
			while (pos+4<=extra.size()) {
				uint16_t header_id=read_le16(&extra[pos]);
				uint16_t data_size=read_le16(&extra[pos+2]);
				pos+=4;
				if (pos+data_size>extra.size()) break;
				if (header_id==0x0001 && data_size>=8) {
					std::size_t rd=0;
					if (cdh->uncompressed_size_==0xFFFFFFFF && rd+8<=data_size) {
						zip64_uncompressed=read_le64(&extra[pos+rd]);
						rd+=8;
					}
					if (cdh->compressed_size_==0xFFFFFFFF && rd+8<=data_size) {
						zip64_compressed=read_le64(&extra[pos+rd]);
						rd+=8;
					}
					if (cdh->local_header_offset_==0xFFFFFFFF && rd+8<=data_size) zip64_offset=read_le64(&extra[pos+rd]);
				}
				pos+=data_size;
			}
			if (zip64_uncompressed>0) file.uncompressed_size(zip64_uncompressed);
			if (zip64_compressed>0) file.compressed_size(zip64_compressed);
			if (zip64_offset>0) file.local_header_offset(zip64_offset);
			ptr+=cdh->extra_field_length_;
			if (cdh->file_comment_length_>0) {
				if (ptr+cdh->file_comment_length_>cd_end) return false;
				file.comment(std::string(ptr,cdh->file_comment_length_));
				ptr+=cdh->file_comment_length_;
			}
			file.method(static_cast<compression_method>(cdh->compression_method_));
			bool utf8=cdh->flags_&(1<<11);
			if (!utf8) { }
			file.compressed_size(cdh->compressed_size_);
			file.uncompressed_size(cdh->uncompressed_size_);
			file.crc32(cdh->crc32_);
			file.local_header_offset(cdh->local_header_offset_);
			file.set_dos_time(cdh->last_mod_time_,cdh->last_mod_date_);
			file.internal_attributes(cdh->internal_attributes_);
			file.external_attributes(cdh->external_attributes_);
			files_.push_back(std::move(file));
		}
		for (auto& it:files_) {
			if (it.local_header_offset()+sizeof(local_file_header)>=size) continue;
			const local_file_header* lfh=reinterpret_cast<const local_file_header*>(data+it.local_header_offset());
			if (!lfh->valid()) continue;
			const uint8_t* name_start=reinterpret_cast<const uint8_t*>(lfh)+sizeof(local_file_header);
			const uint8_t* extra_start=name_start+lfh->file_name_length_;
			const uint8_t* file_data_start=extra_start + lfh->extra_field_length_;
			uint32_t data_len=lfh->compressed_size_;
			bool has_descriptor=(lfh->flags_&0x08)!=0;
			if (data_len==0) data_len=it.compressed_size();
			if (has_descriptor && data_len==0) {
				std::size_t scan=file_data_start-reinterpret_cast<const uint8_t*>(data);
				std::size_t end_data=size;
				while (scan+4<end_data) {
					uint32_t sig=data[scan]|(data[scan+1]<<8)|(data[scan+2]<<16)|(data[scan+3]<<24);
					if (sig==0x04034b50 || sig==0x02014b50 || sig==0x08074b50) break;
					scan++;
				}
				data_len=static_cast<uint32_t>(scan-(file_data_start-reinterpret_cast<const uint8_t*>(data)));
			}
			if ((reinterpret_cast<const uint8_t*>(data)+size)<(file_data_start+data_len)) continue;
			it.data(std::vector<uint8_t>(file_data_start,file_data_start+data_len));
		}
		return true;
	}
	bool parse_central_directory64(const char* data,std::size_t size,const central_direction_view& cdv) {
		data_.assign(reinterpret_cast<const uint8_t*>(data),reinterpret_cast<const uint8_t*>(data)+size);
		uint64_t offset=cdv.offset;
		uint64_t endpos=offset+cdv.size;
		if (offset>=size) return false;
		if (endpos>size) endpos=size;
		const char* cd_start=data+offset;
		const char* cd_end=data+endpos;
		const char* ptr=cd_start;
		for (uint64_t i=0;i<cdv.entries && ptr+sizeof(central_directory_header)<=cd_end;i++) {
			const auto* cdh=reinterpret_cast<const central_directory_header*>(ptr);
			if (!cdh->valid()) return false;
			ptr+=sizeof(central_directory_header);
			if (ptr+cdh->file_name_length_>cd_end) return false;
			file_info file(std::string(ptr,cdh->file_name_length_));
			ptr+=cdh->file_name_length_;
			if (ptr+cdh->extra_field_length_>cd_end) return false;
			file.extra_field(std::vector<uint8_t>(reinterpret_cast<const uint8_t*>(ptr),reinterpret_cast<const uint8_t*>(ptr)+cdh->extra_field_length_));
			ptr+=cdh->extra_field_length_;
			if (cdh->file_comment_length_>0) {
				if (ptr+cdh->file_comment_length_>cd_end) return false;
				file.comment(std::string(ptr,cdh->file_comment_length_));
				ptr+=cdh->file_comment_length_;
			}
			uint64_t uncomp=cdh->uncompressed_size_;
			uint64_t comp=cdh->compressed_size_;
			uint64_t lhoff=cdh->local_header_offset_;
			if (comp==0xFFFFFFFFu || uncomp==0xFFFFFFFFu || lhoff==0xFFFFFFFFu) {
				const auto& extra=file.extra_field();
				std::size_t pos=0;
				while (pos+4<=extra.size()) {
					uint16_t header_id=read_le16(&extra[pos]);
					uint16_t data_size=read_le16(&extra[pos+2]);
					pos+=4;
					if (pos+data_size>extra.size()) break;
					if (header_id==0x0001) {
						std::size_t rd=0;
						if (uncomp==0xFFFFFFFFu && rd+8<=data_size) {
                        	uncomp=read_le64(&extra[pos+rd]);
							rd+=8;
						}
						if (comp==0xFFFFFFFFu && rd+8<=data_size) {
							comp=read_le64(&extra[pos+rd]);
							rd+=8;
						}
						if (lhoff==0xFFFFFFFFu && rd+8<=data_size) lhoff=read_le64(&extra[pos+rd]);
					}
					pos+=data_size;
				}
			}
			file.method(static_cast<compression_method>(cdh->compression_method_));
			file.compressed_size(comp);
			file.uncompressed_size(uncomp);
			file.local_header_offset(lhoff);
			file.crc32(cdh->crc32_);
			file.set_dos_time(cdh->last_mod_time_,cdh->last_mod_date_);
			files_.push_back(std::move(file));
		}
		for (auto& it:files_) {
			uint64_t lhoff=it.local_header_offset();
			if (lhoff+sizeof(local_file_header)>=size) continue;
			const local_file_header* lfh=reinterpret_cast<const local_file_header*>(data+lhoff);
			if (!lfh->valid()) continue;
			const uint8_t* name_start=reinterpret_cast<const uint8_t*>(lfh)+sizeof(local_file_header);
			const uint8_t* extra_start=name_start+lfh->file_name_length_;
			const uint8_t* file_data_start=extra_start+lfh->extra_field_length_;
			uint64_t data_len=lfh->compressed_size_;
			bool has_descriptor=(lfh->flags_&0x08)!=0;
			if (data_len==0) data_len=it.compressed_size();
			if (has_descriptor && data_len==0) {
				std::size_t scan=file_data_start-reinterpret_cast<const uint8_t*>(data);
				std::size_t end_data=size;
				while (scan+4<end_data) {
					uint32_t sig=data[scan]|(data[scan+1]<<8)|(data[scan+2]<<16)|(data[scan+3]<<24);
					if (sig==0x04034b50 || sig==0x02014b50 || sig==0x08074b50) break;
					scan++;
				}
				data_len=static_cast<uint64_t>(scan-(file_data_start-reinterpret_cast<const uint8_t*>(data)));
			}
			if (reinterpret_cast<const uint8_t*>(data)+size<file_data_start+data_len) continue;
			it.data(std::vector<uint8_t>(file_data_start,file_data_start+data_len));
		}
		return true;
	}
	std::string extract_archive_comment(const end_of_central_directory* eocd,const char* data,std::size_t size) {
		if (eocd->comment_length_==0) return "";
		const char* comment_start=reinterpret_cast<const char*>(eocd)+sizeof(end_of_central_directory);
		if (comment_start+eocd->comment_length_>data+size) return "";
		return std::string(comment_start,eocd->comment_length_);
	}

public:	
	archive()=default;
	
	bool open(const std::string& filename) {
		std::ifstream file(filename,std::ios::binary);
		if (!file) return false;
		file.seekg(0,std::ios::end);
		auto size=file.tellg();
		file.seekg(0,std::ios::beg);
		std::vector<char> buffer(size);
		if (!file.read(buffer.data(),size)) return false;
		return parse(buffer.data(),buffer.size());
	}
	bool load(const std::vector<uint8_t>& data) {
		return parse(reinterpret_cast<const char*>(data.data()),data.size());
	}
	bool parse(const char* data,std::size_t size) {
		files_.clear();
		const char* end=data+size;
		const end_of_central_directory* eocd=nullptr;
		const zip64_end_of_central_directory_locator* loc64=nullptr;
		const zip64_end_of_central_directory* eocd64=nullptr;
		const std::size_t max_eocd_search=std::min<std::size_t>(size,0xFFFF+sizeof(end_of_central_directory));
		const char* eocd_search_start=end-max_eocd_search;
		for (const char* p=end-4;p>=eocd_search_start;p--) {
			if (p[0]=='P' && p[1]=='K' && p[2]==0x05 && p[3]==0x06) {
				auto cand=reinterpret_cast<const end_of_central_directory*>(p);
				if (cand->valid()) {
					eocd=cand;
					break;
				}
			}
			if (p==data) break;
		}
		const std::size_t max_loc64_search=std::min<std::size_t>(size,1024*8);
		const char* loc_search_start=end-max_loc64_search;
		for (const char* p=end-4;p>=loc_search_start;--p) {
			if (p[0]=='P' && p[1]=='K' && p[2]==0x06 && p[3]==0x07) {
				auto cand=reinterpret_cast<const zip64_end_of_central_directory_locator*>(p);
				if (cand->valid()) {
					loc64=cand;
					break;
				}
			}
			if (p==data) break; 
		}
		uint64_t cd_offset=0;
		uint64_t cd_size=0;
		uint64_t entries=0;
		if (loc64 && loc64->valid()) {
			uint64_t pos=loc64->offset_of_eocd_;
			if (pos+sizeof(zip64_end_of_central_directory)<=size) {
			auto cand64=reinterpret_cast<const zip64_end_of_central_directory*>(data+pos);
			if (cand64 && cand64->valid()) {
				eocd64=cand64;
				cd_offset=eocd64->central_dir_offset_;
				cd_size=eocd64->central_dir_size_;
				entries=eocd64->total_entries_;
			}
		}
		}
		if (!eocd64 && eocd) {
			if (!eocd->valid()) return false;
			cd_offset=eocd->central_dir_offset_;
			cd_size=eocd->central_dir_size_;
			entries=eocd->total_entries_;
		}
		if (cd_offset==0 || cd_offset>=size) return false;
		if (cd_offset+cd_size>size) cd_size=size-cd_offset;
		central_direction_view cdv{ cd_offset,cd_size,entries };
		return parse_central_directory64(data,size,cdv);
		//end_of_central_directory eocd_tmp{};
		//eocd_tmp.central_dir_offset_=static_cast<uint32_t>(cd_offset);
		//eocd_tmp.central_dir_size_=static_cast<uint32_t>(cd_size);
		//eocd_tmp.total_entries_=static_cast<uint16_t>(entries);
		//return parse_central_directory(data,size,&eocd_tmp);
	}
	std::vector<uint8_t> extract(const file_info& file) const {
		size_t off = static_cast<size_t>(file.local_header_offset());
		if (file.local_header_offset()>=data_.size()) throw std::runtime_error("invalid local header offset");
		std::size_t lhoff=static_cast<std::size_t>(file.local_header_offset());
		if (lhoff>=data_.size()) throw std::runtime_error("invalid local header offset");
		const uint8_t* lfh_ptr=data_.data()+lhoff;
		const local_file_header* lfh=reinterpret_cast<const local_file_header*>(lfh_ptr);
		if (!lfh->valid()) throw std::runtime_error("invalid local file header");
		const uint8_t* file_data_start=lfh_ptr+sizeof(local_file_header)+lfh->file_name_length_+lfh->extra_field_length_;
		std::size_t start_off=file_data_start-data_.data();
		std::size_t end_of_compressed=0;
		if (lfh->compressed_size_!=0) end_of_compressed=start_off+lfh->compressed_size_;
		else if (file.compressed_size()!=0) end_of_compressed=start_off+file.compressed_size();
		else if (lfh->flags_&0x08) {
			std::size_t scan=start_off;
			end_of_compressed=data_.size();
			while (scan+4<=data_.size()) {
				uint32_t sig=data_[scan]|(data_[scan+1]<<8)|(data_[scan+2]<<16)|(data_[scan+3]<<24);
				if (sig==0x08074b50 || sig==0x04034b50 || sig==0x02014b50) {
					end_of_compressed = scan;
					break;
				}
				scan++;
			}
		} else {
			end_of_compressed=start_off+lfh->compressed_size_;
			if (end_of_compressed>data_.size()) end_of_compressed=data_.size();
		}
		if (lfh->compression_method_==0 && (lfh->flags_&0x08) && lfh->compressed_size_==0) end_of_compressed=start_off;
		if (end_of_compressed<start_off || end_of_compressed>data_.size()) throw std::runtime_error("bad compressed range");
		std::vector<uint8_t> compressed_data(data_.begin()+start_off,data_.begin()+end_of_compressed);
		switch (file.method()) {
			case CM_STORED: return compressed_data;
			case CM_DEFLATED: return deflate_compressor::decompress(compressed_data,true);
			case CM_BZIP2: return bzip2_compressor::decompress(compressed_data);
			default: throw std::runtime_error("unsupported compression method");
		}	
	}
	void create() {
		files_.clear();
		data_.clear();
		archive_comment_.clear();
	}
	bool add_file(const std::filesystem::path& file_path,compression_method method=CM_STORED,compression_level level=CL_NORMAL,const std::string& name_in_archive="") {
		std::error_code ec;
		if (!std::filesystem::exists(file_path,ec)) return false;
		file_info file(name_in_archive.empty()?file_path.filename().string():name_in_archive);
		if (std::filesystem::is_directory(file_path,ec)) {
			file.filename(file.filename()+"/");
			file.set_directory(true);
			auto ftime=std::filesystem::last_write_time(file_path,ec);
			auto sctp=std::chrono::time_point_cast<std::chrono::system_clock::duration>(ftime-decltype(ftime)::clock::now()+std::chrono::system_clock::now());
			file.last_write_time(sctp);
			file.external_attributes(0x10);
			files_.push_back(std::move(file));
			return true;
		}
		std::ifstream in_file(file_path,std::ios::binary);
		if (!in_file) return false;
		in_file.seekg(0,std::ios::end);
		auto file_size=in_file.tellg();
		in_file.seekg(0,std::ios::beg);
		std::vector<uint8_t> file_data(static_cast<std::size_t>(file_size));
		if (!in_file.read(reinterpret_cast<char*>(file_data.data()),file_size)) return false;
		file.data(std::move(file_data));
		file.method(method);
		file.calculate_crc32();
		auto ftime=std::filesystem::last_write_time(file_path,ec);
		auto sctp=std::chrono::time_point_cast<std::chrono::system_clock::duration>(ftime-decltype(ftime)::clock::now()+std::chrono::system_clock::now());
		file.last_write_time(sctp);
		file.external_attributes(0x20);
		files_.push_back(std::move(file));
		return true;
	}
	bool add_directory_tree(const std::filesystem::path& directory_path,compression_method method=CM_STORED,compression_level level=CL_NORMAL,const std::string& base_path_in_archive="") {
		std::error_code ec;
		if (!std::filesystem::exists(directory_path) || !std::filesystem::is_directory(directory_path)) return false;
		std::string dir_name=base_path_in_archive.empty()?directory_path.filename().string():base_path_in_archive;
		add_directory(dir_name);
		for (const auto& it:std::filesystem::recursive_directory_iterator(directory_path,ec)) {
			if (ec) continue;
			auto relative_path=std::filesystem::relative(it.path(),directory_path,ec);
			if (ec) continue;
			std::string archive_path=dir_name+"/"+relative_path.string();
			std::replace(archive_path.begin(),archive_path.end(),'\\','/');
			if (it.is_directory(ec)) {
				archive_path+="/";
				add_directory(archive_path);
			} else add_file(it.path(),method,level,archive_path);
		}
		return true;
	}
	void add_data(const std::string& filename,const std::vector<uint8_t>& data,compression_method method=CM_STORED,compression_level level=CL_NORMAL,const std::string& comment="") {
		file_info file(filename);
		file.data(data);
		file.method(method);
		file.calculate_crc32();
		file.last_write_time(std::chrono::system_clock::now());
		file.comment(comment);
		file.external_attributes(0x20);
		file.compress_data(level);
		files_.push_back(std::move(file));
	}
	void add_directory(const std::string& dirname,const std::string& comment="") {
		std::string normalized_dirname=dirname;
		if (!normalized_dirname.empty() && normalized_dirname.back()!='/') {
			normalized_dirname+='/';
		}
		file_info file(normalized_dirname);
		file.set_directory(true);
		file.last_write_time(std::chrono::system_clock::now());
		file.comment(comment);
		file.external_attributes(0x10);
		files_.push_back(std::move(file));
	}
	bool save(const std::string& filename,const std::string& archive_comment="") {
		archive_comment_=archive_comment;
		std::ofstream out_file(filename,std::ios::binary);
		if (!out_file) return false;
		std::vector<uint8_t> zip_data=build_zip_data();
		return !!out_file.write(reinterpret_cast<const char*>(zip_data.data()),zip_data.size());
	}
	
	std::vector<uint8_t> save_to_memory(const std::string& archive_comment="") {
		archive_comment_=archive_comment;
		return build_zip_data();
	}
	
	iterator begin() noexcept { return files_.begin(); }
	iterator end() noexcept { return files_.end(); }
	const_iterator begin() const noexcept { return files_.begin(); }
	const_iterator end() const noexcept { return files_.end(); }
	const_iterator cbegin() const noexcept { return files_.cbegin(); }
	const_iterator cend() const noexcept { return files_.cend(); }
	
	bool empty() const noexcept { return files_.empty(); }
	size_type size() const noexcept { return files_.size(); }
	size_type max_size() const noexcept { return files_.max_size(); }
	void reserve(size_type n) { files_.reserve(n); }
	size_type capacity() const noexcept { return files_.capacity(); }
	
	reference operator [](size_type pos) { return files_[pos]; }
	const_reference operator [](size_type pos) const { return files_[pos]; }
	reference at(size_type pos) { return files_.at(pos); }
	const_reference at(size_type pos) const { return files_.at(pos); }
	
	reference front() { return files_.front(); }
	const_reference front() const { return files_.front(); }
	reference back() { return files_.back(); }
	const_reference back() const { return files_.back(); }

	const std::vector<uint8_t>& raw_data() const noexcept { return data_; }

	iterator find(const std::string& filename) {
		return std::find_if(files_.begin(),files_.end(),[&](const auto& file){ return file.filename()==filename; });
	}
	const_iterator find(const std::string& filename) const {
		return std::find_if(files_.begin(),files_.end(),[&](const auto& file){ return file.filename()==filename; });
	}
	bool contains(const std::string& filename) const {
		return find(filename)!=files_.end();
	}

	iterator erase(const_iterator pos) {
		return files_.erase(pos);
	}
	iterator erase(const_iterator first,const_iterator last) {
		return files_.erase(first,last);
	}
	size_type erase(const std::string& filename) {
		auto it=find(filename);
		if (it != files_.end()) {
			files_.erase(it);
			return 1;
		}
		return 0;
	}

	iterator insert(const_iterator pos,const file_info& value) {
		return files_.insert(pos,value);
	}
	iterator insert(const_iterator pos,file_info&& value) {
		return files_.insert(pos,std::move(value));
	}
	template <typename _InputIt>
	iterator insert(const_iterator pos,_InputIt first,_InputIt last) {
		return files_.insert(pos,first,last);
	}

	void push_back(const file_info& value) {
		files_.push_back(value);
	}
	void push_back(file_info&& value) {
		files_.push_back(std::move(value));
	}
	template <typename... _Args>
	reference emplace_back(_Args&&... args) {
		return files_.emplace_back(std::forward<_Args>(args)...);
	}

	void clear() noexcept {
		files_.clear();
		data_.clear();
		archive_comment_.clear();
	}

	void swap(archive& other) noexcept {
		files_.swap(other.files_);
		data_.swap(other.data_);
		archive_comment_.swap(other.archive_comment_);
	}

	bool extract_to(const file_info& file,const std::filesystem::path& target_path) const {
		std::error_code ec;
		if (file.is_directory()) {
			std::filesystem::create_directories(target_path,ec);
			if (ec) return false;
			return true;
		}
		std::filesystem::create_directories(target_path.parent_path(),ec);
		if (ec) return false;
		auto data=extract(file);
		//if (data.empty()) return false;
		std::ofstream out_file(target_path,std::ios::binary);
		if (!out_file) return false;
		out_file.write(reinterpret_cast<const char*>(data.data()),data.size());
		return !!out_file;
	}
	bool extract_all(const std::filesystem::path& target_dir) const {
		std::error_code ec;
		bool all_ok=true;
		for (const auto& file:files_) {
			auto target_path=target_dir/file.filepath();
			if (!extract_to(file,target_path)) {
				all_ok = false;
				continue; // ← 继续解余下文件
			}
		}
		return all_ok;//true;
	}
	bool verify() const {
		for (const auto& it:files_) {
			if (!it.verify()) return false;
		}
		return true;
	}
	std::string get_archive_comment() const {
		return archive_comment_;
	}
};

inline void swap(archive& lhs,archive& rhs) noexcept {
	lhs.swap(rhs);
}

inline archive read_zip(const std::string& filename) {
	archive arc;
	arc.open(filename);
	return arc;
}

inline archive read_zip(const std::vector<uint8_t>& data) {
	archive arc;
	arc.load(data);
	return arc;
}

inline bool write_zip(const std::string& filename,const archive& arc,const std::string& comment="") {
	archive mutable_arc=arc;
	return mutable_arc.save(filename,comment);
}

inline std::vector<uint8_t> write_zip(const archive& arc,const std::string& comment="") {
	archive mutable_arc=arc;
	return mutable_arc.save_to_memory(comment);
}


}

using zip=zipped::archive;

}

}

#endif