/*
	This file is a C++ adaptation of break_eternity.js by Patashu.
	Original project:
	https://github.com/Patashu/break_eternity.js
	License: MIT
	A copy of the license is provided in: /third_party_licenses/break_eternity.js_MIT.txt

	Notes:
	- Naming and coding style were modified to match this library.
	- Some APIs were reshaped to C++ style.
	- Most of comments were removed while the remainder were retained.
	- The section concerning game application was commented out.
*/
//Last Modified At 2026/02/02
//@Version 1.0.0.0
#ifndef _STDEX_MATH_decimal_H_
#define _STDEX_MATH_decimal_H_ 1

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <iomanip>
#include <limits>
#include <memory>
#include <sstream>
#include <string>
#include <type_traits>

#include "../structure/lru_cache.h"//At Least 1.0

namespace stdex {

namespace math {

class decimal {
public:
	static constexpr int default_from_string_cache_size=(1<<10)-1;

private:
	int sign_;
	double layer_;
	double mag_;
	static inline const double critical_tetr_values_[10][11]={
		{1,1.0891180521811202527,1.1789767925673958433,1.2701455431742086633,1.3632090180450091941,1.4587818160364217007,1.5575237916251418333,1.6601571006859253673,1.7674858188369780435,1.8804192098842727359,2},
		{1,1.1121114330934078681,1.2310389249316089299,1.3583836963111376089,1.4960519303993531879,1.6463542337511945810,1.8121385357018724464,1.9969713246183068478,2.2053895545527544330,2.4432574483385252544,M_E},
		{1,1.1187738849693603,   1.2464963939368214,   1.38527004705667,     1.5376664685821402,   1.7068895236551784,   1.897001227148399,    2.1132403089001035,   2.362480153784171,    2.6539010333870774,   3},
		{1,1.1367350847096405,   1.2889510672956703,   1.4606478703324786,   1.6570295196661111,   1.8850062585672889,   2.1539465047453485,   2.476829779693097,    2.872061932789197,    3.3664204535587183,   4},
		{1,1.1494592900767588,   1.319708228183931,    1.5166291280087583,   1.748171114438024,    2.0253263297298045,   2.3636668498288547,   2.7858359149579424,   3.3257226212448145,   4.035730287722532,    5},
		{1,1.159225940787673,    1.343712473580932,    1.5611293155111927,   1.8221199554561318,   2.14183924486326,     2.542468319282638,    3.0574682501653316,   3.7390572020926873,   4.6719550537360774,   6},
		{1,1.1670905356972596,   1.3632807444991446,   1.5979222279405536,   1.8842640123816674,   2.2416069644878687,   2.69893426559423,     3.3012632110403577,   4.121250340630164,    5.281493033448316,    7},
		{1,1.1736630594087796,   1.379783782386201,    1.6292821855668218,   1.9378971836180754,   2.3289975651071977,   2.8384347394720835,   3.5232708454565906,   4.478242031114584,    5.868592169644505,    8},
		{1,1.1793017514670474,   1.394054150657457,    1.65664127441059,     1.985170999970283,    2.4069682290577457,   2.9647310119960752,   3.7278665320924946,   4.814462547283592,    6.436522247411611,    9},
		{1,1.1840100246247336579,1.4061375836156954169,1.6802272208863963918,2.026757028388618927, 2.4770056063449647580,3.0805252717554819987,3.9191964192627283911,5.1351528408331864230,6.9899611795347148455,10},
	};
	static inline const double critical_slog_values_[10][11]={
		{-1,-0.9194161097107025,-0.8335625019330468,-0.7425599821143978,-0.6466611521029437, -0.5462617907227869, -0.4419033816638769, -0.3342645487554494, -0.224140440909962,  -0.11241087890006762, 0},
		{-1,-0.90603157029014, 	-0.80786507256596,  -0.7064666939634,   -0.60294836853664,   -0.49849837513117,   -0.39430303318768,   -0.29147201034755,   -0.19097820800866,   -0.09361896280296,    0},
		{-1,-0.9021579584316141,-0.8005762598234203,-0.6964780623319391,-0.5911906810998454, -0.486050182576545,  -0.3823089430815083, -0.28106046722897615,-0.1831906535795894, -0.08935809204418144, 0},
		{-1,-0.8917227442365535,-0.781258746326964, -0.6705130326902455,-0.5612813129406509, -0.4551067709033134, -0.35319256652135966,-0.2563741554088552, -0.1651412821106526, -0.0796919581982668,  0},
		{-1,-0.8843387974366064,-0.7678744063886243,-0.6529563724510552,-0.5415870994657841, -0.4352842206588936, -0.33504449124791424,-0.24138853420685147,-0.15445285440944467,-0.07409659641336663, 0},
		{-1,-0.8786709358426346,-0.7577735191184886,-0.6399546189952064,-0.527284921869926,  -0.4211627631006314, -0.3223479611761232, -0.23107655627789858,-0.1472057700818259, -0.07035171210706326, 0},
		{-1,-0.8740862815291583,-0.7497032990976209,-0.6297119746181752,-0.5161838335958787, -0.41036238255751956,-0.31277212146489963,-0.2233976621705518, -0.1418697367979619, -0.06762117662323441, 0},
		{-1,-0.8702632331800649,-0.7430366914122081,-0.6213373075161548,-0.5072025698095242, -0.40171437727184167,-0.30517930701410456,-0.21736343968190863,-0.137710238299109,  -0.06550774483471955, 0},
		{-1,-0.8670016295947213,-0.7373984232432306,-0.6143173985094293,-0.49973884395492807,-0.394584953527678,  -0.2989649949848695, -0.21245647317021688,-0.13434688362382652,-0.0638072667348083,  0},
		{-1,-0.8641642839543857,-0.732534623168535, -0.6083127477059322,-0.4934049257184696, -0.3885773075899922, -0.29376029055315767,-0.2083678561173622, -0.13155653399373268,-0.062401588652553186,0},
	};
	static inline structure::lru_cache<std::string,std::shared_ptr<decimal>> from_string_cache_{default_from_string_cache_size};

private:
	static std::string trim(const std::string& str) {
		std::size_t first=str.find_first_not_of(' ');
		if (std::string::npos==first) return "";
		std::size_t last=str.find_last_not_of(' ');
		return str.substr(first,(last-first+1));
	}
	static std::string to_lower(const std::string& str) {
		std::string result=str;
		std::transform(result.begin(),result.end(),result.begin(),[](unsigned char ch){
			return std::tolower(ch);
		});
		return result;
	}
	void from_components_impl(int sign,double layer,double mag) {
		sign_=sign;
		layer_=layer;
		mag_=mag;
		normalize();
	}
	void from_mantissa_exponent_impl(double mantissa,double exponent) {
		layer_=1;
		sign_=(mantissa>0)?1:(mantissa<0)?-1:0;
		mantissa=std::abs(mantissa);
		mag_=exponent+std::log10(mantissa);
		normalize();
	}
	double get_m() const {
		if (sign_==0) return 0.0;
		if (layer_==0) {
			double exp=std::floor(std::log10(mag_));
			double man=(mag_==5e-324)?5.0:mag_/std::pow(10,exp);
			return sign_*man;
		} else if (layer_==1) {
			double residue=mag_-std::floor(mag_);
			return sign_*std::pow(10,residue);
		}
		return sign_;
	}
	double get_e() const {
		if (sign_==0) return 0.0;
		if (layer_==0) return std::floor(std::log10(mag_));
		else if (layer_==1) return std::floor(mag_);
		else if (layer_==2) return std::floor(std::copysign(std::pow(10,std::abs(mag_)),mag_));
		else return mag_*std::numeric_limits<double>::infinity();
	}
	decimal& normalize() {
		if (sign_==0 || (mag_==0 && layer_==0) || (std::isinf(mag_) && mag_<0 && layer_>0 && std::isfinite(layer_))) {
			sign_=0;
			mag_=0;
			layer_=0;
			return *this;
		}
		if (std::isinf(mag_) || std::isinf(layer_)) {
			mag_=std::numeric_limits<double>::infinity();
			layer_=std::numeric_limits<double>::infinity();
			return *this;
		}
		if (layer_==0 && mag_<0) {
			mag_=-mag_;
			sign_=-sign_;
		}
		if (layer_==0 && mag_<first_neg_layer) {
			layer_+=1;
			mag_=std::log10(mag_);
			return *this;
		}
		double abs_mag=std::abs(mag_);
		double sign_mag=std::copysign(1.0,mag_);
		if (abs_mag>=exp_limit) {
			layer_+=1;
			mag_=sign_mag*std::log10(abs_mag);
			return *this;
		}
		while (abs_mag<layer_down && layer_>0) {
			layer_-=1;
			if (layer_==0) mag_=std::pow(10,mag_);
			else {
				mag_=sign_mag*std::pow(10,abs_mag);
				abs_mag=std::abs(mag_);
				sign_mag=std::copysign(1.0,mag_);
			}
		}
		if (layer_==0) {
			if (mag_<0) {
				mag_=-mag_;
				sign_=-sign_;
			} else if (mag_==0) sign_=0;
		}
		if (std::isnan(sign_) || std::isnan(layer_) || std::isnan(mag_)) {
			sign_=0;//std::numeric_limits<double>::quiet_NaN();
			layer_=std::numeric_limits<double>::quiet_NaN();
			mag_=std::numeric_limits<double>::quiet_NaN();
		}
		return *this;
	}
	static double slog_critical(double base,double height) {
		if (base>10) return height-1;
		return critical_section(base,height,critical_slog_values_);
	}
	static double tetrate_critical(double base,double height) {
		return critical_section(base,height,critical_tetr_values_);
	}
	static double critical_section(double base,double height,const double grid[10][11]) {
		height*=10;
		if (height<0) height=0;
		if (height>10) height=10;
		if (base<2) base=2;
		if (base>10) base=10;
		double lower=0;
		double upper=0;
		for (int i=0;i<10;i++) {
			if (std::abs(critical_headers[i]-base)<1e-10) {
				lower=grid[i][static_cast<int>(std::floor(height))];
   				upper=grid[i][static_cast<int>(std::ceil(height))];
   				break;
   			} else if (critical_headers[i]<base && critical_headers[i+1]>base) {
   				double base_frac=(base-critical_headers[i])/(critical_headers[i+1]-critical_headers[i]);
   				lower=grid[i][static_cast<int>(std::floor(height))]*(1-base_frac)+grid[i+1][static_cast<int>(std::floor(height))]*base_frac;
   				upper=grid[i][static_cast<int>(std::ceil(height))]*(1-base_frac)+grid[i+1][static_cast<int>(std::ceil(height))]*base_frac;
   				break;
   			}
   		}
		double frac=height-std::floor(height);
		if (lower<=0 || upper<=0) return lower*(1-frac)+upper*frac;
		else return std::pow(base,std::log(lower)/std::log(base)*(1-frac)+std::log(upper)/std::log(base)*frac);
		return 0.0;//assert(false)
	}
	static std::pair<decimal,int> excess_slog(const decimal& value,const decimal& base,bool linear=false) {
		decimal value_copy=value;
		decimal base_copy=base;
		double base_num=base.to_number();
		if (base_num==1 || base_num<=0) return {d_nan,0};
		if (base_num>1.44466786100976613366) return {value.slog(base,100,linear),0};
		decimal neg_ln=base.ln().neg();
		decimal lower=neg_ln.lambertw()/neg_ln;
		decimal upper=d_inf;
		if (base_num>1) upper=neg_ln.lambertw(false)/neg_ln;
		if (base_num>1.444667861009099) lower=upper=from_number(M_E);
		if (value<lower) return {value.slog(base,100,linear),0};
		if (value==lower) return {d_inf,0};
		if (value==upper) return {d_neg_inf,2};
		if (value>upper) {
			decimal slog_zero=upper*2;
			decimal slog_one=base_copy.pow(slog_zero);
			double estimate=0;
			if (value>=slog_zero && value<slog_one) estimate=0;
			else if (value>=slog_one) {
				decimal payload=slog_one;
				estimate=1;
				while (payload<value) {
					payload=base_copy.pow(payload);
					estimate+=1;
					if (payload.layer()>3) {
						double layers_left=std::floor(value.layer()-payload.layer()+1);
						payload=base_copy.iterated_exp(layers_left,payload,linear);
						estimate=estimate+layers_left;
					}
				}
				if (payload>value) {
					payload=payload.log(base);
					estimate-=1;
				}
			} else if (value<slog_zero) {
				decimal payload=slog_zero;
				estimate=0;
				while (payload>value) {
					payload=payload.log(base);
					estimate-=1;
				}
			}
			double frac_height=0;
			double tested=0;
			double step_size=0.5;
			decimal tower_top=slog_zero;
			decimal guess=d_zero;
			while (step_size>1e-16) {
				tested=frac_height+step_size;
				tower_top=slog_zero.pow(1-tested)*slog_one.pow(tested);
				guess=decimal(base_num).iterated_exp(estimate,tower_top);
				if (guess==value) return {from_number(estimate+tested),2};
				else if (guess<value) frac_height+=step_size;
				step_size/=2;
			}
			if (guess.neq_tolerance(value,1e-7)) return {d_nan,0};
			return {from_number(estimate+frac_height),2};
		}
		if (value<upper && value>lower) {
			decimal slog_zero=(lower*upper).sqrt();
			decimal slog_one=base_copy.pow(slog_zero);
			double estimate=0;
			if (value<=slog_zero && value>slog_one) estimate=0;
			else if (value<=slog_one) {
				decimal payload=slog_one;
				estimate=1;
				while (payload>value) {
					payload=base_copy.pow(payload);
					estimate+=1;
				}
				if (payload<value) {
					payload=payload.log(base);
					estimate-=1;
				}
			} else if (value>slog_zero) {
				decimal payload=slog_zero;
				estimate=0;
				while (payload<value) {
					payload=payload.log(base);
					estimate-=1;
				}
			}
			double frac_height=0;
			double tested = 0;
			double step_size=0.5;
			decimal tower_top=slog_zero;
			decimal guess=d_zero;
			while (step_size>1e-16) {
				tested=frac_height+step_size;
				tower_top=slog_zero.pow(1-tested)*slog_one.pow(tested);
				guess=decimal(base_num).iterated_exp(estimate,tower_top);
				if (guess==value) return {from_number(estimate+tested),1};
				else if (guess>value) frac_height+=step_size;
				step_size/=2;
			}
			if (guess.neq_tolerance(value,1e-7)) return {d_nan,0};
			return {from_number(estimate+frac_height),1};
		}
		throw std::runtime_error("Unhandled behavior in excess_slog");
		return {decimal(0),0};//assert(false)
	}

public:
	static constexpr int max_significant_digits=17;
	static constexpr double exp_limit=9e15;
	static constexpr double layer_down=std::log10(9e15);
	static constexpr double first_neg_layer=1.0/9e15;
	static constexpr int number_exp_max=308;
	static constexpr int number_exp_min=-324;
	static constexpr int max_es_in_a_row=5;
	static constexpr double exp_n1=0.36787944117144232159553;
	static constexpr double omega=0.56714329040978387299997;
	static constexpr double critical_headers[]={2.0,M_E,3.0,4.0,5.0,6.0,7.0,8.0,9.0,10.0};

	decimal() : sign_(0) , layer_(0) , mag_(0) { }
	decimal(double value) {
		*this=from_number(value);
	}
	decimal(const std::string& value) {
		*this=from_string(value);
	}
	decimal(const decimal& other) {
		sign_=other.sign_;
		layer_=other.layer_;
		mag_=other.mag_;
	}
	decimal(decimal&& other) noexcept : sign_(other.sign_) , layer_(other.layer_) , mag_(other.mag_) {
		other.sign_=0;
		other.layer_=0;
		other.mag_=0;
	}
    
	decimal& operator =(const decimal& other) {
		if (this!=&other) {
			sign_=other.sign_;
			layer_=other.layer_;
			mag_=other.mag_;
		}
		return *this;
	}
	decimal& operator =(decimal&& other) noexcept {
		if (this!=&other) {
			sign_=other.sign_;
			layer_=other.layer_;
			mag_=other.mag_;
			other.sign_=0;
			other.layer_=0;
			other.mag_=0;
		}
		return *this;
	}

	static decimal from_components(int sign,double layer,double mag) {
		decimal d;
		d.from_components_impl(sign,layer,mag);
		return d;
	}
	static decimal from_components_no_normalize(int sign,double layer,double mag) {
		decimal d;
		d.sign_=sign;
		d.layer_=layer;
		d.mag_=mag;
		return d;
	}
	static decimal from_mantissa_exponent(double mantissa,double exponent) {
		decimal d;
		d.from_mantissa_exponent_impl(mantissa,exponent);
		return d;
	}
	static decimal from_mantissa_exponent_no_normalize(double mantissa,double exponent) {
		return from_mantissa_exponent(mantissa,exponent);
	}
	static decimal from_decimal(const decimal& other) {
		decimal d;
		d.sign_=other.sign_;
		d.layer_=other.layer_;
		d.mag_=other.mag_;
		return d;
	}
	static decimal from_number(double value) {
		decimal d;
		if (std::isnan(value)) return d_nan;
		d.mag_=std::abs(value); 
		d.sign_=(value>0)?1:(value<0)?-1:0;
		d.layer_=0;
		d.normalize();
		return d;
	}
	static decimal from_string(const std::string& value,bool linear_hyper_4=false) {
		std::string original_value=value;
		auto set_result=[&](const decimal& result) {
			auto shared_result=std::make_shared<decimal>(result);
			from_string_cache_.set(original_value,shared_result);
			return result;
		};
		auto cached=from_string_cache_.get(original_value);
		if (cached.has_value()) return *(*cached);
		std::string processed=value;
		processed.erase(std::remove(processed.begin(),processed.end(),','),processed.end());
		std::string lower=processed;
		std::transform(lower.begin(),lower.end(),lower.begin(),::tolower);
		if (lower=="nan") return set_result(d_nan);
		if (lower=="infinity" || lower=="inf") return set_result(d_inf);
		if (lower=="-infinity" || lower=="-inf") return set_result(d_neg_inf);
		std::size_t pentation_pos=processed.find("^^^");
		if (pentation_pos!=std::string::npos) {
			try {
				double base=std::stod(processed.substr(0,pentation_pos));
				std::string height_part=processed.substr(pentation_pos+3);
				double height=0;
				double payload=1.0;
				std::size_t semicolon_pos=height_part.find(';');
				if (semicolon_pos!=std::string::npos) {
					height=std::stod(height_part.substr(0,semicolon_pos));
					payload=std::stod(height_part.substr(semicolon_pos+1));
					if (!std::isfinite(payload)) payload=1.0;
				} else height=std::stod(height_part);
				if (std::isfinite(base) && std::isfinite(height)) return set_result(decimal(base).pentate(height,decimal(payload),linear_hyper_4));
			} catch (...) {
			}
		}
		std::size_t tetration_pos=processed.find("^^");
		if (tetration_pos!=std::string::npos) {
			try {
				double base=std::stod(processed.substr(0,tetration_pos));
				std::string height_part=processed.substr(tetration_pos+2);
				double height=0;
				double payload=1.0;
				std::size_t semicolon_pos=height_part.find(';');
				if (semicolon_pos!=std::string::npos) {
					height=std::stod(height_part.substr(0,semicolon_pos));
					payload=std::stod(height_part.substr(semicolon_pos+1));
					if (!std::isfinite(payload)) payload=1.0;
				} else height=std::stod(height_part);
				if (std::isfinite(base) && std::isfinite(height)) return set_result(decimal(base).tetrate(height,decimal(payload),linear_hyper_4));
			} catch (...) {
			}
		}
		std::size_t pow_pos=processed.find('^');
		if (pow_pos!=std::string::npos) {
			try {
				double base=std::stod(processed.substr(0,pow_pos));
				double exponent=std::stod(processed.substr(pow_pos+1));
				if (std::isfinite(base) && std::isfinite(exponent)) return set_result(decimal(base).pow(decimal(exponent)));
			} catch (...) {
			}
		}
		processed=trim(processed);
		lower=to_lower(processed);
		std::size_t pt_pos=lower.find("pt");
		if (pt_pos!=std::string::npos) {
			try {
				bool negative=false;
				std::string height_str=processed.substr(0,pt_pos);
				if (!height_str.empty() && height_str[0]=='-') {
					negative=true;
					height_str=height_str.substr(1);
				}
				double height=std::stod(height_str);
				std::string payload_str=processed.substr(pt_pos+2);
				payload_str.erase(std::remove(payload_str.begin(),payload_str.end(),'('),payload_str.end());
				payload_str.erase(std::remove(payload_str.begin(),payload_str.end(),')'),payload_str.end());
				double payload=1.0;
				if (!payload_str.empty()) {
					payload=std::stod(payload_str);
					if (!std::isfinite(payload)) payload=1.0;
				}
				if (std::isfinite(height)) {
					decimal result=decimal(10.0).tetrate(height,decimal(payload),linear_hyper_4);
					if (negative) result=-result;//NEG->-
					return set_result(result);
				}
			} catch (...) {
			}
		}
		std::size_t p_pos=lower.find('p');
		if (p_pos!=std::string::npos && p_pos>0 && p_pos<processed.length()-1) {
			try {
				bool negative=false;
				std::string height_str=processed.substr(0,p_pos);
				if (!height_str.empty() && height_str[0]=='-') {
					negative=true;
					height_str=height_str.substr(1);
				}
				double height=std::stod(height_str);
				std::string payload_str=processed.substr(p_pos+1);
				payload_str.erase(std::remove(payload_str.begin(),payload_str.end(),'('),payload_str.end());
				payload_str.erase(std::remove(payload_str.begin(),payload_str.end(),')'),payload_str.end());
				double payload=1.0;
				if (!payload_str.empty()) {
					payload=std::stod(payload_str);
					if (!std::isfinite(payload)) payload=1.0;
				}
				if (std::isfinite(height)) {
					decimal result=decimal(10.0).tetrate(height,decimal(payload),linear_hyper_4);
					if (negative) result=-result;//NEG->-
					return set_result(result);
				}
			} catch (...) {
			}
		}
		std::size_t f_pos=lower.find('f');
		if (f_pos!=std::string::npos && f_pos>0 && f_pos<processed.length()-1) {
			try {
				bool negative=false;
				std::string payload_str=processed.substr(0,f_pos);
				if (!payload_str.empty() && payload_str[0]=='-') {
					negative=true;
					payload_str=payload_str.substr(1);
				}
				payload_str.erase(std::remove(payload_str.begin(),payload_str.end(),'('),payload_str.end());
				payload_str.erase(std::remove(payload_str.begin(),payload_str.end(),')'),payload_str.end());
				double payload=1.0;
				if (!payload_str.empty()) {
					payload=std::stod(payload_str);
				if (!std::isfinite(payload)) payload=1.0;
				}
				std::string height_str=processed.substr(f_pos+1);
				height_str.erase(std::remove(height_str.begin(),height_str.end(),'('),height_str.end());
				height_str.erase(std::remove(height_str.begin(),height_str.end(),')'),height_str.end());
				double height=std::stod(height_str);
				if (std::isfinite(height)) {
					decimal result=decimal(10.0).tetrate(height,decimal(payload),linear_hyper_4);
					if (negative) result=-result;//NEG->-
					return set_result(result);
				}
			} catch (...) {
			}
		}
		std::vector<std::string> parts;
		std::stringstream ss(processed);
		std::string part;
		int ecount=0;
		std::size_t e_pos=processed.find('e');
		if (e_pos!=std::string::npos) {
			if (e_pos>0 && processed[e_pos-1]=='(') {
				std::size_t caret_pos=processed.find('^',e_pos);
				if (caret_pos!=std::string::npos && caret_pos==e_pos+1) {
					try {
						std::size_t close_paren=processed.find(')',caret_pos);
						if (close_paren!=std::string::npos) {
							std::string layer_str=processed.substr(caret_pos+1,close_paren-caret_pos-1);
							std::string mag_str=processed.substr(close_paren+1);
							int sign=1;
							if (!layer_str.empty() && layer_str[0]=='-') {
								sign=-1;
								layer_str=layer_str.substr(1);
							}            
							double layer=std::stod(layer_str);
							double mag=std::stod(mag_str);
							return set_result(from_components(sign,layer,mag));
						}
					} catch (...) {
					}
				}
			}
			while (std::getline(ss,part,'e')) parts.push_back(part);
			ecount=parts.size()-1;
		} else parts.push_back(processed);
		if (ecount==0) {
			try {
				double num=std::stod(processed);
				return set_result(from_number(num));
			} catch (...) {
				return set_result(d_zero);
			}
		} else if (ecount==1) {
			try {
				double num=std::stod(processed);
				if (std::isfinite(num) && std::abs(num)>1e-307) return set_result(from_number(num));
			} catch (...) {
			}
		}
		if (ecount<1) return set_result(d_zero);
		try {
			double mantissa=std::stod(parts[0]);
			if (mantissa==0) return set_result(d_zero);
			double exponent=std::stod(parts[parts.size()-1]);
			if (ecount>=2) {
				double me=std::stod(parts[parts.size()-2]);
				if (std::isfinite(me)) {
					exponent*=std::copysign(1.0,me);
					exponent+=mag_log10(me);
				}
			}
			if (!std::isfinite(mantissa)) {
				int sign=(parts[0]=="-")?-1:1;
				return set_result(from_components(sign,ecount,exponent));
			} else if (ecount==1) {
				int sign=mantissa>0?1:-1;
				mantissa=std::abs(mantissa);
				return set_result(from_components(sign,1,exponent+std::log10(mantissa)));
			} else {
				int sign=mantissa>0?1:-1;
				mantissa=std::abs(mantissa);
				if (ecount==2) return set_result(from_components(1,2,exponent)*from_number(mantissa));//MUL->*
				else return set_result(from_components(sign,ecount,exponent));
			}
		} catch (...) {
			return set_result(d_zero);
		}
	}
	static decimal from_value(const decimal& value) {
		return from_decimal(value);
	}
	static decimal from_value(double value) {
		return from_number(value);
	}
	static decimal from_value(const std::string& value) {
		return from_string(value); 
	}
	static decimal from_value_no_alloc(const decimal& value) {
		return value;
	}
	static decimal from_value_no_alloc(double value) {
		return from_number(value); }
	static decimal from_value_no_alloc(const std::string& value) {
		return from_string(value);
	}

	int sign() const { return sign_; }
	double layer() const { return layer_; }
	double mag() const { return mag_; }
    
	double m() const {
		if (sign_==0) return 0.0;
		if (layer_==0) {
			double exp=std::floor(std::log10(mag_));
			double man=(mag_==5e-324)?5.0:mag_/std::pow(10,exp);
			return sign_*man;
		} else if (layer_==1) {
			double residue=mag_-std::floor(mag_);
			return sign_*std::pow(10,residue);
		}
		return sign_;
	}
	void set_m(double value) {
		if (layer_<=2) *this=from_mantissa_exponent(value,e());
		else {
			sign_=(value>0)?1:(value<0)?-1:0;
			if (sign_==0) {
				layer_=0;
				mag_=0;
			}
		}
	}
	double e() const {
		if (sign_==0) return 0.0;
		if (layer_==0) return std::floor(std::log10(mag_));
		else if (layer_==1) return std::floor(mag_);
		else if (layer_==2) return std::floor(std::copysign(std::pow(10,std::abs(mag_)),mag_));
		else return mag_*std::numeric_limits<double>::infinity();
	}
	void set_e(double value) {
		*this=from_mantissa_exponent(m(),value);
	}
	int s() const {
		return sign_;
	}
	void set_s(int value) {
		if (value==0) {
			sign_=0;
			layer_=0;
			mag_=0;
		} else sign_=value;
	}

	double mantissa() const { return m(); }
	void set_mantissa(double value) { set_m(value); }
	double exponent() const { return e(); }
	void set_exponent(double value) { set_e(value); }

	decimal abs() const {
		return from_components_no_normalize((sign_==0)?0:1,layer_,mag_);
	}
	decimal neg() const {
		return from_components_no_normalize(-sign_,layer_,mag_);
	}
	decimal operator +() const {
		return *this;
	}
	decimal operator -() const {
		return neg();
	}
	decimal operator +(const decimal& other) const {
		if ((*this==d_inf && other==d_neg_inf) || (*this==d_neg_inf && other==d_inf)) return d_nan;
		if (!std::isfinite(layer_)) return *this;
		if (!std::isfinite(other.layer_)) return other;
		if (sign_==0) return other;
		if (other.sign_==0) return *this;
		if (sign_==-other.sign_ && layer_==other.layer_ && mag_==other.mag_) return d_zero;
		if (layer_>=2 || other.layer_>=2) return max_abs(other);
		const decimal* a=this;
		const decimal* b=&other;
		if (!cmp_abs(*b)>0) {//! a>b?
			a=&other;
			b=this;
		}
		if (a->layer_==0 && b->layer_==0) return from_number(a->sign_*a->mag_+b->sign_*b->mag_);
		double layera=a->layer_*std::copysign(1.0,a->mag_);
		double layerb=b->layer_*std::copysign(1.0,b->mag_);
		if (layera-layerb>=2) return *a;
		if (layera==0 && layerb==-1) {
			if (std::abs(b->mag_-std::log10(a->mag_))>max_significant_digits) return *a;
			else {
				double mag_diff=std::pow(10,std::log10(a->mag_)-b->mag_);
				double mantissa=b->sign_+a->sign_*mag_diff;
				return from_components(std::copysign(1.0, mantissa),1,b->mag_+std::log10(std::abs(mantissa)));
			}
		}
		if (layera==1 && layerb==0) {
			if (std::abs(a->mag_-std::log10(b->mag_))>max_significant_digits) return *a;
			else {
				double mag_diff=std::pow(10,a->mag_-std::log10(b->mag_));
				double mantissa=b->sign_+a->sign_*mag_diff;
				return from_components(std::copysign(1.0,mantissa),1,std::log10(b->mag_)+std::log10(std::abs(mantissa)));
			}
		}
		if (std::abs(a->mag_-b->mag_)>max_significant_digits) return *a;
		else {
			double mag_diff=std::pow(10,a->mag_-b->mag_);
			double mantissa=b->sign_+a->sign_*mag_diff;
			return from_components(std::copysign(1.0,mantissa),1,b->mag_+std::log10(std::abs(mantissa)));
		}
	}
	decimal& operator +=(const decimal& other) {
		*this=*this+other;
		return *this;
	}
	decimal operator -(const decimal& other) const {
		return operator +(-other);
	}
	decimal& operator -=(const decimal& other) {
		*this=*this-other;
		return *this;
	}
	decimal operator *(const decimal& other) const {
		if ((*this==d_inf && other==d_neg_inf) || (*this==d_neg_inf && other==d_inf)) return d_neg_inf;
		if (std::isinf(mag_) && other==d_zero || *this==d_zero && std::isinf(other.mag_)) return d_nan;
		if (*this==d_neg_inf && other==d_neg_inf) return d_inf;
		if (!std::isfinite(layer_)) return *this;
		if (!std::isfinite(other.layer_)) return other;
		if (sign_==0 || other.sign_==0) return d_zero;
		if (layer_==other.layer_ && mag_==-other.mag_) return from_components_no_normalize(sign_*other.sign_,0,1);
		const decimal* a=this;
		const decimal* b=&other;
		if (layer_<=other.layer_ && (layer_!=other.layer_ || std::abs(mag_)<=std::abs(other.mag_))) {
			a=&other;
			b=this;
		}
		if (a->layer_==0 && b->layer_==0) return from_number(a->sign_*b->sign_*a->mag_*b->mag_);
		if (a->layer_>=3 || a->layer_-b->layer_>=2) return from_components(a->sign_*b->sign_,a->layer_,a->mag_);
		if (a->layer_==1 && b->layer_==0) return from_components(a->sign_*b->sign_,1,a->mag_+std::log10(b->mag_));
		if (a->layer_==1 && b->layer_==1) return from_components(a->sign_*b->sign_,1,a->mag_+b->mag_);
		if (a->layer_==2 && b->layer_==1 || a->layer_==2 && b->layer_==2) {
			decimal new_mag=from_components(std::copysign(1.0,a->mag_),a->layer_-1,std::abs(a->mag_))+from_components(std::copysign(1.0,b->mag_),b->layer_-1,std::abs(b->mag_));
			return from_components(a->sign_*b->sign_,new_mag.layer_+1,new_mag.sign_*new_mag.mag_);
		}
		return d_nan;
	}
	decimal& operator *=(const decimal& other) {
		*this=*this*other;
		return *this;
	}
	decimal recip() const {
		if (mag_==0) return d_nan;
		if (std::isinf(mag_)) return d_zero;
		if (layer_==0) return from_components(sign_,0,1.0/mag_);
		return from_components(sign_,layer_,-mag_);
	}
	decimal operator /(const decimal& other) const {
		return operator *(other.recip());
	}
	decimal& operator /=(const decimal& other) {
		*this=*this/other;
		return *this;
	}
	decimal mod(const decimal& other,bool floored=false) const {
		decimal d=other.abs();
		if (*this==d_zero || d==d_zero) return d_zero;
		if (floored) {
			decimal abs_mod=abs().mod(d);
			if (sign_==-1!=(other.sign_==-1)) abs_mod=d.abs()-abs_mod;
			return abs_mod*from_number(other.sign_);
		}
		double num_this=to_number();
		double num_decimal=d.to_number();
		if (std::isfinite(num_this) && std::isfinite(num_decimal) && num_this!=0 && num_decimal!=0) from_number(std::fmod(num_this,num_decimal));
		if (*this-d==*this) return d_zero;
		if (d-*this==d) return *this;
		if (sign_==-1) return abs().mod(d).neg();
		return *this-((*this/d).floor()*d);
	}
	decimal operator %(const decimal& other) const {
		return mod(other);
	}
	decimal& operator %=(const decimal& other) {
		*this=*this%other;
		return *this;
	}
	int cmp(const decimal& other) const {
		if (sign_>other.sign_) return 1;
		if (sign_<other.sign_) return -1;
		return sign_*cmp_abs(other);
	}
	int cmp_abs(const decimal& other) const {
		double layera=mag_>0?layer_:-layer_;
		double layerb=other.mag_>0?other.layer_:-other.layer_;
		if (layera>layerb) return 1;
		if (layera<layerb) return -1;
		if (mag_>other.mag_) return 1;
		if (mag_<other.mag_) return -1;
		return 0;
	}
	bool operator <(const decimal& other) const {
		return cmp(other)==-1;
	}
	bool operator <=(const decimal& other) const {
		return !(other<*this);
	}
	bool operator >(const decimal& other) const {
		return other<*this;
	}
	bool operator >=(const decimal& other) const {
		return !(*this<other);
	}
	bool operator ==(const decimal& other) const {
		if (is_nan() || other.is_nan()) return false;
		return sign_==other.sign_ && layer_==other.layer_ && mag_==other.mag_;
	}
	bool operator !=(const decimal& other) const {
		return !(*this==other);
	}

	bool is_nan() const {
		return /*std::isnan(sign_) || */std::isnan(layer_) || std::isnan(mag_);
	}
	bool is_finite() const {
		return std::isfinite(sign_) && std::isfinite(layer_) && std::isfinite(mag_);
	}

	int cmp_tolerance(const decimal& other,double tolerance=1e-7) const {
		return eq_tolerance(other,tolerance)?0:cmp(other);
	}
	bool eq_tolerance(const decimal& other,double tolerance=1e-7) const {
		if (sign_!=other.sign_) return false;
		if (std::abs(layer_-other.layer_)>1) return false;
		double magA=mag_;
		double magB=other.mag_;
		if (layer_>other.layer_) magB=mag_log10(magB);
		if (layer_<other.layer_) magA=mag_log10(magA);
		return std::abs(magA-magB)<=tolerance*std::max(std::abs(magA),std::abs(magB));
	}
	bool neq_tolerance(const decimal& other,double tolerance=1e-7) const { 
		return !eq_tolerance(other,tolerance);
	}
	bool lt_tolerance(const decimal& other,double tolerance=1e-7) const {
		return !eq_tolerance(other,tolerance) && *this<other;
	}
	bool lte_tolerance(const decimal& other,double tolerance=1e-7) const {
		return eq_tolerance(other,tolerance) || *this<other;
	}
	bool gt_tolerance(const decimal& other,double tolerance=1e-7) const {
		return !eq_tolerance(other,tolerance) && *this>other;
	}
	bool gte_tolerance(const decimal& other,double tolerance=1e-7) const {
		return eq_tolerance(other,tolerance) || *this>other;
	}
    
	decimal p_log10() const {
		if (*this<d_zero) return d_zero;
		return log10();
	}
	decimal abs_log10() const {
		if (sign_==0) return d_nan;
		if (layer_>0) return from_components(std::copysign(1.0,mag_),layer_-1,std::abs(mag_));
		else return from_components(1,0,std::log10(mag_));
	}
	decimal log10() const {
		if (sign_<=0) return d_nan;
		if (layer_>0) return from_components(std::copysign(1.0,mag_),layer_-1,std::abs(mag_));
		else return from_components(sign_,0,std::log10(mag_));
	}
	decimal log(const decimal& base) const {
		if (sign_<=0) return d_nan;
		if (base.sign_<=0) return d_nan;
		if (base.sign_==1 && base.layer_==0 && base.mag_==1) return d_nan;
		if (layer_==0 && base.layer_==0) return from_components(sign_,0,std::log(mag_)/std::log(base.mag_));
		return (*this/log10())/(base.log10());
	}
	decimal log2() const {
		if (sign_<=0) return d_nan;
		if (layer_==0) return from_components(sign_,0,std::log2(mag_));
		else if (layer_==1) return from_components(std::copysign(1.0,mag_),0,std::abs(mag_)*3.321928094887362);
		else if (layer_==2) return from_components(std::copysign(1.0,mag_),1,std::abs(mag_)+0.5213902276543247);
		else return from_components(std::copysign(1.0,mag_),layer_-1,std::abs(mag_));
	}
	decimal ln() const {
		if (sign_<=0) return d_nan;
		if (layer_==0) return from_components(sign_,0,std::log(mag_));
		else if (layer_==1) return from_components(std::copysign(1.0,mag_),0,std::abs(mag_)*2.302585092994046);
		else if (layer_==2) return from_components(std::copysign(1.0,mag_),1,std::abs(mag_)+0.36221568869946325);
		else return from_components(std::copysign(1.0,mag_),layer_-1,std::abs(mag_));
	}
	decimal pow(const decimal& exponent) const {
		decimal a=*this;
		decimal b=exponent;
		if (a.sign_==0) return b==d_zero?d_one:a;
		if (a.sign_==1 && a.layer_==0 && a.mag_==1) return a;
		if (b.sign_==0) return d_one;
		if (b.sign_==1 && b.layer_==0 && b.mag_==1) return a;
		decimal result=(a.abs_log10()*b).pow10();
		if (sign_==-1) {
			if (std::fmod(std::abs(std::fmod(b.to_number(),2)),2)==1) return result.neg();
			else if (std::fmod(std::abs(std::fmod(b.to_number(),2)),2)==0) return result;
			return d_nan;
		}
		return result;
	}
	decimal pow_base(const decimal& base) const { return base.pow(*this); }
	decimal pow10() const {
		if (*this==d_inf) return d_inf;
		if (*this==d_neg_inf) return d_zero;
		if (!std::isfinite(layer_) || !std::isfinite(mag_)) return d_nan;
		decimal a=*this;
		if (a.layer_==0) {
			double new_mag=std::pow(10,a.sign_*a.mag_);
			if (std::isfinite(new_mag) && std::abs(new_mag)>=0.1) return from_components(1,0,new_mag);
			else {
				if (a.sign_==0) return d_one;
				else a=from_components_no_normalize(a.sign_,a.layer_+1,std::log10(a.mag_));
			}
		}
		if (a.sign_>0 && a.mag_>=0) return from_components(a.sign_,a.layer_+1,a.mag_);
		if (a.sign_<0 && a.mag_>=0) return from_components(-a.sign_,a.layer_+1,-a.mag_);
		return d_one;
	}
    
	decimal root(const decimal& degree) const {
		decimal d=degree;
		if (*this<0 && d.mod(2,true)==1) return neg().root(d).neg();
		return pow(d.recip());
	}
	decimal factorial() const {
		if (mag_<0) return (*this+1).gamma();
		else if (layer_==0) return (*this+1).gamma();
		else if (layer_==1) return (*this*(ln()-1)).exp();
		else return exp();
	}
	decimal gamma() const {
		if (mag_<0) return recip();
		else if (layer_==0) {
			if (*this<from_components(1,0,24)) return from_number(gamma(sign_*mag_));
			double t=mag_-1;
			double l=0.9189385332046727;
			l=l+(t+0.5)*std::log(t);
			l=l-t;
			double n2=t*t;
			double np=t;
			double lm=12*np;
			double adj=1/lm;
			double l2=l+adj;
			if (l2==l) return decimal(l).exp();
			l=l2;
			np=np*n2;
			lm=360*np;
			adj=1/lm;
			l2=l-adj;
			if (l2==l) return decimal(l).exp();
			l=l2;
			np=np*n2;
			lm=1260*np;
			double lt_val=1/lm;
			l=l+lt_val;
			np=np*n2;
			lm=1680*np;
			lt_val=1/lm;
			l=l-lt_val;
			return decimal(l).exp();
		} else if (layer_==1) return (*this*(ln()-1)).exp();
		return exp();
	}
	decimal ln_gamma() const { return gamma().ln(); }
	decimal exp() const {
		if (mag_<0) return d_one;
		if (layer_==0 && mag_<=709.7) return from_number(std::exp(sign_*mag_));
		else if (layer_==0) return from_components(1,1,sign_*std::log10(M_E)*mag_);
		else if (layer_==1) return from_components(1,2,sign_*(std::log10(0.4342944819032518)+mag_));
		else return from_components(1,layer_+1,sign_*mag_);
	}
    
	decimal square() const { return pow(from_number(2)); }
    
	decimal sqrt() const {
		if (layer_==0) return from_number(std::sqrt(sign_*mag_));
		else if (layer_==1) return from_components(1,2,std::log10(mag_)-0.3010299956639812);
		else {
			decimal result=from_components_no_normalize(sign_,layer_-1,mag_)/from_components_no_normalize(1,0,2);
			result.layer_+=1;
			result.normalize();
			return result;
		}
	}
	decimal cube() const { return pow(from_number(3)); }
	decimal cbrt() const {
		if (*this<0) return neg().pow(1.0/3.0).neg();
		return pow(1.0/3.0);
	}
    
	decimal tetrate(double height=2.0,const decimal& payload=d_one,bool linear=false) const {
		if (height==1) return pow(payload);
		if (height==0) return payload;
		if (*this==d_one) return d_one;
		if (*this==from_number(-1)) return pow(payload);
		if (height==std::numeric_limits<double>::infinity()) {
			double this_num=to_number();
			if (this_num<=1.44466786100976613366 && this_num>=0.06598803584531253708) {
				decimal neg_ln=ln().neg();
				decimal lower=neg_ln.lambertw()/neg_ln;
				if (this_num<1) return lower;
				decimal upper=neg_ln.lambertw(false)/neg_ln;
				if (this_num>1.444667861009099) lower=upper=from_number(M_E);
				//payload = D(payload);
				if (payload==upper) return upper;
				else if (payload<upper) return lower;
				else return d_inf;
			} else if (this_num>1.44466786100976613366) return d_inf;
			else return d_nan;
		}
		if (*this==d_zero) {
			double result=std::abs(std::fmod(height+1,2));
			if (result>1) result=2-result;
			return from_number(result);
		}
		if (height<0) return payload.iterated_log(*this,-height,linear);
		decimal payload_copy=payload;
		double frac_height=height-static_cast<int>(height);
		height=static_cast<int>(height);
		if (*this>d_zero && (*this<1 || *this<=1.44466786100976613366 && payload_copy<=ln().neg().lambertw(false)/(ln().neg())) && (height+frac_height>10000 || !linear)) {
			int limit_height=std::min(10000,(int)height);
			if (payload_copy==d_one) payload_copy=pow(frac_height);
			else if (*this<1) payload_copy=payload_copy.pow(1-frac_height)*(pow(payload).pow(frac_height));
			else payload_copy=payload_copy.layer_add(frac_height,*this);
			for (int i=0;i<limit_height;i++) {
				decimal old_payload=payload_copy;
				payload_copy=pow(payload_copy); //stop early if we converge
				if (old_payload==payload_copy) return payload_copy;
			}
			if (height+frac_height>10000 && std::fmod(std::ceil(height+frac_height),2)==1) return pow(payload_copy);
			return payload_copy;
        }
		if (frac_height!=0) {
			if (payload==d_one) {
				if (*this>10 || linear) payload_copy=pow(frac_height);
				else {
					double crit_val=tetrate_critical(to_number(),frac_height);
					payload_copy=from_number(crit_val);
					if (*this<2) payload_copy=(payload_copy-1)*(*this-1)+1;
				}
			} else {
				if (*this==10) payload_copy=payload.layer_add10(frac_height,linear);
				else if (*this<1) payload_copy=payload.pow(1-frac_height)*(pow(payload).pow(frac_height));
				else payload_copy=payload.layer_add(frac_height,*this,linear);
			}
		}
		for (int i=0;i<height;i++) {
			payload_copy=pow(payload_copy);
			if (!std::isfinite(payload_copy.layer()) || !std::isfinite(payload_copy.mag())) return payload_copy.normalize();
			if (payload_copy.layer()-layer()>3) return from_components_no_normalize(payload_copy.sign(),payload_copy.layer()+(height-i-1),payload_copy.mag());
			if (i>10000) return payload_copy;
		}
		return payload_copy;
	}
	decimal iterated_exp(double height=2.0,const decimal& payload=d_one,bool linear=false) const {
		return tetrate(height,payload,linear);
	}
	decimal iterated_log(const decimal& base=d_ten,double times=1.0,bool linear=false) const {
		if (times<0) return base.tetrate(-times,*this,linear);
		decimal base_copy=base;
		decimal result=*this;
		double full_times=times;
		int times_int=static_cast<int>(times);
		double fraction=full_times-times_int;
		if (result.layer()-base.layer()>3) {
			double layer_loss=std::min(times,result.layer()-base.layer()-3);
			times_int-=static_cast<int>(layer_loss);
			result.layer_-=layer_loss;
		}
		for (int i=0;i<times_int;i++) {
			result=result.log(base_copy);
			if (!std::isfinite(result.layer()) || !std::isfinite(result.mag())) return result.normalize();
			if (i>10000) return result;
		}
		if (fraction>0 && fraction<1) {
			if (base==10) result=result.layer_add10(-fraction,linear);
			else result=result.layer_add(-fraction,base,linear);
		}
		return result;
	}
	decimal slog(const decimal& base=d_ten,int iterations=100,bool linear=false) const {
		double step_size=0.001;
		bool has_changed_directions_once=false;
		bool previously_rose=false;
		double result=slog_internal(base,linear).to_number();
		for (int i=1;i<iterations;i++) {
			decimal new_decimal=decimal(base).tetrate(result,d_one,linear);//WHY demical(base) BUTNOT base??
			bool currently_rose=new_decimal>*this;
			if (i>1) {
				if (previously_rose!=currently_rose) has_changed_directions_once=true;
			}
			previously_rose=currently_rose;
			if (has_changed_directions_once) step_size/=2;
			else step_size*=2;
			step_size=std::abs(step_size)*(currently_rose?-1:1);
			result+=step_size;
			if (step_size==0) break;
		}
		return from_number(result);
	}
	decimal slog_internal(const decimal& base=d_ten,bool linear=false) const {
		decimal base_copy=base;
		if (base<=d_zero) return d_nan;
		if (base==d_one) return d_nan;
		if (base<1) {
			if (*this==d_one) return d_zero;
			if (*this==d_zero) return d_neg_one;
			return d_nan;
		}
		if (mag_<0 || *this==d_zero) return d_neg_one;
		if (base<1.44466786100976613366) {
			decimal neg_ln=base.ln().neg();
			decimal inf_tower=neg_ln.lambertw()/neg_ln;
			if (*this==inf_tower) return d_inf;
			if (*this>inf_tower) return d_nan;
		}
		double result=0;
		decimal copy=*this;
		if (copy.layer()-base.layer()>3) {
			double layer_loss=copy.layer()-base.layer()-3;
			result+=layer_loss;
			copy.layer_-=layer_loss;
		}
		for (int i=0;i<100;i++) {
			if (copy<d_zero) {
				copy=base.pow(copy);
				result-=1;
			} else if (copy<=d_one) {
				if (linear) return from_number(result+copy.to_number()-1);
				else return from_number(result+slog_critical(base.to_number(),copy.to_number()));
			} else {
				result+=1;
				copy=copy.log(base);
			}
		}
		return from_number(result);
	}
	decimal layer_add10(double diff,bool linear=false) const {
		double diff_num=diff;
		decimal result=*this;
		if (diff_num>=1) {
			if (result.mag_<0 && result.layer_>0) {
				result.sign_=0;
				result.mag_=0;
				result.layer_=0;
			} else if (result.sign_==-1 && result.layer_==0) {
				result.sign_=1;
				result.mag_=-result.mag_;
			}
			int layer_add=static_cast<int>(diff_num);
			diff_num-=layer_add;
			result.layer_+=layer_add;
		}
		if (diff_num<=-1) {
			int layer_add=static_cast<int>(diff_num);
			diff_num-=layer_add;
			result.layer_+=layer_add;
			if (result.layer_<0) {
				for (int i=0;i<100;i++) {
					result.layer_++;
					result.mag_=std::log10(result.mag_);
					if (!std::isfinite(result.mag_)) {
						if (result.sign_==0) result.sign_=1;
						if (result.layer_<0) result.layer_=0;
						return result.normalize();
					}
					if (result.layer_>=0) break;
				}
			}
		}
		while (result.layer_<0) {
			result.layer_++;
			result.mag_=std::log10(result.mag_);
		}
		if (result.sign_==0) {
			result.sign_=1;
			if (result.mag_==0 && result.layer_>=1) {
				result.layer_-=1;
				result.mag_=1;
			}
		}      
		result.normalize();
		if (diff_num!=0) return result.layer_add(diff_num,10,linear);
		return result;
	}
	decimal layer_add(double diff,const decimal& base=d_ten,bool linear=false) const {
		decimal base_d=base;
		if (base_d>1 && base_d<=1.44466786100976613366) {
			auto excess_slog_result=excess_slog(*this,base,linear);
			double slog_this=excess_slog_result.first.to_number();
			int range=excess_slog_result.second;
			double slog_dest=slog_this+diff;
			decimal neg_ln=base.ln().neg();
			decimal lower=neg_ln.lambertw()/neg_ln;
			decimal upper=neg_ln.lambertw(false)/neg_ln;
			decimal slog_zero=d_one;
			if (range==1) slog_zero=(lower*upper).sqrt();
			else if (range==2) slog_zero=upper*2;
			decimal slog_one=base_d.pow(slog_zero);
			int whole_height=static_cast<int>(std::floor(slog_dest));
			double frac_height=slog_dest-whole_height;
			decimal tower_top=slog_zero.pow(1-frac_height)*(slog_one.pow(frac_height));
			return base_d.tetrate(whole_height,tower_top,linear);
		}
		double slog_this=slog(base,100,linear).to_number();
        	double slog_dest=slog_this+diff;
		if (slog_dest>=0) return base.tetrate(slog_dest,d_one,linear);
		else if (!std::isfinite(slog_dest)) return d_nan;
		else if (slog_dest>=-1) return base.tetrate(slog_dest+1,d_one,linear).log(base);
		return base.tetrate(slog_dest+2,d_one,linear).log(base).log(base);
	}
	decimal lambertw(bool principal=true) const {
		if (*this<-0.3678794411710499) return (d_nan);
		else if (principal) {
			if (abs()<decimal("1e-300")) return *this;
			else if (mag_<0) return from_number(lambertw(to_number()));
			else if (layer_==0) return from_number(lambertw(sign_*mag_));
			else if (*this<decimal("eee15")) return lambertw(this);
			else return ln();
		} else {
			if (sign_==1) return d_nan; //complex
			if (layer_==0) return from_number(lambertw(sign_*mag_,1e-10,false));
			else if (layer_==1) return lambertw(*this,1e-10,false);
			else return neg().recip().lambertw().neg();
		}
	}
	decimal ssqrt() const {
		return linear_sroot(2);
	}
	decimal linear_sroot(double degree) const {
		if (degree==1) return *this;
		if (*this==d_inf) return d_inf;
		if (!is_finite()) return d_nan;
		if (degree>0 && degree<1) return root(from_number(degree));
		if (degree>-2 && degree<-1) return (from_number(degree)+2).pow(recip());
		if (degree<=0) return d_nan;
		if (degree==std::numeric_limits<double>::infinity()) {
			double this_num=to_number();
			if (this_num<M_E && this_num>exp_n1) return pow(recip());
			else return d_nan;
		}
		if (*this==1) return d_one;//FC_NN(1,0,1)
		if (*this<0) return d_nan;
		if (*this<=decimal("1ee-16")) {
			if (static_cast<int>(degree)%2==1) return *this;
			else return d_nan;
		}
		if (*this>1) {
			decimal upper_bound=d_ten;
			if (*this>=decimal(10).tetrate(degree,1,true)) upper_bound=iterated_log(10,degree-1,true);
			if (degree<=1) upper_bound=root(from_number(degree));
			decimal lower=d_zero;
			double layer=upper_bound.layer();
			decimal upper=upper_bound.iterated_log(10,layer,true);
			decimal previous=upper;
			decimal guess=upper/2;
			bool loop_going=true;
			while (loop_going) {
				guess=(lower+upper)/2;
				if (decimal(10).iterated_exp(layer,guess,true).tetrate(degree,1,true)>*this) upper=guess;
				else lower=guess;
				if (guess==previous) loop_going=false;
				else previous=guess;
			}
			return decimal(10).iterated_exp(layer,guess,true);
		} else {
			int stage=1;
			decimal minimum=from_components(1,10,1);
			decimal maximum=from_components(1,10,1);
			decimal lower=from_components(1,10,1); //eeeeeeeee-10, which is effectively 0; I would use decimal.dInf but its reciprocal is NaN
			decimal upper=from_components(1,1,-16); //~ 1 - 1e-16
			decimal prev_span=d_zero;
			decimal difference=from_components(1,10,1);
			decimal upper_bound=upper.pow10().recip();
			decimal distance=d_zero;
			decimal prev_point=upper_bound;
			decimal next_point=upper_bound;
			bool even_degree=static_cast<int>(std::ceil(degree))%2==0;
			int range=0;
			decimal last_valid=from_components(1,10,1);
			bool inf_loop_detector=false;
			decimal previous_upper=d_zero;
			bool decreasing_found=false;
			while (stage<4) {
				if (stage==2) {
					if (even_degree) break;
					else {
						lower=from_components(1,10,1);
						upper=minimum;
						stage=3;
						difference=from_components(1,10,1);
						last_valid=from_components(1,10,1);
					}
				}
				inf_loop_detector=false;
				while (upper!=lower) {
					previous_upper=upper;
					if (upper.pow10().recip().tetrate(degree,1,true)==1 && upper.pow10().recip()<0.4) {
						upper_bound=upper.pow10().recip();
						prev_point=upper.pow10().recip();
						next_point=upper.pow10().recip();
						distance=d_zero;
						range=-1; //This would cause problems with degree < 1 in the linear approximation... but those are already covered as a special case
						if (stage==3) last_valid=upper;
					} else if (upper.pow10().recip().tetrate(degree,1,true)==upper.pow10().recip() && !even_degree && upper.pow10().recip()<0.4) {
						upper_bound=upper.pow10().recip();
						prev_point=upper.pow10().recip();
						next_point=upper.pow10().recip();
						distance=d_zero;
						range=0;
					} else if (upper.pow10().recip().tetrate(degree,1,true)==(upper.pow10().recip()*2).tetrate(degree,1,true)) {
						//If the upper bound is closer to zero than the next point with a discernable tetration, so surely it's in whichever range is closest to zero?
						//This won't happen in a strictly increasing tetration, as there x^^degree ~= x as x approaches zero
						upper_bound=upper.pow10().recip();
						prev_point=d_zero;
						next_point=upper_bound*2;
						distance=upper_bound;
						if (even_degree) range=-1;
						else range=0;
					} else {
						//We want to use prevspan to find the "previous point" right before the upper bound and the "next point" right after the upper bound, as that will let us approximate derivatives
						prev_span=upper*1.2e-16;
						upper_bound=upper.pow10().recip();
						prev_point=(upper+prev_span).pow10().recip();
						distance=upper_bound-prev_point;
						next_point=upper_bound+distance; //...but it's of no use to us while its tetration is equal to upper's tetration, so widen the difference until it's not
						//We add prevspan and subtract nextspan because, since upper is log10(recip(upper bound)), the upper bound gets smaller as upper gets larger and vice versa
						while (prev_point.tetrate(degree,1,true)==upper_bound.tetrate(degree,1,true) || next_point.tetrate(degree,1,true)==upper_bound.tetrate(degree,1,true) || prev_point>=upper_bound || next_point<=upper_bound) {
							prev_span=prev_span*2;
							prev_point=(upper+prev_span).pow10().recip();
							distance=upper_bound-prev_point;
							next_point=upper_bound+distance;
						}
						if (stage==1 && next_point.tetrate(degree,1,true)>upper_bound.tetrate(degree,1,true) && prev_point.tetrate(degree,1,true)>upper_bound.tetrate(degree,1,true) || stage==3 && next_point.tetrate(degree,1,true)<upper_bound.tetrate(degree,1,true) && prev_point.tetrate(degree,1,true)<upper_bound.tetrate(degree,1,true)) last_valid=upper;
						if (next_point.tetrate(degree,1,true)<upper_bound.tetrate(degree,1,true)) range=-1;//Derivative is negative, so we're in decreasing range
						else if (even_degree) range=1;//No zero range, so we're in increasing range
						else if (stage==3 && upper.gt_tolerance(minimum,1e-8)) range=0;//We're already below the minimum, so we can't be in range 1
						else {
							//Number imprecision has left the second derivative somewhat untrustworthy, so we need to expand the bounds to ensure it's correct
							while (prev_point.tetrate(degree,1,true).eq_tolerance(upper_bound.tetrate(degree,1,true),1e-8) || next_point.tetrate(degree,1,true).eq_tolerance(upper_bound.tetrate(degree,1,true),1e-8) || prev_point>=upper_bound || next_point<=upper_bound) {
								prev_span=prev_span*2;
								prev_point=(upper+prev_span).pow10().recip();
								distance=upper_bound-prev_point;
								next_point=upper_bound+distance;
							}
							if (next_point.tetrate(degree,1,true)-upper_bound.tetrate(degree,1,true)<upper_bound.tetrate(degree,1,true)-prev_point.tetrate(degree,1,true)) range=0;//Second derivative is negative, so we're in zero range
							else range=1;//By process of elimination, we're in increasing range
						}
					}
					if (range==-1) decreasing_found=true;
					if (stage==1 && range==1 || stage==3 && range!=0) {
						//The upper bound is too high
						if (lower==from_components(1,10,1)) upper=upper*2;
						else {
							bool cut_off=false;
							if (inf_loop_detector && (range==1 && stage==1 || range==-1 && stage==3)) cut_off=true; //Avoids infinite loops from floating point imprecision
							upper=(upper+lower)/2;
							if (cut_off) break;
						}
					} else {
						if (lower==from_components(1,10,1)) {
							//We've now found an actual lower bound
							lower=upper;
							upper=upper/2;
						} else {
							//The upper bound is too low, meaning last time we decreased the upper bound, we should have gone to the other half of the new range instead
							bool cut_off=false;
							if (inf_loop_detector && (range==1 && stage==1 || range==-1 && stage==3)) cut_off=true; //Avoids infinite loops from floating point imprecision
							lower=lower-difference;
							upper=upper-difference;
							if (cut_off) break;
						}
					}
					if (((lower-upper)/2).abs()>difference*1.5) inf_loop_detector=true;
					difference=((lower-upper)/2).abs();
					if (upper>decimal("1e18")) break;
					if (upper==previous_upper) break; //Another infinite loop catcher
				}
				if (upper>decimal("1e18")) break;
				if (!decreasing_found) break; //If there's no decreasing range, then even if an error caused lastValid to gain a value, the minimum can't exist
				if (last_valid==from_components(1,10,1)) break;//Whatever we're searching for, it doesn't exist. If there's no minimum, then there's no maximum either, so either way we can end the loop here.
				if (stage==1) minimum=last_valid;
				else if (stage==3) maximum=last_valid;
				stage++;
			}//Now we have the minimum and maximum, so it's time to calculate the actual super-root.
			lower=minimum;
			upper=from_components(1,1,-18);
			decimal previous=upper;
			decimal guess=d_zero;
			bool loop_going=true;
			while (loop_going) {
				if (lower==from_components(1,10,1)) guess=upper*2;
				else guess=(lower+upper)/2;
				if (decimal(10).pow(guess).recip().tetrate(degree,1,true)>*this) upper=guess;
				else lower=guess;
				if (guess==previous) loop_going=false;
				else previous=guess;
				if (upper>decimal("1e18")) return d_nan;
			} //using guess.neq(minimum) led to imprecision errors, so here's a fixed version of that
			if (!guess.eq_tolerance(minimum,1e-15)) return guess.pow10().recip();
			else {
				//If guess == minimum, we haven't actually found the super-root, the algorithm just kept going down trying to find a super-root that's not in the increasing range.
				//Check if the root is in the zero range.
				if (maximum==from_components(1,10,1)) return d_nan;//There is no zero range, so the super root doesn't exist
				lower=from_components(1,10,1);
				upper=maximum;
				previous=upper;
				guess=d_zero;
				loop_going=true;
				while (loop_going) {
					if (lower==from_components(1,10,1)) guess=upper*2;
					else guess=(lower+upper)/2;
					if (decimal(10).pow(guess).recip().tetrate(degree,1,true)>*this) upper=guess;
					else lower=guess;
					if (guess==previous) loop_going=false;
					else previous=guess;
					if (upper>decimal("1e18")) return d_nan;
				}
				return guess.pow10().recip();
			}
		}
	}

	decimal pentate(double height=2.0,const decimal& payload=d_one,bool linear=false) const {
		decimal payload_copy=payload;
		double frac_height=height-static_cast<int>(height);
		height=static_cast<int>(height);
		decimal prev_payload=d_zero;
		decimal prev_two_payload=d_zero;
		if (frac_height!=0) {
			if (payload==d_one) {
				height++;
				payload_copy=from_number(frac_height);
			} else return pentate((payload.penta_log(*this,100,linear)+height+frac_height).to_number(),1,linear);
		}
		if (height>0) {
			for (int i=0;i<height;) {
				prev_two_payload=prev_payload;
				prev_payload=payload_copy;
				payload_copy=tetrate(payload_copy.to_number(),d_one,linear);
				i++;
				if (*this>0 && *this<=1 && payload_copy>0 && payload_copy<=1) return tetrate(height-i,payload_copy,linear);
				if (payload_copy==prev_payload || payload_copy==prev_two_payload && i%2==(int)height%2) return payload_copy.normalize();
				if (!std::isfinite(payload_copy.layer()) || !std::isfinite(payload_copy.mag())) return payload_copy.normalize();
				if (i>10000) return payload_copy;
			}
		} else {
			for (int i=0;i<-height;i++) {
				prev_payload=payload_copy;
				payload_copy=payload_copy.slog(*this,100,linear);
				if (payload_copy==prev_payload) return payload_copy.normalize();
				if (!std::isfinite(payload_copy.layer()) || !std::isfinite(payload_copy.mag())) return payload_copy.normalize();
				if (i>100) return payload_copy;
			}
		}
		return payload_copy;
	}
	decimal penta_log(const decimal& base=d_ten,int iterations=100,bool linear=false) const {
		decimal base_copy=base;
		if (base<=1) return d_nan;
		if (*this==1) return d_zero;
		if (*this==d_inf) return d_inf;
		double value_num=1.0;
		double result=0;
		double step_size=1;
		if (*this<-1) {
			if (*this<=-2) return d_nan;
			decimal limit_check=base.tetrate(to_number(),1,linear);
			if (*this==limit_check) return d_neg_inf;
			if (*this>limit_check) return d_nan;
		}
		if (*this>1) {
			while (value_num<to_number()) {
				result++;
				value_num=base.tetrate(value_num,1,linear).to_number();
				if (result>1000) return d_nan;
			}
		} else {
			while (value_num>to_number()) {
				result--;
				value_num=from_number(value_num).slog(base,linear).to_number();
				if (result>100) return d_nan;
			}
		}
		for (int i=1;i<iterations;i++) {
			decimal new_decimal=base.pentate(result,d_one,linear);
			if (new_decimal==*this) break;
			bool currently_rose=new_decimal>*this;
			step_size=std::abs(step_size)*(currently_rose?-1:1);
			result+=step_size;
			step_size/=2;
			if (step_size==0) break;
		}
		return from_number(result);
	}
	decimal linear_penta_root(double degree) const {
		if (degree==1) return *this;
		if (degree<0) return d_nan;
		if (*this==d_inf) return d_inf;
		if (!is_finite()) return d_nan;
		if (degree>0 && degree<1) return root(from_number(degree));
		if (*this==1) return d_one;
		if (*this<0) return d_nan;
		if (*this<1) return linear_sroot(degree);
		auto pentate_func=[degree](const decimal& value)->decimal{
			return value.pentate(degree,d_one,true);
		};
		auto inverse=increasing_inverse(pentate_func,false,120,d_zero,d_layer_max,d_zero,d_layer_max);
		return inverse(*this);
	}
	decimal sin() const {
		if (mag_<0) return *this;
		if (layer_==0) return from_number(std::sin(sign_*mag_));
		return d_zero;
	}
	decimal cos() const {
		if (mag_<0) return d_one;
		if (layer_==0) return from_number(std::cos(sign_*mag_));
		return d_zero;
	}
	decimal tan() const {
		if (mag_<0) return *this;
		if (layer_==0) return from_number(std::tan(sign_*mag_));
		return d_zero;
	}
	decimal asin() const {
		if (mag_<0) return *this;
		if (layer_==0) return from_number(std::asin(sign_*mag_));
		return d_nan;
	}
	decimal acos() const {
		if (mag_<0) return from_number(std::acos(to_number()));
		if (layer_==0) return from_number(std::acos(sign_*mag_));
		return d_nan;
	}
	decimal atan() const {
		if (mag_<0) return *this;
		if (layer_==0) return from_number(std::atan(sign_*mag_));
		return from_number(std::atan(sign_*std::numeric_limits<double>::max()));
	}
	decimal sinh() const { return (exp()-neg().exp())/2; }
	decimal cosh() const { return (exp()+neg().exp())/2; }
	decimal tanh() const { return sinh()/cosh(); }
	decimal asinh() const { return (*this+(square()+1).sqrt()).ln(); }
	decimal acosh() const { return (*this+(square()-1).sqrt()).ln(); }
	decimal atanh() const {
		if (abs()>=1) return d_nan;
		return ((*this+1)/(from_number(1)-*this)).ln()/2;
	}

	decimal round() const {
		if (mag_<0) return d_zero;
		if (layer_==0) return from_components(sign_,0,std::round(mag_));
		return *this;
	}
	decimal floor() const {
		if (mag_<0) {
			if (sign_==-1) return from_components_no_normalize(-1,0,1);
			else return d_zero;
		}
		if (sign_==-1) return neg().ceil().neg();
		if (layer_==0) return from_components(sign_,0,std::floor(mag_));
		return *this;
	}
	decimal ceil() const {
		if (mag_<0) {
			if (sign_==1) return from_components_no_normalize(1,0,1);
			else return d_zero;
		}
		if (sign_==-1) return neg().floor().neg();
		if (layer_==0) return from_components(sign_,0,std::ceil(mag_));
		return *this;
	}
	decimal trunc() const {
		if (mag_<0) return d_zero;
		if (layer_==0) return from_components(sign_,0,std::trunc(mag_));
		return *this;
	}
	decimal max(const decimal& other) const { return operator <(other)?other:*this; }
	decimal min(const decimal& other) const { return operator >(other)?other:*this; }
	decimal max_abs(const decimal& other) const { return cmp_abs(other)<0?other:*this; }
	decimal min_abs(const decimal& other) const { return cmp_abs(other)>0?other:*this; }
	decimal clamp(const decimal& min_val,const decimal& max_val) const { return max(min_val).min(max_val); }
	decimal clamp_min(const decimal& min_val) const { return max(min_val); }
	decimal clamp_max(const decimal& max_val) const { return min(max_val); }
    
	double to_number() const {
		if (std::isinf(mag_) && std::isinf(layer_) && sign_==1) return std::numeric_limits<double>::infinity();
		if (std::isinf(mag_) && std::isinf(layer_) && sign_==-1) return -std::numeric_limits<double>::infinity();
		if (!std::isfinite(layer_)) return std::numeric_limits<double>::quiet_NaN();
		if (layer_==0) return sign_*mag_;
		else if (layer_==1) return sign_*std::pow(10,mag_);
		else return mag_>0?(sign_>0?std::numeric_limits<double>::infinity():-std::numeric_limits<double>::infinity()):0;
	}
    
	double mantissa_with_decimal_places(int places) const {
		if (std::isnan(m())) return std::numeric_limits<double>::quiet_NaN();
		if (m()==0) return 0;
		return decimal_places(m(),places);
	}
	double magnitude_with_decimal_places(int places) const {
		if (std::isnan(mag_)) return std::numeric_limits<double>::quiet_NaN();
		if (mag_==0) return 0;
		return decimal_places(mag_,places);
	}
    
	std::string to_string() const {
		if (is_nan()) return "NaN";
		if (std::isinf(mag_) || std::isinf(layer_)) return sign_==1?"Infinity":"-Infinity";
		if (layer_==0) {
			if ((mag_<1e21 && mag_>1e-7) || mag_==0) return std::to_string(sign_*mag_);
			return std::to_string(m())+"e"+std::to_string(e());
		} else if (layer_==1) return std::to_string(m())+"e"+std::to_string(e());
		else {
			if (layer_<=max_es_in_a_row) return (sign_==-1?"-":"")+std::string(static_cast<int>(layer_),'e')+std::to_string(mag_);
			else return (sign_==-1?"-":"")+std::string("(e^")+std::to_string(layer_)+std::string(")")+std::to_string(mag_);
		}
	}
	std::string to_exponential(int places) const {
		if (layer_==0) {
			std::ostringstream oss;
			oss<<std::scientific<<std::setprecision(places)<<(sign_*mag_);
			return oss.str();
		}
		return to_string_with_decimal_places(places);
	}
	std::string to_fixed(int places) const {
		if (layer_==0) {
			std::ostringstream oss;
			oss<<std::fixed<<std::setprecision(places)<<(sign_*mag_);
			return oss.str();
		}
		return to_string_with_decimal_places(places);
	}
	std::string to_precision(int places) const {
		if (e()<=-7) return to_exponential(places-1);
		if (places>e()) return to_fixed(places-e()-1);
		return to_exponential(places-1);
	}
	std::string to_string_with_decimal_places(int places) const {
		if (layer_==0) {
			if ((mag_<1e21 && mag_>1e-7) || mag_==0) {
				std::ostringstream oss;
				oss<<std::fixed<<std::setprecision(places)<<(sign_*mag_);
				return oss.str();
			}
			return std::to_string(decimal_places(m(),places))+"e"+std::to_string(decimal_places(e(),places));
		} else if (layer_==1) return std::to_string(decimal_places(m(),places))+"e"+std::to_string(decimal_places(e(),places));
		else {
			if (layer_<=max_es_in_a_row) return (sign_==-1?"-":"")+std::string(static_cast<int>(layer_),'e')+std::to_string(decimal_places(mag_,places));
			else return (sign_==-1?"-":"")+std::string("(e^")+std::to_string(layer_)+std::string(")")+std::to_string(decimal_places(mag_,places));
		}
	}
	operator std::string() const {
		return to_string();
	}

	static double decimal_places(double value,int places) {
		if (std::isnan(value) || std::isinf(value)) return value;
		if (value==0.0) return 0.0;
		int len=places+1;
		int num_digits=static_cast<int>(std::ceil(std::log10(std::abs(value))));
		double rounded=std::round(value*std::pow(10,len-num_digits))*std::pow(10,num_digits-len);
		return rounded;
	}
	static double mag_log10(double n) {
		return std::copysign(std::log10(std::abs(n)),n);
	}
	static double gamma(double n) {
		if (!std::isfinite(n)) return n;
		if (n<-50) {
			if (n==std::trunc(n)) return -std::numeric_limits<double>::infinity();
			return 0.0;
		}
		double scal1=1.0;
		while (n<10.0) scal1=scal1*n++;
		n-=1.0;
		double l=0.9189385332046727; // 0.5*log(2*PI)
		l=l+(n+0.5)*std::log(n);
		l=l-n;
		double n2=n*n;
		double np=n;
		l=l+1.0/(12.0*np);
		np=np*n2;
		l=l-1.0/(360.0*np);
		np=np*n2;
		l=l+1.0/(1260.0*np);
		np=np*n2;
		l=l-1.0/(1680.0*np);
		np=np*n2;
		l=l+1.0/(1188.0*np);
		np=np*n2;
		l=l-691.0/(360360.0*np);
		np=np*n2;
		l=l+7.0/(1092.0*np);
		np=np*n2;
		l=l-3617.0/(122400.0*np);
		return std::exp(l)/scal1;
	}
	static double lambertw(double z,double tolerance=1e-10,bool principal=true) {
		if (!std::isfinite(z)) return z;
		double w;
		if (principal) {
			if (z==0.0) return z;
			if (z==1.0) return omega;
			if (z<10.0) w=0.0;
			else w=std::log(z)-std::log(std::log(z));
		} else {
			if (z==0.0) return -std::numeric_limits<double>::infinity();
			if (z<=-0.1) w=-2.0;
			else w=std::log(-z)-std::log(-std::log(-z));
		}
		for (int i=0;i<100;i++) {
			double wn=(z*std::exp(-w)+w*w)/(w+1.0);
			if (std::abs(wn-w)<tolerance*std::abs(wn)) return wn;
			else w=wn;
		}
		return std::numeric_limits<double>::quiet_NaN();
	}
	static decimal lambertw(const decimal& z,double tolerance=1e-10,bool principal=true) {
		decimal w,ew,wewz,wn;
		if (!z.is_finite()) return z;
		if (principal) {
			if (z==d_zero) return d_zero;
			if (z==d_one) return from_number(omega);
			w=z.ln();
		} else {
			if (z==d_zero) return d_neg_inf;
			w=-z.ln();
		}
		for (int i=0;i<100;i++) {
			ew=-w.exp();
			wewz=w-z*ew;
			wn=w-wewz/(w+1-(w+2)*wewz/(decimal(2)*w+2));
			if ((wn-w).abs()<wn.abs()*tolerance) return wn;
			else w=wn;
		}
		throw std::runtime_error("Iteration failed to converge: " + z.to_string());
	}
	static std::function<decimal(const decimal&)> increasing_inverse(std::function<decimal(const decimal&)> func,bool decreasing=false,int iterations=120,const decimal& min_x=d_layer_max.neg(),const decimal& max_x=d_layer_max,const decimal& min_y=d_layer_max.neg(),const decimal& max_y=d_layer_max) {
		return [func,decreasing,iterations,min_x,max_x,min_y,max_y](const decimal& value)->decimal{
			decimal input=value;
			decimal minX=min_x;
			decimal maxX=max_x;
			decimal minY=min_y;
			decimal maxY=max_y;
			if (input.is_nan() || maxX<minX || input<minY || input>maxY) return d_nan;
			std::function<decimal(const decimal&)> range_apply=[](const decimal& value)->decimal{ return value; };
			bool current_check=true;
			if (maxX<0) current_check=false;
			else if (minX>0) current_check=true;
			else {
				decimal val_check=func(d_zero);
				if (val_check==input) return d_zero;
				current_check=input>val_check;
				if (decreasing) current_check=!current_check;
			}
			bool positive=current_check;
			bool reciprocal=false;
			if (current_check) {
				if (maxX<first_neg_layer) current_check=true;
				else if (minX>first_neg_layer) current_check=false;
				else {
					decimal val_check=func(decimal(first_neg_layer));
					current_check=input<val_check;
					if (decreasing) current_check=!current_check;
				}
				if (current_check) {
					reciprocal=true;
					decimal limit=decimal(10).pow(exp_limit).recip();
					if (maxX<limit) current_check=false;
					else if (minX>limit) current_check=true;
					else {
						decimal val_check=func(limit);
						current_check=input>val_check;
						if (decreasing) current_check=!current_check;
					}
					if (current_check) range_apply=[](const decimal& value)->decimal{ return decimal(10).pow(value).recip(); };
					else {
						decimal limit2=decimal(10).tetrate(exp_limit);
						if (maxX<limit2) current_check=false;
						else if (minX>limit2) current_check=true;
						else {
							decimal val_check=func(limit2);
							current_check=input>val_check;
							if (decreasing) current_check=!current_check;
						}
						if (current_check) range_apply=[](const decimal& value)->decimal{ return decimal(10).tetrate(value.to_number()).recip(); };
						else range_apply=[](const decimal& value)->decimal{ return (value>std::log10(std::numeric_limits<double>::max()))?d_zero:(decimal(10).tetrate(decimal(10).pow(value).to_number()).recip()); };
					}
				} else {
					reciprocal=false;
					if (maxX<exp_limit) current_check=true;
					else if (minX>exp_limit) current_check=false;
					else {
						decimal val_check=func(decimal(exp_limit));
						current_check=input<val_check;
						if (decreasing) current_check=!current_check;
					}
					if (current_check) range_apply=[](const decimal& value)->decimal{ return value; };
					else {
						decimal limit2=decimal(10).pow(exp_limit);
						if (maxX<limit2) current_check=true;
						else if (minX>limit2) current_check=false;
						else {
							decimal val_check=func(limit2);
							current_check=input<val_check;
							if (decreasing) current_check=!current_check;
						}
						if (current_check) range_apply=[](const decimal& value)->decimal{ return decimal(10).pow(value); };
						else {
							decimal limit3=decimal(10).tetrate(exp_limit);
							if (maxX<limit3) current_check=true;
							else if (minX>limit3) current_check=false;
							else {
								decimal val_check=func(limit3);
								current_check=input<val_check;
								if (decreasing) current_check=!current_check;
							}
							if (current_check) range_apply=[](const decimal& value)->decimal{ return decimal(10).tetrate(value.to_number()); };
							else range_apply=[](const decimal& value)->decimal{ return (value>std::log10(std::numeric_limits<double>::max()))?d_inf:(decimal(10).tetrate(decimal(10).pow(value).to_number())); };
						}
					}
				}
			} else {
				reciprocal=true;
				if (maxX<-first_neg_layer) current_check=false;//decimal(-first_neg_layer)?
				else if (minX>-first_neg_layer) current_check=true;
				else {
					decimal val_check=func(decimal(-first_neg_layer));
					current_check=input>val_check;
					if (decreasing) current_check=!current_check;
				}
				if (current_check) {
					decimal limit=decimal(10).pow(exp_limit).recip().neg();
					if (maxX<limit) current_check=true;
					else if (minX>limit) current_check=false;
					else {
						decimal val_check=func(limit);
						current_check=input<val_check;
						if (decreasing) current_check=!current_check;
					}
					if (current_check) range_apply=[](const decimal& value)->decimal{ return decimal(10).pow(value).recip().neg(); };
					else {
						decimal limit2=decimal(10).tetrate(exp_limit).neg();
						if (maxX<limit2) current_check=true;
						else if (minX>limit2) current_check=false;
						else {
							decimal val_check=func(limit2);
							current_check=input<val_check;
							if (decreasing) current_check=!current_check;
						}
						if (current_check) range_apply=[](const decimal& value)->decimal{ return decimal(10).tetrate(value.to_number()).recip().neg(); };
						else range_apply=[](const decimal& value)->decimal{ return (value>std::log10(std::numeric_limits<double>::max()))?d_zero:(decimal(10).tetrate(decimal(10).pow(value).to_number()).recip().neg()); };
					}
				} else {
					reciprocal=false;
					if (maxX<-exp_limit) current_check=false;//decimal(-exp_limit)?
					else if (minX>-exp_limit) current_check=true;
					else {
						decimal val_check=func(decimal(-exp_limit));
						current_check=input>val_check;
						if (decreasing) current_check=!current_check;
					}
					if (current_check) range_apply=[](const decimal& value)->decimal{ return value.neg(); };
					else {
						decimal limit2=decimal(10).pow(exp_limit).neg();
						if (maxX<limit2) current_check=false;
						else if (minX>limit2) current_check=true;
						else {
							decimal val_check=func(limit2);
							current_check=input>val_check;
							if (decreasing) current_check=!current_check;
						}
						if (current_check) range_apply=[](const decimal& value)->decimal{ return decimal(10).pow(value).neg(); };
						else {
							decimal limit3=decimal(10).tetrate(exp_limit).neg();
							if (maxX<limit3) current_check=false;
							else if (minX>limit3) current_check=true;
							else {
								decimal val_check=func(limit3);
								current_check=input>val_check;
								if (decreasing) current_check=!current_check;
							}
							if (current_check) range_apply=[](const decimal& value)->decimal{ return decimal(10).tetrate(value.to_number()).neg(); };
							else range_apply=[](const decimal& value)->decimal{ return (value>std::log10(std::numeric_limits<double>::max()))?d_neg_inf:(decimal(10).tetrate(decimal(10).pow(value).to_number()).neg()); };
						}
					}
				}
			}
			bool search_increasing=positive!=reciprocal!=decreasing;
			auto comparative=search_increasing?[](const decimal& lhs,const decimal& rhs)->bool{ return lhs>rhs; }:[](const decimal& lhs,const decimal& rhs)->bool{ return lhs<rhs; };
			double step_size=0.001;
			bool has_changed_directions_once=false;
			bool previously_rose=false;
			double result=1;
			decimal applied_result=d_one;
			double old_result=0;
			bool critical=false;
			for (int i=1;i<iterations;i++) {
				critical=false;
				old_result=result;
				applied_result=range_apply(decimal(result));
				if (applied_result>maxX) {
					applied_result=maxX;
					critical=true;
				}
				if (applied_result<minX) {
					applied_result=minX;
					critical=true;
 				}
				decimal new_decimal=func(applied_result);
				if (new_decimal==input && !critical) break;
				bool currently_rose=comparative(new_decimal,input);
				if (i>1 && previously_rose!=currently_rose) has_changed_directions_once=true;
				previously_rose=currently_rose; 
				if (has_changed_directions_once) step_size/=2;
				else step_size*=2;
				if (currently_rose!=search_increasing && applied_result==maxX || currently_rose==search_increasing && applied_result==minX) return d_nan;
				step_size=std::abs(step_size)*(currently_rose?-1:1);
				result+=step_size;           
				if (step_size==0 || old_result==result) break;
			}
			return range_apply(decimal(result));
		};
	}
	
	static const decimal d_zero;
	static const decimal d_one;
	static const decimal d_neg_one;
	static const decimal d_two;
	static const decimal d_ten;
	static const decimal d_nan;
	static const decimal d_inf;
	static const decimal d_neg_inf;
	static const decimal d_number_max;
	static const decimal d_number_min;
	static const decimal d_layer_safe_max;
	static const decimal d_layer_safe_min;
	static const decimal d_layer_max;
	static const decimal d_layer_min;
/*
	static double afford_geometric_series(const decimal& resources_available,const decimal& price_start,const decimal& price_ratio,double current_owned) {
		decimal actual_start=price_start*price_ratio.pow(current_owned);
		return std::floor(((resources_available/actual_start*(price_ratio-1)+1).log10()/price_ratio.log10()).to_number());
	}
	static decimal sum_geometric_series(double num_items,const decimal& price_start,const decimal& price_ratio,double current_owned) {
		return price_start*price_ratio.pow(current_owned)*(decimal(1)-price_ratio.pow(num_items))/(decimal(1)-price_ratio);
	}
	static double afford_arithmetic_series(const decimal& resources_available,const decimal& price_start,const decimal& price_add,const decimal& current_owned) {
		// n = (-(a-d/2) + sqrt((a-d/2)^2+2dS))/d
		decimal actual_start=price_start+current_owned*price_add;
		decimal b=actual_start-price_add/2;
		decimal b2=b.pow(2);
		return (((-b)+(b2+(price_add*resources_available*2).sqrt()))/price_add).floor().to_number();
	}
	static decimal sum_arithmetic_series(const decimal& num_items,const decimal& price_start,const decimal& price_add,const decimal& current_owned) {
		decimal actual_start=price_start+current_owned*price_add;
		// (n/2)*(2*a+(n-1)*d)
		return num_items/2*(actual_start*2+((num_items-1)*price_add));
	}
	static decimal efficiency_of_purchase(const decimal& cost,const decimal& current_rps,const decimal& delta_rps) {
		return cost/current_rps+cost/delta_rps;
	}
	static decimal random_decimal_for_testing(int max_layers) {
		if (rand()%20<1) return d_zero;
		int random_sign=(rand()>RAND_MAX/2)?1:-1;
        if (rand()%20<1) return from_components_no_normalize(random_sign,0,1);
		int layer=rand()%(max_layers+1);
		double random_exp=(layer==0)?(rand()%616-308+(rand()/(double)RAND_MAX)):(rand()%16+(rand()/(double)RAND_MAX));
		if (rand()>RAND_MAX*0.9) random_exp=std::trunc(random_exp);
		double random_mag=std::pow(10,random_exp);
		if (rand()>RAND_MAX*0.9) random_mag=std::trunc(random_mag);
		return from_components(random_sign,layer,random_mag);
	}
*/
};

const decimal decimal::d_zero=decimal::from_components_no_normalize(0,0,0);
const decimal decimal::d_one=decimal::from_components_no_normalize(1,0,1);
const decimal decimal::d_neg_one=decimal::from_components_no_normalize(-1,0,1);
const decimal decimal::d_two=decimal::from_components_no_normalize(1,0,2);
const decimal decimal::d_ten=decimal::from_components_no_normalize(1,0,10);
const decimal decimal::d_nan=decimal::from_components_no_normalize(0/*std::numeric_limits<double>::quiet_NaN()*/,std::numeric_limits<double>::quiet_NaN(),std::numeric_limits<double>::quiet_NaN());
const decimal decimal::d_inf=decimal::from_components_no_normalize(1,std::numeric_limits<double>::infinity(),std::numeric_limits<double>::infinity());
const decimal decimal::d_neg_inf=decimal::from_components_no_normalize(-1,std::numeric_limits<double>::infinity(),std::numeric_limits<double>::infinity());
const decimal decimal::d_number_max=decimal::from_components(1,0,std::numeric_limits<double>::max());
const decimal decimal::d_number_min=decimal::from_components(1,0,std::numeric_limits<double>::min());
const decimal decimal::d_layer_safe_max=decimal::from_components(1,std::numeric_limits<double>::max(),decimal::exp_limit-1);
const decimal decimal::d_layer_safe_min=decimal::from_components(1,std::numeric_limits<double>::max(),-(decimal::exp_limit-1));
const decimal decimal::d_layer_max=decimal::from_components(1,std::numeric_limits<double>::max(),decimal::exp_limit-1);
const decimal decimal::d_layer_min=decimal::from_components(1,std::numeric_limits<double>::max(),-(decimal::exp_limit-1));

}

}

#endif