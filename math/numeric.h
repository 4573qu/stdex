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

}

}