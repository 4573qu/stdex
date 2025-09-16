//Last Modified At 2025/09/16
//@Version 1.0.0.0
#ifndef _STDEX_MATH_GEOMETRY_GRAPHICS_H_
#define _STDEX_MATH_GEOMETRY_GRAPHICS_H_ 1
#include "point.h"
#include "primitives.h"
#include "shape.h" //Duo Bian Xing


//Rect || Rect -> Duobianxing
//Rect Contains Point
//vector<Point> Xiangjiao
namespace std {
	
namespace math {

template <typename _Tp>	
bool contains(shape<_Tp>& s,point2<_Tp> p) {
	return s.contains(p.x_,p.y_);	
}	

}	
	
}

#endif