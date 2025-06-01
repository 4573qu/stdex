//Last Modified At 2024/08/28
//@Version 1.0
#ifndef _STD4573_MATH_GEOMETRY_SHAPE_H_
#define _STD4573_MATH_GEOMETRY_SHAPE_H_ 1
#include "../math.h"

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