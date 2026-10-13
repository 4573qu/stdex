//Last Modified At 2026/10/13
//@Version 2.1.0.0
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

template <typename _Tp,typename=void>
struct is_kind : std::false_type { };

template <typename _Tp>
struct is_kind<_Tp,std::void_t<typename _Tp::stdex_kind_self>> : std::true_type { };

template <typename _Tp>
constexpr bool is_kind_v=is_kind<_Tp>::value;

template <typename _Lhs,typename _Rhs>
struct kind_related : std::integral_constant<bool,std::is_base_of<_Lhs,_Rhs>::value || std::is_base_of<_Rhs,_Lhs>::value> { };

template <typename _Lhs,typename _Rhs>
constexpr bool kind_related_v=kind_related<_Lhs,_Rhs>::value;

template <typename _Enum,typename=void>
struct kind_enum_owner { using type=void; };

template <typename _Enum>
struct kind_enum_owner<_Enum,std::void_t<decltype(stdex_kind_owner_of(std::declval<_Enum>()))>> {
	using type=decltype(stdex_kind_owner_of(std::declval<_Enum>()));
};

template <typename _Enum>
using kind_enum_owner_t=typename kind_enum_owner<_Enum>::type;

template <typename _Lhs,typename _Rhs>
struct kind_enum_unrelated : std::integral_constant<bool,
	!std::is_same<kind_enum_owner_t<_Lhs>,void>::value
	&& !std::is_same<kind_enum_owner_t<_Rhs>,void>::value
	&& !kind_related<kind_enum_owner_t<_Lhs>,kind_enum_owner_t<_Rhs>>::value> { };

namespace kind_adl {

void stdex_kind_owner_of(...);

}

using kind_adl::stdex_kind_owner_of;

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

template <typename _Lhs,typename _Rhs,std::enable_if_t<
	is_kind<_Lhs>::value && is_kind<_Rhs>::value && !kind_related<_Lhs,_Rhs>::value,int> =0>
constexpr bool operator ==(const _Lhs&,const _Rhs&) noexcept=delete;

template <typename _Lhs,typename _Rhs,std::enable_if_t<
	is_kind<_Lhs>::value && is_kind<_Rhs>::value && !kind_related<_Lhs,_Rhs>::value,int> =0>
constexpr bool operator !=(const _Lhs&,const _Rhs&) noexcept=delete;

template <typename _Lhs,typename _Rhs,std::enable_if_t<
	is_kind<_Lhs>::value && is_kind<_Rhs>::value && !kind_related<_Lhs,_Rhs>::value,int> =0>
constexpr bool operator <(const _Lhs&,const _Rhs&) noexcept=delete;

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
}; \
constexpr name stdex_kind_owner_of(name##_values) noexcept { return name(static_cast<type>(0)); }

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
	static constexpr bool owns(value_type v) noexcept { \
		return v>=static_cast<value_type>(name##_begin_value)+1 && v<end_value; \
	} \
}; \
constexpr name stdex_kind_owner_of(name##_values) noexcept { return name(static_cast<base::value_type>(name##_begin_value)+1); } \
template <typename _Other,std::enable_if_t<stdex::utility::kind_enum_unrelated<name##_values,_Other>::value,int> =0> \
constexpr bool operator ==(name##_values,_Other) noexcept=delete; \
template <typename _Other,std::enable_if_t<stdex::utility::kind_enum_unrelated<name##_values,_Other>::value,int> =0> \
constexpr bool operator !=(name##_values,_Other) noexcept=delete;

#define _STDEX_KIND_ASSERT_DISTINCT(lhs,rhs) \
static_assert(!stdex::utility::kind_related<lhs,rhs>::value,"These two kinds share a lineage; they are not isolated from each other.");

#define _STDEX_KIND_VALUE(name,value) name=(value),
#define _STDEX_KIND_VALUE_AUTO(name) name,

#endif