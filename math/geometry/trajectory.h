//Last Modified At 2024/08/28
//@Version 1.0
#ifndef _STD4573_MATH_GEOMETRY_TRAJECTORY_H_
#define _STD4573_MATH_GEOMETRY_TRAJECTORY_H_ 1
#include <vector>
#include "../math.h"
#include "point.h"

namespace std {
	
namespace math {

template <typename _Tp,template <typename> class _point=point2>	
class line {
public:
	_point<_Tp> start_,end_;
public:
	line() { }
	virtual vector<_point<_Tp> > get_points(int precision) {
		vector<_point<_Tp> > temp_points;
		for (int i=0;i<precision+1;i++) {
			_Tp t=(_Tp)i/(_Tp)precision;
			_point<_Tp> temp=end_-start_;
			temp=temp*t;
			temp+=start_;
			temp_points.push_back(temp);
		}
		return temp_points;
	}
};

template <typename _Tp,template <typename> class _point=point2>	
class curve : public line<_Tp,_point> {
public:
	vector<_point<_Tp> > controls_;	
public:
	curve() : line<_Tp,_point>() { }
	vector<_point<_Tp> > get_points(int precision/*,_Tp t0=(_Tp)0.5*/) override {
		/*if (t0<base_unit_trait<_Tp>::zero() || t0>base_unit_trait<_Tp>::value()) {
        	throw std::invalid_argument("t0 must between 0 and 1");
    	}*/
		int size=controls_.size()+1;
		vector<vector<_point<_Tp> > > temp_points(size+1),middle_points(size+1);
		temp_points[0].push_back(this->start_);
		temp_points[0].insert(temp_points[0].end(),controls_.begin(),controls_.end());
		temp_points[0].push_back(this->end_);
		/*for (int i=1;i<size+1;i++) {
			for (int j=0;j<size+1-i;j++) {
				temp_points[i].push_back(temp_points[i-1][j]*((_Tp)1.0-t0)+temp_points[i-1][j+1]*t0);
			}
		}*/
		middle_points[0]=temp_points[0];
		for (int i=0;i<precision+1;i++) {
			_Tp t=(_Tp)i/(_Tp)precision;
			for (int j=1;j<size;j++) {
				middle_points[j].clear();
				for (int k=0;k<size+1-j;k++) {
					middle_points[j].push_back(middle_points[j-1][k]*((_Tp)1.0-t)+middle_points[j-1][k+1]*t);
				}
			}
			middle_points[size].push_back(middle_points[size-1][0]*((_Tp)1.0-t)+middle_points[size-1][1]*t);
		}
		return middle_points[size];
	}
};

}

}

//#include "trajectory.cpp"
#endif