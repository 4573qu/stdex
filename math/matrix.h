//Last Modified At 2024/08/28
//@Version 1.1
#ifndef _STD4573_MATH_MATRIX_H_
#define _STD4573_MATH_MATRIX_H_ 1
#define matrix_array matrix<int>
#include <stdexcept>
#include "math.h"

namespace std {
	
namespace math {
	
template <typename _Tp>
class matrix {
public:
	int x_dimension_,y_dimension_;
	_Tp** m;

public:
	matrix(int x_dimension,int y_dimension);
	matrix(int x_dimension,int y_dimension,initializer_list<_Tp> init_list);
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
	
	matrix<_Tp> adjoint() const;
	matrix<_Tp> inverse() const;
	matrix<_Tp> transposition() const;
	matrix<_Tp> power(int times) const;
	matrix<_Tp> expansion(int x,int y) const;
	
	matrix<_Tp> expand_to(int to_x,int to_y,int x_dimension,int y_dimension) const;
	matrix<_Tp> expand_to(int x_dimension,int y_dimension) const;
	matrix<_Tp> crop_to(int from_x,int from_y,int x_dimension,int y_dimension) const;
	matrix<_Tp> crop_to(int x_dimension,int y_dimension) const;
	
	bool is_square() const;
	_Tp value() const;
	string print() const;
	string print_with_squares() const;
};

matrix<double> transform_matrix3(double x,double y,double rotate,double scale_x,double scale_y);

}	

}

#endif