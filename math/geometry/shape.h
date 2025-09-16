//Last Modified At 2025/09/16
//@Version 1.0.0.0
#ifndef _STDEX_MATH_GEOMETRY_SHAPE_H_
#define _STDEX_MATH_GEOMETRY_SHAPE_H_ 1
#include "../base.h"//At Least 1.0.0.2

namespace std {
	
namespace math {
	
template <typename _Tp>
class shape {
public:
	
public:
	/*point(_Tp x,_Tp y);
	~point();
	point(const point& other);
	point(point&& other) noexcept;
	
	point<_Tp>& operator =(const point<_Tp>& other);
	point<_Tp>& operator =(point<_Tp>&& other) noexcept;
	
	bool operator ==(const point<_Tp>& other) const;
	bool operator !=(const point<_Tp>& other) const;
	
	point<_Tp> operator +(const point<_Tp>& other) const;
	point<_Tp>& operator +=(const point<_Tp>& other);
	point<_Tp> operator -(const point<_Tp>& other) const;
	point<_Tp>& operator -=(const point<_Tp>& other);
	point<_Tp> operator *(const point<_Tp>& other) const;
	point<_Tp>& operator *=(const point<_Tp>&);
	point<_Tp> operator /(const point<_Tp>& other) const;
	point<_Tp>& operator /=(const point<_Tp>&);*/
	
	virtual bool contains(_Tp x,_Tp y);
	virtual _Tp area();
	
	//DELETE? string print() const;
	
};

}

}

//#include "shape.cpp"
#endif