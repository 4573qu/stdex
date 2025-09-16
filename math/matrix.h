//Last Modified At 2025/09/16
//@Version 1.2.1.1
#ifndef _STDEX_MATH_MATRIX_H_
#define _STDEX_MATH_MATRIX_H_ 1
#define matrix_array matrix<int>

#include <cstddef>
#include <initializer_list>

#include base.h"//At Least 1.0.0.2

namespace stdex {
	
namespace math {
	
template <typename _Tp>
class matrix {
public:
	size_t x_dimension_,y_dimension_;
	_Tp** m_;

public:
	matrix(size_t x_dimension,size_t y_dimension);
	matrix(size_t x_dimension,size_t y_dimension,std::initializer_list<_Tp> init_list);
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

	_Tp& operator ()(size_t i,size_t j);
	const _Tp& operator ()(size_t i,size_t j) const;
	
	matrix<_Tp> adjoint() const;
	matrix<_Tp> inverse() const;
	matrix<_Tp> transposition() const;
	matrix<_Tp> power(int times) const;
	matrix<_Tp> expansion(size_t x,size_t y) const;
	
	matrix<_Tp> expand_to(size_t to_x,size_t to_y,size_t x_dimension,size_t y_dimension) const;
	matrix<_Tp> expand_to(size_t x_dimension,size_t y_dimension) const;
	matrix<_Tp> crop_to(size_t from_x,size_t from_y,size_t x_dimension,size_t y_dimension) const;
	matrix<_Tp> crop_to(size_t x_dimension,size_t y_dimension) const;
	
	bool is_square() const;
	_Tp value() const;
	std::string print() const;
	std::string print_with_squares() const;
};

matrix<double> transform_matrix3(double x,double y,double rotate,double scale_x,double scale_y);

}	

}

#endif