//Last Modified At 2026/01/03
//@Version 1.3.0.0
#ifndef _STDEX_MATH_MATRIX_H_
#define _STDEX_MATH_MATRIX_H_ 1
#define matrix_array matrix<int>

#include <cstddef>
#include <cstdint>
#include <initializer_list>

#include base.h"//At Least 1.0.0.2

namespace stdex {
	
namespace math {
	
template <typename _Tp>
class matrix {
public:
	std::size_t x_dimension_,y_dimension_;
	_Tp** m_;

public:
	matrix(std::size_t x_dimension,std::size_t y_dimension);
	matrix(std::size_t x_dimension,std::size_t y_dimension,std::initializer_list<_Tp> init_list);
	~matrix();
	matrix(const matrix& other);
	matrix(matrix&& other) noexcept;
	
	matrix<_Tp>& operator =(const matrix<_Tp>& other);
	matrix<_Tp>& operator =(matrix<_Tp>&& other) noexcept;
	
	bool operator ==(const matrix<_Tp>& other) const;
	bool operator !=(const matrix<_Tp>& other) const;
	
	matrix<_Tp> operator +(const matrix<_Tp>& other) const;
	matrix<_Tp>& operator +=(const matrix<_Tp>& other);
	matrix<_Tp> operator -(const matrix<_Tp>& other) const;
	matrix<_Tp>& operator -=(const matrix<_Tp>& other);
	matrix<_Tp> operator *(const matrix<_Tp>& other) const;
	matrix<_Tp>& operator *=(const matrix<_Tp>&) = delete;
	matrix<_Tp> operator /(const matrix<_Tp>& other) const;
	matrix<_Tp>& operator /=(const matrix<_Tp>&) = delete;

	_Tp& operator ()(std::size_t i,std::size_t j);
	const _Tp& operator ()(std::size_t i,std::size_t j) const;
	
	matrix<_Tp> adjoint() const;
	matrix<_Tp> inverse() const;
	matrix<_Tp> transposition() const;
	matrix<_Tp> power(int times) const;
	matrix<_Tp> expansion(size_t x,size_t y) const;
	
	matrix<_Tp> expand_to(std::size_t to_x,std::size_t to_y,std::size_t x_dimension,std::size_t y_dimension) const;
	matrix<_Tp> expand_to(std::size_t x_dimension,std::size_t y_dimension) const;
	matrix<_Tp> crop_to(std::size_t from_x,std::size_t from_y,std::size_t x_dimension,std::size_t y_dimension) const;
	matrix<_Tp> crop_to(std::size_t x_dimension,std::size_t y_dimension) const;
	
	bool is_square() const;
	_Tp value() const;
	_Tp trace() const;

	bool is_valid() const;
	static matrix<_Tp> identity(std::size_t n);
	void fill(const _Tp& value);
	template <typename _Func>
	matrix<_Tp> map(_Func func) const {
		matrix<_Tp> result(x_dimension_,y_dimension_);
		for (std::size_t i=0;i<x_dimension_;i++) {
			for (std::size_t j=0;j<y_dimension_;j++) result(i,j)=func((*this)(i,j),i,j);
		}
		return result;
	}

	_Tp determinant2() const;
	_Tp determinant3() const;

	std::string print() const;
	std::string print_with_squares() const;
};

matrix<double> transform_matrix3(double x,double y,double rotate,double scale_x,double scale_y);

}	

}

#endif