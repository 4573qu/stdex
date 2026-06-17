//Last Modified At 2026/06/17
//@Version 1.0.0.0
#ifndef _STDEX_CONTAINER_COPY_ON_WRITE_H_
#define _STDEX_CONTAINER_COPY_ON_WRITE_H_ 1

#include <cstddef>
#include <memory>
#include <type_traits>
#include <utility>

namespace stdex {

namespace container {

template <typename _Tp>
class copy_on_write {
	static_assert(std::is_copy_constructible_v<_Tp>,"copy_on_write requires a copy constructible type.");

	template <typename... _Args>
	struct is_self_arg : std::false_type { };
	template <typename _Arg>
	struct is_self_arg<_Arg> : std::is_same<std::remove_cv_t<std::remove_reference_t<_Arg>>,copy_on_write> { };

	std::shared_ptr<_Tp> ptr_;

	void detach() {
		if (ptr_ && ptr_.use_count()>1) ptr_=std::make_shared<_Tp>(*ptr_);
	}

public:
	using element_type=_Tp;

	template <typename _Dummy=_Tp,std::enable_if_t<std::is_default_constructible_v<_Dummy>,int> =0>
	copy_on_write() : ptr_(std::make_shared<_Tp>()) { }
	copy_on_write(const _Tp& value) : ptr_(std::make_shared<_Tp>(value)) { }
	copy_on_write(_Tp&& value) : ptr_(std::make_shared<_Tp>(std::move(value))) { }
	template <typename... _Args,std::enable_if_t<std::is_constructible_v<_Tp,_Args...> && !is_self_arg<_Args...>::value,int> =0>
	explicit copy_on_write(std::in_place_t,_Args&&... args) : ptr_(std::make_shared<_Tp>(std::forward<_Args>(args)...)) { }

	copy_on_write(const copy_on_write&) noexcept=default;
	copy_on_write(copy_on_write&&) noexcept=default;

	copy_on_write& operator =(const copy_on_write&) noexcept=default;
	copy_on_write& operator =(copy_on_write&&) noexcept=default;

	~copy_on_write()=default;

	const _Tp& read() const noexcept { return *ptr_; }
	const _Tp& operator *() const noexcept { return *ptr_; }
	const _Tp* operator ->() const noexcept { return ptr_.get(); }
	const _Tp* get() const noexcept { return ptr_.get(); }

	_Tp& write() {
		detach();
		return *ptr_;
	}

	long use_count() const noexcept { return ptr_?ptr_.use_count():0; }
	bool unique() const noexcept { return use_count()==1; }
	bool is_shared() const noexcept { return use_count()>1; }
	explicit operator bool() const noexcept { return static_cast<bool>(ptr_); }

	void swap(copy_on_write& other) noexcept {
		ptr_.swap(other.ptr_);
	}

};

template <typename _Tp>
void swap(copy_on_write<_Tp>& lhs,copy_on_write<_Tp>& rhs) noexcept {
	lhs.swap(rhs);
}

template <typename _Tp,typename _Up>
auto operator ==(const copy_on_write<_Tp>& lhs,const copy_on_write<_Up>& rhs)->decltype(lhs.read()==rhs.read()) {
	if (lhs.get() == rhs.get()) return true;
	return lhs.read()==rhs.read();
}

template <typename _Tp,typename _Up>
auto operator !=(const copy_on_write<_Tp>& lhs,const copy_on_write<_Up>& rhs)->decltype(lhs.read()!=rhs.read()) {
	return !(lhs==rhs);
}

template <typename _Tp,typename _Up>
auto operator <(const copy_on_write<_Tp>& lhs,const copy_on_write<_Up>& rhs)->decltype(lhs.read()<rhs.read()) {
	if (lhs.get()==rhs.get()) return false;
	return lhs.read()<rhs.read();
}

template <typename _Tp,typename _Up>
auto operator <=(const copy_on_write<_Tp>& lhs,const copy_on_write<_Up>& rhs)->decltype(lhs.read()<=rhs.read()) {
	return !(rhs<lhs);
}

template <typename _Tp,typename _Up>
auto operator >(const copy_on_write<_Tp>& lhs,const copy_on_write<_Up>& rhs)->decltype(lhs.read()>rhs.read()) {
	return rhs<lhs;
}

template <typename _Tp,typename _Up>
auto operator >=(const copy_on_write<_Tp>& lhs,const copy_on_write<_Up>& rhs)->decltype(lhs.read()>=rhs.read()) {
	return !(lhs<rhs);
}

template <typename _Tp,typename... _Args>
copy_on_write<_Tp> make_copy_on_write(_Args&&... args) {
	return copy_on_write<_Tp>(std::in_place,std::forward<_Args>(args)...);
}

}

}

#endif
