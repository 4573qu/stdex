//Last Modified At 2026/10/07
//@Version 2.0.0.0
#ifndef _STDEX_UTILITY_KIND_H_
#define _STDEX_UTILITY_KIND_H_ 1

#include <cstddef>
#include <limits>
#include <type_traits>

#if __has_include("../macros/cpp_version.h")
#include "../macros/cpp_version.h"//At Least 1.0
#endif

#ifndef _STDEX_CPP20_VERSION
#define _STDEX_CPP20_VERSION 202002L
#endif

namespace stdex {

namespace utility {

constexpr int kind_ordinal_bits=12;

template <typename _Tp>
constexpr int kind_tag_bits() noexcept {
	return std::numeric_limits<_Tp>::digits-kind_ordinal_bits;
}

constexpr unsigned long long kind_name_hash(const char* text) noexcept {
	unsigned long long result=14695981039346656037ull;
	while (*text) {
		result^=static_cast<unsigned long long>(static_cast<unsigned char>(*text++));
		result*=1099511628211ull;
	}
	return result;
}

template <typename _Tp>
constexpr _Tp kind_tag_of(const char* name) noexcept {
	const unsigned long long limit=(1ull<<static_cast<unsigned>(kind_tag_bits<_Tp>()))-1ull;
	return static_cast<_Tp>(kind_name_hash(name)%limit+1ull);
}

template <typename _Tp>
constexpr _Tp kind_begin_of(const char* name,long long start) noexcept {
	const _Tp ordinal=static_cast<_Tp>((start<0)?0:start);
	return static_cast<_Tp>((kind_tag_of<_Tp>(name)<<kind_ordinal_bits)|ordinal);
}

template <typename _Tp>
constexpr _Tp kind_tag_of_value(_Tp value) noexcept {
	return static_cast<_Tp>(value>>kind_ordinal_bits);
}

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

template <typename _Lhs,typename _Rhs>
struct kind_distinct : std::integral_constant<bool,(_Lhs::kind_tag!=_Rhs::kind_tag)> { };

template <typename _Lhs,typename _Rhs>
constexpr bool kind_distinct_v=kind_distinct<_Lhs,_Rhs>::value;

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
	static constexpr type kind_tag=static_cast<type>(0); \
	static constexpr type end_value=static_cast<type>(name##_end_value); \
	static constexpr type get_next() noexcept { return end_value; } \
};

#define _STDEX_DERIVED_KIND(name,base,start,...) \
static_assert((start)==_STDEX_KIND_AUTO_START || ((start)>=0 && (start)<(1<<stdex::utility::kind_ordinal_bits)),"Start value must be a valid ordinal within this kind."); \
enum name##_values : base::value_type { \
	name##_begin_value=stdex::utility::kind_begin_of<base::value_type>(#name,(start))-1, \
	__VA_ARGS__ \
	name##_end_value \
}; \
struct name : stdex::utility::kind_derived<name,base> { \
	using stdex_kind_self=name; \
	using enumeration_type=name##_values; \
	using stdex::utility::kind_derived<name,base>::kind_derived; \
	static constexpr value_type kind_tag=stdex::utility::kind_tag_of<value_type>(#name); \
	static constexpr value_type begin_value=static_cast<value_type>(name##_begin_value)+1; \
	static constexpr value_type end_value=static_cast<value_type>(name##_end_value); \
	static constexpr value_type get_next() noexcept { return end_value; } \
	static constexpr bool owns(value_type value) noexcept { \
		return stdex::utility::kind_tag_of_value(value)==kind_tag; \
	} \
	static_assert(static_cast<base::value_type>(name##_end_value)-static_cast<base::value_type>(name##_begin_value)<=(1<<stdex::utility::kind_ordinal_bits),"Too many values for a single derived kind."); \
};

#define _STDEX_KIND_ASSERT_DISTINCT(lhs,rhs) \
static_assert(stdex::utility::kind_distinct<lhs,rhs>::value,"Two derived kinds share the same lineage tag; rename one of them.");

#define _STDEX_KIND_VALUE(name,value) name=(value),
#define _STDEX_KIND_VALUE_AUTO(name) name,

#endif