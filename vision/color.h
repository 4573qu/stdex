//Last Modified At 2025/04/17
//@Version 1.0
#ifndef _STD4573_VISION_COLOR_H_
#define _STD4573_VISION_COLOR_H_ 1

#include <algorithm>
#include <iomanip>
#include <sstream>

//HOW ABOUT MIXED MODE?

namespace std {
	
namespace vision {

namespace color {

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
	std::string to_rgb_string();
	std::string to_rgba_string();
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
	std::string to_hsl_string();
	std::string to_hsla_string();
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
	std::string to_hsv_string();
	std::string to_hsva_string();
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
};

//struct cmyka { float c,m,y,k,a;	};

//struct yuva { float y,u,v,a; };

hsla rgba_to_hsla(rgba color) {
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

rgba hsla_to_rgba(hsla color) {
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

hsva rgba_to_hsva(rgba color) {
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

rgba hsva_to_rgba(hsva color) {
	float h=color.h_/360.0f;
	float s=color.s_/100.0f;
	float v=color.v_/100.0f;
	int i=static_cast<int>(h*6);
	float f=h*6-i;
	float p=v*(1-s);
	float q=v*(1-f*s);
	float t=v*(1-(1-f)*s);
	float r,g,b;
	switch (i % 6) {
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
	
}

}

}

#endif