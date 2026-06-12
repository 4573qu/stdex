//Last Modified At 2024/11/13
//@Version 1.3
#ifndef _STD4573_TYPE_JSON_H_
#define _STD4573_TYPE_JSON_H_ 1
#include <algorithm>
#include <cmath>
#include <cstring>
#include <exception>
#include <initializer_list>
#include <iostream>
#include <map>
#include <memory>
#include <type_traits>
#include <vector>

namespace std::type {

namespace dtoa {

template<typename Target, typename Source>
Target reinterpret_bits(const Source source) {
	static_assert(sizeof(Target)==sizeof(Source),"size mismatch");
	Target target;
	std::memcpy(&target,&source,sizeof(Source));
	return target;
}

struct diyfp {
	static constexpr int kPrecision=64;
    std::uint64_t f=0;
    int e=0;
    constexpr diyfp(std::uint64_t f_,int e_) noexcept : f(f_) , e(e_) {}
    static diyfp sub(const diyfp& x,const diyfp& y) noexcept {
        return {x.f-y.f,x.e};
	}
    static diyfp mul(const diyfp& x,const diyfp& y) noexcept {
        static_assert(kPrecision==64,"internal error");
		const std::uint64_t u_lo=x.f&0xFFFFFFFFu;
		const std::uint64_t u_hi=x.f>>32u;
		const std::uint64_t v_lo=y.f&0xFFFFFFFFu;
		const std::uint64_t v_hi=y.f>>32u;
		const std::uint64_t p0=u_lo*v_lo;
		const std::uint64_t p1=u_lo*v_hi;
		const std::uint64_t p2=u_hi*v_lo;
		const std::uint64_t p3=u_hi*v_hi;
        const std::uint64_t p0_hi=p0>>32u;
		const std::uint64_t p1_lo=p1&0xFFFFFFFFu;
		const std::uint64_t p1_hi=p1>>32u;
		const std::uint64_t p2_lo=p2&0xFFFFFFFFu;
		const std::uint64_t p2_hi=p2>>32u;
		std::uint64_t Q=p0_hi+p1_lo+p2_lo;
		Q+=std::uint64_t{1}<<(64u-32u-1u);
		const std::uint64_t h=p3+p2_hi+p1_hi+(Q>>32u);
		return {h,x.e+y.e+64};
	}
	static diyfp normalize(diyfp x) noexcept {
        while ((x.f>>63u)==0) {
			x.f<<=1u;
			x.e--;
		}
		return x;
	}
	static diyfp normalize_to(const diyfp& x,const int target_exponent) noexcept {
		const int delta=x.e-target_exponent;
		return {x.f<<delta,target_exponent};
	}
};

struct boundaries{
    diyfp w,minus,plus;
};

template<typename FloatType>
boundaries compute_boundaries(FloatType value) {
	static_assert(std::numeric_limits<FloatType>::is_iec559,"internal error: dtoa_short requires an IEEE-754 floating-point implementation");
	constexpr int kPrecision=std::numeric_limits<FloatType>::digits;
	constexpr int kBias=std::numeric_limits<FloatType>::max_exponent-1+(kPrecision-1);
	constexpr int kMinExp=1-kBias;
	constexpr std::uint64_t kHiddenBit=std::uint64_t{1}<<(kPrecision-1);
	using bits_type=typename std::conditional<kPrecision==24,std::uint32_t,std::uint64_t>::type;
	const auto bits=static_cast<std::uint64_t>(reinterpret_bits<bits_type>(value));
	const std::uint64_t E=bits>>(kPrecision-1);
	const std::uint64_t F=bits&(kHiddenBit-1);
	const bool is_denormal=E==0;
	const diyfp v=is_denormal?diyfp(F,kMinExp):diyfp(F+kHiddenBit,static_cast<int>(E)-kBias);
	const bool lower_boundary_is_closer=F==0 && E>1;
	const diyfp m_plus=diyfp(2*v.f+1,v.e-1);
	const diyfp m_minus=lower_boundary_is_closer?diyfp(4*v.f-1,v.e-2):diyfp(2*v.f-1,v.e-1);
	const diyfp w_plus=diyfp::normalize(m_plus);
	const diyfp w_minus=diyfp::normalize_to(m_minus,w_plus.e);
	return {diyfp::normalize(v),w_minus,w_plus};
}

constexpr int kAlpha=-60;
constexpr int kGamma=-32;

struct cached_power {
	std::uint64_t f;
	int e;
	int k;
};

inline cached_power get_cached_power_for_binary_exponent(int e) {
	constexpr int kCachedPowersMinDecExp=-300;
	constexpr int kCachedPowersDecStep=8;
    static constexpr std::array<cached_power,79> kCachedPowers=
    {
        {
            {0xAB70FE17C79AC6CA,-1060,-300},
            {0xFF77B1FCBEBCDC4F,-1034,-292},
            {0xBE5691EF416BD60C,-1007,-284},
            {0x8DD01FAD907FFC3C,-980,-276},
            {0xD3515C2831559A83,-954,-268},
            {0x9D71AC8FADA6C9B5,-927,-260},
            {0xEA9C227723EE8BCB,-901,-252},
            {0xAECC49914078536D,-874,-244},
            {0x823C12795DB6CE57,-847,-236},
            {0xC21094364DFB5637,-821,-228},
            {0x9096EA6F3848984F,-794,-220},
            {0xD77485CB25823AC7,-768,-212},
            {0xA086CFCD97BF97F4,-741,-204},
            {0xEF340A98172AACE5,-715,-196},
            {0xB23867FB2A35B28E,-688,-188},
            {0x84C8D4DFD2C63F3B,-661,-180},
            {0xC5DD44271AD3CDBA,-635,-172},
            {0x936B9FCEBB25C996,-608,-164},
            {0xDBAC6C247D62A584,-582,-156},
            {0xA3AB66580D5FDAF6,-555,-148},
            {0xF3E2F893DEC3F126,-529,-140},
            {0xB5B5ADA8AAFF80B8,-502,-132},
            {0x87625F056C7C4A8B,-475,-124},
            {0xC9BCFF6034C13053,-449,-116},
            {0x964E858C91BA2655,-422,-108},
            {0xDFF9772470297EBD,-396,-100},
            {0xA6DFBD9FB8E5B88F,-369,-92},
            {0xF8A95FCF88747D94,-343,-84},
            {0xB94470938FA89BCF,-316,-76},
            {0x8A08F0F8BF0F156B,-289,-68},
            {0xCDB02555653131B6,-263,-60},
            {0x993FE2C6D07B7FAC,-236,-52},
            {0xE45C10C42A2B3B06,-210,-44},
            {0xAA242499697392D3,-183,-36},
            {0xFD87B5F28300CA0E,-157,-28},
            {0xBCE5086492111AEB,-130,-20},
            {0x8CBCCC096F5088CC,-103,-12},
            {0xD1B71758E219652C,-77,-4},
            {0x9C40000000000000,-50,4},
            {0xE8D4A51000000000,-24,12},
            {0xAD78EBC5AC620000,3,20},
            {0x813F3978F8940984,30,28},
            {0xC097CE7BC90715B3,56,36},
            {0x8F7E32CE7BEA5C70,83,44},
            {0xD5D238A4ABE98068,109,52},
            {0x9F4F2726179A2245,136,60},
            {0xED63A231D4C4FB27,162,68},
            {0xB0DE65388CC8ADA8,189,76},
            {0x83C7088E1AAB65DB,216,84},
            {0xC45D1DF942711D9A,242,92},
            {0x924D692CA61BE758,269,100},
            {0xDA01EE641A708DEA,295,108},
            {0xA26DA3999AEF774A,322,116},
            {0xF209787BB47D6B85,348,124},
            {0xB454E4A179DD1877,375,132},
            {0x865B86925B9BC5C2,402,140},
            {0xC83553C5C8965D3D,428,148},
            {0x952AB45CFA97A0B3,455,156},
            {0xDE469FBD99A05FE3,481,164},
            {0xA59BC234DB398C25,508,172},
            {0xF6C69A72A3989F5C,534,180},
            {0xB7DCBF5354E9BECE,561,188},
            {0x88FCF317F22241E2,588,196},
            {0xCC20CE9BD35C78A5,614,204},
            {0x98165AF37B2153DF,641,212},
            {0xE2A0B5DC971F303A,667,220},
            {0xA8D9D1535CE3B396,694,228},
            {0xFB9B7CD9A4A7443C,720,236},
            {0xBB764C4CA7A44410,747,244},
            {0x8BAB8EEFB6409C1A,774,252},
            {0xD01FEF10A657842C,800,260},
            {0x9B10A4E5E9913129,827,268},
            {0xE7109BFBA19C0C9D,853,276},
            {0xAC2820D9623BF429,880,284},
            {0x80444B5E7AA7CF85,907,292},
            {0xBF21E44003ACDD2D,933,300},
            {0x8E679C2F5E44FF8F,960,308},
            {0xD433179D9C8CB841,986,316},
            {0x9E19DB92B4E31BA9,1013,324},
        }
    };
    const int f=kAlpha-e-1;
    const int k=(f*78913)/(1<<18)+static_cast<int>(f>0);
	const int index=(-kCachedPowersMinDecExp+k+(kCachedPowersDecStep-1))/kCachedPowersDecStep;
	const cached_power cached=kCachedPowers[static_cast<std::size_t>(index)];
	return cached;
}

inline int find_largest_pow10(const std::uint32_t n,std::uint32_t& pow10) {
    unsigned int tmp=1000000000;
    int ans=10;
    while (tmp>=10) {
    	pow10=tmp;
    	if (n>=tmp) return ans;
    	tmp/=10;
    	ans--;
	}
    pow10=1;
    return 1;
}

inline void grisu2_round(char* buf,int len,std::uint64_t dist,std::uint64_t delta,std::uint64_t rest,std::uint64_t ten_k) {
	while (rest<dist && delta-rest>=ten_k && (rest+ten_k<dist || dist-rest>rest+ten_k-dist)) {
		buf[len-1]--;
		rest+=ten_k;
	}
}

inline void grisu2_digit_gen(char* buffer,int& length,int& decimal_exponent,diyfp M_minus,diyfp w,diyfp M_plus) {
	static_assert(kAlpha>=-60,"internal error");
	static_assert(kGamma<=-32,"internal error");
	std::uint64_t delta=diyfp::sub(M_plus,M_minus).f;
	std::uint64_t dist=diyfp::sub(M_plus,w).f;
	const diyfp one(std::uint64_t{1}<<-M_plus.e,M_plus.e);
	auto p1=static_cast<std::uint32_t>(M_plus.f>>-one.e);
	std::uint64_t p2=M_plus.f&(one.f-1);
	std::uint32_t pow10{};
	const int k=find_largest_pow10(p1,pow10);
	int n=k;
	while (n>0) {
        const std::uint32_t d=p1/pow10;
        const std::uint32_t r=p1%pow10;
		buffer[length++]=static_cast<char>('0'+d);
		p1=r;
        n--;
		const std::uint64_t rest=(std::uint64_t{p1}<<-one.e)+p2;
		if (rest<=delta) {
			decimal_exponent+=n;
			const std::uint64_t ten_n=std::uint64_t{pow10}<<-one.e;
			grisu2_round(buffer,length,dist,delta,rest,ten_n);
			return;
		}
		pow10/=10;
	}
	int m=0;
	while (1) {
        p2*=10;
		const std::uint64_t d=p2>>-one.e;
		const std::uint64_t r=p2&(one.f-1);
		buffer[length++]=static_cast<char>('0'+d);
		p2=r;
		m++;
		delta*=10;
		dist *=10;
		if (p2<=delta) break;
	}
	decimal_exponent-=m;
    const std::uint64_t ten_m=one.f;
    grisu2_round(buffer,length,dist,delta,p2,ten_m);
}

inline void grisu2(char* buf,int& len,int& decimal_exponent,
                   diyfp m_minus,diyfp v,diyfp m_plus) {

    const cached_power cached=get_cached_power_for_binary_exponent(m_plus.e);
	const diyfp c_minus_k(cached.f,cached.e);
    const diyfp w=diyfp::mul(v,c_minus_k);
    const diyfp w_minus=diyfp::mul(m_minus,c_minus_k);
    const diyfp w_plus=diyfp::mul(m_plus,c_minus_k);
    const diyfp M_minus(w_minus.f+1,w_minus.e);
    const diyfp M_plus (w_plus.f-1,w_plus.e);
    decimal_exponent=-cached.k;
    grisu2_digit_gen(buf,len,decimal_exponent,M_minus,w,M_plus);
}

inline char* append_exponent(char* buf,int e) {
	if (e<0) {
        e=-e;
		*buf++='-';
	} else *buf++='+';
	auto k=static_cast<std::uint32_t>(e);
    if (k<10) {
		*buf++='0';
		*buf++=static_cast<char>('0'+k);
	} else if (k < 100) {
		*buf++=static_cast<char>('0'+k/10);
		k%=10;
		*buf++=static_cast<char>('0'+k);
	} else {
		*buf++=static_cast<char>('0'+k/100);
		k%=100;
		*buf++=static_cast<char>('0'+k/10);
		k%=10;
		*buf++=static_cast<char>('0'+k);
	}
	return buf;
}

inline char* format_buffer(char* buf,int len,int decimal_exponent,int min_exp,int max_exp) {
	const int k=len;
	const int n=len+decimal_exponent;
    if (k<=n && n<=max_exp) {
		std::memset(buf+k,'0',static_cast<size_t>(n)-static_cast<size_t>(k));
		buf[n+0]='.';
		buf[n+1]='0';
		return buf+(static_cast<size_t>(n)+2);
	}
    if (0<n && n<=max_exp) {
		std::memmove(buf+(static_cast<size_t>(n)+1),buf+n,static_cast<size_t>(k)-static_cast<size_t>(n));
		buf[n]='.';
		return buf+(static_cast<size_t>(k)+1U);
	}
    if (min_exp<n && n<=0) {
		std::memmove(buf+(2+static_cast<size_t>(-n)),buf,static_cast<size_t>(k));
		buf[0]='0';
		buf[1]='.';
		std::memset(buf+2,'0',static_cast<size_t>(-n));
		return buf+(2U+static_cast<size_t>(-n)+static_cast<size_t>(k));
	}
	if (k==1) buf+=1;
	else {
		std::memmove(buf+2,buf+1,static_cast<size_t>(k)-1);
		buf[1]='.';
		buf+=1+static_cast<size_t>(k);
    }
	*buf++='e';
	return append_exponent(buf,n-1);
}

}

template<typename FloatType>
char* to_chars(char* first,const char* last,FloatType value) {
	static_cast<void>(last);
	if (std::signbit(value)) {
		value=-value;
		*first++='-';
	}
#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
#endif
	if (value==0) {
		*first++='0';
		*first++='.';
		*first++='0';
		return first;
	}
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif
	int len=0;
	int decimal_exponent=0;
	const dtoa::boundaries w=dtoa::compute_boundaries(value);
	dtoa::grisu2(first,len,decimal_exponent,w.minus,w.w,w.plus);
    constexpr int kMinExp=-4;
    constexpr int kMaxExp=std::numeric_limits<FloatType>::digits10;
    return dtoa::format_buffer(first,len,decimal_exponent,kMinExp,kMaxExp);
}

////////INPUT
class file_input_adapter {
public:
	using char_type=char;
	explicit file_input_adapter(std::FILE* f) noexcept : m_file(f) { }
	file_input_adapter(const file_input_adapter&)=delete;
	file_input_adapter(file_input_adapter&&) noexcept=default;
	file_input_adapter& operator=(const file_input_adapter&)=delete;
	file_input_adapter& operator=(file_input_adapter&&)=delete;
	~file_input_adapter()=default;
	std::char_traits<char>::int_type get_character() noexcept {
		return std::fgetc(m_file);
	}
private:
	std::FILE* m_file;
};

class input_stream_adapter {
public:
	using char_type=char;
	~input_stream_adapter() {
		if (is) is->clear(is->rdstate() & std::ios::eofbit);
	}
	explicit input_stream_adapter(std::istream& i) : is(&i) , sb(i.rdbuf()) {}
	input_stream_adapter(const input_stream_adapter&)=delete;
	input_stream_adapter& operator=(input_stream_adapter&)=delete;
	input_stream_adapter& operator=(input_stream_adapter&&)=delete;
	input_stream_adapter(input_stream_adapter&& rhs) noexcept : is(rhs.is) , sb(rhs.sb) {
        rhs.is=nullptr;
		rhs.sb=nullptr;
	}
	std::char_traits<char>::int_type get_character() {
		auto res=sb->sbumpc();
		if (res==std::char_traits<char>::eof()) is->clear(is->rdstate()|std::ios::eofbit);
		return res;
	}
private:
	std::istream* is=nullptr;
	std::streambuf* sb=nullptr;
};

template<typename IteratorType>
class iterator_input_adapter {
public:
    using char_type=typename std::iterator_traits<IteratorType>::value_type;
	iterator_input_adapter(IteratorType first,IteratorType last) : current(std::move(first)) , end(std::move(last)) {}
	typename char_traits<char_type>::int_type get_character() {
		if (current!=end) {
			auto result=char_traits<char_type>::to_int_type(*current);
			std::advance(current, 1);
			return result;
		}
		return char_traits<char_type>::eof();
	}
private:
	IteratorType current;
	IteratorType end;
	template<typename BaseInputAdapter,size_t _Tp>
	friend struct wide_string_input_helper;
	bool empty() const {
        return current==end;
	}
};

template<typename BaseInputAdapter,size_t _Tp>
struct wide_string_input_helper;

template<typename BaseInputAdapter>
struct wide_string_input_helper<BaseInputAdapter,4>{
	static void fill_buffer(BaseInputAdapter& input,std::array<std::char_traits<char>::int_type,4>& utf8_bytes,size_t& utf8_bytes_index,size_t& utf8_bytes_filled) {
		utf8_bytes_index = 0;
		if (input.empty()) {
			utf8_bytes[0]=std::char_traits<char>::eof();
			utf8_bytes_filled = 1;
		} else {
			const auto wc=input.get_character();
			if (wc<0x80) {
				utf8_bytes[0]=static_cast<std::char_traits<char>::int_type>(wc);
				utf8_bytes_filled=1;
			} else if (wc<=0x7FF) {
				utf8_bytes[0]=static_cast<std::char_traits<char>::int_type>(0xC0u|((static_cast<unsigned int>(wc)>>6u)&0x1Fu));
				utf8_bytes[1]=static_cast<std::char_traits<char>::int_type>(0x80u|(static_cast<unsigned int>(wc)&0x3Fu));
				utf8_bytes_filled=2;
			} else if (wc<=0xFFFF) {
				utf8_bytes[0]=static_cast<std::char_traits<char>::int_type>(0xE0u|((static_cast<unsigned int>(wc)>>12u)&0x0Fu));
				utf8_bytes[1]=static_cast<std::char_traits<char>::int_type>(0x80u|((static_cast<unsigned int>(wc)>>6u)&0x3Fu));
				utf8_bytes[2]=static_cast<std::char_traits<char>::int_type>(0x80u|(static_cast<unsigned int>(wc)&0x3Fu));
				utf8_bytes_filled=3;
			} else if (wc<=0x10FFFF) {
				utf8_bytes[0]=static_cast<std::char_traits<char>::int_type>(0xF0u|((static_cast<unsigned int>(wc)>>18u)&0x07u));
				utf8_bytes[1]=static_cast<std::char_traits<char>::int_type>(0x80u|((static_cast<unsigned int>(wc)>>12u)&0x3Fu));
				utf8_bytes[2]=static_cast<std::char_traits<char>::int_type>(0x80u|((static_cast<unsigned int>(wc)>>6u)&0x3Fu));
				utf8_bytes[3]=static_cast<std::char_traits<char>::int_type>(0x80u|(static_cast<unsigned int>(wc)&0x3Fu));
				utf8_bytes_filled=4;
			} else {
				utf8_bytes[0]=static_cast<std::char_traits<char>::int_type>(wc);
				utf8_bytes_filled=1;
			}
		}
	}
};

template<typename BaseInputAdapter>
struct wide_string_input_helper<BaseInputAdapter,2> {
    static void fill_buffer(BaseInputAdapter& input,std::array<std::char_traits<char>::int_type,4>& utf8_bytes,size_t& utf8_bytes_index,size_t& utf8_bytes_filled) {
		utf8_bytes_index=0;
		if (input.empty()) {
			utf8_bytes[0]=std::char_traits<char>::eof();
			utf8_bytes_filled = 1;
		} else {
			const auto wc=input.get_character();
            if (wc<0x80) {
				utf8_bytes[0]=static_cast<std::char_traits<char>::int_type>(wc);
				utf8_bytes_filled=1;
			} else if (wc<=0x7FF) {
				utf8_bytes[0]=static_cast<std::char_traits<char>::int_type>(0xC0u|((static_cast<unsigned int>(wc)>>6u)&0x1Fu));
				utf8_bytes[1]=static_cast<std::char_traits<char>::int_type>(0x80u|(static_cast<unsigned int>(wc)&0x3Fu));
				utf8_bytes_filled=2;
			}  else if (0xD800>wc || wc>=0xE000) {
				utf8_bytes[0]=static_cast<std::char_traits<char>::int_type>(0xE0u|((static_cast<unsigned int>(wc)>>12u)));
				utf8_bytes[1]=static_cast<std::char_traits<char>::int_type>(0x80u|((static_cast<unsigned int>(wc)>>6u)&0x3Fu));
				utf8_bytes[2]=static_cast<std::char_traits<char>::int_type>(0x80u|(static_cast<unsigned int>(wc)&0x3Fu));
				utf8_bytes_filled=3;
			} else {
				if (!input.empty()) {
					const auto wc2=static_cast<unsigned int>(input.get_character());
					const auto charcode=0x10000u + (((static_cast<unsigned int>(wc)&0x3FFu)<<10u)|(wc2&0x3FFu));
					utf8_bytes[0]=static_cast<std::char_traits<char>::int_type>(0xF0u|(charcode>>18u));
                    utf8_bytes[1]=static_cast<std::char_traits<char>::int_type>(0x80u|((charcode>>12u)&0x3Fu));
					utf8_bytes[2]=static_cast<std::char_traits<char>::int_type>(0x80u|((charcode>>6u)&0x3Fu));
					utf8_bytes[3]=static_cast<std::char_traits<char>::int_type>(0x80u|(charcode&0x3Fu));
					utf8_bytes_filled=4;
				} else {
					utf8_bytes[0]=static_cast<std::char_traits<char>::int_type>(wc);
					utf8_bytes_filled=1;
				}
			}
		}
	}
};

template<typename BaseInputAdapter,typename WideCharType>
class wide_string_input_adapter {
public:
	using char_type=char;
	wide_string_input_adapter(BaseInputAdapter base) : base_adapter(base) {}
	typename std::char_traits<char>::int_type get_character() noexcept {
		if (utf8_bytes_index==utf8_bytes_filled) fill_buffer<sizeof(WideCharType)>();
		return utf8_bytes[utf8_bytes_index++];
	}
private:
    BaseInputAdapter base_adapter;
	template<size_t _Tp>
	void fill_buffer() {
		wide_string_input_helper<BaseInputAdapter,_Tp>::fill_buffer(base_adapter,utf8_bytes,utf8_bytes_index,utf8_bytes_filled);
	}
    std::array<std::char_traits<char>::int_type,4> utf8_bytes={{0,0,0,0}};
    std::size_t utf8_bytes_index=0;
    std::size_t utf8_bytes_filled=0;
};

template<typename IteratorType,typename Enable=void>
struct iterator_input_adapter_factory {
	using iterator_type=IteratorType;
	using char_type=typename std::iterator_traits<iterator_type>::value_type;
	using adapter_type=iterator_input_adapter<iterator_type>;
	static adapter_type create(IteratorType first,IteratorType last) {
		return adapter_type(std::move(first),std::move(last));
	}
};

template<typename _Tp>
struct is_iterator_of_multibyte {
	using value_type=typename std::iterator_traits<_Tp>::value_type;
	enum { value=sizeof(value_type)>1 };
};

template<typename IteratorType>
struct iterator_input_adapter_factory<IteratorType,enable_if_t<is_iterator_of_multibyte<IteratorType>::value>> {
	using iterator_type=IteratorType;
	using char_type=typename std::iterator_traits<iterator_type>::value_type;
	using base_adapter_type=iterator_input_adapter<iterator_type>;
	using adapter_type=wide_string_input_adapter<base_adapter_type,char_type>;
	static adapter_type create(IteratorType first, IteratorType last) {
		return adapter_type(base_adapter_type(std::move(first),std::move(last)));
	}
};

template<typename IteratorType>
typename iterator_input_adapter_factory<IteratorType>::adapter_type input_adapter(IteratorType first,IteratorType last) {
	using factory_type=iterator_input_adapter_factory<IteratorType>;
	return factory_type::create(first,last);
}

namespace container_input_adapter_factory_impl
{

using std::begin;
using std::end;

template<typename ContainerType,typename Enable=void>
struct container_input_adapter_factory {};

template<typename ContainerType>
struct container_input_adapter_factory<ContainerType,void_t<decltype(begin(std::declval<ContainerType>()),end(std::declval<ContainerType>()))>> {
	using adapter_type=decltype(input_adapter(begin(std::declval<ContainerType>()),end(std::declval<ContainerType>())));
	static adapter_type create(const ContainerType& container) {
		return input_adapter(begin(container),end(container));
	}
};

}

template<typename ContainerType>
typename container_input_adapter_factory_impl::container_input_adapter_factory<ContainerType>::adapter_type input_adapter(const ContainerType& container) {
	return container_input_adapter_factory_impl::container_input_adapter_factory<ContainerType>::create(container);
}

inline file_input_adapter input_adapter(std::FILE* file){
	return file_input_adapter(file);
}

inline input_stream_adapter input_adapter(std::istream& stream) {
	return input_stream_adapter(stream);
}

inline input_stream_adapter input_adapter(std::istream&& stream) {
	return input_stream_adapter(stream);
}

using contiguous_bytes_input_adapter=decltype(input_adapter(std::declval<const char*>(),std::declval<const char*>()));

template<typename CharT,
		 typename std::enable_if<std::is_pointer<CharT>::value &&
								 !std::is_array<CharT>::value &&
 								 std::is_integral<typename std::remove_pointer<CharT>::type>::value &&
								 sizeof(typename std::remove_pointer<CharT>::type)==1,int>::type = 0>
contiguous_bytes_input_adapter input_adapter(CharT b) {
	auto length=std::strlen(reinterpret_cast<const char*>(b));
	const auto* ptr=reinterpret_cast<const char*>(b);
	return input_adapter(ptr,ptr+length);
}

template<typename _Tp,std::size_t N>
auto input_adapter(_Tp (&array)[N]) -> decltype(input_adapter(array,array +N)) {
	return input_adapter(array,array+N);
}

class span_input_adapter{
public:
	template<typename CharT,
			 typename std::enable_if<std::is_pointer<CharT>::value &&
									 std::is_integral<typename std::remove_pointer<CharT>::type>::value &&
									 sizeof(typename std::remove_pointer<CharT>::type)==1,int>::type = 0>
	span_input_adapter(CharT b,std::size_t l) : ia(reinterpret_cast<const char*>(b),reinterpret_cast<const char*>(b)+l) {}
	template<class IteratorType,
             typename std::enable_if<std::is_same<typename iterator_traits<IteratorType>::iterator_category,std::random_access_iterator_tag>::value,int>::type = 0>
	span_input_adapter(IteratorType first,IteratorType last) : ia(input_adapter(first,last)) {}
	contiguous_bytes_input_adapter&& get() {
		return std::move(ia);
	}
private:
	contiguous_bytes_input_adapter ia;
};

////////OUTPUT
template<typename CharType>
struct output_adapter_protocol {
	virtual void write_character(CharType c)=0;
	virtual void write_characters(const CharType* s,std::size_t length)=0;
	virtual ~output_adapter_protocol()=default;
	output_adapter_protocol()=default;
	output_adapter_protocol(const output_adapter_protocol&)=default;
	output_adapter_protocol(output_adapter_protocol&&) noexcept=default;
	output_adapter_protocol& operator =(const output_adapter_protocol&)=default;
	output_adapter_protocol& operator =(output_adapter_protocol&&) noexcept=default;
};

template<typename CharType>
using output_adapter_t=std::shared_ptr<output_adapter_protocol<CharType>>;

template<typename CharType,typename AllocatorType=std::allocator<CharType>>
class output_vector_adapter : public output_adapter_protocol<CharType> {
public:
	explicit output_vector_adapter(std::vector<CharType,AllocatorType>& vec) noexcept : v(vec) {}
	void write_character(CharType c) override { v.push_back(c); }
    void write_characters(const CharType* s,std::size_t length) override { v.insert(v.end(),s,s+length); }
private:
	std::vector<CharType,AllocatorType>& v;
};

template<typename CharType>
class output_stream_adapter : public output_adapter_protocol<CharType> {
public:
	explicit output_stream_adapter(std::basic_ostream<CharType>& s) noexcept : stream(s) {}
	void write_character(CharType c) override {
		stream.put(c);
	}
	void write_characters(const CharType* s,std::size_t length) override {
		stream.write(s,static_cast<std::streamsize>(length));
	}
private:
	std::basic_ostream<CharType>& stream;
};

template<typename CharType,typename StringType=std::basic_string<CharType>>
class output_string_adapter : public output_adapter_protocol<CharType>{
public:
	explicit output_string_adapter(StringType& s) noexcept : str(s) {}
	void write_character(CharType c) override {
		str.push_back(c);
	}
	void write_characters(const CharType* s,std::size_t length) override {
		str.append(s,length);
	}
private:
	StringType& str;
};


template<typename CharType,typename StringType=std::basic_string<CharType>>
class output_adapter {
public:
	template<typename AllocatorType=std::allocator<CharType>>
	output_adapter(std::vector<CharType, AllocatorType>& vec) : oa(std::make_shared<output_vector_adapter<CharType,AllocatorType>>(vec)) {}
	output_adapter(std::basic_ostream<CharType>& s) : oa(std::make_shared<output_stream_adapter<CharType>>(s)) {}
	output_adapter(StringType& s) : oa(std::make_shared<output_string_adapter<CharType,StringType>>(s)) {}
	operator output_adapter_t<CharType>() {
		return oa;
	}
private:
	output_adapter_t<CharType> oa=nullptr;
};

////////JSON_GENERAL
enum JSON_TYPE {
	JT_null,
	JT_int,
	JT_float,
	JT_bool,
	JT_string,
	JT_array,
	JT_object,
	JT_discarded
};

enum class PARSE_STATUS {
	PS_key,
	PS_value,
	PS_arraybegin,
	PS_arrayend,
	PS_objectbegin,
	PS_objectend,
	PS_unknown
};

template<typename JsonType>
using parser_callback_t=std::function<bool(int,PARSE_STATUS,JsonType&)>;

class json;

template<typename>
struct is_json : std::false_type {};
template<>
struct is_json<json> : std::true_type {};

template<typename JsonType>
class json_ref {
public:
	using value_type=JsonType;
	json_ref(value_type&& value) : owned_value(std::move(value)) {}
	json_ref(const value_type& value) : value_ref(&value) {}
	json_ref(std::initializer_list<json_ref> init) : owned_value(init) {}
	template <class... Args,std::enable_if_t<std::is_constructible<value_type,Args...>::value,int> = 0>
	json_ref(Args&&... args) : owned_value(std::forward<Args>(args)...) {}
	json_ref(json_ref&&) noexcept=default;
	json_ref(const json_ref&)=delete;
	json_ref& operator =(const json_ref&)=delete;
	json_ref& operator =(json_ref&&)=delete;
	~json_ref()=default;
	value_type moved_or_copied() const {
		if (!value_ref) return std::move(owned_value);
		return *value_ref;
	}
    value_type const& operator *() const {
		return value_ref?*value_ref:owned_value;
	}
	value_type const* operator ->() const {
		return &**this;
	}
private:
	mutable value_type owned_value=nullptr;
	value_type const* value_ref=nullptr;
};

////////SERIALIZER
template<typename JsonType>
class serializer {
public:
	enum class error_handler_t {
		strict,
		replace,
		ignore
	};
	using string_t=typename JsonType::string_t;
	using float_t=typename JsonType::float_t;
	using int_t=typename JsonType::int_t;
    
private:
	static constexpr std::uint8_t UTF8_ACCEPT=0;
	static constexpr std::uint8_t UTF8_REJECT=1;
    output_adapter_t<char> o=nullptr;
	std::array<char,64> number_buffer{{}};
	const std::lconv* loc=nullptr;
	const char thousands_sep='\0';
	const char decimal_point='\0';
	std::array<char,512> string_buffer{{}};
	const char indent_char;
	string_t indent_string;
	const error_handler_t error_handler;

public:
	serializer(output_adapter_t<char> s,
			   const char ichar,
			   error_handler_t error_handler_=error_handler_t::strict) :
			   o(std::move(s)) ,
			   loc(std::localeconv()) ,
			   thousands_sep(!loc->thousands_sep?'\0':std::char_traits<char>::to_char_type(*(loc->thousands_sep))) ,
			   decimal_point(!loc->decimal_point?'\0':std::char_traits<char>::to_char_type(*(loc->decimal_point))) ,
			   indent_char(ichar) ,
			   indent_string(512,indent_char) ,
			   error_handler(error_handler_) {}
	serializer(const serializer&)=delete;
	serializer& operator =(const serializer&)=delete;
	serializer(serializer&&)=delete;
	serializer& operator =(serializer&&)=delete;
	~serializer()=default;

    void dump(const JsonType& val,const bool pretty_print,const bool ensure_ascii,const unsigned int indent_step,const unsigned int current_indent=0) {
		switch (val.mdata_.type_) {
			case JSON_TYPE::JT_object: {
				if (val.mdata_.val_.object_->empty()) {
					o->write_characters("{}",2);
					return;
				}
				if (pretty_print) {
					o->write_characters("{\n",2);
					const auto new_indent=current_indent+indent_step;
					if (indent_string.size()<new_indent) indent_string.resize(indent_string.size()*2,' ');
					auto i=val.mdata_.val_.object_->cbegin();
					for (std::size_t cnt=0;cnt<val.mdata_.val_.object_->size()-1;cnt++,i++) {
						o->write_characters(indent_string.c_str(),new_indent);
						o->write_character('\"');
						dump_escaped(i->first,ensure_ascii);
						o->write_characters("\": ",3);
						dump(i->second,true,ensure_ascii,indent_step,new_indent);
						o->write_characters(",\n",2);
					}
					o->write_characters(indent_string.c_str(),new_indent);
					o->write_character('\"');
					dump_escaped(i->first,ensure_ascii);
					o->write_characters("\": ",3);
					dump(i->second,true,ensure_ascii,indent_step,new_indent);
					o->write_character('\n');
					o->write_characters(indent_string.c_str(),current_indent);
					o->write_character('}');
				} else {
					o->write_character('{');
					auto i=val.mdata_.val_.object_->cbegin();
					for (std::size_t cnt=0;cnt<val.mdata_.val_.object_->size()-1;cnt++,i++) {
						o->write_character('\"');
						dump_escaped(i->first,ensure_ascii);
						o->write_characters("\":",2);
 						dump(i->second,false,ensure_ascii,indent_step,current_indent);
						o->write_character(',');
					}
					o->write_character('\"');
					dump_escaped(i->first,ensure_ascii);
					o->write_characters("\":",2);
					dump(i->second,false,ensure_ascii,indent_step,current_indent);
					o->write_character('}');
				}
				return;
			}
			case JSON_TYPE::JT_array: {
				if (val.mdata_.val_.array_->empty()) {
					o->write_characters("[]",2);
					return;
				}
				if (pretty_print) {
					o->write_characters("[\n",2);
                    const auto new_indent=current_indent+indent_step;
                    if (indent_string.size()<new_indent) indent_string.resize(indent_string.size()*2,' ');
					for (auto i=val.mdata_.val_.array_->cbegin();i!=val.mdata_.val_.array_->cend()-1;i++) {
						o->write_characters(indent_string.c_str(),new_indent);
						dump(*i,true,ensure_ascii,indent_step,new_indent);
						o->write_characters(",\n",2);
					}
					o->write_characters(indent_string.c_str(),new_indent);
					dump(val.mdata_.val_.array_->back(),true,ensure_ascii,indent_step,new_indent);
					o->write_character('\n');
					o->write_characters(indent_string.c_str(),current_indent);
					o->write_character(']');
				} else {
					o->write_character('[');
					for (auto i=val.mdata_.val_.array_->cbegin();i!=val.mdata_.val_.array_->cend()-1;i++) {
						dump(*i,false,ensure_ascii,indent_step,current_indent);
						o->write_character(',');
					}
					dump(val.mdata_.val_.array_->back(),false,ensure_ascii,indent_step,current_indent);
					o->write_character(']');
				}
				 return;
			}
			case JSON_TYPE::JT_string: {
				o->write_character('\"');
				dump_escaped(*val.mdata_.val_.string_,ensure_ascii);
				o->write_character('\"');
				return;
			}
			case JSON_TYPE::JT_bool: {
				if (val.mdata_.val_.boolean_) o->write_characters("true",4);
				else o->write_characters("false",5);
				return;
			}
			case JSON_TYPE::JT_int: {
				dump_integer(val.mdata_.val_.integer_);
				return;
			}
			case JSON_TYPE::JT_float: {
				dump_float(val.mdata_.val_.float_);
				return;
			}
			case JSON_TYPE::JT_null: {
				o->write_characters("null",4);
				return;
			}
			default: {
				break;
			}
		}
	}

private:
	void dump_escaped(const string_t& s,const bool ensure_ascii) {
		std::uint32_t codepoint{};
		std::uint8_t state=UTF8_ACCEPT;
		std::size_t bytes=0;
        std::size_t bytes_after_last_accept=0;
        std::size_t undumped_chars=0;
		for (std::size_t i=0;i<s.size();i++) {
			const auto byte=static_cast<std::uint8_t>(s[i]);
			switch (decode(state,codepoint,byte)) {
				case UTF8_ACCEPT: {
					switch (codepoint) {
                        case 0x08: {
							string_buffer[bytes++]='\\';
							string_buffer[bytes++]='b';
							break;
						}
						case 0x09: {
							string_buffer[bytes++]='\\';
 							string_buffer[bytes++]='t';
							break;
						}
						case 0x0A: {
							string_buffer[bytes++]='\\';
							string_buffer[bytes++]='n';
							break;
						}
						case 0x0C: {
							string_buffer[bytes++]='\\';
							string_buffer[bytes++]='f';
							break;
						}
						case 0x0D: {
							string_buffer[bytes++]='\\';
							string_buffer[bytes++]='r';
							break;
						}
						case 0x22:{
							string_buffer[bytes++]='\\';
							string_buffer[bytes++]='\"';
							break;
						}
						case 0x5C: {
							string_buffer[bytes++]='\\';
							string_buffer[bytes++]='\\';
							break;
						}
						default: {
							if ((codepoint<=0x1F) || (ensure_ascii && (codepoint>=0x7F))) {
  								if (codepoint<=0xFFFF) {
									static_cast<void>((std::snprintf)(string_buffer.data()+bytes,7,"\\u%04x",
																	  static_cast<std::uint16_t>(codepoint)));
									bytes+=6;
								} else {
									static_cast<void>((std::snprintf)(string_buffer.data()+bytes,13,"\\u%04x\\u%04x",
																	  static_cast<std::uint16_t>(0xD7C0u+(codepoint>>10u)),
																	  static_cast<std::uint16_t>(0xDC00u+(codepoint&0x3FFu))));
									bytes+=12;
								}
							} else string_buffer[bytes++]=s[i];
							break;
						}
					}
					if (string_buffer.size()-bytes<13) {
						o->write_characters(string_buffer.data(),bytes);
						bytes=0;
					}
					bytes_after_last_accept=bytes;
					undumped_chars=0;
					break;
				}
				case UTF8_REJECT: {
					switch (error_handler) {
						case error_handler_t::strict: {
							throw std::invalid_argument("invalid UTF-8 byte at index "+std::to_string(i)+": 0x"+hex_bytes(byte|0));
						}
						case error_handler_t::ignore:
						case error_handler_t::replace: {
							if (undumped_chars>0) i--;
							bytes=bytes_after_last_accept;
							if (error_handler==error_handler_t::replace) {
								if (ensure_ascii) {
									string_buffer[bytes++]='\\';
									string_buffer[bytes++]='u';
									string_buffer[bytes++]='f';
									string_buffer[bytes++]='f';
									string_buffer[bytes++]='f';
									string_buffer[bytes++]='d';
								}/*else {
									string_buffer[bytes++] = detail::binary_writer<BasicJsonType, char>::to_char_type('\xEF');
									string_buffer[bytes++] = detail::binary_writer<BasicJsonType, char>::to_char_type('\xBF');
									string_buffer[bytes++] = detail::binary_writer<BasicJsonType, char>::to_char_type('\xBD');
                                }*/
								if (string_buffer.size()-bytes<13) {
									o->write_characters(string_buffer.data(),bytes);
									bytes=0;
								}
								bytes_after_last_accept=bytes;
							}
							undumped_chars=0;
							state=UTF8_ACCEPT;
							break;
						}
						default: {
							break;
						}
					}
					break;
				}
				default: {
					if (!ensure_ascii) string_buffer[bytes++]=s[i];
					undumped_chars++;
					break;
				}
			}
		}
		if (state==UTF8_ACCEPT) {
			if (bytes>0) o->write_characters(string_buffer.data(),bytes);
		} else {
			switch (error_handler) {
				case error_handler_t::strict: {
					throw std::invalid_argument("incomplete UTF-8 string; last byte: 0x"+hex_bytes(static_cast<std::uint8_t>(s.back()|0)));
				}
				case error_handler_t::ignore: {
					o->write_characters(string_buffer.data(),bytes_after_last_accept);
					break;
				}
				case error_handler_t::replace: {
					o->write_characters(string_buffer.data(),bytes_after_last_accept);
					if (ensure_ascii) o->write_characters("\\ufffd", 6);
					else o->write_characters("\xEF\xBF\xBD",3);
					break;
				}
				default: {
					break;
				}
			}
		}
	}

private:
	inline unsigned int count_digits(int_t x) noexcept {
		unsigned int n_digits=1;
		while (x>=10) {
        	n_digits++;
        	x/=10;
		}
		return n_digits;
	}
	static std::string hex_bytes(std::uint8_t byte) {
		std::string result="FF";
		constexpr const char* nibble_to_hex="0123456789ABCDEF";
 		result[0]=nibble_to_hex[byte / 16];
		result[1]=nibble_to_hex[byte % 16];
		return result;
	}
    template <typename NumberType,std::enable_if_t<std::is_signed<NumberType>::value,int> = 0>
	bool is_negative_number(NumberType x) {
		return x<0;
	}
    template <typename NumberType,
			  std::enable_if_t<std::is_integral<NumberType>::value ||
                   			   std::is_same<NumberType,int_t>::value,int> = 0>
	void dump_integer(NumberType x) {
		static constexpr std::array<std::array<char,2>,100> digits_to_99 {
			{
				{{'0','0'}},{{'0','1'}},{{'0','2'}},{{'0','3'}},{{'0','4'}},{{'0','5'}},{{'0','6'}},{{'0','7'}},{{'0','8'}},{{'0','9'}},
				{{'1','0'}},{{'1','1'}},{{'1','2'}},{{'1','3'}},{{'1','4'}},{{'1','5'}},{{'1','6'}},{{'1','7'}},{{'1','8'}},{{'1','9'}},
				{{'2','0'}},{{'2','1'}},{{'2','2'}},{{'2','3'}},{{'2','4'}},{{'2','5'}},{{'2','6'}},{{'2','7'}},{{'2','8'}},{{'2','9'}},
				{{'3','0'}},{{'3','1'}},{{'3','2'}},{{'3','3'}},{{'3','4'}},{{'3','5'}},{{'3','6'}},{{'3','7'}},{{'3','8'}},{{'3','9'}},
				{{'4','0'}},{{'4','1'}},{{'4','2'}},{{'4','3'}},{{'4','4'}},{{'4','5'}},{{'4','6'}},{{'4','7'}},{{'4','8'}},{{'4','9'}},
				{{'5','0'}},{{'5','1'}},{{'5','2'}},{{'5','3'}},{{'5','4'}},{{'5','5'}},{{'5','6'}},{{'5','7'}},{{'5','8'}},{{'5','9'}},
				{{'6','0'}},{{'6','1'}},{{'6','2'}},{{'6','3'}},{{'6','4'}},{{'6','5'}},{{'6','6'}},{{'6','7'}},{{'6','8'}},{{'6','9'}},
				{{'7','0'}},{{'7','1'}},{{'7','2'}},{{'7','3'}},{{'7','4'}},{{'7','5'}},{{'7','6'}},{{'7','7'}},{{'7','8'}},{{'7','9'}},
				{{'8','0'}},{{'8','1'}},{{'8','2'}},{{'8','3'}},{{'8','4'}},{{'8','5'}},{{'8','6'}},{{'8','7'}},{{'8','8'}},{{'8','9'}},
				{{'9','0'}},{{'9','1'}},{{'9','2'}},{{'9','3'}},{{'9','4'}},{{'9','5'}},{{'9','6'}},{{'9','7'}},{{'9','8'}},{{'9','9'}},
			}
		};
		if (x==0) {
			o->write_character('0');
			return;
		}
		auto buffer_ptr=number_buffer.begin();
		unsigned abs_value;
		unsigned int n_chars{};
		if (x<0) {
            *buffer_ptr='-';
			abs_value =remove_sign(static_cast<int_t>(x));
			n_chars=1+count_digits(abs_value);
		} else {
			abs_value=static_cast<int_t>(x);
            n_chars=count_digits(abs_value);
		}
		buffer_ptr+=n_chars;
		while (abs_value>=100) {
			const auto digits_index=static_cast<unsigned>((abs_value%100));
			abs_value/=100;
			*(--buffer_ptr)=digits_to_99[digits_index][1];
			*(--buffer_ptr)=digits_to_99[digits_index][0];
        }
		if (abs_value>=10) {
			const auto digits_index=static_cast<unsigned>(abs_value);
			*(--buffer_ptr)=digits_to_99[digits_index][1];
			*(--buffer_ptr)=digits_to_99[digits_index][0];
        } else *(--buffer_ptr)=static_cast<char>('0'+abs_value);
		o->write_characters(number_buffer.data(),n_chars);
	}
	void dump_float(float_t x){
		if (!std::isfinite(x)) {
			if (x<0) o->write_characters("-",1);
			o->write_characters("INF",3);
            return;
        }
        if (std::isnan(x)) {
			o->write_characters("NaN",3);
			return;
		}
		static constexpr bool is_ieee_single_or_double=
			(std::numeric_limits<float_t>::is_iec559 && std::numeric_limits<float_t>::digits==24 && std::numeric_limits<float_t>::max_exponent==128) ||
			(std::numeric_limits<float_t>::is_iec559 && std::numeric_limits<float_t>::digits==53 && std::numeric_limits<float_t>::max_exponent==1024);
		dump_float(x,std::integral_constant<bool,is_ieee_single_or_double>());
	}
	void dump_float(float_t x,std::true_type) {
		auto* begin=number_buffer.data();
		auto* end=to_chars(begin,begin+number_buffer.size(),x);
		o->write_characters(begin,static_cast<size_t>(end-begin));
	}
	void dump_float(float_t x,std::false_type) {	
		static constexpr auto d=std::numeric_limits<float_t>::max_digits10;
		std::ptrdiff_t len=(std::snprintf)(number_buffer.data(),number_buffer.size(),"%.*g",d,x);
		if (thousands_sep!='\0') {
			const auto end=std::remove(number_buffer.begin(),number_buffer.begin()+len,thousands_sep);
			std::fill(end,number_buffer.end(),'\0');
			len=(end-number_buffer.begin());
		}
		if (decimal_point!='\0' && decimal_point!='.') {
			const auto dec_pos=std::find(number_buffer.begin(),number_buffer.end(),decimal_point);
			if (dec_pos!=number_buffer.end()) *dec_pos = '.';
		}
		o->write_characters(number_buffer.data(),static_cast<std::size_t>(len));
		const bool value_is_int_like=std::none_of(number_buffer.begin(),number_buffer.begin()+len+1,[](char c) {
			return c=='.' || c=='e';
		});
		if (value_is_int_like) o->write_characters(".0",2);
	}
	static std::uint8_t decode(std::uint8_t& state,std::uint32_t& codep,const std::uint8_t byte) noexcept {
		static const std::array<std::uint8_t,400> utf8d={
			{
				0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,// 00..1F
				0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,// 20..3F
				0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,// 40..5F
				0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,// 60..7F
				1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,// 80..9F
				7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,// A0..BF
				8,8,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,// C0..DF
				0xA,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x4,0x3,0x3,// E0..EF
				0xB,0x6,0x6,0x6,0x5,0x8,0x8,0x8,0x8,0x8,0x8,0x8,0x8,0x8,0x8,0x8,// F0..FF
				0x0,0x1,0x2,0x3,0x5,0x8,0x7,0x1,0x1,0x1,0x4,0x6,0x1,0x1,0x1,0x1,// s0..s0
				1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,1,1,1,1,1,0,1,0,1,1,1,1,1,1,// s1..s2
				1,2,1,1,1,1,1,2,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,// s3..s4
				1,2,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,3,1,3,1,1,1,1,1,1,// s5..s6
				1,3,1,1,1,1,1,3,1,3,1,1,1,1,1,1,1,3,1,1,1,1,1,1,1,1,1,1,1,1,1,1 // s7..s8
			}
		};
		const std::uint8_t type=utf8d[byte];
		codep=(state!=UTF8_ACCEPT)?(byte&0x3fu)|(codep<<6u):(0xFFu>>type)&(byte);
		const std::size_t index=256u+static_cast<size_t>(state)*16u+static_cast<size_t>(type);
		state=utf8d[index];
		return state;
	}
    unsigned remove_sign(unsigned x) {
		return x;
	}
	inline unsigned remove_sign(int_t x) noexcept {
		return static_cast<unsigned>(-(x+1))+1;
	}
};

namespace parser {
	
struct parse_point {
	std::size_t total_char_=0,curr_line_char_=0,total_line_=0;
};

enum class TOKEN_TYPE {
	TT_uninitialized,
	TT_literal_true,
	TT_literal_false,
	TT_literal_null,
	TT_value_string,
	TT_value_integer,
	TT_value_float,
	TT_array_begin,
	TT_array_end,
	TT_object_begin,
	TT_object_end,
	TT_separator_name,
	TT_separator_value,
	TT_parse_error,
	TT_end_of_input,
	TT_literal_or_value
};
	
template<typename JsonType>
class lexer_base {
public:
	static const char* token_type_name(const TOKEN_TYPE t) noexcept {
		switch (t) {
			case TOKEN_TYPE::TT_uninitialized:
				return "<uninitialized>";
            case TOKEN_TYPE::TT_literal_true:
				return "true literal";
			case TOKEN_TYPE::TT_literal_false:
				return "false literal";
			case TOKEN_TYPE::TT_literal_null:
				return "null literal";
			case TOKEN_TYPE::TT_value_string:
				return "string literal";
			case TOKEN_TYPE::TT_value_integer:
			case TOKEN_TYPE::TT_value_float:
				return "number literal";
			case TOKEN_TYPE::TT_array_begin:
				return "'['";
			case TOKEN_TYPE::TT_array_end:
				return "']'";
			case TOKEN_TYPE::TT_object_begin:
				return "'{'";
			case TOKEN_TYPE::TT_object_end:
				return "'}'";
			case TOKEN_TYPE::TT_separator_name:
				return "':'";
			case TOKEN_TYPE::TT_separator_value:
				return "','";
			case TOKEN_TYPE::TT_parse_error:
				return "<parse error>";
			case TOKEN_TYPE::TT_end_of_input:
				return "EOI";
			case TOKEN_TYPE::TT_literal_or_value:
				return "'[', '{', or a literal";
            default:
				return "unknown token";
		}
	}
};

template<typename JsonType,typename AdapterType>
class lexer : public lexer_base<JsonType> {
    using int_t=typename JsonType::int_t;
    using float_t=typename JsonType::float_t;
    using string_t=typename JsonType::string_t;
    using char_type=typename AdapterType::char_type;
    using char_int_type=typename char_traits<char_type>::int_type;
    using position_t=parse_point;

private:
	AdapterType ia;
	const bool ignore_comments=false;
	char_int_type current=char_traits<char_type>::eof();
	bool next_unget=false;
	position_t position {};
	std::vector<char_type> token_string {};
	string_t token_buffer {};
	const char* error_message="";
	int_t value_integer=0;
	float_t value_float=0;
	const char_int_type decimal_point_char='.';
	bool is_hex_=false;

public:
	using token_type=TOKEN_TYPE;
	explicit lexer(AdapterType&& adapter,bool ignore_comments_=false) noexcept :
		ia(std::move(adapter)) ,
		ignore_comments(ignore_comments_) ,
		decimal_point_char(static_cast<char_int_type>(get_decimal_point())) {}
	lexer(const lexer&)=delete;
	lexer(lexer&&)=default;
	lexer& operator =(lexer&)=delete;
	lexer& operator =(lexer&&)=default;
	~lexer()=default;

private:
	static char get_decimal_point() noexcept {
        const auto* loc=localeconv();
		return (!loc->decimal_point)?'.':*(loc->decimal_point);
	}
	int get_codepoint() {
		int codepoint = 0;
		const auto factors={12u,8u,4u,0u};
		for (const auto factor:factors) {
			get();
			if (current>='0' && current<='9') codepoint+=static_cast<int>((static_cast<unsigned int>(current)-0x30u)<<factor);
			else if (current>='A' && current<='F') codepoint+=static_cast<int>((static_cast<unsigned int>(current)-0x37u)<<factor);
			else if (current>='a' && current<='f') codepoint+=static_cast<int>((static_cast<unsigned int>(current)-0x57u)<<factor);
			else return -1;
		}
		return codepoint;
	}
	bool next_byte_in_range(std::initializer_list<char_int_type> ranges) {
		add(current);
		for (auto range=ranges.begin();range!=ranges.end();range++) {
			get();
			if (*range<=current && current<=*(++range)) add(current);
			else {
				error_message="invalid string: ill-formed UTF-8 byte";
				return false;
			}
		}
		return true;
	}
	token_type scan_string() {
		reset();
		std::vector<std::string> error_control={"NUL","SOH","STX","ETX","EOT","ENQ","ACK","BEL",
												"BS" ,"HT" ,"LF" ,"VT" ,"FF" ,"CR" ,"SO" ,"SI" ,
												"DLE","DC1","DC2","DC3","DC4","NAK","SYN","ETB",
												"CAN","EM" ,"SUB","ESC","FS" ,"GS" ,"RS" ,"US"};
		while (1) {
			int temp_get;
			switch (temp_get=get()) {
				case char_traits<char_type>::eof(): {
					error_message="invalid string: missing closing quote";
					return TOKEN_TYPE::TT_parse_error;
				}
				case '\"': {
					return TOKEN_TYPE::TT_value_string;
				}
				case '\\': {
					switch (get()) {
						case '\"': add('\"'); break;
						case '\\': add('\\'); break;
						case '/':  add('/');  break;
						case 'b':  add('\b'); break;
						case 'f':  add('\f'); break;
						case 'n':  add('\n'); break;
						case 'r':  add('\r'); break;
						case 't':  add('\t'); break;
						case 'u': {
							const int codepoint1=get_codepoint();
							int codepoint=codepoint1;
							if (codepoint1==-1) {
								error_message="invalid string: '\\u' must be followed by 4 hex digits";
								return TOKEN_TYPE::TT_parse_error;
							}
							if (0xD800<=codepoint1 && codepoint1<=0xDBFF)  {
								if (get()=='\\' && get()=='u') {
									const int codepoint2=get_codepoint();
									if (codepoint2==-1) {
										error_message="invalid string: '\\u' must be followed by 4 hex digits";
										return TOKEN_TYPE::TT_parse_error;
									}
									if (0xDC00<=codepoint2 && codepoint2<=0xDFFF) codepoint=static_cast<int>((static_cast<unsigned int>(codepoint1)<<10u)+static_cast<unsigned int>(codepoint2)-0x35FDC00u);
									else {
										error_message="invalid string: surrogate U+D800..U+DBFF must be followed by U+DC00..U+DFFF";
										return TOKEN_TYPE::TT_parse_error;
									}
								} else {
									error_message="invalid string: surrogate U+D800..U+DBFF must be followed by U+DC00..U+DFFF";
									return TOKEN_TYPE::TT_parse_error;
								}
							} else {
								if (0xDC00<=codepoint1 && codepoint1<=0xDFFF) {
									error_message="invalid string: surrogate U+DC00..U+DFFF must follow U+D800..U+DBFF";
									return TOKEN_TYPE::TT_parse_error;
								}
							}
							if (codepoint<0x80) add(static_cast<char_int_type>(codepoint));
                            else if (codepoint<=0x7FF) {
								add(static_cast<char_int_type>(0xC0u|(static_cast<unsigned int>(codepoint)>>6u)));
								add(static_cast<char_int_type>(0x80u|(static_cast<unsigned int>(codepoint)&0x3Fu)));
							} else if (codepoint<=0xFFFF) {
								add(static_cast<char_int_type>(0xE0u|(static_cast<unsigned int>(codepoint)>>12u)));
                                add(static_cast<char_int_type>(0x80u|((static_cast<unsigned int>(codepoint)>>6u)&0x3Fu)));
                                add(static_cast<char_int_type>(0x80u|(static_cast<unsigned int>(codepoint)&0x3Fu)));
							} else {
								add(static_cast<char_int_type>(0xF0u|(static_cast<unsigned int>(codepoint)>>18u)));
								add(static_cast<char_int_type>(0x80u|((static_cast<unsigned int>(codepoint)>>12u)&0x3Fu)));
								add(static_cast<char_int_type>(0x80u|((static_cast<unsigned int>(codepoint)>>6u)&0x3Fu)));
								add(static_cast<char_int_type>(0x80u|(static_cast<unsigned int>(codepoint)&0x3Fu)));
							}
							break;
						}
						default:
							error_message="invalid string: forbidden character after backslash";
							return TOKEN_TYPE::TT_parse_error;
					}
					break;
				}
				case 0x00: case 0x01: case 0x02: case 0x03: case 0x04: case 0x05: case 0x06: case 0x07:
				case 0x08: case 0x09: case 0x0A: case 0x0B: case 0x0C: case 0x0D: case 0x0E: case 0x0F:
				case 0x10: case 0x11: case 0x12: case 0x13: case 0x14: case 0x15: case 0x16: case 0x17:
				case 0x18: case 0x19: case 0x1A: case 0x1B: case 0x1C: case 0x1D: case 0x1E: case 0x1F: {
					char* temp_message=new char[200];
					sprintf(temp_message,"invalid string: control character U+%04X (%s) must be escaped to \\\\u%04X",temp_get,error_control[temp_get].c_str(),temp_get);
					error_message=temp_message;
					return TOKEN_TYPE::TT_parse_error;
				}
				// U+0020..U+007F (except U+0022 (quote) and U+005C (backspace))
				case 0x20: case 0x21: case 0x23: case 0x24: case 0x25: case 0x26: case 0x27: case 0x28:
				case 0x29: case 0x2A: case 0x2B: case 0x2C: case 0x2D: case 0x2E: case 0x2F: case 0x30:
				case 0x31: case 0x32: case 0x33: case 0x34: case 0x35: case 0x36: case 0x37: case 0x38:
				case 0x39: case 0x3A: case 0x3B: case 0x3C: case 0x3D: case 0x3E: case 0x3F: case 0x40:
				case 0x41: case 0x42: case 0x43: case 0x44: case 0x45: case 0x46: case 0x47: case 0x48:
				case 0x49: case 0x4A: case 0x4B: case 0x4C: case 0x4D: case 0x4E: case 0x4F: case 0x50:
				case 0x51: case 0x52: case 0x53: case 0x54: case 0x55: case 0x56: case 0x57: case 0x58:
				case 0x59: case 0x5A: case 0x5B: case 0x5D: case 0x5E: case 0x5F: case 0x60: case 0x61:
				case 0x62: case 0x63: case 0x64: case 0x65: case 0x66: case 0x67: case 0x68: case 0x69:
				case 0x6A: case 0x6B: case 0x6C: case 0x6D: case 0x6E: case 0x6F: case 0x70: case 0x71:
				case 0x72: case 0x73: case 0x74: case 0x75: case 0x76: case 0x77: case 0x78: case 0x79:
				case 0x7A: case 0x7B: case 0x7C: case 0x7D: case 0x7E: case 0x7F: {
					add(current);
					break;
				}
				// U+0080..U+07FF: bytes C2..DF 80..BF
                case 0xC2: case 0xC3: case 0xC4: case 0xC5: case 0xC6: case 0xC7: case 0xC8: case 0xC9:
				case 0xCA: case 0xCB: case 0xCC: case 0xCD: case 0xCE: case 0xCF: case 0xD0: case 0xD1:
				case 0xD2: case 0xD3: case 0xD4: case 0xD5: case 0xD6: case 0xD7: case 0xD8: case 0xD9:
				case 0xDA: case 0xDB: case 0xDC: case 0xDD: case 0xDE: case 0xDF: {
					if (!next_byte_in_range({0x80,0xBF})) return TOKEN_TYPE::TT_parse_error;
					break;
				}
				// U+0800..U+0FFF: bytes E0 A0..BF 80..BF
				case 0xE0: {
					if (!(next_byte_in_range({0xA0,0xBF,0x80,0xBF}))) return TOKEN_TYPE::TT_parse_error;
					break;
				}
				// U+1000..U+CFFF: bytes E1..EC 80..BF 80..BF
				// U+E000..U+FFFF: bytes EE..EF 80..BF 80..BF
				case 0xE1: case 0xE2: case 0xE3: case 0xE4: case 0xE5: case 0xE6: case 0xE7: case 0xE8:
				case 0xE9: case 0xEA: case 0xEB: case 0xEC: case 0xEE: case 0xEF: {
					if (!(next_byte_in_range({0x80,0xBF,0x80,0xBF}))) return TOKEN_TYPE::TT_parse_error;
					break;
				}
				// U+D000..U+D7FF: bytes ED 80..9F 80..BF
				case 0xED: {
					if (!(next_byte_in_range({0x80,0x9F,0x80,0xBF}))) return TOKEN_TYPE::TT_parse_error;
					break;
				}
				// U+10000..U+3FFFF F0 90..BF 80..BF 80..BF
				case 0xF0: {
					if (!(next_byte_in_range({0x90,0xBF,0x80,0xBF,0x80,0xBF}))) return TOKEN_TYPE::TT_parse_error;
					break;
				}
				// U+40000..U+FFFFF F1..F3 80..BF 80..BF 80..BF
				case 0xF1: case 0xF2: case 0xF3: {
					if (!(next_byte_in_range({0x80,0xBF,0x80,0xBF,0x80,0xBF}))) return TOKEN_TYPE::TT_parse_error;
					break;
				}
				// U+100000..U+10FFFF F4 80..8F 80..BF 80..BF
				case 0xF4: {
					if (!(next_byte_in_range({0x80,0x8F,0x80,0xBF,0x80,0xBF}))) return TOKEN_TYPE::TT_parse_error;
					break;
				}
				// remaining bytes (80..C1 and F5..FF) are ill-formed
				default: {
					error_message="invalid string: ill-formed UTF-8 byte";
					return TOKEN_TYPE::TT_parse_error;
				}
			}
		}
	}
	bool scan_comment() {
		switch (get()) {
			case '/': {
				while (1) {
					switch (get()) {
						case '\n': case '\r': case char_traits<char_type>::eof(): case '\0':
							return true;
						default:
						break;
					}
				}
			}
			case '*': {
				while (1) {
					switch (get()) {
						case char_traits<char_type>::eof():
						case '\0': {
							error_message="invalid comment; missing closing '*/'";
							return false;
						}
						case '*': {
							switch (get()) {
								case '/': return true;
								default: {
									unget();
									continue;
								}
							}
						}
						default: continue;
					}
				}
			}
			default: {
				error_message="invalid comment; expecting '/' or '*' after '/'";
				return false;
			}
		}
	}
	static void strtof(float& f,const char* str,char** endptr) noexcept {
 		f=std::strtof(str,endptr);
	}
	static void strtof(double& f,const char* str,char** endptr) noexcept {
		f=std::strtod(str,endptr);
	}
	static void strtof(long double& f,const char* str,char** endptr) noexcept {
		f=std::strtold(str,endptr);
	}
	token_type scan_number() {
		reset();
		token_type number_type=TOKEN_TYPE::TT_value_integer;
		switch (current) {
			case '-': {
				add(current);
				goto scan_number_minus;
			} 
			case '0': {
				add(current);
				goto scan_number_zero;
			}
			case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8':
			case '9': {
				add(current);
				goto scan_number_any1;
			}
			default: {
				break;
			}
		}
scan_number_minus:
		number_type=TOKEN_TYPE::TT_value_integer;
		switch (get()) {
			case '0': {
				add(current);
				goto scan_number_zero;
			}
			case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8':
			case '9': {
				add(current);
				goto scan_number_any1;
			}
			default: {
				error_message="invalid number; expected digit after '-'";
				return TOKEN_TYPE::TT_parse_error;
			}
		}
scan_number_zero:
		switch (get()) {
			case '.': {
				add(decimal_point_char);
				goto scan_number_decimal1;
			}
			case 'e': case 'E': {
				add(current);
				goto scan_number_exponent;
			}
			case 'x': {
				is_hex_=true;
				goto scan_number_hexadecimal;
			}
			default:
				goto scan_number_done;
		}
scan_number_hexadecimal:
		switch (get()) {
			case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7':
			case '8': case '9': case 'A': case 'B': case 'C': case 'D': case 'E': case 'F': {
				add(current);
				goto scan_number_hexadecimal;
			}
			default:
				goto scan_number_done;
		}
scan_number_any1:
		switch (get()) {
            case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7':
			case '8': case '9': {
				add(current);
				goto scan_number_any1;
			}
			case '.': {
				add(decimal_point_char);
				goto scan_number_decimal1;
			}
			case 'e': case 'E': {
				add(current);
				goto scan_number_exponent;
			}
			default:
				goto scan_number_done;
		}
scan_number_decimal1:
		number_type=TOKEN_TYPE::TT_value_float;
		switch (get()) {
			case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7':
			case '8': case '9': {
				add(current);
				goto scan_number_decimal2;
			}
			default: {
			error_message="invalid number; expected digit after '.'";
				return  TOKEN_TYPE::TT_parse_error;
			}
		}
scan_number_decimal2:
        switch (get()) {
			case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7':
			case '8': case '9': {
				add(current);
				goto scan_number_decimal2;
			}
			case 'e': case 'E': {
				add(current);
				goto scan_number_exponent;
			}
			default:
				goto scan_number_done;
		}
scan_number_exponent:
		number_type=TOKEN_TYPE::TT_value_float;
		switch (get()) {
			case '+': case '-': {
				add(current);
				goto scan_number_sign;
			}
			case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7':
			case '8': case '9': {
				add(current);
				goto scan_number_any2;
			}
			default: {
				error_message="invalid number; expected '+', '-', or digit after exponent";
				return TOKEN_TYPE::TT_parse_error;
			}
		}
scan_number_sign:
		switch (get()) {
			case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7':
			case '8': case '9': {
				add(current);
				goto scan_number_any2;
			}
			default: {
				error_message="invalid number; expected digit after exponent sign";
				return TOKEN_TYPE::TT_parse_error;
			}
		}
scan_number_any2:
		switch (get()) {
			case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7':
			case '8': case '9': {
				add(current);
				goto scan_number_any2;
			}
			default:
				goto scan_number_done;
		}
scan_number_done:
		unget();
		char* endptr=nullptr;
		errno=0;
		if (number_type==TOKEN_TYPE::TT_value_integer) {
			const auto x=std::strtoll(token_buffer.data(),&endptr,is_hex_?16:10);
			//std::cout<<token_buffer.data()<<" "<<x<<std::endl;
			if (errno==0) {
				value_integer=static_cast<int_t>(x);
                if (value_integer==x) return TOKEN_TYPE::TT_value_integer;
			}
		}
		strtof(value_float,token_buffer.data(),&endptr);
		return TOKEN_TYPE::TT_value_float;
	}
	token_type scan_literal(const char_type* literal_text,const std::size_t length,token_type return_type) {
		for (std::size_t i=1;i<length;i++) {
			if (char_traits<char_type>::to_char_type(get()) != literal_text[i]) {
				error_message="invalid literal";
				return TOKEN_TYPE::TT_parse_error;
			}
		}
		return return_type;
	}
	void reset() noexcept {
		is_hex_=false;
		token_buffer.clear();
		token_string.clear();
		token_string.push_back(char_traits<char_type>::to_char_type(current));
	}
	char_int_type get() {
		position.total_char_++;
		position.curr_line_char_++;
		if (next_unget) next_unget=false;
		else current=ia.get_character();
		if (current!=char_traits<char_type>::eof()) token_string.push_back(char_traits<char_type>::to_char_type(current));
		if (current=='\n') {
			position.total_line_++;
			position.curr_line_char_=0;
		}
		return current;
	}
	void unget() {
		next_unget=true;
		position.total_char_--;
		if (position.curr_line_char_==0) {
            if (position.total_line_>0) position.total_line_--;
		} else position.curr_line_char_--;
		if (current!=char_traits<char_type>::eof()) token_string.pop_back();
	}
	void add(char_int_type c) {
		token_buffer.push_back(static_cast<typename string_t::value_type>(c));
	}

public:
	constexpr int_t get_integer() const noexcept {
		return value_integer;
	}
	constexpr float_t get_float() const noexcept {
		return value_float;
	}
	string_t& get_string() {
		return token_buffer;
	}
    constexpr position_t get_position() const noexcept {
		return position;
	}
	std::string get_token_string() const {
		std::string result;
		for (const auto c:token_string) {
			if (static_cast<unsigned char>(c)<='\x1F') {
				std::array<char,9> cs{{}};
				static_cast<void>((std::snprintf)(cs.data(),cs.size(),"<U+%.4X>",static_cast<unsigned char>(c)));
				result+=cs.data();
			} else result.push_back(static_cast<std::string::value_type>(c));
		}
		return result;
	}
	constexpr const char* get_error_message() const noexcept {
		return error_message;
	}
	bool skip_bom() {
		if (get()==0xEF) {
			return get()==0xBB && get()==0xBF;
		}
		unget();
		return true;
	}
	void skip_whitespace() {
		do {
			get();
		} while (current==' ' || current=='\t' || current=='\n' || current=='\r');
    }
	token_type scan() {
		if (position.total_char_==0 && !skip_bom()) {
			error_message="invalid BOM; must be 0xEF 0xBB 0xBF if given";
			return TOKEN_TYPE::TT_parse_error;
		}
		skip_whitespace();
		while (ignore_comments && current=='/') {
			if (!scan_comment()) return TOKEN_TYPE::TT_parse_error;
			skip_whitespace();
		}
		switch (current) {
			case '[': return TOKEN_TYPE::TT_array_begin;
			case ']': return TOKEN_TYPE::TT_array_end;
			case '{': return TOKEN_TYPE::TT_object_begin;
			case '}': return TOKEN_TYPE::TT_object_end;
			case ':': return TOKEN_TYPE::TT_separator_name;
			case ',': return TOKEN_TYPE::TT_separator_value;
			case 't': {
				std::array<char_type,4> true_literal={{static_cast<char_type>('t'),static_cast<char_type>('r'),static_cast<char_type>('u'),static_cast<char_type>('e')}};
				return scan_literal(true_literal.data(),true_literal.size(),TOKEN_TYPE::TT_literal_true);
			}
            case 'f': {
				std::array<char_type,5> false_literal={{static_cast<char_type>('f'),static_cast<char_type>('a'),static_cast<char_type>('l'),static_cast<char_type>('s'),static_cast<char_type>('e')}};
				return scan_literal(false_literal.data(),false_literal.size(),TOKEN_TYPE::TT_literal_false);
			}
			case 'n': {
				std::array<char_type,4> null_literal={{static_cast<char_type>('n'),static_cast<char_type>('u'),static_cast<char_type>('l'),static_cast<char_type>('l')}};
				return scan_literal(null_literal.data(),null_literal.size(),TOKEN_TYPE::TT_literal_null);
			}
			case '\"': return scan_string();
			case '-': case '0': case '1': case '2': case '3': case '4': case '5': case '6':
			case '7': case '8': case '9': return scan_number();
            case '\0': case char_traits<char_type>::eof(): return TOKEN_TYPE::TT_end_of_input;
			default:
				error_message="invalid literal";
				return TOKEN_TYPE::TT_parse_error;
        }
    }
};

////////SAX
template<typename JsonType>
struct json_sax {
	using int_t=typename JsonType::int_t;
    using float_t=typename JsonType::float_t;
	using string_t=typename JsonType::string_t;
	virtual bool null()=0;
	virtual bool boolean(bool val)=0;
	virtual bool number_integer(int_t val)=0;
	virtual bool number_float(float_t val,const string_t& s)=0;
	virtual bool string(string_t& val)=0;
	virtual bool start_object(std::size_t elements)=0;
	virtual bool key(string_t& val)=0;
	virtual bool end_object()=0;
	virtual bool start_array(std::size_t elements)=0;
	virtual bool end_array()=0;
	virtual bool parse_error(parse_point position,const std::string& last_token, const std::exception& ex)=0;
	json_sax()=default;
	json_sax(const json_sax&)=default;
	json_sax(json_sax&&) noexcept=default;
	json_sax& operator=(const json_sax&)=default;
	json_sax& operator=(json_sax&&) noexcept=default;
	virtual ~json_sax()=default;
};

template<typename JsonType>
class json_sax_dom_parser {
public:
	using int_t=typename JsonType::int_t;
    using float_t=typename JsonType::float_t;
    using string_t=typename JsonType::string_t;
    explicit json_sax_dom_parser(JsonType& r,const bool allow_exceptions_=true) : root(r), allow_exceptions(allow_exceptions_) {}
    json_sax_dom_parser(const json_sax_dom_parser&)=delete;
    json_sax_dom_parser(json_sax_dom_parser&&)=default; // NOLINT(hicpp-noexcept-move,performance-noexcept-move-constructor)
    json_sax_dom_parser& operator =(const json_sax_dom_parser&)=delete;
    json_sax_dom_parser& operator =(json_sax_dom_parser&&)=default; // NOLINT(hicpp-noexcept-move,performance-noexcept-move-constructor)
    ~json_sax_dom_parser()=default;
	bool null() {
		handle_value(nullptr);
		return true;
	}
	bool boolean(bool val) {
		handle_value(val);
		return true;
	}
    bool number_integer(int_t val) {
		handle_value(val);
		return true;
	}
	bool number_float(float_t val,const string_t&) {
		handle_value(val);
		return true;
	}
	bool string(string_t& val) {
		handle_value(val);
		return true;
	}
	bool start_object(std::size_t len) {
		ref_stack.push_back(handle_value(JSON_TYPE::JT_object));
		if (len!=static_cast<std::size_t>(-1) && len>ref_stack.back()->max_size()) throw std::out_of_range("excessive object size: "+std::to_string(len));
		return true;
	}
	bool key(string_t& val) {
		object_element=&(ref_stack.back()->mdata_.val_.object_->operator[](val));
		return true;
	}
	bool end_object() {
		ref_stack.pop_back();
		return true;
	}
	bool start_array(std::size_t len) {
		ref_stack.push_back(handle_value(JSON_TYPE::JT_array));
        if (len!=static_cast<std::size_t>(-1) && len>ref_stack.back()->max_size()) throw std::out_of_range("excessive array size: "+std::to_string(len));
		return true;
	}
	bool end_array() {
		ref_stack.pop_back();
		return true;
	}
	template<class Exception>
	bool parse_error(parse_point,const std::string&,const Exception& ex) {
		errored=true;
		static_cast<void>(ex);
		if (allow_exceptions) throw ex;
		return false;
	}
	constexpr bool is_errored() const {
		return errored;
	}

private:
    template<typename Value>
	JsonType* handle_value(Value&& v) {
		if (ref_stack.empty()) {
			root=JsonType(std::forward<Value>(v));
			return &root;
		}
		if (ref_stack.back()->mdata_.type_==JSON_TYPE::JT_array) {
			ref_stack.back()->mdata_.val_.array_->emplace_back(std::forward<Value>(v));
			return &(ref_stack.back()->mdata_.val_.array_->back());
		}
		*object_element=JsonType(std::forward<Value>(v));
		return object_element;
	}
    JsonType& root;
    std::vector<JsonType*> ref_stack {};
	JsonType* object_element=nullptr;
    bool errored=false;
	const bool allow_exceptions=true;
};

template<typename JsonType>
class json_sax_dom_callback_parser {
  public:
	using int_t=typename JsonType::int_t;
    using float_t=typename JsonType::float_t;
    using string_t=typename JsonType::string_t;
    using parser_callback_t=typename JsonType::parser_callback_t;
    using parse_event_t=typename JsonType::parse_event_t;
	json_sax_dom_callback_parser(JsonType& r,const parser_callback_t cb,const bool allow_exceptions_=true) : root(r), callback(cb), allow_exceptions(allow_exceptions_) {
		keep_stack.push_back(true);
	}
	json_sax_dom_callback_parser(const json_sax_dom_callback_parser&)=delete;
	json_sax_dom_callback_parser(json_sax_dom_callback_parser&&)=default;
	json_sax_dom_callback_parser& operator =(const json_sax_dom_callback_parser&)=delete;
	json_sax_dom_callback_parser& operator =(json_sax_dom_callback_parser&&)=default;
	~json_sax_dom_callback_parser()=default;
    bool null() {
		handle_value(nullptr);
		return true;
	}
	bool boolean(bool val) {
		handle_value(val);
		return true;
	}
    bool number_integer(int_t val) {
		handle_value(val);
		return true;
	}
	bool number_float(float_t val,const string_t&) {
		handle_value(val);
		return true;
	}
	bool string(string_t& val) {
		handle_value(val);
		return true;
	}
	bool start_object(std::size_t len) {
        const bool keep=callback(static_cast<int>(ref_stack.size()),PARSE_STATUS::PS_objectbegin,discarded);
		keep_stack.push_back(keep);
		auto val=handle_value(JSON_TYPE::JT_object,true);
		ref_stack.push_back(val.second);
		if (ref_stack.back() && len!=static_cast<std::size_t>(-1) && len>ref_stack.back()->max_size()) throw std::out_of_range("excessive object size: "+std::to_string(len));
		return true;
	}
	bool key(string_t& val) {
		JsonType k=JsonType(val);
		const bool keep=callback(static_cast<int>(ref_stack.size()),PARSE_STATUS::PS_key,k);
		key_keep_stack.push_back(keep);
		if (keep && ref_stack.back()) object_element=&(ref_stack.back()->mdata_.val_.object_->operator[](val)=discarded);
		return true;
	}
	bool end_object() {
		if (ref_stack.back()) {
			if (!callback(static_cast<int>(ref_stack.size())-1,PARSE_STATUS::PS_objectend,*ref_stack.back())) *ref_stack.back()=discarded;
		}
		ref_stack.pop_back();
		keep_stack.pop_back();
        if (!ref_stack.empty() && ref_stack.back() && (ref_stack.back()->mdata_.type_==JSON_TYPE::JT_array || ref_stack.back()->mdata_.type_==JSON_TYPE::JT_object)) {
			for (auto it=ref_stack.back()->begin();it!=ref_stack.back()->end();it++) {
				if (it->mdata_.type_==JSON_TYPE::JT_discarded) {
					ref_stack.back()->erase(it);
					break;
				}
			}
		}
		return true;
	}
    bool start_array(std::size_t len) {
		const bool keep=callback(static_cast<int>(ref_stack.size()),PARSE_STATUS::PS_arraybegin,discarded);
		keep_stack.push_back(keep);
		auto val=handle_value(JSON_TYPE::JT_array,true);
		ref_stack.push_back(val.second);
		if (ref_stack.back() && len!=static_cast<std::size_t>(-1) && len>ref_stack.back()->max_size()) throw std::out_of_range("excessive array size: "+std::to_string(len));
        return true;
    }
	bool end_array() {
		bool keep=true;
		if (ref_stack.back()) {
			keep=callback(static_cast<int>(ref_stack.size())-1,PARSE_STATUS::PS_arrayend,*ref_stack.back());
            if (!keep) *ref_stack.back() = discarded;
		}
		ref_stack.pop_back();
		keep_stack.pop_back();
		if (!keep && !ref_stack.empty() && ref_stack.back()->mdata_.type_==JSON_TYPE::JT_array) ref_stack.back()->mdata_.val_.array_->pop_back();
		return true;
    }
	template<class Exception>
	bool parse_error(parse_point,const std::string&,const Exception& ex) {
		errored=true;
		static_cast<void>(ex);
		if (allow_exceptions) throw ex;
		return false;
    }
	constexpr bool is_errored() const {
		return errored;
	}

private:
    template<typename Value>
    std::pair<bool,JsonType*> handle_value(Value&& v,const bool skip_callback=false) {
		if (!keep_stack.back()) return {false,nullptr};
		auto value=JsonType(std::forward<Value>(v));
        const bool keep=skip_callback || callback(static_cast<int>(ref_stack.size()),PARSE_STATUS::PS_value,value);
		if (!keep) return {false,nullptr};
		if (ref_stack.empty()) {
			root=std::move(value);
			return {true,&root};
		}
		if (!ref_stack.back()) {
			return {false,nullptr};
		}
		if (ref_stack.back()->mdata_.type_==JSON_TYPE::JT_array) {
			ref_stack.back()->mdata_.val_.array_->emplace_back(std::move(value));
			return {true,&(ref_stack.back()->mdata_.val_.array_->back())};
		}
		const bool store_element=key_keep_stack.back();
		key_keep_stack.pop_back();
		if (!store_element) return {false, nullptr};
		*object_element=std::move(value);
		return {true,object_element};
	}
	JsonType& root;
	std::vector<JsonType*> ref_stack {};
	std::vector<bool> keep_stack {};
	std::vector<bool> key_keep_stack {};
	JsonType* object_element=nullptr;
	bool errored=false;
	const parser_callback_t callback=nullptr;
	const bool allow_exceptions=true;
	JsonType discarded=JSON_TYPE::JT_discarded;
};

template<typename JsonType>
class json_sax_acceptor {
public:
	using int_t=typename JsonType::int_t;
    using float_t=typename JsonType::float_t;
    using string_t=typename JsonType::string_t;
	bool null() { return true; }
	bool boolean(bool) { return true; }
	bool number_integer(int_t) { return true; }
	bool number_float(float_t, const string_t&) { return true; }
	bool string(string_t&)  { return true; }
	bool start_object(std::size_t=static_cast<std::size_t>(-1)) { return true; }
	bool key(string_t&) { return true; }
	bool end_object() { return true; }
	bool start_array(std::size_t=static_cast<std::size_t>(-1)) { return true; }
	bool end_array() { return true; }
	bool parse_error(std::size_t,const std::string&,const std::exception&) { return false; }
};

template<typename JsonType>
using parser_callback_t=std::function<bool(int,PARSE_STATUS,JsonType&)>;
    
template<typename JsonType,typename AdapterType>
class parser {
	using int_t=typename JsonType::int_t;
	using float_t=typename JsonType::float_t;
	using string_t=typename JsonType::string_t;
    using lexer_t=lexer<JsonType,AdapterType>;
    using token_type=typename lexer_t::token_type;

public:
	explicit parser(AdapterType&& adapter,const parser_callback_t<JsonType> cb=nullptr,const bool allow_exceptions_=true,const bool skip_comments=false) :
		callback(cb) ,
		m_lexer(std::move(adapter),skip_comments) ,
		allow_exceptions(allow_exceptions_) {
		get_token();
	}
	void parse(const bool strict,JsonType& result) {
		if (callback) {
			json_sax_dom_callback_parser<JsonType> sdp(result,callback,allow_exceptions);
			sax_parse_internal(&sdp);
			if (strict && (get_token()!=TOKEN_TYPE::TT_end_of_input)) {
                sdp.parse_error(m_lexer.get_position(),m_lexer.get_token_string(),std::invalid_argument("EOI-value"));
            }
            if (sdp.is_errored()) {
				result=JSON_TYPE::JT_discarded;
				return;
			}
			if (result.mdata_.type_==JSON_TYPE::JT_discarded) result=nullptr;
        } else {
			json_sax_dom_parser<JsonType> sdp(result,allow_exceptions);
			sax_parse_internal(&sdp);
            if (strict && (get_token()!=TOKEN_TYPE::TT_end_of_input)) {
				sdp.parse_error(m_lexer.get_position(),m_lexer.get_token_string(),std::invalid_argument("EOI-value"));
			}
			if (sdp.is_errored()) {
                result=JSON_TYPE::JT_discarded;
				return;
			}
		}
	}
	bool accept(const bool strict=true) {
		json_sax_acceptor<JsonType> sax_acceptor;
		return sax_parse(&sax_acceptor,strict);
    }
    /*
	template<typename SAX>
	bool sax_parse(SAX* sax,const bool strict=true) {
        (void)detail::is_sax_static_asserts<SAX, BasicJsonType> {};
        const bool result = sax_parse_internal(sax);

        // strict mode: next byte must be EOF
        if (result && strict && (get_token() != token_type::end_of_input))
        {
            return sax->parse_error(m_lexer.get_position(),
                                    m_lexer.get_token_string(),
                                    parse_error::create(101, m_lexer.get_position(), exception_message(token_type::end_of_input, "value"), nullptr));
        }

        return result;
    }
    */

private:
    template<typename SAX>
	bool sax_parse_internal(SAX* sax) {
		std::vector<bool> states;
		bool skip_to_state_evaluation=false;
		while (1) {
			if (!skip_to_state_evaluation) {
				switch (last_token) {
                    case TOKEN_TYPE::TT_object_begin: {
						if (!sax->start_object(static_cast<std::size_t>(-1))) return false;
						if (get_token()==TOKEN_TYPE::TT_object_end) {
							if (!sax->end_object()) return false;
							break;
						}
						if (last_token!=TOKEN_TYPE::TT_value_string) return sax->parse_error(m_lexer.get_position(),m_lexer.get_token_string(),std::invalid_argument("string-object key"));
                        if (!sax->key(m_lexer.get_string())) return false;
						if (get_token()!=TOKEN_TYPE::TT_separator_name) return sax->parse_error(m_lexer.get_position(),m_lexer.get_token_string(),std::invalid_argument(":-object separator"));
						states.push_back(false);
						get_token();
						continue;
					}
                    case TOKEN_TYPE::TT_array_begin: {
						if (!sax->start_array(static_cast<std::size_t>(-1))) return false;
						if (get_token()==TOKEN_TYPE::TT_array_end) {
							if (!sax->end_array()) return false;
							break;
						}
						states.push_back(true);
						continue;
					}
					case TOKEN_TYPE::TT_value_float: {
						const auto res=m_lexer.get_float();
						if (!std::isfinite(res)) return sax->parse_error(m_lexer.get_position(),m_lexer.get_token_string(),std::out_of_range("number overflow parsing '"+m_lexer.get_token_string()+"\'"));
						if (!sax->number_float(res,m_lexer.get_string())) return false;
						break;
					}
					case TOKEN_TYPE::TT_literal_false: {
						if (!sax->boolean(false)) return false;
						break;
					}
					case TOKEN_TYPE::TT_literal_null: {
						if (!sax->null()) return false;
						break;
					}
					case TOKEN_TYPE::TT_literal_true: {
						if (!sax->boolean(true)) return false;
						break;
					}
					case TOKEN_TYPE::TT_value_integer: {
						if (!sax->number_integer(m_lexer.get_integer())) return false;
						break;
					}
					case TOKEN_TYPE::TT_value_string: {
						if (!sax->string(m_lexer.get_string())) return false;
						break;
					}
                    case TOKEN_TYPE::TT_parse_error: {
						return sax->parse_error(m_lexer.get_position(),m_lexer.get_token_string(),std::invalid_argument("uninitialized-value"));
					}
					case TOKEN_TYPE::TT_end_of_input: {
						if (m_lexer.get_position().total_char_==1) return sax->parse_error(m_lexer.get_position(),m_lexer.get_token_string(),std::invalid_argument("attempting to parse an empty input; check that your input string or stream contains the expected JSON"));
						return sax->parse_error(m_lexer.get_position(),m_lexer.get_token_string(),std::invalid_argument("lit or val-value"));
					}
					case TOKEN_TYPE::TT_uninitialized:
					case TOKEN_TYPE::TT_array_end:
  					case TOKEN_TYPE::TT_object_end:
					case TOKEN_TYPE::TT_separator_name:
					case TOKEN_TYPE::TT_separator_value:
					case TOKEN_TYPE::TT_literal_or_value:
					default: {
						return sax->parse_error(m_lexer.get_position(),m_lexer.get_token_string(),std::invalid_argument("default lit or val-value"));
					}
				}
			} else skip_to_state_evaluation=false;
			if (states.empty()) return true;
			if (states.back()) {
				if (get_token()==TOKEN_TYPE::TT_separator_value) {
					get_token();
					continue;
				}
				if (last_token==TOKEN_TYPE::TT_array_end) {
					if (!sax->end_array()) return false;
					states.pop_back();
					skip_to_state_evaluation=true;
					continue;
				}
				return sax->parse_error(m_lexer.get_position(),m_lexer.get_token_string(),std::invalid_argument( "]-array"));
			}
			if (get_token()==TOKEN_TYPE::TT_separator_value) {
				if (get_token()!=TOKEN_TYPE::TT_value_string) return sax->parse_error(m_lexer.get_position(),m_lexer.get_token_string(),std::invalid_argument("valuestr-object key"));
				if (!sax->key(m_lexer.get_string())) return false;
				if (get_token()!=TOKEN_TYPE::TT_separator_name) return sax->parse_error(m_lexer.get_position(),m_lexer.get_token_string(),std::invalid_argument(":-object separator"));
				get_token();
				continue;
			}
			if (last_token==TOKEN_TYPE::TT_object_end) {
				if (!sax->end_object()) return false;
				states.pop_back();
				skip_to_state_evaluation=true;
				continue;
			}
			return sax->parse_error(m_lexer.get_position(),m_lexer.get_token_string(),std::invalid_argument("}-object"));
		}
	}
	token_type get_token() {
		return last_token=m_lexer.scan();
	}
	std::string exception_message(const token_type expected,const std::string& context) {
		std::string error_msg="syntax error ";
		if (!context.empty()) error_msg+="while parsing "+context+" ";
		error_msg+="- ";
		if (last_token==TOKEN_TYPE::TT_parse_error) error_msg+=m_lexer.get_error_message()+"; last read: '"+m_lexer.get_token_string()+"\'";
		else error_msg+="unexpected "+lexer_t::token_type_name(last_token);
		if (expected!=TOKEN_TYPE::TT_uninitialized) error_msg+="; expected "+lexer_t::token_type_name(expected);
		return error_msg;
	}

private:
    const parser_callback_t<JsonType> callback=nullptr;
    token_type last_token=TOKEN_TYPE::TT_uninitialized;
    lexer_t m_lexer;
    const bool allow_exceptions=true;
};

}

namespace iterator {
	
class primitive_iterator_t {
private:
	using difference_type=std::ptrdiff_t;
	static constexpr difference_type begin_value=0;
	static constexpr difference_type end_value=begin_value+1;
	difference_type it_=(std::numeric_limits<std::ptrdiff_t>::min)();
public:
	constexpr difference_type get_value() const noexcept {
		return it_;
	}
	void set_begin() noexcept {
		it_=begin_value;
	}
	void set_end() noexcept {
		it_=end_value;
	}
    constexpr bool is_begin() const noexcept {
		return it_==begin_value;
	}
	constexpr bool is_end() const noexcept {
		return it_==end_value;
	}
	friend constexpr bool operator ==(primitive_iterator_t lhs,primitive_iterator_t rhs) noexcept {
		return lhs.it_==rhs.it_;
	}
	friend constexpr bool operator <(primitive_iterator_t lhs, primitive_iterator_t rhs) noexcept {
		return lhs.it_<rhs.it_;
	}
	primitive_iterator_t operator +(difference_type n) noexcept {
		auto result=*this;
		result+=n;
		return result;
	}
	friend constexpr difference_type operator -(primitive_iterator_t lhs,primitive_iterator_t rhs) noexcept {
		return lhs.it_-rhs.it_;
	}
    primitive_iterator_t& operator ++() noexcept {
		++it_;
		return *this;
	}
    primitive_iterator_t operator ++(int)& noexcept {
		auto result=*this;
		it_++;
		return result;
	}
	primitive_iterator_t& operator --() noexcept {
		--it_;
		return *this;
	}
	primitive_iterator_t operator --(int)& noexcept {
		auto result=*this;
		it_--;
		return result;
	}
    primitive_iterator_t& operator +=(difference_type n) noexcept {
		it_+=n;
		return *this;
	}
    primitive_iterator_t& operator -=(difference_type n) noexcept {
		it_-=n;
		return *this;
	}
};

template<typename JsonType>
struct internal_iterator {
	typename JsonType::object_t::iterator object_iterator {};
	typename JsonType::array_t::iterator array_iterator {};
	primitive_iterator_t primitive_iterator {};
};

template<typename JsonType>
class iter_impl {
    using other_iter_impl=iter_impl<typename std::conditional<std::is_const<JsonType>::value,typename std::remove_const<JsonType>::type,const JsonType>::type>;
    friend other_iter_impl;
    friend JsonType;
    //friend iteration_proxy<iter_impl>;
    //friend iteration_proxy_value<iter_impl>;
    using object_t = typename JsonType::object_t;
    using array_t = typename JsonType::array_t;
    static_assert(is_json<typename std::remove_const<JsonType>::type>::value,"iter_impl only accepts (const) json");
    static_assert(std::is_base_of<std::bidirectional_iterator_tag, 
				  std::bidirectional_iterator_tag>::value && 
				  std::is_base_of<std::bidirectional_iterator_tag,
				  typename std::iterator_traits<typename array_t::iterator>::iterator_category>::value,
                  "json iterator assumes array and object type iterators satisfy the LegacyBidirectionalIterator named requirement.");

public:
    using iterator_category=std::bidirectional_iterator_tag;
    using value_type=JSON_TYPE;
    using difference_type=std::ptrdiff_t;
    using pointer = typename std::conditional<std::is_const<JsonType>::value,
          typename JsonType::c_pointer,
          typename JsonType::pointer>::type;
    using reference=typename std::conditional<std::is_const<JsonType>::value,typename JsonType::c_ref,typename JsonType::ref>::type;
    iter_impl()=default;
    ~iter_impl()=default;
    iter_impl(iter_impl&&) noexcept=default;
    iter_impl& operator=(iter_impl&&) noexcept=default;
    explicit iter_impl(pointer object) noexcept : object_(object) {
		switch (object_->mdata_.type_) {
			case JSON_TYPE::JT_object: {
				it_.object_iterator=typename object_t::iterator();
				break;
			}
			case JSON_TYPE::JT_array: {
				it_.array_iterator=typename array_t::iterator();
				break;
			}
			case JSON_TYPE::JT_null:
			case JSON_TYPE::JT_string:
			case JSON_TYPE::JT_bool:
			case JSON_TYPE::JT_int:
			case JSON_TYPE::JT_float:
			default: {
				it_.primitive_iterator=primitive_iterator_t();
				break;
			}
		}
	}
	iter_impl(const iter_impl<const JsonType>& other) noexcept : object_(other.object_),it_(other.it_) {}
	iter_impl& operator =(const iter_impl<const JsonType>& other) noexcept {
		if (&other!=this) {
			object_=other.object_;
			it_=other.it_;
		}
		return *this;
	}
	iter_impl(const iter_impl<typename std::remove_const<JsonType>::type>& other) noexcept : object_(other.object_), it_(other.it_) {}
	iter_impl& operator =(const iter_impl<typename std::remove_const<JsonType>::type>& other) noexcept {
		object_=other.object_;
		it_=other.it_;
		return *this;
	}
	void set_begin() noexcept {
		switch (object_->mdata_.type_) {
			case JSON_TYPE::JT_object: {
				it_.object_iterator=object_->mdata_.val_.object_->begin();
				break;
			}
			case JSON_TYPE::JT_array: {
				it_.array_iterator=object_->mdata_.val_.array_->begin();
 				break;
			}
			case JSON_TYPE::JT_null: {
				it_.primitive_iterator.set_end();	
				break;
			}
            case JSON_TYPE::JT_string:
            case JSON_TYPE::JT_bool:
            case JSON_TYPE::JT_int:
            case JSON_TYPE::JT_float:
	        default: {
				it_.primitive_iterator.set_begin();
				break;
			}
		}
	}

	void set_end() noexcept {
		switch (object_->mdata_.type_){
			case JSON_TYPE::JT_object: {
				it_.object_iterator=object_->mdata_.val_.object_->end();
				break;
			}
			case JSON_TYPE::JT_array:{
				it_.array_iterator=object_->mdata_.val_.array_->end();
				break;
			}
			case JSON_TYPE::JT_null:
			case JSON_TYPE::JT_string:
			case JSON_TYPE::JT_bool:
			case JSON_TYPE::JT_int:
			case JSON_TYPE::JT_float:
			default: {
				it_.primitive_iterator.set_end();
				break;
			}
		}
	}
	
public:
    reference operator *() const {
		switch (object_->mdata_.type_) {
			case JSON_TYPE::JT_object: return it_.object_iterator->second;
			case JSON_TYPE::JT_array: return *it_.array_iterator;
			case JSON_TYPE::JT_null:
			case JSON_TYPE::JT_string:
			case JSON_TYPE::JT_bool:
			case JSON_TYPE::JT_int:
			case JSON_TYPE::JT_float:
            default: {
				if (it_.primitive_iterator.is_begin()) return *object_;
			}
		}
		return *object_;
	}
#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wreturn-type"
#endif
	pointer operator ->() const {
		switch (object_->mdata_.type_) {
			case JSON_TYPE::JT_object: {
				return &(it_.object_iterator->second);
			}
			case JSON_TYPE::JT_array: {
				return &*it_.array_iterator;
			}
			case JSON_TYPE::JT_null:
			case JSON_TYPE::JT_string:
			case JSON_TYPE::JT_bool:
			case JSON_TYPE::JT_int:
			case JSON_TYPE::JT_float:
			default: {
                if (it_.primitive_iterator.is_begin()) return object_;
			}
		}
	}
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif
	iter_impl operator ++(int)& {
		auto result=*this;
		++(*this);
		return result;
	}
	iter_impl& operator ++() {
		switch (object_->mdata_.type_) {
			case JSON_TYPE::JT_object: {
				std::advance(it_.object_iterator,1);
				break;
			}
			case JSON_TYPE::JT_array: {
				std::advance(it_.array_iterator,1);
				break;
			}
			case JSON_TYPE::JT_null:
			case JSON_TYPE::JT_string:
			case JSON_TYPE::JT_bool:
			case JSON_TYPE::JT_int:
			case JSON_TYPE::JT_float:
			default: {
				it_.primitive_iterator++;
				break;
			}
		}
		return *this;
	}
	iter_impl operator --(int)& {
		auto result=*this;
		--(*this);
		return result;
	}
	iter_impl& operator --() {
		switch (object_->mdata_.type_) {
			case JSON_TYPE::JT_object: {
				std::advance(it_.object_iterator,-1);
				break;
			}
			case JSON_TYPE::JT_array: {
				std::advance(it_.array_iterator,-1);
				break;
			}
			case JSON_TYPE::JT_null:
			case JSON_TYPE::JT_string:
			case JSON_TYPE::JT_bool:
			case JSON_TYPE::JT_int:
			case JSON_TYPE::JT_float:
			default: {
				it_.primitive_iterator--;
				break;
			}
		}
		return *this;
	}
	template <typename IterImpl,std::enable_if_t<(std::is_same<IterImpl,iter_impl>::value || std::is_same<IterImpl, other_iter_impl>::value),std::nullptr_t> = nullptr>
	bool operator ==(const IterImpl& other) const {
        //if (JSON_HEDLEY_UNLIKELY(m_object != other.m_object)) JSON_THROW(invalid_iterator::create(212, "cannot compare iterators of different containers", m_object));
		switch (object_->mdata_.type_) {
			case JSON_TYPE::JT_object:
				return (it_.object_iterator==other.it_.object_iterator);
			case JSON_TYPE::JT_array:
				return (it_.array_iterator==other.it_.array_iterator);
			case JSON_TYPE::JT_null:
			case JSON_TYPE::JT_string:
			case JSON_TYPE::JT_bool:
			case JSON_TYPE::JT_int:
			case JSON_TYPE::JT_float:
			default:
				return (it_.primitive_iterator==other.it_.primitive_iterator);
		}
	}
	template <typename IterImpl,std::enable_if_t<(std::is_same<IterImpl,iter_impl>::value || std::is_same<IterImpl,other_iter_impl>::value),std::nullptr_t> = nullptr>
	bool operator !=(const IterImpl& other) const {
		return !operator==(other);
	}
	bool operator <(const iter_impl& other) const {
        //if (m_object != other.m_object) JSON_THROW(invalid_iterator::create(212, "cannot compare iterators of different containers", m_object));
		switch (object_->mdata_.type_) {
			case JSON_TYPE::JT_object:
                //JSON_THROW(invalid_iterator::create(213, "cannot compare order of object iterators", m_object));
			case JSON_TYPE::JT_array:
				return (it_.array_iterator<other.it_.array_iterator);
			case JSON_TYPE::JT_null:
			case JSON_TYPE::JT_string:
			case JSON_TYPE::JT_bool:
			case JSON_TYPE::JT_int:
			case JSON_TYPE::JT_float:
			default:
				return (it_.primitive_iterator<other.it_.primitive_iterator);
		}
	}
	bool operator <=(const iter_impl& other) const {
		return !other.operator <(*this);
	}
	bool operator >(const iter_impl& other) const {
 		return !operator <=(other);
	}
	bool operator >=(const iter_impl& other) const {
		return !operator <(other);
	}
	iter_impl& operator +=(difference_type i) {
		switch (object_->mdata_.type_) {
			case JSON_TYPE::JT_object:
                //JSON_THROW(invalid_iterator::create(209, "cannot use offsets with object iterators", m_object));
			case JSON_TYPE::JT_array: {
				std::advance(it_.array_iterator,i);
				break;
			}
			case JSON_TYPE::JT_null:
			case JSON_TYPE::JT_string:
			case JSON_TYPE::JT_bool:
			case JSON_TYPE::JT_int:
			case JSON_TYPE::JT_float:
			default: {
				it_.primitive_iterator+=i;
				break;
			}
		}
		return *this;
	}
	iter_impl& operator -=(difference_type i) {
		return operator +=(-i);
	}
	iter_impl operator +(difference_type i) const {
		auto result=*this;
		result+=i;
		return result;
	}
	friend iter_impl operator +(difference_type i,const iter_impl& it) {
		auto result=it;
		result+=i;
		return result;
	}
	iter_impl operator -(difference_type i) const {
		auto result=*this;
		result-=i;
		return result;
	}
	difference_type operator -(const iter_impl& other) const {
		switch (object_->mdata_.type_) {
			case JSON_TYPE::JT_object:
				//JSON_THROW(invalid_iterator::create(209, "cannot use offsets with object iterators", m_object));
			case JSON_TYPE::JT_array:
				return it_.array_iterator-other.it_.array_iterator;
			case JSON_TYPE::JT_null:
			case JSON_TYPE::JT_string:
			case JSON_TYPE::JT_bool:
			case JSON_TYPE::JT_int:
			case JSON_TYPE::JT_float:
			default:
				return it_.primitive_iterator-other.it_.primitive_iterator;
		}
	}
	reference operator [](difference_type n) const {
		switch (object_->mdata_.type_) {
			case JSON_TYPE::JT_object:
                //JSON_THROW(invalid_iterator::create(208, "cannot use operator[] for object iterators", m_object));
			case JSON_TYPE::JT_array:
				return *std::next(it_.array_iterator,n);
			case JSON_TYPE::JT_null:
                //JSON_THROW(invalid_iterator::create(214, "cannot get value",object_));
			case JSON_TYPE::JT_string:
			case JSON_TYPE::JT_bool:
			case JSON_TYPE::JT_int:
			case JSON_TYPE::JT_float:
			default: {
				if (it_.primitive_iterator.get_value()==-n) return *object_;
			}
		}
	}
	const typename object_t::key_type& key() const {
		if (object_->mdata_.type_==JSON_TYPE::JT_object) return it_.object_iterator->first;
	}
	reference value() const {
		return operator*();
	}

private:
	pointer object_=nullptr;
	internal_iterator<typename std::remove_const<JsonType>::type> it_ {};
};

}

////////JSON
class json {
public:
	template<typename>
	struct is_json_ref : std::false_type {};
	template<typename _Tp>
	struct is_json_ref<json_ref<_Tp>> : std::true_type {};
	using int_t=std::ptrdiff_t;
	using float_t=double;
	using boolean_t=bool;
	using string_t=std::string;
	using array_t=std::vector<json>;
	using object_t=std::map<std::string,json>;
	using initializer_list_t=std::initializer_list<json_ref<json>>;
	using parse_event_t=PARSE_STATUS;
	using parser_callback_t=parser_callback_t<json>;
	using error_handler_t=serializer<json>::error_handler_t;
	using ref=json&;
	using c_ref=const json&;
	using pointer=json*;
	using c_pointer=const json*;
	template<typename _Tp>
	using uncvref_t=typename std::remove_cv<typename std::remove_reference<_Tp>::type>::type;
private:
	template<typename _Tp,typename=void>
	struct is_complete_type : std::false_type {};
	template<typename _Tp>
	struct is_complete_type<_Tp,decltype(void(sizeof(_Tp)))> : std::true_type {};
	template<typename CompatibleType,typename=void>
	struct is_compatible_type : std::false_type {};
	template<typename CompatibleType>
	struct is_compatible_type<CompatibleType,
		std::enable_if_t<is_complete_type<CompatibleType>::value>>
	{
    	static constexpr bool value=(std::is_same<CompatibleType,int_t>::value || 
    								 std::is_convertible<CompatibleType,int_t>::value ||
									 std::is_same<CompatibleType,float_t>::value || 
									 std::is_convertible<CompatibleType,float_t>::value ||
									 std::is_same<CompatibleType,boolean_t>::value ||
									 std::is_same<CompatibleType,string_t>::value ||
									 std::is_same<CompatibleType,array_t>::value ||
									 std::is_same<CompatibleType,object_t>::value);
	};
private:
	template<typename _Tp,typename... Args>
	__attribute__((__returns_nonnull__))
	static _Tp* create(Args&&... args) {
		std::allocator<_Tp> alloc;
		auto deleter=[&](_Tp* object) {
			std::allocator_traits<std::allocator<_Tp>>::deallocate(alloc,object,1);
		};
		std::unique_ptr<_Tp,decltype(deleter)> object(std::allocator_traits<std::allocator<_Tp>>::allocate(alloc,1),deleter);
		std::allocator_traits<std::allocator<_Tp>>::construct(alloc,object.get(),std::forward<Args>(args)...);
		return object.release();
	}
public:
	union value_ {
		int_t integer_;
		float_t float_;
		boolean_t boolean_;
		string_t* string_;
		array_t* array_;
		object_t* object_;
		value_()=default;
		value_(int_t v) noexcept : integer_(v) {}
		value_(float_t v) noexcept : float_(v) {}
		value_(boolean_t v) noexcept : boolean_(v) {}
		value_(const string_t& value) : string_(create<string_t>(value)) {}
		value_(string_t&& value) : string_(create<string_t>(std::move(value))) {}
		value_(const array_t& value) : array_(create<array_t>(value)) {}
		value_(array_t&& value) : array_(create<array_t>(std::move(value))) {}
		value_(const object_t& value) : object_(create<object_t>(value)) {}
		value_(object_t&& value) : object_(create<object_t>(std::move(value))) {}
		value_(JSON_TYPE t) {
			switch (t) {
				case JSON_TYPE::JT_object: {
					object_=create<object_t>();
					break;
                }
				case JSON_TYPE::JT_array: {
					array_=create<array_t>();
					break;
                }
                case JSON_TYPE::JT_string: {
					string_=create<string_t>(std::string(""));
					break;
				}
				case JSON_TYPE::JT_bool: {
					boolean_=static_cast<boolean_t>(false);
					break;
				}
				case JSON_TYPE::JT_int: {
					integer_=static_cast<int_t>(0);
 					break;
				}
				case JSON_TYPE::JT_float: {
					float_=static_cast<float_t>(0.0);
					break;
				}
				case JSON_TYPE::JT_null:
				default: {
					object_=nullptr;  
					break;
				}
			}
		}
		void destroy(JSON_TYPE t) {
			if ((t==JSON_TYPE::JT_object && !object_) || (t==JSON_TYPE::JT_array && !array_) || (t==JSON_TYPE::JT_string && !string_)) return;
			if (t==JSON_TYPE::JT_array || t==JSON_TYPE::JT_object) {
                std::vector<json> stack;
                if (t==JSON_TYPE::JT_array) {
					stack.reserve(array_->size());
					std::move(array_->begin(),array_->end(),std::back_inserter(stack));
				} else {
					stack.reserve(object_->size());
					for (auto&& it:*object_) stack.push_back(std::move(it.second));
				}
				while (!stack.empty()) {
					json current_item(std::move(stack.back()));
					stack.pop_back();
                    if (current_item.mdata_.type_==JSON_TYPE::JT_array) {
						std::move(current_item.mdata_.val_.array_->begin(),current_item.mdata_.val_.array_->end(),std::back_inserter(stack));
						current_item.mdata_.val_.array_->clear();
					} else if (current_item.mdata_.type_==JSON_TYPE::JT_object) {
						for (auto&& it:*current_item.mdata_.val_.object_) stack.push_back(std::move(it.second));
						current_item.mdata_.val_.object_->clear();
					}
				}
			}
			switch (t) {
				case JSON_TYPE::JT_object: {
					std::allocator<object_t> alloc;
					std::allocator_traits<decltype(alloc)>::destroy(alloc,object_);
					std::allocator_traits<decltype(alloc)>::deallocate(alloc,object_,1);
					break;
				}
				case JSON_TYPE::JT_array: {
					std::allocator<array_t> alloc;
					std::allocator_traits<decltype(alloc)>::destroy(alloc,array_);
					std::allocator_traits<decltype(alloc)>::deallocate(alloc,array_,1);
					break;
				}
				case JSON_TYPE::JT_string: {
					std::allocator<string_t> alloc;
					std::allocator_traits<decltype(alloc)>::destroy(alloc,string_);
					std::allocator_traits<decltype(alloc)>::deallocate(alloc,string_,1);
					break;
				}
				case JSON_TYPE::JT_null:
				case JSON_TYPE::JT_bool:
				case JSON_TYPE::JT_int:
				case JSON_TYPE::JT_float:
				default: {
					break;
				}
			}
		}
	};
	struct data_ {
		JSON_TYPE type_=JSON_TYPE::JT_null;
		value_ val_={};
		data_(const JSON_TYPE t) : type_(t) , val_(t) { }
		data_(std::size_t cnt,const json& val) : type_(JSON_TYPE::JT_array) {
			val_.array_=create<array_t>(cnt,val);
		}
		data_() noexcept { };//=default;
		data_(data_&&) noexcept=default;
		data_(const data_&) noexcept=delete;
		data_& operator =(data_&&) noexcept=delete;
		data_& operator =(const data_&) noexcept=delete;
		~data_() noexcept {
			val_.destroy(type_);
		}
	};
	data_ mdata_ = {};
public:
	json(const JSON_TYPE t) : mdata_(t) { }
	json(std::nullptr_t=nullptr) noexcept : json(JSON_TYPE::JT_null) { }	
	template<typename CompatibleType,typename U=uncvref_t<CompatibleType>,
 			 std::enable_if_t<!is_json<U>::value && is_compatible_type<U>::value,int> = 0>
	json(CompatibleType&& val) noexcept {
		//std::cout<<(!is_json<U>::value && is_compatible_type<U>::value)<<std::endl;
		if constexpr (std::is_same<U,int_t>::value || 
					 (std::is_convertible<U,int_t>::value && !std::is_same<U,float>::value && !std::is_same<U,double>::value)) { 
			mdata_.type_=JSON_TYPE::JT_int;
			mdata_.val_.integer_=val;
		} else if constexpr (std::is_same<U,float_t>::value || std::is_same<U,float>::value) { 
			mdata_.type_=JSON_TYPE::JT_float;
			mdata_.val_.float_=val;
		} else if constexpr (std::is_same<U,boolean_t>::value) { 
			mdata_.type_=JSON_TYPE::JT_bool;
			mdata_.val_.boolean_=val;
		} else if constexpr (std::is_same<U,string_t>::value) { 
			mdata_.type_=JSON_TYPE::JT_string;
			mdata_.val_.string_=create<string_t>(val);
		} else if constexpr (std::is_same<U,array_t>::value) { 
			mdata_.type_=JSON_TYPE::JT_array;
			mdata_.val_.array_=create<array_t>(std::move(val));
		} else if constexpr (std::is_same<U,object_t>::value) { 
			mdata_.type_=JSON_TYPE::JT_object;
			mdata_.val_.object_=create<object_t>(std::move(val));
		} else {
			mdata_.type_=JSON_TYPE::JT_null;
			mdata_.val_=value_(JSON_TYPE::JT_null);
		}
	}
	json(initializer_list_t init,bool type_deduction=true,JSON_TYPE manual_type=JSON_TYPE::JT_array) {
		bool is_an_object=std::all_of(init.begin(),init.end(),[](const json_ref<json>& element_ref) {
			return element_ref->mdata_.type_==JSON_TYPE::JT_array && element_ref->size()==2 && (*element_ref)[static_cast<std::size_t>(0)].mdata_.type_==JSON_TYPE::JT_string;
		});
		if (!type_deduction && manual_type==JSON_TYPE::JT_array) is_an_object=false;
		if (is_an_object) {
			mdata_.type_=JSON_TYPE::JT_object;
			mdata_.val_=JSON_TYPE::JT_object;
			for (auto& element_ref:init) {
				auto element=element_ref.moved_or_copied();
				mdata_.val_.object_->emplace(std::move(*((*element.mdata_.val_.array_)[0].mdata_.val_.string_)),
											 std::move((*element.mdata_.val_.array_)[1]));
			}
		} else {      
			mdata_.type_=JSON_TYPE::JT_array;
			mdata_.val_.array_=create<array_t>(init.begin(),init.end());
		}
	}
	static json array_(initializer_list_t init={}) {
		return json(init,false,JSON_TYPE::JT_array);
	}
    static json object_(initializer_list_t init={}) {
		return json(init,false,JSON_TYPE::JT_object);
	}
	json(std::size_t cnt,const json& val) : mdata_{cnt,val} { }
	template<typename JsonRef,std::enable_if_t<std::conjunction<is_json_ref<JsonRef>,
			 std::is_same<typename JsonRef::value_type,json>>::value,int> = 0>
	json(const JsonRef& ref) : json(ref.moved_or_copied()) {}
	json(const json& other) {
		mdata_.type_=other.mdata_.type_;
		switch (mdata_.type_) {
			case JSON_TYPE::JT_object: {
				mdata_.val_=*other.mdata_.val_.object_;
				break;
			}
			case JSON_TYPE::JT_array: {
				mdata_.val_=*other.mdata_.val_.array_;
				break;
			}
			case JSON_TYPE::JT_string: {
				mdata_.val_=*other.mdata_.val_.string_;
				break;
			}
			case JSON_TYPE::JT_bool: {
				mdata_.val_.boolean_=other.mdata_.val_.boolean_;
				break;
			}
			case JSON_TYPE::JT_int: {
				mdata_.val_.integer_=other.mdata_.val_.integer_;
				break;
			}
			case JSON_TYPE::JT_float: {
				mdata_.val_.float_=other.mdata_.val_.float_;
				break;
			}
			case JSON_TYPE::JT_null:
			default: {
				break;
			}
		}
	}
	json(json&& other) noexcept : mdata_(std::move(other.mdata_)) {
		other.mdata_.type_=JSON_TYPE::JT_null;
		other.mdata_.val_={};
	}
	json& operator =(json other) noexcept (std::is_nothrow_move_constructible<JSON_TYPE>::value &&
										   std::is_nothrow_move_assignable<JSON_TYPE>::value &&
										   std::is_nothrow_move_constructible<value_>::value &&
										   std::is_nothrow_move_assignable<value_>::value) {
		std::swap(mdata_.type_,other.mdata_.type_);
		std::swap(mdata_.val_,other.mdata_.val_);
		return *this;
	}
	~json() noexcept { }
	
public:
	string_t dump(const int indent=-1,const char indent_char=' ',const bool ensure_ascii=false,const error_handler_t error_handler=error_handler_t::strict) const {
		string_t result;
		serializer<json> s(output_adapter<char,string_t>(result),indent_char,error_handler);
		if (indent>=0) s.dump(*this, true, ensure_ascii, static_cast<unsigned int>(indent));
		else s.dump(*this, false, ensure_ascii, 0);
		return result;
	}
	constexpr JSON_TYPE type() const noexcept { return mdata_.type_; }
	constexpr operator JSON_TYPE() const noexcept { return mdata_.type_; }
	
public:
	boolean_t get_impl(boolean_t*) const {
		if (mdata_.type_==JSON_TYPE::JT_bool) return mdata_.val_.boolean_;
    }
	object_t* get_impl_ptr(object_t*) noexcept {
		return mdata_.type_==JSON_TYPE::JT_object?mdata_.val_.object_:nullptr;
	}
	constexpr const object_t* get_impl_ptr(const object_t*) const noexcept {
		return mdata_.type_==JSON_TYPE::JT_object?mdata_.val_.object_:nullptr;
	}
	array_t* get_impl_ptr(array_t*) noexcept {
		return mdata_.type_==JSON_TYPE::JT_array?mdata_.val_.array_:nullptr;
	}
	constexpr const array_t* get_impl_ptr(const array_t*) const noexcept {
		return mdata_.type_==JSON_TYPE::JT_array?mdata_.val_.array_:nullptr;
	}
	string_t* get_impl_ptr(string_t*) noexcept {
		return mdata_.type_==JSON_TYPE::JT_string?mdata_.val_.string_:nullptr;
	}
	constexpr const string_t* get_impl_ptr(const string_t*) const noexcept {
		return mdata_.type_==JSON_TYPE::JT_string?mdata_.val_.string_:nullptr;
	}
	boolean_t* get_impl_ptr(boolean_t*) noexcept {
		return mdata_.type_==JSON_TYPE::JT_bool?&mdata_.val_.boolean_:nullptr;
	}
	constexpr const boolean_t* get_impl_ptr(const boolean_t*) const noexcept {
		return mdata_.type_==JSON_TYPE::JT_bool?&mdata_.val_.boolean_:nullptr;
	}
	int_t* get_impl_ptr(int_t*) noexcept {
		return mdata_.type_==JSON_TYPE::JT_int?&mdata_.val_.integer_:nullptr;
	}
	constexpr const int_t* get_impl_ptr(const int_t*) const noexcept {
		return mdata_.type_==JSON_TYPE::JT_int?&mdata_.val_.integer_:nullptr;
	}
	float_t* get_impl_ptr(float_t*) noexcept {
		return mdata_.type_==JSON_TYPE::JT_float?&mdata_.val_.float_:nullptr;
	}
	constexpr const float_t* get_impl_ptr(const float_t*) const noexcept {
		return mdata_.type_==JSON_TYPE::JT_float?&mdata_.val_.float_:nullptr;
	}
	template<typename ReferenceType, typename ThisType>
	static ReferenceType get_ref_impl(ThisType& obj) {
		auto* ptr=obj.template get_ptr<typename std::add_pointer<ReferenceType>::type>();
		if (ptr) return *ptr;
	}
	template<typename PointerType,
			 typename std::enable_if<std::is_pointer<PointerType>::value,int>::type>
	auto get_ptr() noexcept -> decltype(std::declval<json&>().get_impl_ptr(std::declval<PointerType>())) {
		return get_impl_ptr(static_cast<PointerType>(nullptr));
	}
	template<typename PointerType,
			 typename std::enable_if<std::is_pointer<PointerType>::value && std::is_const<typename std::remove_pointer<PointerType>::type>::value,int>::type>
	constexpr auto get_ptr() const noexcept -> decltype(std::declval<const json&>().get_impl_ptr(std::declval<PointerType>())) {
		return get_impl_ptr(static_cast<PointerType>(nullptr));
	}
	template<typename PointerType,std::enable_if_t<std::is_pointer<PointerType>::value,int> = 0>
	constexpr auto get_impl() const noexcept -> decltype(std::declval<const json&>().template get_ptr<PointerType>()) {
		return get_ptr<PointerType>();
	}

public:
	ref at(std::size_t idx) {
		if (mdata_.type_==JSON_TYPE::JT_array) return mdata_.val_.array_->at(idx);
	}
	c_ref at(std::size_t idx) const {
		if (mdata_.type_==JSON_TYPE::JT_array) return mdata_.val_.array_->at(idx);
	}
	ref at(const typename object_t::key_type& key) {
		if (mdata_.type_==JSON_TYPE::JT_object) {
			auto it=mdata_.val_.object_->find(key);
			return it->second;
		}
	}
	c_ref at(const typename object_t::key_type& key) const {
		if (mdata_.type_==JSON_TYPE::JT_object) {
			auto it=mdata_.val_.object_->find(key);
			return it->second;
		}
	}
	ref operator [](std::size_t idx) {
		if (mdata_.type_==JSON_TYPE::JT_null) {
			mdata_.type_=JSON_TYPE::JT_array;
			mdata_.val_.array_=create<array_t>();
		}
		if (mdata_.type_==JSON_TYPE::JT_array){
			if (idx>=mdata_.val_.array_->size()) mdata_.val_.array_->resize(idx+1);
			return mdata_.val_.array_->operator [](idx);
		}
	}
	c_ref operator [](std::size_t idx) const {
		return mdata_.val_.array_->operator[](idx);
	}
	ref operator [](typename object_t::key_type key) {
		if (mdata_.type_==JSON_TYPE::JT_null) {
			mdata_.type_=JSON_TYPE::JT_object;
			mdata_.val_.object_=create<object_t>();
		}
		if (mdata_.type_==JSON_TYPE::JT_object) {
			auto result=mdata_.val_.object_->emplace(std::move(key),nullptr);
			return result.first->second;
		}
		return *this;
	}
	c_ref operator [](const typename object_t::key_type& key) const {
		if (mdata_.type_==JSON_TYPE::JT_object) {
			auto it=mdata_.val_.object_->find(key);
			return it->second;
		}
	}
	template<typename _Tp>
	ref operator [](_Tp* key) {
		return operator [](typename object_t::key_type(key));
	}
	template<typename _Tp>
	c_ref operator [](_Tp* key) const {
		return operator [](typename object_t::key_type(key));
	}
	/*
    template<class KeyType, detail::enable_if_t<
                 detail::is_usable_as_basic_json_key_type<basic_json_t, KeyType>::value, int > = 0 >
    reference operator[](KeyType && key)
    {
        // implicitly convert null value to an empty object
        if (is_null())
        {
            m_data.m_type = value_t::object;
            m_data.m_value.object = create<object_t>();
            assert_invariant();
        }

        // operator[] only works for objects
        if (JSON_HEDLEY_LIKELY(is_object()))
        {
            auto result = m_data.m_value.object->emplace(std::forward<KeyType>(key), nullptr);
            return set_parent(result.first->second);
        }

        JSON_THROW(type_error::create(305, detail::concat("cannot use operator[] with a string argument with ", type_name()), this));
    }

    /// @brief access specified object element
    /// @sa https://json.nlohmann.me/api/basic_json/operator%5B%5D/
    template<class KeyType, detail::enable_if_t<
                 detail::is_usable_as_basic_json_key_type<basic_json_t, KeyType>::value, int > = 0 >
    const_reference operator[](KeyType && key) const
    {
        // const operator[] only works for objects
        if (JSON_HEDLEY_LIKELY(is_object()))
        {
            auto it = m_data.m_value.object->find(std::forward<KeyType>(key));
            JSON_ASSERT(it != m_data.m_value.object->end());
            return it->second;
        }

        JSON_THROW(type_error::create(305, detail::concat("cannot use operator[] with a string argument with ", type_name()), this));
    }
    */

public:
	bool empty() const noexcept {
		switch (mdata_.type_) {
			case JSON_TYPE::JT_null: {
				return true;
			}
			case JSON_TYPE::JT_array: {
				return mdata_.val_.array_->empty();
			}
			case JSON_TYPE::JT_object: { 
				return mdata_.val_.object_->empty();
			}
			case JSON_TYPE::JT_string:
			case JSON_TYPE::JT_bool:
			case JSON_TYPE::JT_int:
			case JSON_TYPE::JT_float:
			default: { 
				return false;
			}
		}
	}
	std::size_t size() const noexcept {
		switch (mdata_.type_) {
			case JSON_TYPE::JT_null: {
				return 0;
			}
			case JSON_TYPE::JT_array:{
				return mdata_.val_.array_->size();
			}
			case JSON_TYPE::JT_object: {
				return mdata_.val_.object_->size();
			}
			case JSON_TYPE::JT_string:
			case JSON_TYPE::JT_bool:
			case JSON_TYPE::JT_int:
			case JSON_TYPE::JT_float:
			default: {  
				return 1;
			}
		}
	}
	std::size_t max_size() const noexcept{
		switch (mdata_.type_) {
			case JSON_TYPE::JT_array: {
				return mdata_.val_.array_->max_size();
			}
			case JSON_TYPE::JT_object: {
				return mdata_.val_.object_->max_size();
			}
			case JSON_TYPE::JT_null:
			case JSON_TYPE::JT_string:
			case JSON_TYPE::JT_bool:
			case JSON_TYPE::JT_int:
			case JSON_TYPE::JT_float:
			default: {
				return size();
			}
		}
	}
	void clear() noexcept {
		switch (mdata_.type_) {
			case JSON_TYPE::JT_int: {
				mdata_.val_.integer_=0;
				break;
			}
			case JSON_TYPE::JT_float: {
				mdata_.val_.float_=0.0;
				break;
			}
			case JSON_TYPE::JT_bool: {
				mdata_.val_.boolean_=false;
				break;
			}
			case JSON_TYPE::JT_string: {
				 mdata_.val_.string_->clear();
				break;
			}
			case JSON_TYPE::JT_array: {
				mdata_.val_.array_->clear();
				break;
			}
			case JSON_TYPE::JT_object: {
				mdata_.val_.object_->clear();
				break;
			}
			case JSON_TYPE::JT_null:
			default: {
				break;
			}
		}
	}
	void erase(const std::size_t idx) {
		if (mdata_.type_==JSON_TYPE::JT_array) mdata_.val_.array_->erase(mdata_.val_.array_->begin()+static_cast<std::ptrdiff_t>(idx));
    }
    template<class IteratorType,std::enable_if_t<std::is_same<IteratorType,iterator::iter_impl<json>>::value,int> = 0>
	IteratorType erase(IteratorType pos) {
		IteratorType result=end();
		switch (mdata_.type_) {
			case JSON_TYPE::JT_bool:
			case JSON_TYPE::JT_float:
			case JSON_TYPE::JT_int:
			case JSON_TYPE::JT_string: {
				if (mdata_.type_==JSON_TYPE::JT_string) {
					std::allocator<string_t> alloc;
					std::allocator_traits<decltype(alloc)>::destroy(alloc,mdata_.val_.string_);
					std::allocator_traits<decltype(alloc)>::deallocate(alloc,mdata_.val_.string_,1);
					mdata_.val_.string_=nullptr;
				}
				mdata_.type_=JSON_TYPE::JT_null;
				break;
			}
			case JSON_TYPE::JT_object: {
				result.it_.object_iterator=mdata_.val_.object_->erase(pos.it_.object_iterator);
				break;
			}
			case JSON_TYPE::JT_array: {
				result.it_.array_iterator=mdata_.val_.array_->erase(pos.it_.array_iterator);
				break;
			}
			case JSON_TYPE::JT_null:
			case JSON_TYPE::JT_discarded:
			default: {
				break;
			}      
		}
		return result;
	}

	std::size_t count(const typename object_t::key_type* key) const {
    	return mdata_.type_==JSON_TYPE::JT_object?mdata_.val_.object_->count(*key):0;
	}
	bool contains(const typename object_t::key_type* key) const {
		return mdata_.type_==JSON_TYPE::JT_object && mdata_.val_.object_->find(*key)!=mdata_.val_.object_->end();
	}
	void push_back(json&& val) {
		if (mdata_.type_==JSON_TYPE::JT_null) {
			mdata_.type_=JSON_TYPE::JT_array;
			mdata_.val_=JSON_TYPE::JT_array;
		}
		const auto old_capacity=mdata_.val_.array_->capacity();
		mdata_.val_.array_->push_back(std::move(val));
	}
	ref operator +=(json&& val) {
		push_back(std::move(val));
		return *this;
	}
	void push_back(const json& val) {
		if (mdata_.type_==JSON_TYPE::JT_null) {
			mdata_.type_=JSON_TYPE::JT_array;
			mdata_.val_=JSON_TYPE::JT_array;
		}
		const auto old_capacity=mdata_.val_.array_->capacity();
		mdata_.val_.array_->push_back(val);
	}
	ref operator +=(const json& val) {
		push_back(val);
		return *this;
	}
	void push_back(const typename object_t::value_type& val) {
		if (mdata_.type_==JSON_TYPE::JT_null) {
			mdata_.type_=JSON_TYPE::JT_object;
			mdata_.val_=JSON_TYPE::JT_object;
		}
		auto res=mdata_.val_.object_->insert(val);
	}
	ref operator +=(const typename object_t::value_type& val) {
		push_back(val);
		return *this;
	}
	iterator::iter_impl<json> begin() noexcept {
		iterator::iter_impl<json> result(this);
		result.set_begin();
		return result;
	}
	iterator::iter_impl<const json> begin() const noexcept {
		return cbegin();
	}
	iterator::iter_impl<const json> cbegin() const noexcept {
		iterator::iter_impl<const json>result(this);
		result.set_begin();
		return result;
	}
	iterator::iter_impl<json> end() noexcept {
		iterator::iter_impl<json> result(this);
		result.set_end();
		return result;
	}
	iterator::iter_impl<const json> end() const noexcept {
		return cend();
	}
	iterator::iter_impl<const json> cend() const noexcept {
		iterator::iter_impl<const json> result(this);
		result.set_end();
		return result;
	}
	/*
	reverse_iterator rbegin() noexcept
    {
        return reverse_iterator(end());
    }

    const_reverse_iterator rbegin() const noexcept
    {
        return crbegin();
    }

    reverse_iterator rend() noexcept
    {
        return reverse_iterator(begin());
    }

    const_reverse_iterator rend() const noexcept
    {
        return crend();
    }

    const_reverse_iterator crbegin() const noexcept
    {
        return const_reverse_iterator(cend());
    }

    const_reverse_iterator crend() const noexcept
    {
        return const_reverse_iterator(cbegin());
    }*/
    /*
	void push_back(initializer_list_t init) {
		json const& first_json=**(init.begin());
		if (mdata_.type_==JSON_TYPE::JT_object && init.size()==2 && first_json.mdata_.type_==JSON_TYPE::JT_string) {
			json&& key=init.begin()->moved_or_copied();
			push_back(typename object_t::value_type(std::move(key.get_ref<string_t&>()),(init.begin()+1)->moved_or_copied()));
		} else push_back(json(init));
    }
	ref operator +=(initializer_list_t init) {
		push_back(init);
		return *this;
	}
	*/
	template<class... Args>
	ref emplace_back(Args&&... args) {
		if (mdata_.type_==JSON_TYPE::JT_null) {
			mdata_.type_=JSON_TYPE::JT_array;
			mdata_.val_=JSON_TYPE::JT_array;
		}
		const auto old_capacity=mdata_.val_.array_->capacity();
		mdata_.val_.array_->emplace_back(std::forward<Args>(args)...);
		return mdata_.val_.array_->back();
	}
	void swap(ref other) noexcept (std::is_nothrow_move_constructible<JSON_TYPE>::value &&
								   std::is_nothrow_move_assignable<JSON_TYPE>::value &&
								   std::is_nothrow_move_constructible<value_>::value && 
								   std::is_nothrow_move_assignable<value_>::value) {
		std::swap(mdata_.type_,other.mdata_.type_);
		std::swap(mdata_.val_,other.mdata_.val_);
	}
	friend void swap(ref left,ref right) noexcept (std::is_nothrow_move_constructible<JSON_TYPE>::value &&
												   std::is_nothrow_move_assignable<JSON_TYPE>::value &&
												   std::is_nothrow_move_constructible<value_>::value && 
												   std::is_nothrow_move_assignable<value_>::value) {
		left.swap(right);
	}
	void swap(array_t& other) {
		if (mdata_.type_==JSON_TYPE::JT_array) std::swap(*(mdata_.val_.array_),other);
	}
	void swap(object_t& other) {
		if (mdata_.type_==JSON_TYPE::JT_object) std::swap(*(mdata_.val_.object_),other);
	}
	void swap(string_t& other) {
		if (mdata_.type_==JSON_TYPE::JT_string) std::swap(*(mdata_.val_.string_),other);
	}
	friend std::ostream& operator <<(std::ostream& o,const json& j) {
		const bool pretty_print=o.width()>0;
		const auto indentation=pretty_print?o.width():0;
		o.width(0);
		serializer<json> s(output_adapter<char>(o),o.fill());
		s.dump(j,pretty_print,false,static_cast<unsigned int>(indentation));
		return o;
	}
	friend std::ostream& operator >>(const json& j,std::ostream& o) {
		return o<<j;
	}
	//template<typename InputAdapterType>
	//static parser::parser<json,InputAdapterType> parser(InputAdapterType adapter,parser_callback_t cb=nullptr,const bool allow_exceptions=true,const bool ignore_comments=false) {
	//	return parser<json,InputAdapterType>(std::move(adapter),std::move(cb),allow_exceptions,ignore_comments);
	//}
	template<typename InputType>
	static json parse(InputType&& i,const parser_callback_t cb=nullptr,const bool allow_exceptions=true,const bool ignore_comments=false) {
		json result;
		parser::parser<json,decltype(input_adapter(std::forward<InputType>(i)))>(input_adapter(std::forward<InputType>(i)),cb,allow_exceptions,ignore_comments).parse(true,result);
		return result;
	}
	template<typename IteratorType>
	static json parse(IteratorType first,IteratorType last,const parser_callback_t cb=nullptr,const bool allow_exceptions=true,const bool ignore_comments=false) {
		json result;
		parser::parser<json,iterator_input_adapter<IteratorType>>(input_adapter(std::move(first),std::move(last)),cb,allow_exceptions,ignore_comments).parse(true, result);
		return result;
	}
	template<typename InputType>
	static bool accept(InputType&& i,const bool ignore_comments=false) {
		return parser::parser<json,decltype(input_adapter(std::forward<InputType>(i)))>(input_adapter(std::forward<InputType>(i)),nullptr,false,ignore_comments).accept(true);
	}
	template<typename IteratorType>
	static bool accept(IteratorType first,IteratorType last,const bool ignore_comments=false) {
		return parser::parser<json,iterator_input_adapter<IteratorType>>(input_adapter(std::move(first),std::move(last)),nullptr,false,ignore_comments).accept(true);
	}
    
public:
	__attribute__((__returns_nonnull__)) const char* type_name() const noexcept {
		switch (mdata_.type_){
			case JSON_TYPE::JT_null: return "null";
			case JSON_TYPE::JT_object: return "object";
			case JSON_TYPE::JT_array: return "array";
			case JSON_TYPE::JT_string: return "string";
			case JSON_TYPE::JT_bool: return "boolean";
			case JSON_TYPE::JT_int:
			case JSON_TYPE::JT_float:
			default: return "number";
		}
	}
};

}

#endif