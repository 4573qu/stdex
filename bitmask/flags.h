//Last Modified At 2025/09/09
//@Version 1.2.1.1
#ifndef _STD4573_BITMASK_FLAGS_H_
#define _STD4573_BITMASK_FLAGS_H_ 1

#include <algorithm>
#include <set>
#include <stdexcept>
#include <type_traits>
#include <map>

namespace stdex {
	
namespace bitmask {

template <typename _Tp>
class flags {
	static_assert(std::is_enum_v<_Tp>,"_Tp must be an enum type.");
protected:
	std::underlying_type_t<_Tp> value_{0};
	
public:
	struct _flags_enhanced : std::false_type {};
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
	constexpr operator _Tp() const noexcept {
		return (_Tp)value_;
	}
	constexpr void clear() noexcept {
		value_=0;
	}
	constexpr bool empty() const noexcept {
		return !value_;
	}
};

enum conflict_policy {
	CP_REJECT,
	CP_REPLACE,
	CP_EXCEPTION,
};

template <typename _Tp,conflict_policy _Policy>
class exclusive_flags : public flags<_Tp> {
	std::map<_Tp,flags<_Tp>*> conflicts_;
public:
	~exclusive_flags() {
		std::set<flags<_Tp>*> conflicts;
		for (auto& it:conflicts_) conflicts.insert(it.second);
		for (auto& it:conflicts) delete it;
	}
	void set_conflict(_Tp lhs,_Tp rhs) {
		if (lhs==rhs) return;
		if (!conflicts_[lhs] && !conflicts_[rhs]) {
			flags<_Tp>* flag=new flags<_Tp>(lhs);
			*flag<<=rhs;
			conflicts_[lhs]=conflicts_[rhs]=flag;
			return;
		}
		if (!conflicts_[lhs]) std::swap(lhs,rhs);
		if (!conflicts_[rhs]) {
			(*conflicts_[lhs])<<=rhs;
			conflicts_[rhs]=conflicts_[lhs];
			return;
		}
		if (conflicts_[lhs]==conflicts_[rhs]) return;
		(*conflicts_[lhs])<<=(_Tp)(*conflicts_[rhs]);
		delete conflicts_[rhs];
		conflicts_[rhs]=conflicts_[lhs];
	}
	void clear_conflict(_Tp e) {
		if (!conflicts_[e]) return;
		(*conflicts_[e])>>=e;
		conflicts_[e]=nullptr;
	}
	constexpr exclusive_flags& operator <<=(_Tp e) noexcept(_Policy==CP_REJECT || _Policy==CP_REPLACE) {
		if (_Policy==CP_REPLACE) {
			if (conflicts_[e]) flags<_Tp>::operator >>=((_Tp)(*conflicts_[e]));
		} else {
			bool conflict=false;
			if (conflicts_[e]) {
				if (flags<_Tp>::value_ & (_Tp)(*conflicts_[e])) conflict=true;
			}
			if (conflict) {
				if (_Policy==CP_REJECT) return *this;
				throw std::invalid_argument("Invalid operation adding element");
			}
		}
		flags<_Tp>::operator <<=(e);
		return *this;
	}
	constexpr exclusive_flags operator <<(_Tp e) const noexcept {
		return exclusive_flags(*this)<<=e;
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