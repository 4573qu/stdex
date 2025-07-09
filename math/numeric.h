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
	double base_;
	double value_;
	int precision_;
	static double epsilon_=1e-15;
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

    // 隐式类型转换
    operator int() const { return static_cast<int>(value); }
    operator double() const { return value; }
    operator bigint() const { return bigint(static_cast<long long>(value)); }
//precision->    operator rational() const { return rational(static_cast<long long>(value * 1000000), 1000000); }

//convert base
//change epsilon_
    // 算术运算
    

    // 比较操作


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
    friend std::ostream& operator<<(std::ostream& os, const multibase& num) {
        os << num.to_string() << " (base " << num.base << ")";
        return os;
    }
};


}

}