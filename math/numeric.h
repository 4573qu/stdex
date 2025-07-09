//Last Modified At 2025/07/09
//@Version 1.0.0.0
#ifndef _STD4573_MATH_NUMERIC_H_
#define _STD4573_MATH_NUMERIC_H_ 1

#include <cstddef>
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
	double value_;
	double base_;
	int precision_;
	double parse(const std::string& s);

public:
    // 构造函数
    non_integer_base() : base(10.0), value(0.0), precision(6) {}
    
    non_integer_base(double b, double v, int p = 6) : base(b), value(v), precision(p) {
        if (base <= 1) throw std::invalid_argument("Base must be > 1");
    }

    non_integer_base(double b, const std::string& rep, int p = 6) : base(b), precision(p) {
        if (base <= 1) throw std::invalid_argument("Base must be > 1");
        value = parse(rep);
    }
    
    // 从内置类型赋值
    non_integer_base& operator=(int val) {
        value = static_cast<double>(val);
        return *this;
    }
    
    non_integer_base& operator=(double val) {
        value = val;
        return *this;
    }
    
    non_integer_base& operator=(const bigint& val) {
        value = static_cast<double>(val);
        return *this;
    }
    
    non_integer_base& operator=(const rational& val) {
        value = static_cast<double>(val);
        return *this;
    }

    // 隐式类型转换
    operator int() const { return static_cast<int>(value); }
    operator double() const { return value; }
    operator bigint() const { return bigint(static_cast<long long>(value)); }
    operator rational() const { return rational(static_cast<long long>(value * 1000000), 1000000); }

    // 算术运算
    non_integer_base operator+(const non_integer_base& other) const {
        if (std::abs(base - other.base) > 1e-9)
            throw std::domain_error("Base mismatch");
        return non_integer_base(base, value + other.value, std::max(precision, other.precision));
    }

    non_integer_base operator-(const non_integer_base& other) const {
        if (std::abs(base - other.base) > 1e-9)
            throw std::domain_error("Base mismatch");
        return non_integer_base(base, value - other.value, std::max(precision, other.precision));
    }

    non_integer_base operator*(const non_integer_base& other) const {
        if (std::abs(base - other.base) > 1e-9)
            throw std::domain_error("Base mismatch");
        return non_integer_base(base, value * other.value, std::max(precision, other.precision));
    }

    non_integer_base operator/(const non_integer_base& other) const {
        if (std::abs(other.value) < 1e-15)
            throw std::domain_error("Division by zero");
        if (std::abs(base - other.base) > 1e-9)
            throw std::domain_error("Base mismatch");
        return non_integer_base(base, value / other.value, std::max(precision, other.precision));
    }

    // 比较操作
    bool operator==(const non_integer_base& other) const {
        return std::abs(value - other.value) < 1e-9;
    }
    bool operator<(const non_integer_base& other) const { return value < other.value; }
    bool operator>(const non_integer_base& other) const { return value > other.value; }
    bool operator<=(const non_integer_base& other) const { return value <= other.value; }
    bool operator>=(const non_integer_base& other) const { return value >= other.value; }

    // 类型转换
    double to_double() const { return value; }
    
    std::string to_string() const {
        if (value < 0) throw std::domain_error("Negative values not supported");
        
        double intPart;
        double fracPart = std::modf(value, &intPart);
        std::string result;

        // 转换整数部分
        long n = static_cast<long>(intPart);
        do {
            long digit = static_cast<long>(std::fmod(n, base));
            result = char('0' + digit) + result;
            n = static_cast<long>((n - digit) / base);
        } while (n > 0);

        // 转换小数部分
        if (fracPart > 1e-10) {
            result += '.';
            double f = fracPart;
            int count = 0;
            while (f > 1e-10 && count < precision) {
                f *= base;
                double digit;
                f = std::modf(f, &digit);
                result += char('0' + static_cast<int>(digit));
                count++;
            }
        }

        return result.empty() ? "0" : result;
    }

    // 友元输出
    friend std::ostream& operator<<(std::ostream& os, const non_integer_base& num) {
        os << num.to_string() << " (base " << num.base << ")";
        return os;
    }
};


}

}