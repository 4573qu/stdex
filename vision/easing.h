//Last Modified At 2026/06/02
//@Version 1.1.0.0
#ifndef _STDEX_VISION_EASING_H_
#define _STDEX_VISION_EASING_H_ 1

#include <type_traits>

#include "../math/geometry/primitives.h"//At Least 1.0

namespace stdex {
	
namespace vision {

enum easing_type {
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
	ET_UNKNOWN,
};

enum easing_option {
	EO_EASEIN,
	EO_EASEOUT,
	EO_EASEINOUT,
	EO_EASEIN_STRONG,
	EO_EASEOUT_STRONG,
	EO_EASEINOUT_STRONG,
	EO_UNKNOWN,
};

enum easing_inflection {
	EI_NORMAL,
	EI_FAST,
	EI_SLOW,
	EI_UNKNOWN,
};

template <typename _Tp=double>
class easing {
	static_assert(!std::is_integral<_Tp>::value,"_Tp cannot be an integral type for get(value) use _Tp as the type of parameter.");

public:
	easing_type type;
	union {
		struct {
			_Tp base;
			_Tp exponent;
		} exp;
		struct {
			_Tp amplitude;
			_Tp period;
			_Tp damping;
		} elastic;
		struct {
			_Tp scale;
			_Tp offset;
		} back;
		struct {
			_Tp value;
		} constant;
		struct {
			_Tp base;
		} logarithm;
		struct {
			math::curve<_Tp,2>* beizer;
			_Tp precision;
		} custom;
	} parameter;

private:
	_Tp progress(_Tp time);

public:
	easing(easing_type type=ET_UNKNOWN);

	_Tp get(_Tp time);	
	_Tp get(_Tp time,easing_option option,easing_inflection inflection=EI_NORMAL);
};

}
	
}

#endif