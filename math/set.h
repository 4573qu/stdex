//Last Modified At 2025/06/01
//@Version 1.0.0.1
#ifndef _STD4573_MATH_SET_H_
#define _STD4573_MATH_SET_H_ 1
#include <algorithm>
#include <functional>
#include <map>
#include <stdexcept>
#include <vector>

#include "math.h"

namespace stdex {
	
namespace math {

template <typename _Tp>
std::vector<std::vector<_Tp>> cartesian_product(std::vector<std::vector<_Tp>>& input) {
	std::vector<std::vector<_Tp>> result;
	if (input.empty()) return result;    
	for (auto& it:input) {
		if (it.empty()) return std::vector<std::vector<_Tp>>();
	}
	result.push_back(std::vector<_Tp>());
	for (auto& it:input) { 
		std::vector<std::vector<_Tp>> temp;
		for (auto& jt:result) {
			for (auto& kt:it) {
				std::vector<_Tp> res=jt; 
				res.push_back(kt);  
				temp.push_back(res); 
			}
		}
		result.swap(temp);
	}
	return result;
}

template <typename _Tp>
std::vector<std::vector<_Tp>> power_set(std::vector<_Tp>& input) {
	std::vector<std::vector<_Tp>> result;
	size_t n=input.size();
	if (n>sizeof(size_t)*8) throw std::invalid_argument("The amount of elements must be less than or equal to "+std::to_string(sizeof(size_t)*8));
	size_t total=1ULL<<n;
	result.reserve(total);
	for (size_t mask=0;mask<total;mask++) {
		std::vector<_Tp> subset;
		for (int i=0;i<n;i++) {
			if (mask&(1ULL<<i)) subset.push_back(input[i]);
		}
		result.push_back(std::move(subset));
	}
	return result;
}

template <typename _Tp>
std::vector<std::vector<_Tp>> combinations(std::vector<_Tp>& input,int k) {
	std::vector<std::vector<_Tp>> result;
	if (k<0 || static_cast<size_t>(k)>input.size()) return result;
	std::vector<_Tp> current;
	current.reserve(k);
	std::function<void(size_t)> combine;
	combine=[&](size_t start) {
		if (current.size()==k) {
			result.push_back(current);
			return;
		}
		for (int i=start;i<input.size();i++) {
			current.push_back(input[i]);
			combine(i+1);
			current.pop_back();
		}
	};
	combine(0);
	return result;
}

template <typename _Tp>
std::vector<std::vector<_Tp>> permutations(std::vector<_Tp> input) {
	std::vector<std::vector<_Tp>> result;
	std::sort(input.begin(),input.end());
	std::function<void(size_t)> permute;
	permute=[&](size_t level) {
		if (level==input.size()) {
			result.push_back(input);
			return;
		}
		for (int i=level;i<input.size();i++) {
			if (i!=level && input[i]==input[level]) continue;
			std::swap(input[level],input[i]);
			permute(level+1);
			std::swap(input[level],input[i]);
		}
	};
	permute(0);
	return result;
}

template <typename _Tp>
std::vector<_Tp> multiset_intersection(std::vector<_Tp>& set1,std::vector<_Tp>& set2) {
	std::map<_Tp,int> counter;
	for (auto& it:set1) counter[it]++;
	std::vector<_Tp> result;
	for (auto& it:set2) {
		if (counter[it]>0) {
			result.push_back(it);
			counter[it]--;
		}
	}
	return result;
}

constexpr uint64_t factorial(int n) {
	uint64_t result=1;
	for (int i=2;i<=n;i++) result*=i;
	return result;
}

constexpr uint64_t choose_n_k(int n,int k) {
	if (k<0 || k>n) return 0;
	if (k==0 || k==n) return 1;
	k=(k>n-k)?(n-k):k;
	uint64_t result=1;
	for (int i=1;i<=k;i++) {
		result*=(n-k+i);
		result/=i;
	}
	return result;
}
	
}

}

#endif