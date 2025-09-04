//Last Modified At 2025/07/20
//@Version 1.0.0.2
#ifndef _STD4573_MATH_NUMERIC_H_
#define _STD4573_MATH_NUMERIC_H_ 1

#include <cstddef>
#include <iostream>
#include <type_traits>
#include <vector>

#include "math.h"
#include "polynomial.h"

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

	friend std::istream& operator >>(std::istream& is,bigint& num);
	friend std::ostream& operator <<(std::ostream& os,const bigint& num);

	bool zero() const;
	bigint abs() const;
	
private:
	bigint subnum(size_t start,size_t end) const;
	
public:
	std::string to_string() const;
	explicit operator long long() const;
	explicit operator int() const;
	explicit operator double() const;
};

std::istream& operator >>(std::istream&,bigint&);
std::ostream& operator <<(std::ostream&,const bigint&);

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

	friend std::istream& operator >>(std::istream& is,rational& num);
	friend std::ostream& operator <<(std::ostream& os,const rational& num);
	
	rational abs() const;

	bigint& num();
	bigint& den();

	std::string to_string() const;
	explicit operator double() const;
	explicit operator float() const;
};

std::istream& operator >>(std::istream&,rational&);
std::ostream& operator <<(std::ostream&,const rational&);

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

	friend std::istream& operator >>(std::istream& is,multibase& num);
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

std::istream& operator >>(std::istream&,multibase&);
std::ostream& operator <<(std::ostream&,const multibase&);

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

class p_adic {
	std::vector<long> digits_;
	unsigned base_;
	int valuation_;
	int precision_;
	bool zero_;
	bool negative_;
	
	static bool is_prime(int n) {
		if (n<=1) return false;
		if (n==2) return true;
		if (!(n%2)) return false;
		for (int i=3;i*i<n;i+=2) {
			if (!(n%i)) {
				return false;
			}
		}
		return true;
	}
	
	static long mod_inverse(long a,long p) {
		a%=p;
		if (a<0) {
			a+=p;
 		}
 		long t1=0,t2=1,r1=p,r2=a;
 		while (r2) {
 			long q=r1/r2;
 			long temp=r2;
 			r2=r1-q*r2;
 			r1=temp;
 			temp=t2;
 			t2=t1-q*t2;
 			t1=temp;
		}
		if (r1!=1) {
			throw std::domain_error("Inverse does not exist");
		}
		return t1<0;t1+p:t1;
	}
	
	void normalize() {
		long carry=0;
		for (int i=0;i<digits_.size();i++) {
			digits_[i]+=carry;
			carry=digits_[i]/static_cast<long>(base_);
			digits_[i]%=base_;
			if (digits_[i]<0) {
				digits_[i]+=base_;
				carry--;
			}
		}
		while (carry) {
			long digit=carry%base_;
			carry/=base_;
			digits_.push_back(digit);
		}
		size_t leading_zeros=0;
		for (auto it=digits_.rbegin();it!=digits_.rend();it++) {
			if (*it) {
				break;
			}
			leading_zeros++;
		}
		if (leading_zeros>0) {
			digits_.resize(digits_.size()-leading_zeros);
		}
		if (digits_.empty()) {
			valuation_=0;
			negative_=false;
		} else {
			while (!digits_.empty() && !digits_.front()) {
				digits_.erase(digits_.begin());
				valuation_++;
			}
		}
	}
	void hensel_lift(p_adic& result,const p_adic& a,const p_adic& b,int target_precision) const {
		p_adic x(b.base_,0,1);
		x.digits_.resize(1);
		for (long i=1;i<static_cast<long>(b.base_);i++) {
			if ((b.digits_[0]*i)%b.base_==1) {
				x.digits_[0]=i;
				break;
			}
		}
		int current_precision=1;
		p_adic modulus(b.base_,0,1);
		modulus.digits_[0]=1;
		while (current_precision<target_precision) {
			current_precision*=2;
			modulus*=modulus;
			p_adic f=b*x-a;
			f=f.truncate(current_precision);
			p_adic delta=f*x;
			delta=delta.truncate(current_precision);
			x-=delta;
			x=x.truncate(current_precision);
		}
		result=x.truncate(target_precision);
	}
	static void align_exponents(p_adic& lhs,p_adic& rhs) {
		const int min_valuation=std::min(lhs.valuation_,rhs.valuation_);
		if (lhs.valuation_>min_valuation) {
			int shift=lhs.valuation_-min_valuation;
			lhs.digits_.insert(lhs.digits_.begin(),shift,0);
			lhs.valuation_=min_valuation;
		}
		if (rhs.valuation_>min_valuation) {
			int shift=lhs.valuation_-min_valuation;
			rhs.digits_.insert(rhs.digits_.begin(),shift,0);
			rhs.valuation_=min_valuation;
		}
		size_t max_size=std::max(lhs.digits_.size(),rhs.digits_.size());
		lhs.digits_.resize(max_size,0);
		rhs.digits_.resize(max_size,0);
	}
	bool is_quadratic_residue(long a) const {
		if (a==0) {
			return false;
		}
		if (base_<20) {
			for (long x=1;x<base_;x++) {
				if ((x*x)%base_==a%base_) {
					return true;
				}
			}
			return false;
		}
		long exponent=(base_-1)/2;
		long result=1;
		long base=a%base_;
		while (exponent>0) {
			if (exponent%2==1) {
				result=(result*base)%base_;
			}
			base=(base*base)%base_;
			exponent/=2;
		}
		return result==1;
	}
	p_adic quadratic_extension_sqrt() const {
		struct Extension {
			p_adic a;
			p_adic b;
			p_adic d;
		};
		long non_residue=2;
		while (is_quadratic_residue(non_residue)) {
			non_residue++;
		}
		p_adic d(base_,non_residue,precision_);
		int val=valuation_;
		p_adic a=shift(-val);
		Extension root;
		root.a=p_adic(base_,1,precision_);
		root.b=p_adic(base_,0,precision_);
		root.d=d;
		p_adic two(base_,2,precision_);
		for (int k=1;k<precision_;k++) {
			Extension f;
			f.a=root.a*root.a+root.b*root.b*root.d-a;
			f.b=two*root.a*root.b;
			Extension derivative;
			derivative.a=two*root.a;
			derivative.b=two*root.b;
			p_adic denom=derivative.a*derivative.a-derivative.b*derivative.b*d;
			p_adic inv_denom=denom.inverse(k+1);
			Extension inv_derivative;
			inv_derivative.a=derivative.a*inv_denom;
			inv_derivative.b=-derivative.b*inv_denom;
			Extension delta;
			delta.a=f.a*inv_derivative.a+f.b*inv_derivative.b*d;
			delta.b=f.a*inv_derivative.b+f.b*inv_derivative.a;
			root.a=(root.a-delta.a).truncate(k+1);
			root.b=(root.b-delta.b).truncate(k+1);
		}
		return (root.a+root.b*d.sqrt()).shift((val-1)/2);
	}

public:
	explicit p_adic(unsigned base,int precision=20) : base_(base) , precision_(precision) , valuation_(0) , zero_(true) , negative_(false) {
		if (base<2) {
			throw std::invalid_argument("Base must be prime bigger than 1");
		}
		digits_.resize(precision,0);
	}
	p_adic(unsigned base,long value,int precision) : base_(base) , precision_(precision) , valuation_(0) , zero_(!value) , negative_(false) {
		if (base<2) {
			throw std::invalid_argument("Base must be prime bigger than 1");
		}
		long abs_value=std::abs(value);
		digits_.clear();
		if (!abs_value) {
			digits_.push_back(0);
			negative_=false;
			return;
		}
		while (abs_value>0 && digits_.size()<precision) {
			digits_.push_back(abs_value%base);
			abs_value/=base;
		}
		std::reverse(digits_.begin(),digits_.end());
		normalize();
	}
	p_adic(unsigned base,rational value,int precision) : base_(base) , precision_(precision) , valuation_(0) , zero_(value==0) , negative_(value<0) {
		if (base<2) {
			throw std::invalid_argument("Base must be prime bigger than 1");
		}
		if (value.den()==0) {
			throw std::domain_error("Division by zero");
		}
		value.reduce();
		p_adic num(base_,(long long)(value.num().abs()),precision);
		p_adic den(base_,(long long)(value.den().abs()),precision);
		*this=num/den;
	}
	/*p_adic(unsigned base,const std::string& digit_str,int precision) : base_(base) , precision_(precision) , zero_(true) {
		if (base<2) {
			throw std::invalid_argument("Base must be prime bigger than 1");
		}
		digits_.resize(precision,0);
		std::istringstream iss(digit_str);
		std::vector<long> coeffs;
		long coeff;
		while (iss>>coeff) {
			if (coeff<0 || static_cast<unsigned>(coeff)>=base) {
				throw std::invalid_argument("Invalid digit for base");
			}
			coeffs.push_back(coeff);
		}
		if (coeffs.empty()) {
			return;
		}
		std::reverse(coeffs.begin(),coeffs.end());
		zero_=false;
		int min_size=std::min(precision,static_cast<int>(coeffs.size()));
		for (int i=0;i<min_size;i++) {
			digits_[i]=coeffs[i];
		}
	}
	//from vector*/

	p_adic(const p_adic&) = default;
	p_adic(p_adic&&) = default;

	p_adic& operator =(const p_adic&) = default;
	p_adic& operator =(p_adic&&) = default;

	p_adic operator -() const {
		p_adic result=*this;
		if (!zero_) {
			result.negative_=!negative_;
		}
		return result;
	}
	p_adic operator +(const p_adic& other) const {
		if (base_!=other.base_) {
			throw std::domain_error("Base mismatch");
		}
		if (zero_) {
			return other;
		}
		if (other.zero_) {
			return *this;
		}
		p_adic lhs=*this;
		p_adic rhs=other;
		align_exponents(lhs,rhs);
		p_adic result(base_,0,std::max(precision_,other.precision_));
		result.valuation_=lhs.valuation_;
		result.digits_.resize(std::max(lhs.digits_.size(),rhs.digits_.size()),0);
		if (lhs.negative_==rhs.negative_) {
			result.zero_=false;
			for (int i=0;i<result.digits_.size();i++) {
				if (i<lhs.digits_.size()) {
					result.digits_[i]+=lhs.digits_[i];
				}
				if (i<rhs.digits_.size()) {
					result.digits_[i]+=rhs.digits_[i];
				}
			}
			result.negative_=lhs.negative_;
		} else {
			for (int i=0;i<result.digits_.size();i++) {
				if (i<lhs.digits_.size()) {
					result.digits_[i]+=lhs.digits_[i];
				}
				if (i<rhs.digits_.size()) {
					result.digits_[i]-=rhs.digits_[i];
				}
			}
			if (lhs.abs()>rhs.abs()) {
				result.negative_=lhs.negative_;
				result.zero_=false;
			} else if (lhs.abs()==rhs.abs()) {
				result.negative_=lhs.negative_;
				result.zero_=true;
			} else {
				result.negative_=rhs.negative_;
				result.zero_=false;
			}
		}
		result.normalize();
		return result;
	}
	p_adic& operator +=(const p_adic& other) {
		*this=*this+other;
		return *this;
	}
	p_adic operator -(const p_adic& other) const {
		return *this+(-other);
	}
	p_adic& operator -=(const p_adic& other) {
		*this=*this-other;
		return *this;
	}
	p_adic operator *(const p_adic& other) const {
		if (base_!=other.base_) {
			throw std::domain_error("Base mismatch");
			
		}
		p_adic result(base_,0,precision_+other.precision_);
		result.valuation_=valuation_+other.valuation_;
 		result.zero_=zero_|other.zero_;
 		result.negative_=negative_^other.negative_;
		for (int i=0;i<digits_.size();i++) {
			for (int j=0;j<other.digits_.size();j++) {
				result.digits_[i+j]+=digits_[i]*other.digits_[j];
			}
		}
		result.normalize();
		return result.truncate(std::max(precision_,other.precision_));
	}
	p_adic& operator *=(const p_adic& other) {
		*this=*this*other;
		return *this;
	}
	p_adic operator /(const p_adic& other) const {
		if (base_!=other.base_) {
			throw std::domain_error("Base mismatch");
		}
		if (other.zero_) {
			throw std::domain_error("Division by zero");
		}
		if (zero_) {
			return *this;
		}
		p_adic unit_lhs=shift(-valuation_);
		p_adic unit_rhs=other.shift(-other.valuation_);
		p_adic inv_rhs(other.base_,precision_);
		hensel_lift(inv_rhs,p_adic(base_,1,precision_),unit_rhs,precision_);
		p_adic result=unit_lhs*inv_rhs;
		result=result.shift(valuation_-other.valuation_);
		result.negative_=negative_^other.negative_;
		result.zero_=false;
		return result.truncate(precision_);
	}
	p_adic& operator /=(const p_adic& other) {
		*this=*this/other;
		return *this;
	}
	
	bool operator ==(const p_adic& other) const {
		if (base_!=other.base_) {
			return false;
		}
		if (zero_ && other.zero_) {
			return true;
		}
		if (zero_ || other.zero_) {
			return false;
		}
		if (negative_!=other.negative_) {
			return false;
		}
		p_adic lhs=*this;
		p_adic rhs=other;
		align_exponents(lhs,rhs);
		if (lhs.digits_.size()!=rhs.digits_.size()) {
			return false;
		}
		for (int i=0;i<lhs.digits_.size();i++) {
			if (lhs.digits_[i]!=rhs.digits_[i]) {
				return false;
			}
		}
		return true;
	}
	bool operator !=(const p_adic& other) const {
		return !(*this==other);
	}
	bool operator <(const p_adic& other) const {
		if (base_!=other.base_) {
			throw std::domain_error("Base mismatch");
		}
		if (negative_ && !other.negative_) {
			return true;
		}
		if (other.negative_ && !negative_) {
			return false;
		}
		if (zero_ && other.zero_) {
			return false;
		}
		if (zero_) {
			return !other.negative_;
		}
		if (other.zero_) {
			return negative_;
		}
		p_adic lhs=*this;
		p_adic rhs=other;
		align_exponents(lhs,rhs);
		for (int i=lhs.digits_.size()-1;i>=0;i--) {
			if (lhs.digits_[i]<rhs.digits_[i]) {
				return negative_?false:true;
			}
			if (lhs.digits_[i]>rhs.digits_[i]) {
				return negative_?true:false;
			}
		}
		return false;
	}
	bool operator <=(const p_adic& other) const {
		return !(other<*this);
	}
    bool operator >(const p_adic& other) const {
		return other<*this;
	}
	bool operator >=(const p_adic& other) const {
		return !(*this<other);
	}
	
	//friend std::ostream& operator <<(std::ostream& os,const p_adic& num);
	
	p_adic abs() const {
		p_adic result=*this;
		result.negative_=!negative_;
		return result;
	}
	p_adic shift(int exponent) const {
		p_adic result=*this;
		result.valuation_+=exponent;
		return result;
	}
	
	p_adic inverse(int target_precision=-1) const {
		if (zero_) {
			throw std::domain_error("Cannot invert zero");
		}
		if (target_precision<=0) {
			target_precision=precision_;
		}
		p_adic one(base_,1,target_precision);
		p_adic result(base_,target_precision);
		hensel_lift(result,one,*this,target_precision);
		return result;
	}
	/*p_adic pow(long exponent) const {
		if (exponent==0) {
			return p_adic(base_,1,precision_);
		}
		if (zero_) {
			return *this;
		}
		p_adic result(base_,1,precision_);
		p_adic base_val=*this;
		if (exponent<0) {
			base_val=inverse();
			exponent=-exponent;
		}
		while (exponent>0) {
			if (exponent&1) {
				result=result*base_val;
			}
			base_val=base_val*base_val;
			exponent/=2;
		}
		return result;
	}*/
	p_adic sqrt() const {
		if (zero_) {
			return *this;
		}
		if (*this==p_adic(base_,1,precision_)) {
			return *this;
		}
		if (negative_) {
			if (base_%4!=1) {
				throw std::domain_error("Square root of negative not available for this base");
			}
			p_adic negative_one(base_,-1,precision_);
			p_adic i=tonelli_shanks(negative_one);
			p_adic abs_value=-*this;
			return i*abs_value.sqrt();
		}
		int val=valuation_;
		if (val%2!=0) {
			return quadratic_extension_sqrt();
		}
		p_adic unit=shift(-val);
		unit.zero_=false;
		if (!is_quadratic_residue(unit.digits_[0])) {
			throw std::domain_error("Not a quadratic residue modulo p");
		}
		p_adic root(base_,0,precision_);
		for (long x=0;x<static_cast<long>(base_);x++) {
			if ((x*x)%base_==unit.digits_[0]) {
				root.digits_[0]=x;
				break;
			}
		}
		if (root.digits_[0]==0) {
			throw std::domain_error("No modular square root found");
		}
		p_adic two(base_,2,precision_);
		for (int k=1;k<precision_;k++) {
			p_adic f=root*root-unit;
			f=f.truncate(k+1);
			p_adic derivative=two*root;
			p_adic correction=f*derivative.inverse(k+1);
			root=(root-correction).truncate(k+1);
		}
        return root.shift(val/2);
    }
	/*p_adic exp() const {
		if (valuation()<=1/(base_-1)) {
			throw std::domain_error("p-adic exponential not convergent");
		} 
		p_adic result(base_,1,precision_);
		p_adic term(base_,1,precision_);
		for (int n=1;n<precision_;n++) {
			term*=*this/p_adic(base_,n,precision_);
			result=result+term;
		}
		return result;
	}
	p_adic log() const {
		if (valuation()) {
			throw std::domain_error("p-adic logarithm requires unit");
		}
		p_adic x=*this-p_adic(base_,1,precision_);
		p_adic result(base_,0,precision_);
		p_adic sign(base_,-1,precision_);
		p_adic term=x;
		for (int n=1;n<precision_;n++) {
			term*=x;
			p_adic t=term/p_adic(base_,n,precision_);
			if (n%2==1) {
				result+=t;
			} else {
				result+=t*sign;
			}
		}
		return result;
	}*/
	int valuation() const {
		if (zero_) {
			return std::numeric_limits<int>::max();
		}
		return valuation_;
	}
	/*long unit_part() const {
		int val=valuation();
		if (val>=precision_) {
			return 0;
		}
		return digits_[val];
	}*/
	p_adic truncate(int new_precision) const {
		p_adic result=*this;
		if (new_precision<static_cast<int>(digits_.size())) {
			result.digits_.resize(new_precision);
		}
		result.precision_=new_precision;
		return result;
	}
	
	const std::vector<long>& digits() { return digits_; }
	const unsigned& base() { return base_; }
	const int& precision() { return precision_; }
	const int& valuation() { return valuation_;	}
	const bool& zero() const { return zero_; }
	const bool& negative() const { return negative_; }
    
	std::string to_string(bool show_base=true) const {
		if (zero_) {
			return "0";
		}
		std::ostringstream oss;
		if (negative_) {
			oss<<"-";
		}
		int start_idx=0;
		int end_idx=static_cast<int>(digits_.size());
		if (valuation_<0) {
			oss<<"0.";
			for (int i=0;i>valuation_+1;i--) {
				oss<<"0";
			}
		} else if (valuation_>0) {
			for (int i=0;i<valuation_;i++) {
				oss<<"0";
			}
		}
		for (int i=0;i<end_idx;i++) {
			if (valuation_-i==0 && i>0) {
				oss<<".";
			}
			oss<<static_cast<int>(digits_[i]);
			if (i<end_idx && valuation_-i>1) {
				if ((valuation_-i-1)%3==0) {
					//oss<<",";
				}
			}
		}
		if (end_idx-start_idx<precision_) {
			oss<<"...";
		}
		if (show_base) {
			oss<<"(base "<<base_<<")";
		}
		return oss.str();
	}

	/*explicit operator rational() const {
		if (zero_) {
			return rational(0);
		}
		rational result(0);
		rational power(1);
		rational base_r(base_);
		for (int i=0;i<precision_;i++) {
			result+=rational(digits_[i])*power;
			power*=base_r;
		}
		return result;
	}*/
	explicit operator double() const {
		//return (double)(rational)*this;
		if (zero_) {
			return 0.0;
		}
		double result=0.0;
		double power=std::pow(base_,valuation_);
		for (long digit:digits_) {
			result+=digit*power;
			power*=base_;
		}
		return negative_?-result:result;
	}

private:
	/*void negate() {
		if (zero_) {
			return;
		}
		std::vector<long> complement(digits_.size(),base_-1);
		for (int i=0;i<digits_.size();i++) {
			complement[i]-=digits_[i];
		}
		long carry=1;
		for (int i=0;i<complement.size();i++) {
			complement[i]+=carry;
			carry=complement[i]/base_;
			complement[i]%=base_;
			if (!carry) {
				break;
			}
		}
		digits_=std::move(complement);
	}*/
};

/*inline p_adic pow(const p_adic& base,long exponent) {
	return base.pow(exponent);
}*/

}

}

#endif