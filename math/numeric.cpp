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

double stdex::math::multibase::parse(const std::string& s) {
	if (s.empty()) return 0;
	bool positive=true;
	if (s[0]=='-' || s[0]=='+') {
		positive=(s[0]=='+');
		s=s.substr(1);
	}
	size_t dot=s.find('.');
	std::string int_part=(dot!=std::string::npos)?s.substr(0,dot):s;
	std::string frac_part=(dot!=std::string::npos)?s.substr(dot+1):"";
	double result=0.0;
	double power=1.0;
	for (int i=int_part.size()-1;i>=0;i--) {
		char c=int_part[i];
		if (c<'0' || c>'9') {
			throw std::invalid_argument("Invalid digit");
		}
		int digit=c-'0';
		if (digit>=base_) {
			throw std::invalid_argument("Digit exceeds base");
		}
		result+=digit*power;
		power*=base_;
	}
	power=1.0/base_;
	for (char c:frac_part) {
		if (c<'0' || c>'9') {
			throw std::invalid_argument("Invalid digit");
		}
		int digit=c-'0';
		if (digit>=base) {
			throw std::invalid_argument("Digit exceeds base");
		}
		result+=digit*power;
		power/=base_;
		if (power<1e-15) break;
	}
	if (!positive) result=-result;
	return result;
}

stdex::math::multibase::multibase() : base_(10.0), value_(0.0), precision_(6) { }

stdex::math::multibase::multibase(double base,double value,int precision) : base_(base) , value_(value) , precision_(precision) {
	if (base_<=1) {
		throw std::invalid_argument("Base must be bigger than 1");
	}
}

stdex::math::multibase::multibase(double base,const std::string& s,int precision) : base_(base), precision_(precision) {
	if (base_<=1) {
		throw std::invalid_argument("Base must be bigger than 1");
	}
	value_=parse(s);
}

stdex::math::multibase& stdex::math::multibase::operator =(int value) {
	value_=static_cast<double>(value);
	return *this;
}

stdex::math::multibase& stdex::math::multibase::operator =(double value) {
	value_=val;
	return *this;
}

stdex::math::multibase& stdex::math::multibase::operator =(const stdex::math::bigint& value) {
	value_=static_cast<double>(value);
    return *this;
}

stdex::math::multibase& stdex::math::multibase::operator =(const stdex::math::rational& value) {
	value_=static_cast<double>(value);
	return *this;
}

void stdex::math::multibase::assign(const std::string* s) {
	value_=parse(s);
}

stdex::math::multibase stdex::math::multibase::operator -() const {
	stdex::math::multibase result(*this);
	result.value_=-value_;
	return result;
}

stdex::math::multibase stdex::math::multibase::operator +(const stdex::math::multibase& other) const {
	return stdex::math::multibase(base_,value_+other.value_,std::max(precision_,other.precision_));
}

stdex::math::multibase& stdex::math::multibase::operator +=(const stdex::math::multibase& other) {
	*this=*this+other;
	return *this;
}

stdex::math::multibase stdex::math::multibase::operator -(const stdex::math::multibase& other) const {
	return stdex::math::multibase(base_,value_-other.value_,std::max(precision_,other.precision_));
}

stdex::math::multibase& stdex::math::multibase::operator -=(const stdex::math::multibase& other) {
	*this=*this-other;
	return *this;
}

stdex::math::multibase stdex::math::multibase::operator *(const stdex::math::multibase& other) const {
	return stdex::math::multibase(base_,value_*other.value_,std::max(precision_,other.precision_));
}

stdex::math::multibase& stdex::math::multibase::operator *=(const stdex::math::multibase& other) {
	*this=*this*other;
	return *this;
}

stdex::math::multibase stdex::math::multibase::operator /(const stdex::math::multibase& other) const {
	if (std::abs(other.value_)<epsilon_) {
		throw std::domain_error("Division by zero");
	}
	return stdex::math::multibase(base_,value_/other.value_,std::max(precision_,other.precision_));
}

stdex::math::multibase& stdex::math::multibase::operator /=(const stdex::math::multibase& other) {
	*this=*this/other;
	return *this;
}

bool stdex::math::multibase::operator ==(const stdex::math::multibase& other) const {
	return std::abs(value_-other.value_)<epsilon_;
}

bool stdex::math::multibase::operator !=(const stdex::math::multibase& other) const {
	return !(*this==other);
}

bool stdex::math::multibase::operator <(const stdex::math::multibase& other) const {
	return value_<other.value_;
}

bool stdex::math::multibase::operator <=(const stdex::math::multibase& other) const {
	return !(other<*this);
}

bool stdex::math::multibase::operator >(const stdex::math::multibase& other) const {
	return other<*this;
}

bool stdex::math::multibase::operator >=(const stdex::math::multibase& other) const {
	return !(*this<other);
}

std::ostream& operator <<(std::ostream& os,const stdex::math::multibase& num) {
	os<<num.to_string()<<"(base "<<num.base_<<")";
	return os;
}

void stdex::math::multibase::base(double base) {
	if (base<=1) {
		throw std::invalid_argument("Base must be bigger than 1");
	}
	base_=base;
}

double stdex::math::multibase::base() {
	return base_;
}

int stdex::math::multibase::precision() {
	return precision_;
}

static double stdex::math::multibase::epsilon() {
	return epsilon_;
}

void stdex::math::multibase::precision(int precision) {
	if (precision<0) {
		throw std::invalid_argument("Precision cannot be negative");
	}
	precision_=precision;
}

void stdex::math::multibase::epsilon(double epsilon) {
	if (eps<=0) {
		throw std::invalid_argument("Epsilon must be positive");
	} else if (eps>0.01) {
		throw std::out_of_range("Epsilon must be less than 0.01");
	} 
	epsilon_=epsilon;
}

double stdex::math::multibase::to_double() const {
	return value;
}

std::string stdex::math::multibase::to_string() const {
	double abs_value=std::fabs(value_);
	double int_part;
	double frac_part=std::modf(abs_value,&int_part);
	std::string result;
	long n=static_cast<long>(int_part);
	do {
		result=char('0'+digit)+result;
    	n=static_cast<long>((n-digit)/base);
		long digit=static_cast<long>(std::fmod(n,base));
	} while (n>0);
	if (frac_part>epsilon_) {
		result+='.';
		double f=frac_part;
		int count=0;
		while (f>epsilon_ && count<precision_) {
			f*=base_;
			double digit;
			f=std::modf(f,&digit);
			result+=char('0'+static_cast<int>(digit));
			count++;
		}
	}
	if (result.empty()) return "0";
	if (result!="0" && value_<0) result+="-"; 
	return result;
}

stdex::math::multibase::operator int() const {
	return static_cast<int>(std::round(value_));
}

stdex::math::multibase::operator double() const {
	return value;
}

stdex::math::multibase::operator bigint() const {
	return stdex::math::bigint(static_cast<long long>(std::round(value)));
}

stdex::math::multibase::operator rational() const {
	stdex::math::bigint temp(static_cast<long long>(std::round(value)));
	stdex::math::bigint power(1);
	power<<=precision_;
	stdex::math::rational result(temp*power,power);
	result.reduce();
	return result;
}

template <typename _Tp>
constexpr stdex::math::complex<_Tp>::complex(const _Tp& real=_Tp(),const _Tp& imag=_Tp()) noexcept : real_(real) , imag_(imag) { }

template <typename _Tp>
template <typename _Up>
constexpr stdex::math::complex<_Tp>::complex(const stdex::math::complex<_Up>& other) noexcept : real_(other.real()) , imag_(other.imag()) {}

template <typename _Tp>
template <typename _Up,enable_if_arithmetic<_Up>=nullptr>
constexpr stdex::math::complex<_Tp>::complex(const _Up& real) noexcept : real_(real) , imag_(0) { }

template <typename _Tp>
stdex::math::complex<_Tp>& stdex::math::complex<_Tp>::operator =(const _Tp& real) {
	real_=real;
	imag_=0;
    return *this;
}

template <typename _Tp>
template <typename _Up>
stdex::math::complex<_Tp>& stdex::math::complex<_Tp>::operator =(const stdex::math::complex<_Up>& other) {
	real_=other.real();
	imag_=other.imag();
    return *this;
}

template <typename _Tp>
constexpr stdex::math::complex<_Tp> stdex::math::complex<_Tp>::operator +() const noexcept {
	return *this;
}

template <typename _Tp>
constexpr stdex::math::complex<_Tp> stdex::math::complex<_Tp>::operator -() const noexcept {
	return stdex::math::complex<_Tp>(-real_,-imag_);
}

template <typename _Tp>
stdex::math::complex<_Tp>& stdex::math::complex<_Tp>::operator +=(const stdex::math::complex<_Tp>& other) noexcept {
	real_+=other.real_;
	imag_+=other.imag_;
	return *this;
}

template <typename _Tp>
template <typename _Up>
stdex::math::complex<_Tp>& stdex::math::complex<_Tp>::operator +=(const stdex::math::complex<_Up>& other) noexcept {
	real_+=other.real();
	imag_+=other.imag();
    return *this;
}

template <typename _Tp>
template <typename _Up,enable_if_arithmetic<_Up>=nullptr>
stdex::math::complex<_Tp>& stdex::math::complex<_Tp>::operator +=(const _Up& scalar) noexcept {
	real_+=scalar;
	return *this;
}

template <typename _Tp>
stdex::math::complex<_Tp>& stdex::math::complex<_Tp>::operator -=(const stdex::math::complex<_Tp>& other) noexcept {
	real_-=other.real_;
	imag_-=other.imag_;
	return *this;
}

template <typename _Tp>
template <typename _Up>
stdex::math::complex<_Tp>& stdex::math::complex<_Tp>::operator -=(const stdex::math::complex<_Up>& other) noexcept {
	real_-=other.real();
	imag_-=other.imag();
    return *this;
}

template <typename _Tp>
template <typename _Up,enable_if_arithmetic<_Up>=nullptr>
stdex::math::complex<_Tp>& stdex::math::complex<_Tp>::operator -=(const _Up& scalar) noexcept {
	real_-=scalar;
	return *this;
}

template <typename _Tp>
stdex::math::complex<_Tp>& stdex::math::complex<_Tp>::operator *=(const stdex::math::complex<_Tp>& other) noexcept {
	const _Tp r=real_*other.real_-imag_*other.imag_;
	const _Tp i=real_*other.imag_+imag_*other.real_;
	real_=r;
	imag_=i;
	return *this;
}

template <typename _Tp>
template <typename _Up>
stdex::math::complex<_Tp>& stdex::math::complex<_Tp>::operator *=(const stdex::math::complex<_Up>& other) noexcept {
	const _Tp r=real_*other.real()-imag_*other.imag();
	const _Tp i=real_*other.imag()+imag_*other.real();
	real_=r;
	imag_=i;
	return *this;
}

template <typename _Tp>
template <typename _Up,enable_if_arithmetic<_Up>=nullptr>
stdex::math::complex<_Tp>& stdex::math::complex<_Tp>::operator *=(const _Up& scalar) noexcept {
	real_*=scalar;
	imag_*=scalar;
	return *this;
}

template <typename _Tp>
stdex::math::complex<_Tp>& stdex::math::complex<_Tp>::operator /=(const stdex::math::complex<_Tp>& other) noexcept {
	const _Tp denom=other.real_*other.real_+other.imag_*other.imag_;
	if (denom==0) {
		throw std::domain_error("Complex division by zero");
	}   
	const _Tp r=(real_*other.real_+imag_*other.imag_)/denom;
	const _Tp i=(imag_*other.real_-real_*other.imag_)/denom;
	real_=r;
	imag_=i;
	return *this;
}

template <typename _Tp>
template <typename _Up>
stdex::math::complex<_Tp>& stdex::math::complex<_Tp>::operator /=(const stdex::math::complex<_Up>& other) noexcept {
	const _Tp denom=other.real()*other.real()+other.imag()*other.imag();
	if (denom==0) {
		throw std::domain_error("Complex division by zero");
	}
	const _Tp r=(real_*other.real()+imag_*other.imag())/denom;
	const _Tp i=(imag_*other.real()-real_*other.imag())/denom;
	real_=r;
	imag_=i;
	return *this;
}

template <typename _Tp>
template <typename _Up,enable_if_arithmetic<_Up>=nullptr>
stdex::math::complex<_Tp>& stdex::math::complex<_Tp>::operator /=(const _Up& scalar) noexcept {
	if (scalar==0) {
		throw std::domain_error("Complex division by zero");
	}
	real_/=scalar;
    imag_/=scalar;
    return *this;
}

template <typename _Tp>
constexpr stdex::math::complex<_Tp> stdex::math::complex<_Tp>::conj() const noexcept {
	return stdex::math::complex<_Tp>(real_,-imag_);
}

template <typename _Tp>
template <typename _Up=_Tp,enable_if_arithmetic<_Up>=nullptr>
constexpr _Tp stdex::math::complex<_Tp>::norm() const noexcept { 
	return real_*real_+imag_*imag_; 
}

template <typename _Tp>
template <typename _Up=_Tp,enable_if_floating<_Up>=nullptr>
_Tp stdex::math::complex<_Tp>::abs() const noexcept { 
	return std::hypot(real_,imag_); 
}

template <typename _Tp>
template <typename _Up=_Tp,enable_if_floating<_Up>=nullptr>
_Tp stdex::math::complex<_Tp>::arg() const noexcept { 
	return std::atan2(imag_,real_); 
}

template <typename _Tp>
template <typename _Up=_Tp,enable_if_floating<_Up>=nullptr>
stdex::math::complex<_Tp> stdex::math::complex<_Tp>::proj() const noexcept {
	if (std::isinf(real_) || std::isinf(imag_)) {
		return complex(std::numeric_limits<_Tp>::infinity(),std::copysign(_Tp(0),imag_));
	}
	return *this;
}

template <typename _Tp>
template <typename _Up=_Tp,enable_if_floating<_Up>=nullptr>
static stdex::math::complex<_Tp> stdex::math::complex<_Tp>::polar(const _Tp& r,const _Tp& theta) noexcept {
	return stdex::math::complex<_Tp>(r*std::cos(theta),r*std::sin(theta));
}

template <typename _Tp>
template <typename _Up=_Tp,enable_if_floating<_Up>=nullptr>
stdex::math::complex<_Tp> stdex::math::complex<_Tp>::exp() const noexcept {
	const _Tp exp_real=std::exp(real_);
	return stdex::math::complex<_Tp>(exp_real*std::cos(imag_),exp_real*std::sin(imag_));
}

template <typename _Tp>
template <typename _Up=_Tp,enable_if_floating<_Up>=nullptr>
stdex::math::complex<_Tp> stdex::math::complex<_Tp>::log() const noexcept {
	return stdex::math::complex<_Tp>(std::log(this->abs()),this->arg());
}

template <typename _Tp>
template <typename _Up=_Tp,enable_if_floating<_Up>=nullptr>
stdex::math::complex<_Tp> stdex::math::complex<_Tp>::pow(const stdex::math::complex<_Tp>& exponent) const {
	return (exponent*this->log()).exp();
}

template <typename _Tp>
template <typename _Up=_Tp,enable_if_floating<_Up>=nullptr>
stdex::math::complex<_Tp> stdex::math::complex<_Tp>::sqrt() const noexcept {
	const _Tp r=abs();
	return stdex::math::complex<_Tp>(std::sqrt((r+real_)/2),std::copysign(std::sqrt((r-real_)/2),imag_));
}

template <typename _Tp>
template <typename _Up=_Tp,enable_if_floating<_Up>=nullptr>
stdex::math::complex<_Tp> stdex::math::complex<_Tp>::sin() const noexcept {
	return stdex::math::complex<_Tp>(std::sin(real_)*std::cosh(imag_),std::cos(real_) * std::sinh(imag_));
}

template <typename _Tp>
template <typename _Up=_Tp,enable_if_floating<_Up>=nullptr>
stdex::math::complex<_Tp> stdex::math::complex<_Tp>::cos() const noexcept {
	return stdex::math::complex<_Tp>(std::cos(real_)*std::cosh(imag_),-std::sin(real_)*std::sinh(imag_));
}

template <typename _Tp>
template <typename _Up=_Tp,enable_if_floating<_Up>=nullptr>
stdex::math::complex<_Tp> stdex::math::complex<_Tp>::tan() const {
	const stdex::math::complex<_Tp> c=cos();
	if (c.real()==0 && c.imag()==0) {
		throw std::domain_error("Complex tangent undefined");
	}
	return sin()/c;
}

template <typename _Tp>
template <typename _Up=_Tp,enable_if_floating<_Up>=nullptr>
stdex::math::complex<_Tp> stdex::math::complex<_Tp>::sinh() const noexcept {
	return stdex::math::complex<_Tp>(std::sinh(real_)*std::cos(imag_),std::cosh(real_)*std::sin(imag_));
}

template <typename _Tp>
template <typename _Up=_Tp,enable_if_floating<_Up>=nullptr>
stdex::math::complex<_Tp> stdex::math::complex<_Tp>::cosh() const noexcept {
	return stdex::math::complex<_Tp>(std::cosh(real_)*std::cos(imag_),std::sinh(real_)*std::sin(imag_));
}

template <typename _Tp>
template <typename _Up=_Tp,enable_if_floating<_Up>=nullptr>
stdex::math::complex<_Tp> stdex::math::complex<_Tp>::tanh() const {
	const stdex::math::complex<_Tp> c=cosh();
	if (c.real()==0 && c.imag()==0) {
		throw std::domain_error("Complex hyperbolic tangent undefined");
	}
	return sinh()/c;
}

template <typename _Tp>
const _Tp& stdex::math::complex<_Tp>::real() const {
	return real_;
}

template <typename _Tp>
const _Tp& stdex::math::complex<_Tp>::imag() const {
	return imag_;
}

template <typename _Tp>
std::string stdex::math::complex<_Tp>::to_string() const {
	std::ostringstream oss;
	oss<<"("<<real_<<","<<imag_<<"i)";
	return oss.str();
}

template <typename _Tp>
constexpr stdex::math::quaternion<_Tp>::quaternion(const _Tp& w,const _Tp& x,const _Tp& y,const _Tp& z) noexcept : w_(w) , x_(x) , y_(y) , z_(z) { }

template <typename _Tp>
constexpr stdex::math::quaternion<_Tp>::quaternion(const _Tp& real) noexcept : w_(real) , x_(0) , y_(0) , z_(0) { }

template <typename _Tp>
template <typename _Up>
constexpr stdex::math::quaternion(const stdex::math::complex<_Up>& c) noexcept : w_(c.real()) , x_(c.imag()) , y_(0) , z_(0) { }

template <typename _Tp>
stdex::math::quaternion<_Tp>& stdex::math::quaternion<_Tp>::operator =(const _Tp& real) {
	w_=real_;
	x_=y_=z_=0;
}

template <typename _Tp>
template <typename _Up>
stdex::math::quaternion<_Tp>& stdex::math::quaternion<_Tp>::operator =(const stdex::math::complex<_Up>& other) {
	w_=other.real();
	x_=other.imag();
}

template <typename _Tp>
template <typename _Up>
stdex::math::quaternion<_Tp>& stdex::math::quaternion<_Tp>::operator =(const stdex::math::quaternion<_Up>& other) {
	w_=other.w();
	x_=other.x();
	y_=other.y();
	z_=other.z();
}

template <typename _Tp>
constexpr stdex::math::quaternion<_Tp> stdex::math::quaternion<_Tp>::operator +() const noexcept {
	return *this;
}

template <typename _Tp>
constexpr stdex::math::quaternion<_Tp> stdex::math::quaternion<_Tp>::operator -() const noexcept {
	return stdex::math::quaternion<_Tp>(-w_,-x_,-y_,-z_);
}

template <typename _Tp>
stdex::math::quaternion<_Tp>& stdex::math::quaternion<_Tp>::operator +=(const stdex::math::quaternion<_Tp>& other) noexcept {
	w_+=other.w_;
	x_+=other.x_;
	y_+=other.y_;
	z_+=other.z_;
    return *this;
}

template <typename _Tp>
template <typename _Up>
stdex::math::quaternion<_Tp>& stdex::math::quaternion<_Tp>::operator +=(const stdex::math::quaternion<_Up>& other) noexcept {
	w_+=other.w();
	x_+=other.x();
	y_+=other.y();
	z_+=other.z();
    return *this;
}

template <typename _Tp>
template <typename _Up,enable_if_arithmetic<_Up>=nullptr>
stdex::math::quaternion<_Tp>& stdex::math::quaternion<_Tp>::operator +=(const _Up& scalar) noexcept {
	w_+=scalar;
    return *this;
}

template <typename _Tp>
template <typename _Up,enable_if_arithmetic<_Up>=nullptr>
stdex::math::quaternion<_Tp>& stdex::math::quaternion<_Tp>::operator +=(const stdex::math::complex<_Up>& complexor) noexcept {
	w_+=complexor.real();
	x_+=complexor.imag();
    return *this;
}

template <typename _Tp>
stdex::math::quaternion<_Tp>& stdex::math::quaternion<_Tp>::operator -=(const stdex::math::quaternion<_Tp>& other) noexcept {
	w_-=other.w_;
	x_-=other.x_;
	y_-=other.y_;
	z_-=other.z_;
    return *this;
}

template <typename _Tp>
template <typename _Up>
stdex::math::quaternion<_Tp>& stdex::math::quaternion<_Tp>::operator -=(const stdex::math::quaternion<_Up>& other) noexcept {
	w_-=other.w();
	x_-=other.x();
	y_-=other.y();
	z_-=other.z();
    return *this;
}

template <typename _Tp>
template <typename _Up,enable_if_arithmetic<_Up>=nullptr>
stdex::math::quaternion<_Tp>& stdex::math::quaternion<_Tp>::operator -=(const _Up& scalar) noexcept {
	w_-=scalar;
    return *this;
}

template <typename _Tp>
template <typename _Up,enable_if_arithmetic<_Up>=nullptr>
stdex::math::quaternion<_Tp>& stdex::math::quaternion<_Tp>::operator -=(const stdex::math::complex<_Up>& complexor) noexcept {
	w_-=complexor.real();
	x_-=complexor.imag();
    return *this;
}

template <typename _Tp>
stdex::math::quaternion<_Tp>& stdex::math::quaternion<_Tp>::operator *=(const stdex::math::quaternion<_Tp>& other) noexcept {
	const _Tp w=w_*other.w_-x_*other.x_-y_*other.y_-z_*other.z_;
	const _Tp x=w_*other.x_+x_*other.w_+y_*other.z_-z_*other.y_;
	const _Tp y=w_*other.y_-x_*other.z_+y_*other.w_+z_*other.x_;
	const _Tp z=w_*other.z_+x_*other.y_-y_*other.x_+z_*other.w_;
	w_=w;
	x_=x;
	y_=y;
	z_=z;
	return *this;
}

template <typename _Tp>
template <typename _Up>
stdex::math::quaternion<_Tp>& stdex::math::quaternion<_Tp>::operator *=(const stdex::math::quaternion<_Up>& other) noexcept {
	const _Tp w=w_*other.w()-x_*other.x()-y_*other.y()-z_*other.z();
	const _Tp x=w_*other.x()+x_*other.w()+y_*other.z()-z_*other.y();
	const _Tp y=w_*other.y()-x_*other.z()+y_*other.w()+z_*other.x();
	const _Tp z=w_*other.z()+x_*other.y()-y_*other.x()+z_*other.w();
	w_=w;
	x_=x;
	y_=y;
	z_=z;
	return *this;
}

template <typename _Tp>
template <typename _Up,enable_if_arithmetic<_Up>=nullptr>
stdex::math::quaternion<_Tp>& stdex::math::quaternion<_Tp>::operator *=(const _Up& scalar) noexcept {
	w_*=scalar;
	x_*=scalar;
	y_*=scalar;
	z_*=scalar;
    return *this;
}

template <typename _Tp>
template <typename _Up,enable_if_arithmetic<_Up>=nullptr>
stdex::math::quaternion<_Tp>& stdex::math::quaternion<_Tp>::operator *=(const stdex::math::complex<_Up>& complexor) noexcept {
	const _Tp w=w_*complexor.real()-x_*complexor.imag();
	const _Tp x=w_*complexor.imag()+x_*complexor.real();
	const _Tp y=y_*complexor.real()+z_*complexor.imag();
	const _Tp z=y_*complexor.imag()+z_*complexor.real();
	w_=w;
	x_=x;
	y_=y;
	z_=z;
    return *this;
}

template <typename _Tp>
stdex::math::quaternion<_Tp>& stdex::math::quaternion<_Tp>::operator /=(const stdex::math::quaternion<_Tp>& other) noexcept {
	const _Tp norm=other.norm();
	if (norm==0) {
		throw std::domain_error("Quaternion division by zero");
	}
	*this*=other.inverse();
	return *this;
}

template <typename _Tp>
template <typename _Up>
stdex::math::quaternion<_Tp>& stdex::math::quaternion<_Tp>::operator /=(const stdex::math::quaternion<_Up>& other) noexcept {
	const _Up norm=other.norm();
	if (norm==0) {
		throw std::domain_error("Quaternion division by zero");
	}
	*this*=other.inverse();
	return *this;
}

template <typename _Tp>
template <typename _Up,enable_if_arithmetic<_Up>=nullptr>
stdex::math::quaternion<_Tp>& stdex::math::quaternion<_Tp>::operator *=(const _Up& scalar) noexcept {
	w_/=scalar;
	x_/=scalar;
	y_/=scalar;
	z_/=scalar;
    return *this;
}

template <typename _Tp>
template <typename _Up,enable_if_arithmetic<_Up>=nullptr>
stdex::math::quaternion<_Tp>& stdex::math::quaternion<_Tp>::operator *=(const stdex::math::complex<_Up>& complexor) noexcept {
	stdex::math::quaternion<_Up> temp(complexor);
	*this*=complexor;
    return *this;
}

template <typename _Tp>
constexpr stdex::math::quaternion<_Tp> stdex::math::quaternion<_Tp>::conj() const noexcept { 
	return stdex::math::quaternion<_Tp>(w_,-x_,-y_,-z_); 
}

template <typename _Tp>
template <typename _Up,enable_if_arithmetic<_Up>=nullptr>
constexpr _Tp stdex::math::quaternion<_Tp>::norm_sq() const noexcept { 
	return w_*w_+x_*x_+y_*y_+z_*z_; 
}

template <typename _Tp>
template <typename _Up=_Tp,enable_if_floating<_Up>=nullptr>
_Tp stdex::math::quaternion<_Tp>::norm() const noexcept { 
	return std::sqrt(norm_sq()); 
}

template <typename _Tp>
template <typename _Up,enable_if_arithmetic<_Up>=nullptr>
stdex::math::quaternion<_Tp> stdex::math::quaternion<_Tp>::inverse() const {
	const _Tp n=norm_sq();
	if (n==0) {
		throw std::domain_error("Quaternion has zero norm");
	}
	return conj()/n;
}

template <typename _Tp>
template <typename _Up=_Tp,enable_if_floating<_Up>=nullptr>
stdex::math::quaternion<_Tp> stdex::math::quaternion<_Tp>::unit() const {
	const _Tp n=norm();
	if (n==0) {
		throw std::domain_error("Quaternion has zero norm");
	}
	return *this/n;
}

template <typename _Tp>
_Tp dot(const stdex::math::quaternion<_Tp>& other) const noexcept {
	return w_*other.w_+x_*other.x_+y_*other.y_+z_*other.z_;
}

template <typename _Tp>
template <typename _Up>
_Tp dot(const stdex::math::quaternion<_Up>& other) const noexcept {
	return w_*other.w()+x_*other.x()+y_*other.y()+z_*other.z();
}