//Last Modified At 2026/01/08
//@Version 1.0.0.0
//@H_Version 1.0.0.0
#include "point.h"

template <typename _Tp,std::size_t _N>
stdex::math::point<_Tp,_N>::point() : stdex::math::point_reference::x_ref<point<_Tp,_N>,_Tp,_N>() , stdex::math::point_reference::y_ref<point<_Tp,_N>,_Tp,_N>() , stdex::math::point_reference::z_ref<point<_Tp,_N>,_Tp,_N>() , stdex::math::point_reference::w_ref<point<_Tp,_N>,_Tp,_N>() {
	for (std::size_t i=0;i<_N;i++) coords_[i]=stdex::math::base_unit_trait<_Tp>::zero();
}

template <typename _Tp,std::size_t _N>
stdex::math::point<_Tp,_N>::point(_Tp (&coords)[_N]) : point() {
	for (std::size_t i=0;i<_N;i++) coords_[i]=coords[i];
}

template <typename _Tp,std::size_t _N>
stdex::math::point<_Tp,_N>::~point() { }

template <typename _Tp,std::size_t _N>
stdex::math::point<_Tp,_N>::point(const stdex::math::point<_Tp,_N>& other) : point() {
	for (int i=0;i<_N;i++) coords_[i]=other.coords_[i];
}

template <typename _Tp,std::size_t _N>
stdex::math::point<_Tp,_N>::point(stdex::math::point<_Tp,_N>&& other) noexcept : point() {
	for (int i=0;i<_N;i++) coords_[i]=std::move(other.coords_[i]);
}

template <typename _Tp,std::size_t _N>
stdex::math::point<_Tp,_N>& stdex::math::point<_Tp,_N>::operator =(const stdex::math::point<_Tp,_N>& other) {
	if (this!=&other) {
		for (int i=0;i<_N;i++) coords_[i]=other.coords_[i];
	}
	return *this;
}

template <typename _Tp,std::size_t _N>
stdex::math::point<_Tp,_N>& stdex::math::point<_Tp,_N>::operator =(stdex::math::point<_Tp,_N>&& other) noexcept {
	if (this!=&other) {
		for (int i=0;i<_N;i++) coords_[i]=std::move(other.coords_[i]);
	}
	return *this;
}

template <typename _Tp,std::size_t _N>
template <std::size_t _N2>
bool stdex::math::point<_Tp,_N>::operator ==(const stdex::math::point<_Tp,_N2>& other) const {
	if (_N!=_N2) return false;
	for (int i=0;i<_N;i++) {
		if (coords_[i]!=other.coords_[i]) return false;
	}
	return true;
}

template <typename _Tp,std::size_t _N>
template <std::size_t _N2>
bool stdex::math::point<_Tp,_N>::operator !=(const stdex::math::point<_Tp,_N2>& other) const {
	return !(*this==other);
}

template <typename _Tp,std::size_t _N>
stdex::math::point<_Tp,_N> stdex::math::point<_Tp,_N>::operator +(const stdex::math::point<_Tp,_N>& other) const {
	stdex::math::point<_Tp,_N> result(coords_);
	return result+=other;
}

template <typename _Tp,std::size_t _N>
stdex::math::point<_Tp,_N>& stdex::math::point<_Tp,_N>::operator +=(const stdex::math::point<_Tp,_N>& other) {
	for (int i=0;i<_N;i++) coords_[i]+=other.coords_[i];
	return *this;
}

template <typename _Tp,std::size_t _N>
stdex::math::point<_Tp,_N> stdex::math::point<_Tp,_N>::operator +(const _Tp& other) const {
	stdex::math::point<_Tp,_N> result(coords_);
	for (int i=0;i<_N;i++) result.coords_[i]+=other;
	return result;
}

template <typename _Tp,std::size_t _N>
stdex::math::point<_Tp,_N> stdex::math::point<_Tp,_N>::operator -(const stdex::math::point<_Tp,_N>& other) const {
	stdex::math::point<_Tp,_N> result(coords_);
	return result-=other;
}

template <typename _Tp,std::size_t _N>
stdex::math::point<_Tp,_N>& stdex::math::point<_Tp,_N>::operator -=(const stdex::math::point<_Tp,_N>& other) {
	for (int i=0;i<_N;i++) coords_[i]-=other.coords_[i];
	return *this;
}

template <typename _Tp,std::size_t _N>
stdex::math::point<_Tp,_N> stdex::math::point<_Tp,_N>::operator -(const _Tp& other) const {
	stdex::math::point<_Tp,_N> result(coords_);
	for (int i=0;i<_N;i++) result.coords_[i]-=other;
	return result;
}

template <typename _Tp,std::size_t _N>
stdex::math::point<_Tp,_N> stdex::math::point<_Tp,_N>::operator *(const stdex::math::point<_Tp,_N>& other) const {
	stdex::math::point<_Tp,_N> result(coords_);
	return result*=other;
}

template <typename _Tp,std::size_t _N>
stdex::math::point<_Tp,_N>& stdex::math::point<_Tp,_N>::operator *=(const stdex::math::point<_Tp,_N>& other) {
	for (int i=0;i<_N;i++) coords_[i]*=other.coords_[i];
	return *this;
}

template <typename _Tp,std::size_t _N>
stdex::math::point<_Tp,_N> stdex::math::point<_Tp,_N>::operator *(const _Tp& other) const {
	stdex::math::point<_Tp,_N> result(coords_);
	for (int i=0;i<_N;i++) result.coords_[i]*=other;
	return result;
}

template <typename _Tp,std::size_t _N>
stdex::math::point<_Tp,_N> stdex::math::point<_Tp,_N>::operator /(const stdex::math::point<_Tp,_N>& other) const {
	stdex::math::point<_Tp,_N> result(coords_);
	return result/=other;
}

template <typename _Tp,std::size_t _N>
stdex::math::point<_Tp,_N>& stdex::math::point<_Tp,_N>::operator /=(const stdex::math::point<_Tp,_N>& other) {
	for (int i=0;i<_N;i++) coords_[i]/=other.coords_[i];
	return *this;
}

template <typename _Tp,std::size_t _N>
stdex::math::point<_Tp,_N> stdex::math::point<_Tp,_N>::operator /(const _Tp& other) const {
	stdex::math::point<_Tp,_N> result(coords_);
	for (int i=0;i<_N;i++) result.coords_[i]/=other;
	return result;
}

template <typename _Tp,std::size_t _N>
std::string stdex::math::point<_Tp,_N>::print() const {
	std::string result="(";
	for (int i=0;i<_N;i++) {
		result+=stdex::math::base_unit_trait<_Tp>::to_string(coords_[i]);
		if (i!=_N-1) result+=",";
	}
	result+=")";
	return result;
}
