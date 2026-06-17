//Last Modified At 2026/06/17
//@Version 1.0.0.0
#ifndef _STDEX_CONTAINER_COMPRESSED_H_
#define _STDEX_CONTAINER_COMPRESSED_H_ 1

#include <cstddef>
#include <tuple>
#include <type_traits>
#include <utility>

namespace stdex {

namespace container {

template <typename _Tp,int _Id,bool=std::is_empty_v<_Tp> && !std::is_final_v<_Tp>>
class compressed_elem {
	_Tp value_{};

public:
	compressed_elem()=default;
	template <typename _Up,std::enable_if_t<!std::is_same_v<std::decay_t<_Up>,compressed_elem> && std::is_constructible_v<_Tp,_Up>,int> =0>
	explicit compressed_elem(_Up&& u) : value_(std::forward<_Up>(u)) { }
	template <typename _Tuple,std::size_t... _Is>
	compressed_elem(_Tuple&& t,std::index_sequence<_Is...>) : value_(std::get<_Is>(std::forward<_Tuple>(t))...) { }

	_Tp& get() noexcept { return value_; }
	const _Tp& get() const noexcept { return value_; }
};

template <typename _Tp,int _Id>
class compressed_elem<_Tp,_Id,true> : private _Tp {
public:
	compressed_elem()=default;
	template <typename _Up,std::enable_if_t<!std::is_same_v<std::decay_t<_Up>,compressed_elem> && std::is_constructible_v<_Tp,_Up>,int> =0>
	explicit compressed_elem(_Up&& u) : _Tp(std::forward<_Up>(u)) { }
	template <typename _Tuple,std::size_t... _Is>
	compressed_elem(_Tuple&& t,std::index_sequence<_Is...>) : _Tp(std::get<_Is>(std::forward<_Tuple>(t))...) { }

	_Tp& get() noexcept { return *this; }
	const _Tp& get() const noexcept { return *this; }
};

template <typename _Tp,typename _Up>
class compressed : private compressed_elem<_Tp,0>,private compressed_elem<_Up,1> {
	using first_base=compressed_elem<_Tp,0>;
	using second_base=compressed_elem<_Up,1>;

public:
	using first_type=_Tp;
	using second_type=_Up;

	compressed()=default;

	template <typename _T1,typename _U1,std::enable_if_t<std::is_constructible_v<_Tp,_T1&&> && std::is_constructible_v<_Up,_U1&&>,int> =0>
	compressed(_T1&& a,_U1&& b) : first_base(std::forward<_T1>(a)) , second_base(std::forward<_U1>(b)) { }
	template <typename _Tuple1,typename _Tuple2>
	compressed(std::piecewise_construct_t,_Tuple1&& t1,_Tuple2&& t2) : first_base(std::forward<_Tuple1>(t1),std::make_index_sequence<std::tuple_size_v<std::decay_t<_Tuple1>>>{}) , second_base(std::forward<_Tuple2>(t2),std::make_index_sequence<std::tuple_size_v<std::decay_t<_Tuple2>>>{}) { }

	_Tp& first() noexcept { return static_cast<first_base&>(*this).get(); }
	const _Tp& first() const noexcept { return static_cast<const first_base&>(*this).get(); }
	_Up& second() noexcept { return static_cast<second_base&>(*this).get(); }
	const _Up& second() const noexcept { return static_cast<const second_base&>(*this).get(); }

	void swap(compressed& other) noexcept(std::is_nothrow_swappable_v<_Tp> && std::is_nothrow_swappable_v<_Up>) {
		using std::swap;
		swap(first(),other.first());
		swap(second(),other.second());
	}

};

template <typename _Tp,typename _Up>
void swap(compressed<_Tp,_Up>& lhs,compressed<_Tp,_Up>& rhs) noexcept(noexcept(lhs.swap(rhs))) {
	lhs.swap(rhs);
}

template <typename _Tp,typename _Up>
compressed<std::decay_t<_Tp>,std::decay_t<_Up>> make_compressed(_Tp&& lhs,_Up&& rhs) {
	return compressed<std::decay_t<_Tp>,std::decay_t<_Up>>(std::forward<_Tp>(lhs),std::forward<_Up>(rhs));
}

template <std::size_t _Id,typename _Tp,typename _Up>
constexpr decltype(auto) get(compressed<_Tp,_Up>& c) noexcept {
	static_assert(_Id<2,"compressed index out of range.");
	if constexpr (_Id==0) return c.first();
	else return c.second();
}

template <std::size_t _Id,typename _Tp,typename _Up>
constexpr decltype(auto) get(const compressed<_Tp,_Up>& c) noexcept {
	static_assert(_Id<2,"compressed index out of range.");
	if constexpr (_Id==0) return c.first();
	else return c.second();
}

template <std::size_t _Id,typename _Tp,typename _Up>
constexpr decltype(auto) get(compressed<_Tp,_Up>&& c) noexcept {
	static_assert(_Id<2,"compressed index out of range.");
	if constexpr (_Id==0) return std::move(c.first());
	else return std::move(c.second());
}

}

}

namespace std {

template <typename _Tp,typename _Up>
struct tuple_size<stdex::container::compressed<_Tp,_Up>> : std::integral_constant<std::size_t,2> {};

template <typename _Tp,typename _Up>
struct tuple_element<0,stdex::container::compressed<_Tp,_Up>> {
	using type=_Tp;
};

template <typename _Tp,typename _Up>
struct tuple_element<1,stdex::container::compressed<_Tp,_Up>> {
	using type=_Up;
};

}

#endif
