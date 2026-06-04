//Last Modified At 2026/06/04
//@Version 1.1.0.0
//@H_Version 1.1.0.0

#include <cmath>
#include <stdexcept>

#include "easing.h"

template <typename _Tp>
stdex::vision::easing<_Tp>::easing(easing_type type) : type(type) {
	switch (type) {
		case ET_EXPONENT: {
			parameter.exp.base=stdex::math::base_unit_trait<_Tp>::E();
			parameter.exp.exponent=(_Tp)10;			
			break;
		}	
		case ET_ELASTIC: {
			parameter.elastic.amplitude=(_Tp)0.7;
			parameter.elastic.period=(_Tp)0.3;
			parameter.elastic.damping=(_Tp)0.2;			
			break;
		}
		case ET_BACK: {
			parameter.back.scale=(_Tp)2.5;
			parameter.back.offset=(_Tp)1.70158;
			break;
		}
		case ET_CONSTANT: {
			parameter.constant.value=stdex::math::base_unit_trait<_Tp>::value();	
			break;
		}
		case ET_LOGARITHM: {
			parameter.logarithm.base=stdex::math::base_unit_trait<_Tp>::E();
			break;
		}
		case ET_CUSTOM: {
			parameter.custom.beizer=new stdex::math::curve<_Tp,2>();
			parameter.custom.beizer->start=stdex::math::point2<_Tp>(stdex::math::base_unit_trait<_Tp>::zero(),stdex::math::base_unit_trait<_Tp>::zero());
			parameter.custom.beizer->end=stdex::math::point2<_Tp>(stdex::math::base_unit_trait<_Tp>::value(),stdex::math::base_unit_trait<_Tp>::value());
			parameter.custom.precision=(_Tp)100;
			break;
		}
		default: {
			break;
		}
	}	
}

template <typename _Tp>
_Tp stdex::vision::easing<_Tp>::progress(_Tp time) {
	if (time<stdex::math::base_unit_trait<_Tp>::zero() || time>stdex::math::base_unit_trait<_Tp>::value()) throw std::invalid_argument("Time muse be between 0 and 1");
	if (type==ET_EXPONENT) {
		if (parameter.exp.base<=1) throw std::invalid_argument("Base must be bigger than or equal to 1");
		if (parameter.exp.exponent<=0) throw std::invalid_argument("Exponent cannot be negative");
	}
	if (type==ET_ELASTIC) {
		if (parameter.elastic.amplitude<stdex::math::base_unit_trait<_Tp>::zero() || parameter.elastic.amplitude>stdex::math::base_unit_trait<_Tp>::value()) throw std::out_of_range("Amplitude must be between 0 and 1");
		if (parameter.elastic.period<stdex::math::base_unit_trait<_Tp>::zero() || parameter.elastic.period>stdex::math::base_unit_trait<_Tp>::value()) throw std::out_of_range("Period must be between 0 and 1");
		if (parameter.elastic.damping<stdex::math::base_unit_trait<_Tp>::zero() || parameter.elastic.damping>stdex::math::base_unit_trait<_Tp>::value()) throw std::out_of_range("Damping must be between 0 and 1");
	}
	if (type==ET_BACK) {
		if (parameter.back.scale<(_Tp)2 || parameter.back.scale>(_Tp)4) throw std::out_of_range("Scale must be between 2 and 4");
		if (parameter.back.offset<(_Tp)1.70158 || parameter.back.offset>(_Tp)2.59491) throw std::out_of_range("Offset must be between 1.70158 and 2.59491");
	}
	if (type==ET_CUSTOM) {
		stdex::math::point2<_Tp> temp_point;
		temp_point=parameter.custom.beizer->start;
		if (temp_point.x<stdex::math::base_unit_trait<_Tp>::zero() || temp_point.y<stdex::math::base_unit_trait<_Tp>::zero() || temp_point.x>stdex::math::base_unit_trait<_Tp>::value() || temp_point.y>stdex::math::base_unit_trait<_Tp>::value()) throw std::out_of_range("Beizer point must be between 0 and 1");
		temp_point=parameter.custom.beizer->end;
		if (temp_point.x<stdex::math::base_unit_trait<_Tp>::zero() || temp_point.y<stdex::math::base_unit_trait<_Tp>::zero() || temp_point.x>stdex::math::base_unit_trait<_Tp>::value() || temp_point.y>stdex::math::base_unit_trait<_Tp>::value()) throw std::out_of_range("Beizer point must be between 0 and 1");
		if (parameter.custom.precision<=0) throw std::invalid_argument("Precision must be positive");
	}
	_Tp result;
	switch (type) {
		case ET_LINEAR: { //f(t)=t
			result=time;
			break;
		}
		case ET_QUAD: { //f(t)=t^2
			result=time*time;
			break;
		}
		case ET_INVERSE_QUAD: { //f(t)=2t-t^2
			result=(_Tp)2*time-time*time;
			break;
		}
		case ET_CUBIC: { //f(t)=t^3
			result=time*time*time;
			break;
		}
		case ET_INVERSE_CUBIC: { //f(t)=t^3-3t^2+3t;
			result=time*time*time-(_Tp)3*time*time+(_Tp)3*time;
			break;
		}
		case ET_QUART: { //f(t)=t^4
			result=time*time*time*time;
			break;
		}
		case ET_QUINT: { //f(t)=t^5
			result=time*time*time*time*time;
			break;
		}
		case ET_EXPONENT: { //f(t)=b^(e*(t-1))
			result=time-stdex::math::base_unit_trait<_Tp>::value();
			result=parameter.exp.exponent*result;
			result=stdex::math::base_unit_trait<_Tp>::pow(parameter.exp.base,result);
			break;
		}
		case ET_SINE: {
			result=(_Tp)stdex::math::base_unit_trait<_Tp>::PI()/(_Tp)2*time;
			result=stdex::math::base_unit_trait<_Tp>::cos(result);
			result=result*stdex::math::base_unit_trait<_Tp>::neg_unit();
			result=result+stdex::math::base_unit_trait<_Tp>::value();
			break;
		}
		case ET_CIRCLE: { //f(t)=1-(1-t^2)^(1/2)
			result=time*time;
			result=stdex::math::base_unit_trait<_Tp>::value()-result;
			result=stdex::math::base_unit_trait<_Tp>::sqrt(result);
			result=stdex::math::base_unit_trait<_Tp>::value()-result;
			break;
		}
		case ET_INVERSE_CIRCLE: { //f(t)=(2t-t^2)*(1/2)
			result=stdex::math::base_unit_trait<_Tp>::sqrt((_Tp)2*time-time*time);
			break;
		}
		case ET_ELASTIC: { //f(t)=A*2^(-10t)*sin((t-D)*2*pi/P)+1;Recommand:g(t)=f(t)+1-f(0)
			result=(_Tp)-10*time;
			result=stdex::math::base_unit_trait<_Tp>::pow((_Tp)2,result);
			result=parameter.elastic.amplitude*result;
			_Tp temp=time-parameter.elastic.damping;
			temp=temp*(_Tp)2*stdex::math::base_unit_trait<_Tp>::PI()/parameter.elastic.period;
			result=result*stdex::math::base_unit_trait<_Tp>::sin(temp);
			result=result+stdex::math::base_unit_trait<_Tp>::value();
			break;
		}
		case ET_SWING: { //f(t)=0.5(1-cos(pi*t))
			result=time*stdex::math::base_unit_trait<_Tp>::PI();
			result=stdex::math::base_unit_trait<_Tp>::cos(result);
			result=result*(_Tp)0.5;
			result=(_Tp)0.5-result;
			break;
		}
		case ET_BACK: { //f(t)=s*t^3-o*t^2;s[2,4] o[1.70158,259491];Recommend:g(t)=f(t)+1+o-s
			result=parameter.back.scale*time*time*time;
			result=result-parameter.back.offset*time*time;
			break;
		}
		case ET_SMOOTH_BOUNCE: { //f(t)=1-abs(cos(10pi*t)*e^(-6*t))
			result=(_Tp)10*time*stdex::math::base_unit_trait<_Tp>::PI();
			result=stdex::math::base_unit_trait<_Tp>::cos(result);	
			_Tp temp=(_Tp)-6*time;
			temp=stdex::math::base_unit_trait<_Tp>::pow(stdex::math::base_unit_trait<_Tp>::E(),temp);
			result=result*temp;
			result=stdex::math::base_unit_trait<_Tp>::abs(result);
			result=stdex::math::base_unit_trait<_Tp>::value()-result;
			break;
		}
		case ET_LINEAR_BOUNCE: { //f(t)=1-abs(2t-1)
			result=(_Tp)2*time-stdex::math::base_unit_trait<_Tp>::value();
			result=stdex::math::base_unit_trait<_Tp>::value()-stdex::math::base_unit_trait<_Tp>::abs(result);
			break;
		}
		case ET_CONSTANT: { //f(t)=C
			result=parameter.constant.value;
			break;
		}
		case ET_LOGARITHM: { //f(t)=log(b,1+(b-1)*t)
			result=parameter.logarithm.base-stdex::math::base_unit_trait<_Tp>::value();
			result=result*time;
			result=stdex::math::base_unit_trait<_Tp>::value()+result;
			result=stdex::math::base_unit_trait<_Tp>::log(parameter.logarithm.base,result);
			break;
		}
		case ET_SQUARE: { //f(t)=t^(1/2)
			result=stdex::math::base_unit_trait<_Tp>::sqrt(time);
			break;
		}
		case ET_CURVES: { //f(t)=3t^2-2t^3
			result=(_Tp)3*time*time;
			result=result-(_Tp)2*time*time*time;
			break;
		}
		case ET_QUADS: { //f(t)=2t^2;[0,0.5);f(t)=4t-1-2t^2;[0.5,1]
			if (time<(_Tp)0.5) result=(_Tp)2*time*time;
			else result=(_Tp)4*time-stdex::math::base_unit_trait<_Tp>::value()-(_Tp)2*time*time;
			break;
		}
		case ET_INVERSE_QUADS: { //f(t)=2t-2t^2;[0,0.5);f(t)=1-2t+2t^2;[0.5,1]
			result=time-time*time;
			result=(_Tp)2*result;
			result=time<(_Tp)0.5?result:stdex::math::base_unit_trait<_Tp>::value()-result;
			break;
		}
		case ET_CUBICS: { //f(t)=4t^3;[0,0.5);f(t)=4t^3-12t^2+12t-3;[0.5,1]
			if (time<(_Tp)0.5) result=(_Tp)2*time*time;
			else {
				result=time-stdex::math::base_unit_trait<_Tp>::value();
				result=(_Tp)8*result*result*result+(_Tp)2;
				result=result/(_Tp)2;
			}
			break;
		}
		case ET_CUSTOM: {
			std::vector<stdex::math::point2<_Tp>> temp_points=parameter.custom.beizer->get_points(parameter.custom.precision);
			for (int i=0;i<temp_points.size()-1;i++) {
				stdex::math::point2<_Tp> temp_point1=temp_points[i];
				stdex::math::point2<_Tp> temp_point2=temp_points[i+1];
				if (temp_point1.x>=temp_point2.x) {
					throw std::out_of_range("Beizer point must be less than next point");
					break;
				}
			}
			result=stdex::math::base_unit_trait<_Tp>::neg_unit();
			int low=0,high=temp_points.size()-1;
			while (low<high) {
				int mid=(low+high)/2;
				if (temp_points[mid].x==time) {
					result=temp_points[mid].y;
					break;
				} else if (temp_points[mid].x<time) low=mid+1;
				else high=mid;
			}
			if (result!=stdex::math::base_unit_trait<_Tp>::neg_unit()) break;
			if (low==0) result=temp_points[0].y;
			else if (low==temp_points.size()) return temp_points[temp_points.size()-1].y;
			else if (stdex::math::base_unit_trait<_Tp>::abs(time-temp_points[low-1].x)<stdex::math::base_unit_trait<_Tp>::abs(time-temp_points[low].x)) result=temp_points[low-1].y;
			else result=temp_points[low].y;
			//int index=(int)(time*parameter.custom.precision+(_Tp)0.5);
			//result=temp_points[index];
			break;
		}
		default: {
			result=time;
			break;
		}
	}
	return result;
}

template <typename _Tp>
_Tp stdex::vision::easing<_Tp>::get(_Tp time) {
	return get(time,stdex::vision::EO_EASEIN,stdex::vision::EI_NORMAL);
}

template <typename _Tp>
_Tp stdex::vision::easing<_Tp>::get(_Tp time,stdex::vision::easing_option option,stdex::vision::easing_inflection inflection) {
	_Tp result=time;
	switch (option) {
		case stdex::vision::EO_EASEIN: {
			result=progress(result);
			break;
		}
		case stdex::vision::EO_EASEOUT: {
			result=stdex::math::base_unit_trait<_Tp>::value()-progress(stdex::math::base_unit_trait<_Tp>::value()-result);
			break;
		}
		case stdex::vision::EO_EASEINOUT: {
			if (result<=(_Tp)0.5) result=progress(result*(_Tp)2)/(_Tp)2;
			else result=(stdex::math::base_unit_trait<_Tp>::value()-progress(stdex::math::base_unit_trait<_Tp>::value()-(result*(_Tp)2-stdex::math::base_unit_trait<_Tp>::value()))+stdex::math::base_unit_trait<_Tp>::value())/(_Tp)2;
			break;
		}
		case stdex::vision::EO_EASEIN_STRONG: {
			result=progress(result);
			result=progress(result);
			break;
		}
		case stdex::vision::EO_EASEOUT_STRONG: {
			result=stdex::math::base_unit_trait<_Tp>::value()-progress(stdex::math::base_unit_trait<_Tp>::value()-result);
			result=stdex::math::base_unit_trait<_Tp>::value()-progress(stdex::math::base_unit_trait<_Tp>::value()-result);
			break;
		}
		case stdex::vision::EO_EASEINOUT_STRONG: {
			if (result<=(_Tp)0.5) {
				result=progress(result*(_Tp)2);
				result=progress(result)/(_Tp)2;
			} else {
				result=stdex::math::base_unit_trait<_Tp>::value()-progress(stdex::math::base_unit_trait<_Tp>::value()-(result*(_Tp)2-stdex::math::base_unit_trait<_Tp>::value()));
				result=(stdex::math::base_unit_trait<_Tp>::value()-progress(stdex::math::base_unit_trait<_Tp>::value()-result)+stdex::math::base_unit_trait<_Tp>::value())/(_Tp)2;
			}
			break;
		}
		default: {
			result=progress(result);
			break;
		}
	}
	switch (inflection) {
		case stdex::vision::EI_NORMAL: {
			result=result;
			break;
		}
		case stdex::vision::EI_FAST: {
			stdex::vision::easing<_Tp> temp_easing(stdex::vision::ET_QUAD);
			result=temp_easing.get(result,option,stdex::vision::EI_NORMAL);
			break;
		}
		case stdex::vision::EI_SLOW: {
			stdex::vision::easing<_Tp> temp_easing(stdex::vision::ET_INVERSE_QUAD);
			result=temp_easing.get(result,option,stdex::vision::EI_NORMAL);
			break;
		}
		default: {
			result=result;
			break;
		}
	}
	return result;
}