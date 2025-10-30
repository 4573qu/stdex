//Last Modified At 2025/10/30
//@Version 1.0.0.0
#ifndef _STDEX_TYPE_ZIP_H_
#define _STDEX_TYPE_ZIP_H_ 1

#include <algorithm>
#include <array>
#include <bitset>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iterator>
#include <iomanip>
#include <map>
#include <memory>
#include <queue>
#include <sstream>
#include <stdexcept>
#include <string>
#include <system_error>
#include <type_traits>
#include <vector>

namespace stdex {

namespace type {

namespace zip {

class deflate_compressor {
public:
	static std::vector<uint8_t> compress(const std::vector<uint8_t>& data,int level=6) {
		std::vector<uint8_t> compressed;
		uint8_t cmf=0x78;
		uint8_t flg=0x01;
		if (level>=6) flg=0x9C;
		else if (level>=3) flg=0x5E;
		compressed.push_back(cmf);
		compressed.push_back(flg);
		std::size_t pos=0;
		while (pos<data.size()) {
			size_t block_size=std::min<size_t>(data.size()-pos,65535);
			uint8_t bfinal=(pos + block_size >= data.size())?1:0;
			uint8_t btype=0;
			compressed.push_back(static_cast<uint8_t>(bfinal|(btype<<1)));
			compressed.push_back(static_cast<uint8_t>(block_size&0xFF));
			compressed.push_back(static_cast<uint8_t>((block_size>>8)&0xFF));
			compressed.push_back(static_cast<uint8_t>((~block_size)&0xFF));
			compressed.push_back(static_cast<uint8_t>(((~block_size)>>8)&0xFF));
			compressed.insert(compressed.end(),data.begin()+pos,data.begin()+pos+block_size);
			pos+=block_size;
		}
		uint32_t adler=compute_adler32(data);
		compressed.push_back(static_cast<uint8_t>((adler>>24)&0xFF));
		compressed.push_back(static_cast<uint8_t>((adler>>16)&0xFF));
		compressed.push_back(static_cast<uint8_t>((adler>>8)&0xFF));
		compressed.push_back(static_cast<uint8_t>(adler&0xFF));
		return compressed;
	}
	static std::vector<uint8_t> decompress(const std::vector<uint8_t>& compressed) {
		if (compressed.size()<6) throw std::runtime_error("Invalid compressed data!");
		uint8_t cmf=compressed[0];
		uint8_t flg=compressed[1];
		if ((cmf&0x0F)!=8) throw std::runtime_error("Invalid compression method!");
		if ((cmf*256+flg)%31!=0) throw std::runtime_error("Invalid FCHECK");
		std::vector<uint8_t> decompressed;
		size_t pos=2;
		while (pos<compressed.size()-4) {
			if (pos>=compressed.size()) throw std::runtime_error("Unexpected end of compressed data!");
			uint8_t header=compressed[pos++];
			bool bfinal=(header&0x01)!=0;
			uint8_t btype=(header>>1)&0x03;
			if (btype==0) {
				if (pos+4>compressed.size()) throw std::runtime_error("Unexpected end of compressed data!");
				uint16_t len=static_cast<uint16_t>(compressed[pos])|(static_cast<uint16_t>(compressed[pos+1])<<8);
				uint16_t nlen=static_cast<uint16_t>(compressed[pos+2])|(static_cast<uint16_t>(compressed[pos+3])<<8);
				if (len!=static_cast<uint16_t>(~nlen)) throw std::runtime_error("Invalid length check!");
				pos+=4;
				if (pos+len>compressed.size()) throw std::runtime_error("Stored block length exceeds available data!");	
				decompressed.insert(decompressed.end(),compressed.begin()+pos,compressed.begin()+pos+len);
				pos+=len;
			} else throw std::runtime_error("Compressed blocks not supported!");
			if (bfinal) break;
		}
		if (pos+4>compressed.size()) throw std::runtime_error("Missing ADLER32 checksum!");
		uint32_t expected_adler=(static_cast<uint32_t>(compressed[pos])<<24)|(static_cast<uint32_t>(compressed[pos+1])<<16)|(static_cast<uint32_t>(compressed[pos+2])<<8)|static_cast<uint32_t>(compressed[pos+3]);
		uint32_t actual_adler=compute_adler32(decompressed);
		if (expected_adler!=actual_adler) throw std::runtime_error("ADLER32 checksum mismatch!");
		return decompressed;
	}

private:
	static uint32_t compute_adler32(const std::vector<uint8_t>& data) {
		uint32_t a=1,b=0;
		for (uint8_t it:data) {
			a=(a+it)%65521;
			b=(b+a)%65521;
		}
		return (b<<16)|a;
	}
};

class bzip2_compressor {
	struct huffman_node {
		int symbol_;
		int frequency_;
		std::shared_ptr<huffman_node> left_;
		std::shared_ptr<huffman_node> right_;
		huffman_node(int s,int f) : symbol_(s) , frequency_(f) , left_(nullptr) , right_(nullptr) { }
		huffman_node(std::shared_ptr<huffman_node> l,std::shared_ptr<huffman_node> r) : symbol_(-1) , frequency_(l->frequency_+r->frequency_) , left_(l) , right_(r) { }
		bool is_leaf() const { return symbol!=-1; }
	};
	struct huffman_compare {
		bool operator ()(const std::shared_ptr<huffman_node>& a,const std::shared_ptr<huffman_node>& b) {
			return a->frequency_>b->frequency_;
		}
	};
	static void build_huffman_codes(const std::shared_ptr<huffman_node>& node,const std::string& code,std::vector<std::string>& codes) {
		if (node->is_leaf()) codes[node->symbol_]=code;
		else {
			build_huffman_codes(node->left_,code+"0",codes);
			build_huffman_codes(node->right_,code+"1",codes);
		}
	}
	static std::shared_ptr<huffman_node> build_huffman_tree(const std::vector<int>& frequencies) {
		std::priority_queue<std::shared_ptr<huffman_node>,std::vector<std::shared_ptr<huffman_node>>,huffman_compare> pq;
		for (std::size_t i=0;i<frequencies.size();i++) {
			if (frequencies[i]>0) pq.push(std::make_shared<huffman_node>(static_cast<int>(i),frequencies[i]));
		}
		while (pq.size()>1) {
			auto left=pq.top();
			pq.pop();
			auto right=pq.top();
			pq.pop();
			pq.push(std::make_shared<huffman_node>(left,right));
		}
		return pq.top();
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
		result.reserve(data.size());
		for (const auto& it:rotations) result.push_back(it.back());
		return result;
	}
	static std::vector<uint8_t> apply_mtf(const std::vector<uint8_t>& data) {
		std::vector<uint8_t> dictionary(256);
		for (int i=0;i<256;i++) dictionary[i]=static_cast<uint8_t>(i);
		std::vector<uint8_t> result;
		sresult.reserve(data.size());
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
		std::string bit_stream;
		for (uint8_t it:transformed) bit_stream+=huffman_codes[it];
		while (bit_stream.length()%8!=0) bit_stream+='0';
		for (std::size_t i=0;i<bit_stream.length();i+=8) {
			std::string byte_str=bit_stream.substr(i,8);
			uint8_t byte=static_cast<uint8_t>(std::bitset<8>(byte_str).to_ulong());
			compressed.push_back(byte);
		}
		uint32_t crc=integrity::crc32::calculate(data);
		compressed.push_back(static_cast<uint8_t>((crc>>24)&0xFF));
		compressed.push_back(static_cast<uint8_t>((crc>>16)&0xFF));
		compressed.push_back(static_cast<uint8_t>((crc>>8)&0xFF));
		compressed.push_back(static_cast<uint8_t>(crc&0xFF));
		return compressed;
	}
	static std::vector<uint8_t> decompress(const std::vector<uint8_t>& compressed) {
		if (compressed.size()<10) throw std::runtime_error("Invalid BZIP2 data!");
		if (compressed[0]!='B' || compressed[1]!='Z' || compressed[2]!='h') throw std::runtime_error("Invalid BZIP2 header!");
		int level=compressed[3]-'0';
		if (level<1 || level>9) throw std::runtime_error("Invalid BZIP2 compression level!");
		std::vector<uint8_t> transformed(compressed.begin()+4,compressed.end()-4);
		std::vector<uint8_t> result=transformed;
		uint32_t expected_crc=(static_cast<uint32_t>(compressed[compressed.size()-4])<<24)|(static_cast<uint32_t>(compressed[compressed.size()-3])<<16)|(static_cast<uint32_t>(compressed[compressed.size()-2])<<8)|static_cast<uint32_t>(compressed[compressed.size()-1]);
		uint32_t actual_crc=integrity::crc32::calculate(result);
		if (expected_crc!=actual_crc) throw std::runtime_error("BZIP2 CRC32 mismatch!");
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
	static constexpr uint32_t expected_signature_=0x02014B50_;
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
#pragma pack(pop)

enum compression_method : uint16_t {
	CM_STORED=0,
	//CM_SHRUNK=1,
	//CM_REDUCED_1=2,
	//CM_REDUCED_2=3,
	//CM_REDUCED_3=4,
	//CM_REDUCED_4=5,
	//CM_IMPLODED=6,
	CM_DEFLATED=8,
	//CM_ENHANCED_DEFLATED=9,
	//CM_PKWARE_DCL_IMPLODED=10,
	CM_BZIP2=12,
	//CM_LZMA=14,
	//CM_IBM_TERSE=18,
	//CM_IBM_LZ77=19,
};

enum compression_level : int {
	CL_NONE=0,
	CL_FAST=1,
	CL_NORMAL=6,
	CL_MAXIMUM=9,
}

class file_info {
public:
	using string_type=std::string;
	using byte_vector=std::vector<uint8_t>;
	using path_type=std::filesystem::path;

private:
	string_type filename_;
	path_type filepath_;
	compression_method method_=CM_STORED;
	uint32_t compressed_size_=0;
	uint32_t uncompressed_size_=0;
	uint32_t crc32_=0;
	uint32_t local_header_offset_=0;
	bool is_directory_=false;
	std::chrono::system_clock::time_point last_write_time_=std::chrono::system_clock::now();
	byte_vector data_;
	std::string comment_;
	uint16_t internal_attributes_=0;
	uint32_t external_attributes_=0;
	byte_vector extra_field_;

	void update_filepath() {
		filepath_=filename_;
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
	uint32_t compressed_size() const noexcept { return compressed_size_; }
	uint32_t uncompressed_size() const noexcept { return uncompressed_size_; }
	uint32_t crc32() const noexcept { return crc32_; }
	uint32_t local_header_offset() const noexcept { return local_header_offset_; }
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
	
	void filename(string_type name) { 
		filename_=std::move(name);
		update_filepath();
	}
	
	void method(compression_method m) { method_=m; }
	void compressed_size(uint32_t size) { compressed_size_=size; }
	void uncompressed_size(uint32_t size) { uncompressed_size_=size; }
	void crc32(uint32_t crc) { crc32_=crc; }
	void local_header_offset(uint32_t offset) { local_header_offset_=offset; }
	void last_write_time(std::chrono::system_clock::time_point time) { 
		last_write_time_=time; 
	}
	void data(byte_vector d) { 
		data_=std::move(d);
		uncompressed_size_=static_cast<uint32_t>(data_.size());
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
			return;
		}
		std::vector<uint8_t> compressed;
		bool compression_successful=false;
		switch (method_) {
			case CM_DEFLATED: {
				compressed=deflate_compressor::compress(data_,static_cast<int>(level));
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
				return;
			}
		}
		if (compression_successful) {
			data_=std::move(compressed);
			compressed_size_=static_cast<uint32_t>(data_.size());
		} else {
			method_=CM_STORED;
			compressed_size_=uncompressed_size_;
		}
	}
	void decompress_data() {
		if (method_==CM_STORED || data_.empty()) return;
		switch (method_) {
			case CM_DEFLATED: {
				data_=deflate_compressor::decompress(data_);
				break;
			}
			case CM_BZIP2: {
				data_=bzip2_compressor::decompress(data_);
				break;
			}
			default: throw std::runtime_error("Unsupported compression method for decompression!");
		}
		uncompressed_size_=static_cast<uint32_t>(data_.size());
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
	std::vector<value_type> files_;
	std::vector<char> data_;

	std::vector<uint8_t> build_zip_data() {
		std::vector<uint8_t> zip_data;
		std::vector<std::pair<size_t,uint32_t>> local_header_offsets;
		for (std::size_t i=0;i<files_.size();i++) {
			auto& file=files_[i];
			uint32_t local_header_offset=static_cast<uint32_t>(zip_data.size());
			local_header_offsets.emplace_back(i,local_header_offset);
			local_file_header lfh{};
			lfh.signature_=local_file_header::expected_signature_;
			lfh.version_needed_=20;
			lfh.flags_=0;
			lfh.compression_method_=static_cast<uint16_t>(file.method());
			auto [dos_time,dos_date]=file.to_dos_time();
			lfh.last_mod_time_=dos_time;
			lfh.last_mod_date_=dos_date;
			lfh.crc32_=file.crc32();
			lfh.compressed_size_=file.compressed_size();
			lfh.uncompressed_size_=file.uncompressed_size();
			lfh.file_name_length_=static_cast<uint16_t>(file.filename().size());
			lfh.extra_field_length_=static_cast<uint16_t>(file.extra_field().size());
			const uint8_t* lfh_ptr=reinterpret_cast<const uint8_t*>(&lfh);
			zip_data.insert(zip_data.end(),lfh_ptr,lfh_ptr+sizeof(lfh));
			const uint8_t* filename_ptr=reinterpret_cast<const uint8_t*>(file.filename().data());
			zip_data.insert(zip_data.end(),filename_ptr,filename_ptr+file.filename().size());
			zip_data.insert(zip_data.end(),file.extra_field().begin(),file.extra_field().end());
			if (!file.is_directory()) zip_data.insert(zip_data.end(), file.data().begin(), file.data().end());
		}
		uint32_t central_dir_start=static_cast<uint32_t>(zip_data.size());
		for (const auto& [index,local_offset]:local_header_offsets) {
			const auto& file=files_[index];
			central_directory_header cdh{};
			cdh.signature=central_directory_header::expected_signature_;
			cdh.version_made_by_=20;
			cdh.version_needed_=20;
			cdh.flags_=0;
			cdh.compression_method_=static_cast<uint16_t>(file.method());
			auto [dos_time,dos_date]=file.to_dos_time();
			cdh.last_mod_time_=dos_time;
			cdh.last_mod_date_=dos_date;
			cdh.crc32_=file.crc32();
			cdh.compressed_size_=file.compressed_size();
			cdh.uncompressed_size_=file.uncompressed_size();
			cdh.file_name_length_=static_cast<uint16_t>(file.filename().size());
			cdh.extra_field_length_=static_cast<uint16_t>(file.extra_field().size());
			cdh.file_comment_length_=static_cast<uint16_t>(file.comment().size());
			cdh.disk_number_start_=0;
			cdh.internal_attributes_=file.internal_attributes();
			cdh.external_attributes_=file.external_attributes();
			cdh.local_header_offset_=local_offset;
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
		uint32_t central_dir_size=static_cast<uint32_t>(zip_data.size()-central_dir_start);
		end_of_central_directory eocd{};
		eocd.signature_=end_of_central_directory::expected_signature;
		eocd.disk_number_=0;
		eocd.disk_start_=0;
		eocd.num_entries_on_disk_=static_cast<uint16_t>(files_.size());
		eocd.total_entries_=static_cast<uint16_t>(files_.size());
		eocd.central_dir_size_=central_dir_size;
		eocd.central_dir_offset_=central_dir_start;
		eocd.comment_length_=static_cast<uint16_t>(archive_comment_.size());
		const uint8_t* eocd_ptr=reinterpret_cast<const uint8_t*>(&eocd);
		zip_data.insert(zip_data.end(),eocd_ptr,eocd_ptr+sizeof(eocd));
		if (!archive_comment_.empty()) {
			const uint8_t* comment_ptr=reinterpret_cast<const uint8_t*>(archive_comment_.data());
			zip_data.insert(zip_data.end(),comment_ptr,comment_ptr+archive_comment_.size());
		}
		return zip_data;
	}
	bool parse_central_directory(const char* data,std::size_t size,const end_of_central_directory* eocd) {
		data_=std::vector<char>(data,data+size);
		if (eocd->central_dir_offset_>=size)return false;
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
			ptr+=cdh->extra_field_length;
			if (cdh->file_comment_length_>0) {
				if (ptr+cdh->file_comment_length_>cd_end) return false;
				file.comment(std::string(ptr,cdh->file_comment_length_));
				ptr+=cdh->file_comment_length_;
			}
			file.method(static_cast<compression_method>(cdh->compression_method_));
			file.compressed_size(cdh->compressed_size_);
			file.uncompressed_size(cdh->uncompressed_size_);
			file.crc32(cdh->crc32_);
			file.local_header_offset_(cdh->local_header_offset_);
			file.set_dos_time(cdh->last_mod_time_,cdh->last_mod_date_);
			file.internal_attributes(cdh->internal_attributes_);
			file.external_attributes(cdh->external_attributes_);
			files_.push_back(std::move(file));
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
		constexpr std::size_t min_eocd_size=sizeof(end_of_central_directory);
		if (size<min_eocd_size) return false;
		const auto* end_ptr=data+size;
		for (const char* ptr=end_ptr-min_eocd_size;ptr>=data;ptr--) {
			if (ptr+min_eocd_size>end_ptr) continue;
			const auto* eocd=reinterpret_cast<const end_of_central_directory*>(ptr);
			if (eocd->valid()) {
				archive_comment_=extract_archive_comment(eocd,data,size);
				return parse_central_directory(data,size,eocd);
			}
		}
		return false;
	}
	std::vector<uint8_t> extract(const file_info& file) const {
		if (file.local_header_offset()>=data_.size()) return {};
		const auto* lfh_ptr=data_.data()+file.local_header_offset();
		const auto* lfh=reinterpret_cast<const local_file_header*>(lfh_ptr);
		if (!lfh->valid() || lfh_ptr+sizeof(local_file_header)>data_.data()+data_.size()) return {};
		const uint8_t* file_data_start=lfh_ptr+sizeof(local_file_header)+lfh->file_name_length+lfh->extra_field_length;
		if (file_data_start+lfh->compressed_size>data_.data()+data_.size()) return {};
		std::vector<uint8_t> compressed_data(lfh->compressed_size);
		try {
			switch (file.method()) {
				case CM_STORED: return compressed_data;
				case CM_DEFLATED: return deflate_compressor::decompress(compressed_data);
				case CM_BZIP2: return bzip2_compressor::decompress(compressed_data);
				default: throw std::runtime_error("Unsupported compression method!");
			}
		} catch (const std::exception& e) {
			return {};
		}
		return {};
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
			file.is_directory_=true;
			file.last_write_time(std::filesystem::last_write_time(file_path,ec));
			file.external_attributes(0x10);
			files_.push_back(std::move(file));
			return true;
		}
		std::ifstream in_file(file_path,std::ios::binary);
		if (!in_file) return false;
		in_file.seekg(0,std::ios::end);
		auto file_size=in_file.tellg();
		in_file.seekg(0,std::ios::beg);
		std::vector<uint8_t> file_data(static_cast<size_t>(file_size));
		if (!in_file.read(reinterpret_cast<char*>(file_data.data()),file_size)) return false;
		file.data(std::move(file_data));
		file.method(method);
		file.calculate_crc32();
		file.last_write_time(fs::last_write_time(file_path,ec));
		file.external_attributes(0x20); // DOS文件属性
		file.compress_data(level);
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
			} else add_file(entry.path(),method,level,archive_path);
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
		file.is_directory_=true;
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
		if (file.is_directory()) return std::filesystem::create_directories(target_path);
		auto data=extract(file);
		if (data.empty()) return false;
		std::ofstream out_file(target_path,std::ios::binary);
		if (!out_file) return false;
		out_file.write(reinterpret_cast<const char*>(data.data()),data.size());
		return !!out_file;
	}
	bool extract_all(const fs::path& target_dir) const {
		std::error_code ec;
		for (const auto& file:files_) {
			auto target_path=target_dir/file.filepath();
			if (!extract_to(file,target_path))return false;
		}
		return true;
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

}

}

#endif