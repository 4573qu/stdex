//Last Modified At 2025/11/05
//@Version 1.0.0.0
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
	using value_type=_Tp;
	value_type value_;

	constexpr kind(value_type value) : value_(value) {
		if (value>=next_value_) next_value_=value+1;
	}
	constexpr operator value_type() const { return value_; }

	constexpr bool operator ==(const kind& other) const { return value_==other.value_; }
	constexpr bool operator !=(const kind& other) const { return value_!=other.value_; }
	constexpr bool operator <(const kind& other) const { return value_<other.value_; }

#if __cplusplus>=_STDEX_CPP20_VERSION
	constexpr auto operator <=>(const kind& other) const { return value<=>other.value_; }
#else
	constexpr bool operator <=(const kind& other) const { return value_<=other.value_; }
	constexpr bool operator >(const kind& other) const { return value_>other.value_; }
	constexpr bool operator >=(const kind& other) const { return value_>=other.value_; }
#endif
protected:
	static inline value_type next_value_;
	static constexpr value_type get_next() { return next_value_; }
};

template <typename _Derived,typename _Base,typename _Tp=_Base::value_type>
struct kind_derived : kind {
	using _Base::_Base;

	constexpr kind_derived(_Tp value) : _Base(value) { }

	struct auto_start { };
	static constexpr auto_start auto_start_ = { };

	constexpr kind_derived(auto_start) : _Base(_Base::get_next()) { }
};

}

}

#define _STDEX_KIND(name,type,...) \
struct name : stdex::utility::kind<name,type> { \
	using stdex::utility::kind<name,type>::kind; \
	__VA_ARGS__ \
};

#define _STDEX_DERIVED_KIND(name,base,start,...) \
struct name : stdex::utility::kind_derived<name,base> { \
	static_assert(start==_STDEX_KIND_AUTO_START || start>=base::get_next(),"Start value cannot be negative than next available value."); \
	using stdex::utility::kind_derived<name,base>::kind_derived; \
	__VA_ARGS__ \
};

#define _STDEX_KIND_VALUE(name,value) static constexpr name name{value};
#define _STDEX_KIND_VALUE_AUTO(name) static constexpr name name{stdex::utility::kind_derived<std::decay_t<decltype(name)>,std::decay_t<decltype(name)>>::auto_start_};
#define _STDEX_KIND_AUTO_START stdex::utility::kind_derived<std::decay_t<decltype(*this)>,std::decay_t<decltype(*this)>>::AUTO_START

#endif