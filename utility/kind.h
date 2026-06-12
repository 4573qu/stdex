//Last Modified At 2026/06/10
//@Version 2.0.0.0
#ifndef _STDEX_UTILITY_KIND_H_
#define _STDEX_UTILITY_KIND_H_ 1

#include <type_traits>

#if __has_include("../macros/cpp_version.h")
#include "../macros/cpp_version.h"//At Least 1.0
#endif

#ifndef _STDEX_CPP20_VERSION
#define _STDEX_CPP20_VERSION 202002L
#endif

namespace stdex {

namespace utility {

template <typename _Base,typename _Tp=int>
struct kind {
	using self_type=_Base;
	using value_type=_Tp;
	static_assert(std::is_integral<_Tp>::value,"kind value_type must be an integral type.");

	value_type value;

	constexpr kind(value_type value) noexcept : value(value) { }
	constexpr operator value_type() const noexcept { return value; }

	constexpr bool operator ==(const kind& other) const { return value==other.value; }
	constexpr bool operator !=(const kind& other) const { return value!=other.value; }
	constexpr bool operator <(const kind& other) const { return value<other.value; }

	constexpr bool operator ==(value_type other) const noexcept { return value==other; }
	constexpr bool operator !=(value_type other) const noexcept { return value!=other; }
	constexpr bool operator <(value_type other) const noexcept { return value<other; }

#if __cplusplus>=_STDEX_CPP20_VERSION
	constexpr auto operator <=>(const kind& other) const { return value<=>other.value; }
	constexpr auto operator <=>(value_type other) const noexcept { return value<=>other; }
#else
	constexpr bool operator <=(const kind& other) const { return value<=other.value; }
	constexpr bool operator >(const kind& other) const { return value>other.value; }
	constexpr bool operator >=(const kind& other) const { return value>=other.value; }
	constexpr bool operator <=(value_type other) const noexcept { return value<=other; }
	constexpr bool operator >(value_type other) const noexcept { return value>other; }
	constexpr bool operator >=(value_type other) const noexcept { return value>=other; }
	friend constexpr bool operator ==(value_type lhs,const kind& rhs) noexcept { return lhs==rhs.value; }
	friend constexpr bool operator !=(value_type lhs,const kind& rhs) noexcept { return lhs!=rhs.value; }
	friend constexpr bool operator <(value_type lhs,const kind& rhs) noexcept { return lhs<rhs.value; }
	friend constexpr bool operator <=(value_type lhs,const kind& rhs) noexcept { return lhs<=rhs.value; }
	friend constexpr bool operator >(value_type lhs,const kind& rhs) noexcept { return lhs>rhs.value; }
	friend constexpr bool operator >=(value_type lhs,const kind& rhs) noexcept { return lhs>=rhs.value; }
#endif

	static constexpr value_type begin_value=static_cast<value_type>(0);
};

template <typename _Derived,typename _Base,typename _Tp=typename _Base::value_type>
struct kind_derived : _Base {
	using self_type=_Derived;
	using base_type=_Base;
	using value_type=_Tp;
	using _Base::_Base;

	constexpr kind_derived(_Tp value) noexcept : _Base(value) { }

	static constexpr value_type begin_value=_Base::end_value;
};

}

}

#define _STDEX_KIND_AUTO_START (-1)

#define _STDEX_KIND(name,type,...) \
enum name##_values : type { \
	__VA_ARGS__ \
	name##_end_value \
}; \
struct name : stdex::utility::kind<name,type> { \
	using stdex_kind_self=name; \
	using enumeration_type=name##_values; \
	using stdex::utility::kind<name,type>::kind; \
	static constexpr type end_value=static_cast<type>(name##_end_value); \
	static constexpr type get_next() noexcept { return end_value; } \
};

#define _STDEX_DERIVED_KIND(name,base,start,...) \
static_assert((start)==_STDEX_KIND_AUTO_START || (start)>=base::get_next(),"Start value cannot be less than the next available value."); \
enum name##_values : base::value_type { \
	name##_begin_value=((start)==_STDEX_KIND_AUTO_START?base::get_next():(start))-1, \
	__VA_ARGS__ \
	name##_end_value \
}; \
struct name : stdex::utility::kind_derived<name,base> { \
	using stdex_kind_self=name; \
	using enumeration_type=name##_values; \
	using stdex::utility::kind_derived<name,base>::kind_derived; \
	static constexpr value_type end_value=static_cast<value_type>(name##_end_value); \
	static constexpr value_type get_next() noexcept { return end_value; } \
};

#define _STDEX_KIND_VALUE(name,value) name=(value),
#define _STDEX_KIND_VALUE_AUTO(name) name,

#endif