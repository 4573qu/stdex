//Last Modified At 2026/01/08
//@Version 1.0.0.0
#ifndef _STDEX_MATH_GEOMETRY_POINT_H_
#define _STDEX_MATH_GEOMETRY_POINT_H_ 1

#include <cstddef>
#include <type_traits>
#include <utility>

#include "../base.h"//At Least 1.0.0.2

namespace stdex {
	
namespace math {

namespace point_reference {

template <typename _Derived,typename _Tp,std::size_t _N,typename = void>
struct x_ref { };

template <typename _Derived,typename _Tp,std::size_t _N>
struct x_ref<_Derived,_Tp,_N,std::enable_if_t<(_N>=1)>> {
    _Tp& x_;
    constexpr x_ref() noexcept : x_(static_cast<_Derived*>(this)->coords_[0]) { }
};

template <typename _Derived,typename _Tp,std::size_t _N,typename=void>
struct y_ref { };

template <typename _Derived,typename _Tp,std::size_t _N>
struct y_ref<_Derived,_Tp,_N,std::enable_if_t<(_N>=2)>> {
    _Tp& y_;
    constexpr y_ref() noexcept : y_(static_cast<_Derived*>(this)->coords_[1]) { }
};

template <typename _Derived,typename _Tp,std::size_t _N,typename=void>
struct z_ref { };

template <typename _Derived,typename _Tp,std::size_t _N>
struct z_ref<_Derived,_Tp,_N,std::enable_if_t<(_N>=3)>> {
    _Tp& z_;
    constexpr z_ref() noexcept : z_(static_cast<_Derived*>(this)->coords_[2]) { }
};

template <typename _Derived,typename _Tp,std::size_t _N,typename=void>
struct w_ref { };

template <typename _Derived,typename _Tp,std::size_t _N>
struct w_ref<_Derived,_Tp,_N,std::enable_if_t<(_N>=4)>> {
    _Tp& w_;
    constexpr w_ref() noexcept : w_(static_cast<_Derived*>(this)->coords_[3]) {}
};

template <typename _Tp,typename=void>
struct export_y { };

template <typename _Tp>
struct export_y<_Tp,std::enable_if_t<(_Tp::dimension_>=2)>> {
    using typename _Tp::y_base;
    using y_base::y_;
};

template <typename _Tp,typename=void>
struct export_z { };

template <typename _Tp>
struct export_z<_Tp,std::enable_if_t<(_Tp::dimension_>=3)>> {
    using typename _Tp::z_base;
    using z_base::z_;
};

template <typename _Tp,typename=void>
struct export_w { };

template <typename _Tp>
struct export_w<_Tp,std::enable_if_t<(_Tp::dimension_>=4)>> {
    using typename _Tp::w_base;
    using w_base::w_;
};

}
	
template <typename _Tp,std::size_t _N>
class point : public point_reference::x_ref<point<_Tp,_N>,_Tp,_N> , public point_reference::y_ref<point<_Tp,_N>,_Tp,_N> , public point_reference::z_ref<point<_Tp,_N>,_Tp,_N> , public point_reference::w_ref<point<_Tp,_N>,_Tp,_N> , private point_reference::export_y<point<_Tp,_N>> , private point_reference::export_z<point<_Tp,_N>> , private point_reference::export_w<point<_Tp,_N>> {
	static_assert(_N>0,"_N must be positive.");
	static_assert(std::is_trivially_copyable<_Tp>::value,"_Tp must be trivially copyable for union aliasing.");

public:
	static constexpr std::size_t dimension_=_N;
	using x_base=point_reference::x_ref<point<_Tp,_N>,_Tp,_N>;
	using y_base=point_reference::y_ref<point<_Tp,_N>,_Tp,_N>;
	using z_base=point_reference::z_ref<point<_Tp,_N>,_Tp,_N>;
	using w_base=point_reference::w_ref<point<_Tp,_N>,_Tp,_N>;
	
	//using coords_struct=typename std::conditional<_N==1,struct { _Tp x_; },typename std::conditional<_N==2,struct { _Tp x_, y_; },typename std::conditional<_N==3,struct { _Tp x_, y_, z_; },struct { _Tp x_, y_, z_, w_; }>::type>::type>::type;
	_Tp coords_[_N]{};
	using point_reference::x_ref<point<_Tp,_N>,_Tp,_N>::x_;

public:
	point();
	point(_Tp (&coords)[_N]);
	~point();
	point(const point& other);
	point(point&& other) noexcept;
	
	point<_Tp,_N>& operator =(const point<_Tp,_N>& other);
	point<_Tp,_N>& operator =(point<_Tp,_N>&& other) noexcept;
	
	template <std::size_t _N2=_N>
	bool operator ==(const point<_Tp,_N2>& other) const;
	template <std::size_t _N2=_N>
	bool operator !=(const point<_Tp,_N2>& other) const;
	
	point<_Tp,_N> operator +(const point<_Tp,_N>& other) const;
	point<_Tp,_N>& operator +=(const point<_Tp,_N>& other);
	point<_Tp,_N> operator +(const _Tp& other) const;
	point<_Tp,_N> operator -(const point<_Tp,_N>& other) const;
	point<_Tp,_N>& operator -=(const point<_Tp,_N>& other);
	point<_Tp,_N> operator -(const _Tp& other) const;
	point<_Tp,_N> operator *(const point<_Tp,_N>& other) const;
	point<_Tp,_N>& operator *=(const point<_Tp,_N>&);
	point<_Tp,_N> operator *(const _Tp& other) const;
	point<_Tp,_N> operator /(const point<_Tp,_N>& other) const;
	point<_Tp,_N>& operator /=(const point<_Tp,_N>&);
	point<_Tp,_N> operator /(const _Tp& other) const;
	
	constexpr _Tp& operator [](std::size_t i) noexcept { return coords_[i]; }
	constexpr const _Tp& operator [](std::size_t i) const noexcept { return coords_[i]; }

	constexpr _Tp* coords() noexcept { return coords_; }
	constexpr const _Tp* data() const noexcept { return coords_; }

	std::string print() const;
};

template <typename _Tp>
using point2=point<_Tp,2>;

template <typename _Tp>
using point3=point<_Tp,3>;

}

}

#endif