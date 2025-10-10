//Last Modified At 2025/10/11
//@Version 1.0.0.0
#ifndef _STDEX_MEMORY_OBSERVER_PTR_H_
#define _STDEX_MEMORY_OBSERVER_PTR_H_ 1

#include <memory>
#include <type_traits>

namespace stdex {

namespace memory {

template <typename _Tp>
class observer_ptr {
	_Tp* ptr_=nullptr;

public:
	constexpr observer_ptr() noexcept=default;
	constexpr observer_ptr(std::nullptr_t) noexcept { }
	explicit constexpr observer_ptr(_Tp* p) noexcept : ptr_(p) { }

	template <typename _Up,typename=std::enable_if_t<std::is_convertible_v<_Up*,_Tp*>>>
	observer_ptr(const observer_ptr<_Up>& other) noexcept : ptr_(other.get()) { }
	observer_ptr& operator =(std::nullptr_t) noexcept {
		ptr_=nullptr;
		return *this;
	}
	observer_ptr& operator =(_Tp* p) noexcept {
		ptr_=p;
		return *this;
	}
	template <typename _Up>
	std::enable_if_t<std::is_convertible_v<_Up*,_Tp*>,observer_ptr&>
	operator =(const observer_ptr<_Up>& other) noexcept {
		ptr_=other.get();
		return *this;
	}

	constexpr _Tp* get() const noexcept { return ptr_; }
	constexpr _Tp& operator *() const noexcept { return *ptr_; }
	constexpr _Tp* operator ->() const noexcept { return ptr_; }
	constexpr explicit operator bool() const noexcept { return ptr_!=nullptr; }
	constexpr explicit operator _Tp*() const noexcept { return ptr_; }

	constexpr _Tp* release() noexcept {
		_Tp* p=ptr_;
		ptr_=nullptr;
		return p;
	}
	constexpr void reset(_Tp* p=nullptr) noexcept { ptr_=p; }
	constexpr void swap(observer_ptr& other) noexcept {
		std::swap(ptr_,other.ptr_);
	}
	
};

template <typename _Tp,typename _Up>
constexpr bool operator ==(const observer_ptr<_Tp>& p1,const observer_ptr<_Up>& p2) {
	return p1.get()==p2.get();
}
template <typename _Tp,typename _Up>
constexpr bool operator !=(const observer_ptr<_Tp>& p1,const observer_ptr<_Up>& p2) {
	return p1.get()!=p2.get();
}
template <typename _Tp>
constexpr bool operator ==(const observer_ptr<_Tp>& p,std::nullptr_t) noexcept {
	return p.get()==nullptr;
}
template <typename _Tp>
constexpr bool operator ==(std::nullptr_t,const observer_ptr<_Tp>& p) noexcept {
	return p.get()==nullptr;
}
template <typename _Tp>
constexpr bool operator !=(const observer_ptr<_Tp>& p,std::nullptr_t) noexcept {
	return p.get()!=nullptr;
}
template <typename _Tp>
constexpr bool operator !=(std::nullptr_t,const observer_ptr<_Tp>& p) noexcept {
	return p.get()!=nullptr;
}

template <typename _Tp>
constexpr void swap(observer_ptr<_Tp>& p1,observer_ptr<_Tp>& p2) noexcept {
	p1.swap(p2);
}
template <typename _Tp>
constexpr observer_ptr<_Tp> make_observer(_Tp* p) noexcept {
	return observer_ptr<_Tp>(p);
}

}

}

#endif