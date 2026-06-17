//Last Modified At 2026/06/17
//@Version 1.0.0.0
#ifndef _STDEX_CONTAINER_PROPAGATE_CONST_H_
#define _STDEX_CONTAINER_PROPAGATE_CONST_H_ 1

#include <cstddef>
#include <functional>
#include <type_traits>
#include <utility>

namespace stdex {

namespace container {

template <typename _Tp>
class propagate_const;

template <typename>
struct is_propagate_const : std::false_type {};
template <typename _Tp>
struct is_propagate_const<propagate_const<_Tp>> : std::true_type {};

template <typename _Tp>
class propagate_const {
	template <typename _Up>
	friend class propagate_const;

	template <typename _Up>
	friend constexpr const _Up& get_underlying(const propagate_const<_Up>& pt) noexcept;
	template <typename _Up>
	friend constexpr _Up& get_underlying(propagate_const<_Up>& pt) noexcept;

	template <typename _Up>
	static constexpr auto to_raw(_Up* p) noexcept { return p; }
	template <typename _Up>
	static constexpr auto to_raw(_Up& p) { return to_raw(p.get()); }

	_Tp t_{};

public:
	using element_type=std::remove_reference_t<decltype(*std::declval<_Tp&>())>;

	constexpr propagate_const()=default;
	template <typename _Up,std::enable_if_t<std::is_constructible_v<_Tp,_Up&&> && std::is_convertible_v<_Up&&,_Tp>,int> =0>
	constexpr propagate_const(propagate_const<_Up>&& pu) : t_(std::move(pu.t_)) { }
	template <typename _Up,std::enable_if_t<std::is_constructible_v<_Tp,_Up&&> && !std::is_convertible_v<_Up&&,_Tp>,int> =0>
	explicit constexpr propagate_const(propagate_const<_Up>&& pu) : t_(std::move(pu.t_)) { }
	template <typename _Up,std::enable_if_t<!is_propagate_const<std::decay_t<_Up>>::value && std::is_constructible_v<_Tp,_Up&&> && std::is_convertible_v<_Up&&,_Tp>,int> =0>
	constexpr propagate_const(_Up&& u) : t_(std::forward<_Up>(u)) { }
	template <typename _Up,std::enable_if_t<!is_propagate_const<std::decay_t<_Up>>::value && std::is_constructible_v<_Tp,_Up&&> && !std::is_convertible_v<_Up&&,_Tp>,int> =0>
	explicit constexpr propagate_const(_Up&& u) : t_(std::forward<_Up>(u)) { }

	propagate_const(const propagate_const&)=delete;
	constexpr propagate_const(propagate_const&&)=default;

	propagate_const& operator =(const propagate_const&)=delete;
	constexpr propagate_const& operator =(propagate_const&&)=default;
	template <typename _Up,std::enable_if_t<std::is_convertible_v<_Up&&,_Tp>,int> =0>
	constexpr propagate_const& operator =(propagate_const<_Up>&& pu) {
		t_=std::move(pu.t_);
		return *this;
	}
	template <typename _Up,std::enable_if_t<!is_propagate_const<std::decay_t<_Up>>::value && std::is_convertible_v<_Up&&,_Tp>,int> =0>
	constexpr propagate_const& operator =(_Up&& u) {
		t_=std::forward<_Up>(u);
		return *this;
	}

	constexpr void swap(propagate_const& pt) noexcept(std::is_nothrow_swappable_v<_Tp>) {
		using std::swap;
		swap(t_,pt.t_);
	}

	constexpr element_type* get() { return to_raw(t_); }
	constexpr const element_type* get() const { return to_raw(t_); }

	constexpr explicit operator bool() const { return get()!=nullptr; }
	constexpr element_type& operator *() { return *get(); }
	constexpr const element_type& operator *() const { return *get(); }
	constexpr element_type* operator ->() { return get(); }
	constexpr const element_type* operator ->() const { return get(); }

	template <typename _Up=_Tp,std::enable_if_t<std::is_pointer_v<_Up> || std::is_convertible_v<_Up,element_type*>,int> =0>
	constexpr operator element_type*() { return get(); }
	template <typename _Up=_Tp,std::enable_if_t<std::is_pointer_v<_Up> || std::is_convertible_v<const _Up,const element_type*>,int> =0>
	constexpr operator const element_type*() const { return get(); }

};

template <typename _Tp>
constexpr const _Tp& get_underlying(const propagate_const<_Tp>& pt) noexcept {
	return pt.t_;
}
template <typename _Tp>
constexpr _Tp& get_underlying(propagate_const<_Tp>& pt) noexcept {
	return pt.t_;
}

template <typename _Tp>
constexpr void swap(propagate_const<_Tp>& lhs,propagate_const<_Tp>& rhs) noexcept(std::is_nothrow_swappable_v<_Tp>) {
	lhs.swap(rhs);
}

template <typename _Tp>
constexpr bool operator ==(const propagate_const<_Tp>& p,std::nullptr_t) {
	return get_underlying(p)==nullptr;
}

template <typename _Tp>
constexpr bool operator ==(std::nullptr_t,const propagate_const<_Tp>& p) {
	return p==nullptr;
}

template <typename _Tp>
constexpr bool operator !=(const propagate_const<_Tp>& p,std::nullptr_t) {
	return !(p==nullptr);
}

template <typename _Tp>
constexpr bool operator !=(std::nullptr_t,const propagate_const<_Tp>& p) {
	return !(p==nullptr);
}

template <typename _Tp,typename _Up>
constexpr bool operator ==(const propagate_const<_Tp>& lhs,const propagate_const<_Up>& rhs) {
	return get_underlying(lhs)==get_underlying(rhs);
}

template <typename _Tp,typename _Up>
constexpr bool operator !=(const propagate_const<_Tp>& lhs,const propagate_const<_Up>& rhs) {
	return !(lhs==rhs);
}

template <typename _Tp,typename _Up>
constexpr bool operator <(const propagate_const<_Tp>& lhs,const propagate_const<_Up>& rhs) {
	return get_underlying(lhs)<get_underlying(rhs);
}

template <typename _Tp,typename _Up>
constexpr bool operator <=(const propagate_const<_Tp>& lhs,const propagate_const<_Up>& rhs) {
	return !(rhs<lhs);
}

template <typename _Tp,typename _Up>
constexpr bool operator >(const propagate_const<_Tp>& lhs,const propagate_const<_Up>& rhs) {
	return rhs<lhs;
}

template <typename _Tp,typename _Up>
constexpr bool operator >=(const propagate_const<_Tp>& lhs,const propagate_const<_Up>& rhs) {
	return !(lhs<rhs);
}

}

}

namespace std {

template <typename _Tp>
struct hash<stdex::container::propagate_const<_Tp>> {
	std::size_t operator ()(const stdex::container::propagate_const<_Tp>& p) const
		noexcept(noexcept(std::hash<_Tp>{}(stdex::container::get_underlying(p)))) {
		return std::hash<_Tp>{}(stdex::container::get_underlying(p));
	}
};

template <typename _Tp>
struct equal_to<stdex::container::propagate_const<_Tp>> {
	constexpr bool operator ()(const stdex::container::propagate_const<_Tp>& lhs,const stdex::container::propagate_const<_Tp>& rhs) const {
		return std::equal_to<_Tp>{}(stdex::container::get_underlying(lhs),stdex::container::get_underlying(rhs));
	}
};

template <typename _Tp>
struct less<stdex::container::propagate_const<_Tp>> {
	constexpr bool operator ()(const stdex::container::propagate_const<_Tp>& lhs,const stdex::container::propagate_const<_Tp>& rhs) const {
		return std::less<_Tp>{}(stdex::container::get_underlying(lhs),stdex::container::get_underlying(rhs));
	}
};

template <typename _Tp>
struct greater<stdex::container::propagate_const<_Tp>> {
	constexpr bool operator ()(const stdex::container::propagate_const<_Tp>& lhs,const stdex::container::propagate_const<_Tp>& rhs) const {
		return std::greater<_Tp>{}(stdex::container::get_underlying(lhs),stdex::container::get_underlying(rhs));
	}
};

}

#endif
