//Last Modified At 2024/09/09
//@Version 1.2
#ifndef _STD4573_VISION_EASING_H_
#define _STD4573_VISION_EASING_H_ 1
#include <stdexcept>
#include <math.h>
#include "../math/geometry/primitives.h"

namespace std {
	
namespace vision {

enum EASING_TYPE {
	ET_LINEAR,
	ET_QUAD,
	ET_INVERSE_QUAD,
	ET_CUBIC,
	ET_INVERSE_CUBIC,
	ET_QUART,
	ET_QUINT,
	ET_EXPONENT,
	ET_SINE,
	ET_CIRCLE,
	ET_INVERSE_CIRCLE,
	ET_ELASTIC,
	ET_SWING,
	ET_BACK,
	ET_SMOOTH_BOUNCE,
	ET_LINEAR_BOUNCE,
	ET_CONSTANT,
	ET_LOGARITHM,
	ET_SQUARE,
	ET_CURVES,
	ET_QUADS,
	ET_INVERSE_QUADS,
	ET_CUBICS,
	ET_CUSTOM,
	ET_UNKNOWN
};

enum EASING_OPTION {
	EO_EASEIN,
	EO_EASEOUT,
	EO_EASEINOUT,
	EO_EASEIN_STRONG,
	EO_EASEOUT_STRONG,
	EO_EASEINOUT_STRONG,
	EO_UNKNOWN
};

enum EASING_MIDDLE {
	EM_NORMAL,
	EM_FAST,
	EM_SLOW,
	EM_UNKNOWN
};

template <typename _Tp>
class easing {
public:
	EASING_TYPE type_;
	union {
		struct {
			_Tp base_;
			_Tp exponent_;
		} exp_;
		struct {
			_Tp amplitude_;
			_Tp period_;
			_Tp damping_;
		} elastic_;
		struct {
			_Tp scale_;
			_Tp offset_;
		} back_;
		struct {
			_Tp val_;
		} constant_;
		struct {
			_Tp base_;
		} logarithm_;
		struct {
			math::curve<_Tp>* beizer_;
			_Tp precision_;
		} custom_;
	} parameter_;
public:
	easing(EASING_TYPE type=ET_UNKNOWN);
	_Tp get_progress_details(_Tp time);
	_Tp get_progress(_Tp time);
	_Tp get_progress(_Tp time,EASING_OPTION option);
	_Tp get_progress(_Tp time,EASING_OPTION option,EASING_MIDDLE middle=EM_NORMAL);
};

}
	
}

#endif