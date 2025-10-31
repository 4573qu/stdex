//Last Modified At 2025/10/31
//@Version 1.0.0.0
#ifndef _STDEX_CONTAINER_REFERNECE_H_
#define _STDEX_CONTAINER_REFERENCE_H_ 1

#include <initializer_list>

namespace stdex {

namespace container {

template <typename _Tp>
class reference {
private:
	mutable _Tp value_=nullptr;
	_Tp const* value_ref_=nullptr;

public:
	reference(_Tp&& value) : value_(std::move(value)) { }
	reference(const _Tp& value) : value_ref_(&value) {}
	reference(std::initializer_list<reference> init_list) : value_(init_list) {}
	template <class... _Args,std::enable_if_t<std::is_constructible<_Tp,_Args...>::value,int> = 0>
	reference(_Args&&... args) : value_(std::forward<_Args>(args)...) { }
	reference(reference&&) noexcept=default;
	reference(const reference&)=delete;
	reference& operator =(const reference&)=delete;
	reference& operator =(reference&&)=delete;
	~reference()=default;
	_Tp moved_or_copied() const {
		if (!value_ref_) return std::move(value_);
		return *value_ref_;
	}
	_Tp const& operator *() const {
		return value_ref_?*value_ref_:value_;
	}
	reference const* operator ->() const {
		return &**this;
	}
};

template <typename _Tp>
using ref=reference<_Tp>;

}

}

#endif