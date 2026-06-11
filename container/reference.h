//Last Modified At 2026/06/10
//@Version 1.1.0.0
#ifndef _STDEX_CONTAINER_REFERENCE_H_
#define _STDEX_CONTAINER_REFERENCE_H_ 1

#include <initializer_list>
#include <type_traits>
#include <utility>

namespace stdex {

namespace container {

template <typename _Tp>
class reference {
	static_assert(!std::is_reference<_Tp>::value,"reference does not accept reference types.");

	template <typename... _Args>
	struct is_self_arg : std::false_type { };
	template <typename _Arg>
	struct is_self_arg<_Arg> : std::integral_constant<bool,std::is_same<typename std::remove_cv<typename std::remove_reference<_Arg>::type>::type,reference>::value || std::is_same<typename std::remove_cv<typename std::remove_reference<_Arg>::type>::type,_Tp>::value> { };

	mutable _Tp value_{};
	_Tp const* value_ref_=nullptr;

public:
	using value_type=_Tp;

	reference(_Tp&& value) : value_(std::move(value)) { }
	reference(const _Tp& value) : value_ref_(&value) { }
	reference(std::initializer_list<reference> init_list) : value_(init_list) { }
	template <typename... _Args,std::enable_if_t<std::is_constructible<_Tp,_Args...>::value && !is_self_arg<_Args...>::value,int> =0>
	reference(_Args&&... args) : value_(std::forward<_Args>(args)...) { }
	~reference()=default;

	reference(const reference&)=delete;
	reference(reference&&) noexcept(std::is_nothrow_move_constructible<_Tp>::value)=default;
	reference& operator =(const reference&)=delete;
	reference& operator =(reference&&)=delete;
	
	bool holds_value() const noexcept {
		return !value_ref_;
	}

	_Tp moved_or_copied() const {
		if (!value_ref_) return std::move(value_);
		return *value_ref_;
	}
	_Tp const& operator *() const {
		return value_ref_?*value_ref_:value_;
	}
	_Tp const* operator ->() const {
		return &**this;
	}
};

template <typename _Tp>
using ref=reference<_Tp>;

}

}

#endif