//Last Modified At 2026/06/04
//@Version 1.1.0.0
#ifndef _STDEX_MATH_GEOMETRY_POINT_H_
#define _STDEX_MATH_GEOMETRY_POINT_H_ 1

#include <algorithm>
#include <cstddef>
#include <initializer_list>
#include <stdexcept>
#include <type_traits>
#include <utility>

#include "../foundations.h"//At Least 1.0.0.2

namespace stdex {
	
namespace math {

namespace point_reference {

template <typename _Derived,typename _Tp,intptr_t _N,typename = void>
struct x_ref { };

template <typename _Derived,typename _Tp,intptr_t _N>
struct x_ref<_Derived,_Tp,_N,std::enable_if_t<(_N>=1)>> {
    _Tp& x;
    constexpr x_ref() noexcept : x(static_cast<_Derived*>(this)->coords[0]) { }
};

template <typename _Derived,typename _Tp,intptr_t _N,typename=void>
struct y_ref { };

template <typename _Derived,typename _Tp,intptr_t _N>
struct y_ref<_Derived,_Tp,_N,std::enable_if_t<(_N>=2)>> {
    _Tp& y;
    constexpr y_ref() noexcept : y(static_cast<_Derived*>(this)->coords[1]) { }
};

template <typename _Derived,typename _Tp,intptr_t _N,typename=void>
struct z_ref { };

template <typename _Derived,typename _Tp,intptr_t _N>
struct z_ref<_Derived,_Tp,_N,std::enable_if_t<(_N>=3)>> {
    _Tp& z;
    constexpr z_ref() noexcept : z(static_cast<_Derived*>(this)->coords[2]) { }
};

template <typename _Derived,typename _Tp,intptr_t _N,typename=void>
struct w_ref { };

template <typename _Derived,typename _Tp,intptr_t _N>
struct w_ref<_Derived,_Tp,_N,std::enable_if_t<(_N>=4)>> {
    _Tp& w;
    constexpr w_ref() noexcept : w(static_cast<_Derived*>(this)->coords[3]) { }
};

template <typename _Tp,typename=void>
struct export_y { };

template <typename _Tp>
struct export_y<_Tp,std::enable_if_t<(_Tp::dimension_>=2)>> {
    using typename _Tp::y_base;
    using y_base::y;
};

template <typename _Tp,typename=void>
struct export_z { };

template <typename _Tp>
struct export_z<_Tp,std::enable_if_t<(_Tp::dimension_>=3)>> {
    using typename _Tp::z_base;
    using z_base::z;
};

template <typename _Tp,typename=void>
struct export_w { };

template <typename _Tp>
struct export_w<_Tp,std::enable_if_t<(_Tp::dimension_>=4)>> {
    using typename _Tp::w_base;
    using w_base::w;
};

}
	
template <typename _Tp,intptr_t _N>
class point : public point_reference::x_ref<point<_Tp,_N>,_Tp,_N> , public point_reference::y_ref<point<_Tp,_N>,_Tp,_N> , public point_reference::z_ref<point<_Tp,_N>,_Tp,_N> , public point_reference::w_ref<point<_Tp,_N>,_Tp,_N> , private point_reference::export_y<point<_Tp,_N>> , private point_reference::export_z<point<_Tp,_N>> , private point_reference::export_w<point<_Tp,_N>> {
	static_assert(_N>0,"_N must be positive.");
	static_assert(std::is_trivially_copyable<_Tp>::value,"_Tp must be trivially copyable for union aliasing.");

public:
	static constexpr std::size_t dimension=_N;
	using x_base=point_reference::x_ref<point<_Tp,_N>,_Tp,_N>;
	using y_base=point_reference::y_ref<point<_Tp,_N>,_Tp,_N>;
	using z_base=point_reference::z_ref<point<_Tp,_N>,_Tp,_N>;
	using w_base=point_reference::w_ref<point<_Tp,_N>,_Tp,_N>;
	
	_Tp coords[_N]{};
	using point_reference::x_ref<point<_Tp,_N>,_Tp,_N>::x;

	point();
	point(const _Tp (&coords)[_N]);
	template <typename... _Args,typename=std::enable_if_t<sizeof...(_Args)==_N>>
	point(_Args&&... args) : coords{std::forward<_Args>(args)...} {
		static_assert((std::is_convertible_v<_Args,_Tp> && ...),"All arguments must be implicitly convertible to type _Tp.");
	}
	point(std::initializer_list<_Tp> init_list) {
		if (init_list.size()!=_N) throw std::invalid_argument("The amount of the initializer arguments for point must be _N");
		std::copy(init_list.begin(),init_list.end(),coords);
	}
	~point();
	point(const point& other);
	point(point&& other) noexcept;
	
	point<_Tp,_N>& operator =(const point<_Tp,_N>& other);
	point<_Tp,_N>& operator =(point<_Tp,_N>&& other) noexcept;
	
	template <intptr_t _N2=_N>
	bool operator ==(const point<_Tp,_N2>& other) const;
	template <intptr_t _N2=_N>
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
	
	constexpr _Tp& operator [](intptr_t i) noexcept { 
		if (i<0 || i>=dimension) throw std::out_of_range("operator [] out of range");
		return coords[i];
	}
	constexpr const _Tp& operator [](intptr_t i) const noexcept {
		if (i<0 || i>=dimension) throw std::out_of_range("operator [] out of range");
		return coords[i];
	}

	std::string print() const;
};

template <typename _Tp>
using point2=point<_Tp,2>;

template <typename _Tp>
using point3=point<_Tp,3>;

}

}

#endif