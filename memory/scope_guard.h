//Last Modified At 2025/10/10
//@Version 1.0.0.0
#ifndef _STDEX_MEMORY_SCOPE_GUARD_H_
#define _STDEX_MEMORY_SCOPE_GUARD_H_ 1

#include <type_traits>
#include <utility>

namespace stdex {

namespace memory {

template <typename _Func>
class scope_guard {
	_Func func_;
	bool active_;

public:
	explicit scope_guard(_Func&& func) noexcept(std::is_nothrow_move_constructible_v<_Func>) : func_(std::move(func)) , active_(true) { }
	scope_guard(scope_guard&& other) noexcept(std::is_nothrow_move_constructible_v<_Func>) : func_(std::move(other.func_)) , active_(other.active_) {
		other.active_=false;
	}
	scope_guard(const scope_guard&)=delete;
	scope_guard& operator =(const scope_guard&)=delete;
	scope_guard& operator =(scope_guard&&)=delete;
	~scope_guard() {
		if (active_) {
			try {
				func_();
			} catch (...) {
				//destructor shouldn't catch any exception
			}
		}
	}
	void release() noexcept { active_=false; }
};

template <typename _Func>
scope_guard(_Func&&)->scope_guard<std::decay_t<_Func>>;

template <typename _Func>
[[nodiscard]]
auto make_scope_guard(_Func&& func) noexcept(std::is_nothrow_constructible_v<std::decay_t<_Func>,_Func&&>) {
	return scope_guard<std::decay_t<_Func>>(std::forward<_Func>(func));
}

}

}

#endif