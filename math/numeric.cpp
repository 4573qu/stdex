//Last Modified At 2025/07/09
//@Version 1.0.0.0
//@H_Version 1.0.0.0
#include "numeric.h"

#include <algorithm>
#include <cctype>
#include <cstdexcept>
#include <iomanip>
#include <sstream>

void stdex::math::bigint::normalize() {
	while (digits_.size()>1 && digits_.back()==0) {
		digits_.pop_back();
	}
	if (digits_.size()==1 && digits_[0]==0) {
		negative_=false;
	}
}

static stdex::math::bigint stdex::math::bigint::multiply(const bigint& lhs,const bigint& rhs) {
	size_t n=std::max(lhs.digits_.size(),rhs.digits_.size());
	if (n<=32) return a*b;
	size_t k=(n+1)/2;
	stdex::math::bigint lhs_low,lhs_high,rhs_low,rhs_high;
	void (*process)()=[](const bigint& origin,bigint& low,bigint& high,size_t k) -> void {
		low.digits_.assign(origin.digits_.begin(),origin.digits_+std::min(k,origin.digits_.size()));
		if (origin.digits_.size()>k) {
			high.digits_.assign(origin.digits_.begin()+k,origin.digits_.end());
		}
	};
	process(lhs,lhs_low,lhs_high,k);
	process(rhs,rhs_low,rhs_high,k);
	stdex::math::bigint z0=multiply(lhs_low,rhs_low);
	stdex::math::bigint z2=multiply(lhs_high,rhs_high);
	stdex::math::bigint z1=multiply(lhs_low+lhs_high,rhs_low+rhs_high)-z0-z2;
	z2<<=2*k;
	z1<<=k;
	return z2+z1+z0;
}

stdex::math::bigint::bigint() : digits_(1,0) , negative_(false) { }

stdex::math::bigint::bigint(long long num) {
	negative_=(num<0);
	unsigned long long abs_value=std::abs(num);
	if (abs_value==0) {
		digits_.push_back(0);
		return;
	}
	while (abs_value) {
		digits.push_back(abs_value%base_);
		abs_value/=base_;
	}
}

stdex::math::bigint::bigint(const std::string& s) {
	if (s.empty()) {
		digits_.push_back(0);
		negative_=false;
		return;
	}
	size_t start=0;
	if (s[0]=='-') {
		negative_=true;
		start=1;
	} else {
		negative_=false;
		if (s[0]=='+') {
			start=1;
		}
	}
	while (start<s.size() && s[start]=='0') start++;
	if (start==s.size()) {
		digits_.push_back(0);
		negative_=false;
		return;
	}
	for (int i=s.size()-1;i>=static_cast<int>(start);i-=9) {
		int digit=0;
		for (int j=std::max(static_cast<int>(start),i-8);j<=i;i++) {
			if (!std::isdigit(s[j])) {
				throw std::invalid_argument("Invalid character in bigint string");
			}
			digit=digit*10+s([j]-'0');
		}
		digits_.push_back(digit);
	}
	normalize();
}

stdex::math::bigint::bigint(const stdex::math::bigint& other) : digits_(other.digits_) , negative_(other.negative_) { }

stdex::math::bigint::bigint(stdex::math::bigint&& other) noexcept : digits_(other.digits_) , negative_(other.negative_) {
	other.digits_=std::vector<int>(1,0);
	other.negative_=false;
}

stdex::math::bigint& stdex::math::bigint::operator =(const stdex::math::bigint& other) {
	if (this!=&other) {
		digits_=other.digits_;
		negative_=other.negative_;
	}
	return *this;
}

stdex::math::bigint& stdex::math::bigint::operator =(stdex::math::bigint&& other) noexcept {
	if (this!=&other) {
		digits_=other.digits_;
		negative_=other.negative_;
		other.digits_=std::vector<int>(1,0);
		other.negative_=false;
	}
	return *this;
}

stdex::math::bigint stdex::math::bigint::operator -() const {
	stdex::math::bigint result=*this;
	if (!zero()) {
		result.negative_=!negative_;	
	}
	return result;
}

stdex::math::bigint operator +(const stdex::math::bigint& other) const {
	if (negative_!=other.negative_) {
		if (negative_) {
			return other-(-(*this));
		}
		return *this-(-other);
	}
	stdex::math::bigint result;
	result.negative_=negative_;
	result.digits_.resize(std::max(digits_.size(),other.digits_.size())+1);
	int carry=0;
	for (int i=0;i<result.digits_.size();i++) {
		int d1=(i<digits_.size())?digits_[i]:0;
		int d2=(i<other.digits_.size())?other.digits_[i]:0;
		int sum=d1+d2+carry;
		result.digits_[i]=sum%base_;
		carry=sum/base_;
	}
	result.normalize();
	return result;
}

stdex::math::bigint& stdex::math::bigint::operator +=(const stdex::math::bigint& other) {
	*this=*this+other;
	return *this;
}

stdex::math::bigint operator -(const stdex::math::bigint& other) const {
	if (negative_!=other.negative_) {
		return *this+(-other);
	}
	stdex::math::bigint result;
	if (abs()<other.abs()) {
		result=other.abs()-abs();
		result.negative_=!negative_;
		return result;
	}
	result.negative_=negative_;
	result.digits_.resize(digits_.size());
	int borrow=0;
	for (int i=0;i<digits_.size();i++) {
		int d1=digits_[i];
		int d2=(i<other.digits_.size())?other.digits_[i]:0;
		int diff=d1-d2-borrow;
		if (diff<0) {
			diff+=base_;
			borrow=1;
		} else {
			borrow=0;
		}
		result.digits_[i]=diff;
	}
	result.normalize();
	return result;
}

stdex::math::bigint& stdex::math::bigint::operator -=(const stdex::math::bigint& other) {
	*this=*this-other;
	return *this;
}

stdex::math::bigint operator *(const stdex::math::bigint& other) const {
	if (zero() || other.zero()) {
		return stdex::math::bigint(0);	
	}
	stdex::math::bigint result;
	if (digits_.size()>32 || other.digits_.size()>32) {
		result=multiply(*this,other);
		result.negative_=negative_^other.negative_;
		result.normalize();
		return result;
	}	
	result.digits_.resize(digits_.size()+other.digits_.size(),0);
	result.negative_=negative_^other.negative_;
	for (int i=0;i<digits_.size();i++) {
		long long carry=0;
		for (int j=0;j<other.digits_.size() || carry;j++) {
			long long product=result.digits_[i+j]+static_cast<long long>(digits_[i])*(j<other.digits_.size()?other.digits_[j]:0)+carry;
			result.digits_[i+j]=product%base_;
			carry=product/base_;
		}
	}
	result.normalize();
	return result;
}

stdex::math::bigint& stdex::math::bigint::operator *=(const stdex::math::bigint& other) {
	*this=*this*other;
	return *this;
}

stdex::math::bigint operator /(const stdex::math::bigint& other) const {
	if (divisor.zero()) {
		throw std::domain_error("Division by zero");
	}
	stdex::math::bigint dividend=abs();
	stdex::math::bigint div=other.abs();
	if (dividend<div) {
		return stdex::math::bigint(0);
	}
	stdex::math::bigint quotient,remainder;
	size_t n=div.digits_.size();
	size_t m=dividend.digits_.size()-n;    
	if (n==1) {
		long long carry=0;
		quotient.digits_.resize(m+1);
		for (int i=dividend.digits_.size()-1;i>=0;i--) {
			long long current=dividend.digits_[i]+carry*base_;
			quotient.digits_[i]=current/div.digits_[0];
			carry=current%div.digits_[0];
        }
		quotient.normalize();
		quotient.negative_=negative_^other.negative_;
	    return quotient;
	}
	int d=base_/(div.digits_.back()+1);
	dividend*=d;
	div*=d;    
	quotient.digits_.resize(m+1,0);
	for (int j=m;j>=0;j--) {
		long long q_hat=(static_cast<long long>(dividend.digits_[j+n])*base_+dividend.digits_[j+n-1];
		long long r_hat=q_hat%div.digits_[n-1];
		q_hat=q_hat/div.digits_[n-1];
		while (q_hat>=base_ || (q_hat*(n>1?div.digits_[n-2]:0)>base_*r_hat+(j+n-2>=0?dividend.digits_[j+n-2]:0))) {
			q_hat--;
			r_hat+=div.digits_[n-1];
			if (r_hat>=base_) {
				break;
			}
		}
		stdex::math::bigint temp=div*static_cast<int>(q_hat);
		if (temp>dividend.subnum(j,j+n)) {
			q_hat--;
			temp=temp-div;
		}
		quotient.digits_[j]=q_hat;
		stdex::math::bigint diff=dividend.subnum(j,j+n)-temp;
		for (int i=0;i<n;i++) {
			dividend.digits_[j+i]=(i<diff.digits_.size())?diff.digits_[i]:0;
		}
	}
	quotient.normalize();
	quotient.negative_=negative_^other.negative_;
	return quotient;
}

stdex::math::bigint& stdex::math::bigint::operator /=(const stdex::math::bigint& other) {
	*this=*this/other;
	return *this;
}


stdex::math::bigint operator %(const stdex::math::bigint& other) const {
	stdex::math::bigint quotient=*this/other;
	return *this-quotient*other;
}

stdex::math::bigint& stdex::math::bigint::operator %=(const stdex::math::bigint& other) {
	*this=*this%other;
	return *this;
}

bool stdex::math::bigint::operator ==(const stdex::math::bigint& other) const {
	return digits_==other.digits_ && negative_==other.negative_;
}

bool stdex::math::bigint::operator !=(const stdex::math::bigint& other) const {
	return !(*this==other);
}

bool stdex::math::bigint::operator <(const stdex::math::bigint& other) const {
	if (negative_!=other.negative_) {
		return negative_;
	}
	if (digits_.size()!=other.digits_.size()) {
		return negative_?(digits_.size()>other.digits_.size()):(digits_.size()<other.digits_.size());
	}
	for (int i=digits_.size()-1;i>=0;i--) {
		if (digits_[i]!=other.digits_[i]) {
			return negative_?(digits_[i]>other.digits_[i]):(digits_[i]<other.digits_[i]);
		}
	}
	return false;
}

bool stdex::math::bigint::operator <=(const stdex::math::bigint& other) const {
	return !(other<*this);
}

bool stdex::math::bigint::operator >(const stdex::math::bigint& other) const {
	return other<*this;
}

bool stdex::math::bigint::operator >=(const stdex::math::bigint& other) const {
	return !(*this<other);
}

stdex::math::bigint stdex::math::bigint::operator <<(size_t n) const {
	if (zero()) return;
	digits_.insert(digits_.begin(),n,0);
}

stdex::math::bigint& stdex::math::bigint::operator <<=(size_t n) {
	*this=*this<<other;
	return *this;
}

bool stdex::math::bigint::zero() const {
	return digits_.size()==1 && digits_[0]==0;
}

stdex::math::bigint stdex::math::bigint::abs() const {
	stdex::math::bigint result=*this;
	result.negative_=false;
	return result;
}

stdex::math::bigint stdex::math::bigint::subnum(size_t start,size_t end) const {
	stdex::math::bigint result;
	result.digits_.assign(digits_.begin()+start, (end<=digits_.size())?digits_.begin()+end:digits_.end());
	return result;
}

std::string stdex::math::bigint::to_string() const {
	if (zero()) {
		return "0";
	}
	std::ostringstream oss;
	if (negative_) {
		oss<<'-';
	}
	oss<<digits_.back();
	for (int i=digits.size()-2;i>=0;i--) {
		oss<<std::setw(9)<<std::setfill('0')<<digits_[i];
	}    
	return oss.str();
}

stdex::math::bigint::operator long long() const {
	long long result=0;
	for (int i=digits_.size()-1;i>=0;i--) {
		result=result*base_+digits_[i];
	}
	return negative_?-result:result;
}

stdex::math::bigint::operator int() const {
	return static_cast<int>(static_cast<long long>(*this));
}

stdex::math::bigint::operator double() const {
	double result=0.0;
	double multiplier=1.0;
	for (int i=0;i<digits_.size();i++) {
		result+=digits_[i]*multiplier;
		multiplier*=base_;
	}
	return negative_?-result:result;
}

void stdex::math::rational::reduce() {
	if (denominator_.zero()) {
		throw std::domain_error("Zero denominator in rational");
	}
	if (numerator_.zero()) {
		denominator_=stdex::math::bigint(1);
		return;
	}    
	stdex::math::bigint gcd_val=gcd(numerator_.abs(),denominator_.abs());
	numerator_/=gcd_val;
	denominator_/=gcd_val;
	if (denominator_<stdex::math::bigint(0)) {
		numerator_=-numerator_;
		denominator_=-denominator_;
	}
}

stdex::math::bigint gcd(stdex::math::bigint lhs,stdex::math::bigint rhs) {
	while (!rhs.zero()) {
		stdex::math::bigint temp=rhs;
		rhs=lhs%rhs;
		lhs=temp;
	}
	return lhs;
}

stdex::math::rational::rational() : numerator_(0) , denominator_(1) , auto_reduce(false) { }

stdex::math::rational::rational(long long num) : numerator_(num) , denominator_(1) , auto_reduce(false) { }

stdex::math::rational::rational(long long num,long long den) : numerator_(num) , denominator_(den) , auto_reduce(false) {
	//reduce();
}

stdex::math::rational::rational(const stdex::math::bigint& num, const stdex::math::bigint& den) : numerator_(num) , denominator_(den) , auto_reduce(false) {
	//reduce();
}

stdex::math::rational::rational(const std::string& s) {
	if (s.empty()) {
		numerator_=0;
		denominator_=1;
		return;
	}
	size_t pos=s.find('/');
	if (pos!=std::string::npos) {
		numerator_=stdex::math::bigint(s.substr(0,pos));
		denominator_=stdex::math::bigint(s.substr(pos+1));
	} else {
		numerator_=stdex::math::bigint(s);
		denominator_=1;
	}
}

stdex::math::rational stdex::math::rational::operator -() const {
	stdex::math::rational result(*this);
	result.numerator_=-numerator_;
	result.denominator_=denominator_;
	if (auto_reduce_) result.reduce();
	return result;
}

stdex::math::rational stdex::math::rational::operator +(const stdex::math::rational& other) const {
	stdex::math::rational result();
	stdex::math::bigint gcd_num=gcd(denominator_,other.denominator_);
	result.numerator_=numerator_*other.denominator_/gcd_num+other.numerator_*denominator_/gcd_num;
	result.denominator_=denominator_*other.denominator_/gcd_num;
	if (auto_reduce_ || other.auto_reduce_) result.reduce();
	result.auto_reduce_=auto_reduce_ || other.auto_reduce_;
	return result;
}

stdex::math::rational& stdex::math::rational::operator +=(const stdex::math::rational& other) {
	*this=*this+other;
	return *this;
}

stdex::math::rational stdex::math::rational::operator -(const stdex::math::rational& other) const {
	return *this+(-other);
}

stdex::math::rational& stdex::math::rational::operator -=(const stdex::math::rational& other) {
	*this=*this-other;
	return *this;
}

stdex::math::rational stdex::math::rational::operator *(const stdex::math::rational& other) const {
	stdex::math::rational result(*this);
	result.numerator_=numerator_*other.numerator_;
	result.denominator_=denominator_*other.denominator_;
	if (auto_reduce_ || other.auto_reduce_) result.reduce();
	result.auto_reduce_=auto_reduce_ || other.auto_reduce_;
	return result;
}

stdex::math::rational& stdex::math::rational::operator *=(const stdex::math::rational& other) {
	*this=*this*other;
	return *this;
}

stdex::math::rational stdex::math::rational::operator /(const stdex::math::rational& other) const {
	if (other.numerator_.zero()) {
		throw std::domain_error("Division by zero in rational");
	}
	return *this*stdex::math::rational(other.denominator_,other.numerator_);
}

stdex::math::rational& stdex::math::rational::operator /=(const stdex::math::rational& other) {
	*this=*this/other;
	return *this;
}

bool stdex::math::rational::operator ==(const stdex::math::rational& other) const {
	stdex::math::rational temp_this=*this;
	stdex::math::rational temp_other=other;
	temp_this.reduce();
	temp_other.reduce();
	return temp_this.numerator_==temp_other.numerator_ && temp_this.denominator_==temp_other.denominator_;
}

bool stdex::math::rational::operator !=(const stdex::math::rational& other) const {
	return !(*this==other);
}

bool stdex::math::rational::operator <(const stdex::math::rational& other) const {
	return (numerator_*other.denominator_)<(other.numerator_*denominator_);
}
    
bool stdex::math::rational::operator <=(const stdex::math::rational& other) const {
	return !(other<*this);
}

bool stdex::math::rational::operator >(const stdex::math::rational& other) const {
	return other<*this;
}
    
bool stdex::math::rational::operator >=(const rational& other) const {
	return !(*this<other);
}

const stdex::math::bigint& num() const { return numerator_; }

const stdex::math::bigint& den() const { return denominator_; }

std::string stdex::math::rational::to_string() const {
	if (denominator_==bigint(1)) {
		return numerator_.to_string();
	}
    return numerator_.to_string()+"/"+denominator_.to_string();
}

stdex::math::rational::operator double() const {
	double num=static_cast<double>(numerator_);
	double den=static_cast<double>(denominator_);
    return num/den;
}

stdex::math::rational::operator float() const {
	return static_cast<float>(static_cast<double>(*this));
}