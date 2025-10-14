//Last Modified At 2025/10/11
//@Version 1.0.0.0
#ifndef _STDEX_UTILITY_MATCH_H_
#define _STDEX_UTILITY_MATCH_H_ 1

#include <cstddef>
#include <tuple>
#include <type_traits>
#include <utility>

namespace stdex {

namespace utility

template <typename... _Args>
class auto_argument_binder {
	std::tuple<_Args&...> args_;

public:
	explicit constexpr auto_argument_binder(_Args&... args) noexcept : args_(args...) {}

	auto_argument_binder(const auto_argument_binder&)=delete;
	auto_argument_binder(auto_argument_binder&&)=delete;
	auto_argument_binder& operator =(const auto_argument_binder&)=delete;
	auto_argument_binder& operator =(auto_argument_binder&&)=delete;

	template <std::size_t _N>
	constexpr auto& get() & noexcept {
		return std::get<_N>(args_);
	}
	template <std::size_t _N>
	constexpr const auto& get() const& noexcept {
		return std::get<_N>(args_);
	}
	template <std::size_t _N>
	constexpr auto&& get() && noexcept {
		return std::get<_N>(std::move(args_));
	}
	template <std::size_t _N>
	constexpr operator std::tuple_element_t<_N,std::tuple<_Args...>>&() & noexcept {
		return std::get<_N>(args_);
	}
	template <std::size_t _N>
	constexpr operator const std::tuple_element_t<_N,std::tuple<_Args...>>&() const& noexcept {
		return std::get<_N>(args_);
	}
	template <std::size_t _N>
	constexpr operator std::tuple_element_t<_N,std::tuple<_Args...>>() && noexcept {
		return std::get<_N>(std::move(args_));
	}
	template <std::size_t _N,typename _Tp>
	constexpr auto& operator =(_Tp&& value) & {
		std::get<_N>(args_)=std::forward<_Tp>(value);
		return *this;
	}
	template <std::size_t _N,typename _Tp>
	constexpr auto& operator =(_Tp&& value) && {
		std::get<_N>(args_)=std::forward<_Tp>(value);
		return *this;
	}
};

template <std::size_t _N,typename _Binder>
class placeholder_proxy {
	_Binder& binder_;

public:
	explicit constexpr placeholder_proxy(_Binder& binder) noexcept : binder_(binder) {}

	template <typename _Tp>
	constexpr auto& operator =(_Tp&& value) & {
		binder_.template operator =<_N>(std::forward<_Tp>(value));
		return *this;
	}
	template <typename _Tp>
	constexpr auto& operator =(_Tp&& value) && {
		binder_.template operator =<_N>(std::forward<_Tp>(value));
		return *this;
	}
	constexpr auto& operator ()() & {
		return binder_.template get<_N>();
	}
	constexpr const auto& operator ()() const& {
		return binder_.template get<_N>();
	}
	constexpr auto&& operator ()() && {
		return binder_.template get<_N>();
	}

	constexpr operator std::tuple_element_t<_N,typename _Binder::tuple_type>&() & {
		return binder_.template get<_N>();
	}
	constexpr operator const std::tuple_element_t<_N,typename _Binder::tuple_type>&() const& {
		return binder_.template get<_N>();
	}
	constexpr operator std::tuple_element_t<_N,typename _Binder::tuple_type>() && {
		return binder_.template get<_N>();
	}
};

template <typename... _Args>
inline auto_argument_binder<_Args...>* g_current_binder=nullptr;

template <typename... _Args>
class argument_counter {
public:
	static constexpr std::size_t count=sizeof...(_Args);
};

}

}

#define _STDEX_BIND_PARAMETER_BASE(name) \
	auto name##parameter_binder=stdex::utility::auto_argument_binder(__VA_ARGS__); \
	[[maybe_unused]] auto& name##1=stdex::utility::placeholder_proxy<0>(name##parameter_binder); \
	[[maybe_unused]] auto& name##2=stdex::utility::placeholder_proxy<1>(name##parameter_binder); \
	[[maybe_unused]] auto& name##3=stdex::utility::placeholder_proxy<2>(name##parameter_binder); \
	[[maybe_unused]] auto& name##4=stdex::utility::placeholder_proxy<3>(name##parameter_binder); \
	[[maybe_unused]] auto& name##5=stdex::utility::placeholder_proxy<4>(name##parameter_binder); \
	[[maybe_unused]] auto& name##6=stdex::utility::placeholder_proxy<5>(name##parameter_binder); \
	[[maybe_unused]] auto& name##7=stdex::utility::placeholder_proxy<6>(name##parameter_binder); \
	[[maybe_unused]] auto& name##8=stdex::utility::placeholder_proxy<7>(name##parameter_binder); \
	[[maybe_unused]] auto& name##9=stdex::utility::placeholder_proxy<8>(name##parameter_binder); \
	[[maybe_unused]] auto& name##10=stdex::utility::placeholder_proxy<9>(name##parameter_binder); \
	[[maybe_unused]] auto& name##11=stdex::utility::placeholder_proxy<10>(name##parameter_binder); \
	[[maybe_unused]] auto& name##12=stdex::utility::placeholder_proxy<11>(name##parameter_binder); \
	[[maybe_unused]] auto& name##13=stdex::utility::placeholder_proxy<12>(name##parameter_binder); \
	[[maybe_unused]] auto& name##14=stdex::utility::placeholder_proxy<13>(name##parameter_binder); \
	[[maybe_unused]] auto& name##15=stdex::utility::placeholder_proxy<14>(name##parameter_binder); \
	[[maybe_unused]] auto& name##16=stdex::utility::placeholder_proxy<15>(name##parameter_binder); \
	[[maybe_unused]] auto& name##17=stdex::utility::placeholder_proxy<16>(name##parameter_binder); \
	[[maybe_unused]] auto& name##18=stdex::utility::placeholder_proxy<17>(name##parameter_binder); \
	[[maybe_unused]] auto& name##19=stdex::utility::placeholder_proxy<18>(name##parameter_binder); \
	[[maybe_unused]] auto& name##20=stdex::utility::placeholder_proxy<19>(name##parameter_binder); \
	[[maybe_unused]] auto& name##21=stdex::utility::placeholder_proxy<20>(name##parameter_binder); \
	[[maybe_unused]] auto& name##22=stdex::utility::placeholder_proxy<21>(name##parameter_binder); \
	[[maybe_unused]] auto& name##23=stdex::utility::placeholder_proxy<22>(name##parameter_binder); \
	[[maybe_unused]] auto& name##24=stdex::utility::placeholder_proxy<23>(name##parameter_binder); \
	[[maybe_unused]] auto& name##25=stdex::utility::placeholder_proxy<24>(name##parameter_binder); \
	[[maybe_unused]] auto& name##26=stdex::utility::placeholder_proxy<25>(name##parameter_binder); \
	[[maybe_unused]] auto& name##27=stdex::utility::placeholder_proxy<26>(name##parameter_binder); \
	[[maybe_unused]] auto& name##28=stdex::utility::placeholder_proxy<27>(name##parameter_binder); \
	[[maybe_unused]] auto& name##29=stdex::utility::placeholder_proxy<28>(name##parameter_binder)

#define _STDEX_BIND_PARAMETER _STDEX_BIND_PARAMETER_BASE(_)

#define _STDEX_BIND_PARAMETER_NAME(name) _STDEX_BIND_PARAMETER_BASE(name##_)

#endif