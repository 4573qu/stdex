//Last Modified At 2025/09/16
//@Version 1.0.0.0
#ifndef _STDEX_MATH_GEOMETRY_POINT_H_
#define _STDEX_MATH_GEOMETRY_POINT_H_ 1
#include "../base.h"//At Least 1.0.0.2

namespace std {
	
namespace math {
	
template <typename _Tp>
class point2 {//point3 is for xyz
public:
	_Tp x_,y_;
public:
	point2();
	point2(_Tp x,_Tp y);
	~point2();
	point2(const point2& other);
	point2(point2&& other) noexcept;
	
	point2<_Tp>& operator =(const point2<_Tp>& other);
	point2<_Tp>& operator =(point2<_Tp>&& other) noexcept;
	
	bool operator ==(const point2<_Tp>& other) const;
	bool operator !=(const point2<_Tp>& other) const;
	
	point2<_Tp> operator +(const point2<_Tp>& other) const;
	point2<_Tp>& operator +=(const point2<_Tp>& other);
	point2<_Tp> operator +(const _Tp& other) const;
	point2<_Tp> operator -(const point2<_Tp>& other) const;
	point2<_Tp>& operator -=(const point2<_Tp>& other);
	point2<_Tp> operator -(const _Tp& other) const;
	point2<_Tp> operator *(const point2<_Tp>& other) const;
	point2<_Tp>& operator *=(const point2<_Tp>&);
	point2<_Tp> operator *(const _Tp& other) const;
	point2<_Tp> operator /(const point2<_Tp>& other) const;
	point2<_Tp>& operator /=(const point2<_Tp>&);
	point2<_Tp> operator /(const _Tp& other) const;
	
	string print() const;
	
};

}

}

#endif