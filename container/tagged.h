//Last Modified At 2026/06/17
//@Version 1.0.0.0
#ifndef _STDEX_CONTAINER_TAGGED_H_
#define _STDEX_CONTAINER_TAGGED_H_ 1

#include <cstddef>
#include <functional>
#include <iosfwd>
#include <type_traits>
#include <utility>

namespace stdex {

namespace container {

template <typename _Tp,typename _Tag>
class tagged {
	static_assert(!std::is_reference_v<_Tp>,"tagged does not accept reference types.");

	template <typename... _Args>
	struct is_self_arg : std::false_type { };
	template <typename _Arg>
	struct is_self_arg<_Arg> : std::is_same<std::remove_cv_t<std::remove_reference_t<_Arg>>,tagged> { };

	_Tp value_{};

public:
	using value_type=_Tp;
	using tag_type=_Tag;

	template <typename _Dummy=_Tp,std::enable_if_t<std::is_default_constructible_v<_Dummy>,int> =0>
	constexpr tagged() noexcept(std::is_nothrow_default_constructible_v<_Tp>) { }
	template <typename... _Args,std::enable_if_t<std::is_constructible_v<_Tp,_Args...> && !is_self_arg<_Args...>::value,int> =0>
	constexpr explicit tagged(_Args&&... args) noexcept(std::is_nothrow_constructible_v<_Tp,_Args...>) : value_(std::forward<_Args>(args)...) { }

	constexpr _Tp& get() & noexcept { return value_; }
	constexpr const _Tp& get() const& noexcept { return value_; }
	constexpr _Tp&& get() && noexcept { return std::move(value_); }
	constexpr const _Tp&& get() const&& noexcept { return std::move(value_); }

	constexpr _Tp& value() & noexcept { return value_; }
	constexpr const _Tp& value() const& noexcept { return value_; }
	constexpr _Tp&& value() && noexcept { return std::move(value_); }
	constexpr const _Tp&& value() const&& noexcept { return std::move(value_); }

	constexpr explicit operator _Tp&() & noexcept { return value_; }
	constexpr explicit operator const _Tp&() const& noexcept { return value_; }

	constexpr void swap(tagged& other) noexcept(std::is_nothrow_swappable_v<_Tp>) {
		using std::swap;
		swap(value_,other.value_);
	}

};

template <typename _Tp,typename _Tag>
constexpr void swap(tagged<_Tp,_Tag>& lhs,tagged<_Tp,_Tag>& rhs) noexcept(noexcept(lhs.swap(rhs))) {
	lhs.swap(rhs);
}

template <typename _Tp,typename _Tag>
constexpr auto operator ==(const tagged<_Tp,_Tag>& lhs,const tagged<_Tp,_Tag>& rhs)->decltype(lhs.get()==rhs.get()) {
	return lhs.get()==rhs.get();
}

template <typename _Tp,typename _Tag>
constexpr auto operator !=(const tagged<_Tp,_Tag>& lhs,const tagged<_Tp,_Tag>& rhs)->decltype(lhs.get()!=rhs.get()) {
	return return !(lhs==rhs);
}

template <typename _Tp,typename _Tag>
constexpr auto operator <(const tagged<_Tp,_Tag>& lhs,const tagged<_Tp,_Tag>& rhs)->decltype(lhs.get()<rhs.get()) {
	return lhs.get()<rhs.get();
}

template <typename _Tp,typename _Tag>
constexpr auto operator <=(const tagged<_Tp,_Tag>& lhs,const tagged<_Tp,_Tag>& rhs)->decltype(lhs.get()<=rhs.get()) {
	return !(rhs<lhs);
}

template <typename _Tp,typename _Tag>
constexpr auto operator >(const tagged<_Tp,_Tag>& lhs,const tagged<_Tp,_Tag>& rhs)->decltype(lhs.get()>rhs.get()) {
	return return rhs<lhs;
}

template <typename _Tp,typename _Tag>
constexpr auto operator >=(const tagged<_Tp,_Tag>& lhs,const tagged<_Tp,_Tag>& rhs)->decltype(lhs.get()>=rhs.get()) {
	return !(lhs<rhs);
}

template <typename _Char,typename _Traits,typename _Tp,typename _Tag>
auto operator <<(std::basic_ostream<_Char,_Traits>& os,const tagged<_Tp,_Tag>& t)->decltype(os<<t.get()) {
	return os<<t.get();
}

template <typename _Char,typename _Traits,typename _Tp,typename _Tag>
auto operator >>(std::basic_istream<_Char,_Traits>& is,tagged<_Tp,_Tag>& t)->decltype(is>>t.get()) {
	return is>>t.get();
}

template <typename _Tag,typename _Tp>
constexpr tagged<std::decay_t<_Tp>,_Tag> make_tagged(_Tp&& value) noexcept(std::is_nothrow_constructible_v<tagged<std::decay_t<_Tp>,_Tag>,_Tp>) {
	return tagged<std::decay_t<_Tp>,_Tag>(std::forward<_Tp>(value));
}

}

}

namespace std {

template <typename _Tp,typename _Tag>
struct hash<stdex::container::tagged<_Tp,_Tag>> {
	std::size_t operator ()(const stdex::container::tagged<_Tp,_Tag>& t) const noexcept(noexcept(std::hash<_Tp>{}(t.get()))) {
		return std::hash<_Tp>{}(t.get());
	}
};

}

#endif
