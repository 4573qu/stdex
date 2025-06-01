//Last Modified At 2025/04/17
//@Version 1.0
//@H_Version 1.0
#include "color.h"

std::string std::vision::color::rgba::to_rgb_string() {
	std::stringstream ss;
	ss<<"#"<<std::hex
		<<std::setw(2)<<std::setfill('0')<<r_
		<<std::setw(2)<<std::setfill('0')<<g_
		<<std::setw(2)<<std::setfill('0')<<b_;
	return ss.str();
}
std::string std::vision::color::rgba::to_rgba_string() {
	std::string result=to_rgb_string();
	std::stringstream ss;
	ss<<result<<std::hex<<std::setw(2)<<std::setfill('0')<<static_cast<int>(a_*255);
	return ss.str();
}

std::string std::vision::color::hsla::to_hsl_string() {
	return hsla_to_rgba(*this).to_rgb_string();
}

std::string std::vision::color::hsla::to_hsla_string() {
	return hsla_to_rgba(*this).to_rgba_string();
}

std::string std::vision::color::hsva::to_hsv_string() {
	return hsva_to_rgba(*this).to_rgb_string();
}

std::string std::vision::color::hsva::to_hsva_string() {
	return hsva_to_rgba(*this).to_rgba_string();
}