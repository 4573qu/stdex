//Last Modified At 2026/04/17
//@Version 1.2.0.0
#ifndef _STDEX_VISION_COLOR_H_
#define _STDEX_VISION_COLOR_H_ 1

#include <algorithm>
#include <cerrno>
#include <cmath>
#include <cstddef>
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
	int r,g,b;
	float a;
	rgba() : r(0) , g(0) , b(0) , a(0) { }
	rgba(int red,int green,int blue) : rgba(red,blue,green,0.0f) { }
	rgba(int red,int green,int blue,float alpha) {
		set_red(red);
		set_green(green);
		set_blue(blue);
		set_alpha(alpha);
	}
	rgba(int red,int green,int blue,int alpha) : rgba(red,blue,green) {
		set_alpha(alpha);
	}
	int& red() { return r; }
	int& green() { return g; }
	int& blue() { return b; }
	float& alpha() { return a; }
	std::string to_rgb_string() {
		std::stringstream ss;
		ss<<"#"<<std::hex<<std::setw(2)<<std::setfill('0')<<r<<std::setw(2)<<std::setfill('0')<<g<<std::setw(2)<<std::setfill('0')<<b;
		return ss.str();
	}
	std::string to_rgba_string() {
		return add_alpha_string(to_rgb_string(),a);
	}
	std::size_t from_string(const std::string& s,bool initialize=false,bool strict=true) {
		if (initialize) {
			r=g=b=0;
			a=0;
		}
		int red=0,green=0,blue=0,alpha=255;
		std::size_t used=parse_hex_color_to_rgba(red,green,blue,alpha,s,strict);
		if (!used) return 0;
		set_rgb(red,green,blue);
		set_alpha(alpha);
		return used;
	}
	void set_red(int red) { r=std::clamp(red,0,255); }
	void set_r(int red) { set_red(red); }
	void set_green(int green) { g=std::clamp(green,0,255); }
	void set_g(int green) { set_green(green); }
	void set_blue(int blue) { b=std::clamp(blue,0,255); }
	void set_b(int blue) { set_blue(blue); }
	void set_rgb(int red,int green,int blue) {
		set_red(red);
		set_green(green);
		set_blue(blue);
	}
	void set_alpha(float alpha) { a=std::clamp(alpha,0.0f,1.0f); }
	void set_alpha(int alpha) {
		float real_a=alpha/255.0f;
		set_alpha(real_a);
	}
	bool operator ==(const rgba& other) const {
		return r==other.r && g==other.g && b==other.b && a==other.a;
	}
	bool operator !=(const rgba& other) const {
		return !(*this==other);
	}
};

struct hsla {
	float h,s,l,a;
	hsla() : h(0) , s(0) , l(0) , a(0) { }
	hsla(float hue,float saturation,float lightness) : hsla(hue,saturation,lightness,0.0f) { }
	hsla(float hue,float saturation,float lightness,float alpha) {
		set_hue(hue);
		set_saturation(saturation);
		set_lightness(lightness);
		set_alpha(alpha);
	}
	hsla(float hue,float saturation,float lightness,int alpha) : hsla(hue,saturation,lightness) {
		set_alpha(alpha);
	}
	float& hue() { return h; }
	float& saturation() { return s; }
	float& lightness() { return l; }
	float& alpha() { return a; }
	std::string to_hsl_string() {
		uint16_t ph=static_cast<uint16_t>(std::lround(std::clamp(h,0.0f,360.0f)*100.0f));
		uint16_t ps=static_cast<uint16_t>(std::lround(std::clamp(s,0.0f,100.0f)*100.0f));
		uint16_t pl=static_cast<uint16_t>(std::lround(std::clamp(l,0.0f,100.0f)*100.0f));
		std::stringstream ss;
		ss<<"#HS"<<std::hex<<std::setw(4)<<std::setfill('0')<<ph<<std::setw(4)<<std::setfill('0')<<ps<<std::setw(4)<<std::setfill('0')<<pl;
		return ss.str();
	}
	std::string to_hsla_string() {
		return add_alpha_string(to_hsl_string(),a);
	}
	std::size_t from_string(const std::string& s,bool initialize=false,bool strict=true);
	void set_hue(float hue) { h=std::clamp(hue,0.0f,360.0f); }
	void set_h(float hue) { set_hue(hue); }
	void set_saturation(float saturation) { s=std::clamp(saturation,0.0f,100.0f); }
	void set_s(float saturation) { set_saturation(saturation); }
	void set_lightness(float lightness) { l=std::clamp(lightness,0.0f,100.0f); }
	void set_l(float lightness) { set_lightness(lightness); }
	void set_hsl(float hue,float saturation,float lightness) {
		set_hue(hue);
		set_saturation(saturation);
		set_lightness(lightness);
	}
	void set_alpha(float alpha) { a=std::clamp(alpha,0.0f,1.0f); }
	void set_alpha(int alpha) {
		float real_a=alpha/255.0f;
		set_alpha(real_a);
	}
	bool operator ==(const hsla& other) const {
		return h==other.h && s==other.s && l==other.l && a==other.a;
	}
	bool operator !=(const hsla& other) const {
		return !(*this==other);
	}
};

struct hsva {
	float h,s,v,a;
	hsva() : h(0) , s(0) , v(0) , a(0) { }
	hsva(float hue,float saturation,float value) : hsva(hue,saturation,value,0.0f) { }
	hsva(float hue,float saturation,float value,float alpha) {
		set_hue(hue);
		set_saturation(saturation);
		set_value(value);
		set_alpha(alpha);
	}
	hsva(float hue,float saturation,float value,int alpha) : hsva(hue,saturation,value) {
		set_alpha(alpha);
	}
	float& hue() { return h; }
	float& saturation() { return s; }
	float& value() { return v; }
	float& alpha() { return a; }
	std::string to_hsv_string() {
		uint16_t ph=static_cast<uint16_t>(std::lround(std::clamp(h,0.0f,360.0f)*100.0f));
		uint16_t ps=static_cast<uint16_t>(std::lround(std::clamp(s,0.0f,100.0f)*100.0f));
		uint16_t pv=static_cast<uint16_t>(std::lround(std::clamp(v,0.0f,100.0f)*100.0f));
		std::stringstream ss;
		ss<<"#HV"<<std::hex<<std::setw(4)<<std::setfill('0')<<ph<<std::setw(4)<<std::setfill('0')<<ps<<std::setw(4)<<std::setfill('0')<<pv;
		return ss.str();
	}
	std::string to_hsva_string() {
		return add_alpha_string(to_hsv_string(),a);
	}
	std::size_t from_string(const std::string& s,bool initialize=false,bool strict=true);
	void set_hue(float hue) { h=std::clamp(hue,0.0f,360.0f); }
	void set_h(float hue) { set_hue(hue); }
	void set_saturation(float saturation) { s=std::clamp(saturation,0.0f,100.0f); }
	void set_s(float saturation) { set_saturation(saturation); }
	void set_value(float value) { v=std::clamp(value,0.0f,100.0f); }
	void set_v(float value) { set_value(value); }
	void set_hsv(float hue,float saturation,float value) {
		set_hue(hue);
		set_saturation(saturation);
		set_value(value);
	}
	void set_alpha(float alpha) { a=std::clamp(alpha,0.0f,1.0f); }
	void set_alpha(int alpha) {
		float real_a=alpha/255.0f;
		set_alpha(real_a);
	}
	bool operator ==(const hsva& other) const {
		return h==other.h && s==other.s && v==other.v && a==other.a;
	}
	bool operator !=(const hsva& other) const {
		return !(*this==other);
	}
};

struct cmyka {
	float c,m,y,k,a;
	cmyka() : c(0) , m(0) , y(0) , k(0) , a(0) { }
	cmyka(float cyan,float magenta,float yellow,float key) : cmyka(cyan,magenta,yellow,key,0.0f) { }
	cmyka(float cyan,float magenta,float yellow,float key,float alpha) {
		set_cyan(cyan);
		set_magenta(magenta);
		set_yellow(yellow);
		set_key(key);
		set_alpha(alpha);
	}
	cmyka(float cyan,float magenta,float yellow,float key,int alpha) : cmyka(cyan,magenta,yellow,key) {
		set_alpha(alpha);
	}
	float& cyan() { return c; }
	float& magenta() { return m; }
	float& yellow() { return y; }
	float& key() { return k; }
	float& alpha() { return a; }
	std::string to_cmyk_string() {
		uint16_t pc=static_cast<uint16_t>(std::lround(std::clamp(c,0.0f,100.0f)*100.0f));
		uint16_t pm=static_cast<uint16_t>(std::lround(std::clamp(m,0.0f,100.0f)*100.0f));
		uint16_t py=static_cast<uint16_t>(std::lround(std::clamp(y,0.0f,100.0f)*100.0f));
		uint16_t pk=static_cast<uint16_t>(std::lround(std::clamp(k,0.0f,100.0f)*100.0f));
		std::stringstream ss;
		ss<<"#CK"<<std::hex<<std::setw(4)<<std::setfill('0')<<pc<<std::setw(4)<<std::setfill('0')<<pm<<std::setw(4)<<std::setfill('0')<<py<<std::setw(4)<<std::setfill('0')<<pk;
		return ss.str();
	}
	std::string to_cmyka_string() {
		return add_alpha_string(to_cmyk_string(),a);
	}
	std::size_t from_string(const std::string& s,bool initialize=false,bool strict=true);
	void set_cyan(float cyan) { c=std::clamp(cyan,0.0f,100.0f); }
	void set_c(float cyan) { set_cyan(cyan); }
	void set_magenta(float magenta) { m=std::clamp(magenta,0.0f,100.0f); }
	void set_m(float magenta) { set_magenta(magenta); }
	void set_yellow(float yellow) { y=std::clamp(yellow,0.0f,100.0f); }
	void set_y(float y) { set_yellow(y); }
	void set_key(float key) { k=std::clamp(key,0.0f,100.0f); }
	void set_k(float key) { set_key(key); }
	void set_cmyk(float cyan,float magenta,float yellow,float key) {
		set_cyan(cyan);
		set_magenta(magenta);
		set_yellow(yellow);
		set_key(key);
	}
	void set_alpha(float alpha) { a=std::clamp(alpha,0.0f,1.0f); }
	void set_alpha(int alpha) {
		float real_a=alpha/255.0f;
		set_alpha(real_a);
	}
	bool operator ==(const cmyka& other) const {
		return c==other.c && m==other.m && y==other.y && k==other.k && a==other.a;
	}
	bool operator !=(const cmyka& other) const {
		return !(*this==other);
	}
};

//BT.709
struct yuva {
	float y,u,v,a;
	yuva() : y(0) , u(0) , v(0) , a(0) { }
	yuva(float luma,float cb,float cr) : yuva(luma,cb,cr,0.0f) { }
	yuva(float luma,float cb,float cr,float alpha) {
		set_luma(luma);
		set_cb(cb);
		set_cr(cr);
		set_alpha(alpha);
	}
	yuva(float luma,float cb,float cr,int alpha) : yuva(luma,cb,cr) {
		set_alpha(alpha);
	}
	float& luma() { return y; }
	float& cb() { return u; }
	float& u() { return u; }
	float& cr() { return v; }
	float& v() { return v; }
	float& alpha() { return a; }
	std::string to_yuv_string() {
		uint16_t py=static_cast<uint16_t>(std::lround(std::clamp(y,0.0f,1.0f)*65535.0f));
		int16_t iu=static_cast<int16_t>(std::lround(std::clamp(u,-0.5f,0.5f)*65535.0f));
		int16_t iv=static_cast<int16_t>(std::lround(std::clamp(v,-0.5f,0.5f)*65535.0f));
		std::stringstream ss;
		ss<<"#YU"<<std::hex<<std::setw(4)<<std::setfill('0')<<py<<std::setw(4)<<std::setfill('0')<<static_cast<uint16_t>(iu)<<std::setw(4)<<std::setfill('0')<<static_cast<uint16_t>(iv);
		return ss.str();
	}
	std::string to_yuva_string() {
		return add_alpha_string(to_yuv_string(),a);
	}
	std::size_t from_string(const std::string& s,bool initialize=false,bool strict=true);
	void set_luma(float luma) { y=std::clamp(luma,0.0f,1.0f); }
	void set_y(float y) { set_luma(y); }
	void set_cb(float cb) { u=std::clamp(cb,-0.5f,0.5f); }
	void set_u(float u) { set_cb(u); }
	void set_cr(float cr) { v=std::clamp(cr,-0.5f,0.5f); }
	void set_v(float v) { set_cr(v); }
	void set_yuv(float y,float u,float v) {
		set_y(y);
		set_u(u);
		set_v(v);
	}
	void set_ycbcr(float luma,float cb,float cr) {
		set_luma(luma);
		set_cb(cb);
		set_cr(cr);
	}
	void set_alpha(float alpha) { a=std::clamp(alpha,0.0f,1.0f); }
	void set_alpha(int alpha) {
		float real_a=alpha/255.0f;
		set_alpha(real_a);
	}
	bool operator ==(const yuva& other) const {
		return y==other.y && u==other.u && v==other.v && a==other.a;
	}
	bool operator !=(const yuva& other) const {
		return !(*this==other);
	}
};

struct xyza {
	float x,y,z,a;
	xyza() : x(0) , y(0) , z(0) , a(0) { }
	xyza(float tx,float ty,float tz) : xyza(tx,ty,tz,0.0f) { }
	xyza(float tx,float ty,float tz,float alpha) {
		set_x(tx);
		set_y(ty);
		set_z(tz);
		set_alpha(alpha);
	}
	xyza(float tx,float ty,float tz,int alpha) : xyza(tx,ty,tz) {
		set_alpha(alpha);
	}
	float& x() { return x; }
	float& tx() { return x; }
	float& y() { return y; }
	float& ty() { return y; }
	float& luma() { return y; }
	float& z() { return z; }
	float& tz() { return z; }
	float& alpha() { return a; }
	std::string to_xyz_string() {
		auto pack_u16_0_2=[](float v)->uint16_t{
			v=std::clamp(v,0.0f,2.0f);
			return static_cast<uint16_t>(std::lround((v/2.0f)*65535.0f));
		};
		uint16_t px=pack_u16_0_2(x);
		uint16_t py=pack_u16_0_2(y);
		uint16_t pz=pack_u16_0_2(z);
		std::stringstream ss;
		ss<<"#XZ"<<std::hex<<std::setw(4)<<std::setfill('0')<<px<<std::setw(4)<<std::setfill('0')<<py<<std::setw(4)<<std::setfill('0')<<pz;
		return ss.str();
	}
	std::string to_xyza_string() {
		return add_alpha_string(to_xyz_string(),a);
	}
	std::size_t from_string(const std::string& s,bool initialize=false,bool strict=true);
	void set_x(float tx) { x=std::max(0.0f,tx); }
	void set_y(float ty) { y=std::max(0.0f,ty); }
	void set_luma(float luma) { set_y(luma); }
	void set_z(float tz) { z=std::max(0.0f,tz); }
	void set_xyz(float tx,float ty,float tz) {
		set_x(tx);
		set_y(ty);
		set_z(tz);
	}
	void set_alpha(float alpha) { a=std::clamp(alpha,0.0f,1.0f); }
	void set_alpha(int alpha) {
		float real_a=alpha/255.0f;
		set_alpha(real_a);
	}
	bool operator ==(const xyza& other) const {
		return x==other.x && y==other.y && z==other.z && a==other.a;
	}
	bool operator !=(const xyza& other) const {
		return !(*this==other);
	}
};

//CIELAB
struct laba {
	float l,as,bs,a;
	laba() : l(0) , as(0) , bs(0) , a(0) { }
	laba(float lightness,float a_star,float b_star) : laba(lightness,a_star,b_star,0.0f) { }
	laba(float lightness,float a_star,float b_star,float alpha) {
		set_lightness(lightness);
		set_a(a_star);
		set_b(b_star);
		set_alpha(alpha);
	}
	laba(float lightness,float a_star,float b_star,int alpha) : laba(lightness,a_star,b_star) {
		set_alpha(alpha);
	}
	float& lightness() { return l; }
	float& l_star() { return l; }
	float& a() { return as; }
	float& green_red() { return as; }
	float& a_star() { return as;}
	float& b() { return bs; }
	float& blue_yellow() { return bs; }
	float& b_star() { return bs; }
	float& alpha() { return a; }
	std::string to_lab_string() {
		uint16_t pl=static_cast<uint16_t>(std::lround(std::clamp(l,0.0f,100.0f)*100.0f));
		int16_t pa=static_cast<int16_t>(std::lround(std::clamp(as,-128.0f,127.0f)*100.0f));
		int16_t pb=static_cast<int16_t>(std::lround(std::clamp(bs,-128.0f,127.0f)*100.0f));
		std::stringstream ss;
		ss<<"#LB"<<std::hex<<std::setw(4)<<std::setfill('0')<<pl<<std::setw(4)<<std::setfill('0')<<static_cast<uint16_t>(pa)<<std::setw(4)<<std::setfill('0')<<static_cast<uint16_t>(pb);
		return ss.str();
	}
	std::string to_laba_string() {
		return add_alpha_string(to_lab_string(),a);
	}
	std::size_t from_string(const std::string& s,bool initialize=false,bool strict=true);
	void set_lightness(float lightness) { l=std::clamp(lightness,0.0f,100.0f); }
	void set_l(float lightness) { set_lightness(lightness); }
	void set_l_star(float l_star) { set_lightness(l_star); }
	void set_a(float a_star) { as=a_star; }
	void set_green_red(float green_red) { set_a(green_red); }
	void set_a_star(float a_star) { set_a(a_star); }
	void set_b(float b_star) { bs=b_star; }
	void set_blue_yellow(float blue_yellow) { set_b(blue_yellow); }
	void set_b_star(float b_star) { set_b(b_star); }
	void set_lab(float lightness,float a_star,float b_star) {
		set_lightness(l);
		set_a(a_star);
		set_b(b_star);
	}
	void set_alpha(float alpha) { a=std::clamp(alpha,0.0f,1.0f); }
	void set_alpha(int alpha) {
		float real_a=alpha/255.0f;
		set_alpha(real_a);
	}
	bool operator ==(const laba& other) const {
		return l==other.l && as==other.as && bs==other.bs && a==other.a;
	}
	bool operator !=(const laba& other) const {
		return !(*this==other);
	}
};

struct lcha {
	float l,c,h,a;
	lcha() : l(0) , c(0) , h(0) , a(0) { }
	lcha(float lightness,float chroma,float hue) : lcha(lightness,chroma,hue,0.0f) { }
	lcha(float lightness,float chroma,float hue,float alpha) {
		set_lightness(lightness);
		set_chroma(chroma);
		set_hue(hue);
		set_alpha(alpha);
	}
	lcha(float lightness,float chroma,float hue,int alpha) : lcha(lightness,chroma,hue) {
		set_alpha(alpha);
	}
	float& lightness() { return l; }
	float& chroma() { return c; }
	float& hue() { return h; }
	float& alpha() { return a; }
	std::string to_lch_string() {
		uint16_t pl=static_cast<uint16_t>(std::lround(std::clamp(l,0.0f,100.0f)*100.0f));
		uint16_t pc=static_cast<uint16_t>(std::lround(std::clamp(c,0.0f,200.0f)*100.0f)); // cap 200
		uint16_t ph=static_cast<uint16_t>(std::lround(std::clamp(h,0.0f,360.0f)*100.0f));
		std::stringstream ss;
		ss<<"#LC"<<std::hex<<std::setw(4)<<std::setfill('0')<<pl<<std::setw(4)<<std::setfill('0')<<pc<<std::setw(4)<<std::setfill('0')<<ph;
		return ss.str();
	}
	std::string to_lcha_string() {
		return add_alpha_string(to_lch_string(),a);
	}
	std::size_t from_string(const std::string& s,bool initialize=false,bool strict=true);
	void set_lightness(float lightness) { l=std::clamp(lightness,0.0f,100.0f); }
	void set_l(float lightness) { set_lightness(lightness); }
	void set_chroma(float chroma) { c=std::max(0.0f,chroma); }
	void set_c(float chroma) { set_chroma(chroma); }
	void set_hue(float hue) { h=std::clamp(hue,0.0f,360.0f); }
	void set_h(float hue) { set_hue(hue); }
	void set_lch(float lightness,float chroma,float hue) {
		set_lightness(lightness);
		set_chroma(chroma);
		set_hue(hue);
	}
	void set_alpha(float alpha) { a=std::clamp(alpha,0.0f,1.0f); }
	void set_alpha(int alpha) {
		float real_a=alpha/255.0f;
		set_alpha(real_a);
	}
	bool operator ==(const lcha& other) const {
		return l==other.l && c==other.c && h==other.h && a==other.a;
	}
	bool operator !=(const lcha& other) const {
		return !(*this==other);
	}
};

struct oklaba {
	float lp,arg,ayb,a;
	oklaba() : lp(0) , arg(0) , ayb(0) , a(0) { }
	oklaba(float lightness,float a,float b) : oklaba(lightness,a,b,0.0f) { }
	oklaba(float lightness,float a,float b,float alpha) {
		set_lightness(lightness);
		set_a(a);
		set_b(b);
		set_alpha(alpha);
	}
	oklaba(float lightness,float a,float b,int alpha) : oklaba(lightness,a,b) {
		set_alpha(alpha);
	}
	float& lightness() { return lp; }
	float& lightness_perceptual() { return lp; }
	float& a() { return arg; }
	float& axis_red_green() { return arg; }
	float& b() { return ayb; }
	float& axis_yellow_blue() { return ayb; }
	float& alpha() { return a; }
	std::string to_oklab_string() {
		uint16_t pl=static_cast<uint16_t>(std::lround(std::clamp(lp,0.0f,1.0f)*65535.0f));
		int16_t pa=static_cast<int16_t>(std::lround(std::clamp(arg,-0.5f,0.5f)*65535.0f));
		int16_t pb=static_cast<int16_t>(std::lround(std::clamp(ayb,-0.5f,0.5f)*65535.0f));
		std::stringstream ss;
		ss<<"#OB"<<std::hex<<std::setw(4)<<std::setfill('0')<<pl<<std::setw(4)<<std::setfill('0')<<static_cast<uint16_t>(pa)<<std::setw(4)<<std::setfill('0')<<static_cast<uint16_t>(pb);
		return ss.str();
	}
	std::string to_oklaba_string() {
		return add_alpha_string(to_oklab_string(),a);
	}
	std::size_t from_string(const std::string& s,bool initialize=false,bool strict=true);
	void set_lightness(float lightness) { lp=std::clamp(lightness,0.0f,1.0f); }
	void set_l(float lightness) { set_lightness(lightness); }
	void set_a(float a) { arg=a; }
	void set_axis_red_green(float a) { set_a(a); }
	void set_b(float b) { ayb=b; }
	void set_axis_yellow_blue(float b) { set_b(b); } 
	void set_oklab(float lightness,float a,float b) {
		set_lightness(lightness);
		set_a(a);
		set_b(b);
	}
	void set_alpha(float alpha) { a=std::clamp(alpha,0.0f,1.0f); }
	void set_alpha(int alpha) {
		float real_a=alpha/255.0f;
		set_alpha(real_a);
	}
	bool operator ==(const oklaba& other) const {
		return lp==other.lp && arg==other.arg && ayb==other.ayb && a==other.a;
	}
	bool operator !=(const oklaba& other) const {
		return !(*this==other);
	}
};

struct oklcha {
	float l,c,h,a;
	oklcha() : l(0) , c(0) , h(0) , a(0) { }
	oklcha(float lightness,float chroma,float hue) : oklcha(lightness,chroma,hue,0.0f) { }
	oklcha(float lightness,float chroma,float hue,float alpha) {
		set_lightness(lightness);
		set_chroma(chroma);
		set_hue(hue);
		set_alpha(alpha);
	}
	oklcha(float lightness,float chroma,float hue,int alpha) : oklcha(lightness,chroma,hue) {
		set_alpha(alpha);
	}
	float& lightness() { return l; }
	float& chroma() { return c; }
	float& hue() { return h; }
	float& alpha() { return a; }
	std::string to_oklch_string() {
		uint16_t pl=static_cast<uint16_t>(std::lround(std::clamp(l,0.0f,1.0f)*65535.0f));
		uint16_t pc=static_cast<uint16_t>(std::lround(std::clamp(c,0.0f,0.5f)*65535.0f));
		uint16_t ph=static_cast<uint16_t>(std::lround(std::clamp(h,0.0f,360.0f)*100.0f));
		std::stringstream ss;
		ss<<"#OC"<<std::hex<<std::setw(4)<<std::setfill('0')<<pl<<std::setw(4)<<std::setfill('0')<<pc<<std::setw(4)<<std::setfill('0')<<ph;
		return ss.str();
	}
	std::string to_oklcha_string() {
		return add_alpha_string(to_oklch_string(),a);
	}
	std::size_t from_string(const std::string& s,bool initialize=false,bool strict=true);
	void set_lightness(float lightness) { l=std::clamp(lightness,0.0f,1.0f); }
	void set_l(float lightness) { set_lightness(lightness); }
	void set_chroma(float chroma) { c=std::max(0.0f,chroma); }
	void set_c(float chroma) { set_chroma(chroma); }
	void set_hue(float hue) { h=std::clamp(hue,0.0f,360.0f); }
	void set_h(float hue) { set_hue(hue); }
	void set_oklch(float lightness,float chroma,float hue) {
		set_lightness(lightness);
		set_chroma(chroma);
		set_hue(hue);
	}
	void set_alpha(float alpha) { a=std::clamp(alpha,0.0f,1.0f); }
	void set_alpha(int alpha) {
		float real_a=alpha/255.0f;
		set_alpha(real_a);
	}
	bool operator ==(const oklcha& other) const {
		return l==other.l && c==other.c && h==other.h && a==other.a;
	}
	bool operator !=(const oklcha& other) const {
		return !(*this==other);
	}
};

struct hwba {
	float h,w,b,a;
	hwba() : h(0) , w(0) , b(0) , a(0) { }
	hwba(float hue,float whiteness,float blackness) : hwba(hue,whiteness,blackness,0.0f) { }
	hwba(float hue,float whiteness,float blackness,float alpha) {
		set_hue(hue);
		set_whiteness(whiteness);
		set_blackness(blackness);
		set_alpha(alpha);
	}
	hwba(float hue,float whiteness,float blackness,int alpha) : hwba(hue,whiteness,blackness) {
		set_alpha(alpha);
	}
	float& hue() { return h; }
	float& whiteness() { return w; }
	float& blackness() { return b; }
	float& alpha() { return a; }
	std::string to_hwb_string() {
		uint16_t ph=static_cast<uint16_t>(std::lround(std::clamp(h,0.0f,360.0f)*100.0f));
		uint16_t pw=static_cast<uint16_t>(std::lround(std::clamp(w,0.0f,100.0f)*100.0f));
		uint16_t pb=static_cast<uint16_t>(std::lround(std::clamp(b,0.0f,100.0f)*100.0f));
		std::stringstream ss;
		ss<<"#HW"<<std::hex<<std::setw(4)<<std::setfill('0')<<ph<<std::setw(4)<<std::setfill('0')<<pw<<std::setw(4)<<std::setfill('0')<<pb;
		return ss.str();
	}
	std::string to_hwba_string() {
		return add_alpha_string(to_hwb_string(),a);
	}
	std::size_t from_string(const std::string& s,bool initialize=false,bool strict=true);
	void set_hue(float hue) { h=std::clamp(hue,0.0f,360.0f); }
	void set_h(float hue) { set_hue(hue); }
	void set_whiteness(float whiteness) { w=std::clamp(whiteness,0.0f,100.0f); }
	void set_w(float whiteness) { set_whiteness(whiteness); }
	void set_blackness(float blackness) { b=std::clamp(blackness,0.0f,100.0f); }
	void set_b(float blackness) { set_blackness(blackness); }
	void set_hwb(float hue,float whiteness,float blackness) {
		set_hue(hue);
		set_whiteness(whiteness);
		set_blackness(blackness);
	}
	void set_alpha(float alpha) { a=std::clamp(alpha,0.0f,1.0f); }
	void set_alpha(int alpha) {
		float real_a=alpha/255.0f;
		set_alpha(real_a);
	}
	bool operator ==(const hwba& other) const {
		return h==other.h && w==other.w && b==other.b && a==other.a;
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
	float r=std::clamp(color.red()/255.0f,0.0f,1.0f);
	float g=std::clamp(color.green()/255.0f,0.0f,1.0f);
	float b=std::clamp(color.blue()/255.0f,0.0f,1.0f);
	float max_val=std::max({r,g,b});
	float min_val=std::min({r,g,b});
	float delta=max_val-min_val;
	hsla result;
	result.lightness()=(max_val+min_val)/2.0f;
	if (delta<1e-5) {
		result.set_hue(0);
		result.set_saturation(0);
	} else  {
		result.set_saturation((result.lightness()>0.5f)?(delta/(2.0f-max_val-min_val)):(delta/(max_val+min_val)));
		if (max_val==r) result.hue()=(g-b)/delta+((g<b)?6.0f:0.0f);
		else if (max_val==g) result.hue()=(b-r)/delta+2.0f;
		else result.hue()=(r-g)/delta+4.0f;
		result.hue()*=60.0f;
		if (result.hue()<0) result.hue()+=360.0f;
	}
	result.saturation()*=100.0f;
	result.lightness()*=100.0f;
	result.set_alpha(color.alpha());
	return result;
}

inline rgba hsla_to_rgba(hsla color) {
	float h=color.hue()/360.0f;
	float s=color.saturation()/100.0f;
	float l=color.lightness()/100.0f;
	if (s<1e-5) {
		int val=static_cast<int>(l*255.0f);
		return rgba(val,val,val,color.alpha());
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
	return rgba(static_cast<int>(r*255),static_cast<int>(g*255),static_cast<int>(b*255),color.alpha());
}

inline hsva rgba_to_hsva(rgba color) {
	float r=std::clamp(color.red()/255.0f,0.0f,1.0f);
	float g=std::clamp(color.green()/255.0f,0.0f,1.0f);
	float b=std::clamp(color.blue()/255.0f,0.0f,1.0f);
	float max_val=std::max({r,g,b});
	float min_val=std::min({r,g,b});
	float delta=max_val-min_val;
	hsva result;
	result.v=max_val*100.0f;
	if (delta<1e-5) {
		result.set_hue(0);
		result.set_saturation(0);
	} else {
		result.set_saturation((delta/max_val)*100.0f);
		if (max_val==r) result.hue()=(g-b)/delta+((g<b)?6.0f:0.0f);
		else if (max_val==g) result.hue()=(b-r)/delta+2.0f;
		else result.hue()=(r-g)/delta+4.0f;
		result.hue()*=60.0f;
		if (result.hue()<0) result.hue()+=360.0f;
	}
	result.set_alpha(color.alpha());
	return result;
}

inline rgba hsva_to_rgba(hsva color) {
	float h=color.hue()/360.0f;
	float s=color.saturation()/100.0f;
	float v=color.value()/100.0f;
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
	return rgba(static_cast<int>(r*255),static_cast<int>(g*255),static_cast<int>(b*255),color.alpha());
}

hsva hsla_to_hsva(hsla color) {
	return rgba_to_hsva(hsla_to_rgba(color));
}

hsla hsva_to_hsla(hsva color) {
	return rgba_to_hsla(hsva_to_rgba(color));
}

inline cmyka rgba_to_cmyka(rgba color) {
	float r=std::clamp(color.red()/255.0f,0.0f,1.0f);
	float g=std::clamp(color.green()/255.0f,0.0f,1.0f);
	float b=std::clamp(color.blue()/255.0f,0.0f,1.0f);
	float k=1.0f-std::max({r,g,b});
	float c=0,m=0,y=0;
	if (k<0.999999f) {
		c=(1.0f-r-k)/(1.0f-k);
		m=(1.0f-g-k)/(1.0f-k);
		y=(1.0f-b-k)/(1.0f-k);
	}
	return cmyka(std::clamp(c*100.0f,0.0f,100.0f),std::clamp(m*100.0f,0.0f,100.0f),std::clamp(y*100.0f,0.0f,100.0f),std::clamp(k*100.0f,0.0f,100.0f),std::clamp(color.a,0.0f,1.0f));
}
inline rgba cmyka_to_rgba(cmyka color) {
	float c=std::clamp(color.cyan(),0.0f,100.0f)/100.0f;
	float m=std::clamp(color.magenta(),0.0f,100.0f)/100.0f;
	float y=std::clamp(color.yellow(),0.0f,100.0f)/100.0f;
	float k=std::clamp(color.key(),0.0f,100.0f)/100.0f;
	float r=(1.0f-c)*(1.0f-k);
	float g=(1.0f-m)*(1.0f-k);
	float b=(1.0f-y)*(1.0f-k);
	return rgba(round_u8_from01(r),round_u8_from01(g),round_u8_from01(b),color.alpha());
}

inline yuva rgba_to_yuva(rgba color) {
	float r=std::clamp(color.red()/255.0f,0.0f,1.0f);
	float g=std::clamp(color.green()/255.0f,0.0f,1.0f);
	float b=std::clamp(color.blue()/255.0f,0.0f,1.0f);
	float y=0.299f*r+0.587f*g+0.114f*b;
	float u=0.492f*(b-y);
	float v=0.877f*(r-y);
	return yuva(std::clamp(y,0.0f,1.0f),std::clamp(u,-0.5f,0.5f),std::clamp(v,-0.5f,0.5f),color.alpha());
}

inline rgba yuva_to_rgba(yuva color) {
	float y=std::clamp(color.luma(),0.0f,1.0f);
	float u=std::clamp(color.cb(),-0.5f,0.5f);
	float v=std::clamp(color.cr(),-0.5f,0.5f);
	float r=y+v/0.877f;
	float b=y+u/0.492f;
	float g=(y-0.299f*r-0.114f*b)/0.587f;
	return rgba(round_u8_from01(r),round_u8_from01(g),round_u8_from01(b),color.alpha());
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
	rgb_working_space space;
	rgb_transfer transfer;
	rgb_profile() : space(RWS_SRGB),transfer(RT_SRGB) { }
	rgb_profile(rgb_working_space s,rgb_transfer t) : space(s),transfer(t) { }
};

struct white_point {
	float x,y,z;
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

inline xyza rgba_to_xyza(rgba color,rgb_profile profile=rgb_profile()) {
	float r=to_linear(profile.transfer,std::clamp(color.red()/255.0f,0.0f,1.0f));
	float g=to_linear(profile.transfer,std::clamp(color.green()/255.0f,0.0f,1.0f));
	float b=to_linear(profile.transfer,std::clamp(color.blue()/255.0f,0.0f,1.0f));
	float m00,m01,m02,m10,m11,m12,m20,m21,m22;
	rgb_to_xyz_matrix(profile.space,m00,m01,m02,m10,m11,m12,m20,m21,m22);
	xyza result(m00*r+m01*g+m02*b,m10*r+m11*g+m12*b,m20*r+m21*g+m22*b,color.alpha());
	return result;
}

inline rgba xyza_to_rgba(xyza color,rgb_profile profile=rgb_profile()) {
	float m00,m01,m02,m10,m11,m12,m20,m21,m22;
	xyz_to_rgb_matrix(profile.space,m00,m01,m02,m10,m11,m12,m20,m21,m22);
	float r=m00*color.x()+m01*color.y()+m02*color.z();
	float g=m10*color.x()+m11*color.y()+m12*color.z();
	float b=m20*color.x()+m21*color.y()+m22*color.z();
	r=to_nonlinear(profile.transfer,r);
	g=to_nonlinear(profile.transfer,g);
	b=to_nonlinear(profile.transfer,b);
	return rgba(round_u8_from01(r),round_u8_from01(g),round_u8_from01(b),color.alpha());
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

inline laba xyza_to_laba(xyza color,white_point wp=wp_d65()) {
	float x=color.x()/wp.x;
	float y=color.y()/wp.y;
	float z=color.z()/wp.z;
	float fx=lab_f(x);
	float fy=lab_f(y);
	float fz=lab_f(z);
	return laba(std::max(0.0f,116.0f*fy-16.0f),500.0f*(fx-fy),200.0f*(fy-fz),color.alpha());
}

inline xyza laba_to_xyza(laba color,white_point wp=wp_d65()) {
	float fy=(color.lightness()+16.0f)/116.0f;
	float fx=fy+color.a()/500.0f;
	float fz=fy-color.b()/200.0f;
	return xyza(wp.x*lab_f_inv(fx),wp.y*lab_f_inv(fy),wp.z*lab_f_inv(fz),color.alpha());
}

inline laba rgba_to_laba(rgba color,rgb_profile profile=rgb_profile(),white_point wp=wp_d65()) {
	return xyza_to_laba(rgba_to_xyza(color,profile),wp);
}

inline rgba laba_to_rgba(laba color,rgb_profile profile=rgb_profile(),white_point wp=wp_d65()) {
	return xyza_to_rgba(laba_to_xyza(color,wp),profile);
}

inline lcha laba_to_lcha(laba color) {
	float C=std::sqrt(color.a()*color.a()+color.b()*color.b());
	float H=std::atan2(color.b(),color.a())*180.0f/3.14159265358979323846f;
	if (H<0) H+=360.0f;
	return lcha(color.lightness(),C,H,color.alpha());
}

inline laba lcha_to_laba(lcha color) {
	float hr=color.hue()*3.14159265358979323846f/180.0f;
	float a=color.chroma()*std::cos(hr);
	float b=color.chroma()*std::sin(hr);
	return laba(color.lightness(),a,b,color.alpha());
}

inline lcha rgba_to_lcha(rgba color,rgb_profile profile=rgb_profile(),white_point wp=wp_d65()) {
	return laba_to_lcha(rgba_to_laba(color,profile,wp));
}

inline rgba lcha_to_rgba(lcha color,rgb_profile profile=rgb_profile(),white_point wp=wp_d65()) {
	return laba_to_rgba(lcha_to_laba(color),profile,wp);
}

inline oklaba rgba_to_oklaba(rgba color) {
	float r=srgb_to_linear(std::clamp(color.red()/255.0f,0.0f,1.0f));
	float g=srgb_to_linear(std::clamp(color.green()/255.0f,0.0f,1.0f));
	float b=srgb_to_linear(std::clamp(color.blue()/255.0f,0.0f,1.0f));
	float l=0.4122214708f*r+0.5363325363f*g+0.0514459929f*b;
	float m=0.2119034982f*r+0.6806995451f*g+0.1073969566f*b;
	float s=0.0883024619f*r+0.2817188376f*g+0.6299787005f*b;
	float l3=std::cbrt(l);
	float m3=std::cbrt(m);
	float s3=std::cbrt(s);
	return oklaba(0.2104542553f*l3+0.7936177850f*m3-0.0040720468f*s3,1.9779984951f*l3-2.4285922050f*m3+0.4505937099f*s3,0.0259040371f*l3+0.7827717662f*m3-0.8086757660f*s3,color.alpha());
}

inline rgba oklaba_to_rgba(oklaba color) {
	float l_=color.lightness()+0.3963377774f*color.a()+0.2158037573f*color.b();
	float m_=color.lightness()-0.1055613458f*color.a()-0.0638541728f*color.b();
	float s_=color.lightness()-0.0894841775f*color.a()-1.2914855480f*color.b();
	float l=l_*l_*l_;
	float m=m_*m_*m_;
	float s=s_*s_*s_;
	float r=4.0767416621f*l-3.3077115913f*m+0.2309699292f*s;
	float g=-1.2684380046f*l+2.6097574011f*m-0.3413193965f*s;
	float b=-0.0041960863f*l-0.7034186147f*m+1.7076147010f*s;
	r=linear_to_srgb(r);
	g=linear_to_srgb(g);
	b=linear_to_srgb(b);
	return rgba(round_u8_from01(r),round_u8_from01(g),round_u8_from01(b),color.alpha());
}

inline oklcha oklaba_to_oklcha(oklaba color) {
	float C=std::sqrt(color.a()*color.a()+color.b()*color.b());
	float H=std::atan2(color.b(),color.a())*180.0f/3.14159265358979323846f;
	if (H<0) H+=360.0f;
	return oklcha(color.lightness(),C,H,color.alpha());
}

inline oklaba oklcha_to_oklaba(oklcha color) {
	float hr=color.hue()*3.14159265358979323846f/180.0f;
	float a=color.chroma()*std::cos(hr);
	float b=color.chroma()*std::sin(hr);
	return oklaba(color.lightness(),a,b,color.alpha());
}

inline oklcha rgba_to_oklcha(rgba color) {
	return oklaba_to_oklcha(rgba_to_oklaba(color));
}

inline rgba oklcha_to_rgba(oklcha color) {
	return oklaba_to_rgba(oklcha_to_oklaba(color));
}

inline hsva hwba_to_hsva(hwba color) {
	hsva result;
	result.set_hue(color.hue());
	float w=std::clamp(color.whiteness(),0.0f,100.0f)/100.0f;
	float b=std::clamp(color.blackness(),0.0f,100.0f)/100.0f;
	float v=1.0f-b;
	float s=(v<=1e-6f)?0.0f:1.0f-(w/v);
	result.saturation()=std::clamp(s*100.0f,0.0f,100.0f);
	result.value()=std::clamp(v*100.0f,0.0f,100.0f);
	result.set_alpha(color.alpha());
	return result;
}

inline hwba hsva_to_hwba(hsva color) {
	hwba result;
	result.set_hue(color.hue());
	float s=std::clamp(color.saturation(),0.0f,100.0f)/100.0f;
	float v=std::clamp(color.value(),0.0f,100.0f)/100.0f;
	float w=(1.0f-s)*v;
	float b=1.0f-v;
	result.whiteness()=std::clamp(w*100.0f,0.0f,100.0f);
	result.blackness()=std::clamp(b*100.0f,0.0f,100.0f);
	result.set_alpha(color.alpha());
	return result;
}

inline rgba hwba_to_rgba(hwba color) {
	return hsva_to_rgba(hwba_to_hsva(color));
}

inline hwba rgba_to_hwba(rgba color) {
	return hsva_to_hwba(rgba_to_hsva(color));
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
	lcha result(a.lightness()+(b.lightness()-a.lightness())*t,a.chroma()+(b.chroma()-a.chroma())*t,lerp_hue_shortest(a.hue(),b.hue(),t),a.alpha()+(b.alpha()-a.alpha())*t);
}

inline oklcha lerp_oklcha(oklcha a,oklcha b,float t) {
	t=std::clamp(t,0.0f,1.0f);
	oklcha result(a.lightness()+(b.lightness()-a.lightness())*t,a.chroma()+(b.chroma()-a.chroma())*t,lerp_hue_shortest(a.hue(),b.hue(),t),a.alpha()+(b.alpha()-a.alpha())*t);
}

inline float delta_e76(laba a,laba b) {
	float dl=a.lightness()-b.lightness();
	float da=a.a()-b.a();
	float db=a.b()-b.b();
	return std::sqrt(dl*dl+da*da+db*db);
}
inline float delta_e_ok(oklaba a,oklaba b) {
	float dl=a.lightness()-b.lightness();
	float da=a.a()-b.a();
	float db=a.b()-b.b();
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
	return rgba(round_u8_from01(r),round_u8_from01(g),round_u8_from01(b),alpha);
}

inline float relative_luminance(rgba color) {
	float r=srgb_to_linear(std::clamp(color.red()/255.0f,0.0f,1.0f));
	float g=srgb_to_linear(std::clamp(color.green()/255.0f,0.0f,1.0f));
	float b=srgb_to_linear(std::clamp(color.blue()/255.0f,0.0f,1.0f));
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
	float sa=std::clamp(src.alpha(),0.0f,1.0f);
	float da=std::clamp(dst.alpha(),0.0f,1.0f);
	float out_a=sa+da*(1.0f-sa);
	if (out_a<1e-6f) return rgba(0,0,0,0.0f);
	float sr=std::clamp(src.red()/255.0f,0.0f,1.0f);
	float sg=std::clamp(src.green()/255.0f,0.0f,1.0f);
	float sb=std::clamp(src.blue()/255.0f,0.0f,1.0f);
	float dr=std::clamp(dst.red()/255.0f,0.0f,1.0f);
	float dg=std::clamp(dst.green()/255.0f,0.0f,1.0f);
	float db=std::clamp(dst.blue()/255.0f,0.0f,1.0f);
	float out_r=(sr*sa+dr*da*(1.0f-sa))/out_a;
	float out_g=(sg*sa+dg*da*(1.0f-sa))/out_a;
	float out_b=(sb*sa+db*da*(1.0f-sa))/out_a;
	return rgba(round_u8_from01(out_r),round_u8_from01(out_g),round_u8_from01(out_b),out_a);
}


inline rgba lerp_rgba(rgba a,rgba b,float t) {
	t=std::clamp(t,0.0f,1.0f);
	int r=static_cast<int>(std::lround(a.red()+(b.red()-a.red())*t));
	int g=static_cast<int>(std::lround(a.green()+(b.green()-a.green())*t));
	int bl=static_cast<int>(std::lround(a.blue()+(b.blue()-a.blue())*t));
	float alpha=a.alpha()+(b.alpha()-a.alpha())*t;
	return rgba(clamp_u8(r),clamp_u8(g),clamp_u8(bl),alpha);
}

inline rgba lerp_rgba_linear(rgba a,rgba b,float t) {
	t=std::clamp(t,0.0f,1.0f);
	float ar=srgb_to_linear(std::clamp(a.red()/255.0f,0.0f,1.0f));
	float ag=srgb_to_linear(std::clamp(a.green()/255.0f,0.0f,1.0f));
	float ab=srgb_to_linear(std::clamp(a.blue()/255.0f,0.0f,1.0f));
	float br=srgb_to_linear(std::clamp(b.red()/255.0f,0.0f,1.0f));
	float bg=srgb_to_linear(std::clamp(b.green()/255.0f,0.0f,1.0f));
	float bb=srgb_to_linear(std::clamp(b.blue()/255.0f,0.0f,1.0f));
	float rr=ar+(br-ar)*t;
	float rg=ag+(bg-ag)*t;
	float rb=ab+(bb-ab)*t;
	int r=round_u8_from01(linear_to_srgb(rr));
	int g=round_u8_from01(linear_to_srgb(rg));
	int bl=round_u8_from01(linear_to_srgb(rb));
	float alpha=a.alpha()+(b.alpha()-a.alpha())*t;
	return rgba(r,g,bl,alpha);
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

inline float blend_channel(blend_mode mode,float backdrop,float source) {
	float b=std::clamp(backdrop,0.0f,1.0f);
	float s=std::clamp(source,0.0f,1.0f);
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
	float br=std::clamp(base.red()/255.0f,0.0f,1.0f);
	float bg=std::clamp(base.green()/255.0f,0.0f,1.0f);
	float bb=std::clamp(base.blue()/255.0f,0.0f,1.0f);
	float sr=std::clamp(blend.red()/255.0f,0.0f,1.0f);
	float sg=std::clamp(blend.green()/255.0f,0.0f,1.0f);
	float sb=std::clamp(blend.blue()/255.0f,0.0f,1.0f);
	float rr=blend_channel(mode,br,sr);
	float rg=blend_channel(mode,bg,sg);
	float rb=blend_channel(mode,bb,sb);
	return rgba(round_u8_from01(rr),round_u8_from01(rg),round_u8_from01(rb),blend.alpha());
}

inline rgba blend_and_composite(rgba base,rgba blend,blend_mode mode) {
	rgba mixed=blend_rgb(base,blend,mode);
	return composite_over(mixed,base);
}

inline std::size_t hsla::from_string(const std::string& s,bool initialize,bool strict) {
	if (initialize) h=this->s=l=a=0;
	std::size_t pos=0;
	if (!parse_packed_prefix(s,pos,'H','S')) {
		rgba temp;
		std::size_t used=temp.from_string(s,initialize,strict);
		if (!used) return 0;
		*this=rgba_to_hsla(temp);
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
	set_hue(ph/100.0f);
	set_saturation(ps/100.0f);
	set_lightness(pl/100.0f);
	set_alpha(1.0f);
	std::size_t used=pos+12;
	if (remain>=14) {
		uint8_t aa=0;
		if (!read_hex_u8(s,pos+12,aa)) return 0;
		set_alpha(static_cast<int>(aa));
		used=pos+14;
	}
	return used;
}

inline std::size_t hsva::from_string(const std::string& s,bool initialize,bool strict) {
	if (initialize) h=this->s=v=a=0;
	std::size_t pos=0;
	if (!parse_packed_prefix(s,pos,'H','V')) {
		rgba temp;
		std::size_t used=temp.from_string(s,initialize,strict);
		if (!used) return 0;
		*this=rgba_to_hsva(temp);
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
	set_hue(ph/100.0f);
	set_saturation(ps/100.0f);
	set_value(pv/100.0f);
	set_alpha(1.0f);
	std::size_t used=pos+12;
	if (remain>=14) {
		uint8_t aa=0;
		if (!read_hex_u8(s,pos+12,aa)) return 0;
		set_alpha(static_cast<int>(aa));
		used=pos+14;
	}
	return used;
}

inline std::size_t cmyka::from_string(const std::string& s,bool initialize,bool strict) {
	if (initialize) c=m=y=k=a=0;
	std::size_t pos=0;
	if (!parse_packed_prefix(s,pos,'C','K')) {
		rgba temp;
		std::size_t used=temp.from_string(s,initialize,strict);
		if (!used) return 0;
		*this=rgba_to_cmyka(temp);
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
	set_cyan(pc/100.0f);
	set_magenta(pm/100.0f);
	set_yellow(py/100.0f);
	set_key(pk/100.0f);
	set_alpha(1.0f);
	std::size_t used=pos+16;
	if (remain>=18) {
		uint8_t aa=0;
		if (!read_hex_u8(s,pos+16,aa)) return 0;
		set_alpha(static_cast<int>(aa));
		used=pos+18;
	}
	return used;
}

inline std::size_t yuva::from_string(const std::string& s,bool initialize,bool strict) {
	if (initialize) y=u()=v()=a=0;
	std::size_t pos=0;
	if (!parse_packed_prefix(s,pos,'Y','U')) {
		rgba temp;
		std::size_t used=temp.from_string(s,initialize,strict);
		if (!used) return 0;
		*this=rgba_to_yuva(temp);
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
	set_luma(py/65535.0f);
	int16_t iu=static_cast<int16_t>(pu);
	int16_t iv=static_cast<int16_t>(pv);
	set_cb(iu/65535.0f);
	set_cr(iv/65535.0f);
	set_alpha(1.0f);
	std::size_t used=pos+12;
	if (remain>=14) {
		uint8_t aa=0;
		if (!read_hex_u8(s,pos+12,aa)) return 0;
		set_alpha(static_cast<int>(aa));
		used=pos+14;
	}
	return used;
}

inline std::size_t xyza::from_string(const std::string& s,bool initialize,bool strict) {
	if (initialize) x()=y()=z()=a=0;
	std::size_t pos=0;
	if (!parse_packed_prefix(s,pos,'X','Z')) {
		rgba temp;
		std::size_t used=temp.from_string(s,initialize,strict);
		if (!used) return 0;
		*this=rgba_to_xyza(temp);
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
	set_x((px/65535.0f)*2.0f);
	set_y((py/65535.0f)*2.0f);
	set_z((pz/65535.0f)*2.0f);
	set_alpha(1.0f);
	std::size_t used=pos+12;
	if (remain>=14) {
		uint8_t aa=0;
		if (!read_hex_u8(s,pos+12,aa)) return 0;
		set_alpha(static_cast<int>(aa));
		used=pos+14;
	}
	return used;
}

inline std::size_t laba::from_string(const std::string& s,bool initialize,bool strict) {
	if (initialize) l=as=bs=a()=0;
	std::size_t pos=0;
	if (!parse_packed_prefix(s,pos,'L','B')) {
		rgba temp;
		std::size_t used=temp.from_string(s,initialize,strict);
		if (!used) return 0;
		*this=rgba_to_laba(temp);
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
	set_lightness(pl/100.0f);
	set_a(static_cast<int16_t>(pa)/100.0f);
	set_b(static_cast<int16_t>(pb)/100.0f);
	set_alpha(1.0f);
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
	if (initialize) l=c=h=a=0;
	std::size_t pos=0;
	if (!parse_packed_prefix(s,pos,'L','C'))  {
		rgba temp;
		std::size_t used=temp.from_string(s,initialize,strict);
		if (!used) return 0;
		*this=rgba_to_lcha(temp);
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
	set_lightness(pl/100.0f);
	set_chroma(pc/100.0f);
	set_hue(ph/100.0f);
	set_alpha(1.0f);
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
	if (initialize) lp=arg=ayb=a()=0;
	std::size_t pos=0;
	if (!parse_packed_prefix(s,pos,'O','B'))  {
		rgba temp;
		std::size_t used=temp.from_string(s,false,strict);
		if (!used) return 0;
		*this=rgba_to_oklaba(temp);
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
	set_lightness(pl/65535.0f);
	set_a(static_cast<int16_t>(pa)/65535.0f);
	set_b(static_cast<int16_t>(pb)/65535.0f);
	set_alpha(1.0f);
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
	if (initialize) l=c=h=a=0;
	std::size_t pos=0;
	if (!parse_packed_prefix(s,pos,'O','C'))  {
		rgba temp;
		std::size_t used=temp.from_string(s,false,strict);
		if (!used) return 0;
		*this=rgba_to_oklcha(temp);
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
	set_lightness(pl/65535.0f);
	set_chroma(pc/65535.0f);
	set_hue(ph/100.0f);
	set_alpha(1.0f);
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
	if (initialize) h=w=b=a=0;
	std::size_t pos=0;
	if (!parse_packed_prefix(s,pos,'H','W')) {
		rgba temp;
		std::size_t used=temp.from_string(s,false,strict);
		if (!used) return 0;
		*this=rgba_to_hwba(temp);
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
	set_hue(ph/100.0f);
	set_whiteness(pw/100.0f);
	set_blackness(pb/100.0f);
	set_alpha(1.0f);
	std::size_t used=pos+12;
	if (remain>=14) {
		uint8_t aa=0;
		if (!read_hex_u8(s,pos+12,aa)) return 0;
		set_alpha(static_cast<int>(aa));
		used=pos+14;
	}
	return used;
}

inline rgba with_alpha(rgba color,float a) {
	color.set_alpha(a);
	return color;
}

inline rgba invert_rgb(rgba color) {
	return rgba(255-clamp_u8(color.red()),255-clamp_u8(color.green()),255-clamp_u8(color.blue()),color.alpha());
}

inline rgba adjust_brightness(rgba color,float amount) {
	hsva h=rgba_to_hsva(color);
	h.set_value(h.value()+amount*100.0f);
	return hsva_to_rgba(h);
}

inline rgba adjust_saturation(rgba color,float factor) {
	factor=std::max(0.0f,factor);
	hsla h=rgba_to_hsla(color);
	h.set_saturation(h.saturation()*factor);
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
		hsla color;
		if (color.from_string(s,true,strict)) {
			if (success) *success=true;
			return hsla_to_rgba(color);
		}
	}
	if (parse_packed_prefix(s,pos,'H','V')) {
		hsva color;
		if (color.from_string(s,true,strict)) {
			if (success) *success=true;
			return hsva_to_rgba(color);
		}
	}
	if (parse_packed_prefix(s,pos,'C','K')) {
		cmyka color;
		if (color.from_string(s,true,strict)) {
			if (success) *success=true;
			return cmyka_to_rgba(color);
		}
	}
	if (parse_packed_prefix(s,pos,'Y','U')) {
		yuva color;
		if (color.from_string(s,true,strict)) {
			if (success) *success=true;
			return yuva_to_rgba(color);
		}
	}
	if (parse_packed_prefix(s,pos,'X','Z')) {
		xyza color;
		if (color.from_string(s,true,strict)) {
			if (success) *success=true;
			return xyza_to_rgba(color,rgb_profile());
		}
	}
	if (parse_packed_prefix(s,pos,'L','B')) {
		laba color;
		if (color.from_string(s,true,strict)) {
			if (success) *success=true;
			return laba_to_rgba(color,rgb_profile(),wp_d65());
		}
	}
	if (parse_packed_prefix(s,pos,'L','C')) {
		lcha color;
		if (color.from_string(s,true,strict)) {
			if (success) *success=true;
			return lcha_to_rgba(color,rgb_profile(),wp_d65());
		}
	}
	if (parse_packed_prefix(s,pos,'O','B')) {
		oklaba color;
		if (color.from_string(s,true,strict)) {
			if (success) *success=true;
			return oklaba_to_rgba(color);
		}
	}
	if (parse_packed_prefix(s,pos,'O','C')) {
		oklcha color;
		if (color.from_string(s,true,strict)) {
			if (success) *success=true;
			return oklcha_to_rgba(color);
		}
	}
	if (parse_packed_prefix(s,pos,'H','W')) {
		hwba color;
		if (color.from_string(s,true,strict)) {
			if (success) *success=true;
			return hwba_to_rgba(color);
		}
	}
	return rgba();
}
	
}

}

}

#endif