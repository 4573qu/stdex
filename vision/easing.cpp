//Last Modified At 2024/09/09
//@Version 1.2
//@H_Version 1.2
#include "easing.h"
template <typename _Tp>
std::vision::easing<_Tp>::easing(EASING_TYPE type) : type_(type) {
	switch (type_) {
		case ET_EXPONENT: {
			parameter_.exp_.base_=std::math::base_unit_trait<_Tp>::E();
			parameter_.exp_.exponent_=(_Tp)10;			
			break;
		}	
		case ET_ELASTIC: {
			parameter_.elastic_.amplitude_=(_Tp)0.7;
			parameter_.elastic_.period_=(_Tp)0.3;
			parameter_.elastic_.damping_=(_Tp)0.2;			
			break;
		}
		case ET_BACK: {
			parameter_.back_.scale_=(_Tp)2.5;
			parameter_.back_.offset_=(_Tp)1.70158;
			break;
		}
		case ET_CONSTANT: {
			parameter_.constant_.val_=std::math::base_unit_trait<_Tp>::value();	
			break;
		}
		case ET_LOGARITHM: {
			parameter_.logarithm_.base_=std::math::base_unit_trait<_Tp>::E();
			break;
		}
		case ET_CUSTOM: {
			parameter_.custom_.beizer_=new math::curve<_Tp>();
			parameter_.custom_.beizer_->start_=std::math::point2<_Tp>(std::math::base_unit_trait<_Tp>::zero(),std::math::base_unit_trait<_Tp>::zero());
			parameter_.custom_.beizer_->end_=std::math::point2<_Tp>(std::math::base_unit_trait<_Tp>::value(),std::math::base_unit_trait<_Tp>::value());
			parameter_.custom_.precision_=(_Tp)100;
			break;
		}
		default: {
			break;
		}
	}	
}

template <typename _Tp>
_Tp std::vision::easing<_Tp>::get_progress_details(_Tp time) {
	if (time<std::math::base_unit_trait<_Tp>::zero() || time>std::math::base_unit_trait<_Tp>::value()) {
		throw std::invalid_argument("Time muse be between 0 and 1");
	}
	if (type_==ET_EXPONENT) {
		if (parameter_.exp_.base_<=1) {
			throw std::invalid_argument("Base must be bigger than or equal to 1");
		}
		if (parameter_.exp_.exponent_<=0) {
			throw std::invalid_argument("Exponent must be bigger than or equal to 0");
		}
	}
	if (type_==ET_ELASTIC) {
		if (parameter_.elastic_.amplitude_<std::math::base_unit_trait<_Tp>::zero() || parameter_.elastic_.amplitude_>std::math::base_unit_trait<_Tp>::value()) {
			throw std::out_of_range("Amplitude must be between 0 and 1");
		}
		if (parameter_.elastic_.period_<std::math::base_unit_trait<_Tp>::zero() || parameter_.elastic_.period_>std::math::base_unit_trait<_Tp>::value()) {
			throw std::out_of_range("Period must be between 0 and 1");
		}
		if (parameter_.elastic_.damping_<std::math::base_unit_trait<_Tp>::zero() || parameter_.elastic_.damping_>std::math::base_unit_trait<_Tp>::value()) {
			throw std::out_of_range("Damping must be between 0 and 1");
		}
	}
	if (type_==ET_BACK) {
		if (parameter_.back_.scale_<(_Tp)2 || parameter_.back_.scale_>(_Tp)4) {
			throw std::out_of_range("Scale must be between 2 and 4");
		}
		if (parameter_.back_.offset_<(_Tp)1.70158 || parameter_.back_.offset_>(_Tp)2.59491) {
			throw std::out_of_range("Offset must be between 1.70158 and 2.59491");
		}
	}
	if (type_==ET_CUSTOM) {
		std::math::point2<_Tp> temp_point;
		temp_point=parameter_.custom_.beizer_->start_;
		if (temp_point.x_<std::math::base_unit_trait<_Tp>::zero() ||
			temp_point.y_<std::math::base_unit_trait<_Tp>::zero() ||
			temp_point.x_>std::math::base_unit_trait<_Tp>::value() ||
			temp_point.y_>std::math::base_unit_trait<_Tp>::value()) {
			//throw std::out_of_range("Beizer must starts from 0 or bigger");
			throw std::out_of_range("Beizer point must be between 0 and 1");
		}
		temp_point=parameter_.custom_.beizer_->end_;
		if (temp_point.x_<std::math::base_unit_trait<_Tp>::zero() || 
			temp_point.y_<std::math::base_unit_trait<_Tp>::zero() ||
			temp_point.x_>std::math::base_unit_trait<_Tp>::value() ||
			temp_point.y_>std::math::base_unit_trait<_Tp>::value()) {
			//throw std::out_of_range("Beizer must ends up with 1 or smaller");
			throw std::out_of_range("Beizer point must be between 0 and 1");
		}
		if (parameter_.custom_.precision_<=0) {
			throw std::invalid_argument("Precision must be bigger than 0");
		}
	}
	_Tp result;
	switch (type_) {
		case ET_LINEAR: {
			/*
				f(t)=t
			*/
			result=time;
			break;
		}
		case ET_QUAD: {
			/*
				f(t)=t^2
			*/
			result=time*time;
			break;
		}
		case ET_INVERSE_QUAD: {
			/*
				f(t)=2t-t^2
			*/
			result=(_Tp)2*time-time*time;
			break;
		}
		case ET_CUBIC: {
			/*
				f(t)=t^3
			*/
			result=time*time*time;
			break;
		}
		case ET_INVERSE_CUBIC: {
			/*
				f(t)=t^3-3t^2+3t;
			*/
			result=time*time*time-(_Tp)3*time*time+(_Tp)3*time;
			break;
		}
		case ET_QUART: {
			/*
				f(t)=t^4
			*/
			result=time*time*time*time;
			break;
		}
		case ET_QUINT: {
			/*
				f(t)=t^5
			*/
			result=time*time*time*time*time;
			break;
		}
		case ET_EXPONENT: {
			/*
				f(t)=b^(e*(t-1))
			*/
			result=time-std::math::base_unit_trait<_Tp>::value();
			result=parameter_.exp_.exponent_*result;
			result=std::math::base_unit_trait<_Tp>::pow(parameter_.exp_.base_,result);
			break;
		}
		case ET_SINE: {
			result=(_Tp)std::math::base_unit_trait<_Tp>::PI()/(_Tp)2*time;
			result=std::math::base_unit_trait<_Tp>::cos(result);
			result=result*std::math::base_unit_trait<_Tp>::neg_unit();
			result=result+std::math::base_unit_trait<_Tp>::value();
			break;
		}
		case ET_CIRCLE: {
			/*
				f(t)=1-(1-t^2)^(1/2)
			*/
			result=time*time;
			result=std::math::base_unit_trait<_Tp>::value()-result;
			result=std::math::base_unit_trait<_Tp>::sqrt(result);
			result=std::math::base_unit_trait<_Tp>::value()-result;
			break;
		}
		case ET_INVERSE_CIRCLE: {
			/*
				f(t)=(2t-t^2)*(1/2)
			*/
			result=std::math::base_unit_trait<_Tp>::sqrt((_Tp)2*time-time*time);
			break;
		}
		case ET_ELASTIC: {
			/*
				f(t)=A*2^(-10t)*sin((t-D)*2*pi/P)+1
				Recommand:g(t)=f(t)+1-f(0)
			*/
			result=(_Tp)-10*time;
			result=std::math::base_unit_trait<_Tp>::pow((_Tp)2,result);
			result=parameter_.elastic_.amplitude_*result;
			_Tp temp=time-parameter_.elastic_.damping_;
			temp=temp*(_Tp)2*std::math::base_unit_trait<_Tp>::PI()/parameter_.elastic_.period_;
			result=result*std::math::base_unit_trait<_Tp>::sin(temp);
			result=result+std::math::base_unit_trait<_Tp>::value();
			break;
		}
		case ET_SWING: {
			/*
				f(t)=0.5(1-cos(pi*t))
			*/
			result=time*std::math::base_unit_trait<_Tp>::PI();
			result=std::math::base_unit_trait<_Tp>::cos(result);
			result=result*(_Tp)0.5;
			result=(_Tp)0.5-result;
			break;
		}
		case ET_BACK: {
			/*
				f(t)=s*t^3-o*t^2	s[2,4] o[1.70158,259491]
				Recommend:g(t)=f(t)+1+o-s;
			*/
			result=parameter_.back_.scale_*time*time*time;
			result=result-parameter_.back_.offset_*time*time;
			break;
		}
		case ET_SMOOTH_BOUNCE: {
			/*
				f(t)=1-abs(cos(10pi*t)*e^(-6*t))
			*/
			result=(_Tp)10*time*std::math::base_unit_trait<_Tp>::PI();
			result=std::math::base_unit_trait<_Tp>::cos(result);	
			_Tp temp=(_Tp)-6*time;
			temp=std::math::base_unit_trait<_Tp>::pow(std::math::base_unit_trait<_Tp>::E(),temp);
			result=result*temp;
			result=std::math::base_unit_trait<_Tp>::abs(result);
			result=std::math::base_unit_trait<_Tp>::value()-result;
			break;
		}
		case ET_LINEAR_BOUNCE: {
			/*
				f(t)=1-abs(2t-1)
			*/
			result=(_Tp)2*time-std::math::base_unit_trait<_Tp>::value();
			result=std::math::base_unit_trait<_Tp>::value()-std::math::base_unit_trait<_Tp>::abs(result);
			break;
		}
		case ET_CONSTANT: {
			/*
				f(t)=C
			*/
			result=parameter_.constant_.val_;
			break;
		}
		case ET_LOGARITHM: {
			/*
				f(t)=log(b,1+(b-1)*t)
			*/
			result=parameter_.logarithm_.base_-std::math::base_unit_trait<_Tp>::value();
			result=result*time;
			result=std::math::base_unit_trait<_Tp>::value()+result;
			result=std::math::base_unit_trait<_Tp>::log(parameter_.logarithm_.base_,result);
			break;
		}
		case ET_SQUARE: {
			/*
				f(t)=t^(1/2)
			*/
			result=std::math::base_unit_trait<_Tp>::sqrt(time);
			break;
		}
		case ET_CURVES: {
			/*
				f(t)=3t^2-2t^3
			*/
			result=(_Tp)3*time*time;
			result=result-(_Tp)2*time*time*time;
			break;
		}
		case ET_QUADS: {
			/*
				f(t)=2t^2		[0,0.5)
				f(t)=4t-1-2t^2	[0.5,1]
			*/
			if (time<(_Tp)0.5) {
				result=(_Tp)2*time*time;
			} else {
				result=(_Tp)4*time-std::math::base_unit_trait<_Tp>::value()-(_Tp)2*time*time;
			}
			break;
		}
		case ET_INVERSE_QUADS: {
			/*
				f(t)=2t-2t^2	[0,0.5)
				f(t)=1-2t+2t^2	[0.5,1]
			*/
			result=time-time*time;
			result=(_Tp)2*result;
			result=time<(_Tp)0.5?result:std::math::base_unit_trait<_Tp>::value()-result;
			break;
		}
		case ET_CUBICS: {
			/*
				f(t)=4t^3				[0,0.5)
				f(t)=4t^3-12t^2+12t-3	[0.5,1]
			*/
			if (time<(_Tp)0.5) {
				result=(_Tp)2*time*time;
			} else {
				result=time-std::math::base_unit_trait<_Tp>::value();
				result=(_Tp)8*result*result*result+(_Tp)2;
				result=result/(_Tp)2;
			}
			break;
		}
		case ET_CUSTOM: {
			std::vector<std::math::point2<_Tp> > temp_points=parameter_.custom_.beizer_->get_points(parameter_.custom_.precision_);
			for (int i=0;i<temp_points.size();i++) {
				std::math::point2<_Tp> temp_point=temp_points[i];
				if (temp_point.x_<std::math::base_unit_trait<_Tp>::zero() || 
					temp_point.y_<std::math::base_unit_trait<_Tp>::zero() ||
					temp_point.x_>std::math::base_unit_trait<_Tp>::value() ||
					temp_point.y_>std::math::base_unit_trait<_Tp>::value())
					throw std::out_of_range("Beizer point must be between 0 and 1");
			}
			for (int i=0;i<temp_points.size()-1;i++) {
				std::math::point2<_Tp> temp_point1=temp_points[i];
				std::math::point2<_Tp> temp_point2=temp_points[i+1];
				if (temp_point1.x_>=temp_point2.x_) {
					throw std::out_of_range("Beizer point must be less than next point");
					break;
				}
			}
			result=std::math::base_unit_trait<_Tp>::neg_unit();
			int low=0,high=temp_points.size()-1;
			while (low<high) {
				int mid=(low+high)/2;
				if (temp_points[mid].x_==time) {
					result=temp_points[mid].y_;
					break;
				}
				else if (temp_points[mid].x_<time) low=mid+1;
				else high=mid;
			}
			if (result!=std::math::base_unit_trait<_Tp>::neg_unit()) break;
			if (low==0) result=temp_points[0].y_;
			else if (low==temp_points.size()) return temp_points[temp_points.size()-1].y_;
			else if (std::math::base_unit_trait<_Tp>::abs(time-temp_points[low-1].x_)<std::math::base_unit_trait<_Tp>::abs(time-temp_points[low].x_)) result=temp_points[low-1].y_;
			else result=temp_points[low].y_;
			//int index=(int)(time*parameter_.custom_.precision_+(_Tp)0.5);
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
_Tp std::vision::easing<_Tp>::get_progress(_Tp time) {
	return get_progress(time,std::vision::EO_EASEIN,std::vision::EM_NORMAL);
}

template <typename _Tp>
_Tp std::vision::easing<_Tp>::get_progress(_Tp time,EASING_OPTION option) {
	return get_progress(time,option,std::vision::EM_NORMAL);
}

template <typename _Tp>
_Tp std::vision::easing<_Tp>::get_progress(_Tp time,EASING_OPTION option,EASING_MIDDLE middle) {
	_Tp result=time;
	switch (option) {
		case EO_EASEIN: {
			result=get_progress_details(result);
			break;
		}
		case EO_EASEOUT: {
			result=std::math::base_unit_trait<_Tp>::value()-get_progress_details(std::math::base_unit_trait<_Tp>::value()-result);
			break;
		}
		case EO_EASEINOUT: {
			if (result<=(_Tp)0.5) {
				result=get_progress_details(result*(_Tp)2)/(_Tp)2;
			} else {
				result=(std::math::base_unit_trait<_Tp>::value()-get_progress_details(std::math::base_unit_trait<_Tp>::value()-(result*(_Tp)2-std::math::base_unit_trait<_Tp>::value()))+std::math::base_unit_trait<_Tp>::value())/(_Tp)2;
			}
			break;
		}
		case EO_EASEIN_STRONG: {
			result=get_progress_details(result);
			result=get_progress_details(result);
			break;
		}
		case EO_EASEOUT_STRONG: {
			result=std::math::base_unit_trait<_Tp>::value()-get_progress_details(std::math::base_unit_trait<_Tp>::value()-result);
			result=std::math::base_unit_trait<_Tp>::value()-get_progress_details(std::math::base_unit_trait<_Tp>::value()-result);
			break;
		}
		case EO_EASEINOUT_STRONG: {
			if (result<=(_Tp)0.5) {
				result=get_progress_details(result*(_Tp)2);
				result=get_progress_details(result)/(_Tp)2;
			} else {
				result=std::math::base_unit_trait<_Tp>::value()-get_progress_details(std::math::base_unit_trait<_Tp>::value()-(result*(_Tp)2-std::math::base_unit_trait<_Tp>::value()));
				result=(std::math::base_unit_trait<_Tp>::value()-get_progress_details(std::math::base_unit_trait<_Tp>::value()-result)+std::math::base_unit_trait<_Tp>::value())/(_Tp)2;
			}
			break;
		}
		default: {
			result=get_progress_details(result);
			break;
		}
	}
	switch (middle) {
		case EM_NORMAL: {
			result=result;
			break;
		}
		case EM_FAST: {
			std::vision::easing<_Tp> temp_easing(std::vision::ET_QUAD);
			result=temp_easing.get_progress(result,option,EM_NORMAL);
			break;
		}
		case EM_SLOW: {
			std::vision::easing<_Tp> temp_easing(std::vision::ET_INVERSE_QUAD);
			result=temp_easing.get_progress(result,option,EM_NORMAL);
			break;
		}
		default: {
			result=result;
			break;
		}
	}
	return result;
}