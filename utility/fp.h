//Last Modified At 2026/05/16
//@Version 1.0.0.0
#ifndef _STDEX_UTILITY_FP_H_
#define _STDEX_UTILITY_FP_H_ 1

#include <algorithm>
#include <any>
#include <cstddef>
#include <functional>
#include <iterator>
#include <numeric>
#include <optional>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

#if __has_include("../macros/cpp_version.h")
#include "../macros/cpp_version.h"//At Least 1.0
#endif

#ifndef _STDEX_CPP20_VERSION
#define _STDEX_CPP20_VERSION 202002L
#endif
#ifndef _STDEX_CPP23_VERSION
#define _STDEX_CPP23_VERSION 202302L
#endif

#if __cplusplus>=_STDEX_CPP20_VERSION
#include <concepts>
#include <ranges>
#endif

namespace stdex {

namespace utility {

template <typename _Func,typename... _Args>
class curried_function {
	_Func func_;
	std::tuple<_Args...> args_;

public:
	curried_function(_Func func,std::tuple<_Args...> args) : func_(std::move(func)) , args_(std::move(args)) { }

	template <typename... _NewArgs>
	auto operator ()(_NewArgs&&... new_args) const {
		auto all_args=std::tuple_cat(args_,std::make_tuple(std::forward<_NewArgs>(new_args)...));
		if constexpr (std::is_invocable_v<_Func,_Args...,_NewArgs...>) {
			return std::apply(func_,std::move(all_args));
		} else {
			return curried_function<_Func,_Args...,std::decay_t<_NewArgs>...>(func_,std::move(all_args));
		}
	}
};

template <typename _Func>
auto curry(_Func&& func) {
	return curried_function<std::decay_t<_Func>>(std::forward<_Func>(func),std::tuple<>{});
}

template <typename _Tp,typename _Func,typename... _Args>
auto operator |(_Tp&& arg,const curried_function<_Func,_Args...>& func) {
	return func(std::forward<_Tp>(arg));
}

template <typename _Func1,typename _Func2>
auto compose(_Func1&& func1,_Func2&& func2) {
	return [f=std::forward<_Func1>(func1),g=std::forward<_Func2>(func2)](auto&&... args){
		return f(g(std::forward<decltype(args)>(args)...));
	};
}

template <typename _Func,typename... _Funcs>
auto compose(_Func&& func,_Funcs&&... funcs) {
	return compose(std::forward<_Func>(func),compose(std::forward<_Funcs>(funcs)...));
}

inline auto map=curry([](auto func,auto container){
#if __cplusplus>=_STDEX_CPP20_VERSION
	static_assert(std::ranges::range<decltype(container)>,"map requires a range.");
	std::vector<std::decay_t<decltype(func(*std::begin(container)))>> result;
	result.reserve(std::ranges::distance(container));
#else
	std::vector<std::decay_t<decltype(func(*std::begin(container)))>> result;
	result.reserve(std::size(container));
#endif
	for (auto&& it:container) result.push_back(func(std::forward<decltype(it)>(it)));
	return result;
});

#if __cplusplus>=_STDEX_CPP23_VERSION
template <template <typename...> typename _Container>
auto map_as=curry([](auto func,auto container){
	return container | std::views::transform(func) | std::ranges::to<_Container>();
});
#endif

inline auto filter=curry([](auto predicate,auto container){
	std::decay_t<decltype(container)> result;
	auto inserter=std::inserter(result,std::end(result));
	for (auto&& it:container) {
		if (predicate(it)) *inserter++=std::move(it);
	}
	return result;
});

inline auto reduce=curry([](auto init,auto binary_op,auto container){
	return std::accumulate(std::begin(container),std::end(container),init,binary_op);
});

inline auto fold_left=curry([](auto init,auto binary_op,auto container){
	auto result=init;
	for (auto&& it:container) result=binary_op(result,it);
	return result;
});

inline auto fold_right=curry([](auto init,auto binary_op,auto container){
	auto result=init;
	for (auto it=std::rbegin(container);it!=std::rend(container);it++) result=binary_op(*it,result);
	return result;
});

inline auto scanl=curry([](auto init,auto binary_op,auto container){
	std::vector<std::decay_t<decltype(init)>> result;
	result.push_back(init);
	auto acc=std::move(init);
	for (auto&& it:container) {
		acc=binary_op(std::move(acc),std::forward<decltype(it)>(it));
		result.push_back(acc);
	}
	return result;
});

auto scanr=curry([](auto init,auto binary_op,auto container){
	std::vector<std::decay_t<decltype(init)>> result;
	result.push_back(init);
	auto acc=std::move(init);
	for (auto it=std::rbegin(container);it!=std::rend(container);it++) {
		acc=binary_op(std::move(*it),std::move(acc));
		result.push_back(acc);
	}
	std::reverse(result.begin(),result.end());
	return result;
});

inline auto zip=curry([](auto c1,auto c2){
	std::vector<std::pair<std::decay_t<decltype(*std::begin(c1))>,std::decay_t<decltype(*std::begin(c2))>>> result;
	auto it1=std::begin(c1);
	auto it2=std::begin(c2);
	while (it1!=std::end(c1) && it2!=std::end(c2)) {
		result.emplace_back(*it1,*it2);
		it1++;
		it2++;
 	}
	return result;
});

inline auto zip_with=curry([](auto binary_op,auto c1,auto c2){
#if __cplusplus>=_STDEX_CPP23_VERSION
	return std::views::zip_transform(binary_op,c1,c2) | std::ranges::to<std::vector<std::decay_t<decltype(binary_op(*std::begin(c1),*std::begin(c2)))>>>();
#else
#if __cplusplus>=_STDEX_CPP20_VERSION
	static_assert(std::ranges::range<decltype(c1)>,"zip_with requires c1 to be a range.");
	static_assert(std::ranges::range<decltype(c2)>,"zip_with requires c2 to be a range.");
#endif
	std::vector<std::decay_t<decltype(binary_op(*std::begin(c1),*std::begin(c2)))>> result;
	auto it1=std::begin(c1);
	auto it2=std::begin(c2);
	while (it1!=std::end(c1) && it2!=std::end(c2)) {
		result.push_back(binary_op(*it1,*it2));
		it1++;
		it2++;
 	}
	return result;
#endif
});

#if __cplusplus>=_STDEX_CPP23_VERSION
template <template <typename...> typename _Container>
auto zip_with_as=curry([](auto binary_op,auto c1,auto c2){
	return std::views::zip_transform(binary_op,c1,c2) | std::ranges::to<_Container>();
});
#endif

inline auto concat_map=curry([](auto func,auto container){
#if __cplusplus>=_STDEX_CPP23_VERSION
	return container | std::views::transform(func) | std::views::join | std::ranges::to<std::vector<std::decay_t<decltype(*std::begin(std::declval<std::decay_t<decltype(func(*std::begin(container)))>>()))>>>();
#else
#if __cplusplus>=_STDEX_CPP20_VERSION
	static_assert(std::ranges::range<decltype(container)>,"concat_map requires a range.");
	static_assert(std::ranges::range<std::decay_t<decltype(func(*std::begin(container)))>>,"concat_map requires func to return a range.");
#endif
	std::vector<std::decay_t<decltype(*std::begin(std::declval<std::decay_t<decltype(func(*std::begin(container)))>>()))>> result;
	for (auto&& it:container) {
		auto inner=func(std::forward<decltype(it)>(it));
		for (auto&& jt:inner) result.push_back(std::move(jt));
	}
	return result;
#endif
});

#if __cplusplus>=_STDEX_CPP23_VERSION
template <template <typename...> typename _Container>
auto concat_map_as=curry([](auto func,auto container){
	return container | std::views::transform(func) | std::views::join | std::ranges::to<_Container>();
});
#endif

inline auto take=curry([](std::size_t n,auto container){
#if __cplusplus>=_STDEX_CPP23_VERSION
	return container | std::views::take(n) | std::ranges::to<std::vector<std::decay_t<decltype(*std::begin(container))>>>();
#else
#if __cplusplus>=_STDEX_CPP20_VERSION
	static_assert(std::ranges::range<decltype(container)>,"take requires a range.");
#endif
	std::vector<std::decay_t<decltype(*std::begin(container))>> result;
	result.reserve(n);
	std::size_t count=0;
	for (auto&& it:container) {
		if (count++>=n) break;
		result.push_back(std::move(it));
	}
	return result;
#endif
});

inline auto drop=curry([](std::size_t n,auto container){
#if __cplusplus>=_STDEX_CPP23_VERSION
	return container | std::views::drop(n) | std::ranges::to<std::vector<std::decay_t<decltype(*std::begin(container))>>>();
#else
#if __cplusplus>=_STDEX_CPP20_VERSION
	static_assert(std::ranges::range<decltype(container)>,"drop requires a range.");
#endif
	std::vector<std::decay_t<decltype(*std::begin(container))>> result;
	std::size_t count=0;
	for (auto&& it:container) {
		if (count++<n) continue;
		result.push_back(std::move(it));
	}
	return result;
#endif
});

inline auto take_while=curry([](auto predicate,auto container){
#if __cplusplus>=_STDEX_CPP23_VERSION
	return container | std::views::take_while(predicate) | std::ranges::to<std::vector<std::decay_t<decltype(*std::begin(container))>>>();
#else
#if __cplusplus>=_STDEX_CPP20_VERSION
	static_assert(std::ranges::range<decltype(container)>,"take_while requires a range.");
#endif
	std::vector<std::decay_t<decltype(*std::begin(container))>> result;
	for (auto&& it:container) {
		if (!predicate(it)) break;
		result.push_back(std::move(it));
	}
	return result;
#endif
});

inline auto drop_while=curry([](auto predicate,auto container){
#if __cplusplus>=_STDEX_CPP23_VERSION
	return container | std::views::drop_while(predicate) | std::ranges::to<std::vector<std::decay_t<decltype(*std::begin(container))>>>();
#else
#if __cplusplus>=_STDEX_CPP20_VERSION
	static_assert(std::ranges::range<decltype(container)>,"drop_while requires a range.");
#endif
	std::vector<std::decay_t<decltype(*std::begin(container))>> result;
	bool dropping=true;
	for (auto&& it:container) {
		if (dropping && predicate(it)) continue;
		dropping=false;
		result.push_back(std::move(it));
	}
	return result;
#endif
});

#if __cplusplus>=_STDEX_CPP23_VERSION
template <template <typename...> typename _Container>
auto take_as=curry([](std::size_t n,auto container){
	return container | std::views::take(n) | std::ranges::to<_Container>();
});

template <template <typename...> typename _Container>
auto drop_as=curry([](std::size_t n,auto container){
	return container | std::views::drop(n) | std::ranges::to<_Container>();
});

template <template <typename...> typename _Container>
auto take_while_as=curry([](auto predicate,auto container){
	return container | std::views::take_while(predicate) | std::ranges::to<_Container>();
});

template <template <typename...> typename _Container>
auto drop_while_as=curry([](auto predicate,auto container){
	return container | std::views::drop_while(predicate) | std::ranges::to<_Container>();
});
#endif

inline auto partition=curry([](auto predicate,auto container) {
	std::decay_t<decltype(container)> yes,no;
	auto yes_ins=std::inserter(yes,std::end(yes));
	auto no_ins=std::inserter(no,std::end(no));
	for (auto&& it:container) {
		if (predicate(it)) *yes_ins++=std::move(it);
		else *no_ins++=std::move(it);
	}
	return std::make_pair(std::move(yes),std::move(no));
});

inline auto any_of=curry([](auto predicate,auto container)->bool{
	return std::any_of(std::begin(container),std::end(container),predicate);
});

inline auto all_of=curry([](auto predicate,auto container)->bool{
	return std::all_of(std::begin(container), std::end(container),predicate);
});

inline auto none_of=curry([](auto predicate,auto container)->bool{
	return std::none_of(std::begin(container),std::end(container),predicate);
});

inline auto find_if=curry([](auto predicate,auto container)->std::optional<std::decay_t<decltype(*std::begin(container))>>{
	auto it=std::find_if(std::begin(container),std::end(container),predicate);
	if (it==std::end(container)) return std::nullopt;
	return *it;
});

inline auto group_by=curry([](auto key_func,auto container){
	std::vector<std::pair<std::decay_t<decltype(key_func(*std::begin(container)))>,std::vector<std::decay_t<decltype(*std::begin(container))>>>> result;
	for (auto&& it:container) {
		auto key=key_func(it);
		auto pos=std::find_if(result.begin(),result.end(),[&key](const std::pair<std::decay_t<decltype(key_func(*std::begin(container)))>,std::vector<std::decay_t<decltype(*std::begin(container))>>>& p){
			return p.first==key;
		});
		if (pos==result.end()) result.emplace_back(key,std::vector<std::decay_t<decltype(*std::begin(container))>>{it});
		else pos->second.push_back(it);
	}
	return result;
});

inline auto sort_by=curry([](auto comparator,auto container){
	auto result=std::move(container);
	std::sort(std::begin(result),std::end(result),comparator);
	return result;
});

inline auto unique_by=curry([](auto key_func,auto container){
	auto result=std::move(container);
	auto last=std::unique(std::begin(result),std::end(result),[&key_func](const auto& a,const auto& b){
		return key_func(a)==key_func(b);
	});
	result.erase(last,std::end(result));
	return result;
});

template <typename _Func>
auto flip(_Func&& func){
	return [f=std::forward<_Func>(func)](auto&& lhs,auto&& rhs){
		return f(std::forward<decltype(rhs)>(rhs),std::forward<decltype(lhs)>(lhs));
	};
}

inline auto identity=[](auto&& x)->decltype(auto){
	return std::forward<decltype(x)>(x);
};

inline auto constant=curry([](auto x,auto){
	return x;
});

template <typename _BinaryOp,typename _Func>
auto on(_BinaryOp&& binary_op,_Func&& func) {
	return [b=std::forward<_BinaryOp>(binary_op),f=std::forward<_Func>(func)](auto&& a,auto&& b2){
		return b(f(std::forward<decltype(a)>(a)),f(std::forward<decltype(b2)>(b2)));
	};
}

inline auto maybe=curry([](auto default_val,auto func,auto opt){
	if (opt.has_value()) return func(opt.value());
	return default_val;
});

inline auto fmap_opt=curry([](auto func,auto opt)->std::optional<std::decay_t<decltype(func(*opt))>>{
	if (!opt.has_value()) return std::nullopt;
	return func(*opt);
});

inline auto iterate=curry([](auto func,auto init,std::size_t n){
	std::vector<std::decay_t<decltype(init)>> result;
	result.reserve(n);
	auto cur=init;
	for (std::size_t i=0;i<n;i++) {
		result.push_back(cur);
		cur=func(std::move(cur));
	}
	return result;
});

inline auto unfold=curry([](auto func,auto seed){
	std::vector<std::decay_t<decltype(std::declval<std::decay_t<decltype(func(seed))>::value_type>().first)>> result;
	auto cur=seed;
	while (true) {
		auto opt=func(cur);
		if (!opt.has_value()) break;
		result.push_back(opt->first);
		cur=opt->second;
	}
	return result;
});

template <typename _Func>
auto memoize(_Func&& func) {
#if __cplusplus>=_STDEX_CPP20_VERSION
	auto cache=std::make_shared<std::unordered_map<std::size_t,std::vector<std::pair<std::vector<std::any>,std::any>>>>();
	return [f=std::forward<_Func>(func),cache](auto&&... args) mutable requires (std::equality_comparable<std::decay_t<decltype(args)>>&&...){
		std::size_t seed=0;
		((seed^=std::hash<std::decay_t<decltype(args)>>{}(args)+0x9e3779b9+(seed<<6)+(seed>>2)),...);
		std::vector<std::any> key={std::any(args)...};
		auto& bucket=(*cache)[seed];
		for (auto& [k,v]:bucket) {
			if (k.size()!=key.size()) continue;
			bool match=true;
			std::size_t i=0;
			([&](){
				if (!match) return;
				using arg_t=std::decay_t<decltype(args)>;
				auto* lhs=std::any_cast<arg_t>(&k[i]);
				auto* rhs=std::any_cast<arg_t>(&key[i]);
				if (!lhs||!rhs||!(*lhs==*rhs)) match=false;
				i++;
			}(),...);
			if (match) return std::any_cast<std::decay_t<decltype(f(std::forward<decltype(args)>(args)...))>>(v);
		}
		auto result=f(std::forward<decltype(args)>(args)...);
		bucket.emplace_back(std::move(key),std::any(result));
		return result;
	};
#else
	auto cache=std::make_shared<std::unordered_map<std::size_t,std::any>>();
	return [f=std::forward<_Func>(func),cache](auto&&... args) mutable{
		std::size_t seed=0;
		((seed^=std::hash<std::decay_t<decltype(args)>>{}(args)+0x9e3779b9+(seed<<6)+(seed>>2)),...);
		auto it=cache->find(seed);
		if (it!=cache->end()) return std::any_cast<std::decay_t<decltype(f(std::forward<decltype(args)>(args)...))>>(it->second);
		auto result=f(std::forward<decltype(args)>(args)...);
		(*cache)[seed]=result;
		return result;
	};
#endif
}

template <typename _Tp,typename _Func>
auto operator |(_Tp&& value,_Func&& func)->decltype(func(std::forward<_Tp>(value))){
	return func(std::forward<_Tp>(value));
}

}

}

#endif
