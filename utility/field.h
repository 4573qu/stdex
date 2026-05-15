//Last Modified At 2026/05/15
//@Version 1.0.0.0
#ifndef _STDEX_UTILITY_FIELD_H_
#define _STDEX_UTILITY_FIELD_H_ 1

#include <cstddef>
#include <type_traits>
#include <utility>

namespace stdex {

namespace utility {

template <typename _Tp>
struct field {
	_Tp value{};
	
	constexpr field(const _Tp& value) noexcept(std::is_nothrow_copy_constructible_v<_Tp>) : value(value) { }
	constexpr field(_Tp&& value) noexcept(std::is_nothrow_move_constructible_v<_Tp>) : value(std::move(value)) { }
	
	constexpr field& operator =(const _Tp& value) noexcept(std::is_nothrow_copy_assignable_v<_Tp>) {
		this->value=value;
		return *this;
	}
	constexpr field& operator =(_Tp&& value) noexcept(std::is_nothrow_move_assignable_v<_Tp>) {
		this->value=std:move(value);
		return *this;
	}

	operator _Tp&() noexcept {
		return value;
	}
	operator const _Tp&() const noexcept {
		return value;
	}

	_Tp& operator ()() noexcept {
		return value;
	}
	const _Tp& operator ()() const noexcept {
		return value;
	}

};

}

}

#endif