//Last Modified At 2024/08/28
//@Version 1.0
//@H_Version 1.1
#include "matrix.h"
template <typename _Tp>
std::math::matrix<_Tp>::matrix(int x_dimension,int y_dimension) : x_dimension_(x_dimension) , y_dimension_(y_dimension) {
	if (x_dimension<1 || y_dimension<1) {
		throw std::invalid_argument("Matrix dimensions do not match for create");
	}
	m=new _Tp*[x_dimension];
	for (int i=0;i<x_dimension;i++) {
		m[i]=new _Tp[y_dimension];
		for (int j=0;j<y_dimension;j++) {
			m[i][j]=std::math::base_unit_trait<_Tp>::zero();
		}
	}
}

template <typename _Tp>
std::math::matrix<_Tp>::matrix(int x_dimension,int y_dimension,std::initializer_list<_Tp> init_list) : x_dimension_(x_dimension) , y_dimension_(y_dimension) {
	if (init_list.size()!=x_dimension*y_dimension) {
        throw std::invalid_argument("Initializer list size does not match matrix dimensions");
    }
	m=new _Tp*[x_dimension];
	typename std::initializer_list<_Tp>::iterator it=init_list.begin();
	for (int i=0;i<x_dimension;i++) {
		m[i]=new _Tp[y_dimension];
		for (int j=0;j<y_dimension;j++) {
			m[i][j]=*it++;
		}
	}
}

template <typename _Tp>
std::math::matrix<_Tp>::~matrix() {
	for (int i=0;i<x_dimension_;i++) {
		delete[] m[i];
	}
	delete[] m;
}

template <typename _Tp>
std::math::matrix<_Tp>::matrix(const std::math::matrix<_Tp>& other) : x_dimension_(other.x_dimension_) , y_dimension_(other.y_dimension_) {
	m=new _Tp*[x_dimension_];
	for(int i=0;i<x_dimension_;i++) {
		m[i]=new _Tp[y_dimension_];
		std::copy(other.m[i],other.m[i]+y_dimension_,m[i]);
	}
}

template <typename _Tp>
std::math::matrix<_Tp>::matrix(std::math::matrix<_Tp>&& other) noexcept : x_dimension_(other.x_dimension_) , y_dimension_(other.y_dimension_), m(other.m) {
	other.m=nullptr;
	other.x_dimension_=0;
	other.y_dimension_=0;
}

template <typename _Tp>
std::math::matrix<_Tp>& std::math::matrix<_Tp>::operator =(const std::math::matrix<_Tp>& other) {
	if (this!=&other) {
		for (int i=0;i<x_dimension_;i++) {
			delete[] m[i];
		}
		delete[] m;
        x_dimension_=other.x_dimension_;
        y_dimension_=other.y_dimension_;
        m=new _Tp*[x_dimension_];
		for (int i=0;i<x_dimension_;i++) {
			m[i]=new _Tp[y_dimension_];
			for (int j=0;j<y_dimension_;j++) {
				m[i][j]=other.m[i][j];
			}
		}
	}
	return *this;
}

template <typename _Tp>
std::math::matrix<_Tp>& std::math::matrix<_Tp>::operator =(std::math::matrix<_Tp>&& other) noexcept {
	if (this!=&other) {
		for (int i=0;i<x_dimension_;i++) {
			delete[] m[i];
		}
		delete[] m;
		x_dimension_=other.x_dimension_;
		y_dimension_=other.y_dimension_;
		m=other.m;
		other.m=nullptr;
		other.x_dimension_=0;
		other.y_dimension_=0;
	}
	return *this;
}

template <typename _Tp>
bool std::math::matrix<_Tp>::operator ==(const std::math::matrix<_Tp>& other) const {
	if (x_dimension_ != other.x_dimension_ || y_dimension_ != other.y_dimension_) {
		return false;
	}
	for (int i=0;i<x_dimension_;i++) {
		for (int j=0;j<y_dimension_;j++) {
			if (m[i][j]!=other.m[i][j]) {
				return false;
			}
		}
	}
	return true;
}

template <typename _Tp>
bool std::math::matrix<_Tp>::operator !=(const std::math::matrix<_Tp>& other) const {
	return !((*this)==other);
}

template <typename _Tp>
std::math::matrix<_Tp> std::math::matrix<_Tp>::operator +(const std::math::matrix<_Tp>& other) const {
	if (x_dimension_ != other.x_dimension_ || y_dimension_ != other.y_dimension_) {
		throw std::invalid_argument("Matrices dimensions do not match for addition");
	}
	std::math::matrix<_Tp> result(x_dimension_,y_dimension_);
	for (int i=0;i<x_dimension_;i++) {
		for (int j=0;j<y_dimension_;j++) {
			result.m[i][j]=m[i][j]+other.m[i][j];
		}
	}
	return result;
}

template <typename _Tp>
std::math::matrix<_Tp>& std::math::matrix<_Tp>::operator +=(const std::math::matrix<_Tp>& other) {
	if (x_dimension_ != other.x_dimension_ || y_dimension_ != other.y_dimension_) {
		throw std::invalid_argument("Matrices dimensions do not match for addition");
	}
	for (int i=0;i<x_dimension_;i++) {
		for (int j=0;j<y_dimension_;j++) {
			m[i][j]+=other.m[i][j];
		}
	}
	return *this;
}

template <typename _Tp>
std::math::matrix<_Tp> std::math::matrix<_Tp>::operator -(const std::math::matrix<_Tp>& other) const {
	if (x_dimension_ != other.x_dimension_ || y_dimension_ != other.y_dimension_) {
		throw std::invalid_argument("Matrices dimensions do not match for substraction");
	}
	std::math::matrix<_Tp> result(x_dimension_,y_dimension_);
	for (int i=0;i<x_dimension_;i++) {
		for (int j=0;j<y_dimension_;j++) {
			result.m[i][j]=m[i][j]-other.m[i][j];
		}
	}
	return result;
}

template <typename _Tp>
std::math::matrix<_Tp>& std::math::matrix<_Tp>::operator -=(const std::math::matrix<_Tp>& other) {
	if (x_dimension_ != other.x_dimension_ || y_dimension_ != other.y_dimension_) {
		throw std::invalid_argument("Matrices dimensions do not match for substraction");
	}
	for (int i=0;i<x_dimension_;i++) {
		for (int j=0;j<y_dimension_;j++) {
			m[i][j]-=other.m[i][j];
		}
	}
	return *this;
}

template <typename _Tp>
std::math::matrix<_Tp> std::math::matrix<_Tp>::operator *(const std::math::matrix<_Tp>& other) const {
	if (y_dimension_ != other.x_dimension_) {
		throw std::invalid_argument("Matrices dimensions do not match for multiply");
	}
	std::math::matrix<_Tp> result(x_dimension_,other.y_dimension_);
	for (int i=0;i<x_dimension_;i++) {
		for (int j=0;j<other.y_dimension_;j++) {
			_Tp temp_value=std::math::base_unit_trait<_Tp>::zero();
			for (int k=0;k<y_dimension_;k++) temp_value=temp_value+m[i][k]*other.m[k][j];
			result.m[i][j]=temp_value;
		}
	}
	return result;
}

template <typename _Tp>
std::math::matrix<_Tp> std::math::matrix<_Tp>::operator /(const std::math::matrix<_Tp>& other) const {
	if (!other.is_square() || y_dimension_!=other.y_dimension_) {
		throw std::invalid_argument("Matrices dimensions do not match for division");
	}
	std::math::matrix<_Tp> result=other.inverse();
	return (*this)*result;
}

template <typename _Tp>
std::math::matrix<_Tp> std::math::matrix<_Tp>::adjoint() const {
	if (!is_square()) {
		throw std::invalid_argument("Matrix dimensions do not match for adjoint");
	}
	std::math::matrix<_Tp> result(x_dimension_,y_dimension_);
	for (int i=0;i<x_dimension_;i++) {
		for (int j=0;j<y_dimension_;j++) {
			result.m[j][i]=expansion(i,j).value()*((i+j)%2?-1:1);
		}
	}
	return result;
}

template <typename _Tp>
std::math::matrix<_Tp> std::math::matrix<_Tp>::inverse() const {
	if (!is_square()) {
		throw std::invalid_argument("Matrix dimensions do not match for inversion");
	}
	int dimension=x_dimension_;
	std::math::matrix<_Tp> temp=expand_to(2*dimension,2*dimension);
	std::math::matrix<_Tp> result(dimension,dimension);
	for (int i=0;i<dimension;i++) {
		for (int j=0;j<dimension;j++) {
			if (i==j) temp.m[i][j+dimension]=std::math::base_unit_trait<_Tp>::value();
			else temp.m[i][j+dimension]=std::math::base_unit_trait<_Tp>::zero();
		}
	}
    	for (int i=0;i<dimension;i++) {
		if (temp.m[i][i]==std::math::base_unit_trait<_Tp>::zero()) {
			int j;
			for (j=i+1;j<dimension && temp.m[j][i]==std::math::base_unit_trait<_Tp>::zero();j++);
			if (j==dimension) {
				throw std::domain_error("Matrix is singular and cannot be inverted.");
			}
			_Tp* temp_row=temp.m[i];
			temp.m[i]=temp.m[j];
			temp.m[j]=temp_row;
		}
		_Tp pivot=temp.m[i][i];
		for (int j=0;j<2*dimension;j++) temp.m[i][j]/=pivot;
		for (int j=0;j<dimension;j++) {
			if (i!=j) {
				_Tp ratio=temp.m[j][i];
				for (int k=0;k<2*dimension;k++) temp.m[j][k]-=ratio*temp.m[i][k];
			}
		}
	}
	for (int i=0;i<dimension;i++) {
		for (int j=0;j<dimension;j++) {
			result.m[i][j]=temp.m[i][j+dimension];
		}
	}
	return result;	
}

template <typename _Tp>
std::math::matrix<_Tp> std::math::matrix<_Tp>::transposition() const {
	std::math::matrix<_Tp> result(y_dimension_,x_dimension_);
	for (int i=0;i<x_dimension_;i++) {
		for (int j=0;j<y_dimension_;j++) {
			result.m[j][i]=m[i][j];
		}
	}
	return result;
}

template <typename _Tp>
std::math::matrix<_Tp> std::math::matrix<_Tp>::power(int times) const {
	if (!is_square()) {
		throw std::invalid_argument("Matrix must be square to power");
	}
	if (times<0) {
		throw std::invalid_argument("Times must be positive");
	}
	std::math::matrix<_Tp> result=*this;
	int dimension=x_dimension_;
	if (times==0) {
		for (int i=0;i<dimension;i++) {
			for (int j=0;j<dimension;j++) {
				if (i==j) result.m[i][j]=std::math::base_unit_trait<_Tp>::value();
				else result.m[i][j]=std::math::base_unit_trait<_Tp>::zero();
			}
		}
		return result;
	}
	for (int i=0;i<times-1;i++) {
		result=result*(*this);
	}
	return result;
}

template <typename _Tp>
std::math::matrix<_Tp> std::math::matrix<_Tp>::expansion(int x,int y) const {
	if (x_dimension_<2 || y_dimension_<2) {
		throw std::invalid_argument("Matrix dimensions do not match for expansion");
	}
	matrix<_Tp> result(x_dimension_-1,y_dimension_-1);
	for (int i=0;i<x_dimension_;i++) {
		for (int j=0;j<y_dimension_;j++) {
        	if (i!=x && j!=y) {
            	result.m[i-(i>x)][j-(j>y)] = m[i][j];
            }
        }
    }
    return result;
}

template <typename _Tp>
std::math::matrix<_Tp> std::math::matrix<_Tp>::expand_to(int to_x,int to_y,int x_dimension,int y_dimension) const {
	if (x_dimension<x_dimension_ || y_dimension<y_dimension_ || (x_dimension==x_dimension_ && y_dimension==y_dimension_)) {
		throw std::invalid_argument("Traget dimensions are too small");
	}
	if (to_x+x_dimension_>x_dimension || to_y+y_dimension_>y_dimension) {
		throw std::out_of_range("Target position is out of range");
	}
	std::math::matrix<_Tp> result(x_dimension,y_dimension);
	for (int i=0;i<x_dimension_;i++) {
		for (int j=0;j<y_dimension_;j++) {
			result.m[i+to_x][j+to_y]=m[i][j];
		}
	}
	return result;
}

template <typename _Tp>
std::math::matrix<_Tp> std::math::matrix<_Tp>::expand_to(int x_dimension,int y_dimension) const {
	return expand_to(0,0,x_dimension,y_dimension);
}

template <typename _Tp>
std::math::matrix<_Tp> std::math::matrix<_Tp>::crop_to(int from_x,int from_y,int x_dimension,int y_dimension) const {
	if (x_dimension>x_dimension_ || y_dimension>y_dimension_ || (x_dimension==x_dimension_ && y_dimension==y_dimension_)) {
		throw std::invalid_argument("Traget dimensions are too large");
	}
	if (from_x+x_dimension>x_dimension_ || from_y+y_dimension>y_dimension_) {
		throw std::out_of_range("Target position is out of range");
	}
	std::math::matrix<_Tp> result(x_dimension,y_dimension);
	for (int i=0;i<x_dimension;i++) {
		for (int j=0;j<y_dimension;j++) {
			result.m[i][j]=m[i+from_x][j+from_y];
		}
	}
	return result;
}

template <typename _Tp>
std::math::matrix<_Tp> std::math::matrix<_Tp>::crop_to(int x_dimension,int y_dimension) const {
	return crop_to(0,0,x_dimension,y_dimension);
}

template <typename _Tp>
bool std::math::matrix<_Tp>::is_square() const {
	return x_dimension_==y_dimension_;
}

template <typename _Tp>
_Tp std::math::matrix<_Tp>::value() const {
	if (!is_square()) {
		throw std::invalid_argument("Matrix must be square to calculate value");
	}
	int dimension=x_dimension_;
	_Tp det=std::math::base_unit_trait<_Tp>::value();
	for (int i=0;i<dimension;i++) {
		int pivot=i;
		for (int j=i+1;j<dimension;j++) {
			if (abs(m[j][i])>abs(m[pivot][i])) {
				pivot=j;
			}
		}
		if (m[pivot][i]==0) {
			return std::math::base_unit_trait<_Tp>::zero();
		}
		if (i!=pivot) {
			_Tp* temp_row=m[i];
			m[i]=m[pivot];
			m[pivot]=temp_row;
			det*=std::math::base_unit_trait<_Tp>::neg_unit();
		}
		det*=m[i][i];
		for (int j=i+1;j<dimension;j++) {
			m[i][j]/=m[i][i];
		}
		for (int j=0;j<dimension;j++) {
			if (j!=i) {
				for (int k=i+1;k<dimension;k++) {
					m[j][k]-=m[j][i]*m[i][k];
				}
			}
		}
	}
	return det;
}

template <typename _Tp>
std::string std::math::matrix<_Tp>::print() const {
	int widths[x_dimension_];
	std::string result="┌";
	for (int j=0;j<y_dimension_;j++) {
		int curr_width=0;
		for (int i=0;i<x_dimension_;i++) {
			curr_width=std::max(curr_width,(int)base_unit_trait<_Tp>::to_string(m[i][j]).size());
		}
		curr_width+=2;
		widths[j]=curr_width-2;
		for (int i=0;i<curr_width;i++) {
			result+="─";
		}
	}
	result+="┐\n";
	for (int i=0;i<x_dimension_;i++) {
		result+="│";
		for (int j=0;j<y_dimension_;j++) {
			result+=" ";
			int total_space=widths[j]-base_unit_trait<_Tp>::to_string(m[i][j]).size();
			result+=std::string(total_space/2+total_space%2,' ')+base_unit_trait<_Tp>::to_string(m[i][j])+std::string(total_space/2,' ')+" ";
		}
		result+="│\n";
	}
	result+="└";
	for (int j=0;j<y_dimension_;j++) {
		for (int i=0;i<widths[j]+2;i++) {
			result+="─";
		}
	}
	result+="┘";
	return result;
}

template <typename _Tp>
std::string std::math::matrix<_Tp>::print_with_squares() const {
	int widths[x_dimension_];
	std::string result="┌";
	for (int j=0;j<y_dimension_;j++) {
		int curr_width=0;
		for (int i=0;i<x_dimension_;i++) {
			curr_width=std::max(curr_width,(int)base_unit_trait<_Tp>::to_string(m[i][j]).size());
		}
		widths[j]=curr_width;
		for (int i=0;i<curr_width;i++) {
			result+="─";
		}
		if (j+1!=y_dimension_) result+="┬";
	}
	result+="┐\n";
	for (int i=0;i<x_dimension_;i++) {
		result+="│";
		for (int j=0;j<y_dimension_;j++) {
			int total_space=widths[j]-base_unit_trait<_Tp>::to_string(m[i][j]).size();
			result+=std::string(total_space/2+total_space%2,' ')+base_unit_trait<_Tp>::to_string(m[i][j])+std::string(total_space/2,' ');
			result+="│";
		}
		result+="\n";
		if (i+1!=x_dimension_) {
			result+="├";
			for (int j=0;j<y_dimension_;j++) {

				for (int i=0;i<widths[j];i++) {
					result+="─";
				}
				if (j+1!=y_dimension_) result+="┼";
			}
			result+="┤\n";
		}
	}
	result+="└";
	for (int j=0;j<y_dimension_;j++) {
		for (int i=0;i<widths[j];i++) {
			result+="─";
		}
		if (j+1!=y_dimension_) result+="┴";
	}
	result+="┘";
	return result;
}

#include <cmath>
std::math::matrix<double> transform_matrix3(double x,double y,double rotate,double scale_x,double scale_y) {
	std::math::matrix<double> result(3,3);
	result.m[0][0]=cos(rotate)*scale_x;
	result.m[1][0]=-1.0*sin(rotate)*scale_x;
	result.m[0][1]=sin(rotate)*scale_y;
	result.m[1][1]=cos(rotate)*scale_y;
	result.m[0][2]=x;
	result.m[1][2]=y;
	result.m[2][2]=1.0;
	return result;
}

