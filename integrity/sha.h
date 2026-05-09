//Last Modified At 2026/05/10
//@Version 1.0.1.0
#ifndef _STDEX_INTEGRITY_SHA_H_
#define _STDEX_INTEGRITY_SHA_H_ 1

#include <array>
#include <climits>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iomanip>
#include <iterator>
#include <sstream>
#include <string>
#include <type_traits>

#include "../bitwise/bit_reader.h"
#include "../bitwise/bit_writer.h"
#include "../bitwise/bits.h"

#if __has_include("../macros/cpp_version.h")
#include "../macros/cpp_version.h" // At Least 1.0
#endif

#ifndef _STDEX_CPP20_VERSION
#define _STDEX_CPP20_VERSION 202002L
#endif

#if __cplusplus>=_STDEX_CPP20_VERSION
	#include <span>
#endif

namespace stdex {

namespace integrity {

namespace sha {

template <std::size_t _Size>
inline std::string to_hex_string(const std::array<std::uint8_t,_Size>& digest) {
	std::ostringstream oss;
	oss<<std::hex<<std::setfill('0');
	for (std::size_t i=0;i<_Size;i++) oss<<std::setw(2)<<static_cast<unsigned int>(digest[i]);
	return oss.str();
}

template <typename _Word>
struct sigma_traits;

template <>
struct sigma_traits<std::uint32_t> {
	static constexpr std::size_t rounds_=64;
	static constexpr std::size_t block_size_=64;
	static constexpr std::size_t length_field_size_=8;
	static constexpr std::uint32_t k_[64]={
		0x428a2f98u,0x71374491u,0xb5c0fbcfu,0xe9b5dba5u,0x3956c25bu,0x59f111f1u,0x923f82a4u,0xab1c5ed5u,
		0xd807aa98u,0x12835b01u,0x243185beu,0x550c7dc3u,0x72be5d74u,0x80deb1feu,0x9bdc06a7u,0xc19bf174u,
		0xe49b69c1u,0xefbe4786u,0x0fc19dc6u,0x240ca1ccu,0x2de92c6fu,0x4a7484aau,0x5cb0a9dcu,0x76f988dau,
		0x983e5152u,0xa831c66du,0xb00327c8u,0xbf597fc7u,0xc6e00bf3u,0xd5a79147u,0x06ca6351u,0x14292967u,
		0x27b70a85u,0x2e1b2138u,0x4d2c6dfcu,0x53380d13u,0x650a7354u,0x766a0abbu,0x81c2c92eu,0x92722c85u,
		0xa2bfe8a1u,0xa81a664bu,0xc24b8b70u,0xc76c51a3u,0xd192e819u,0xd6990624u,0xf40e3585u,0x106aa070u,
		0x19a4c116u,0x1e376c08u,0x2748774cu,0x34b0bcb5u,0x391c0cb3u,0x4ed8aa4au,0x5b9cca4fu,0x682e6ff3u,
		0x748f82eeu,0x78a5636fu,0x84c87814u,0x8cc70208u,0x90befffau,0xa4506cebu,0xbef9a3f7u,0xc67178f2u
	};
	static constexpr std::uint32_t big_sigma0(std::uint32_t x) noexcept {
		return bitwise::rotate_right(x,2)^bitwise::rotate_right(x,13)^bitwise::rotate_right(x,22);
	}
	static constexpr std::uint32_t big_sigma1(std::uint32_t x) noexcept {
		return bitwise::rotate_right(x,6)^bitwise::rotate_right(x,11)^bitwise::rotate_right(x,25);
	}
	static constexpr std::uint32_t small_sigma0(std::uint32_t x) noexcept {
		return bitwise::rotate_right(x,7)^bitwise::rotate_right(x,18)^(x>>3);
	}
	static constexpr std::uint32_t small_sigma1(std::uint32_t x) noexcept {
		return bitwise::rotate_right(x,17)^bitwise::rotate_right(x,19)^(x>>10);
	}
};

template <>
struct sigma_traits<std::uint64_t> {
	static constexpr std::size_t rounds_=80;
	static constexpr std::size_t block_size_=128;
	static constexpr std::size_t length_field_size_=16;

	static constexpr std::uint64_t k_[80]={
		0x428a2f98d728ae22ull,0x7137449123ef65cdull,0xb5c0fbcfec4d3b2full,0xe9b5dba58189dbbcull,
		0x3956c25bf348b538ull,0x59f111f1b605d019ull,0x923f82a4af194f9bull,0xab1c5ed5da6d8118ull,
		0xd807aa98a3030242ull,0x12835b0145706fbeull,0x243185be4ee4b28cull,0x550c7dc3d5ffb4e2ull,
		0x72be5d74f27b896full,0x80deb1fe3b1696b1ull,0x9bdc06a725c71235ull,0xc19bf174cf692694ull,
		0xe49b69c19ef14ad2ull,0xefbe4786384f25e3ull,0x0fc19dc68b8cd5b5ull,0x240ca1cc77ac9c65ull,
		0x2de92c6f592b0275ull,0x4a7484aa6ea6e483ull,0x5cb0a9dcbd41fbd4ull,0x76f988da831153b5ull,
		0x983e5152ee66dfabull,0xa831c66d2db43210ull,0xb00327c898fb213full,0xbf597fc7beef0ee4ull,
		0xc6e00bf33da88fc2ull,0xd5a79147930aa725ull,0x06ca6351e003826full,0x142929670a0e6e70ull,
		0x27b70a8546d22ffcult,0x2e1b21385c26c926ull,0x4d2c6dfc5ac42aedull,0x53380d139d95b3dfull,
		0x650a73548baf63deull,0x766a0abb3c77b2a8ull,0x81c2c92e47edaee6ull,0x92722c851482353bull,
		0xa2bfe8a14cf10364ull,0xa81a664bbc423001ull,0xc24b8b70d0f89791ull,0xc76c51a30654be30ull,
		0xd192e819d6ef5218ull,0xd69906245565a910ull,0xf40e35855771202aull,0x106aa07032bbd1b8ull,
		0x19a4c116b8d2d0c8ull,0x1e376c085141ab53ull,0x2748774cdf8eeb99ull,0x34b0bcb5e19b48a8ull,
		0x391c0cb3c5c95a63ull,0x4ed8aa4ae3418acbull,0x5b9cca4f7763e373ull,0x682e6ff3d6b2b8a3ull,
		0x748f82ee5defb2fcull,0x78a5636f43172f60ull,0x84c87814a1f0ab72ull,0x8cc702081a6439ecull,
		0x90befff23631e28bull,0xa4506cebde82bde9ull,0xbef9a3f7b2c67915ull,0xc67178f2e372532bull,
		0xca273eceea26619cull,0xd186b8c721c0c207ull,0xeada7dd6cde0eb1eull,0xf57d4f7fee6ed178ull,
		0x06f067aa72176fbaull,0x0a637dc5a2c898a6ull,0x113f9804bef90daeull,0x1b710b35131c471bull,
		0x28db77f523047d84ull,0x32caab7b40c72493ull,0x3c9ebe0a15c9bebcull,0x431d67c49c100d4cull,
		0x4cc5d4becb3e42b6ull,0x597f299cfc657e2aull,0x5fcb6fab3ad6faecull,0x6c44198c4a475817ull
	};

	static constexpr std::uint64_t big_sigma0(std::uint64_t x) noexcept {
		return bitwise::rotate_right(x,28)^bitwise::rotate_right(x,34)^bitwise::rotate_right(x,39);
	}
	static constexpr std::uint64_t big_sigma1(std::uint64_t x) noexcept {
		return bitwise::rotate_right(x,14)^bitwise::rotate_right(x,18)^bitwise::rotate_right(x,41);
	}
	static constexpr std::uint64_t small_sigma0(std::uint64_t x) noexcept {
		return bitwise::rotate_right(x,1)^bitwise::rotate_right(x,8)^(x>>7);
	}
	static constexpr std::uint64_t small_sigma1(std::uint64_t x) noexcept {
		return bitwise::rotate_right(x,19)^bitwise::rotate_right(x,61)^(x>>6);
	}
};

template <std::size_t _Bits>
struct sha2_traits;

template <>
struct sha2_traits<224> {
	using word_type=std::uint32_t;
	static constexpr std::size_t bits_=224;
	static constexpr std::size_t digest_size_=28;
	static constexpr std::array<word_type,8> initial_state_={{
		0xc1059ed8u,0x367cd507u,0x3070dd17u,0xf70e5939u,
		0xffc00b31u,0x68581511u,0x64f98fa7u,0xbefa4fa4u
	}};
};

template <>
struct sha2_traits<256> {
	using word_type=std::uint32_t;
	static constexpr std::size_t bits_=256;
	static constexpr std::size_t digest_size_=32;
	static constexpr std::array<word_type,8> initial_state_={{
		0x6a09e667u,0xbb67ae85u,0x3c6ef372u,0xa54ff53au,
		0x510e527fu,0x9b05688cu,0x1f83d9abu,0x5be0cd19u
	}};
};

template <>
struct sha2_traits<384> {
	using word_type=std::uint64_t;
	static constexpr std::size_t bits_=384;
	static constexpr std::size_t digest_size_=48;
	static constexpr std::array<word_type,8> initial_state_={{
		0xcbbb9d5dc1059ed8ull,0x629a292a367cd507ull,0x9159015a3070dd17ull,0x152fecd8f70e5939ull,
		0x67332667ffc00b31ull,0x8eb44a8768581511ull,0xdb0c2e0d64f98fa7ull,0x47b5481dbefa4fa4ull
	}};
};

template <>
struct sha2_traits<512> {
	using word_type=std::uint64_t;
	static constexpr std::size_t bits_=512;
	static constexpr std::size_t digest_size_=64;
	static constexpr std::array<word_type,8> initial_state_={{
		0x6a09e667f3bcc908ull,0xbb67ae8584caa73bull,0x3c6ef372fe94f82bull,0xa54ff53a5f1d36f1ull,
		0x510e527fade682d1ull,0x9b05688c2b3e6c1full,0x1f83d9abfb41bd6bull,0x5be0cd19137e2179ull
	}};
};

template <>
struct sha2_traits<512224> {
	using word_type=std::uint64_t;
	static constexpr std::size_t bits_=512224;
	static constexpr std::size_t digest_size_=28;
	static constexpr std::array<word_type,8> initial_state_={{
		0x8C3D37C819544DA2ull,0x73E1996689DCD4D6ull,0x1DFAB7AE32FF9C82ull,0x679DD514582F9FCFull,
		0x0F6D2B697BD44DA8ull,0x77E36F7304C48942ull,0x3F9D85A86A1D36C8ull,0x1112E6AD91D692A1ull
	}};
};

template <>
struct sha2_traits<512256> {
	using word_type=std::uint64_t;
	static constexpr std::size_t bits_=512256;
	static constexpr std::size_t digest_size_=32;
	static constexpr std::array<word_type,8> initial_state_={{
		0x22312194FC2BF72Cull,0x9F555FA3C84C64C2ull,0x2393B86B6F53B151ull,0x963877195940EABDull,
		0x96283EE2A88EFFE3ull,0xBE5E1E2553863992ull,0x2B0199FC2C85B8AAull,0x0EB72DDC81C52CA2ull
	}};
};

template <std::size_t _Bits>
struct sha3_traits;

template <>
struct sha3_traits<224> {
	static constexpr std::size_t bits_=224;
	static constexpr std::size_t digest_size_=28;
	static constexpr std::size_t rate_=144;
	static constexpr std::uint8_t domain_=0x06;
};

template <>
struct sha3_traits<256> {
	static constexpr std::size_t bits_=256;
	static constexpr std::size_t digest_size_=32;
	static constexpr std::size_t rate_=136;
	static constexpr std::uint8_t domain_=0x06;
};

template <>
struct sha3_traits<384> {
	static constexpr std::size_t bits_=384;
	static constexpr std::size_t digest_size_=48;
	static constexpr std::size_t rate_=104;
	static constexpr std::uint8_t domain_=0x06;
};

template <>
struct sha3_traits<512> {
	static constexpr std::size_t bits_=512;
	static constexpr std::size_t digest_size_=64;
	static constexpr std::size_t rate_=72;
	static constexpr std::uint8_t domain_=0x06;
};

template <typename _Derived,std::size_t _DigestSize>
class hash_hex_support {
public:
	using digest_type=std::array<std::uint8_t,_DigestSize>;

	[[nodiscard]]
	std::string hex_digest() const {
		return to_hex_string(static_cast<const _Derived*>(this)->digest());
	}
};

template <typename _Derived>
class hash_update_support {
public:
#if __cplusplus>=_STDEX_CPP20_VERSION
	void update(std::span<const std::byte> data) noexcept {
		static_cast<_Derived*>(this)->update(data.data(),data.size());
	}
#endif

	template <typename _It>
	void update(_It begin,_It end) noexcept {
		for (auto it=begin;it!=end;++it) {
			const std::uint8_t byte=static_cast<std::uint8_t>(*it);
			static_cast<_Derived*>(this)->update(&byte,1);
		}
	}

	template <typename _Container>
	auto update(const _Container& container) noexcept->decltype(std::data(container),std::size(container),void()) {
		static_cast<_Derived*>(this)->update(std::data(container),std::size(container));
	}
};

template <typename _Derived,typename _DigestType>
class hash_calculate_support {
public:
#if __cplusplus>=_STDEX_CPP20_VERSION
	static _DigestType calculate(std::span<const std::byte> data) noexcept {
		_Derived calculator;
		calculator.update(data);
		return calculator.digest();
	}
	static std::string hex_calculate(std::span<const std::byte> data) {
		_Derived calculator;
		calculator.update(data);
		return calculator.hex_digest();
	}
#endif

	static _DigestType calculate(const void* data,std::size_t length) noexcept {
		_Derived calculator;
		calculator.update(data,length);
		return calculator.digest();
	}
	static std::string hex_calculate(const void* data,std::size_t length) {
		_Derived calculator;
		calculator.update(data,length);
		return calculator.hex_digest();
	}

	template <typename _Container>
	static auto calculate(const _Container& container) noexcept->decltype(std::data(container),std::size(container),_DigestType()) {
		return calculate(std::data(container),std::size(container));
	}
	template <typename _Container>
	static auto hex_calculate(const _Container& container)->decltype(std::data(container),std::size(container),std::string()) {
		return hex_calculate(std::data(container),std::size(container));
	}
};

template <typename _Traits>
class sha2_core : public hash_update_support<sha2_core<_Traits>> , public hash_hex_support<sha2_core<_Traits>,_Traits::digest_size_> , public hash_calculate_support<sha2_core<_Traits>,std::array<std::uint8_t,_Traits::digest_size_>> {
public:
	using traits_type=_Traits;
	using word_type=typename traits_type::word_type;
	using digest_type=std::array<std::uint8_t,traits_type::digest_size_>;

	static constexpr std::size_t bits=traits_type::bits_;
	static constexpr std::size_t digest_size=traits_type::digest_size_;
	static constexpr std::size_t block_size=sigma_traits<word_type>::block_size_;

private:
	static constexpr std::size_t rounds_=sigma_traits<word_type>::rounds_;
	static constexpr std::size_t word_size_=sizeof(word_type);
	static constexpr std::size_t schedule_words_=rounds_;
	static constexpr std::size_t length_field_size_=sigma_traits<word_type>::length_field_size_;

	std::array<word_type,8> state_=traits_type::initial_state_;
	std::array<std::uint8_t,block_size> buffer_{};
	std::size_t buffer_size_=0;
	std::uint64_t total_size_low_=0;
	std::uint64_t total_size_high_=0;

	void add_total_size(std::size_t n) noexcept {
		const std::uint64_t old=total_size_low_;
		total_size_low_+=static_cast<std::uint64_t>(n);
		if (total_size_low_<old) total_size_high_++;
	}

	void process_block(const std::uint8_t* block) noexcept {
		word_type w[schedule_words_];
		bitwise::bit_reader reader(block,16*word_size_,bitwise::BO_MSBYTE);
		for (std::size_t i=0;i<16;i++) w[i]=reader.read_bits<word_type>(word_size_*CHAR_BIT);
		for (std::size_t i=16;i<rounds_;i++) w[i]=sigma_traits<word_type>::small_sigma1(w[i-2])+w[i-7]+sigma_traits<word_type>::small_sigma0(w[i-15])+w[i-16];
		word_type a=state_[0],b=state_[1],c=state_[2],d=state_[3];
		word_type e=state_[4],f=state_[5],g=state_[6],h=state_[7];
		for (std::size_t i=0;i<rounds_;i++) {
			const word_type S1=sigma_traits<word_type>::big_sigma1(e);
			const word_type ch=(e&f)^((~e)&g);
			const word_type temp1=h+S1+ch+sigma_traits<word_type>::k_[i]+w[i];
			const word_type S0=sigma_traits<word_type>::big_sigma0(a);
			const word_type maj=(a&b)^(a&c)^(b&c);
			const word_type temp2=S0+maj;
			h=g;
			g=f;
			f=e;
			e=d+temp1;
			d=c;
			c=b;
			b=a;
			a=temp1+temp2;
		}
		state_[0]+=a;
		state_[1]+=b;
		state_[2]+=c;
		state_[3]+=d;
		state_[4]+=e;
		state_[5]+=f;
		state_[6]+=g;
		state_[7]+=h;
	}

	void append_length_and_finalize_padding() noexcept {
		buffer_[buffer_size_++]=0x80;
		const std::size_t payload_size=block_size-length_field_size_;
		if (buffer_size_>payload_size) {
			while (buffer_size_<block_size) buffer_[buffer_size_++]=0;
			process_block(buffer_.data());
			buffer_size_=0;
		}
		while (buffer_size_<payload_size) buffer_[buffer_size_++]=0;
		if constexpr (std::is_same_v<word_type,std::uint32_t>) {
			const std::uint64_t bit_length=total_size_low_<<3;
			bitwise::bit_writer writer(bitwise::BO_MSBYTE);
			writer.write_u64(bit_length);
			std::copy(writer.buffer().begin(),writer.buffer().end(),buffer_.data()+payload_size);
		} else {
			const std::uint64_t bit_high=(total_size_high_<<3)|(total_size_low_>>61);
			const std::uint64_t bit_low=total_size_low_<<3;
			bitwise::bit_writer writer(bitwise::BO_MSBYTE);
			writer.write_u64(bit_high);
			std::copy(writer.buffer().begin(),writer.buffer().end(),buffer_.data()+payload_size);
			writer.clear();
			writer.write_u64(bit_low);
			std::copy(writer.buffer().begin(),writer.buffer().end(),buffer_.data()+payload_size+8);
		}
		process_block(buffer_.data());
		buffer_size_=0;
	}

public:
	sha2_core() noexcept=default;

	void update(const void* data,std::size_t length) noexcept {
		const auto* bytes=static_cast<const std::uint8_t*>(data);
		add_total_size(length);
		while (length>0) {
			const std::size_t n=(length<(block_size_-buffer_size_))?length:(block_size_-buffer_size_);
			std::memcpy(buffer_.data()+buffer_size_,bytes,n);
			buffer_size_+=n;
			bytes+=n;
			length-=n;
			if (buffer_size_==block_size_) {
				process_block(buffer_.data());
				buffer_size_=0;
			}
		}
	}

	[[nodiscard]]
	digest_type digest() const noexcept {
		auto copy=*this;
		copy.append_length_and_finalize_padding();
		std::array<std::uint8_t,word_size_*CHAR_BIT> full{};
		bitwise::bit_writer writer(bitwise::BO_MSBYTE);
		for (std::size_t i=0;i<8;i++) writer.write_bits(sizeof(word_type)*CHAR_BIT,copy.state_[i]);
		std::memcpy(full.data(),writer.buffer().data(),word_size_*8);
		digest_type result{};
		for (std::size_t i=0;i<digest_size;i++) result[i]=full[i];
		return result;
	}

	void reset() noexcept {
		state_=traits_type::initial_state_;
		buffer_.fill(0);
		buffer_size_=0;
		total_size_low_=0;
		total_size_high_=0;
	}
};

class sha1 : public hash_update_support<sha1> , public hash_hex_support<sha1,20> , public hash_calculate_support<sha1,std::array<std::uint8_t,20>> {
public:
	using word_type=std::uint32_t;
	using digest_type=std::array<std::uint8_t,20>;
	static constexpr std::size_t bits=160;
	static constexpr std::size_t digest_size=20;
	static constexpr std::size_t block_size=64;

private:
	std::array<word_type,5> state_{{
		0x67452301u,0xEFCDAB89u,0x98BADCFEu,0x10325476u,0xC3D2E1F0u
	}};
	std::array<std::uint8_t,block_size_> buffer_{};
	std::size_t buffer_size_=0;
	std::uint64_t total_size_=0;

	void process_block(const std::uint8_t* block) noexcept {
		std::uint32_t w[80];
		bitwise::bit_reader reader(block,64,bitwise::BO_MSBYTE);
		for (int i=0;i<16;i++) w[i]=reader.read_bits<std::uint32_t>(32);
		for (int i=16;i<80;i++) w[i]=bitwise::rotate_left<uint32_t>(w[i-3]^w[i-8]^w[i-14]^w[i-16],1);
		std::uint32_t a=state_[0],b=state_[1],c=state_[2],d=state_[3],e=state_[4];
		for (int i=0;i<80;i++) {
			std::uint32_t f=0,k=0;
			if (i<20) {
				f=(b&c)|((~b)&d);
				k=0x5A827999u;
			} else if (i<40) {
				f=b^c^d;
				k=0x6ED9EBA1u;
			} else if (i<60) {
				f=(b&c)|(b&d)|(c&d);
				k=0x8F1BBCDCu;
			} else {
				f=b^c^d;
				k=0xCA62C1D6u;
			}
			const std::uint32_t temp=bitwise::rotate_left(a,5)+f+e+k+w[i];
			e=d;
			d=c;
			c=bitwise::rotate_left(b,30);
			b=a;
			a=temp;
		}
		state_[0]+=a;
		state_[1]+=b;
		state_[2]+=c; 
		state_[3]+=d;
		state_[4]+=e;
	}

	void finalize_padding() noexcept {
		const std::uint64_t bit_length=total_size_*CHAR_BIT;
		buffer_[buffer_size_++]=0x80;
		if (buffer_size_>56) {
			while (buffer_size_<64) buffer_[buffer_size_++]=0;
			process_block(buffer_.data());
			buffer_size_=0;
		}
		while (buffer_size_<56) buffer_[buffer_size_++]=0;
		bitwise::bit_writer writer(bitwise::BO_MSBYTE);
		writer.write_bits(64,bit_length);
		std::memcpy(buffer_.data()+56,writer.buffer().data(),8);
		process_block(buffer_.data());
		buffer_size_=0;
	}

public:
	sha1() noexcept=default;

	void update(const void* data,std::size_t length) noexcept {
		const auto* bytes=static_cast<const std::uint8_t*>(data);
		total_size_+=length;
		while (length>0) {
			const std::size_t n=(length<(block_size-buffer_size_))?length:(block_size-buffer_size_);
			std::memcpy(buffer_.data()+buffer_size_,bytes,n);
			buffer_size_+=n;
			bytes+=n;
			length-=n;
			if (buffer_size_==block_size_) {
				process_block(buffer_.data());
				buffer_size_=0;
			}
		}
	}

	[[nodiscard]]
	digest_type digest() const noexcept {
		auto copy=*this;
		copy.finalize_padding();
		digest_type result{};
		bitwise::bit_writer writer(bitwise::BO_MSBYTE);
		for (std::size_t i=0;i<5;i++) writer.write_bits(32,copy.state_[i]);
		std::memcpy(result.data(),writer.buffer().data(),20);
		return result;
	}

	void reset() noexcept {
		state_={{
			0x67452301u,0xEFCDAB89u,0x98BADCFEu,0x10325476u,0xC3D2E1F0u
		}};
		buffer_.fill(0);
		buffer_size_=0;
		total_size_=0;
	}
};

template <std::size_t _Bits>
class sha2 : public sha2_core<sha2_traits<_Bits>> {
	static_assert(_Bits==224 || _Bits==256 || _Bits==384 || _Bits==512 || _Bits==512224 || _Bits==512256,"SHA-2 bits must be 224, 256, 384, 512, 512224, or 512256.");
};

template <std::size_t _Bits>
class sha3 : public hash_update_support<sha3<_Bits>> , public hash_hex_support<sha3<_Bits>,sha3_traits<_Bits>::digest_size_> , public hash_calculate_support<sha3<_Bits>,std::array<std::uint8_t,sha3_traits<_Bits>::digest_size_>> {
	static_assert(_Bits==224 || _Bits==256 || _Bits==384 || _Bits==512,"SHA3 bits must be 224, 256, 384, or 512.");
public:
	using traits_type=sha3_traits<_Bits>;
	using digest_type=std::array<std::uint8_t,traits_type::digest_size_>;

	static constexpr std::size_t bits=traits_type::bits_;
	static constexpr std::size_t digest_size=traits_type::digest_size_;
	static constexpr std::size_t block_size=traits_type::rate_;
	static constexpr std::size_t state_size=200;
	static constexpr std::size_t rate=traits_type::rate_;
	static constexpr std::uint8_t domain=traits_type::domain_;

private:
	std::array<std::uint8_t,state_size> state_{};
	std::array<std::uint8_t,rate> buffer_{};
	std::size_t buffer_size_=0;

	static constexpr std::uint64_t round_constants_[24]={
		0x0000000000000001ull,0x0000000000008082ull,0x800000000000808aull,0x8000000080008000ull,
		0x000000000000808bull,0x0000000080000001ull,0x8000000080008081ull,0x8000000000008009ull,
		0x000000000000008aull,0x0000000000000088ull,0x0000000080008009ull,0x000000008000000aull,
		0x000000008000808bull,0x800000000000008bull,0x8000000000008089ull,0x8000000000008003ull,
		0x8000000000008002ull,0x8000000000000080ull,0x000000000000800aull,0x800000008000000aull,
		0x8000000080008081ull,0x8000000000008080ull,0x0000000080000001ull,0x8000000080008008ull
	};

	static constexpr int rho_offsets_[25]={
		0,1,62,28,27,
		36,44,6,55,20,
		3,10,43,25,39,
		41,45,15,21,8,
		18,2,61,56,14
	};

	static constexpr std::uint64_t rotl64_local(std::uint64_t x,int n) noexcept {
		return n==0?x:((x<<n)|(x>>(64-n)));
	}

	void keccak_f() noexcept {
		std::uint64_t a[25];
		bitwise::bit_reader reader(state_.data(),200,bitwise::BO_LSBYTE);
		for (int i=0;i<25;i++) a[i]=reader.read_u64();
		for (int round=0;round<24;round++) {
			std::uint64_t c[5],d[5],b[25];
			for (int x=0;x<5;x++) c[x]=a[x]^a[x+5]^a[x+10]^a[x+15]^a[x+20];
			for (int x=0;x<5;x++) d[x]=c[(x+4)%5]^rotl64_local(c[(x+1)%5],1);
			for (int y=0;y<5;y++) {
				for (int x=0;x<5;x++) a[x+5*y]^=d[x];
			}
			for (int y=0;y<5;y++) {
				for (int x=0;x<5;x++) {
					const int idx=x+5*y;
					const int nx=y;
					const int ny=(2*x+3*y)%5;
					b[nx+5*ny]=rotl64_local(a[idx],rho_offsets_[idx]);
				}
			}
			for (int y=0;y<5;y++) {
				for (int x=0;x<5;x++) a[x+5*y]=b[x+5*y]^((~b[((x+1)%5)+5*y])&b[((x+2)%5)+5*y]);
			}
			a[0]^=round_constants_[round];
		}
		bitwise::bit_writer writer(bitwise::BO_LSBYTE);
		for (int i=0;i<25;i++) writer.write_u64(a[i]);
		std::memcpy(state_.data(),writer.buffer().data(),200);
	}

	void absorb_block(const std::uint8_t* block) noexcept {
		for (std::size_t i=0;i<rate_;i++) state_[i]^=block[i];
		keccak_f();
	}

	void finalize_padding() noexcept {
		buffer_[buffer_size_++]=domain_;
		while (buffer_size_<rate_) buffer_[buffer_size_++]=0;
		buffer_[rate_-1]^=0x80;
		absorb_block(buffer_.data());
		buffer_size_=0;
	}

public:
	sha3() noexcept=default;

	void update(const void* data,std::size_t length) noexcept {
		const auto* bytes=static_cast<const std::uint8_t*>(data);

		while (length>0) {
			const std::size_t n=(length<(rate_-buffer_size_))?length:(rate_-buffer_size_);
			std::memcpy(buffer_.data()+buffer_size_,bytes,n);
			buffer_size_+=n;
			bytes+=n;
			length-=n;
			if (buffer_size_==rate_) {
				absorb_block(buffer_.data());
				buffer_size_=0;
			}
		}
	}

	[[nodiscard]]
	digest_type digest() const noexcept {
		auto copy=*this;
		copy.finalize_padding();
		digest_type result{};
		for (std::size_t i=0;i<digest_size;i++) result[i]=copy.state_[i];
		return result;
	}

	void reset() noexcept {
		state_.fill(0);
		buffer_.fill(0);
		buffer_size_=0;
	}
};

template <typename _Hash>
class hmac : public hash_update_support<hmac<_Hash>> , public hash_hex_support<hmac<_Hash>,_Hash::digest_size_> , public hash_calculate_support<hmac<_Hash>,std::array<std::uint8_t,_Hash::digest_size_>> {
public:
	using hash_type=_Hash;
	using digest_type=std::array<std::uint8_t,hash_type::digest_size_>;

	static constexpr std::size_t digest_size=hash_type::digest_size_;
	static constexpr std::size_t block_size=hash_type::block_size_;

private:
	hash_type inner_;
	hash_type outer_;
	bool initialized_=false;

	void initialize_key(const void* key,std::size_t key_length) noexcept {
		std::array<std::uint8_t,block_size> key_block{};
		if (key_length>block_size_) {
			const auto key_digest=hash_type::calculate(key,key_length);
			for (std::size_t i=0;i<digest_size;i++) key_block[i]=key_digest[i];
		} else std::memcpy(key_block.data(),key,key_length);
		std::array<std::uint8_t,block_size_> ipad{};
		std::array<std::uint8_t,block_size_> opad{};
		for (std::size_t i=0;i<block_size_;i++) {
			ipad[i]=static_cast<std::uint8_t>(key_block[i]^0x36);
			opad[i]=static_cast<std::uint8_t>(key_block[i]^0x5C);
		}
		inner_.reset();
		outer_.reset();
		inner_.update(ipad.data(),ipad.size());
		outer_.update(opad.data(),opad.size());
		initialized_=true;
	}

public:
	hmac() noexcept=default;
	hmac(const void* key,std::size_t key_length) noexcept {
		reset(key,key_length);
	}
#if __cplusplus>=_STDEX_CPP20_VERSION
	hmac(std::span<const std::byte> key) noexcept {
		reset(key);
	}
#endif
	void reset(const void* key,std::size_t key_length) noexcept {
		initialize_key(key,key_length);
	}
#if __cplusplus>=_STDEX_CPP20_VERSION
	void reset(std::span<const std::byte> key) noexcept {
		reset(key.data(),key.size());
	}
#endif
	template <typename _Container>
	auto reset(const _Container& container) noexcept->decltype(std::data(container),std::size(container),void()) {
		reset(std::data(container),std::size(container));
	}

	void update(const void* data,std::size_t length) noexcept {
		if (!initialized_) return;
		inner_.update(data,length);
	}

	[[nodiscard]]
	digest_type digest() const noexcept {
		if (!initialized_) return {};
		auto inner_copy=inner_;
		const auto inner_digest=inner_copy.digest();
		auto outer_copy=outer_;
		outer_copy.update(inner_digest.data(),inner_digest.size());
		return outer_copy.digest();
	}

	void reset() noexcept {
		inner_.reset();
		outer_.reset();
		initialized_=false;
	}

#if __cplusplus>=_STDEX_CPP20_VERSION
	static digest_type calculate(std::span<const std::byte> key,std::span<const std::byte> data) noexcept {
		hmac calculator(key);
		calculator.update(data);
		return calculator.digest();
	}
	static std::string hex_calculate(std::span<const std::byte> key,std::span<const std::byte> data) {
		hmac calculator(key);
		calculator.update(data);
		return calculator.hex_digest();
	}
#endif

	static digest_type calculate(const void* key,std::size_t key_length,const void* data,std::size_t data_length) noexcept {
		hmac calculator(key,key_length);
		calculator.update(data,data_length);
		return calculator.digest();
	}
	static std::string hex_calculate(const void* key,std::size_t key_length,const void* data,std::size_t data_length) {
		hmac calculator(key,key_length);
		calculator.update(data,data_length);
		return calculator.hex_digest();
	}
	template <typename _KeyContainer,typename _DataContainer>
	static auto calculate(const _KeyContainer& key,const _DataContainer& data) noexcept->decltype(std::data(key),std::size(key),std::data(data),std::size(data),digest_type()) {
		return calculate(std::data(key),std::size(key),std::data(data),std::size(data));
	}
	template <typename _KeyContainer,typename _DataContainer>
	static auto hex_calculate(const _KeyContainer& key,const _DataContainer& data)->decltype(std::data(key),std::size(key),std::data(data),std::size(data),std::string()) {
		return hex_calculate(std::data(key),std::size(key),std::data(data),std::size(data));
	}
};

}

using sha::sha1;
using sha224=sha::sha2<224>;
using sha256=sha::sha2<256>;
using sha384=sha::sha2<384>;
using sha512=sha::sha2<512>;
using sha512_224=sha::sha2<512224>;
using sha512_256=sha::sha2<512256>;
using sha3_224=sha::sha3<224>;
using sha3_256=sha::sha3<256>;
using sha3_384=sha::sha3<384>;
using sha3_512=sha::sha3<512>;
using hmac_sha1=sha::hmac<sha1>;
using hmac_sha224=sha::hmac<sha224>;
using hmac_sha256=sha::hmac<sha256>;
using hmac_sha384=sha::hmac<sha384>;
using hmac_sha512=sha::hmac<sha512>;
using hmac_sha512_224=sha::hmac<sha512_224>;
using hmac_sha512_256=sha::hmac<sha512_256>;
using hmac_sha3_224=sha::hmac<sha3_224>;
using hmac_sha3_256=sha::hmac<sha3_256>;
using hmac_sha3_384=sha::hmac<sha3_384>;
using hmac_sha3_512=sha::hmac<sha3_512>;

}

}

#endif