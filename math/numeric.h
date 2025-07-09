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
	static const int base_=1000000000;
	std::vector<int> digits_;
	bool negative_;
	
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
	static double epsilon_=1e-9;
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

	double base();
	int precision();
	static double epsilon();
	void base(double base);
	void precision(int precision);
	static void epsilon(double epsilon);

	double to_double() const;
	std::string to_string() const;

	explicit operator int() const { return static_cast<int>(value); }
	explicit operator double() const { return value; }
	explicit operator bigint() const { return bigint(static_cast<long long>(value)); }
	explicit operator rational() const;
};

template <typename _Tp>
struct is_complex_arithmetic : std::false_type {};
template <typename _Tp>
struct is_complex_arithmetic<_Tp,std::enable_if_t<std::is_arithmetic<_Tp>::value>> : std::true_type {};
template <>
struct is_complex_arithmetic<bigint> : std::true_type {};
template <>
struct is_complex_arithmetic<rational> : std::true_type {};



template <typename _Tp>
class complex {
	_Tp real_;
	_Tp imag_;

	template <typename _Up>
	using enable_if_arithmetic=typename is_complex_arithmetic<_Up>::type*;
	template <typename _Up>
	using enable_if_floating=typename std::enable_if<std::is_floating_point<_Up>::value>::type*;
	//template <typename _Up>
	//using enable_if_complex=typename std::enable_if<std::is_same<_Up,complex<typename _Up::value_type>>::value>::type*;
	//template <typename _Func>
	//using enable_if_callable=decltype(std::declval<_Func>()(std::declval<_Tp>()),int>;

public:
	constexpr complex(const _Tp& real=_Tp(),const _Tp& imag=_Tp()) noexcept;
	template <typename _Up>
	constexpr complex(const complex<_Up>& other) noexcept;
	template <typename _Up,enable_if_arithmetic<_Up>=nullptr>
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
	template <typename _Up,enable_if_arithmetic<_Up>=nullptr>
	complex<_Tp>& operator +=(const _Up& scalar) noexcept;
	complex<_Tp>& operator -=(const complex<_Tp>& other) noexcept;
	template <typename _Up>
	complex<_Tp>& operator -=(const complex<_Up>& other) noexcept;
	template <typename _Up,enable_if_arithmetic<_Up>=nullptr>
	complex<_Tp>& operator -=(const _Up& scalar) noexcept;
	complex<_Tp>& operator *=(const complex<_Tp>& other) noexcept;
	template <typename _Up>
	complex<_Tp>& operator *=(const complex<_Up>& other) noexcept;
	template <typename _Up,enable_if_arithmetic<_Up>=nullptr>
	complex<_Tp>& operator *=(const _Up& scalar) noexcept;
	complex<_Tp>& operator /=(const complex<_Tp>& other) noexcept;
	template <typename _Up>
	complex<_Tp>& operator /=(const complex<_Up>& other) noexcept;
	template <typename _Up,enable_if_arithmetic<_Up>=nullptr>
	complex<_Tp>& operator /=(const _Up& scalar) noexcept;
    
	constexpr complex<_Tp> conj() const noexcept;
	template <typename _Up=_Tp,enable_if_arithmetic<_Up>=nullptr>
	constexpr _Tp norm() const noexcept;
	template <typename _Up=_Tp,enable_if_floating<_Up>=nullptr>
	_Tp abs() const noexcept;
	template <typename _Up=_Tp,enable_if_floating<_Up>=nullptr>
	_Tp arg() const noexcept;
	template <typename _Up=_Tp,enable_if_floating<_Up>=nullptr>
	complex<_Tp> proj() const noexcept;
	template <typename _Up=_Tp,enable_if_floating<_Up>=nullptr>
	static complex<_Tp> polar(const _Tp& r,const _Tp& theta=_Tp()) noexcept;

	template <typename _Up=_Tp,enable_if_floating<_Up>=nullptr>
	complex<_Tp> exp() const noexcept;
	template <typename _Up=_Tp,enable_if_floating<_Up>=nullptr>
	complex<_Tp> log() const noexcept;
	template <typename _Up=_Tp,enable_if_floating<_Up>=nullptr>
	complex<_Tp> pow(const complex<_Tp>& exponent) const;
	template <typename _Up=_Tp,enable_if_floating<_Up>=nullptr>
	complex sqrt() const noexcept;

	template <typename _Up=_Tp,enable_if_floating<_Up>=nullptr>
	complex<_Tp> sin() const noexcept;
	template <typename _Up=_Tp,enable_if_floating<_Up>=nullptr>
	complex<_Tp> cos() const noexcept;
	template <typename _Up=_Tp,enable_if_floating<_Up>=nullptr>
	complex<_Tp> tan() const;

	template <typename _Up=_Tp,enable_if_floating<_Up>=nullptr>
	complex<_Tp> sinh() const noexcept;
	template <typename _Up=_Tp,enable_if_floating<_Up>=nullptr>
	complex<_Tp> cosh() const noexcept;
	template <typename _Up=_Tp,enable_if_floating<_Up>=nullptr>
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
	using enable_if_arithmetic=typename is_complex_arithmetic<_Up>::type*;
	template <typename _Up>
	using enable_if_floating=typename std::enable_if<std::is_floating_point<_Up>::value>::type*;

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
	template <typename _Up,enable_if_arithmetic<_Up>=nullptr>
	quaternion<_Tp>& operator +=(const _Up& scalar) noexcept;
	template <typename _Up,enable_if_arithmetic<_Up>=nullptr>
	quaternion<_Tp>& operator +=(const complex<_Up>& complexor) noexcept;
	quaternion<_Tp>& operator -=(const quaternion<_Tp>& other) noexcept;
	template <typename _Up>
	quaternion<_Tp>& operator -=(const quaternion<_Up>& other) noexcept;
	template <typename _Up,enable_if_arithmetic<_Up>=nullptr>
	quaternion<_Tp>& operator -=(const _Up& scalar) noexcept;
	template <typename _Up,enable_if_arithmetic<_Up>=nullptr>
	quaternion<_Tp>& operator -=(const complex<_Up>& complexor) noexcept;
	quaternion<_Tp>& operator *=(const quaternion<_Tp>& other) noexcept;
	template <typename _Up>
	quaternion<_Tp>& operator *=(const quaternion<_Up>& other) noexcept;
	template <typename _Up,enable_if_arithmetic<_Up>=nullptr>
	quaternion<_Tp>& operator *=(const _Up& scalar) noexcept;
	template <typename _Up,enable_if_arithmetic<_Up>=nullptr>
	quaternion<_Tp>& operator *=(const complex<_Up>& complexor) noexcept;
	quaternion<_Tp>& operator /=(const quaternion<_Tp>& other) noexcept;
	template <typename _Up>
	quaternion<_Tp>& operator /=(const quaternion<_Up>& other) noexcept;
	template <typename _Up,enable_if_arithmetic<_Up>=nullptr>
	quaternion<_Tp>& operator /=(const _Up& scalar) noexcept;
	template <typename _Up,enable_if_arithmetic<_Up>=nullptr>
	quaternion<_Tp>& operator /=(const complex<_Up>& complexor) noexcept;

	constexpr quaternion<_Tp> conj() const noexcept;
	template <typename _Up,enable_if_arithmetic<_Up>=nullptr>
	constexpr _Tp norm_sq();
	template <typename _Up=_Tp,enable_if_floating<_Up>=nullptr>
	_Tp norm() const noexcept;
	template <typename _Up,enable_if_arithmetic<_Up>=nullptr>
	quaternion<_Tp> inverse() const;
	template <typename _Up=_Tp,enable_if_floating<_Up>=nullptr>
	quaternion<_Tp> unit() const;
	_Tp dot(const quaternion<_Tp>& other) const noexcept;
	template <typename _Up>
	_Tp dot(const quaternion<_Up>& other) const noexcept;
    
    // 旋转向量（仅对浮点类型启用）
    template <typename U = T, EnableIfFloatingPoint<U> = nullptr>
    static quaternion rotation(const T& angle, const T& ax, const T& ay, const T& az) noexcept {
        const T half_angle = angle / 2;
        const T sin_half = std::sin(half_angle);
        return quaternion(std::cos(half_angle), 
                          ax * sin_half,
                          ay * sin_half,
                          az * sin_half);
    }
    
    // 旋转向量（重载）
    template <typename U = T, EnableIfFloatingPoint<U> = nullptr>
    static quaternion rotation(const T& angle, const complex<T>& axis) noexcept {
        return rotation(angle, axis.real(), axis.imag(), 0);
    }
    
    // 应用旋转到向量
    template <typename U = T, EnableIfFloatingPoint<U> = nullptr>
    quaternion rotate_vector(const T& vx, const T& vy, const T& vz) const {
        const quaternion p(0, vx, vy, vz);
        const quaternion result = *this * p * this->inverse();
        return result;
    }
    
    // 应用旋转到复数（视为2D向量）
    template <typename U = T, EnableIfFloatingPoint<U> = nullptr>
    complex<T> rotate_complex(const complex<T>& c) const {
        const quaternion result = rotate_vector(c.real(), c.imag(), 0);
        return complex<T>(result.x(), result.y());
    }
    
    // 指数函数（仅对浮点类型启用）
    template <typename U = T, EnableIfFloatingPoint<U> = nullptr>
    quaternion exp() const noexcept {
        const complex<T> v(x_, y_);
        const T v_norm = v.abs();
        const T exp_w = std::exp(w_);
        
        if (v_norm == 0) {
            return quaternion(exp_w, 0, 0, 0);
        }
        
        const T cos_v = std::cos(v_norm);
        const T sin_v = std::sin(v_norm);
        const T scale = exp_w * sin_v / v_norm;
        
        return quaternion(exp_w * cos_v,
                          x_ * scale,
                          y_ * scale,
                          z_ * scale);
    }
    
    // 对数函数（仅对浮点类型启用）
    template <typename U = T, EnableIfFloatingPoint<U> = nullptr>
    quaternion log() const noexcept {
        const T q_norm = this->norm();
        const complex<T> v(x_, y_);
        const T v_norm = v.abs();
        
        if (q_norm == 0 || v_norm == 0) {
            return quaternion(std::log(q_norm), 0, 0, 0);
        }
        
        const T scale = std::acos(w_ / q_norm) / v_norm;
        return quaternion(std::log(q_norm),
                          x_ * scale,
                          y_ * scale,
                          z_ * scale);
    }
    
    // 幂函数（仅对浮点类型启用）
    template <typename U = T, EnableIfFloatingPoint<U> = nullptr>
    quaternion pow(const T& exponent) const {
        return (log() * exponent).exp();
    }
    
        
    // 分量访问
    const _Tp& w() const noexcept { return w_; }
    const _Tp& x() const noexcept { return x_; }
    const _Tp& y() const noexcept { return y_; }
    const _Tp& z() const noexcept { return z_; }
        constexpr T real() const noexcept { return w_; }
    constexpr complex<T> imag() const noexcept { return complex<T>(x_, y_); }
    
    // 字符串表示
    std::string to_string() const {
        std::ostringstream oss;
        oss << "(" << w_ << ", " << x_ << "i, " << y_ << "j, " << z_ << "k)";
        return oss.str();
    }
};

// 非成员函数
template <typename T>
constexpr quaternion<T> operator+(const quaternion<T>& lhs, const quaternion<T>& rhs) noexcept {
    return quaternion<T>(lhs) += rhs;
}

template <typename T>
constexpr quaternion<T> operator*(const quaternion<T>& lhs, const quaternion<T>& rhs) noexcept {
    return quaternion<T>(lhs) *= rhs;
}

// 标量乘法
template <typename T, typename U,
          typename = typename std::enable_if<std::is_arithmetic<U>::value>::type>
constexpr quaternion<T> operator*(const quaternion<T>& lhs, const U& scalar) noexcept {
    return quaternion<T>(lhs) *= scalar;
}

template <typename T, typename U,
          typename = typename std::enable_if<std::is_arithmetic<T>::value>::type>
constexpr quaternion<U> operator*(const T& scalar, const quaternion<U>& rhs) noexcept {
    return quaternion<U>(rhs) *= scalar;
}

// 比较操作符
template <typename T>
bool operator==(const quaternion<T>& lhs, const quaternion<T>& rhs) noexcept {
    return lhs.w() == rhs.w() && 
           lhs.x() == rhs.x() && 
           lhs.y() == rhs.y() && 
           lhs.z() == rhs.z();
}

template <typename T>
bool operator!=(const quaternion<T>& lhs, const quaternion<T>& rhs) noexcept {
    return !(lhs == rhs);
}

// 流操作符
template <typename T>
std::ostream& operator<<(std::ostream& os, const quaternion<T>& q) {
    os << q.to_string();
    return os;
}

}

}