//Last Modified At 2026/01/08
//@Version 1.0.0.2
#ifndef _STDEX_MATH_GEOMETRY_TRAJECTORY_H_
#define _STDEX_MATH_GEOMETRY_TRAJECTORY_H_ 1

#include <type_traits>
#include <vector>

#include "../base.h"
#include "point.h"//At Least 1.0

namespace stdex {
	
namespace math {

template <typename _Tp,std::size_t _N>	
class line {
	virtual point<_Tp,_N> get_cached(float position) {
		if (position<0 || position>1) throw std::invalid_argument("position must be between 0 and 1");
		point<_Tp,_N> result=end_-start_;
		result=result*position;
		result+=start_;
		return result;
	}

	virtual void before_get_points() { }

public:
	point<_Tp,_N> start_,end_;

public:
	line() { }
	virtual point<_Tp,_N> get(float position) {
		return get_cached(position);
	}
	vector<point<_Tp,_N>> get_points(std::size_t precision) {
		before_get_points();
		vector<point<_Tp,_N>> result;
		for (std::size_t i=0;i<precision+1;i++) result.push_back(get_cached((float)i/(float)precision));
		return result;
	}
};

template <typename _Tp,std::size_t _N>
class curve : public line<_Tp,_N> {
	mutable std::vector<std::vector<point<_Tp,_N>>> cache_;

private:
	static point<_Tp,_N> lerp_point(const point<_Tp,_N>& a,const point<_Tp,_N>& b,float t) {
		point<_Tp,_N> result;
		const float minus_t=1-t;
		for (std::size_t i=0;i<_N;i++) result.coords_[i]=a.coords_[i]*minus_t+b.coords_[i]*t;
		return result;
	}
	point<_Tp,_N> get_cached(float position) override {
		if (position<0 || position>1) throw std::invalid_argument("position must be between 0 and 1");
		const int n=(int)controls_.size()+1;
		if ((int)cache_.size()<n+1) cache_.resize(n+1);
		cache_[0].clear();
		cache_[0].reserve(n+1);
		cache_[0].push_back(start_);
		cache_[0].insert(cache_[0].end(),controls_.begin(),controls_.end());
		cache_[0].push_back(end_);
		for (int r=1;r<=n;r++) {
			cache_[r].clear();
			cache_[r].reserve(n+1-r);
			for (int i=0;i<=n-r;i++) cache_[r].push_back(lerp_point(cache_[r-1][i],cache_[r-1][i+1],position));
		}
		return cache_[n][0];
	}
	void before_get_points() override {
		cache_.clear();
	}

public:
	vector<point<_Tp,_N>> controls_;

public:
	curve() : line<_Tp,_N>() { }

	point<_Tp,_N> get(float position) override {
		if (position<0 || position>1) throw std::invalid_argument("position must be between 0 and 1");
		const int n=(int)controls_.size()+1;
		std::vector<_point<_Tp>> work;
		work.reserve(n+1);
		work.push_back(start_);
		work.insert(work.end(),controls_.begin(),controls_.end());
		work.push_back(end_);
		for (int r=1;r<=n;r++) {
			for (int i=0;i<=n-r;i++) work[i]=lerp_point(work[i],work[i+1],position);
		}
		return work[0];
	}
};

}

}

#endif