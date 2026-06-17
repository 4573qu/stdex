//Last Modified At 2026/06/17
//@Version 1.0.0.0
#ifndef _STDEX_CONTAINER_POLYMORPHIC_H_
#define _STDEX_CONTAINER_POLYMORPHIC_H_ 1

#include <type_traits>
#include <utility>

namespace stdex {

namespace container {

template <typename _Tp>
class polymorphic {
	static_assert(std::is_class_v<_Tp>,"polymorphic requires a class type.");

	_Tp* ptr_=nullptr;
	_Tp* (*clone_)(const _Tp*)=nullptr;
	void (*delete_)(_Tp*) noexcept=nullptr;

	template <typename _Up>
	void bind_ops() noexcept {
		clone_=[](const _Tp* p)->_Tp*{ return new _Up(*static_cast<const _Up*>(p)); };
		delete_=[](_Tp* p) noexcept{ delete static_cast<_Up*>(p); };
	}
	void destroy() noexcept {
		if (ptr_) delete_(ptr_);
	}

	template <typename _Up>
	using stored_t=std::remove_cv_t<std::remove_reference_t<_Up>>;
	template <typename _Up>
	using enable_value=std::enable_if_t<!std::is_same_v<stored_t<_Up>,polymorphic> && std::is_convertible_v<stored_t<_Up>*,_Tp*> && std::is_copy_constructible_v<stored_t<_Up>>,int>;

public:
	using element_type=_Tp;

	template <typename _Dummy=_Tp,std::enable_if_t<std::is_default_constructible_v<_Dummy> && !std::is_abstract_v<_Dummy>,int> =0>
	polymorphic() : ptr_(new _Tp()) {
		bind_ops<_Tp>();
	}
	template <typename _Up,enable_value<_Up> =0>
	polymorphic(_Up&& value) : ptr_(new stored_t<_Up>(std::forward<_Up>(value))) {
		bind_ops<stored_t<_Up>>();
	}
	template <typename _Up,typename... _Args,std::enable_if_t<std::is_convertible_v<_Up*,_Tp*> && std::is_constructible_v<_Up,_Args...> && std::is_copy_constructible_v<_Up>,int> =0>
	explicit polymorphic(std::in_place_type_t<_Up>,_Args&&... args) : ptr_(new _Up(std::forward<_Args>(args)...)) {
		bind_ops<_Up>();
	}
	~polymorphic() {
		destroy();
	}

	polymorphic(const polymorphic& other) : clone_(other.clone_) , delete_(other.delete_) {
		if (other.ptr_) ptr_=other.clone_(other.ptr_);
	}
	polymorphic(polymorphic&& other) noexcept : ptr_(other.ptr_) , clone_(other.clone_) , delete_(other.delete_) {
		other.ptr_=nullptr;
	}

	polymorphic& operator =(const polymorphic& other) {
		polymorphic tmp(other);
		swap(tmp);
		return *this;
	}
	polymorphic& operator =(polymorphic&& other) noexcept {
		if (this!=&other) {
			destroy();
			ptr_=other.ptr_;
			clone_=other.clone_;
			delete_=other.delete_;
			other.ptr_=nullptr;
		}
		return *this;
	}



	_Tp& operator *() & noexcept { return *ptr_; }
	const _Tp& operator *() const& noexcept { return *ptr_; }
	_Tp&& operator *() && noexcept { return std::move(*ptr_); }
	const _Tp&& operator *() const&& noexcept { return std::move(*ptr_); }
	_Tp* operator ->() noexcept { return ptr_; }
	const _Tp* operator ->() const noexcept { return ptr_; }

	bool valueless_after_move() const noexcept { return ptr_==nullptr; }
	explicit operator bool() const noexcept { return ptr_!=nullptr; }

	void swap(polymorphic& other) noexcept {
		std::swap(ptr_,other.ptr_);
		std::swap(clone_,other.clone_);
		std::swap(delete_,other.delete_);
	}

};

template <typename _Tp>
void swap(polymorphic<_Tp>& lhs,polymorphic<_Tp>& rhs) noexcept {
	lhs.swap(rhs);
}

template <typename _Tp,typename... _Args>
polymorphic<_Tp> make_polymorphic(_Args&&... args) {
	return polymorphic<_Tp>(std::in_place_type<_Tp>,std::forward<_Args>(args)...);
}

}

}

#endif
