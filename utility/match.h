//Last Modified At 2025/10/10
//@Version 1.0.0.0
#ifndef _STDEX_UTILITY_MATCH_H_
#define _STDEX_UTILITY_MATCH_H_ 1

#include <stdexcept>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>

#include "../macros/cpp_version.h"//At Least 1.0

#ifndef _STDEX_CPP20_VERSION
#define _STDEX_CPP20_VERSION 202002L
#endif

#if __cplusplus>=_STDEX_CPP20_VERSION
#include <concepts>
#endif

namespace stdex {

namespace utility {

#if __cplusplus>=_STDEX_CPP20_VERSION
template <typename _Tp>
concept stdex_tuple_like=requires(_Tp t) {
	std::tuple_size<std::decay_t<_Tp>>::value;
	std::get<0>(t);
};
template <typename _Tp>
concept stdex_variant_like=requires(_Tp t) {
	std::variant_size<std::decay_t<_Tp>>::value;
};
#else
template <typename _Tp,typename=void>
struct is_tuple_like : std::false_type {};

template <typename _Tp>
struct is_tuple_like<_Tp,std::void_t<decltype(std::tuple_size<std::decay_t<_Tp>>::value),decltype(std::get<0>(std::declval<_Tp>()))>> : std::true_type {};

template <typename _Tp>
inline constexpr bool is_tuple_like_v=is_tuple_like<_Tp>::value;

template <typename _Tp,typename=void>
struct is_variant_like : std::false_type {};

template <typename _Tp>
struct is_variant_like<_Tp,std::void_t<decltype(std::variant_size<std::decay_t<_Tp>>::value)>> : std::true_type {};
    
template <typename _Tp>
inline constexpr bool is_variant_like_v=is_variant_like<_Tp>::value;
#endif

struct wildcard_t {
	template <typename _Tp>
	constexpr bool matches(_Tp&&) const noexcept { return true; }
	template <typename _Tp,typename _Func>
	constexpr auto execute(_Tp&& value,_Func&& func) const {
		if constexpr (std::is_invocable_v<_Func,_Tp>) {
			return func(std::forward<_Tp>(value));
		} else {
			return func();
		}
	}
};
inline constexpr wildcard_t _;

template <typename _Tp>
class value_pattern {
	_Tp expected_;
public:
	constexpr value_pattern(_Tp expected) : expected_(std::move(expected)) { }
	template <typename _Up>
	constexpr bool matches(_Up&& value) const {
		return value==expected_;
	}
	template <typename _Up,typename _Func>
	constexpr auto execute(_Up&& value,_Func&& func) const {
		if constexpr (std::is_invocable_v<_Func,_Up>) {
			return func(std::forward<_Up>(value));
		} else {
	    	return func();
		}
	}
};

template <typename _Predicate>
class predicate_pattern {
	_Predicate pred_;
public:
	constexpr predicate_pattern(_Predicate pred) : pred_(std::move(pred)) { }
	template <typename _Tp>
	constexpr bool matches(_Tp&& value) const {
		return pred_(std::forward<_Tp>(value));
	}
	template <typename _Tp,typename _Func>
	constexpr auto execute(_Tp&& value,_Func&& func) const {
		if constexpr (std::is_invocable_v<_Func,_Tp>) {
			return func(std::forward<_Tp>(value));
		} else {
	    	return func();
		}
	}
};

template <typename _Tp>
class type_pattern {
public:
	template <typename _Up>
	constexpr bool matches(_Up&&) const {
		return std::is_same_v<std::decay_t<_Up>,_Tp>;
	}
	template <typename _Up,typename _Func>
	constexpr auto execute(_Up&& value,_Func&& func) const {
		if constexpr (std::is_invocable_v<_Func,_Up>) {
			return func(std::forward<_Up>(value));
		} else {
	    	return func();
		}
	}
};

enum range_bound_type {
	RBT_OPEN,
	RBT_CLOSED
};

template <typename _Low,typename _High,range_bound_type _Lower=RBT_CLOSED,range_bound_type _Upper=RBT_CLOSED>
class range_pattern {
	_Low low_;
	_High high_;
public:
	constexpr range_pattern(_Low low,_High high) : low_(std::move(low)) , high_(std::move(high)) { }
	template <typename _Tp>
	constexpr bool matches(_Tp&& value) const {
		bool lower_ok=_Lower==RBT_CLOSED?(value>=low_):(value>low_);
		bool upper_ok=_Upper==RBT_CLOSED?(value<=high_):(value<high_);
		return lower_ok&&upper_ok;
	}
	template <typename _Tp,typename _Func>
	constexpr auto execute(_Tp&& value,_Func&& func) const {
		if constexpr (std::is_invocable_v<_Func,_Tp>) {
			return func(std::forward<_Tp>(value));
		} else {
	    	return func();
		}
	}
};

template <typename _Low,typename _High>
constexpr auto closed_range(_Low low,_High high) {
	return range_pattern<_Low,_High,RBT_CLOSED,RBT_CLOSED>(std::move(low),std::move(high));
}
template <typename _Low,typename _High>
constexpr auto open_range(_Low low,_High high) {
	return range_pattern<_Low,_High,RBT_OPEN,RBT_OPEN>(std::move(low),std::move(high));
}
template <typename _Low,typename _High>
constexpr auto lopen_range(_Low low,_High high) {
	return range_pattern<_Low,_High,RBT_OPEN,RBT_CLOSED>(std::move(low),std::move(high));
}
template <typename _Low,typename _High>
constexpr auto ropen_range(_Low low,_High high) {
	return range_pattern<_Low,_High,RBT_CLOSED,RBT_OPEN>(std::move(low),std::move(high));
}

template <typename... _Patterns>
class ds_pattern {
	std::tuple<_Patterns...> patterns_;

	template <typename _Tp,size_t... _Is>
	constexpr bool matches_impl(_Tp&& value,std::index_sequence<_Is...>) const {
		return (std::get<_Is>(patterns_).matches(std::get<_Is>(value)) && ...);
	}	
	template <typename _Tp,typename _Func,size_t... _Is>
	constexpr auto execute_impl(_Tp&& value,_Func&& func,std::index_sequence<_Is...>) const {
		if constexpr (std::is_invocable_v<_Func,decltype(std::get<_Is>(std::forward<_Tp>(value)))...>) {
			return func(std::get<_Is>(std::forward<_Tp>(value))...);
		} else {
			return func();
		}
	}

public:
	constexpr ds_pattern(_Patterns... patterns) : patterns_(std::move(patterns)...) { }
	template <typename _Tp>
	constexpr bool matches(_Tp&& value) const {
		if constexpr (sizeof...(_Patterns)==1) {
			return std::get<0>(patterns_).matches(std::forward<_Tp>(value));
		} else {
#if __cplusplus>=_STDEX_CPP20_VERSION
			if constexpr (stdex_tuple_like<_Tp>) {
#else
			if constexpr (is_tuple_like_v<_Tp>) {
#endif
				if (std::tuple_size_v<std::decay_t<_Tp>>!=sizeof...(_Patterns)) return false;
				return matches_impl(std::forward<_Tp>(value),std::index_sequence_for<_Patterns...>{});
			} else return false;
		}
	}
	template <typename _Tp,typename _Func>
	constexpr auto execute(_Tp&& value,_Func&& func) const {
		return execute_impl(std::forward<_Tp>(value),std::forward<_Func>(func),std::index_sequence_for<_Patterns...>{});
	}
};

template <typename _Pattern1,typename _Pattern2>
class and_pattern {
	_Pattern1 p1_;
	_Pattern2 p2_;
public:
	constexpr and_pattern(_Pattern1 p1,_Pattern2 p2) : p1_(std::move(p1)) , p2_(std::move(p2)) { }
	template <typename _Tp>
	constexpr bool matches(_Tp&& value) const {
		return p1_.matches(value) && p2_.matches(value);
	}
	template <typename _Tp,typename _Func>
	constexpr auto execute(_Tp&& value,_Func&& func) const {
		if constexpr (std::is_invocable_v<_Func,_Tp>) {
			return func(std::forward<_Tp>(value));
		} else {
	    	return func();
		}
	}
};

template <typename _Pattern1,typename _Pattern2>
class or_pattern {
	_Pattern1 p1_;
	_Pattern2 p2_;
public:
	constexpr or_pattern(_Pattern1 p1,_Pattern2 p2) : p1_(std::move(p1)) , p2_(std::move(p2)) { }
	template <typename _Tp>
	constexpr bool matches(_Tp&& value) const {
		return p1_.matches(value) || p2_.matches(value);
	}
	template <typename _Tp,typename _Func>
	constexpr auto execute(_Tp&& value,_Func&& func) const {
		if constexpr (std::is_invocable_v<_Func,_Tp>) {
			return func(std::forward<_Tp>(value));
		} else {
	    	return func();
		}
	}
};

template <typename _Pattern>
class not_pattern {
	_Pattern p_;
public:
	constexpr not_pattern(_Pattern p) : p_(std::move(p)) { }
	template <typename _Tp>
	constexpr bool matches(_Tp&& value) const {
		return !p_.matches(value);
	}
	template <typename _Tp,typename _Func>
	constexpr auto execute(_Tp&& value,_Func&& func) const {
		if constexpr (std::is_invocable_v<_Func,_Tp>) {
			return func(std::forward<_Tp>(value));
		} else {
	    	return func();
		}
	}
};

template<typename _Tp>
constexpr auto pattern(_Tp&& value) {
	if constexpr (std::is_same_v<std::decay_t<_Tp>,wildcard_t>) {
		return wildcard_t{};
	} else {
		return value_pattern<std::decay_t<_Tp>>(std::forward<_Tp>(value));
	}
}
template <typename _Tp>
constexpr auto as() {
	return type_pattern<_Tp>{};
}
template <typename... _Patterns>
constexpr auto ds(_Patterns... patterns) {
	return ds_pattern<_Patterns...>(std::move(patterns)...);
}
template <typename _Predicate>
constexpr auto when(_Predicate pred) {
	return predicate_pattern<_Predicate>(std::move(pred));
}
template <typename _Tp>
constexpr auto operator ==(wildcard_t,_Tp&& value) {
	return value_pattern<std::decay_t<_Tp>>(std::forward<_Tp>(value));
}
template <typename _Tp>
constexpr auto operator !=(wildcard_t,_Tp&& value) {
	return predicate_pattern([value=std::forward<_Tp>(value)](auto&& x){ 
		return x!=value;
	});
}
template <typename _Tp>
constexpr auto operator <(wildcard_t,_Tp&& value) {
	return predicate_pattern([value=std::forward<_Tp>(value)](auto&& x){ 
		return x<value;
	});
}
template <typename _Tp>
constexpr auto operator <=(wildcard_t,_Tp&& value) {
	return predicate_pattern([value=std::forward<_Tp>(value)](auto&& x){ 
		return x<=value;
	});
}
template <typename _Tp>
constexpr auto operator >(wildcard_t,_Tp&& value) {
	return predicate_pattern([value=std::forward<_Tp>(value)](auto&& x){ 
		return x>value; 
	});
}
template <typename _Tp>
constexpr auto operator >=(wildcard_t,_Tp&& value) {
	return predicate_pattern([value=std::forward<_Tp>(value)](auto&& x){ 
		return x>=value; 
	});
}
template <typename _Pattern1,typename _Pattern2>
constexpr auto operator &&(_Pattern1 p1,_Pattern2 p2) {
	return and_pattern<_Pattern1,_Pattern2>(std::move(p1),std::move(p2));
}
template <typename _Pattern1,typename _Pattern2>
constexpr auto operator ||(_Pattern1 p1,_Pattern2 p2) {
	return or_pattern<_Pattern1,_Pattern2>(std::move(p1),std::move(p2));
}
template <typename _Pattern>
constexpr auto operator !(_Pattern p) {
	return not_pattern<_Pattern>(std::move(p));
}

template <typename _Pattern,typename _Action>
struct case_wrapper {
	_Pattern pattern;
	_Action action;
	constexpr case_wrapper(_Pattern p,_Action a) : pattern(std::move(p)) , action(std::move(a)) { }
};

template <typename _Pattern,typename _Action>
constexpr auto operator >>(_Pattern&& pattern,_Action&& action) {
	return case_wrapper<_Pattern,_Action>(std::forward<_Pattern>(pattern),std::forward<_Action>(action));
}

struct match_options {
    bool throw_on_no_match=true;
};

template <typename _Tp>
class matcher {
	_Tp&& value_;
	match_options options_;

	template <typename _Case,typename... _Rest>
	constexpr auto match_impl(_Case&& case_,_Rest&&... rest) {
		if (case_.pattern.matches(value_)) return case_.pattern.execute(value_,std::move(case_.action));
		if constexpr (sizeof...(_Rest)>0) {
			return match_impl(std::forward<_Rest>(rest)...);
		} else {
			if constexpr (std::is_same_v<std::decay_t<decltype(case_.pattern)>,wildcard_t>) {
				return case_.pattern.execute(value_,std::move(case_.action));
			} else {
				if (options_.throw_on_no_match) throw std::runtime_error("Pattern match failed: no case matched");
				else {
					using result_type=decltype(case_.pattern.execute(value_,std::move(case_.action)));
					if constexpr (std::is_void_v<result_type>) {
						return;
					} else {
						return result_type{};
					}
				}	
			}
		}
	}

public:
	constexpr matcher(_Tp&& value,match_options opts={}) : value_(std::forward<_Tp>(value)) , options_(opts) { }
	template <typename... _Cases>
	constexpr auto operator()(_Cases&&... cases) {
		return match_impl(std::forward<_Cases>(cases)...);
	}
};

template <typename _Tp>
constexpr auto match(_Tp&& value,match_options opts={}) {
	return matcher<_Tp>(std::forward<_Tp>(value),opts);
}

constexpr match_options no_throw{false};

}

}

#endif