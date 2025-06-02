//Last Modified At 2025/06/02
//@Version 1.1.0.1
#ifndef _STD4573_BITMASK_FLAGS_H_
#define _STD4573_BITMASK_FLAGS_H_ 1

#include <type_traits>

namespace stdex {
	
namespace bitmask {

template <typename _Tp>
class flags {
	struct _flags_enhanced : std::false_type {};
	static_assert(std::is_enum_v<_Tp>,"_Tp must be an enum type.");
	std::underlying_type_t<_Tp> value_{0};
	
public:
	constexpr flags()=default;
	constexpr flags(_Tp e) : value_(static_cast<std::underlying_type_t<_Tp>>(e)) {}
	constexpr flags& operator =(_Tp e) noexcept {
		value_=static_cast<std::underlying_type_t<_Tp>>(e);
		return *this;
	}
	constexpr flags& operator <<=(_Tp e) noexcept {
		value_|=static_cast<std::underlying_type_t<_Tp>>(e);
		return *this;
	}
	constexpr flags operator <<(_Tp e) const noexcept {
		return flags(*this)<<=e;
	}
	constexpr flags& operator >>=(_Tp e) noexcept {
		value_&=~static_cast<std::underlying_type_t<_Tp>>(e);
		return *this;
	}
	constexpr flags operator >>(_Tp e) const noexcept {
		return flags(*this)>>=e;
	}
	constexpr bool contains(_Tp e) const noexcept {
		return (value_ & static_cast<std::underlying_type_t<_Tp>>(e));
	}
	constexpr operator typename std::underlying_type_t<_Tp>() const noexcept {
		return value_;
	}
	constexpr void clear() noexcept {
		value_=0;
	}
	constexpr bool empty() const noexcept {
		return !value_;
	}
};	

}

}

#define _STDEX_ENABLE_FLAGS_ENHANCED(EnumType) \
template<> \
struct stdex::bitmask::flags<EnumType>::_flags_enhanced : std::true_type {}; \
constexpr stdex::bitmask::flags<EnumType> operator <<(EnumType lhs,EnumType rhs) noexcept { \
	return stdex::bitmask::flags<EnumType>(lhs)<<rhs; \
} \
constexpr stdex::bitmask::flags<EnumType> operator >>(EnumType lhs,EnumType rhs) noexcept { \
	return stdex::bitmask::flags<EnumType>(lhs)>>rhs; \
} 

#endif