//Last Modified At 2025/09/14
//@Version 1.0.0.1
#ifndef _STD4573_BITWISE_BIT_FIELD_H_
#define _STD4573_BITWISE_BIT_FIELD_H_ 1

#include <climits>
#include <cstdint>
#include <type_traits>

namespace stdex {

namespace bitwise {

template <typename _Tp,int _Offset,int _Count>
class bit_field {
	static_assert(std::is_unsigned_v<_Tp>,"_Tp must be an unsigned type.");
	static_assert(_Offset>=0,"_Offset must be non-negative.");
	static_assert(_Count>0,"_Count must be positive.");
	static_assert(_Offset+_Count<=sizeof(_Tp)*8,"Bit_field out of range.");
	_Tp& storage_;
	static constexpr _Tp mask_=(_Tp(1)<<_Count)-1;

public:
	explicit bit_field(_Tp& storage) noexcept : storage_(storage) { }
	bit_field(const bit_field& other) noexcept : storage_(other.storage_) { }
	bit_field& operator =(const bit_field&)=delete;
	bit_field& operator =(_Tp value) noexcept {
		set(value);
		return *this;
	}
	operator _Tp() const noexcept {
		return get();
	}
	_Tp get() const noexcept {
		return (storage_>>_Offset)&mask_;
	}
	void set(_Tp value) noexcept {
		_Tp mask=mask_<<_Offset;
		storage_=(storage_&~mask)|((value&mask_)<<_Offset);
	}
};

}

}

#define _STDEX_BIT_BIND(raw,offset,bits,name) \
stdex::bitwise::bit_field<std::remove_reference_t<decltype(raw)>,(offset),(bits)> name{raw}
#define _STDEX_BIT_BIND_T(raw,offset,type,name) _STDEX_BIT_BIND(raw,offset,sizeof(type)*CHAR_BIT,name)

#endif