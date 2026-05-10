//Last Modified At 2026/05/11
//@Version 1.0.0.0
#ifndef _STDEX_UTILITY_FORMAT_H_
#define _STDEX_UTILITY_FORMAT_H_ 1

#include <cctype>
#include <cstddef>
#include <initializer_list>
#include <memory>
#include <optional>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

#if __has_include("../macros/cpp_version.h")
#include "../macros/cpp_version.h"//At Least 1.0
#endif

#ifndef _STDEX_CPP20_VERSION
#define _STDEX_CPP20_VERSION 202002L
#endif

namespace stdex {

namespace utility {

class format_arg {
	template <typename _Tp,typename=void>
	struct has_adl_to_string : std::false_type {};

	template <typename _Tp>
	struct has_adl_to_string<_Tp,std::void_t<decltype(to_string(std::declval<const _Tp&>()))>> : std::true_type {};

	template <typename _Tp,typename=void>
	struct has_stream_insert : std::false_type {};

	template <typename _Tp>
	struct has_stream_insert<_Tp,std::void_t<decltype(std::declval<std::ostringstream&>() << std::declval<const _Tp&>())>> : std::true_type {};

	struct holder_base {
		virtual ~holder_base()=default;
		virtual std::string str() const=0;
		virtual std::unique_ptr<holder_base> clone() const=0;
	};

	template <typename _Tp>
	struct holder : holder_base {
		_Tp value;
		explicit holder(_Tp v) : value(std::move(v)) {}

		std::string str() const override {
			if constexpr (has_adl_to_string<_Tp>::value) {
				return to_string(value);
			} else if constexpr (std::is_same_v<_Tp,std::string>) {
				return value;
			} else if constexpr (has_stream_insert<_Tp>::value) {
				std::ostringstream oss;
				oss<<value;
				return oss.str();
			} else {
				return {};
			}
		}

		std::unique_ptr<holder_base> clone() const override {
			return std::make_unique<holder<_Tp>>(value);
		}
	};

	std::unique_ptr<holder_base> holder_;

public:
	template <typename _Tp,typename _Decay=std::decay_t<_Tp>,typename=std::enable_if_t<!std::is_same_v<_Decay,format_arg> && (has_adl_to_string<_Decay>::value || has_stream_insert<_Decay>::value || std::is_same_v<_Decay,std::string> || std::is_same_v<_Decay,std::string_view> || std::is_convertible_v<_Decay,const char*>)>>
	format_arg(_Tp&& value) : holder_(std::make_unique<holder<_Decay>>(std::forward<_Tp>(value))) { }
	format_arg(std::string_view sv) : holder_(std::make_unique<holder<std::string>>(std::string(sv))) { }
	format_arg(const char* s) : holder_(std::make_unique<holder<std::string>>(std::string(s?s:""))) { }
	~format_arg()=default;

	format_arg(const format_arg& other) : holder_(other.holder_?other.holder_->clone():nullptr) { }
	format_arg(format_arg&&) noexcept=default;

	format_arg& operator =(const format_arg& other) {
		if (this!=&other) holder_=other.holder_?other.holder_->clone():nullptr;
		return *this;
	}
	format_arg& operator =(format_arg&&) noexcept=default;


	std::string str() const {
		return holder_?holder_->str():std::string{};
	}

	explicit operator std::string() const { return str(); }
	bool valid() const noexcept { return holder_!=nullptr; }
	explicit operator bool() const noexcept { return valid(); }
};

class format_result {
	std::string raw_;

	static std::string apply_positional(const std::string& tmpl,const std::vector<format_arg>& args) {
		std::string result;
		result.reserve(tmpl.size());
		std::size_t i=0;
		while (i<tmpl.size()) {
			if (tmpl[i]=='{' && i+1<tmpl.size() && tmpl[i+1]=='{') {
				result+='{';
				i+=2;
				continue;
			}
			if (tmpl[i]=='}' && i+1<tmpl.size() && tmpl[i+1]=='}') {
				result+='}';
				i+=2;
				continue;
			}
			if (tmpl[i]=='{') {
				std::size_t j=tmpl.find('}',i+1);
				if (j!=std::string::npos) {
					std::string_view token(tmpl.data()+i+1,j-i-1);
					bool is_num=!token.empty();
					for (char ch:token) {
						if (std::isdigit(static_cast<unsigned char>(ch))) {
							is_num=false;
							break;
						}
					}
					if (is_num) {
						std::size_t idx=0;
						for (char ch:token) idx=idx*10+(ch-'0');
						if (idx<args.size()) {
							result+=args[idx].str();
							i=j+1;
							continue;
						}
					}
				}
			}
			result+=tmpl[i];
			i++;
		}
		return result;
	}

	static std::string apply_named(const std::string& tmpl,const std::unordered_map<std::string,format_arg>& args){
		std::string result;
		result.reserve(tmpl.size());
		std::size_t i=0;
		while (i<tmpl.size()) {
			if (tmpl[i]=='{' && i+1<tmpl.size() && tmpl[i+1]=='{') {
				result+='{';
				i+=2;
				continue;
			}
			if (tmpl[i]=='}' && i+1<tmpl.size() && tmpl[i+1]=='}') {
				result+='}';
				i+=2;
				continue;
			}
			if (tmpl[i]=='{') {
				std::size_t j=tmpl.find('}',i+1);
				if (j!=std::string::npos) {
					std::string name(tmpl.data()+i+1,j-i-1);
					auto it=args.find(name);
					if (it!=args.end()) {
						result+=it->second.str();
						i=j+1;
						continue;
					}
				}
			}
			result+=tmpl[i];
			i++;
		}
		return result;
	}

public:
	format_result()=default;
	format_result(std::string s) : raw_(std::move(s)) { }
	explicit format_result(std::string_view sv) : raw_(sv) { }
	format_result(const char* s) : raw_(s?s:"") { }
	explicit format_result(std::optional<std::string> opt) : raw_(opt?*std::move(opt):std::string{}) { }
	explicit format_result(std::optional<std::string_view> opt) : raw_(opt?std::string(*opt):std::string{}) { }
	~format_result()=default;

	format_result(const format_result&)=default;
	format_result(format_result&&) noexcept=default;

	format_result& operator =(const format_result&)=default;
	format_result& operator =(format_result&&) noexcept=default;
	
	template <typename... _Args>
	format_result format(_Args&&... args) const& {
		std::vector<format_arg> vec{format_arg(std::forward<_Args>(args))...};
		return format_result(apply_positional(raw_,vec));
	}
	template <typename... _Args>
	format_result format(_Args&&... args) && {
		std::vector<format_arg> vec{format_arg(std::forward<_Args>(args))...};
		return format_result(apply_positional(raw_,vec));
	}
	format_result format(const std::vector<format_arg>& args) const& {
		return format_result(apply_positional(raw_,args));
	}
	format_result format(std::vector<format_arg>&& args) const& {
		return format_result(apply_positional(raw_,args));
	}

	format_result format_named(std::initializer_list<std::pair<std::string_view,format_arg>> args) const& {
		std::unordered_map<std::string,format_arg> map;
		map.reserve(args.size());
		for (const auto& [k,v]:args) map.emplace(std::string(k),v);
		return format_result(apply_named(raw_,map));
	}

	format_result format_named(const std::unordered_map<std::string,format_arg>& args) const& {
		return format_result(apply_named(raw_,args));
	}

	template <typename _Map,typename=std::enable_if_t<std::is_same_v<typename _Map::mapped_type,format_arg>>>
	format_result format_named(const _Map& args) const {
		std::unordered_map<std::string,format_arg> map;
		for (const auto& [k,v]:args) map.emplace(k,v);
		return format_result(apply_named(raw_,map));
	}

	const std::string& str() const& noexcept { return raw_; }
	std::string str() && noexcept { return std::move(raw_); }
	std::string_view view() const noexcept { return raw_; }
	const char* c_str() const noexcept { return raw_.c_str(); }
	bool empty() const noexcept { return raw_.empty(); }
	std::size_t size() const noexcept { return raw_.size(); }
	std::size_t length() const noexcept { return raw_.length(); }

	operator std::string() const& { return raw_; }
	operator std::string() && { return std::move(raw_); }
	operator std::string_view() const noexcept { return raw_; }

	bool operator ==(const format_result& other) const noexcept { return raw_==other.raw_; }
	bool operator !=(const format_result& other) const noexcept { return !(*this==other); }
	bool operator ==(const std::string& s) const noexcept { return raw_==s; }
	bool operator !=(const std::string& s) const noexcept { return !(*this==s); }
	bool operator ==(std::string_view sv) const noexcept { return raw_==sv; }
	bool operator !=(std::string_view sv) const noexcept { return !(*this==sv); }
	bool operator ==(const char* s) const noexcept { return raw_==s; }
	bool operator !=(const char* s) const noexcept { return !(*this==s); }
	bool operator <(const format_result& other) const noexcept { return raw_<other.raw_; }
	bool operator <=(const format_result& other) const noexcept { return !(other<*this); }
	bool operator >(const format_result& other) const noexcept { return other<*this; }
	bool operator >=(const format_result& other) const noexcept { return !(*this<other); }

#if __cplusplus>=_STDEX_CPP20_VERSION
	auto operator <=>(const format_result& other) const noexcept { return raw_<=>other.raw_; }
	auto operator <=>(const std::string& s) const noexcept { return raw_<=>s; }
	auto operator <=>(std::string_view sv) const noexcept { return raw_<=>sv; }
#endif

	format_result operator +(const format_result& other) const {
		return format_result(raw_+other.raw_);
	}
	format_result operator +(const std::string& s) const {
		return format_result(raw_+s);
	}
	format_result operator +(std::string_view sv) const {
		return format_result(raw_+std::string(sv));
	}
	format_result operator +(const char* s) const {
		return format_result(raw_+s);
	}
	format_result& operator +=(const format_result& other) {
		raw_+=other.raw_;
		return *this;
	}
	format_result& operator +=(const std::string& s) {
		raw_+=s;
		return *this;
	}
	format_result& operator +=(std::string_view sv) {
		raw_+=sv;
		return *this;
	}
	format_result& operator +=(const char* s) {
		raw_+=s;
		return *this;
	}

	friend std::ostream& operator <<(std::ostream& os,const format_result& r) {
		return os<<r.raw_;
	}
};

inline bool operator ==(const std::string& s,const format_result& r) noexcept {
	return r==s;
}
inline bool operator !=(const std::string& s,const format_result& r) noexcept {
	return !(r==s);
}
inline bool operator==(std::string_view sv,const format_result& r) noexcept {
	return r==sv;
}
inline bool operator!=(std::string_view sv,const format_result& r) noexcept {
	return !(r==sv);
}
inline bool operator ==(const char* s,const format_result& r) noexcept {
	return r==s;
}
inline bool operator !=(const char* s,const format_result& r) noexcept {
	return !(r==s);
}
inline format_result operator +(const std::string& s,const format_result& r) {
	return format_result(s+r.str());
}
inline format_result operator +(std::string_view sv,const format_result& r) {
	return format_result(std::string(sv)+r.str());
}
inline format_result operator +(const char* s,const format_result& r) {
	return format_result(std::string(s)+r.str());
}

}

}

namespace std {

template <>
struct hash<stdex::utility::format_result> {
	std::size_t operator ()(const stdex::utility::format_result& r) const noexcept {
		return std::hash<std::string_view>{}(r.view());
	}
};



#endif