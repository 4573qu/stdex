//Last Modified At 2026/04/06
//@Version 1.0.0.0
#ifndef _STDEX_CRYPTO_BASE_H_
#define _STDEX_CRYPTO_BASE_H_ 1

#include <array>
#include <cctype>
#include <climits>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <limits>
#include <numeric>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

#if __has_include("../macros/cpp_version.h")
#include "../macros/cpp_version.h" // At Least 1.0
#endif

#ifndef _STDEX_CPP20_VERSION
#define _STDEX_CPP20_VERSION 202002L
#endif

#if __cplusplus>=_STDEX_CPP20_VERSION
#include <bit>
#include <span>
#endif

namespace stdex {

namespace crypto {

namespace base {

enum padding_mode {
	PM_REQUIRE,
	PM_ALLOW,
	PM_REJECT,
};

enum case_mode {
	CM_STRICT,
	CM_IGNORE_CASE,
};

enum error_mode {
	EM_STRICT,
	EM_RELAXED,
};

struct decode_options {
	padding_mode padding=PM_ALLOW;
	case_mode casing=CM_STRICT;
	error_mode err=EM_STRICT;
	bool ignore_whitespace=true;
	bool accept_url_safe_alias=false;
};



#if __cplusplus>=_STDEX_CPP20_VERSION
constexpr bool has_single_bit(std::size_t x) noexcept {
	return std::has_single_bit(x);
}
#else
constexpr bool has_single_bit(std::size_t x) noexcept {
	return x>=2 && ((x&(x-1))==0);
}
#endif

constexpr std::size_t bits_per_char_if_pow2(std::size_t x) noexcept {
	std::size_t bits=0;
	while (x>1) {
		x>>=1;
		bits++;
	}
	return bits;
}

template <typename _Tp>
constexpr bool is_ascii_whitespace(_Tp ch) noexcept {
	return ch==' ' || ch=='\t' || ch=='\r' || ch=='\n' || ch=='\f' || ch=='\v';
}

constexpr unsigned char ascii_tolower(unsigned char ch) noexcept {
	return (ch>='A' && ch<='Z') ? static_cast<unsigned char>(ch-'A'+'a') : ch;
}

constexpr unsigned char ascii_toupper(unsigned char ch) noexcept {
	return (ch>='a' && ch<='z') ? static_cast<unsigned char>(ch-'a'+'A') : ch;
}

inline bool has_duplicate_chars(const std::string& alphabet) noexcept {
	for (std::size_t i=0;i<alphabet.size();i++) {
		for (std::size_t j=i+1;j<alphabet.size();j++) if (alphabet[i]==alphabet[j]) return true;
	}
	return false;
}

inline bool alphabet_has_padding_char(const std::string& alphabet,char padding_char) noexcept {
	for (char ch:alphabet) {
		if (ch==padding_char) return true;
	}
	return false;
}

inline std::size_t encoded_max_size_generic(std::size_t input_size,std::size_t radix) noexcept {
	if (input_size==0) return 0;
	if (radix<2) return 0;
	return input_size*8;
}

inline std::size_t decoded_max_size_generic(std::size_t encoded_size,std::size_t radix) noexcept {
	if (encoded_size==0) return 0;
	if (radix<2) return 0;
	return encoded_size;
}

struct alphabet_data {
	std::string alphabet;
	std::array<int16_t,256> reverse_table_strict{};
	std::array<int16_t,256> reverse_table_nocase{};
	std::size_t radix=0;
	bool uses_padding=false;
	char padding_char='=';
	bool has_single_bit=false;
	std::size_t bits_per_char=0;
	std::size_t input_block_bytes=0;
	std::size_t output_block_chars=0;
	std::size_t alphabet_mask=0;
	bool supports_case_insensitive_decode=false;
	alphabet_data() {
		for (std::size_t i=0;i<256;i++) {
			reverse_table_strict[i]=-1;
			reverse_table_nocase[i]=-1;
		}
	}
};

inline void initialize_alphabet_data(alphabet_data& data,const std::string& alphabet,bool uses_padding,char padding_char) {
	if (alphabet.size()<2 || alphabet.size()>255) throw std::invalid_argument("Alphabet size must be between 2 and 255.");
	if (has_duplicate_chars(alphabet)) throw std::invalid_argument("Alphabet characters must be unique.");
	if (uses_padding && alphabet_has_padding_char(alphabet,padding_char)) throw std::invalid_argument("Padding character must not appear in alphabet.");
	data.alphabet=alphabet;
	data.radix=alphabet.size();
	data.uses_padding=uses_padding;
	data.padding_char=padding_char;
	data.has_single_bit=has_single_bit(data.radix);
	data.bits_per_char=data.has_single_bit?bits_per_char_if_pow2(data.radix):0;
	data.input_block_bytes=data.has_single_bit?std::lcm(static_cast<std::size_t>(CHAR_BIT),data.bits_per_char)/CHAR_BIT:0;
	data.output_block_chars=data.has_single_bit?std::lcm(static_cast<std::size_t>(CHAR_BIT),data.bits_per_char)/data.bits_per_char:0;
	data.alphabet_mask=data.has_single_bit?(data.radix-1):0;
	for (std::size_t i=0;i<256;i++) {
		data.reverse_table_strict[i]=-1;
		data.reverse_table_nocase[i]=-1;
	}
	for (std::size_t i=0;i<data.alphabet.size();i++) data.reverse_table_strict[static_cast<unsigned char>(data.alphabet[i])]=static_cast<int16_t>(i);
	bool nocase_ok=true;
	for (std::size_t i=0;i<data.alphabet.size();i++) {
		unsigned char ch=static_cast<unsigned char>(data.alphabet[i]);
		unsigned char low=ascii_tolower(ch);
		unsigned char upp=ascii_toupper(ch);
		if (data.reverse_table_nocase[low]==-1) data.reverse_table_nocase[low]=static_cast<int16_t>(i);
		else if (data.reverse_table_nocase[low]!=static_cast<int16_t>(i)) nocase_ok=false;
		if (data.reverse_table_nocase[upp]==-1) data.reverse_table_nocase[upp]=static_cast<int16_t>(i);
		else if (data.reverse_table_nocase[upp]!=static_cast<int16_t>(i)) nocase_ok=false;
	}
	if (!nocase_ok) {
		for (std::size_t i=0;i<256;i++) data.reverse_table_nocase[i]=-2;
	}
	data.supports_case_insensitive_decode=nocase_ok;
}

inline int decode_char(const alphabet_data& data,unsigned char ch,const decode_options& options) {
	if (options.casing==CM_IGNORE_CASE) {
		if (!data.supports_case_insensitive_decode) throw std::invalid_argument("Case-insensitive decoding is not supported for this alphabet");
		return data.reverse_table_nocase[ch];
	}
	return data.reverse_table_strict[ch];
}

inline int decode_char_with_alias(const alphabet_data& data,unsigned char ch,const decode_options& options) {
	int value=decode_char(data,ch,options);
	if (value>=0) return value;
	if (options.accept_url_safe_alias) {
		if (ch=='-') return decode_char(data,static_cast<unsigned char>('+'),options);
		if (ch=='_') return decode_char(data,static_cast<unsigned char>('/'),options);
		//if (ch=='+') return decode_char(data,static_cast<unsigned char>('-'),options);
		//if (ch=='/') return decode_char(data,static_cast<unsigned char>('_'),options);
	}
	return value;
}

inline bool is_padding_char(const alphabet_data& data,char ch) noexcept {
	return data.uses_padding && ch==data.padding_char;
}

inline std::size_t count_pad_chars_for_tail_bytes(const alphabet_data& data,std::size_t tail_bytes) noexcept {
	if (!data.uses_padding || !data.has_single_bit) return 0;
	if (tail_bytes==0) return 0;
	const std::size_t chars=(tail_bytes*CHAR_BIT+data.bits_per_char-1)/data.bits_per_char;
	return data.output_block_chars-chars;
}

template <typename _It>
_It encode_pow2_to(const alphabet_data& data,const uint8_t* bytes,std::size_t length,_It out,bool with_padding) {
	uint32_t buffer=0;
	int bits_in_buffer=0;
	for (std::size_t i=0;i<length;i++) {
		buffer=(buffer<<CHAR_BIT)|bytes[i];
		bits_in_buffer+=CHAR_BIT;
		while (bits_in_buffer>=static_cast<int>(data.bits_per_char)) {
			bits_in_buffer-=static_cast<int>(data.bits_per_char);
			std::size_t index=(buffer>>bits_in_buffer)&data.alphabet_mask;
			*out++=data.alphabet[index];
		}
	}
	if (bits_in_buffer>0) {
		std::size_t index=(static_cast<std::size_t>(buffer)<<(data.bits_per_char-static_cast<std::size_t>(bits_in_buffer)))&data.alphabet_mask;
		*out++=data.alphabet[index];
	}
	if (with_padding && data.uses_padding) {
		const std::size_t rem=length%data.input_block_bytes;
		if (rem!=0) {
			const std::size_t pad_count=count_pad_chars_for_tail_bytes(data,rem);
			for (std::size_t i=0;i<pad_count;i++) *out++=data.padding_char;
		}
	}
	return out;
}

template <typename _It>
_It decode_pow2_to(const alphabet_data& data,const char* text,std::size_t length,_It out,const decode_options& options) {
	uint32_t buffer=0;
	int bits_in_buffer=0;
	bool padding_started=false;
	std::size_t effective_chars=0;
	std::size_t pad_chars=0;
	for (std::size_t i=0;i<length;i++) {
		unsigned char ch=static_cast<unsigned char>(text[i]);
		if (options.ignore_whitespace && is_ascii_whitespace(ch)) continue;
		if (is_padding_char(data,static_cast<char>(ch))) {
			if (options.padding==PM_REJECT) throw std::invalid_argument("Padding is not allowed");
			padding_started=true;
			pad_chars++;
			continue;
		}
		if (padding_started) {
			//if (options.ignore_whitespace && is_ascii_whitespace(ch)) continue;
			throw std::invalid_argument("Invalid base decode sequence: data found after padding");
		}
		int value=decode_char_with_alias(data,ch,options);
		if (value<0) throw std::invalid_argument("Invalid base decode character");
		buffer=(buffer<<data.bits_per_char)|static_cast<uint32_t>(value);
		bits_in_buffer+=static_cast<int>(data.bits_per_char);
		effective_chars++;
		while (bits_in_buffer>=8) {
			bits_in_buffer-=8;
			*out++=static_cast<uint8_t>((buffer>>bits_in_buffer)&0xFFu);
		}
	}
	if (data.uses_padding) {
		if (options.padding==PM_REQUIRE) {
			if ((effective_chars+pad_chars)%data.output_block_chars!=0) throw std::invalid_argument("Invalid padded length");
			if (pad_chars==0 && effective_chars!=0 && (effective_chars%data.output_block_chars)!=0) throw std::invalid_argument("Padding is required");
		}
		if (pad_chars>0 && ((effective_chars+pad_chars)%data.output_block_chars)!=0) throw std::invalid_argument("Invalid padding length");
	}
	if (options.err==EM_STRICT) {
		if (bits_in_buffer>0) {
			uint32_t mask=(static_cast<uint32_t>(1u)<<bits_in_buffer)-1u;
			if ((buffer&mask)!=0) throw std::invalid_argument("Invalid base decode tail bits");
		}
	}
	return out;
}

template <typename _It>
_It encode_generic_to(const alphabet_data& data,const uint8_t* bytes,std::size_t length,_It out) {
	if (length==0) return out;
	std::size_t zero_count=0;
	while (zero_count<length && bytes[zero_count]==0) zero_count++;
	std::vector<uint8_t> digits;
	digits.reserve(length*2);
	//digits.push_back(0);
	for (std::size_t i=zero_count;i<length;i++) {
		uint32_t carry=bytes[i];
		for (std::size_t j=0;j<digits.size();j++) {
			uint32_t value=static_cast<uint32_t>(digits[j])*256u+carry;
			digits[j]=static_cast<uint8_t>(value%data.radix);
			carry=value/static_cast<uint32_t>(data.radix);
		}
		while (carry>0) {
			digits.push_back(static_cast<uint8_t>(carry%data.radix));
			carry/=static_cast<uint32_t>(data.radix);
		}
	}
	for (std::size_t i=0;i<zero_count;i++) *out++=data.alphabet[0];
	for (auto it=digits.rbegin();it!=digits.rend();it++) *out++=data.alphabet[*it];
	return out;
}

template <typename _It>
_It decode_generic_to(const alphabet_data& data,const char* text,std::size_t length,_It out,const decode_options& options) {
	std::vector<unsigned char> filtered;
	filtered.reserve(length);
	for (std::size_t i=0;i<length;i++) {
		unsigned char ch=static_cast<unsigned char>(text[i]);
		if (options.ignore_whitespace && is_ascii_whitespace(ch)) continue;
		if (is_padding_char(data,static_cast<char>(ch))) {
			if (options.padding==PM_REJECT) throw std::invalid_argument("Padding is not allowed");
			if (options.err==EM_STRICT) throw std::invalid_argument("Padding is not supported for generic radix decoding");
			continue;
		}
		int value=decode_char_with_alias(data,ch,options);
		if (value<0) throw std::invalid_argument("Invalid base decode character");
		filtered.push_back(ch);
	}
	if (filtered.empty()) return out;
	std::size_t zero_count=0;
	while (zero_count<filtered.size() && decode_char_with_alias(data,filtered[zero_count],options)==0) zero_count++;
	std::vector<uint8_t> result;
	result.reserve(filtered.size());
	for (std::size_t i=zero_count;i<filtered.size();i++) {
		int digit=decode_char_with_alias(data,filtered[i],options);
		if (digit<0) throw std::invalid_argument("Invalid base decode character");
		uint32_t carry=static_cast<uint32_t>(digit);
		for (std::size_t j=0;j<result.size();j++) {
			uint32_t value=static_cast<uint32_t>(result[j])*static_cast<uint32_t>(data.radix)+carry;
			result[j]=static_cast<uint8_t>(value&0xFFu);
			carry=value>>CHAR_BIT;
		}
		while (carry>0) {
			result.push_back(static_cast<uint8_t>(carry&0xFFu));
			carry>>=CHAR_BIT;
		}
	}
	for (std::size_t i=0;i<zero_count;i++) *out++=0;
	for (auto it=result.rbegin();it!=result.rend();it++) *out++=*it;
	return out;
}

inline std::size_t decoded_size_pow2(const alphabet_data& data,const char* text,std::size_t length,const decode_options& options) {
	std::size_t effective_chars=0;
	std::size_t pad_chars=0;
	bool padding_started=false;
	for (std::size_t i=0;i<length;i++) {
		unsigned char ch=static_cast<unsigned char>(text[i]);
		if (options.ignore_whitespace && is_ascii_whitespace(ch)) continue;
		if (is_padding_char(data,static_cast<char>(ch))) {
			if (options.padding==PM_REJECT) throw std::invalid_argument("Padding is not allowed");
			padding_started=true;
			pad_chars++;
			continue;
		}
		if (padding_started) {
			//if (options.ignore_whitespace && is_ascii_whitespace(ch)) continue;
			throw std::invalid_argument("Invalid base decode sequence: data found after padding");
		}
		int value=decode_char_with_alias(data,ch,options);
		if (value<0) throw std::invalid_argument("Invalid base decode character");
		(void)value;//[[maybe_unused]] auto _ = value;
		effective_chars++;
	}
	if (data.uses_padding) {
		if (options.padding==PM_REQUIRE) {
			if ((effective_chars+pad_chars)%data.output_block_chars!=0) throw std::invalid_argument("Invalid padded length");
			if (pad_chars==0 && effective_chars!=0 && (effective_chars%data.output_block_chars)!=0) throw std::invalid_argument("Padding is required");
		}
		if (pad_chars>0 && ((effective_chars+pad_chars)%data.output_block_chars)!=0) throw std::invalid_argument("Invalid padding length");
	}
	return (effective_chars*data.bits_per_char)/CHAR_BIT;
}

inline std::size_t decoded_size_generic(const alphabet_data& data,const char* text,std::size_t length,const decode_options& options) {
	std::size_t filtered_chars=0;
	for (std::size_t i=0;i<length;i++) {
		unsigned char ch=static_cast<unsigned char>(text[i]);
		if (options.ignore_whitespace && is_ascii_whitespace(ch)) continue;
		if (is_padding_char(data,static_cast<char>(ch))) {
			if (options.padding==PM_REJECT) throw std::invalid_argument("Padding is not allowed");
			if (options.err==EM_STRICT) throw std::invalid_argument("Padding is not supported for generic radix decoding");
			continue;
		}
		int value=decode_char_with_alias(data,ch,options);
		if (value<0) throw std::invalid_argument("Invalid base decode character");
		(void)value;//[[maybe_unused]] auto _ = value;
		filtered_chars++;
	}
	return decoded_max_size_generic(filtered_chars,data.radix);
}

template <bool _Padding=false,char _PaddingChar='=',char... _AlphabetChars>
class base;

template <>
class base<> {
public:
	using byte=uint8_t;
	using string_type=std::string;
	using value_type=char;

protected:
	alphabet_data alphabet_data_;

	void assign_alphabet(const std::string& alphabet,bool uses_padding=false,char padding_char='=') {
		initialize_alphabet_data(alphabet_data_,alphabet,uses_padding,padding_char);
	}

public:
	base()=default;
	explicit base(std::string alphabet,bool uses_padding=false,char padding_char='=') {
		assign_alphabet(alphabet,uses_padding,padding_char);
	}
	explicit base(const char* alphabet,bool uses_padding=false,char padding_char='=') {
		assign_alphabet(std::string(alphabet),uses_padding,padding_char);
	}

	const std::string& alphabet() const noexcept {
		return alphabet_data_.alphabet;
	}
	std::size_t radix() const noexcept {
		return alphabet_data_.radix;
	}
	bool uses_padding() const noexcept {
		return alphabet_data_.uses_padding;
	}
	char padding_char() const noexcept {
		return alphabet_data_.padding_char;
	}
	bool is_power_of_two_encoding() const noexcept {
		return alphabet_data_.has_single_bit;
	}
	std::size_t bits_per_char() const noexcept {
		return alphabet_data_.bits_per_char;
	}
	bool supports_case_insensitive_decode() const noexcept {
		return alphabet_data_.supports_case_insensitive_decode;
	}

	std::size_t encoded_max_size(std::size_t input_size,bool with_padding=true) const noexcept {
		if (alphabet_data_.has_single_bit) {
			if (input_size==0) return 0;
			const std::size_t chars=(input_size*CHAR_BIT+alphabet_data_.bits_per_char-1)/alphabet_data_.bits_per_char;
			if (with_padding && alphabet_data_.uses_padding) return ((chars+alphabet_data_.output_block_chars-1)/alphabet_data_.output_block_chars)*alphabet_data_.output_block_chars;
			return chars;
		}
		(void)with_padding;
		return encoded_max_size_generic(input_size,alphabet_data_.radix);
	}
	std::size_t decoded_max_size(std::size_t encoded_size) const noexcept {
		if (alphabet_data_.has_single_bit) return (encoded_size*alphabet_data_.bits_per_char)/CHAR_BIT;
		return decoded_max_size_generic(encoded_size,alphabet_data_.radix);
	}
	std::size_t decoded_size(const char* text,const decode_options& options=decode_options()) const {
		return decoded_size(text,std::char_traits<char>::length(text),options);
	}
	std::size_t decoded_size(const char* text,std::size_t length,const decode_options& options=decode_options()) const {
		if (alphabet_data_.has_single_bit) return decoded_size_pow2(alphabet_data_,text,length,options);
		return decoded_size_generic(alphabet_data_,text,length,options);
	}
	std::size_t decoded_size(const std::string& text,const decode_options& options=decode_options()) const {
		return decoded_size(text.data(),text.size(),options);
	}
	std::size_t decoded_size(std::string_view text,const decode_options& options=decode_options()) const {
		return decoded_size(text.data(),text.size(),options);
	}

	template <typename _It>
	_It encode_to(const char* data,_It out,bool with_padding=true) const {
		return encode_to(data,std::char_traits<char>::length(data),out,with_padding);
	}
	template <typename _It>
	_It encode_to(const void* data,std::size_t length,_It out,bool with_padding=true) const {
		const byte* bytes=static_cast<const byte*>(data);
		if (alphabet_data_.has_single_bit) return encode_pow2_to(alphabet_data_,bytes,length,out,with_padding);
		return encode_generic_to(alphabet_data_,bytes,length,out);
	}
	template <typename _Container,typename _It,typename=decltype(std::data(std::declval<const _Container&>()),std::size(std::declval<const _Container&>()))>
	_It encode_to(const _Container& container,_It out,bool with_padding=true) const {
		return encode_to(std::data(container),std::size(container),out,with_padding);
	}
#if __cplusplus>=_STDEX_CPP20_VERSION
	template <typename _It>
	_It encode_to(std::span<const std::byte> data,_It out,bool with_padding=true) const {
		return encode_to(data.data(),data.size(),out,with_padding);
	}
#endif
	template <typename _It>
	_It encode_to(std::string_view data,_It out,bool with_padding=true) const {
		return encode_to(data.data(),data.size(),out,with_padding);
	}
	std::string encode(const char* data,bool with_padding=true) const {
		return encode(data,std::char_traits<char>::length(data),with_padding);
	}
	std::string encode(const void* data,std::size_t length,bool with_padding=true) const {
		std::string result;
		result.reserve(encoded_max_size(length,with_padding));
		encode_to(data,length,std::back_inserter(result),with_padding);
		return result;
	}
	template <typename _Container>
	auto encode(const _Container& container,bool with_padding=true) const->decltype(std::data(container),std::size(container),std::string()) {
		return encode(std::data(container),std::size(container),with_padding);
	}
#if __cplusplus>=_STDEX_CPP20_VERSION
	std::string encode(std::span<const std::byte> data,bool with_padding=true) const {
		return encode(data.data(),data.size(),with_padding);
	}
#endif
	std::string encode(std::string_view data,bool with_padding=true) const {
		return encode(data.data(),data.size(),with_padding);
	}
	template <typename _It>
	_It decode_to(const char* text,_It out,const decode_options& options=decode_options()) const {
		return decode_to(text,std::char_traits<char>::length(text),out,options);
	}
	template <typename _It>
	_It decode_to(const char* text,std::size_t length,_It out,const decode_options& options=decode_options()) const {
		if (alphabet_data_.has_single_bit) return decode_pow2_to(alphabet_data_,text,length,out,options);
		return decode_generic_to(alphabet_data_,text,length,out,options);
	}
	template <typename _It>
	_It decode_to(const std::string& text,_It out,const decode_options& options=decode_options()) const {
		return decode_to(text.data(),text.size(),out,options);
	}
	template <typename _It>
	_It decode_to(std::string_view text,_It out,const decode_options& options=decode_options()) const {
		return decode_to(text.data(),text.size(),out,options);
	}
	std::vector<byte> decode(const char* text,const decode_options& options=decode_options()) const {
	    return decode(text,std::char_traits<char>::length(text),options);
	}
	std::vector<byte> decode(const char* text,std::size_t length,const decode_options& options=decode_options()) const {
		std::vector<byte> result;
		result.reserve(decoded_max_size(length));
		decode_to(text,length,std::back_inserter(result),options);
		return result;
	}
	std::vector<byte> decode(const std::string& text,const decode_options& options=decode_options()) const {
		return decode(text.data(),text.size(),options);
	}
	std::vector<byte> decode(std::string_view text,const decode_options& options=decode_options()) const {
		return decode(text.data(),text.size(),options);
	}
	void reset(std::string alphabet,bool uses_padding=false,char padding_char='=') {
		assign_alphabet(alphabet,uses_padding,padding_char);
	}
	void reset(const char* alphabet,bool uses_padding=false,char padding_char='=') {
		assign_alphabet(std::string(alphabet),uses_padding,padding_char);
	}
	static base make_base16_upper() {
		return base("0123456789ABCDEF",false,'=');
	}
	static base make_base16_lower() {
		return base("0123456789abcdef",false,'=');
	}
	static base make_base32() {
		return base("ABCDEFGHIJKLMNOPQRSTUVWXYZ234567",true,'=');
	}
	static base make_base64() {
		return base("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/",true,'=');
	}
	static base make_base64url() {
		return base("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_",true,'=');
	}
	static base make_base36() {
		return base("0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ",false,'=');
	}
	static base make_base58() {
		return base("123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz",false,'=');
	}
	static base make_base62() {
		return base("0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz",false,'=');
	}
};

template <bool _Padding,char _PaddingChar,char... _AlphabetChars>
class base : public base<> {
private:
	static constexpr std::size_t radix_=sizeof...(_AlphabetChars);
	static_assert(radix_>=2 && radix_<=255,"Alphabet size must be between 2 and 255.");

	inline static constexpr std::array<char,radix_> static_alphabet_={_AlphabetChars...};
	static constexpr bool has_duplicate_static_chars() noexcept {
		for (std::size_t i=0;i<radix_;i++) {
			for (std::size_t j=i+1;j<radix_;j++) if (static_alphabet_[i]==static_alphabet_[j]) return true;
		}
		return false;
	}
	static_assert(!has_duplicate_static_chars(),"Alphabet characters must be unique.");

	static constexpr bool static_alphabet_has_padding_char() noexcept {
		for (std::size_t i=0;i<radix_;i++) {
			if (static_alphabet_[i]==_PaddingChar) return true;
		}
		return false;
	}
	static_assert(!_Padding || !static_alphabet_has_padding_char(),"Padding character must not appear in alphabet.");

	static std::string make_alphabet_string() {
		return std::string(static_alphabet_.begin(),static_alphabet_.end());
	}

public:
	base() {
		this->assign_alphabet(make_alphabet_string(),_Padding,_PaddingChar);
	}

	static constexpr std::size_t static_radix=radix_;
	static constexpr bool static_uses_padding=_Padding;
	static constexpr char static_padding_char=_PaddingChar;

	[[nodiscard]]
	static std::string alphabet_static() {
		return make_alphabet_string();
	}

	[[nodiscard]]
	static decode_options strict_decode_options() noexcept {
		return decode_options{};
	}
};

}

using base16_upper=base::base<false,'=','0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'>;
using base16_lower=base::base<false,'=','0','1','2','3','4','5','6','7','8','9','a','b','c','d','e','f'>;
using base16=base16_upper;
using base32=base::base<true,'=','A','B','C','D','E','F','G','H','I','J','K','L','M','N','O','P','Q','R','S','T','U','V','W','X','Y','Z','2','3','4','5','6','7'>;
using base64=base::base<true,'=','A','B','C','D','E','F','G','H','I','J','K','L','M','N','O','P','Q','R','S','T','U','V','W','X','Y','Z','a','b','c','d','e','f','g','h','i','j','k','l','m','n','o','p','q','r','s','t','u','v','w','x','y','z','0','1','2','3','4','5','6','7','8','9','+','/'>;
using base64url=base::base<true,'=','A','B','C','D','E','F','G','H','I','J','K','L','M','N','O','P','Q','R','S','T','U','V','W','X','Y','Z','a','b','c','d','e','f','g','h','i','j','k','l','m','n','o','p','q','r','s','t','u','v','w','x','y','z','0','1','2','3','4','5','6','7','8','9','-','_'>;
using base36=base::base<false,'=','0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F','G','H','I','J','K','L','M','N','O','P','Q','R','S','T','U','V','W','X','Y','Z'>;
using base58=base::base<false,'=','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F','G','H','J','K','L','M','N','P','Q','R','S','T','U','V','W','X','Y','Z','a','b','c','d','e','f','g','h','i','j','k','m','n','o','p','q','r','s','t','u','v','w','x','y','z'>;
using base62=base::base<false,'=','0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F','G','H','I','J','K','L','M','N','O','P','Q','R','S','T','U','V','W','X','Y','Z','a','b','c','d','e','f','g','h','i','j','k','l','m','n','o','p','q','r','s','t','u','v','w','x','y','z'>;

}

}

#endif