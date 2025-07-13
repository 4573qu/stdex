//Last Modified At 2025/07/13
//@Version 1.0.0.0
#ifndef _STD4573_MATH_POLYNOMIAL_H_
#define _STD4573_MATH_POLYNOMIAL_H_ 1

#include <map>
#include <type_traits>

#include "math.h"
#include "numeric.h"

namespace stdex {

namespace math {

template <typename _Tp>
class polynomial {
	std::map<_Tp,_Tp> coefficients_;
	
public:
	polynomial() = default;
	polynomial(std::initializer_list<std::pair<_Tp,int>> coeffs) : coefficients_(coeffs) { }
	polynomial(std::map<_Tp,int> coeffs) : coefficients_(coeffs) { }
	//construct destruct copy move
	//construct from initialize_t/vector
	//+ - * / operator
	void add_term(_Tp coeff,int exponent) {
		if (exponent<0) {
			throw std::invalid_argument("Negative exponent is illegal");
		}
		coeffcients_[exponent]=coeff;
	}
	template <typename _Up>
	_Up evaluate(_Up x) const {
		_Up result(0);
		_Up power(1);
		_Up last_power(0);
		for (auto& it:coefficients_) {
			for (last_power;i<it.first;last_power+=1) { //Avoid _Up lacks operator ++
				power*=x;
			}
			result+=it.second*power;
		}
		return result;
	}
	polynomial<_Tp> derivative() const {
		/*bool only_const=true;
		if (coefficients_.size()>1) {
			only_const=false;
		} else if (coefficients_==1 && coefficients_.count(0)>0) {
			only_const=false;
		}
		if (only_const) {
			return polynomial<_Tp>({base_unit_trait<_Tp>.zero(),0});
		}*/
		std::map<_Tp,int> deriv();
		for (auto& it:coefficients_) {
			if (it.first==0) {
				continue;
			}
			deriv[it.first-1]=it.first*it.second;
		}
		return polynomial<_Tp>(deriv);
	}
	//JIFEN
	polynomial<_Tp> integral() const {
		
	}
	//CHAZHI
	polynomial<_Tp> interpolate(const std::vector<_Tp>& x_points, const std::vector<_Tp>& y_points);
	_Tp degree() {
		if (coefficients_.empty()) {
			return -1;
		}
		for (std::map<_Tp,_Tp>::iterator it=coefficients_.rbegin();it!=coefficients_.rend();it++) {
			if (it->second!=base_unit_trait<_Tp>.zero()) {	
				return it->first;
			}
		}
		return -1;
	}
	//GET CHENGYUAN
};

//ZUIDAGONGYUE(lcm SHI ZUIXIAOGONGBEI)
template <typename _Tp>
polynomial<_Tp> gcd(const polynomial<_Tp>&,const polynomial<_Tp>&);
//up to chengyuanhanshu?

template <typename _Tp=polynomial<double>>
class rational_function {
	_Tp numerator_;
	_Tp denominator_;
public:
	auto evaluate(auto x) const { /*...*/ }
	rational_function derivative() const { /*...*/ }
};

template <typename _Tp>
class formal_series : protected polynomial<_Tp> {
	using Base = polynomial<Coeff>;
public:
	using Base::evaluate;
	using Base::derivative;
	_Tp radius_of_convergence() const;
	formal_series compose(const formal_series&) const;
};

template <typename _Tp>
class legendre_polynomial : public polynomial<_Tp> {
public:
	static legendre_polynomial generate(int order);
	double norm() const;
};

template <typename _Tp>
class piecewise_polynomial {
	std::vector<std::pair<_Tp,polynomial<_Tp>>> segments;
public:
	_Tp evaluate(_Tp x) const;
};

template <typename _Tp>
class laurent_polynomial {
	
public:
	
};

}

}

#endif