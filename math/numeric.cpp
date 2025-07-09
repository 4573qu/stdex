//Last Modified At 2025/07/09
//@Version 1.0.0.0
//@H_Version 1.0.0.0
#include "numeric.h"

#include <algorithm>
#include <cctype>
#include <cstdexcept>

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
	bigint lhs_low,lhs_high,rhs_low,rhs_high;
	void (*process)()=[](const bigint& origin,bigint& low,bigint& high,size_t k) -> void {
		low.digits_.assign(origin.digits_.begin(),origin.digits_+std::min(k,origin.digits_.size()));
		if (origin.digits_.size()>k) {
			high.digits_.assign(origin.digits_.begin()+k,origin.digits_.end());
		}
	};
	process(lhs,lhs_low,lhs_high,k);
	process(rhs,rhs_low,rhs_high,k);
	bigint z0=multiply(lhs_low,rhs_low);
	bigint z2=multiply(lhs_high,rhs_high);
	bigint z1=multiply(lhs_low+lhs_high,rhs_low+rhs_high)-z0-z2;
	z2<<=2*k;
	z1<<=k;
	return z2+z1+z0;
}

stdex::math::bigint::bigint() : digits_(1,0) , negative_(false) { }

stdex::math::bigint::bigint(long long value) {
	negative_=(value<0);
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