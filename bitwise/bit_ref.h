//Last Modified At 2025/09/13
//@Version 1.0.0.0
#ifndef _STDEX_BITWISE_BIT_REF_H_
#define _STDEX_BITWISE_BIT_REF_H_ 1

#include <cstdint>
#include <type_traits>

namespace stdex {

namespace bitwise {

class bit_ref {
	uint8_t* data_;
	int index_;

public:
	bit_ref()=delete;
	bit_ref(const bit_ref&)=default;
	bit_ref(bit_ref&&)=default;

	template <typename _Tp>
	bit_ref(_Tp& data,int index) noexcept : data_(reinterpret_cast<uint8_t*>(&data)) , index_(index) {
		static_assert(std::is_unsigned_v<_Tp>,"_Tp must be an unsigned type.");
	}
	template <typename _Tp>
	bit_ref(const _Tp& data,int index) noexcept : data_(reinterpret_cast<uint8_t*>(const_cast<_Tp*>(&data))) , index_(index) {
		static_assert(std::is_unsigned_v<_Tp>, "_Tp must be an unsigned type.");
	}

	bit_ref& operator =(bool value) noexcept {
		if (value) *data_|=(1<<index_);
		else *data_&=~(1<<index_);
		return *this;
	}
	operator bool() const noexcept {
		return (*data_&(1<<index_));
	}
	bool operator ~() const noexcept {
		return !static_cast<bool>(*this);
	}
	void swap(bit_ref other) noexcept {
		bool temp=static_cast<bool>(*this);
		*this=static_cast<bool>(other);
		other=temp;
	}
};

inline void swap(bit_ref& lhs,bit_ref& rhs) noexcept {
	lhs.swap(rhs);
}

}

}

#endif