//Last Modified At 2026/06/17
//@Version 1.0.0.0
#ifndef _STDEX_CONTAINER_NOT_NULL_H_
#define _STDEX_CONTAINER_NOT_NULL_H_ 1

#include <cstddef>
#include <functional>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace stdex {

namespace container {

template <typename _Tp>
class not_null {
	static_assert(std::is_convertible_v<decltype(std::declval<const _Tp&>()==nullptr),bool>,"not_null requires a type that is comparable with nullptr.");

	_Tp ptr_;

	static constexpr bool by_value_=sizeof(_Tp)<=2*sizeof(void*) && std::is_trivially_copyable_v<_Tp>;
	using return_t=std::conditional_t<by_value_,_Tp,const _Tp&>;

	template <typename _Up>
	friend class not_null;

public:
	using element_type=_Tp;

	template <typename _Up,std::enable_if_t<std::is_convertible_v<_Up,_Tp>,int> =0>
	constexpr not_null(_Up&& u) : ptr_(std::forward<_Up>(u)) {
		if (ptr_==nullptr) throw std::invalid_argument("not_null cannot be constructed from a null pointer");
	}
	template <typename _Up,std::enable_if_t<std::is_convertible_v<_Up,_Tp>,int> =0>
	constexpr not_null(const not_null<_Up>& other) : not_null(other.get()) { }

	not_null(const not_null&)=default;
	not_null(std::nullptr_t)=delete;

	not_null& operator =(const not_null&)=default;
	not_null& operator =(std::nullptr_t)=delete;

	template <typename _Up,std::enable_if_t<std::is_convertible_v<_Up,_Tp>,int> =0>
	constexpr not_null& operator =(const not_null<_Up>& other) {
		ptr_=other.get();
		return *this;
	}

	constexpr return_t get() const noexcept { return ptr_; }
	constexpr operator return_t() const noexcept { return get(); }
	constexpr decltype(auto) operator ->() const { return get(); }
	constexpr decltype(auto) operator *() const { return *get(); }

	not_null& operator ++()=delete;
	not_null& operator --()=delete;
	not_null operator ++(int)=delete;
	not_null operator --(int)=delete;
	not_null& operator +=(std::ptrdiff_t)=delete;
	not_null& operator -=(std::ptrdiff_t)=delete;
	void operator [](std::ptrdiff_t) const=delete;

};

template <typename _Tp>
not_null(_Tp)->not_null<_Tp>;

template <typename _Tp>
not_null<std::decay_t<_Tp>> make_not_null(_Tp&& t) {
	return not_null<std::decay_t<_Tp>>(std::forward<_Tp>(t));
}

}

}

#endif
