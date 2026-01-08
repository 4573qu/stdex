//Last Modified At 2026/01/03
//@Version 1.0.0.0
#ifndef _STDEX_VISION_COLOR_H_
#define _STDEX_VISION_COLOR_H_ 1

#include <algorithm>
#include <cerrno>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <iomanip>
#include <limits>
#include <sstream>
#include <string>
#include <vector>

namespace stdex {
	
namespace vision {

namespace color {

inline int clamp_u8(int v) {
	return std::clamp(v,0,255);
}

inline int round_u8_from01(float v) {
	v=std::clamp(v,0.0f,1.0f);
	return clamp_u8(static_cast<int>(std::lround(v*255.0f)));
}

inline std::string add_alpha_string(std::string str,float alpha) {
	uint8_t a=static_cast<uint8_t>(std::lround(std::clamp(alpha,0.0f,1.0f)*255.0f));
	std::stringstream ss;
	ss<<str<<std::hex<<std::setw(2)<<std::setfill('0')<<static_cast<int>(a);
	return ss.str();
}

inline float srgb_to_linear(float c) {
	c=std::clamp(c,0.0f,1.0f);
	if (c<=0.04045f) return c/12.92f;
	return std::pow((c+0.055f)/1.055f,2.4f);
}
inline float linear_to_srgb(float c) {
	c=std::clamp(c,0.0f,1.0f);
	if (c<=0.0031308f) return 12.92f*c;
	return 1.055f*std::pow(c,1.0f/2.4f)-0.055f;
}

inline int hex_nibble(char ch,bool& ok) {
	unsigned char c=static_cast<unsigned char>(ch);
	if (c>='0' && c<='9') return c-'0';
	if (c>='a' && c<='f') return 10+(c-'a');
	if (c>='A' && c<='F') return 10+(c-'A');
	ok=false;
	return 0;
}

inline bool all_hex(const std::string& s) {
	return std::all_of(s.begin(),s.end(),[](unsigned char ch){
		return std::isxdigit(ch)!=0;
	});
}

inline std::size_t parse_hex_color_to_rgba(int& r,int& g,int& b,int& a,const std::string& s,bool strict) {
	if (s.empty()) return 0;
	std::size_t pos=0;
	if (s.size()>=2 && s[0]=='0' && (s[1]=='x' || s[1]=='X')) pos=2;
	else if (s[0]=='#') pos=1;
	if (pos>=s.size()) return 0;
	std::size_t remain=s.size()-pos;
	auto accept_len=[&](std::size_t need)->bool{
		if (strict) return remain==need;
		return remain>=need;
	};
	auto parse_short=[&](bool with_a)->std::size_t{
		std::size_t need=with_a?4:3;
		if (!accept_len(need)) return 0;
		std::string t=s.substr(pos,need);
		if (!all_hex(t)) return 0;
		bool ok=true;
		int rn=hex_nibble(t[0],ok);
		int gn=hex_nibble(t[1],ok);
		int bn=hex_nibble(t[2],ok);
		int an=with_a?hex_nibble(t[3],ok):15;
		if (!ok) return 0;
		r=rn*16+rn;
		g=gn*16+gn;
		b=bn*16+bn;
		a=an*16+an;
		return pos+need;
	};
	auto parse_long=[&](bool with_a)->std::size_t{
		std::size_t need=with_a?8:6;
		if (!accept_len(need)) return 0;
		std::string t=s.substr(pos,need);
		if (!all_hex(t)) return 0;
		bool ok=true;
		auto byte_at=[&](int idx)->int{
			int hi=hex_nibble(t[idx],ok);
			int lo=hex_nibble(t[idx+1],ok);
			return hi*16+lo;
		};
		r=byte_at(0);
		g=byte_at(2);
		b=byte_at(4);
		a=with_a?byte_at(6):255;
		if (!ok) return 0;
		return pos+need;
	};
	std::size_t used=0;
	if (accept_len(8)) {
		used=parse_long(true);
		if (used) return used;
	}
	if (accept_len(6)) {
		used=parse_long(false);
		if (used) return used;
	}
	if (accept_len(4)) {
		used=parse_short(true);
		if (used) return used;
	}
	if (accept_len(3)) {
		used=parse_short(false);
		if (used) return used;
	}
	return 0;
}

struct rgba {
	int r_,g_,b_;
	float a_;
	rgba () : r_(0) , g_(0) , b_(0) , a_(0) { }
	rgba (int r,int g,int b) : r_(r) , g_(g) , b_(b) , a_(0) { }
	rgba (int r,int g,int b,float a) : r_(r) , g_(g) , b_(b) , a_(a) { }
	rgba (int r,int g,int b,int a) : r_(r) , g_(g) , b_(b) {
		set_a(a);
	}
	int& red() { return r_; }
	int& green() { return g_; }
	int& blue() { return b_; }
	float& alpha() { return a_; }
	std::string to_rgb_string() {
		std::stringstream ss;
		ss<<"#"<<std::hex<<std::setw(2)<<std::setfill('0')<<r_<<std::setw(2)<<std::setfill('0')<<g_<<std::setw(2)<<std::setfill('0')<<b_;
		return ss.str();
	}
	std::string to_rgba_string() {
		return add_alpha_string(to_rgb_string(),a_);
	}
	std::size_t from_string(const std::string& s,bool initialize=false,bool strict=true) {
		if (initialize) { r_=g_=b_=0; a_=0; }
		int r=0,g=0,b=0,a=255;
		std::size_t used=parse_hex_color_to_rgba(r,g,b,a,s,strict);
		if (!used) return 0;
		set_rgb(r,g,b);
		set_a(a);
		return used;
	}
	void set_r(int r) { r_=std::clamp(r,0,255); } 
	void set_g(int g) { g_=std::clamp(g,0,255); } 
	void set_b(int b) { b_=std::clamp(b,0,255); } 
	void set_rgb(int r,int g,int b) {
		set_r(r);
		set_g(g);
		set_b(b);
	}
	void set_a(float a) { a_=std::clamp(a,0.0f,1.0f); }
	void set_a(int a) {
		float real_a=a/255.0f;
		set_a(real_a);
	}
	bool operator ==(const rgba& other) const {
		return r_==other.r_ && g_==other.g_ && b_==other.b_ && a_==other.a_;
	}
	bool operator !=(const rgba& other) const {
		return !(*this==other);
	}
};

struct hsla {
	float h_,s_,l_,a_;
	hsla () : h_(0) , s_(0) , l_(0) , a_(0) { }
	hsla (float h,float s,float l) : h_(h) , s_(s) , l_(l) , a_(0) { }
	hsla (float h,float s,float l,float a) : h_(h) , s_(s) , l_(l) , a_(a) { }
	hsla (float h,float s,float l,int a) : h_(h) , s_(s) , l_(l) {
		set_a(a);
	}
	float& hue() { return h_; }
	float& saturation() { return s_; }
	float& lightness() { return l_; }
	float& alpha() { return a_; }
	std::string to_hsl_string() {
		uint16_t ph=static_cast<uint16_t>(std::lround(std::clamp(h_,0.0f,360.0f)*100.0f));
		uint16_t ps=static_cast<uint16_t>(std::lround(std::clamp(s_,0.0f,100.0f)*100.0f));
		uint16_t pl=static_cast<uint16_t>(std::lround(std::clamp(l_,0.0f,100.0f)*100.0f));
		std::stringstream ss;
		ss<<"#HS"<<std::hex<<std::setw(4)<<std::setfill('0')<<ph<<std::setw(4)<<std::setfill('0')<<ps<<std::setw(4)<<std::setfill('0')<<pl;
		return ss.str();
	}
	std::string to_hsla_string() {
		return add_alpha_string(to_hsl_string(),a_);
	}
	std::size_t from_string(const std::string& s,bool initialize=false,bool strict=true);
	void set_h(float h) { h_=std::clamp(h,0.0f,360.0f); } 
	void set_s(float s) { s_=std::clamp(s,0.0f,100.0f); } 
	void set_l(float l) { l_=std::clamp(l,0.0f,100.0f); } 
	void set_hsl(float h,float s,float l) {
		set_h(h);
		set_s(s);
		set_l(l);
	}
	void set_a(float a) { a_=std::clamp(a,0.0f,1.0f); }
	void set_a(int a) {
		float real_a=a/255.0f;
		set_a(real_a);
	}
	bool operator ==(const hsla& other) const {
		return h_==other.h_ && s_==other.s_ && l_==other.l_ && a_==other.a_;
	}
	bool operator !=(const hsla& other) const {
		return !(*this==other);
	}
};

struct hsva {
	float h_,s_,v_,a_;
	hsva () : h_(0) , s_(0) , v_(0) , a_(0) { }
	hsva (float h,float s,float v) : h_(h) , s_(s) , v_(v) , a_(0) { }
	hsva (float h,float s,float v,float a) : h_(h) , s_(s) , v_(v) , a_(a) { }
	hsva (float h,float s,float v,int a) : h_(h) , s_(s) , v_(v) {
		set_a(a);
	}
	float& hue() { return h_; }
	float& saturation() { return s_; }
	float& value() { return v_; }
	float& alpha() { return a_; }
	std::string to_hsv_string() {
		uint16_t ph=static_cast<uint16_t>(std::lround(std::clamp(h_,0.0f,360.0f)*100.0f));
		uint16_t ps=static_cast<uint16_t>(std::lround(std::clamp(s_,0.0f,100.0f)*100.0f));
		uint16_t pv=static_cast<uint16_t>(std::lround(std::clamp(v_,0.0f,100.0f)*100.0f));
		std::stringstream ss;
		ss<<"#HV"<<std::hex<<std::setw(4)<<std::setfill('0')<<ph<<std::setw(4)<<std::setfill('0')<<ps<<std::setw(4)<<std::setfill('0')<<pv;
		return ss.str();
	}
	std::string to_hsva_string() {
		return add_alpha_string(to_hsv_string(),a_);
	}
	std::size_t from_string(const std::string& s,bool initialize=false,bool strict=true);
	void set_h(float h) { h_=std::clamp(h,0.0f,360.0f); } 
	void set_s(float s) { s_=std::clamp(s,0.0f,100.0f); } 
	void set_v(float v) { v_=std::clamp(v,0.0f,100.0f); } 
	void set_hsv(float h,float s,float v) {
		set_h(h);
		set_s(s);
		set_v(v);
	}
	void set_a(float a) { a_=std::clamp(a,0.0f,1.0f); }
	void set_a(int a) {
		float real_a=a/255.0f;
		set_a(real_a);
	}
	bool operator ==(const hsva& other) const {
		return h_==other.h_ && s_==other.s_ && v_==other.v_ && a_==other.a_;
	}
	bool operator !=(const hsva& other) const {
		return !(*this==other);
	}
};

struct cmyka {
	float c_,m_,y_,k_,a_;
	cmyka () : c_(0) , m_(0) , y_(0) , k_(0) , a_(0) { }
	cmyka (float c,float m,float y,float k) : c_(c) , m_(m) , y_(y) , k_(k) , a_(0) { }
	cmyka (float c,float m,float y,float k,float a) : c_(c) , m_(m) , y_(y) , k_(k) , a_(a) { }
	cmyka (float c,float m,float y,float k,int a) : c_(c) , m_(m) , y_(y) , k_(k) { set_a(a); }
	float& cyan() { return c_; }
	float& magenta() { return m_; }
	float& yellow() { return y_; }
	float& key() { return k_; }
	float& alpha() { return a_; }
	std::string to_cmyk_string() {
		uint16_t pc=static_cast<uint16_t>(std::lround(std::clamp(c_,0.0f,100.0f)*100.0f));
		uint16_t pm=static_cast<uint16_t>(std::lround(std::clamp(m_,0.0f,100.0f)*100.0f));
		uint16_t py=static_cast<uint16_t>(std::lround(std::clamp(y_,0.0f,100.0f)*100.0f));
		uint16_t pk=static_cast<uint16_t>(std::lround(std::clamp(k_,0.0f,100.0f)*100.0f));
		std::stringstream ss;
		ss<<"#CK"<<std::hex<<std::setw(4)<<std::setfill('0')<<pc<<std::setw(4)<<std::setfill('0')<<pm<<std::setw(4)<<std::setfill('0')<<py<<std::setw(4)<<std::setfill('0')<<pk;
		return ss.str();
	}
	std::string to_cmyka_string() {
		return add_alpha_string(to_cmyk_string(),a_);
	}
	std::size_t from_string(const std::string& s,bool initialize=false,bool strict=true);
	void set_c(float c) { c_=std::clamp(c,0.0f,100.0f); }
	void set_m(float m) { m_=std::clamp(m,0.0f,100.0f); }
	void set_y(float y) { y_=std::clamp(y,0.0f,100.0f); }
	void set_k(float k) { k_=std::clamp(k,0.0f,100.0f); }
	void set_cmyk(float c,float m,float y,float k) {
		set_c(c);
		set_m(m);
		set_y(y);
		set_k(k);
	}
	void set_a(float a) { a_=std::clamp(a,0.0f,1.0f); }
	void set_a(int a) {
		float real_a=a/255.0f;
		set_a(real_a);
	}
	bool operator ==(const cmyka& other) const {
		return c_==other.c_ && m_==other.m_ && y_==other.y_ && k_==other.k_ && a_==other.a_;
	}
	bool operator !=(const cmyka& other) const {
		return !(*this==other);
	}
};

struct yuva {
	float y_,u_,v_,a_;
	yuva () : y_(0) , u_(0) , v_(0) , a_(0) { }
	yuva (float y,float u,float v) : y_(y) , u_(u) , v_(v) , a_(0) { }
	yuva (float y,float u,float v,float a) : y_(y) , u_(u) , v_(v) , a_(a) { }
	yuva (float y,float u,float v,int a) : y_(y) , u_(u) , v_(v) { set_a(a); }
	float& luma() { return y_; }
	float& u() { return u_; }
	float& v() { return v_; }
	float& alpha() { return a_; }
	std::string to_yuv_string() {
		uint16_t py=static_cast<uint16_t>(std::lround(std::clamp(y_,0.0f,1.0f)*65535.0f));
		int16_t iu=static_cast<int16_t>(std::lround(std::clamp(u_,-0.5f,0.5f)*65535.0f));
		int16_t iv=static_cast<int16_t>(std::lround(std::clamp(v_,-0.5f,0.5f)*65535.0f));
		std::stringstream ss;
		ss<<"#YU"<<std::hex<<std::setw(4)<<std::setfill('0')<<py<<std::setw(4)<<std::setfill('0')<<static_cast<uint16_t>(iu)<<std::setw(4)<<std::setfill('0')<<static_cast<uint16_t>(iv);
		return ss.str();
	}
	std::string to_yuva_string() {
		return add_alpha_string(to_yuv_string(),a_);
	}
	std::size_t from_string(const std::string& s,bool initialize=false,bool strict=true);
	void set_y(float y) { y_=std::clamp(y,0.0f,1.0f); }
	void set_u(float u) { u_=std::clamp(u,-0.5f,0.5f); }
	void set_v(float v) { v_=std::clamp(v,-0.5f,0.5f); }
	void set_yuv(float y,float u,float v) {
		set_y(y);
		set_u(u);
		set_v(v);
	}
	void set_a(float a) { a_=std::clamp(a,0.0f,1.0f); }
	void set_a(int a) {
		float real_a=a/255.0f;
		set_a(real_a);
	}
	bool operator ==(const yuva& other) const {
		return y_==other.y_ && u_==other.u_ && v_==other.v_ && a_==other.a_;
	}
	bool operator !=(const yuva& other) const {
		return !(*this==other);
	}
};

struct xyza {
	float x_,y_,z_,a_;
	xyza () : x_(0) , y_(0) , z_(0) , a_(0) { }
	xyza (float x,float y,float z) : x_(x) , y_(y) , z_(z) , a_(0) { }
	xyza (float x,float y,float z,float a) : x_(x) , y_(y) , z_(z) , a_(a) { }
	float& x() { return x_; }
	float& y() { return y_; }
	float& z() { return z_; }
	float& alpha() { return a_; }
	std::string to_xyz_string() {
		auto pack_u16_0_2=[](float v)->uint16_t{
			v=std::clamp(v,0.0f,2.0f);
			return static_cast<uint16_t>(std::lround((v/2.0f)*65535.0f));
		};
		uint16_t px=pack_u16_0_2(x_);
		uint16_t py=pack_u16_0_2(y_);
		uint16_t pz=pack_u16_0_2(z_);
		std::stringstream ss;
		ss<<"#XZ"<<std::hex<<std::setw(4)<<std::setfill('0')<<px<<std::setw(4)<<std::setfill('0')<<py<<std::setw(4)<<std::setfill('0')<<pz;
		return ss.str();
	}
	std::string to_xyza_string() {
		return add_alpha_string(to_xyz_string(),a_);
	}
	std::size_t from_string(const std::string& s,bool initialize=false,bool strict=true);
	void set_x(float x) { x_=std::max(0.0f,x); }
	void set_y(float y) { y_=std::max(0.0f,y); }
	void set_z(float z) { z_=std::max(0.0f,z); }
	void set_xyz(float x,float y,float z) {
		set_x(x);
		set_y(y);
		set_z(z);
	}
	void set_a(float a) { a_=std::clamp(a,0.0f,1.0f); }
	void set_a(int a) {
		float real_a=a/255.0f;
		set_a(real_a);
	}
	bool operator ==(const xyza& other) const {
		return x_==other.x_ && y_==other.y_ && z_==other.z_ && a_==other.a_;
	}
	bool operator !=(const xyza& other) const {
		return !(*this==other);
	}
};

struct laba {
	float l_,a_,b_,alpha_;
	laba () : l_(0) , a_(0) , b_(0) , alpha_(0) { }
	laba (float l,float a,float b) : l_(l) , a_(a) , b_(b) , alpha_(0) { }
	laba (float l,float a,float b,float alpha) : l_(l) , a_(a) , b_(b) , alpha_(alpha) { }
	float& lightness() { return l_; }
	float& a() { return a_; }
	float& b() { return b_; }
	float& alpha() { return alpha_; }
	std::string to_lab_string() {
		uint16_t pl=static_cast<uint16_t>(std::lround(std::clamp(l_,0.0f,100.0f)*100.0f));
		int16_t pa=static_cast<int16_t>(std::lround(std::clamp(a_,-128.0f,127.0f)*100.0f));
		int16_t pb=static_cast<int16_t>(std::lround(std::clamp(b_,-128.0f,127.0f)*100.0f));
		std::stringstream ss;
		ss<<"#LB"<<std::hex<<std::setw(4)<<std::setfill('0')<<pl<<std::setw(4)<<std::setfill('0')<<static_cast<uint16_t>(pa)<<std::setw(4)<<std::setfill('0')<<static_cast<uint16_t>(pb);
		return ss.str();
	}
	std::string to_laba_string() {
		return add_alpha_string(to_lab_string(),alpha_);
	}
	std::size_t from_string(const std::string& s,bool initialize=false,bool strict=true);
	void set_l(float l) { l_=std::clamp(l,0.0f,100.0f); }
	void set_a(float a) { a_=a; }
	void set_b(float b) { b_=b; }
	void set_lab(float l,float a,float b) {
		set_l(l);
		set_a(a);
		set_b(b);
	}
	void set_alpha(float alpha) { alpha_=std::clamp(alpha,0.0f,1.0f); }
	void set_alpha(int a) {
		float real_a=a/255.0f;
		set_a(real_a);
	}
	bool operator ==(const laba& other) const {
		return l_==other.l_ && a_==other.a_ && b_==other.b_ && alpha_==other.alpha_;
	}
	bool operator !=(const laba& other) const {
		return !(*this==other);
	}
};

struct lcha {
	float l_,c_,h_,alpha_;
	lcha () : l_(0) , c_(0) , h_(0) , alpha_(0) { }
	lcha (float l,float c,float h) : l_(l) , c_(c) , h_(h) , alpha_(0) { }
	lcha (float l,float c,float h,float alpha) : l_(l) , c_(c) , h_(h) , alpha_(alpha) { }
	float& lightness() { return l_; }
	float& chroma() { return c_; }
	float& hue() { return h_; }
	float& alpha() { return alpha_; }
	std::string to_lch_string() {
		uint16_t pl=static_cast<uint16_t>(std::lround(std::clamp(l_,0.0f,100.0f)*100.0f));
		uint16_t pc=static_cast<uint16_t>(std::lround(std::clamp(c_,0.0f,200.0f)*100.0f)); // cap 200
		uint16_t ph=static_cast<uint16_t>(std::lround(std::clamp(h_,0.0f,360.0f)*100.0f));
		std::stringstream ss;
		ss<<"#LC"<<std::hex<<std::setw(4)<<std::setfill('0')<<pl<<std::setw(4)<<std::setfill('0')<<pc<<std::setw(4)<<std::setfill('0')<<ph;
		return ss.str();
	}
	std::string to_lcha_string() {
		return add_alpha_string(to_lch_string(),alpha_);
	}
	std::size_t from_string(const std::string& s,bool initialize=false,bool strict=true);
	void set_l(float l) { l_=std::clamp(l,0.0f,100.0f); }
	void set_c(float c) { c_=std::max(0.0f,c); }
	void set_h(float h) { h_=std::clamp(h,0.0f,360.0f); }
	void set_lch(float l,float c,float h) {
		set_l(l);
		set_c(c);
		set_h(h);
	}
	void set_alpha(float alpha) { alpha_=std::clamp(alpha,0.0f,1.0f); }
	void set_alpha(int a) {
		float real_a=a/255.0f;
		set_alpha(real_a);
	}
	bool operator ==(const lcha& other) const {
		return l_==other.l_ && c_==other.c_ && h_==other.h_ && alpha_==other.alpha_;
	}
	bool operator !=(const lcha& other) const {
		return !(*this==other);
	}
};

struct oklaba {
	float l_,a_,b_,alpha_;
	oklaba () : l_(0) , a_(0) , b_(0) , alpha_(0) { }
	oklaba (float l,float a,float b) : l_(l) , a_(a) , b_(b) , alpha_(0) { }
	oklaba (float l,float a,float b,float alpha) : l_(l) , a_(a) , b_(b) , alpha_(alpha) { }
	float& lightness() { return l_; }
	float& a() { return a_; }
	float& b() { return b_; }
	float& alpha() { return alpha_; }
	std::string to_oklab_string() {
		uint16_t pl=static_cast<uint16_t>(std::lround(std::clamp(l_,0.0f,1.0f)*65535.0f));
		int16_t pa=static_cast<int16_t>(std::lround(std::clamp(a_,-0.5f,0.5f)*65535.0f));
		int16_t pb=static_cast<int16_t>(std::lround(std::clamp(b_,-0.5f,0.5f)*65535.0f));
		std::stringstream ss;
		ss<<"#OB"<<std::hex<<std::setw(4)<<std::setfill('0')<<pl<<std::setw(4)<<std::setfill('0')<<static_cast<uint16_t>(pa)<<std::setw(4)<<std::setfill('0')<<static_cast<uint16_t>(pb);
		return ss.str();
	}
	std::string to_oklaba_string() {
		return add_alpha_string(to_oklab_string(),alpha_);
	}
	std::size_t from_string(const std::string& s,bool initialize=false,bool strict=true);
	void set_l(float l) { l_=std::clamp(l,0.0f,1.0f); }
	void set_a(float a) { a_=a; }
	void set_b(float b) { b_=b; }
	void set_oklab(float l,float a,float b) {
		set_l(l);
		set_a(a);
		set_b(b);
	}
	void set_alpha(float alpha) { alpha_=std::clamp(alpha,0.0f,1.0f); }
	void set_alpha(int a) {
		float real_a=a/255.0f;
		set_alpha(real_a);
	}
	bool operator ==(const oklaba& other) const {
		return l_==other.l_ && a_==other.a_ && b_==other.b_ && alpha_==other.alpha_;
	}
	bool operator !=(const oklaba& other) const {
		return !(*this==other);
	}
};

struct oklcha {
	float l_,c_,h_,alpha_;
	oklcha () : l_(0) , c_(0) , h_(0) , alpha_(0) { }
	oklcha (float l,float c,float h) : l_(l) , c_(c) , h_(h) , alpha_(0) { }
	oklcha (float l,float c,float h,float alpha) : l_(l) , c_(c) , h_(h) , alpha_(alpha) { }
	float& lightness() { return l_; }
	float& chroma() { return c_; }
	float& hue() { return h_; }
	float& alpha() { return alpha_; }
	std::string to_oklch_string() {
		uint16_t pl=static_cast<uint16_t>(std::lround(std::clamp(l_,0.0f,1.0f)*65535.0f));
		uint16_t pc=static_cast<uint16_t>(std::lround(std::clamp(c_,0.0f,0.5f)*65535.0f));
		uint16_t ph=static_cast<uint16_t>(std::lround(std::clamp(h_,0.0f,360.0f)*100.0f));
		std::stringstream ss;
		ss<<"#OC"<<std::hex<<std::setw(4)<<std::setfill('0')<<pl<<std::setw(4)<<std::setfill('0')<<pc<<std::setw(4)<<std::setfill('0')<<ph;
		return ss.str();
	}
	std::string to_oklcha_string() {
		return add_alpha_string(to_oklch_string(),alpha_);
	}
	std::size_t from_string(const std::string& s,bool initialize=false,bool strict=true);
	void set_l(float l) { l_=std::clamp(l,0.0f,1.0f); }
	void set_c(float c) { c_=std::max(0.0f,c); }
	void set_h(float h) { h_=std::clamp(h,0.0f,360.0f); }
	void set_oklch(float l,float c,float h) {
		set_l(l);
		set_c(c);
		set_h(h);
	}
	void set_alpha(float alpha) { alpha_=std::clamp(alpha,0.0f,1.0f); }
	void set_alpha(int a) {
		float real_a=a/255.0f;
		set_alpha(real_a);
	}
	bool operator ==(const oklcha& other) const {
		return l_==other.l_ && c_==other.c_ && h_==other.h_ && alpha_==other.alpha_;
	}
	bool operator !=(const oklcha& other) const {
		return !(*this==other);
	}
};

struct hwba {
	float h_,w_,b_,a_;
	hwba () : h_(0) , w_(0) , b_(0) , a_(0) { }
	hwba (float h,float w,float b) : h_(h) , w_(w) , b_(b) , a_(0) { }
	hwba (float h,float w,float b,float a) : h_(h) , w_(w) , b_(b) , a_(a) { }
	float& hue() { return h_; }
	float& whiteness() { return w_; }
	float& blackness() { return b_; }
	float& alpha() { return a_; }
	std::string to_hwb_string() {
		uint16_t ph=static_cast<uint16_t>(std::lround(std::clamp(h_,0.0f,360.0f)*100.0f));
		uint16_t pw=static_cast<uint16_t>(std::lround(std::clamp(w_,0.0f,100.0f)*100.0f));
		uint16_t pb=static_cast<uint16_t>(std::lround(std::clamp(b_,0.0f,100.0f)*100.0f));
		std::stringstream ss;
		ss<<"#HW"<<std::hex<<std::setw(4)<<std::setfill('0')<<ph<<std::setw(4)<<std::setfill('0')<<pw<<std::setw(4)<<std::setfill('0')<<pb;
		return ss.str();
	}
	std::string to_hwba_string() {
		return add_alpha_string(to_hwb_string(),a_);
	}
	std::size_t from_string(const std::string& s,bool initialize=false,bool strict=true);
	void set_h(float h) { h_=std::clamp(h,0.0f,360.0f); }
	void set_w(float w) { w_=std::clamp(w,0.0f,100.0f); }
	void set_b(float b) { b_=std::clamp(b,0.0f,100.0f); }
	void set_hwb(float h,float w,float b) {
		set_h(h);
		set_w(w);
		set_b(b);
	}
	void set_a(float a) { a_=std::clamp(a,0.0f,1.0f); }
	void set_a(int a) {
		float real_a=a/255.0f;
		set_a(real_a);
	}
	bool operator ==(const hwba& other) const {
		return h_==other.h_ && w_==other.w_ && b_==other.b_ && a_==other.a_;
	}
	bool operator !=(const hwba& other) const {
		return !(*this==other);
	}
};

inline unsigned long long safe_strtoull_base16(const std::string& s,bool& success) {
	if (s.empty()) {
		success=false;
		return 0;
	}
	for (unsigned char ch:s) {
		if (std::isxdigit(ch)==0) {
			success=false;
			return 0;
		}
	}
	char* end=nullptr;
	errno=0;
	unsigned long long val=std::strtoull(s.c_str(),&end,16);
	if (end==nullptr || *end!='\0' || errno==ERANGE) {
		success=false;
		return 0;
	}
	return val;
}

inline std::size_t parse_packed_prefix(const std::string& s,std::size_t& pos,char p0,char p1) {
	pos=0;
	if (s.empty()) return 0;
	if (s[0]=='#') pos=1;
	if (s.size()<pos+2) return 0;
	if (!(s[pos]==p0 && s[pos+1]==p1)) return 0;
	pos+=2;
	return pos;
}

inline bool read_hex_u16(const std::string& s,std::size_t off,uint16_t& v) {
	if (off+4> s.size()) return false;
	bool ok=true;
	unsigned long long val=safe_strtoull_base16(s.substr(off,4),ok);
	if (!ok || val>0xFFFFULL) return false;
	v=static_cast<uint16_t>(val);
	return true;
}

inline bool read_hex_u8(const std::string& s,std::size_t off,uint8_t& v) {
	if (off+2> s.size()) return false;
	bool ok=true;
	unsigned long long val=safe_strtoull_base16(s.substr(off,2),ok);
	if (!ok || val>0xFFULL) return false;
	v=static_cast<uint8_t>(val);
	return true;
}

inline hsla rgba_to_hsla(rgba color) {
	float r=color.r_/255.0f;
	float g=color.g_/255.0f;
	float b=color.b_/255.0f;
	float max_val=std::max({r,g,b});
	float min_val=std::min({r,g,b});
	float delta=max_val-min_val;
	hsla result;
	result.l_=(max_val+min_val)/2.0f;
	if (delta<1e-5) result.h_=result.s_=0;
	else  {
		result.s_=(result.l_>0.5f)?(delta/(2.0f-max_val-min_val)):(delta/(max_val+min_val));
		if (max_val==r) result.h_=(g-b)/delta+((g<b)?6.0f:0.0f);
		else if (max_val==g) result.h_=(b-r)/delta+2.0f;
		else result.h_=(r-g)/delta+4.0f;
		result.h_*=60.0f;
		if (result.h_<0) result.h_+=360.0f;
	}
	result.s_*=100.0f;
	result.l_*=100.0f;
	result.a_=color.a_;
	return result;
}

inline rgba hsla_to_rgba(hsla color) {
	float h=color.h_/360.0f;
	float s=color.s_/100.0f;
	float l=color.l_/100.0f;
	if (s<1e-5) {
		int val=static_cast<int>(l*255.0f);
		return rgba(val,val,val,color.a_);
	}
	auto hue_to_rgb=[](float p,float q,float t) {
		while (t<0) t+=1;
		while (t>1) t-=1;
		if (t<1.0f/6) return p+(q-p)*6*t;
		if (t<1.0f/2) return q;
		if (t<2.0f/3) return p+(q-p)*(2.0f/3-t)*6;
		return p;
	};
	float q=(l<0.5f)?(l*(1+s)):(l+s-l*s);
	float p=2*l-q;
	float r=hue_to_rgb(p,q,h+1.0f/3);
	float g=hue_to_rgb(p,q,h);
	float b=hue_to_rgb(p,q,h-1.0f/3);
	return rgba(static_cast<int>(r*255),static_cast<int>(g*255),static_cast<int>(b*255),color.a_);
}

inline hsva rgba_to_hsva(rgba color) {
	float r=color.r_/255.0f;
	float g=color.g_/255.0f;
	float b=color.b_/255.0f;
	float max_val=std::max({r,g,b});
	float min_val=std::min({r,g,b});
	float delta=max_val-min_val;
	hsva result;
	result.v_=max_val*100.0f;
	if (delta<1e-5) result.h_=result.s_=0;
	else  {
		result.s_=(delta/max_val)*100.0f;
		if (max_val==r) result.h_=(g-b)/delta+((g<b)?6.0f:0.0f);
		else if (max_val==g) result.h_=(b-r)/delta+2.0f;
		else result.h_=(r-g)/delta+4.0f;
		result.h_*=60.0f;
		if (result.h_<0) result.h_+=360.0f;
	}
	result.a_=color.a_;
	return result;
}

inline rgba hsva_to_rgba(hsva color) {
	float h=color.h_/360.0f;
	float s=color.s_/100.0f;
	float v=color.v_/100.0f;
	int i=static_cast<int>(h*6);
	float f=h*6-i;
	float p=v*(1-s);
	float q=v*(1-f*s);
	float t=v*(1-(1-f)*s);
	float r,g,b;
	switch (i%6) {
		case 0: { r=v,g=t,b=p; break; }
		case 1: { r=q,g=v,b=p; break; }
		case 2: { r=p,g=v,b=t; break; }
		case 3: { r=p,g=q,b=v; break; }
		case 4: { r=t,g=p,b=v; break; }
		case 5: { r=v,g=p,b=q; break; }
	}
	return rgba(static_cast<int>(r*255),static_cast<int>(g*255),static_cast<int>(b*255),color.a_);
}

hsva hsla_to_hsva(hsla color) {
	return rgba_to_hsva(hsla_to_rgba(color));
}

hsla hsva_to_hsla(hsva color) {
	return rgba_to_hsla(hsva_to_rgba(color));
}

inline cmyka rgba_to_cmyka(rgba color) {
	float r=std::clamp(color.r_/255.0f,0.0f,1.0f);
	float g=std::clamp(color.g_/255.0f,0.0f,1.0f);
	float b=std::clamp(color.b_/255.0f,0.0f,1.0f);
	float k=1.0f-std::max({r,g,b});
	float c=0,m=0,y=0;
	if (k<0.999999f) {
		c=(1.0f-r-k)/(1.0f-k);
		m=(1.0f-g-k)/(1.0f-k);
		y=(1.0f-b-k)/(1.0f-k);
	}
	cmyka result;
	result.c_=std::clamp(c*100.0f,0.0f,100.0f);
	result.m_=std::clamp(m*100.0f,0.0f,100.0f);
	result.y_=std::clamp(y*100.0f,0.0f,100.0f);
	result.k_=std::clamp(k*100.0f,0.0f,100.0f);
	result.a_=std::clamp(color.a_,0.0f,1.0f);
	return result;
}
inline rgba cmyka_to_rgba(cmyka color) {
	float c=std::clamp(color.c_,0.0f,100.0f)/100.0f;
	float m=std::clamp(color.m_,0.0f,100.0f)/100.0f;
	float y=std::clamp(color.y_,0.0f,100.0f)/100.0f;
	float k=std::clamp(color.k_,0.0f,100.0f)/100.0f;
	float r=(1.0f-c)*(1.0f-k);
	float g=(1.0f-m)*(1.0f-k);
	float b=(1.0f-y)*(1.0f-k);
	return rgba(round_u8_from01(r),round_u8_from01(g),round_u8_from01(b),std::clamp(color.a_,0.0f,1.0f));
}

inline yuva rgba_to_yuva(rgba color) {
	float r=std::clamp(color.r_/255.0f,0.0f,1.0f);
	float g=std::clamp(color.g_/255.0f,0.0f,1.0f);
	float b=std::clamp(color.b_/255.0f,0.0f,1.0f);
	float y=0.299f*r+0.587f*g+0.114f*b;
	float u=0.492f*(b-y);
	float v=0.877f*(r-y);
	yuva result;
	result.y_=std::clamp(y,0.0f,1.0f);
	result.u_=std::clamp(u,-0.5f,0.5f);
	result.v_=std::clamp(v,-0.5f,0.5f);
	result.a_=std::clamp(color.a_,0.0f,1.0f);
	return result;
}

inline rgba yuva_to_rgba(yuva color) {
	float y=std::clamp(color.y_,0.0f,1.0f);
	float u=std::clamp(color.u_,-0.5f,0.5f);
	float v=std::clamp(color.v_,-0.5f,0.5f);
	float r=y+v/0.877f;
	float b=y+u/0.492f;
	float g=(y-0.299f*r-0.114f*b)/0.587f;
	return rgba(round_u8_from01(r),round_u8_from01(g),round_u8_from01(b),std::clamp(color.a_,0.0f,1.0f));
}

inline cmyka hsla_to_cmyka(hsla c) {
	return rgba_to_cmyka(hsla_to_rgba(c));
}

inline hsla cmyka_to_hsla(cmyka c) {
	return rgba_to_hsla(cmyka_to_rgba(c));
}

inline cmyka hsva_to_cmyka(hsva c) {
	return rgba_to_cmyka(hsva_to_rgba(c));
}

inline hsva cmyka_to_hsva(cmyka c) {
	return rgba_to_hsva(cmyka_to_rgba(c));
}

inline yuva hsla_to_yuva(hsla c) {
	return rgba_to_yuva(hsla_to_rgba(c));
}

inline hsla yuva_to_hsla(yuva c) {
	return rgba_to_hsla(yuva_to_rgba(c));
}

inline yuva hsva_to_yuva(hsva c) {
	return rgba_to_yuva(hsva_to_rgba(c));
}

inline hsva yuva_to_hsva(yuva c) {
	return rgba_to_hsva(yuva_to_rgba(c)); 
}

enum rgb_working_space {
	RWS_SRGB,
	RWS_REC709,
	RWS_DISPLAY_P3,
};

enum rgb_transfer {
	RT_SRGB,
	RT_LINEAR,
};

struct rgb_profile {
	rgb_working_space space_;
	rgb_transfer transfer_;
	rgb_profile() : space_(RWS_SRGB),transfer_(RT_SRGB) { }
	rgb_profile(rgb_working_space s,rgb_transfer t) : space_(s),transfer_(t) { }
};

struct white_point {
	float x_,y_,z_;
};

inline white_point wp_d65() {
	return white_point{0.95047f,1.00000f,1.08883f};
}

inline float to_linear(rgb_transfer t,float v) {
	return (t==RT_LINEAR)?std::clamp(v,0.0f,1.0f):srgb_to_linear(v);
}

inline float to_nonlinear(rgb_transfer t,float v) {
	return (t==RT_LINEAR)?std::clamp(v,0.0f,1.0f):linear_to_srgb(v);
}

inline void rgb_to_xyz_matrix(rgb_working_space s,float& m00,float& m01,float& m02,float& m10,float& m11,float& m12,float& m20,float& m21,float& m22) {
	switch (s) {
		case RWS_SRGB:
		case RWS_REC709: {
			m00=0.4124564f; m01=0.3575761f; m02=0.1804375f;
			m10=0.2126729f; m11=0.7151522f; m12=0.0721750f;
			m20=0.0193339f; m21=0.1191920f; m22=0.9503041f;
			return;
		}
		case RWS_DISPLAY_P3: {
			m00=0.4865709f; m01=0.2656676f; m02=0.1982173f;
			m10=0.2289746f; m11=0.6917385f; m12=0.0792869f;
			m20=0.0000000f; m21=0.0451134f; m22=1.0439444f;
			return;
		}
	}
	m00=0.4124564f; m01=0.3575761f; m02=0.1804375f;
	m10=0.2126729f; m11=0.7151522f; m12=0.0721750f;
	m20=0.0193339f; m21=0.1191920f; m22=0.9503041f;
}

inline void xyz_to_rgb_matrix(rgb_working_space s,float& m00,float& m01,float& m02,float& m10,float& m11,float& m12,float& m20,float& m21,float& m22) {
	switch (s) {
		case RWS_SRGB:
		case RWS_REC709: {
			m00= 3.2404542f; m01=-1.5371385f; m02=-0.4985314f;
			m10=-0.9692660f; m11= 1.8760108f; m12= 0.0415560f;
			m20= 0.0556434f; m21=-0.2040259f; m22= 1.0572252f;
			return;
		}
		case RWS_DISPLAY_P3: {
			m00= 2.4934969f; m01=-0.9313836f; m02=-0.4027108f;
			m10=-0.8294889f; m11= 1.7626640f; m12= 0.0236247f;
			m20= 0.0358458f; m21=-0.0761724f; m22= 0.9568845f;
			return;
		}
	}
	m00= 3.2404542f; m01=-1.5371385f; m02=-0.4985314f;
	m10=-0.9692660f; m11= 1.8760108f; m12= 0.0415560f;
	m20= 0.0556434f; m21=-0.2040259f; m22= 1.0572252f;
}

inline xyza rgba_to_xyza(rgba c,rgb_profile p=rgb_profile()) {
	float r=to_linear(p.transfer_,std::clamp(c.r_/255.0f,0.0f,1.0f));
	float g=to_linear(p.transfer_,std::clamp(c.g_/255.0f,0.0f,1.0f));
	float b=to_linear(p.transfer_,std::clamp(c.b_/255.0f,0.0f,1.0f));
	float m00,m01,m02,m10,m11,m12,m20,m21,m22;
	rgb_to_xyz_matrix(p.space_,m00,m01,m02,m10,m11,m12,m20,m21,m22);
	xyza result;
	result.x_=m00*r+m01*g+m02*b;
	result.y_=m10*r+m11*g+m12*b;
	result.z_=m20*r+m21*g+m22*b;
	result.a_=std::clamp(c.a_,0.0f,1.0f);
	return result;
}

inline rgba xyza_to_rgba(xyza c,rgb_profile p=rgb_profile()) {
	float m00,m01,m02,m10,m11,m12,m20,m21,m22;
	xyz_to_rgb_matrix(p.space_,m00,m01,m02,m10,m11,m12,m20,m21,m22);
	float r=m00*c.x_+m01*c.y_+m02*c.z_;
	float g=m10*c.x_+m11*c.y_+m12*c.z_;
	float b=m20*c.x_+m21*c.y_+m22*c.z_;
	r=to_nonlinear(p.transfer_,r);
	g=to_nonlinear(p.transfer_,g);
	b=to_nonlinear(p.transfer_,b);
	return rgba(round_u8_from01(r),round_u8_from01(g),round_u8_from01(b),std::clamp(c.a_,0.0f,1.0f));
}

inline float lab_f(float t) {
	const float delta=6.0f/29.0f;
	const float delta3=delta*delta*delta;
	if (t>delta3) return std::cbrt(t);
	return t/(3*delta*delta)+4.0f/29.0f;
}
inline float lab_f_inv(float t) {
	const float delta=6.0f/29.0f;
	if (t>delta) return t*t*t;
	return 3*delta*delta*(t-4.0f/29.0f);
}

inline laba xyza_to_laba(xyza c,white_point wp=wp_d65()) {
	float x=c.x_/wp.x_;
	float y=c.y_/wp.y_;
	float z=c.z_/wp.z_;
	float fx=lab_f(x);
	float fy=lab_f(y);
	float fz=lab_f(z);
	laba result;
	result.l_=std::max(0.0f,116.0f*fy-16.0f);
	result.a_=500.0f*(fx-fy);
	result.b_=200.0f*(fy-fz);
	result.alpha_=std::clamp(c.a_,0.0f,1.0f);
	return result;
}

inline xyza laba_to_xyza(laba c,white_point wp=wp_d65()) {
	float fy=(c.l_+16.0f)/116.0f;
	float fx=fy+c.a_/500.0f;
	float fz=fy-c.b_/200.0f;
	xyza result;
	result.x_=wp.x_*lab_f_inv(fx);
	result.y_=wp.y_*lab_f_inv(fy);
	result.z_=wp.z_*lab_f_inv(fz);
	result.a_=std::clamp(c.alpha_,0.0f,1.0f);
	return result;
}

inline laba rgba_to_laba(rgba c,rgb_profile p=rgb_profile(),white_point wp=wp_d65()) {
	return xyza_to_laba(rgba_to_xyza(c,p),wp);
}

inline rgba laba_to_rgba(laba c,rgb_profile p=rgb_profile(),white_point wp=wp_d65()) {
	return xyza_to_rgba(laba_to_xyza(c,wp),p);
}

inline lcha laba_to_lcha(laba c) {
	float C=std::sqrt(c.a_*c.a_+c.b_*c.b_);
	float H=std::atan2(c.b_,c.a_)*180.0f/3.14159265358979323846f;
	if (H<0) H+=360.0f;
	return lcha(c.l_,C,H,c.alpha_);
}

inline laba lcha_to_laba(lcha c) {
	float hr=c.h_*3.14159265358979323846f/180.0f;
	float a=c.c_*std::cos(hr);
	float b=c.c_*std::sin(hr);
	return laba(c.l_,a,b,c.alpha_);
}

inline lcha rgba_to_lcha(rgba c,rgb_profile p=rgb_profile(),white_point wp=wp_d65()) {
	return laba_to_lcha(rgba_to_laba(c,p,wp));
}

inline rgba lcha_to_rgba(lcha c,rgb_profile p=rgb_profile(),white_point wp=wp_d65()) {
	return laba_to_rgba(lcha_to_laba(c),p,wp);
}

inline oklaba rgba_to_oklaba(rgba c) {
	float r=srgb_to_linear(std::clamp(c.r_/255.0f,0.0f,1.0f));
	float g=srgb_to_linear(std::clamp(c.g_/255.0f,0.0f,1.0f));
	float b=srgb_to_linear(std::clamp(c.b_/255.0f,0.0f,1.0f));
	float l=0.4122214708f*r+0.5363325363f*g+0.0514459929f*b;
	float m=0.2119034982f*r+0.6806995451f*g+0.1073969566f*b;
	float s=0.0883024619f*r+0.2817188376f*g+0.6299787005f*b;
	float l3=std::cbrt(l);
	float m3=std::cbrt(m);
	float s3=std::cbrt(s);
	oklaba result;
	result.l_=0.2104542553f*l3+0.7936177850f*m3-0.0040720468f*s3;
	result.a_=1.9779984951f*l3-2.4285922050f*m3+0.4505937099f*s3;
	result.b_=0.0259040371f*l3+0.7827717662f*m3-0.8086757660f*s3;
	result.alpha_=std::clamp(c.a_,0.0f,1.0f);
	return result;
}

inline rgba oklaba_to_rgba(oklaba c) {
	float l_=c.l_+0.3963377774f*c.a_+0.2158037573f*c.b_;
	float m_=c.l_-0.1055613458f*c.a_-0.0638541728f*c.b_;
	float s_=c.l_-0.0894841775f*c.a_-1.2914855480f*c.b_;
	float l=l_*l_*l_;
	float m=m_*m_*m_;
	float s=s_*s_*s_;
	float r=4.0767416621f*l-3.3077115913f*m+0.2309699292f*s;
	float g=-1.2684380046f*l+2.6097574011f*m-0.3413193965f*s;
	float b=-0.0041960863f*l-0.7034186147f*m+1.7076147010f*s;
	r=linear_to_srgb(r);
	g=linear_to_srgb(g);
	b=linear_to_srgb(b);
	return rgba(round_u8_from01(r),round_u8_from01(g),round_u8_from01(b),std::clamp(c.alpha_,0.0f,1.0f));
}

inline oklcha oklaba_to_oklcha(oklaba c) {
	float C=std::sqrt(c.a_*c.a_+c.b_*c.b_);
	float H=std::atan2(c.b_,c.a_)*180.0f/3.14159265358979323846f;
	if (H<0) H+=360.0f;
	return oklcha(c.l_,C,H,c.alpha_);
}

inline oklaba oklcha_to_oklaba(oklcha c) {
	float hr=c.h_*3.14159265358979323846f/180.0f;
	float a=c.c_*std::cos(hr);
	float b=c.c_*std::sin(hr);
	return oklaba(c.l_,a,b,c.alpha_);
}

inline oklcha rgba_to_oklcha(rgba c) {
	return oklaba_to_oklcha(rgba_to_oklaba(c));
}

inline rgba oklcha_to_rgba(oklcha c) {
	return oklaba_to_rgba(oklcha_to_oklaba(c));
}

inline hsva hwba_to_hsva(hwba c) {
	hsva result;
	result.h_=std::clamp(c.h_,0.0f,360.0f);
	float w=std::clamp(c.w_,0.0f,100.0f)/100.0f;
	float b=std::clamp(c.b_,0.0f,100.0f)/100.0f;
	float v=1.0f-b;
	float s=(v<=1e-6f)?0.0f:1.0f-(w/v);
	result.s_=std::clamp(s*100.0f,0.0f,100.0f);
	result.v_=std::clamp(v*100.0f,0.0f,100.0f);
	result.a_=std::clamp(c.a_,0.0f,1.0f);
	return result;
}

inline hwba hsva_to_hwba(hsva c) {
	hwba result;
	result.h_=std::clamp(c.h_,0.0f,360.0f);
	float s=std::clamp(c.s_,0.0f,100.0f)/100.0f;
	float v=std::clamp(c.v_,0.0f,100.0f)/100.0f;
	float w=(1.0f-s)*v;
	float b=1.0f-v;
	result.w_=std::clamp(w*100.0f,0.0f,100.0f);
	result.b_=std::clamp(b*100.0f,0.0f,100.0f);
	result.a_=std::clamp(c.a_,0.0f,1.0f);
	return result;
}

inline rgba hwba_to_rgba(hwba c) {
	return hsva_to_rgba(hwba_to_hsva(c));
}

inline hwba rgba_to_hwba(rgba c) {
	return hsva_to_hwba(rgba_to_hsva(c));
}

inline float wrap_hue_360(float h) {
	h=std::fmod(h,360.0f);
	if (h<0) h+=360.0f;
	return h;
}
inline float lerp_hue_shortest(float h1,float h2,float t) {
	h1=wrap_hue_360(h1);
	h2=wrap_hue_360(h2);
	float d=h2-h1;
	if (d>180.0f) d-=360.0f;
	if (d<-180.0f) d+=360.0f;
	return wrap_hue_360(h1+d*std::clamp(t,0.0f,1.0f));
}

inline lcha lerp_lcha(lcha a,lcha b,float t) {
	t=std::clamp(t,0.0f,1.0f);
	lcha result;
	result.l_=a.l_+(b.l_-a.l_)*t;
	result.c_=a.c_+(b.c_-a.c_)*t;
	result.h_=lerp_hue_shortest(a.h_,b.h_,t);
	result.alpha_=a.alpha_+(b.alpha_-a.alpha_)*t;
	return result;
}

inline oklcha lerp_oklcha(oklcha a,oklcha b,float t) {
	t=std::clamp(t,0.0f,1.0f);
	oklcha result;
	result.l_=a.l_+(b.l_-a.l_)*t;
	result.c_=a.c_+(b.c_-a.c_)*t;
	result.h_=lerp_hue_shortest(a.h_,b.h_,t);
	result.alpha_=a.alpha_+(b.alpha_-a.alpha_)*t;
	return result;
}

inline float delta_e76(laba a,laba b) {
	float dl=a.l_-b.l_;
	float da=a.a_-b.a_;
	float db=a.b_-b.b_;
	return std::sqrt(dl*dl+da*da+db*db);
}
inline float delta_e_ok(oklaba a,oklaba b) {
	float dl=a.l_-b.l_;
	float da=a.a_-b.a_;
	float db=a.b_-b.b_;
	return std::sqrt(dl*dl+da*da+db*db);
}

inline rgba kelvin_to_rgba(float kelvin,float alpha=1.0f) {
	float t=std::clamp(kelvin,1000.0f,40000.0f)/100.0f;
	float r,g,b;
	if (t<=66.0f) r=1.0f;
	else {
		float x=t-60.0f;
		r=329.698727446f*std::pow(x,-0.1332047592f)/255.0f;
	}
	if (t<=66.0f) g=(99.4708025861f*std::log(t)-161.1195681661f)/255.0f;
	else {
		float x=t-60.0f;
		g=288.1221695283f*std::pow(x,-0.0755148492f)/255.0f;
	}
	if (t>=66.0f) b=1.0f;
	else if (t<=19.0f) b=0.0f;
	else b=(138.5177312231f*std::log(t-10.0f)-305.0447927307f)/255.0f;
	return rgba(round_u8_from01(r),round_u8_from01(g),round_u8_from01(b),std::clamp(alpha,0.0f,1.0f));
}

inline float relative_luminance(rgba c) {
	float r=srgb_to_linear(std::clamp(c.r_/255.0f,0.0f,1.0f));
	float g=srgb_to_linear(std::clamp(c.g_/255.0f,0.0f,1.0f));
	float b=srgb_to_linear(std::clamp(c.b_/255.0f,0.0f,1.0f));
	return 0.2126f*r+0.7152f*g+0.0722f*b;
}

inline float contrast_ratio(rgba a,rgba b) {
	float la=relative_luminance(a);
	float lb=relative_luminance(b);
	float l1=std::max(la,lb);
	float l2=std::min(la,lb);
	return (l1+0.05f)/(l2+0.05f);
}

inline rgba composite_over(rgba src,rgba dst) {
	float sa=std::clamp(src.a_,0.0f,1.0f);
	float da=std::clamp(dst.a_,0.0f,1.0f);
	float out_a=sa+da*(1.0f-sa);
	if (out_a<1e-6f) return rgba(0,0,0,0.0f);
	float sr=std::clamp(src.r_/255.0f,0.0f,1.0f);
	float sg=std::clamp(src.g_/255.0f,0.0f,1.0f);
	float sb=std::clamp(src.b_/255.0f,0.0f,1.0f);
	float dr=std::clamp(dst.r_/255.0f,0.0f,1.0f);
	float dg=std::clamp(dst.g_/255.0f,0.0f,1.0f);
	float db=std::clamp(dst.b_/255.0f,0.0f,1.0f);
	float out_r=(sr*sa+dr*da*(1.0f-sa))/out_a;
	float out_g=(sg*sa+dg*da*(1.0f-sa))/out_a;
	float out_b=(sb*sa+db*da*(1.0f-sa))/out_a;
	return rgba(round_u8_from01(out_r),round_u8_from01(out_g),round_u8_from01(out_b),out_a);
}


inline rgba lerp_rgba(rgba a,rgba b,float t) {
	t=std::clamp(t,0.0f,1.0f);
	int r=static_cast<int>(std::lround(a.r_+(b.r_-a.r_)*t));
	int g=static_cast<int>(std::lround(a.g_+(b.g_-a.g_)*t));
	int bl=static_cast<int>(std::lround(a.b_+(b.b_-a.b_)*t));
	float al=a.a_+(b.a_-a.a_)*t;
	return rgba(clamp_u8(r),clamp_u8(g),clamp_u8(bl),std::clamp(al,0.0f,1.0f));
}

inline rgba lerp_rgba_linear(rgba a,rgba b,float t) {
	t=std::clamp(t,0.0f,1.0f);
	float ar=srgb_to_linear(std::clamp(a.r_/255.0f,0.0f,1.0f));
	float ag=srgb_to_linear(std::clamp(a.g_/255.0f,0.0f,1.0f));
	float ab=srgb_to_linear(std::clamp(a.b_/255.0f,0.0f,1.0f));
	float br=srgb_to_linear(std::clamp(b.r_/255.0f,0.0f,1.0f));
	float bg=srgb_to_linear(std::clamp(b.g_/255.0f,0.0f,1.0f));
	float bb=srgb_to_linear(std::clamp(b.b_/255.0f,0.0f,1.0f));
	float rr=ar+(br-ar)*t;
	float rg=ag+(bg-ag)*t;
	float rb=ab+(bb-ab)*t;
	int r=round_u8_from01(linear_to_srgb(rr));
	int g=round_u8_from01(linear_to_srgb(rg));
	int bl=round_u8_from01(linear_to_srgb(rb));
	float al=a.a_+(b.a_-a.a_)*t;
	return rgba(r,g,bl,std::clamp(al,0.0f,1.0f));
}

enum blend_mode {
	BM_NORMAL,
	BM_MULTIPLY,
	BM_SCREEN,
	BM_OVERLAY,
	BM_DARKEN,
	BM_LIGHTEN,
	BM_COLOR_DODGE,
	BM_COLOR_BURN,
	BM_HARD_LIGHT,
	BM_SOFT_LIGHT,
	BM_DIFFERENCE,
	BM_EXCLUSION,
};

inline float blend_channel(blend_mode mode,float b,float s) {
	b=std::clamp(b,0.0f,1.0f);
	s=std::clamp(s,0.0f,1.0f);
	switch (mode) {
		case BM_NORMAL: {
			return s;
		}
		case BM_MULTIPLY: {
			return b*s;
		}
		case BM_SCREEN: {
			return 1.0f-(1.0f-b)*(1.0f-s);
		}
		case BM_OVERLAY: {
			return (b<0.5f)?(2.0f*b*s):(1.0f-2.0f*(1.0f-b)*(1.0f-s));
		}
		case BM_DARKEN: {
			return std::min(b,s);
		}
		case BM_LIGHTEN: {
			return std::max(b,s);
		}
		case BM_COLOR_DODGE: {
			return (s>=1.0f)?1.0f:std::min(1.0f,b/(1.0f-s));
		}
		case BM_COLOR_BURN: {
			return (s<=0.0f)?0.0f:1.0f-std::min(1.0f,(1.0f-b)/s);
		}
		case BM_HARD_LIGHT: {
			return (s<0.5f)?(2.0f*b*s):(1.0f-2.0f*(1.0f-b)*(1.0f-s));
		}
		case BM_SOFT_LIGHT: {
			if (s<=0.5f) return b-(1.0f-2.0f*s)*b*(1.0f-b);
			float d=(b<=0.25f)?(((16.0f*b-12.0f)*b+4.0f)*b):std::sqrt(b);
			return b+(2.0f*s-1.0f)*(d-b);
		}
		case BM_DIFFERENCE: {
			return std::fabs(b-s);
		}
		case BM_EXCLUSION: {
			return b+s-2.0f*b*s;
		}
	}
	return s;
}

inline rgba blend_rgb(rgba base,rgba blend,blend_mode mode) {
	float br=std::clamp(base.r_/255.0f,0.0f,1.0f);
	float bg=std::clamp(base.g_/255.0f,0.0f,1.0f);
	float bb=std::clamp(base.b_/255.0f,0.0f,1.0f);
	float sr=std::clamp(blend.r_/255.0f,0.0f,1.0f);
	float sg=std::clamp(blend.g_/255.0f,0.0f,1.0f);
	float sb=std::clamp(blend.b_/255.0f,0.0f,1.0f);
	float rr=blend_channel(mode,br,sr);
	float rg=blend_channel(mode,bg,sg);
	float rb=blend_channel(mode,bb,sb);
	return rgba(round_u8_from01(rr),round_u8_from01(rg),round_u8_from01(rb),std::clamp(blend.a_,0.0f,1.0f));
}

inline rgba blend_and_composite(rgba base,rgba blend,blend_mode mode) {
	rgba mixed=blend_rgb(base,blend,mode);
	return composite_over(mixed,base);
}

inline std::size_t hsla::from_string(const std::string& s,bool initialize,bool strict) {
	if (initialize) h_=s_=l_=a_=0;
	std::size_t pos=0;
	if (!parse_packed_prefix(s,pos,'H','S')) {
		rgba tmp;
		std::size_t used=tmp.from_string(s,false,strict);
		if (!used) return 0;
		*this=rgba_to_hsla(tmp);
		return used;
	}
	std::size_t remain=s.size()-pos;
	if (strict) {
		if (!(remain==12 || remain==14)) return 0;
	} else {
		if (remain<12) return 0;
	}
	uint16_t ph=0,ps=0,pl=0;
	if (!read_hex_u16(s,pos+0,ph)) return 0;
	if (!read_hex_u16(s,pos+4,ps)) return 0;
	if (!read_hex_u16(s,pos+8,pl)) return 0;
	h_=std::clamp(ph/100.0f,0.0f,360.0f);
	s_=std::clamp(ps/100.0f,0.0f,100.0f);
	l_=std::clamp(pl/100.0f,0.0f,100.0f);
	a_=1.0f;
	std::size_t used=pos+12;
	if (remain>=14) {
		uint8_t aa=0;
		if (!read_hex_u8(s,pos+12,aa)) return 0;
		set_a(static_cast<int>(aa));
		used=pos+14;
	}
	return used;
}

inline std::size_t hsva::from_string(const std::string& s,bool initialize,bool strict) {
	if (initialize) h_=s_=v_=a_=0;
	std::size_t pos=0;
	if (!parse_packed_prefix(s,pos,'H','V')) {
		rgba tmp;
		std::size_t used=tmp.from_string(s,false,strict);
		if (!used) return 0;
		*this=rgba_to_hsva(tmp);
		return used;
	}
	std::size_t remain=s.size()-pos;
	if (strict) {
		if (!(remain==12 || remain==14)) return 0;
	} else {
		if (remain<12) return 0;
	}
	uint16_t ph=0,ps=0,pv=0;
	if (!read_hex_u16(s,pos+0,ph)) return 0;
	if (!read_hex_u16(s,pos+4,ps)) return 0;
	if (!read_hex_u16(s,pos+8,pv)) return 0;
	h_=std::clamp(ph/100.0f,0.0f,360.0f);
	s_=std::clamp(ps/100.0f,0.0f,100.0f);
	v_=std::clamp(pv/100.0f,0.0f,100.0f);
	a_=1.0f;
	std::size_t used=pos+12;
	if (remain>=14) {
		uint8_t aa=0;
		if (!read_hex_u8(s,pos+12,aa)) return 0;
		set_a(static_cast<int>(aa));
		used=pos+14;
	}
	return used;
}

inline std::size_t cmyka::from_string(const std::string& s,bool initialize,bool strict) {
	if (initialize) c_=m_=y_=k_=a_=0;
	std::size_t pos=0;
	if (!parse_packed_prefix(s,pos,'C','K')) {
		rgba tmp;
		std::size_t used=tmp.from_string(s,false,strict);
		if (!used) return 0;
		*this=rgba_to_cmyka(tmp);
		return used;
	}
	std::size_t remain=s.size()-pos;
	if (strict) {
		if (!(remain==16 || remain==18)) return 0;
	} else {
		if (remain<16) return 0;
	}
	uint16_t pc=0,pm=0,py=0,pk=0;
	if (!read_hex_u16(s,pos+0,pc)) return 0;
	if (!read_hex_u16(s,pos+4,pm)) return 0;
	if (!read_hex_u16(s,pos+8,py)) return 0;
	if (!read_hex_u16(s,pos+12,pk)) return 0;
	c_=std::clamp(pc/100.0f,0.0f,100.0f);
	m_=std::clamp(pm/100.0f,0.0f,100.0f);
	y_=std::clamp(py/100.0f,0.0f,100.0f);
	k_=std::clamp(pk/100.0f,0.0f,100.0f);
	a_=1.0f;
	std::size_t used=pos+16;
	if (remain>=18) {
		uint8_t aa=0;
		if (!read_hex_u8(s,pos+16,aa)) return 0;
		set_a(static_cast<int>(aa));
		used=pos+18;
	}
	return used;
}

inline std::size_t yuva::from_string(const std::string& s,bool initialize,bool strict) {
	if (initialize) y_=u_=v_=a_=0;
	std::size_t pos=0;
	if (!parse_packed_prefix(s,pos,'Y','U')) {
		rgba tmp;
		std::size_t used=tmp.from_string(s,false,strict);
		if (!used) return 0;
		*this=rgba_to_yuva(tmp);
		return used;
	}
	std::size_t remain=s.size()-pos;
	if (strict) {
		if (!(remain==12 || remain==14)) return 0;
	} else {
		if (remain<12) return 0;
	}
	uint16_t py=0,pu=0,pv=0;
	if (!read_hex_u16(s,pos+0,py)) return 0;
	if (!read_hex_u16(s,pos+4,pu)) return 0;
	if (!read_hex_u16(s,pos+8,pv)) return 0;
	y_=std::clamp(py/65535.0f,0.0f,1.0f);
	int16_t iu=static_cast<int16_t>(pu);
	int16_t iv=static_cast<int16_t>(pv);
	u_=std::clamp(iu/65535.0f,-0.5f,0.5f);
	v_=std::clamp(iv/65535.0f,-0.5f,0.5f);
	a_=1.0f;
	std::size_t used=pos+12;
	if (remain>=14) {
		uint8_t aa=0;
		if (!read_hex_u8(s,pos+12,aa)) return 0;
		set_a(static_cast<int>(aa));
		used=pos+14;
	}
	return used;
}

inline std::size_t xyza::from_string(const std::string& s,bool initialize,bool strict) {
	if (initialize) x_=y_=z_=a_=0;
	std::size_t pos=0;
	if (!parse_packed_prefix(s,pos,'X','Z')) {
		rgba tmp;
		std::size_t used=tmp.from_string(s,false,strict);
		if (!used) return 0;
		*this=rgba_to_xyza(tmp);
		return used;
	}
	std::size_t remain=s.size()-pos;
	if (strict) {
		if (!(remain==12 || remain==14)) return 0;
	} else {
		if (remain<12) return 0;
	}
	uint16_t px=0,py=0,pz=0;
	if (!read_hex_u16(s,pos+0,px)) return 0;
	if (!read_hex_u16(s,pos+4,py)) return 0;
	if (!read_hex_u16(s,pos+8,pz)) return 0;
	x_=(px/65535.0f)*2.0f;
	y_=(py/65535.0f)*2.0f;
	z_=(pz/65535.0f)*2.0f;
	a_=1.0f;
	std::size_t used=pos+12;
	if (remain>=14) {
		uint8_t aa=0;
		if (!read_hex_u8(s,pos+12,aa)) return 0;
		set_a(static_cast<int>(aa));
		used=pos+14;
	}
	return used;
}

inline std::size_t laba::from_string(const std::string& s,bool initialize,bool strict) {
	if (initialize) l_=a_=b_=alpha_=0;
	std::size_t pos=0;
	if (!parse_packed_prefix(s,pos,'L','B')) {
		rgba tmp;
		std::size_t used=tmp.from_string(s,false,strict);
		if (!used) return 0;
		*this=rgba_to_laba(tmp);
		return used;
	}
	std::size_t remain=s.size()-pos;
	if (strict) {
		if (!(remain==12 || remain==14)) return 0;
	} else {
		if (remain<12) return 0;
	}
	uint16_t pl=0,pa=0,pb=0;
	if (!read_hex_u16(s,pos+0,pl)) return 0;
	if (!read_hex_u16(s,pos+4,pa)) return 0;
	if (!read_hex_u16(s,pos+8,pb)) return 0;
	l_=std::clamp(pl/100.0f,0.0f,100.0f);
	a_=static_cast<int16_t>(pa)/100.0f;
	b_=static_cast<int16_t>(pb)/100.0f;
	alpha_=1.0f;
	std::size_t used=pos+12;
	if (remain>=14) {
		uint8_t aa=0;
		if (!read_hex_u8(s,pos+12,aa)) return 0;
		set_alpha(static_cast<int>(aa));
		used=pos+14;
	}
	return used;
}

inline std::size_t lcha::from_string(const std::string& s,bool initialize,bool strict) {
	if (initialize) l_=c_=h_=alpha_=0;
	std::size_t pos=0;
	if (!parse_packed_prefix(s,pos,'L','C'))  {
		rgba tmp;
		std::size_t used=tmp.from_string(s,false,strict);
		if (!used) return 0;
		*this=rgba_to_lcha(tmp);
		return used;
	}
	std::size_t remain=s.size()-pos;
	if (strict) {
		if (!(remain==12 || remain==14)) return 0;
	} else {
		if (remain<12) return 0;
	}
	uint16_t pl=0,pc=0,ph=0;
	if (!read_hex_u16(s,pos+0,pl)) return 0;
	if (!read_hex_u16(s,pos+4,pc)) return 0;
	if (!read_hex_u16(s,pos+8,ph)) return 0;
	l_=std::clamp(pl/100.0f,0.0f,100.0f);
	c_=std::clamp(pc/100.0f,0.0f,200.0f);
	h_=std::clamp(ph/100.0f,0.0f,360.0f);
	alpha_=1.0f;
	std::size_t used=pos+12;
	if (remain>=14) {
		uint8_t aa=0;
		if (!read_hex_u8(s,pos+12,aa)) return 0;
		set_alpha(static_cast<int>(aa));
		used=pos+14;
	}
	return used;
}

inline std::size_t oklaba::from_string(const std::string& s,bool initialize,bool strict) {
	if (initialize) l_=a_=b_=alpha_=0;
	std::size_t pos=0;
	if (!parse_packed_prefix(s,pos,'O','B'))  {
		rgba tmp;
		std::size_t used=tmp.from_string(s,false,strict);
		if (!used) return 0;
		*this=rgba_to_oklaba(tmp);
		return used;
	}
	std::size_t remain=s.size()-pos;
	if (strict) {
		if (!(remain==12 || remain==14)) return 0;
	} else { if (remain<12) return 0; }
	uint16_t pl=0,pa=0,pb=0;
	if (!read_hex_u16(s,pos+0,pl)) return 0;
	if (!read_hex_u16(s,pos+4,pa)) return 0;
	if (!read_hex_u16(s,pos+8,pb)) return 0;
	l_=pl/65535.0f;
	a_=std::clamp(static_cast<int16_t>(pa)/65535.0f,-0.5f,0.5f);
	b_=std::clamp(static_cast<int16_t>(pb)/65535.0f,-0.5f,0.5f);
	alpha_=1.0f;
	std::size_t used=pos+12;
	if (remain>=14) {
		uint8_t aa=0;
		if (!read_hex_u8(s,pos+12,aa)) return 0;
		set_alpha(static_cast<int>(aa));
		used=pos+14;
	}
	return used;
}

inline std::size_t oklcha::from_string(const std::string& s,bool initialize,bool strict) {
	if (initialize) l_=c_=h_=alpha_=0;
	std::size_t pos=0;
	if (!parse_packed_prefix(s,pos,'O','C'))  {
		rgba tmp;
		std::size_t used=tmp.from_string(s,false,strict);
		if (!used) return 0;
		*this=rgba_to_oklcha(tmp);
		return used;
	}
	std::size_t remain=s.size()-pos;
	if (strict) {
		if (!(remain==12 || remain==14)) return 0;
	} else {
		if (remain<12) return 0;
	}
	uint16_t pl=0,pc=0,ph=0;
	if (!read_hex_u16(s,pos+0,pl)) return 0;
	if (!read_hex_u16(s,pos+4,pc)) return 0;
	if (!read_hex_u16(s,pos+8,ph)) return 0;
	l_=pl/65535.0f;
	c_=std::clamp(pc/65535.0f,0.0f,0.5f);
	h_=std::clamp(ph/100.0f,0.0f,360.0f);
	alpha_=1.0f;
	std::size_t used=pos+12;
	if (remain>=14) {
		uint8_t aa=0;
		if (!read_hex_u8(s,pos+12,aa)) return 0;
		set_alpha(static_cast<int>(aa));
		used=pos+14;
	}
	return used;
}

inline std::size_t hwba::from_string(const std::string& s,bool initialize,bool strict) {
	if (initialize) h_=w_=b_=a_=0;
	std::size_t pos=0;
	if (!parse_packed_prefix(s,pos,'H','W')) {
		rgba tmp;
		std::size_t used=tmp.from_string(s,false,strict);
		if (!used) return 0;
		*this=rgba_to_hwba(tmp);
		return used;
	}
	std::size_t remain=s.size()-pos;
	if (strict) {
		if (!(remain==12 || remain==14)) return 0;
	} else {
		if (remain<12) return 0;
	}
	uint16_t ph=0,pw=0,pb=0;
	if (!read_hex_u16(s,pos+0,ph)) return 0;
	if (!read_hex_u16(s,pos+4,pw)) return 0;
	if (!read_hex_u16(s,pos+8,pb)) return 0;
	h_=std::clamp(ph/100.0f,0.0f,360.0f);
	w_=std::clamp(pw/100.0f,0.0f,100.0f);
	b_=std::clamp(pb/100.0f,0.0f,100.0f);
	a_=1.0f;
	std::size_t used=pos+12;
	if (remain>=14) {
		uint8_t aa=0;
		if (!read_hex_u8(s,pos+12,aa)) return 0;
		set_a(static_cast<int>(aa));
		used=pos+14;
	}
	return used;
}

inline rgba with_alpha(rgba c,float a) {
	c.set_a(a);
	return c;
}

inline rgba invert_rgb(rgba c) {
	return rgba(255-clamp_u8(c.r_),255-clamp_u8(c.g_),255-clamp_u8(c.b_),std::clamp(c.a_,0.0f,1.0f));
}

inline rgba adjust_brightness(rgba c,float amount) {
	hsva h=rgba_to_hsva(c);
	h.v_=std::clamp(h.v_+amount*100.0f,0.0f,100.0f);
	h.a_=c.a_;
	return hsva_to_rgba(h);
}

inline rgba adjust_saturation(rgba c,float factor) {
	factor=std::max(0.0f,factor);
	hsla h=rgba_to_hsla(c);
	h.s_=std::clamp(h.s_*factor,0.0f,100.0f);
	h.a_=c.a_;
	return hsla_to_rgba(h);
}

inline rgba rgba_from_string(const std::string& s,bool strict=true,bool* success=nullptr) {
	if (success) *success=false;
	rgba result;
	if (result.from_string(s,true,strict)) {
		if (success) *success=true;
		return result;
	}
	std::size_t pos=0;
	if (parse_packed_prefix(s,pos,'H','S')) {
		hsla c;
		if (c.from_string(s,true,strict)) {
			if (success) *success=true;
			return hsla_to_rgba(c);
		}
	}
	if (parse_packed_prefix(s,pos,'H','V')) {
		hsva c;
		if (c.from_string(s,true,strict)) {
			if (success) *success=true;
			return hsva_to_rgba(c);
		}
	}
	if (parse_packed_prefix(s,pos,'C','K')) {
		cmyka c;
		if (c.from_string(s,true,strict)) {
			if (success) *success=true;
			return cmyka_to_rgba(c);
		}
	}
	if (parse_packed_prefix(s,pos,'Y','U')) {
		yuva c;
		if (c.from_string(s,true,strict)) {
			if (success) *success=true;
			return yuva_to_rgba(c);
		}
	}
	if (parse_packed_prefix(s,pos,'X','Z')) {
		xyza c;
		if (c.from_string(s,true,strict)) {
			if (success) *success=true;
			return xyza_to_rgba(c,rgb_profile());
		}
	}
	if (parse_packed_prefix(s,pos,'L','B')) {
		laba c;
		if (c.from_string(s,true,strict)) {
			if (success) *success=true;
			return laba_to_rgba(c,rgb_profile(),wp_d65());
		}
	}
	if (parse_packed_prefix(s,pos,'L','C')) {
		lcha c;
		if (c.from_string(s,true,strict)) {
			if (success) *success=true;
			return lcha_to_rgba(c,rgb_profile(),wp_d65());
		}
	}
	if (parse_packed_prefix(s,pos,'O','B')) {
		oklaba c;
		if (c.from_string(s,true,strict)) {
			if (success) *success=true;
			return oklaba_to_rgba(c);
		}
	}
	if (parse_packed_prefix(s,pos,'O','C')) {
		oklcha c;
		if (c.from_string(s,true,strict)) {
			if (success) *success=true;
			return oklcha_to_rgba(c);
		}
	}
	if (parse_packed_prefix(s,pos,'H','W')) {
		hwba c;
		if (c.from_string(s,true,strict)) {
			if (success) *success=true;
			return hwba_to_rgba(c);
		}
	}
	return rgba(0,0,0,0.0f);
}
	
}

}

}

#endif