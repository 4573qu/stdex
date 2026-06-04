//Last Modified At 2026/01/03
//@Version 1.2.0.0
//@H_Version 1.3.0.0

#include "matrix.h"

#include <cmath>
#include <stdexcept>

template <typename _Tp>
stdex::math::matrix<_Tp>::matrix(std::size_t x_dimension,std::size_t y_dimension) : x_dimension_(x_dimension) , y_dimension_(y_dimension) {
	if (x_dimension<1 || y_dimension<1) throw std::invalid_argument("Matrix dimensions do not match for create");
	m_=new _Tp*[x_dimension];
	for (int i=0;i<x_dimension;i++) {
		m_[i]=new _Tp[y_dimension];
		for (int j=0;j<y_dimension;j++) m_[i][j]=std::math::base_unit_trait<_Tp>::zero();
	}
}

template <typename _Tp>
stdex::math::matrix<_Tp>::matrix(std::size_t x_dimension,std::size_t y_dimension,std::initializer_list<_Tp> init_list) : x_dimension_(x_dimension) , y_dimension_(y_dimension) {
	if (init_list.size()!=x_dimension*y_dimension) throw std::invalid_argument("Initializer list size does not match matrix dimensions");
	m_=new _Tp*[x_dimension];
	typename std::initializer_list<_Tp>::iterator it=init_list.begin();
	for (int i=0;i<x_dimension;i++) {
		m_[i]=new _Tp[y_dimension];
		for (int j=0;j<y_dimension;j++) m_[i][j]=*it++;
	}
}

template <typename _Tp>
stdex::math::matrix<_Tp>::~matrix() {
	if (m_) {
		for (int i=0;i<x_dimension_;i++) delete[] m_[i];
	}
	delete[] m_;
}

template <typename _Tp>
stdex::math::matrix<_Tp>::matrix(const stdex::math::matrix<_Tp>& other) : x_dimension_(other.x_dimension_) , y_dimension_(other.y_dimension_) {
	m_=new _Tp*[x_dimension_];
	for(int i=0;i<x_dimension_;i++) {
		m_[i]=new _Tp[y_dimension_];
		std::copy(other.m_[i],other.m_[i]+y_dimension_,m_[i]);
	}
}

template <typename _Tp>
stdex::math::matrix<_Tp>::matrix(stdex::math::matrix<_Tp>&& other) noexcept : x_dimension_(other.x_dimension_) , y_dimension_(other.y_dimension_) , m_(other.m_) {
	other.m_=nullptr;
	other.x_dimension_=0;
	other.y_dimension_=0;
}

template <typename _Tp>
stdex::math::matrix<_Tp>& stdex::math::matrix<_Tp>::operator =(const stdex::math::matrix<_Tp>& other) {
	if (this!=&other) {
		if (m_) {
			for (int i=0;i<x_dimension_;i++) delete[] m_[i];
		}
		delete[] m_;
		x_dimension_=other.x_dimension_;
		y_dimension_=other.y_dimension_;
		m_=new _Tp*[x_dimension_];
		for (int i=0;i<x_dimension_;i++) {
			m_[i]=new _Tp[y_dimension_];
			for (int j=0;j<y_dimension_;j++) m_[i][j]=other.m_[i][j];
		}
	}
	return *this;
}

template <typename _Tp>
stdex::math::matrix<_Tp>& stdex::math::matrix<_Tp>::operator =(stdex::math::matrix<_Tp>&& other) noexcept {
	if (this!=&other) {
		if (m_) {
			for (int i=0;i<x_dimension_;i++) delete[] m_[i];
		}
		delete[] m_;
		x_dimension_=other.x_dimension_;
		y_dimension_=other.y_dimension_;
		m_=other.m_;
		other.m_=nullptr;
		other.x_dimension_=0;
		other.y_dimension_=0;
	}
	return *this;
}

template <typename _Tp>
bool stdex::math::matrix<_Tp>::operator ==(const stdex::math::matrix<_Tp>& other) const {
	if (x_dimension_ != other.x_dimension_ || y_dimension_ != other.y_dimension_) return false;
	for (int i=0;i<x_dimension_;i++) {
		for (int j=0;j<y_dimension_;j++) {
			if (m_[i][j]!=other.m_[i][j]) return false;
		}
	}
	return true;
}

template <typename _Tp>
bool stdex::math::matrix<_Tp>::operator !=(const stdex::math::matrix<_Tp>& other) const {
	return !((*this)==other);
}

template <typename _Tp>
stdex::math::matrix<_Tp> stdex::math::matrix<_Tp>::operator +(const stdex::math::matrix<_Tp>& other) const {
	if (x_dimension_ != other.x_dimension_ || y_dimension_ != other.y_dimension_) throw std::invalid_argument("Matrices dimensions do not match for addition");
	stdex::math::matrix<_Tp> result(x_dimension_,y_dimension_);
	for (int i=0;i<x_dimension_;i++) {
		for (int j=0;j<y_dimension_;j++) result.m_[i][j]=m_[i][j]+other.m_[i][j];
	}
	return result;
}

template <typename _Tp>
stdex::math::matrix<_Tp>& stdex::math::matrix<_Tp>::operator +=(const stdex::math::matrix<_Tp>& other) {
	if (x_dimension_ != other.x_dimension_ || y_dimension_ != other.y_dimension_) throw std::invalid_argument("Matrices dimensions do not match for addition");
	for (int i=0;i<x_dimension_;i++) {
		for (int j=0;j<y_dimension_;j++) m_[i][j]+=other.m_[i][j];
	}
	return *this;
}

template <typename _Tp>
stdex::math::matrix<_Tp> stdex::math::matrix<_Tp>::operator -(const stdex::math::matrix<_Tp>& other) const {
	if (x_dimension_ != other.x_dimension_ || y_dimension_ != other.y_dimension_) throw std::invalid_argument("Matrices dimensions do not match for substraction");
	stdex::math::matrix<_Tp> result(x_dimension_,y_dimension_);
	for (int i=0;i<x_dimension_;i++) {
		for (int j=0;j<y_dimension_;j++) result.m_[i][j]=m_[i][j]-other.m_[i][j];
	}
	return result;
}

template <typename _Tp>
stdex::math::matrix<_Tp>& stdex::math::matrix<_Tp>::operator -=(const stdex::math::matrix<_Tp>& other) {
	if (x_dimension_ != other.x_dimension_ || y_dimension_ != other.y_dimension_) throw std::invalid_argument("Matrices dimensions do not match for substraction");
	for (int i=0;i<x_dimension_;i++) {
		for (int j=0;j<y_dimension_;j++) m_[i][j]-=other.m_[i][j];
	}
	return *this;
}

template <typename _Tp>
stdex::math::matrix<_Tp> stdex::math::matrix<_Tp>::operator *(const stdex::math::matrix<_Tp>& other) const {
	if (y_dimension_ != other.x_dimension_) throw std::invalid_argument("Matrices dimensions do not match for multiply");
	stdex::math::matrix<_Tp> result(x_dimension_,other.y_dimension_);
	for (int i=0;i<x_dimension_;i++) {
		for (int j=0;j<other.y_dimension_;j++) {
			_Tp temp_value=stdex::math::base_unit_trait<_Tp>::zero();
			for (int k=0;k<y_dimension_;k++) temp_value=temp_value+m_[i][k]*other.m_[k][j];
			result.m_[i][j]=temp_value;
		}
	}
	return result;
}

template <typename _Tp>
stdex::math::matrix<_Tp> stdex::math::matrix<_Tp>::operator /(const stdex::math::matrix<_Tp>& other) const {
	if (!other.is_square() || y_dimension_!=other.y_dimension_) throw std::invalid_argument("Matrices dimensions do not match for division");
	stdex::math::matrix<_Tp> result=other.inverse();
	return (*this)*result;
}

template <typename _Tp>
_Tp& stdex::math::matrix<_Tp>::operator ()(std::size_t i,std::size_t j) {
	if (i>=x_dimension_ || j>=y_dimension_) throw std::out_of_range("Index out of range");
	return m_[i][j];
}

template <typename _Tp>
const _Tp& stdex::math::matrix<_Tp>::operator ()(std::size_t i,std::size_t j) const {
	if (i>=x_dimension_ || j>=y_dimension_) throw std::out_of_range("Index out of range");
	return m_[i][j];
}

template <typename _Tp>
stdex::math::matrix<_Tp> stdex::math::matrix<_Tp>::adjoint() const {
	if (!is_square()) throw std::invalid_argument("Matrix dimensions do not match for adjoint");
	stdex::math::matrix<_Tp> result(x_dimension_,y_dimension_);
	for (int i=0;i<x_dimension_;i++) {
		for (int j=0;j<y_dimension_;j++) result.m_[j][i]=expansion(i,j).value()*((i+j)%2?-1:1);
	}
	return result;
}

template <typename _Tp>
stdex::math::matrix<_Tp> stdex::math::matrix<_Tp>::inverse() const {
	if (!is_square()) throw std::invalid_argument("Matrix dimensions do not match for inversion");
	int dimension=x_dimension_;
	stdex::math::matrix<_Tp> temp=expand_to(2*dimension,2*dimension);
	stdex::math::matrix<_Tp> result(dimension,dimension);
	for (int i=0;i<dimension;i++) {
		for (int j=0;j<dimension;j++) {
			if (i==j) temp.m_[i][j+dimension]=stdex::math::base_unit_trait<_Tp>::value();
			else temp.m_[i][j+dimension]=stdex::math::base_unit_trait<_Tp>::zero();
		}
	}
    	for (int i=0;i<dimension;i++) {
		if (temp.m_[i][i]==stdex::math::base_unit_trait<_Tp>::zero()) {
			int j;
			for (j=i+1;j<dimension && temp.m_[j][i]==stdex::math::base_unit_trait<_Tp>::zero();j++);
			if (j==dimension) throw std::domain_error("Matrix is singular and cannot be inverted.");
			_Tp* temp_row=temp.m_[i];
			temp.m_[i]=temp.m_[j];
			temp.m_[j]=temp_row;
		}
		_Tp pivot=temp.m_[i][i];
		for (int j=0;j<2*dimension;j++) temp.m_[i][j]/=pivot;
		for (int j=0;j<dimension;j++) {
			if (i!=j) {
				_Tp ratio=temp.m_[j][i];
				for (int k=0;k<2*dimension;k++) temp.m_[j][k]-=ratio*temp.m_[i][k];
			}
		}
	}
	for (int i=0;i<dimension;i++) {
		for (int j=0;j<dimension;j++) result.m_[i][j]=temp.m_[i][j+dimension];
	}
	return result;	
}

template <typename _Tp>
stdex::math::matrix<_Tp> stdex::math::matrix<_Tp>::transposition() const {
	stdex::math::matrix<_Tp> result(y_dimension_,x_dimension_);
	for (int i=0;i<x_dimension_;i++) {
		for (int j=0;j<y_dimension_;j++) result.m_[j][i]=m_[i][j];
	}
	return result;
}

template <typename _Tp>
stdex::math::matrix<_Tp> stdex::math::matrix<_Tp>::power(int times) const {
	if (!is_square()) throw std::invalid_argument("Matrix must be square to power");
	stdex::math::matrix<_Tp> result=*this;
	if (times<0) return inverse().power(-times);
	int dimension=x_dimension_;
	if (times==0) {
		for (int i=0;i<dimension;i++) {
			for (int j=0;j<dimension;j++) {
				if (i==j) result.m_[i][j]=stdex::math::base_unit_trait<_Tp>::value();
				else result.m_[i][j]=stdex::math::base_unit_trait<_Tp>::zero();
			}
		}
		return result;
	}
	for (int i=0;i<times-1;i++) result=result*(*this);
	return result;
}

template <typename _Tp>
stdex::math::matrix<_Tp> stdex::math::matrix<_Tp>::expansion(std::size_t x,std::size_t y) const {
	if (x_dimension_<2 || y_dimension_<2) throw std::invalid_argument("Matrix dimensions do not match for expansion");
	stdex::math::matrix<_Tp> result(x_dimension_-1,y_dimension_-1);
	for (int i=0;i<x_dimension_;i++) {
		for (int j=0;j<y_dimension_;j++) {
			if (i!=x && j!=y) result.m_[i-(i>x)][j-(j>y)]=m_[i][j];
		}
	}
	return result;
}

template <typename _Tp>
stdex::math::matrix<_Tp> stdex::math::matrix<_Tp>::expand_to(std::size_t to_x,std::size_t to_y,std::size_t x_dimension,std::size_t y_dimension) const {
	if (x_dimension<x_dimension_ || y_dimension<y_dimension_ || (x_dimension==x_dimension_ && y_dimension==y_dimension_)) throw std::invalid_argument("Target dimensions are too small");
	if (to_x+x_dimension_>x_dimension || to_y+y_dimension_>y_dimension) throw std::out_of_range("Target position is out of range");
	stdex::math::matrix<_Tp> result(x_dimension,y_dimension);
	for (int i=0;i<x_dimension_;i++) {
		for (int j=0;j<y_dimension_;j++) result.m_[i+to_x][j+to_y]=m_[i][j];
	}
	return result;
}

template <typename _Tp>
stdex::math::matrix<_Tp> stdex::math::matrix<_Tp>::expand_to(std::size_t x_dimension,std::size_t y_dimension) const {
	return expand_to(0,0,x_dimension,y_dimension);
}

template <typename _Tp>
stdex::math::matrix<_Tp> stdex::math::matrix<_Tp>::crop_to(std::size_t from_x,std::size_t from_y,std::size_t x_dimension,std::size_t y_dimension) const {
	if (x_dimension>x_dimension_ || y_dimension>y_dimension_ || (x_dimension==x_dimension_ && y_dimension==y_dimension_)) throw std::invalid_argument("Target dimensions are too large");
	if (from_x+x_dimension>x_dimension_ || from_y+y_dimension>y_dimension_) throw std::out_of_range("Target position is out of range");
	stdex::math::matrix<_Tp> result(x_dimension,y_dimension);
	for (int i=0;i<x_dimension;i++) {
		for (int j=0;j<y_dimension;j++) result.m_[i][j]=m_[i+from_x][j+from_y];
	}
	return result;
}

template <typename _Tp>
stdex::math::matrix<_Tp> stdex::math::matrix<_Tp>::crop_to(std::size_t x_dimension,std::size_t y_dimension) const {
	return crop_to(0,0,x_dimension,y_dimension);
}

template <typename _Tp>
bool stdex::math::matrix<_Tp>::is_square() const {
	return x_dimension_==y_dimension_;
}

template <typename _Tp>
_Tp stdex::math::matrix<_Tp>::value() const {
	if (!is_square()) throw std::invalid_argument("Matrix must be square to calculate value");
	int dimension=x_dimension_;
	_Tp det=stdex::math::base_unit_trait<_Tp>::value();
	for (int i=0;i<dimension;i++) {
		int pivot=i;
		for (int j=i+1;j<dimension;j++) {
			if (abs(m[j][i])>abs(m_[pivot][i])) pivot=j;
		}
		if (m_[pivot][i]==0) return stdex::math::base_unit_trait<_Tp>::zero();
		if (i!=pivot) {
			_Tp* temp_row=m_[i];
			m_[i]=m_[pivot];
			m_[pivot]=temp_row;
			det*=stdex::math::base_unit_trait<_Tp>::neg_unit();
		}
		det*=m_[i][i];
		for (int j=i+1;j<dimension;j++) m_[i][j]/=m_[i][i];
		for (int j=0;j<dimension;j++) {
			if (j!=i) {
				for (int k=i+1;k<dimension;k++) m_[j][k]-=m_[j][i]*m_[i][k];
			}
		}
	}
	return det;
}

template <typename _Tp>
_Tp stdex::math::matrix<_Tp>::trace() const {
	if (!is_square()) return stdex::math::base_unit_trait<_Tp>::zero();
	_Tp sum=stdex::math::base_unit_trait<_Tp>::zero();;
	for (std::size_t i=0;i<x_dimension_;i++) sum+=(*this)(i,i);
	return sum;
}

template <typename _Tp>
bool stdex::math::matrix<_Tp>::is_valid() const {
	return m_ && x_dimension_>0 && y_dimension_>0;
}

template <typename _Tp>
stdex::math::matrix<_Tp> stdex::math::matrix<_Tp>::identity(std::size_t n) {
	stdex::math::matrix<_Tp> m(n,n);
	for (std::size_t i=0;i<n;i++) {
		for (std::size_t j=0;j<n;j++) m(i,j)=(i==j)?stdex::math::base_unit_trait<_Tp>::value();:stdex::math::base_unit_trait<_Tp>::zero();
	}
	return m;
}

template <typename _Tp>
void stdex::math::matrix<_Tp>::fill(const _Tp& value) {
	for (std::size_t i=0;i<x_dimension_;i++) {
		for (std::size_t j=0;j<y_dimension_;j++) (*this)(i,j)=value;
	}
}

template <typename _Tp>
_Tp stdex::math::matrix<_Tp>::determinant2() const {
	if (x_dimension_!=2 || y_dimension_!=2) return stdex::math::base_unit_trait<_Tp>::zero();;
	return (*this)(0,0)*(*this)(1,1)-(*this)(0,1)*(*this)(1,0);
}

template <typename _Tp>
_Tp stdex::math::matrix<_Tp>::determinant3() const {
	if (x_dimension_!=3 || y_dimension_!=3) return stdex::math::base_unit_trait<_Tp>::zero();;
	const _Tp& a=(*this)(0,0); const _Tp& b=(*this)(0,1); const _Tp& c=(*this)(0,2);
	const _Tp& d=(*this)(1,0); const _Tp& e=(*this)(1,1); const _Tp& f=(*this)(1,2);
	const _Tp& g=(*this)(2,0); const _Tp& h=(*this)(2,1); const _Tp& i=(*this)(2,2);
	return a*(e*i-f*h)-b*(d*i-f*g)+c*(d*h-e*g);
}

template <typename _Tp>
std::string stdex::math::matrix<_Tp>::print() const {
	int widths[x_dimension_];
	std::string result="┌";
	for (int j=0;j<y_dimension_;j++) {
		int curr_width=0;
		for (int i=0;i<x_dimension_;i++) curr_width=std::max(curr_width,(int)stdex::math::base_unit_trait<_Tp>::to_string(m_[i][j]).size());
		curr_width+=2;
		widths[j]=curr_width-2;
		for (int i=0;i<curr_width;i++) result+="─";
	}
	result+="┐\n";
	for (int i=0;i<x_dimension_;i++) {
		result+="│";
		for (int j=0;j<y_dimension_;j++) {
			result+=" ";
			int total_space=widths[j]-stdex::math::base_unit_trait<_Tp>::to_string(m_[i][j]).size();
			result+=std::string(total_space/2+total_space%2,' ')+stdex::math::base_unit_trait<_Tp>::to_string(m_[i][j])+std::string(total_space/2,' ')+" ";
		}
		result+="│\n";
	}
	result+="└";
	for (int j=0;j<y_dimension_;j++) {
		for (int i=0;i<widths[j]+2;i++) result+="─";
	}
	result+="┘";
	return result;
}

template <typename _Tp>
std::string stdex::math::matrix<_Tp>::print_with_squares() const {
	int widths[x_dimension_];
	std::string result="┌";
	for (int j=0;j<y_dimension_;j++) {
		int curr_width=0;
		for (int i=0;i<x_dimension_;i++) curr_width=std::max(curr_width,(int)stdex::math::base_unit_trait<_Tp>::to_string(m_[i][j]).size());
		widths[j]=curr_width;
		for (int i=0;i<curr_width;i++) result+="─";
		if (j+1!=y_dimension_) result+="┬";
	}
	result+="┐\n";
	for (int i=0;i<x_dimension_;i++) {
		result+="│";
		for (int j=0;j<y_dimension_;j++) {
			int total_space=widths[j]-stdex::math::base_unit_trait<_Tp>::to_string(m_[i][j]).size();
			result+=std::string(total_space/2+total_space%2,' ')+stdex::math::base_unit_trait<_Tp>::to_string(m_[i][j])+std::string(total_space/2,' ');
			result+="│";
		}
		result+="\n";
		if (i+1!=x_dimension_) {
			result+="├";
			for (int j=0;j<y_dimension_;j++) {
				for (int i=0;i<widths[j];i++) result+="─";
				if (j+1!=y_dimension_) result+="┼";
			}
			result+="┤\n";
		}
	}
	result+="└";
	for (int j=0;j<y_dimension_;j++) {
		for (int i=0;i<widths[j];i++) result+="─";
		if (j+1!=y_dimension_) result+="┴";
	}
	result+="┘";
	return result;
}

stdex::math::matrix<double> transform_matrix3(double x,double y,double rotate,double scale_x,double scale_y) {
	stdex::math::matrix<double> result(3,3);
	result.m_[0][0]=cos(rotate)*scale_x;
	result.m_[1][0]=-1.0*sin(rotate)*scale_x;
	result.m_[0][1]=sin(rotate)*scale_y;
	result.m_[1][1]=cos(rotate)*scale_y;
	result.m_[0][2]=x;
	result.m_[1][2]=y;
	result.m_[2][2]=1.0;
	return result;
}

