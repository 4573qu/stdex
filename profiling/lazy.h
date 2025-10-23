//Last Modified At 2025/10/11
//@Version 1.0.0.0
#ifndef _STDEX_PROFILING_LAZY_H_
#define _STDEX_PROFILING_LAZY_H_ 1

#include <functional>
#include <optional>
#include <type_traits>
#include <utility>

namespace stdex {

namespace profiling {

template <typename _Func>
class lazy {
	static_assert(std::is_invocable_v<_Func>,"_Func must be invocable without arguments");
	mutable std::optional<value_type> value_;
	mutable _Func func_;

public:
	lazy()=delete;
	explicit lazy(_Func&& func) : func_(std::move(func)) { }
	explicit lazy(const _Func& func) : func_(func) { }
	template<typename... _Args>
	lazy(std::in_place_t,_Args&&... args) : value_(std::forward<_Args>(args)...) { }

	std::invoke_result_t<_Func>& value() {
		if (!value_.has_value()) value_=func_();
		return *value_;
	}
	const std::invoke_result_t<_Func>& value() const {
		if (!value_.has_value()) value_=func_();
		return *value_;
	}
	
	bool has_value() const noexcept { return value_.has_value(); }
	void reset() noexcept { value_.reset(); }

	std::invoke_result_t<_Func>& operator *() { return value(); }
	const std::invoke_result_t<_Func>& operator *() const { return value(); }
	std::invoke_result_t<_Func>* operator ->() { return &value(); }
	const std::invoke_result_t<_Func>* operator ->() const { return &value(); }
	
	explicit operator bool() const noexcept { return has_value(); }
};

template <typename _Func>
lazy(_Func) -> lazy<_Func>;

template <typename _Func>
auto make_lazy(_Func&& func) {
	return lazy<std::decay_t<_Func>>(std::forward<_Func>(func));
}

}

}

#endif