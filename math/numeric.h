//Last Modified At 2025/07/09
//@Version 1.0.0.0
#ifndef _STD4573_MATH_NUMERIC_H_
#define _STD4573_MATH_NUMERIC_H_ 1

#include <cstddef>
#include <type_traits>
#include <vector>

#include "math.h"

namespace stdex {

namespace math {
	
class bigint {
public:
	static const int base_=1000000000;
	std::vector<int> digits_;
	bool negative_;

private:
	void normalize();
	static bigint multiply(const bigint& lhs,const bigint& rhs);
	
public:
	bigint();
	bigint(long long num);
	bigint(const std::string& s);
	
	bigint(const bigint& other);
	bigint(bigint&& other) noexcept;
	
	bigint& operator =(const bigint& other);
	bigint& operator =(bigint&& other) noexcept;
	
	bigint operator -() const;

	bigint operator +(const bigint& other) const;
	bigint& operator +=(const bigint& other);
	bigint operator -(const bigint& other) const;
	bigint& operator -=(const bigint& other);
	bigint operator *(const bigint& other) const;
	bigint& operator *=(const bigint& other);
	bigint operator /(const bigint& other) const;
	bigint& operator /=(const bigint& other);
	bigint operator %(const bigint& other) const;
	bigint& operator %=(const bigint& other);
	
	bool operator ==(const bigint& other) const;
	bool operator !=(const bigint& other) const;
	bool operator <(const bigint& other) const;
	bool operator <=(const bigint& other) const;
	bool operator >(const bigint& other) const;
	bool operator >=(const bigint& other) const;
	
	bigint operator <<(size_t n) const;
	bigint& operator <<=(size_t n);
	
	bool zero() const;
	bigint abs() const;
	bigint subnum(size_t start,size_t end) const;
	
	std::string to_string() const;
	explicit operator long long() const;
	explicit operator int() const;
	explicit operator double() const;
};

class rational {
	bigint numerator_;
	bigint denominator_;
	
public:
	bool auto_reduce_;
	void reduce();
	
private:
	static bigint gcd(bigint lhs,bigint rhs);

public:
	rational();
	rational(long long num);
	rational(long long num,long long den);
	rational(const bigint& num, const bigint& den);
	rational(const std::string& s);
    
	rational(const rational&) = default;
	rational(rational&&) = default;

	rational& operator =(const rational&) = default;
	rational& operator =(rational&&) = default;

	rational operator -() const;

	rational operator +(const rational& other) const;
	rational& operator +=(const rational& other);
	rational operator -(const rational& other) const;
	rational& operator -=(const rational& other);
	rational operator *(const rational& other) const;
	rational& operator *=(const rational& other);
	rational operator /(const rational& other) const;
	rational& operator /=(const rational& other);

	bool operator ==(const rational& other) const;
	bool operator !=(const rational& other) const;
	bool operator <(const rational& other) const;
	bool operator <=(const rational& other) const;
	bool operator >(const rational& other) const;
	bool operator >=(const rational& other) const;
	
	const bigint& num() const;
	const bigint& den() const;
	
	std::string to_string() const;
	explicit operator double() const;
	explicit operator float() const;
};

class multibase {
	double base_;
	double value_;
	int precision_;
	static double epsilon_;
	double parse(const std::string& s);

public:
	multibase();
	multibase(double base,double value,int precision=6);
	multibase(double base,const std::string& s,int precision=6);
	
	multibase(const multibase&) = default;
	multibase(multibase&&) = default;

	multibase& operator =(const multibase&) = default;
	multibase& operator =(multibase&&) = default;
    
	multibase& operator =(int value);
	multibase& operator =(double value);
	multibase& operator =(const bigint& value);
	multibase& operator =(const rational& value);
	
	void assign(const std::string& s);
	
	multibase operator -() const;

	multibase operator +(const multibase& other) const;
	multibase& operator +=(const multibase& other);
	multibase operator -(const multibase& other) const;
	multibase& operator -=(const multibase& other);
	multibase operator *(const multibase& other) const;
	multibase& operator *=(const multibase& other);
	multibase operator /(const multibase& other) const;
	multibase& operator /=(const multibase& other);

	bool operator ==(const multibase& other) const;
	bool operator !=(const multibase& other) const;
	bool operator <(const multibase& other) const;
	bool operator <=(const multibase& other) const;
	bool operator >(const multibase& other) const;
	bool operator >=(const multibase& other) const;

	friend std::ostream& operator <<(std::ostream& os,const multibase& num);

	double base() const;
	int precision();
	static double epsilon();
	void base(double base);
	void precision(int precision);
	static void epsilon(double epsilon);

	double to_double() const;
	std::string to_string() const;

	explicit operator int() const;
	explicit operator double() const;
	explicit operator bigint() const;
	explicit operator rational() const;
};

template <typename _Tp>
struct is_complex_arithmetic : std::is_arithmetic<_Tp> {};
template <>
struct is_complex_arithmetic<bigint> : std::true_type {};
template <>
struct is_complex_arithmetic<rational> : std::true_type {};

template <typename _Tp>
class complex {
	_Tp real_;
	_Tp imag_;

	template <typename _Up>
	using enable_if_arithmetic=std::enable_if_t<is_complex_arithmetic<_Up>::value>;
	template <typename _Up>
	using enable_if_floating=std::enable_if_t<std::is_floating_point<_Up>::value,int>;
	//template <typename _Up>
	//using enable_if_complex=typename std::enable_if<std::is_same<_Up,complex<typename _Up::value_type>>::value>::type*;
	//template <typename _Func>
	//using enable_if_callable=decltype(std::declval<_Func>()(std::declval<_Tp>()),int>;

public:
	constexpr complex(const _Tp& real=_Tp(),const _Tp& imag=_Tp()) noexcept;
	template <typename _Up>
	constexpr complex(const complex<_Up>& other) noexcept;
	template <typename _Up,typename=enable_if_arithmetic<_Up>>
	constexpr complex(const _Up& real) noexcept;

	complex(const complex&) = default;
	complex(complex&&) = default;

	complex<_Tp>& operator =(const complex<_Tp>&) = default;
	complex<_Tp>& operator =(complex<_Tp>&&) = default;

	complex<_Tp>& operator =(const _Tp& real);
	template <typename _Up>
	complex<_Tp>& operator =(const complex<_Up>& other);

	constexpr complex<_Tp> operator +() const noexcept;
	constexpr complex<_Tp> operator -() const noexcept;

	complex<_Tp>& operator +=(const complex<_Tp>& other) noexcept;
	template <typename _Up>
	complex<_Tp>& operator +=(const complex<_Up>& other) noexcept;
	template <typename _Up,typename=enable_if_arithmetic<_Up>>
	complex<_Tp>& operator +=(const _Up& scalar) noexcept;
	complex<_Tp>& operator -=(const complex<_Tp>& other) noexcept;
	template <typename _Up>
	complex<_Tp>& operator -=(const complex<_Up>& other) noexcept;
	template <typename _Up,typename=enable_if_arithmetic<_Up>>
	complex<_Tp>& operator -=(const _Up& scalar) noexcept;
	complex<_Tp>& operator *=(const complex<_Tp>& other) noexcept;
	template <typename _Up>
	complex<_Tp>& operator *=(const complex<_Up>& other) noexcept;
	template <typename _Up,typename=enable_if_arithmetic<_Up>>
	complex<_Tp>& operator *=(const _Up& scalar) noexcept;
	complex<_Tp>& operator /=(const complex<_Tp>& other) noexcept;
	template <typename _Up>
	complex<_Tp>& operator /=(const complex<_Up>& other) noexcept;
	template <typename _Up,typename=enable_if_arithmetic<_Up>>
	complex<_Tp>& operator /=(const _Up& scalar) noexcept;
    
	constexpr complex<_Tp> conj() const noexcept;
	template <typename _Up=_Tp,typename=enable_if_arithmetic<_Up>>
	constexpr _Tp norm() const noexcept;
	template <typename _Up=_Tp,typename=enable_if_floating<_Up>>
	_Tp abs() const noexcept;
	template <typename _Up=_Tp,typename=enable_if_floating<_Up>>
	_Tp arg() const noexcept;
	template <typename _Up=_Tp,typename=enable_if_floating<_Up>>
	complex<_Tp> proj() const noexcept;
	template <typename _Up=_Tp,typename=enable_if_floating<_Up>>
	static complex<_Tp> polar(const _Tp& r,const _Tp& theta=_Tp()) noexcept;

	template <typename _Up=_Tp,typename=enable_if_floating<_Up>>
	complex<_Tp> exp() const noexcept;
	template <typename _Up=_Tp,typename=enable_if_floating<_Up>>
	complex<_Tp> log() const noexcept;
	template <typename _Up=_Tp,typename=enable_if_floating<_Up>>
	complex<_Tp> pow(const complex<_Tp>& exponent) const;
	template <typename _Up=_Tp,typename=enable_if_floating<_Up>>
	complex sqrt() const noexcept;

	template <typename _Up=_Tp,typename=enable_if_floating<_Up>>
	complex<_Tp> sin() const noexcept;
	template <typename _Up=_Tp,typename=enable_if_floating<_Up>>
	complex<_Tp> cos() const noexcept;
	template <typename _Up=_Tp,typename=enable_if_floating<_Up>>
	complex<_Tp> tan() const;

	template <typename _Up=_Tp,typename=enable_if_floating<_Up>>
	complex<_Tp> sinh() const noexcept;
	template <typename _Up=_Tp,typename=enable_if_floating<_Up>>
	complex<_Tp> cosh() const noexcept;
	template <typename _Up=_Tp,typename=enable_if_floating<_Up>>
	complex<_Tp> tanh() const;

	const _Tp& real() const;
	const _Tp& imag() const;
	
	std::string to_string() const;
};

template <typename _Tp>
constexpr complex<_Tp> operator +(const complex<_Tp>& lhs,const complex<_Tp>& rhs) noexcept {
	return complex<_Tp>(lhs)+=rhs;
}
template <typename _Tp,typename _Up>
constexpr auto operator +(const complex<_Tp>& lhs,const complex<_Up>& rhs) noexcept {
	using R=decltype(lhs.real()+rhs.real());
	return complex<R>(lhs)+=rhs;
}
template <typename _Tp,typename _Up,typename=typename is_complex_arithmetic<_Up>::type>
constexpr auto operator +(const complex<_Tp>& lhs,const _Up& rhs) noexcept {
	return complex<_Tp>(lhs)+=rhs;
}
template <typename _Tp,typename _Up,typename=typename is_complex_arithmetic<_Up>::type>
constexpr auto operator +(const _Tp& lhs,const complex<_Up>& rhs) noexcept {
	return complex<_Up>(lhs)+=rhs;
}

template <typename _Tp>
constexpr complex<_Tp> operator -(const complex<_Tp>& lhs,const complex<_Tp>& rhs) noexcept {
	return complex<_Tp>(lhs)-=rhs;
}
template <typename _Tp,typename _Up>
constexpr auto operator -(const complex<_Tp>& lhs,const complex<_Up>& rhs) noexcept {
	using R=decltype(lhs.real()-rhs.real());
	return complex<R>(lhs)-=rhs;
}
template <typename _Tp,typename _Up,typename=typename is_complex_arithmetic<_Up>::type>
constexpr auto operator -(const complex<_Tp>& lhs,const _Up& rhs) noexcept {
	return complex<_Tp>(lhs)-=rhs;
}
template <typename _Tp,typename _Up,typename=typename is_complex_arithmetic<_Up>::type>
constexpr auto operator -(const _Tp& lhs,const complex<_Up>& rhs) noexcept {
	return complex<_Up>(lhs)-=rhs;
}

template <typename _Tp>
constexpr complex<_Tp> operator *(const complex<_Tp>& lhs,const complex<_Tp>& rhs) noexcept {
	return complex<_Tp>(lhs)*=rhs;
}
template <typename _Tp,typename _Up>
constexpr auto operator *(const complex<_Tp>& lhs,const complex<_Up>& rhs) noexcept {
	using R=decltype(lhs.real()*rhs.real());
	return complex<R>(lhs)*=rhs;
}
template <typename _Tp,typename _Up,typename=typename is_complex_arithmetic<_Up>::type>
constexpr auto operator *(const complex<_Tp>& lhs,const _Up& rhs) noexcept {
	return complex<_Tp>(lhs)*=rhs;
}
template <typename _Tp,typename _Up,typename=typename is_complex_arithmetic<_Up>::type>
constexpr auto operator *(const _Tp& lhs,const complex<_Up>& rhs) noexcept {
	return complex<_Up>(lhs)*=rhs;
}

template <typename _Tp>
constexpr complex<_Tp> operator /(const complex<_Tp>& lhs,const complex<_Tp>& rhs) noexcept {
	return complex<_Tp>(lhs)/=rhs;
}
template <typename _Tp,typename _Up>
constexpr auto operator /(const complex<_Tp>& lhs,const complex<_Up>& rhs) noexcept {
	using R=decltype(lhs.real()*rhs.real());
	return complex<R>(lhs)/=rhs;
}
template <typename _Tp,typename _Up,typename=typename is_complex_arithmetic<_Up>::type>
constexpr auto operator /(const complex<_Tp>& lhs,const _Up& rhs) noexcept {
	return complex<_Tp>(lhs)/=rhs;
}
template <typename _Tp,typename _Up,typename=typename is_complex_arithmetic<_Up>::type>
constexpr auto operator /(const _Tp& lhs,const complex<_Up>& rhs) noexcept {
	return complex<_Up>(lhs)/=rhs;
}

template <typename _Tp>
bool operator ==(const complex<_Tp>& lhs,const complex<_Tp>& rhs) noexcept {
	return lhs.real()==rhs.real() && lhs.imag()==rhs.imag();
}
template <typename _Tp,typename _Up>
bool operator ==(const complex<_Tp>& lhs,const complex<_Up>& rhs) noexcept {
	return lhs.real()==rhs.real() && lhs.imag()==rhs.imag();
}
template <typename _Tp,typename _Up,typename=typename is_complex_arithmetic<_Up>::type>
bool operator ==(const complex<_Tp>& lhs,const _Up& rhs) noexcept {
	return lhs.real()==rhs && lhs.imag()==0;
}
template <typename _Tp,typename _Up,typename=typename is_complex_arithmetic<_Up>::type>
bool operator ==(const _Tp& lhs,const complex<_Up>& rhs) noexcept {
	return lhs==rhs.real() && 0==rhs.imag();
}

template <typename _Tp>
bool operator !=(const complex<_Tp>& lhs,const complex<_Tp>& rhs) noexcept {
	return !(lhs==rhs);
}
template <typename _Tp,typename _Up>
bool operator !=(const complex<_Tp>& lhs,const complex<_Up>& rhs) noexcept {
	return !(lhs==rhs);
}
template <typename _Tp,typename _Up,typename=typename is_complex_arithmetic<_Up>::type>
bool operator !=(const complex<_Tp>& lhs,const _Up& rhs) noexcept {
	return !(lhs==rhs);
}
template <typename _Tp,typename _Up,typename=typename is_complex_arithmetic<_Up>::type>
bool operator !=(const _Tp& lhs,const complex<_Up>& rhs) noexcept {
	return !(lhs==rhs);
}

template <typename _Tp>
std::ostream& operator <<(std::ostream& os,const complex<_Tp>& c) {
	os<<c.to_string();
	return os;
}

template <typename _Tp>
class quaternion {	
	_Tp w_,x_,y_,z_;

	template <typename _Up>
	using enable_if_arithmetic=std::enable_if_t<is_complex_arithmetic<_Up>::value>;
	template <typename _Up>
	using enable_if_floating=std::enable_if_t<std::is_floating_point<_Up>::value>;

public:
	constexpr quaternion(const _Tp& w=_Tp(),const _Tp& x=_Tp(),const _Tp& y=_Tp(),const _Tp& z=_Tp()) noexcept;
	constexpr quaternion(const _Tp& real) noexcept;
	template <typename _Up>
	constexpr quaternion(const complex<_Up>& c) noexcept;

	quaternion(const quaternion&) = default;
	quaternion(quaternion&&) = default;

	quaternion<_Tp>& operator =(const quaternion<_Tp>&) = default;
	quaternion<_Tp>& operator =(quaternion<_Tp>&&) = default;

	quaternion<_Tp>& operator =(const _Tp& real);
	template <typename _Up>
	quaternion<_Tp>& operator =(const complex<_Up>& other);
	template <typename _Up>
	quaternion<_Tp>& operator =(const quaternion<_Up>& other);

	constexpr quaternion<_Tp> operator +() const noexcept;
	constexpr quaternion<_Tp> operator -() const noexcept;

	quaternion<_Tp>& operator +=(const quaternion<_Tp>& other) noexcept;
	template <typename _Up>
	quaternion<_Tp>& operator +=(const quaternion<_Up>& other) noexcept;
	template <typename _Up,typename=enable_if_arithmetic<_Up>>
	quaternion<_Tp>& operator +=(const _Up& scalar) noexcept;
	template <typename _Up,typename=enable_if_arithmetic<_Up>>
	quaternion<_Tp>& operator +=(const complex<_Up>& complexor) noexcept;
	quaternion<_Tp>& operator -=(const quaternion<_Tp>& other) noexcept;
	template <typename _Up>
	quaternion<_Tp>& operator -=(const quaternion<_Up>& other) noexcept;
	template <typename _Up,typename=enable_if_arithmetic<_Up>>
	quaternion<_Tp>& operator -=(const _Up& scalar) noexcept;
	template <typename _Up,typename=enable_if_arithmetic<_Up>>
	quaternion<_Tp>& operator -=(const complex<_Up>& complexor) noexcept;
	quaternion<_Tp>& operator *=(const quaternion<_Tp>& other) noexcept;
	template <typename _Up>
	quaternion<_Tp>& operator *=(const quaternion<_Up>& other) noexcept;
	template <typename _Up,typename=enable_if_arithmetic<_Up>>
	quaternion<_Tp>& operator *=(const _Up& scalar) noexcept;
	template <typename _Up,typename=enable_if_arithmetic<_Up>>
	quaternion<_Tp>& operator *=(const complex<_Up>& complexor) noexcept;
	quaternion<_Tp>& operator /=(const quaternion<_Tp>& other) noexcept;
	template <typename _Up>
	quaternion<_Tp>& operator /=(const quaternion<_Up>& other) noexcept;
	template <typename _Up,typename=enable_if_arithmetic<_Up>>
	quaternion<_Tp>& operator /=(const _Up& scalar) noexcept;
	template <typename _Up,typename=enable_if_arithmetic<_Up>>
	quaternion<_Tp>& operator /=(const complex<_Up>& complexor) noexcept;

	constexpr quaternion<_Tp> conj() const noexcept;
	template <typename _Up,typename=enable_if_arithmetic<_Up>>
	constexpr _Tp norm_sq() const noexcept;
	template <typename _Up=_Tp,typename=enable_if_floating<_Up>>
	_Tp norm() const noexcept;
	template <typename _Up,typename=enable_if_arithmetic<_Up>>
	quaternion<_Tp> inverse() const;
	template <typename _Up=_Tp,typename=enable_if_floating<_Up>>
	quaternion<_Tp> unit() const;
	_Tp dot(const quaternion<_Tp>& other) const noexcept;
	template <typename _Up>
	_Tp dot(const quaternion<_Up>& other) const noexcept;

	template <typename _Up=_Tp,typename=enable_if_floating<_Up>>
	static quaternion<_Tp> rotation(const _Tp& angle,const _Tp& ax,const _Tp& ay,const _Tp& az) noexcept;
	template <typename _Up=_Tp,typename=enable_if_floating<_Up>>
	static quaternion<_Tp> rotation(const _Tp& angle,const complex<_Tp>& axis) noexcept;
	template <typename _Up,typename=enable_if_arithmetic<_Up>>
	quaternion<_Tp> rotate_vector(const _Tp& vx,const _Tp& vy,const _Tp& vz) const;
	template <typename _Up,typename=enable_if_arithmetic<_Up>>
	complex<_Tp> rotate_complex(const complex<_Tp>& c) const;

	template <typename _Up=_Tp,typename=enable_if_floating<_Up>>
	quaternion<_Tp> exp() const noexcept;
	template <typename _Up=_Tp,typename=enable_if_floating<_Up>>
	quaternion<_Tp> log() const noexcept;
	template <typename _Up=_Tp,typename=enable_if_floating<_Up>>
	quaternion<_Tp> pow(const _Tp& exponent) const;

	const _Tp& w() const noexcept;
	const _Tp& x() const noexcept;
	const _Tp& y() const noexcept;
	const _Tp& z() const noexcept;
	const _Tp& real() const noexcept;
	const _Tp& imag() const noexcept;
	const complex<_Tp> complex() const noexcept;

	std::string to_string() const;
};

template <typename _Tp>
constexpr quaternion<_Tp> operator +(const quaternion<_Tp>& lhs,const quaternion<_Tp>& rhs) noexcept {
    return quaternion<_Tp>(lhs)+=rhs;
}
template <typename _Tp,typename _Up>
constexpr quaternion<_Tp> operator +(const quaternion<_Tp>& lhs,const quaternion<_Up>& rhs) noexcept {
    using R=decltype(lhs.w()+rhs.w());
    return quaternion<R>(lhs)+rhs;
}
template <typename _Tp,typename _Up,typename=typename is_complex_arithmetic<_Up>::type>
constexpr auto operator +(const quaternion<_Tp>& lhs,const _Up& rhs) noexcept {
	return quaternion<_Tp>(lhs)+=rhs;
}
template <typename _Tp,typename _Up,typename=typename is_complex_arithmetic<_Up>::type>
constexpr auto operator +(const _Tp& lhs,const quaternion<_Up>& rhs) noexcept {
	return qauternion<_Up>(lhs)+=rhs;
}
template <typename _Tp,typename _Up,typename=typename is_complex_arithmetic<_Up>::type>
constexpr auto operator +(const quaternion<_Tp>& lhs,const complex<_Up>& rhs) noexcept {
	return quaternion<_Tp>(lhs)+=rhs;
}
template <typename _Tp,typename _Up,typename=typename is_complex_arithmetic<_Up>::type>
constexpr auto operator +(const complex<_Tp>& lhs,const quaternion<_Up>& rhs) noexcept {
	return qauternion<_Up>(lhs)+=rhs;
}

template <typename _Tp>
constexpr quaternion<_Tp> operator -(const quaternion<_Tp>& lhs,const quaternion<_Tp>& rhs) noexcept {
    return quaternion<_Tp>(lhs)-=rhs;
}
template <typename _Tp,typename _Up>
constexpr auto operator -(const quaternion<_Tp>& lhs,const quaternion<_Up>& rhs) noexcept {
    using R=decltype(lhs.w()-rhs.w());
    return quaternion<R>(lhs)-rhs;
}
template <typename _Tp,typename _Up,typename=typename is_complex_arithmetic<_Up>::type>
constexpr auto operator -(const quaternion<_Tp>& lhs,const _Up& rhs) noexcept {
	return quaternion<_Tp>(lhs)-=rhs;
}
template <typename _Tp,typename _Up,typename=typename is_complex_arithmetic<_Up>::type>
constexpr auto operator -(const _Tp& lhs,const quaternion<_Up>& rhs) noexcept {
	return qauternion<_Up>(lhs)-=rhs;
}
template <typename _Tp,typename _Up,typename=typename is_complex_arithmetic<_Up>::type>
constexpr auto operator -(const quaternion<_Tp>& lhs,const complex<_Up>& rhs) noexcept {
	return quaternion<_Tp>(lhs)-=rhs;
}
template <typename _Tp,typename _Up,typename=typename is_complex_arithmetic<_Up>::type>
constexpr auto operator -(const complex<_Tp>& lhs,const quaternion<_Up>& rhs) noexcept {
	return qauternion<_Up>(lhs)-=rhs;
}

template <typename _Tp>
constexpr quaternion<_Tp> operator *(const quaternion<_Tp>& lhs,const quaternion<_Tp>& rhs) noexcept {
    return quaternion<_Tp>(lhs)*=rhs;
}
template <typename _Tp,typename _Up>
constexpr quaternion<_Tp> operator *(const quaternion<_Tp>& lhs,const quaternion<_Up>& rhs) noexcept {
    using R=decltype(lhs.w()*rhs.w());
    return quaternion<R>(lhs)*rhs;
}
template <typename _Tp,typename _Up,typename=typename is_complex_arithmetic<_Up>::type>
constexpr auto operator *(const quaternion<_Tp>& lhs,const _Up& rhs) noexcept {
	return quaternion<_Tp>(lhs)*=rhs;
}
template <typename _Tp,typename _Up,typename=typename is_complex_arithmetic<_Up>::type>
constexpr auto operator *(const _Tp& lhs,const quaternion<_Up>& rhs) noexcept {
	return qauternion<_Up>(lhs)*=rhs;
}
template <typename _Tp,typename _Up,typename=typename is_complex_arithmetic<_Up>::type>
constexpr auto operator *(const quaternion<_Tp>& lhs,const complex<_Up>& rhs) noexcept {
	return quaternion<_Tp>(lhs)*=rhs;
}
template <typename _Tp,typename _Up,typename=typename is_complex_arithmetic<_Up>::type>
constexpr auto operator *(const complex<_Tp>& lhs,const quaternion<_Up>& rhs) noexcept {
	return qauternion<_Up>(lhs)*=rhs;
}

template <typename _Tp>
constexpr quaternion<_Tp> operator /(const quaternion<_Tp>& lhs,const quaternion<_Tp>& rhs) noexcept {
    return quaternion<_Tp>(lhs)/=rhs;
}
template <typename _Tp,typename _Up>
constexpr quaternion<_Tp> operator /(const quaternion<_Tp>& lhs,const quaternion<_Up>& rhs) noexcept {
    using R=decltype(lhs.w()*rhs.w());
    return quaternion<R>(lhs)/rhs;
}
template <typename _Tp,typename _Up,typename=typename is_complex_arithmetic<_Up>::type>
constexpr auto operator /(const quaternion<_Tp>& lhs,const _Up& rhs) noexcept {
	return quaternion<_Tp>(lhs)/=rhs;
}
template <typename _Tp,typename _Up,typename=typename is_complex_arithmetic<_Up>::type>
constexpr auto operator /(const _Tp& lhs,const quaternion<_Up>& rhs) noexcept {
	return qauternion<_Up>(lhs)/=rhs;
}
template <typename _Tp,typename _Up,typename=typename is_complex_arithmetic<_Up>::type>
constexpr auto operator /(const quaternion<_Tp>& lhs,const complex<_Up>& rhs) noexcept {
	return quaternion<_Tp>(lhs)/=rhs;
}
template <typename _Tp,typename _Up,typename=typename is_complex_arithmetic<_Up>::type>
constexpr auto operator /(const complex<_Tp>& lhs,const quaternion<_Up>& rhs) noexcept {
	return qauternion<_Up>(lhs)/=rhs;
}

template <typename _Tp>
bool operator ==(const quaternion<_Tp>& lhs,const quaternion<_Tp>& rhs) noexcept {
	return lhs.w()==rhs.w() && lhs.x()==rhs.x() && lhs.y()==rhs.y() && lhs.z()==rhs.z();
}
template <typename _Tp,typename _Up>
bool operator ==(const quaternion<_Tp>& lhs,const quaternion<_Up>& rhs) noexcept {
	return lhs.w()==rhs.w() && lhs.x()==rhs.x() && lhs.y()==rhs.y() && lhs.z()==rhs.z();
}
template <typename _Tp,typename _Up,typename=typename is_complex_arithmetic<_Up>::type>
bool operator ==(const quaternion<_Tp>& lhs,const _Up& rhs) noexcept {
	return lhs.w()==rhs && lhs.x()==0 && lhs.y()==0 && lhs.z()==0;
}
template <typename _Tp,typename _Up,typename=typename is_complex_arithmetic<_Up>::type>
bool operator ==(const _Tp& lhs,const quaternion<_Up>& rhs) noexcept {
	return lhs==rhs.w() && 0==rhs.x() && 0==rhs.y() && 0==rhs.z();
}
template <typename _Tp,typename _Up,typename=typename is_complex_arithmetic<_Up>::type>
bool operator ==(const quaternion<_Tp>& lhs,const complex<_Up>& rhs) noexcept {
	return lhs.w()==rhs.real() && lhs.x()==rhs.imag() && lhs.y()==0 && lhs.z()==0;
}
template <typename _Tp,typename _Up,typename=typename is_complex_arithmetic<_Up>::type>
bool operator ==(const complex<_Tp>& lhs,const quaternion<_Up>& rhs) noexcept {
	return lhs.real()==rhs.w() && lhs.imag()==rhs.x() && 0==rhs.y() && 0==rhs.z();
}

template <typename _Tp>
bool operator !=(const quaternion<_Tp>& lhs,const quaternion<_Tp>& rhs) noexcept {
	return !(lhs==rhs);
}
template <typename _Tp,typename _Up>
bool operator !=(const quaternion<_Tp>& lhs,const quaternion<_Up>& rhs) noexcept {
	return !(lhs==rhs);
}
template <typename _Tp,typename _Up,typename=typename is_complex_arithmetic<_Up>::type>
bool operator !=(const quaternion<_Tp>& lhs,const _Up& rhs) noexcept {
	return !(lhs==rhs);
}
template <typename _Tp,typename _Up,typename=typename is_complex_arithmetic<_Up>::type>
bool operator !=(const _Tp& lhs,const quaternion<_Up>& rhs) noexcept {
	return !(lhs==rhs);
}
template <typename _Tp,typename _Up,typename=typename is_complex_arithmetic<_Up>::type>
bool operator !=(const quaternion<_Tp>& lhs,const complex<_Up>& rhs) noexcept {
	return !(lhs==rhs);
}
template <typename _Tp,typename _Up,typename=typename is_complex_arithmetic<_Up>::type>
bool operator !=(const complex<_Tp>& lhs,const quaternion<_Up>& rhs) noexcept {
	return !(lhs==rhs);
}

template <typename _Tp>
std::ostream& operator <<(std::ostream& os,const quaternion<_Tp>& q) {
	os<<q.to_string();
	return os;
}

}

}

#endif