//Last Modified At 2024/08/28
//@Version 1.0
//@H_Version 1.0
#include "point.h"
template <typename _Tp>
std::math::point2<_Tp>::point2() : x_(stdex::math::base_unit_trait<_Tp>::zero()) , y_(base_unit_trait<_Tp>::zero()) { }

template <typename _Tp>
std::math::point2<_Tp>::point2(_Tp x,_Tp y) : x_(x) , y_(y) { }

template <typename _Tp>
std::math::point2<_Tp>::~point2() { }

template <typename _Tp>
std::math::point2<_Tp>::point2(const std::math::point2<_Tp>& other) : x_(other.x_) , y_(other.y_) { }

template <typename _Tp>
std::math::point2<_Tp>::point2(std::math::point2<_Tp>&& other) noexcept : x_(other.x_) , y_(other.y_) {
	other.x_=0;
	other.y_=0;
}

template <typename _Tp>
std::math::point2<_Tp>& std::math::point2<_Tp>::operator =(const std::math::point2<_Tp>& other) {
	if (this!=&other) {
		x_=other.x_;
		y_=other.y_;
	}
	return *this;
}

template <typename _Tp>
std::math::point2<_Tp>& std::math::point2<_Tp>::operator =(std::math::point2<_Tp>&& other) noexcept {
	if (this!=&other) {
        x_=other.x_;
		y_=other.y_;
		other.x_=0;
		other.y_=0;
	}
	return *this;
}

template <typename _Tp>
bool std::math::point2<_Tp>::operator ==(const std::math::point2<_Tp>& other) const {
	if (x_ != other.x_ || y_ != other.y_) {
		return false;
	}
	return true;
}

template <typename _Tp>
bool std::math::point2<_Tp>::operator !=(const std::math::point2<_Tp>& other) const {
	return !((*this)==other);
}

template <typename _Tp>
std::math::point2<_Tp> std::math::point2<_Tp>::operator +(const std::math::point2<_Tp>& other) const {
	std::math::point2<_Tp> result(x_,y_);
	result.x_=x_+other.x_;
	result.y_=y_+other.y_;
	return result;
}

template <typename _Tp>
std::math::point2<_Tp>& std::math::point2<_Tp>::operator +=(const std::math::point2<_Tp>& other) {
	x_+=other.x_;
	y_+=other.y_;
	return *this;
}

template <typename _Tp>
std::math::point2<_Tp> std::math::point2<_Tp>::operator +(const _Tp& other) const {
	std::math::point2<_Tp> result(x_,y_);
	result.x_=x_+other;
	result.y_=y_+other;
	return result;
}

template <typename _Tp>
std::math::point2<_Tp> std::math::point2<_Tp>::operator -(const std::math::point2<_Tp>& other) const {
	std::math::point2<_Tp> result(x_,y_);
	result.x_=x_-other.x_;
	result.y_=y_-other.y_;
	return result;
}

template <typename _Tp>
std::math::point2<_Tp>& std::math::point2<_Tp>::operator -=(const std::math::point2<_Tp>& other) {
	x_-=other.x_;
	y_-=other.y_;
	return *this;
}

template <typename _Tp>
std::math::point2<_Tp> std::math::point2<_Tp>::operator -(const _Tp& other) const {
	std::math::point2<_Tp> result(x_,y_);
	result.x_=x_-other;
	result.y_=y_-other;
	return result;
}

template <typename _Tp>
std::math::point2<_Tp> std::math::point2<_Tp>::operator *(const std::math::point2<_Tp>& other) const {
	std::math::point2<_Tp> result(x_,y_);
	result.x_=x_*other.x_;
	result.y_=y_*other.y_;
	return result;
}

template <typename _Tp>
std::math::point2<_Tp>& std::math::point2<_Tp>::operator *=(const std::math::point2<_Tp>& other) {
	x_*=other.x_;
	y_*=other.y_;
	return *this;
}

template <typename _Tp>
std::math::point2<_Tp> std::math::point2<_Tp>::operator *(const _Tp& other) const {
	std::math::point2<_Tp> result(x_,y_);
	result.x_=x_*other;
	result.y_=y_*other;
	return result;
}

template <typename _Tp>
std::math::point2<_Tp> std::math::point2<_Tp>::operator /(const std::math::point2<_Tp>& other) const {
	std::math::point2<_Tp> result(x_,y_);
	result.x_=x_/other.x_;
	result.y_=y_/other.y_;
	return result;
}

template <typename _Tp>
std::math::point2<_Tp>& std::math::point2<_Tp>::operator /=(const std::math::point2<_Tp>& other) {
	x_/=other.x_;
	y_/=other.y_;
	return *this;
}

template <typename _Tp>
std::math::point2<_Tp> std::math::point2<_Tp>::operator /(const _Tp& other) const {
	std::math::point2<_Tp> result(x_,y_);
	result.x_=x_/other;
	result.y_=y_/other;
	return result;
}

template <typename _Tp>
std::string std::math::point2<_Tp>::print() const {
	std::string result="x="+stdex::math::base_unit_trait<_Tp>::to_string(x_)+",y="+stdex::math::base_unit_trait<_Tp>::to_string(y_);
	return result;
}
