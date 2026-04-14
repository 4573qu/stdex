//Last Modified At 2026/04/14
//@Version 1.1.0.0
#ifndef _STDEX_META_REFLECT_H_
#define _STDEX_META_REFLECT_H_ 1

#include <any>
#include <array>
#include <cstddef>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>


#if __has_include("../macros/cpp_version.h")
#include "../macros/cpp_version.h"//At Least 1.0
#endif

#ifndef _STDEX_CPP20_VERSION
#define _STDEX_CPP20_VERSION 202002L
#endif

#if __cplusplus>=_STDEX_CPP20_VERSION
#include <span>
#endif

namespace stdex {

namespace meta {

namespace reflect {

enum access_kind {
	AK_PUBLIC,
	AK_PROTECTED,
	AK_PRIVATE,
};

enum type_kind {
	TK_UNKNOWN,
	TK_FUNDAMENTAL,
	TK_CLASS,
	TK_ENUM,
};

template <typename _Tp>
struct remove_cvref {
	using type=typename std::remove_cv<typename std::remove_reference<_Tp>::type>::type;
};

template <typename _Tp>
using remove_cvref_t=typename remove_cvref<_Tp>::type;

template <typename _Tp>
struct remove_cv {
	using type=typename std::remove_cv<_Tp>::type;
};

template <typename _Tp>
using remove_cv_t=typename remove_cv<_Tp>::type;

template <typename _Tp>
struct remove_ref {
	using type=typename std::remove_reference<_Tp>::type;
};

template <typename _Tp>
using remove_ref_t=typename remove_ref<_Tp>::type;

template <typename _Tp>
struct remove_pointer {
	using type=typename std::remove_pointer<_Tp>::type;
};

template <typename _Tp>
using remove_pointer_t=typename remove_pointer<_Tp>::type;

template <typename _Tp>
struct identity {
	using type=_Tp;
};

template <typename _Tp>
using identity_t=typename identity<_Tp>::type;

struct attribute;
struct base;
struct constructor;
struct enum_value;
struct field;
struct method;
struct type;

struct attribute {
	std::string_view name_;
	std::any value_;
	type(*value_type_getter)() noexcept;

	constexpr std::string_view name() const noexcept { return name_; }
	const std::any& value() const noexcept { return value_; }
	type value_type() const noexcept;

	template <typename _Tp>
	const _Tp* value_as() const noexcept {
		return std::any_cast<_Tp>(&value_);
	}
};

struct attribute_list {
	const attribute* data;
	std::size_t size_;

	constexpr const attribute* begin() const noexcept { return data; }
	constexpr const attribute* end() const noexcept { return data+size_; }
	constexpr std::size_t size() const noexcept { return size_; }
	constexpr bool empty() const noexcept { return size_==0; }
	constexpr const attribute& operator [](std::size_t index) const noexcept { return data[index]; }

#if __cplusplus>=_STDEX_CPP20_VERSION
	constexpr std::span<const attribute> as_span() const noexcept {
		return std::span<const attribute>(data,size_);
	}
	constexpr operator std::span<const attribute>() const noexcept {
		return std::span<const attribute>(data,size_);
	}
#endif
};

struct base {
	type(*value_type_getter)() noexcept;
	void*(*cast_mut)(void*) noexcept;
	const void*(*cast_const)(const void*) noexcept;
	access_kind access_;
	bool virtual_;

	type value_type() const noexcept;
	void* get(void* obj) const noexcept { return cast_mut?cast_mut(obj):nullptr; }
	const void* get(const void* obj) const noexcept { return cast_const?cast_const(obj):nullptr; }
	constexpr access_kind access() const noexcept { return access_; }
	constexpr bool is_virtual() const noexcept { return virtual_; }
};

struct base_list {
	const base* data;
	std::size_t size_;

	constexpr const base* begin() const noexcept { return data; }
	constexpr const base* end() const noexcept { return data+size_; }
	constexpr std::size_t size() const noexcept { return size_; }
	constexpr bool empty() const noexcept { return size_==0; }
	constexpr const base& operator [](std::size_t index) const noexcept { return data[index]; }

#if __cplusplus>=_STDEX_CPP20_VERSION
	constexpr std::span<const base> as_span() const noexcept {
		return std::span<const base>(data,size_);
	}
	constexpr operator std::span<const base>() const noexcept {
		return std::span<const base>(data,size_);
	}
#endif
};

struct constructor {
	const type* parameter_types_;
	std::size_t parameter_count_;
	access_kind access_;
	const attribute* attributes_;
	std::size_t attribute_count_;
	std::any(*invoke)(const std::any*,std::size_t);

	constexpr const type* parameter_types() const noexcept { return parameter_types_; }
	constexpr std::size_t parameter_count() const noexcept { return parameter_count_; }
	constexpr access_kind access() const noexcept { return access_; }
	constexpr const attribute* attributes() const noexcept { return attributes_; }
	constexpr std::size_t attribute_count() const noexcept { return attribute_count_; }

	std::any create(const std::any* args,std::size_t count) const {
		return invoke?invoke(args,count):std::any{};
	}

	template <typename _Alloc=std::allocator<unsigned char>>
#ifndef _STDEX_IGNORE_META_REFLECT_WARNINGS
	[[deprecated("Create raw is not working correctly at present")]]
#endif
	void* create_raw(const std::any* args,std::size_t count,_Alloc alloc=_Alloc{}) const {
		std::any v=create(args,count);
		if (!v.has_value()) return nullptr;
		return nullptr;
	}
};

struct constructor_list {
	const constructor* data;
	std::size_t size_;

	constexpr const constructor* begin() const noexcept { return data; }
	constexpr const constructor* end() const noexcept { return data+size_; }
	constexpr std::size_t size() const noexcept { return size_; }
	constexpr bool empty() const noexcept { return size_==0; }
	constexpr const constructor& operator [](std::size_t index) const noexcept { return data[index]; }

#if __cplusplus>=_STDEX_CPP20_VERSION
	constexpr std::span<const constructor> as_span() const noexcept {
		return std::span<const constructor>(data,size_);
	}
	constexpr operator std::span<const constructor>() const noexcept {
		return std::span<const constructor>(data,size_);
	}
#endif
};

struct enum_value {
	std::string_view name_;
	long long value_;
	const attribute* attributes_;
	std::size_t attribute_count_;

	constexpr std::string_view name() const noexcept { return name_; }
	constexpr long long value() const noexcept { return value_; }
	constexpr const attribute* attributes() const noexcept { return attributes_; }
	constexpr std::size_t attribute_count() const noexcept { return attribute_count_; }
};

struct enum_value_list {
	const enum_value* data;
	std::size_t size_;

	constexpr const enum_value* begin() const noexcept { return data; }
	constexpr const enum_value* end() const noexcept { return data+size_; }
	constexpr std::size_t size() const noexcept { return size_; }
	constexpr bool empty() const noexcept { return size_==0; }
	constexpr const enum_value& operator [](std::size_t index) const noexcept { return data[index]; }

#if __cplusplus>=_STDEX_CPP20_VERSION
	constexpr std::span<const enum_value> as_span() const noexcept {
		return std::span<const enum_value>(data,size_);
	}
	constexpr operator std::span<const enum_value>() const noexcept {
		return std::span<const enum_value>(data,size_);
	}
#endif
};

struct field {
	std::string_view name_;
	type(*value_type_getter)() noexcept;
	void*(*get_mut)(void*) noexcept;
	const void*(*get_const)(const void*) noexcept;
	bool(*set_from_any)(void*,const std::any&) noexcept;
	std::any(*get_as_any)(const void*);
	access_kind access_;
	bool static_;
	bool const_;
	const attribute* attributes_;
	std::size_t attribute_count_;

	constexpr std::string_view name() const noexcept { return name_; }
	type value_type() const noexcept;
	void* get(void* obj) const noexcept { return get_mut?get_mut(obj):nullptr; }
	const void* get(const void* obj) const noexcept { return get_const?get_const(obj):nullptr; }

	bool set(void* obj,const std::any& value) const noexcept {
		return set_from_any?set_from_any(obj,value):false;
	}

	std::any get_any(const void* obj) const {
		return get_as_any?get_as_any(obj):std::any{};
	}

	constexpr access_kind access() const noexcept { return access_; }
	constexpr bool is_static() const noexcept { return static_; }
	constexpr bool is_const() const noexcept { return const_; }
	constexpr const attribute* attributes() const noexcept { return attributes_; }
	constexpr std::size_t attribute_count() const noexcept { return attribute_count_; }
};

struct field_list {
	const field* data;
	std::size_t size_;

	constexpr const field* begin() const noexcept { return data; }
	constexpr const field* end() const noexcept { return data+size_; }
	constexpr std::size_t size() const noexcept { return size_; }
	constexpr bool empty() const noexcept { return size_==0; }
	constexpr const field& operator [](std::size_t index) const noexcept { return data[index]; }

#if __cplusplus>=_STDEX_CPP20_VERSION
	constexpr std::span<const field> as_span() const noexcept {
		return std::span<const field>(data,size_);
	}
	constexpr operator std::span<const field>() const noexcept {
		return std::span<const field>(data,size_);
	}
#endif
};

struct method {
	std::string_view name_;
	type(*return_type_getter)() noexcept;
	const type* parameter_types_;
	std::size_t parameter_count_;
	std::any(*invoke_mut)(void*,const std::any*,std::size_t);
	std::any(*invoke_const)(const void*,const std::any*,std::size_t);
	std::any(*invoke_static)(const std::any*,std::size_t);
	access_kind access_;
	bool const_;
	bool static_;
	const attribute* attributes_;
	std::size_t attribute_count_;

	constexpr std::string_view name() const noexcept { return name_; }
	type return_type() const noexcept;
	constexpr const type* parameter_types() const noexcept { return parameter_types_; }
	constexpr std::size_t parameter_count() const noexcept { return parameter_count_; }
	constexpr access_kind access() const noexcept { return access_; }
	constexpr bool is_const() const noexcept { return const_; }
	constexpr bool is_static() const noexcept { return static_; }
	constexpr const attribute* attributes() const noexcept { return attributes_; }
	constexpr std::size_t attribute_count() const noexcept { return attribute_count_; }

	std::any invoke(void* obj,const std::any* args,std::size_t count) const {
		if (static_) return invoke_static?invoke_static(args,count):std::any{};
		return invoke_mut?invoke_mut(obj,args,count):std::any{};
	}

	std::any invoke(const void* obj,const std::any* args,std::size_t count) const {
		if (static_) return invoke_static?invoke_static(args,count):std::any{};
		return invoke_const?invoke_const(obj,args,count):std::any{};
	}
	
	std::any invoke(std::nullptr_t,const std::any* args,std::size_t count) const {
		return static_?(invoke_static?invoke_static(args,count):std::any{}):std::any{};
	}
};

struct method_list {
	const method* data;
	std::size_t size_;

	constexpr const method* begin() const noexcept { return data; }
	constexpr const method* end() const noexcept { return data+size_; }
	constexpr std::size_t size() const noexcept { return size_; }
	constexpr bool empty() const noexcept { return size_==0; }
	constexpr const method& operator [](std::size_t index) const noexcept { return data[index]; }

#if __cplusplus>=_STDEX_CPP20_VERSION
	constexpr std::span<const method> as_span() const noexcept {
		return std::span<const method>(data,size_);
	}
	constexpr operator std::span<const method>() const noexcept {
		return std::span<const method>(data,size_);
	}
#endif
};

struct type {
	std::string_view name_;
	type_kind kind_;
	field_list fields_;
	method_list methods_;
	constructor_list constructors_;
	base_list bases_;
	enum_value_list enum_values_;
	attribute_list attributes_;
	std::size_t size_;
	std::size_t alignment_;
	bool const_;
	bool volatile_;
	bool reference;
	bool pointer;
	bool array;
	bool default_constructible;
	bool copy_constructible;
	bool move_constructible;
	bool copy_assignable;
	bool move_assignable;

	constexpr std::string_view name() const noexcept { return name_; }
	constexpr type_kind kind() const noexcept { return kind_; }
	constexpr bool is_unknown() const noexcept { return kind_==TK_UNKNOWN; }
	constexpr bool is_fundamental() const noexcept { return kind_==TK_FUNDAMENTAL; }
	constexpr bool is_class() const noexcept { return kind_==TK_CLASS; }
	constexpr bool is_enum() const noexcept { return kind_==TK_ENUM; }
	constexpr field_list fields() const noexcept { return fields_; }
	constexpr method_list methods() const noexcept { return methods_; }
	constexpr constructor_list constructors() const noexcept { return constructors_; }
	constexpr base_list bases() const noexcept { return bases_; }
	constexpr enum_value_list enum_values() const noexcept { return enum_values_; }
	constexpr attribute_list attributes() const noexcept { return attributes_; }
	constexpr std::size_t size() const noexcept { return size_; }
	constexpr std::size_t alignment() const noexcept { return alignment_; }
	constexpr bool is_const() const noexcept { return const_; }
	constexpr bool is_volatile() const noexcept { return volatile_; }
	constexpr bool is_reference() const noexcept { return reference; }
	constexpr bool is_pointer() const noexcept { return pointer; }
	constexpr bool is_array() const noexcept { return array; }
	constexpr bool is_default_constructible() const noexcept { return default_constructible; }
	constexpr bool is_copy_constructible() const noexcept { return copy_constructible; }
	constexpr bool is_move_constructible() const noexcept { return move_constructible; }
	constexpr bool is_copy_assignable() const noexcept { return copy_assignable; }
	constexpr bool is_move_assignable() const noexcept { return move_assignable; }

#if __cplusplus>=_STDEX_CPP20_VERSION
	constexpr std::span<const field> fields_span() const noexcept { return fields_; }
	constexpr std::span<const method> methods_span() const noexcept { return methods_; }
	constexpr std::span<const constructor> constructors_span() const noexcept { return constructors_; }
	constexpr std::span<const base> bases_span() const noexcept { return bases_; }
	constexpr std::span<const enum_value> enum_values_span() const noexcept { return enum_values_; }
	constexpr std::span<const attribute> attributes_span() const noexcept { return attributes_; }
#endif

	constexpr bool operator ==(const type& other) const noexcept {
		return kind_==other.kind_ && name_==other.name_ && size_==other.size_ && alignment_==other.alignment_;
	}

	constexpr bool operator !=(const type& other) const noexcept {
		return !(*this==other);
	}
};

inline type attribute::value_type() const noexcept {
	return value_type_getter();
}

inline type base::value_type() const noexcept {
	return value_type_getter();
}

inline type field::value_type() const noexcept {
	return value_type_getter();
}

inline type method::return_type() const noexcept {
	return return_type_getter();
}

template <typename _Tp>
struct descriptor {
	static constexpr bool reflectable=false;
	static constexpr bool class_reflectable=false;
	static constexpr bool enum_reflectable=false;

	static constexpr type get() noexcept {
		using _Up=remove_cvref_t<_Tp>;
		return type{std::string_view("<unreflected>"),TK_UNKNOWN,field_list{nullptr,0},method_list{nullptr,0},constructor_list{nullptr,0},base_list{nullptr,0},enum_value_list{nullptr,0},attribute_list{nullptr,0},std::is_void<_Up>::value?0:sizeof(_Up),std::is_void<_Up>::value?0:alignof(_Up),std::is_const<_Tp>::value,std::is_volatile<_Tp>::value,std::is_reference<_Tp>::value,std::is_pointer<_Up>::value,std::is_array<_Up>::value,std::is_default_constructible<_Up>::value,std::is_copy_constructible<_Up>::value,std::is_move_constructible<_Up>::value,std::is_copy_assignable<_Up>::value,std::is_move_assignable<_Up>::value};
	}
};

template <>
struct descriptor<void> {
	static constexpr bool reflectable=true;
	static constexpr bool class_reflectable=false;
	static constexpr bool enum_reflectable=false;

	static constexpr type get() noexcept {
		return type{std::string_view("void"),TK_FUNDAMENTAL,field_list{nullptr,0},method_list{nullptr,0},constructor_list{nullptr,0},base_list{nullptr,0},enum_value_list{nullptr,0},attribute_list{nullptr,0},0,0,false,false,false,false,false,false,false,false,false,false};
	}
};

template <typename _Tp>
constexpr type reflect() noexcept {
	return descriptor<_Tp>::get();
}

template <typename _Tp>
struct is_reflectable : std::integral_constant<bool,descriptor<remove_cvref_t<_Tp>>::reflectable> { };

template <typename _Tp>
struct is_class_reflectable : std::integral_constant<bool,descriptor<remove_cvref_t<_Tp>>::class_reflectable> { };

template <typename _Tp>
struct is_enum_reflectable : std::integral_constant<bool,descriptor<remove_cvref_t<_Tp>>::enum_reflectable> { };

template <typename _Tp>
constexpr bool is_reflectable_v=is_reflectable<_Tp>::value;

template <typename _Tp>
constexpr bool is_class_reflectable_v=is_class_reflectable<_Tp>::value;

template <typename _Tp>
constexpr bool is_enum_reflectable_v=is_enum_reflectable<_Tp>::value;

template <typename _Attr>
struct attribute_descriptor {
	std::string_view name;
	_Attr value;

	constexpr attribute_descriptor(std::string_view name,const _Attr& value) noexcept : name(name) , value(value) { }

	static type value_type_get() noexcept {
		return descriptor<_Attr>::get();
	}
	attribute make_runtime() const {
		return attribute{name,std::any(value),&value_type_get};
	}
};

template <typename _Attr>
constexpr auto make_attribute(std::string_view name,const _Attr& value) noexcept {
	return attribute_descriptor<_Attr>(name,value);
}

template <typename... _Args>
inline auto make_attributes(_Args... args) {
	return std::array<attribute,sizeof...(_Args)>{args.make_runtime()...};
}

template <typename _Tp,typename _Base,access_kind _Access,bool _Virtual>
struct base_descriptor {
	using class_type=_Tp;
	using value_type=_Base;

	static type value_type_get() noexcept {
		return descriptor<value_type>::get();
	}
	static base make_runtime() noexcept {
		return base{&value_type_get,&cast_mut,&cast_const,_Access,_Virtual};
	}
	static void* cast_mut(void* obj) noexcept {
		return static_cast<value_type*>(static_cast<class_type*>(obj));
	}
	static const void* cast_const(const void* obj) noexcept {
		return static_cast<const value_type*>(static_cast<const class_type*>(obj));
	}
};

template <typename _Tp,typename _Mem,_Mem _Tp::*_Member,access_kind _Access,bool _Const,std::size_t _AttrCount>
struct member_field_descriptor {
	using class_type=_Tp;
	using value_type=_Mem;

	std::string_view name;
	std::array<attribute,_AttrCount> attributes;

	constexpr member_field_descriptor(std::string_view name,std::array<attribute,_AttrCount> attributes) noexcept : name(name) , attributes(attributes) { }

	static type value_type_get() noexcept {
		return descriptor<value_type>::get();
	}
	static void* get_mut(void* obj) noexcept {
		return &(static_cast<class_type*>(obj)->*_Member);
	}
	static const void* get_const(const void* obj) noexcept {
		return &(static_cast<const class_type*>(obj)->*_Member);
	}
	static bool set_from_any(void* obj,const std::any& value) noexcept {
		if constexpr (std::is_const<value_type>::value || _Const) {
			(void)obj;
			(void)value;
			return false;
		} else {
			using _Store=remove_cvref_t<value_type>;
			if (const auto* p=std::any_cast<_Store>(&value)) {
				static_cast<class_type*>(obj)->*_Member=*p;
				return true;
			}
			return false;
		}
	}
	static std::any get_as_any(const void* obj) {
		return std::any(static_cast<const class_type*>(obj)->*_Member);
	}
	field make_runtime() const noexcept {
		return field{name,&value_type_get,&get_mut,&get_const,&set_from_any,&get_as_any,_Access,false,std::is_const<value_type>::value || _Const,attributes.data(),attributes.size()};
	}
};

template <typename _Tp,typename _Mem,_Mem* _Member,access_kind _Access,bool _Const,std::size_t _AttrCount>
struct static_field_descriptor {
	using class_type=_Tp;
	using value_type=_Mem;

	std::string_view name;
	std::array<attribute,_AttrCount> attributes;

	constexpr static_field_descriptor(std::string_view name,std::array<attribute,_AttrCount> attributes) noexcept : name(name) , attributes(attributes) { }

	static type value_type_get() noexcept {
		return descriptor<value_type>::get();
	}
	static void* get_mut(void*) noexcept {
		return _Member;
	}
	static const void* get_const(const void*) noexcept {
		return _Member;
	}
	static bool set_from_any(void*,const std::any& value) noexcept {
		if constexpr (std::is_const<value_type>::value || _Const) {
			(void)value;
			return false;
		} else {
			using _Store=remove_cvref_t<value_type>;
			if (const auto* p=std::any_cast<_Store>(&value)) {
				*_Member=*p;
				return true;
			}
			return false;
		}
	}
	static std::any get_as_any(const void*) {
		return std::any(*_Member);
	}
	field make_runtime() const noexcept {
		return field{name,&value_type_get,&get_mut,&get_const,&set_from_any,&get_as_any,_Access,true,std::is_const<value_type>::value || _Const,attributes.data(),attributes.size()};
	}
};

template <typename _Tp,typename _Mem,_Mem _Tp::*_Member,access_kind _Access=AK_PUBLIC,bool _Const=false,std::size_t _AttrCount=0>
constexpr auto make_field(std::string_view name,const std::array<attribute,_AttrCount>& attributes=std::array<attribute,_AttrCount>{}) noexcept {
	return member_field_descriptor<_Tp,_Mem,_Member,_Access,_Const,_AttrCount>{name,attributes};
}

template <typename _Tp,typename _Mem,_Mem* _Member,access_kind _Access=AK_PUBLIC,bool _Const=false,std::size_t _AttrCount=0>
constexpr auto make_static_field(std::string_view name,const std::array<attribute,_AttrCount>& attributes=std::array<attribute,_AttrCount>{}) noexcept {
	return static_field_descriptor<_Tp,_Mem,_Member,_Access,_Const,_AttrCount>{name,attributes};
}

template <typename _Tp,typename _Base,access_kind _Access=AK_PUBLIC,bool _Virtual=false>
constexpr auto make_base() noexcept {
	return base_descriptor<_Tp,_Base,_Access,_Virtual>{};
}

template <typename _Tp,access_kind _Access,std::size_t _AttrCount,typename... _Args>
struct constructor_descriptor {
	std::array<attribute,_AttrCount> attributes;

	constexpr constructor_descriptor(std::array<attribute,_AttrCount> attributes) noexcept : attributes(attributes) { }

	static inline std::array<type,sizeof...(_Args)> parameter_types={descriptor<_Args>::get()...};
	template <std::size_t..._Index>
	static std::any create_impl(const std::any* args,std::index_sequence<_Index...>) {
		if constexpr (std::is_constructible<_Tp,_Args...>::value) {
			return std::any(_Tp(std::any_cast<remove_cvref_t<_Args>>(args[_Index])...));
		} else {
			throw std::runtime_error("type is not constructible with given signature");
		}
	}
	static std::any invoke(const std::any* args,std::size_t count) {
		if (count!=sizeof...(_Args)) throw std::runtime_error("argument count mismatch");
		return create_impl(args,std::index_sequence_for<_Args...>{});
	}

	constructor make_runtime() const noexcept {
		return constructor{parameter_types.data(),parameter_types.size(),_Access,attributes.data(),attributes.size(),&invoke};
	}
};

template <typename _Tp,access_kind _Access=AK_PUBLIC,typename... _Args,std::size_t _AttrCount=0>
constexpr auto make_constructor(const std::array<attribute,_AttrCount>& attributes=std::array<attribute,_AttrCount>{}) noexcept {
	return constructor_descriptor<_Tp,_Access,_AttrCount,_Args...>{attributes};
}

template <typename _Enum,_Enum _Value,std::size_t _AttrCount>
struct enum_value_descriptor {
	std::string_view name;
	std::array<attribute,_AttrCount> attributes;

	constexpr enum_value_descriptor(std::string_view name,std::array<attribute,_AttrCount> attributes) noexcept : name(name) , attributes(attributes) { }
	enum_value make_runtime() const noexcept {
		return enum_value{name,static_cast<long long>(_Value),attributes.data(),attributes.size()};
	}
};

template <typename _Enum,_Enum _Value,std::size_t _AttrCount=0>
constexpr auto make_enum_value(std::string_view name,const std::array<attribute,_AttrCount>& attributes=std::array<attribute,_AttrCount>{}) noexcept {
	return enum_value_descriptor<_Enum,_Value,_AttrCount>{name,attributes};
}

template <typename _Tp>
struct function_traits;

template <typename _Class,typename _Return,typename... _Args>
struct function_traits<_Return(_Class::*)(_Args...)> {
	using class_type=_Class;
	using return_type=_Return;
	using args_tuple=std::tuple<_Args...>;
	static constexpr bool const_=false;
	static constexpr bool static_=false;
};

template <typename _Class,typename _Return,typename... _Args>
struct function_traits<_Return(_Class::*)(_Args...) const> {
	using class_type=_Class;
	using return_type=_Return;
	using args_tuple=std::tuple<_Args...>;
	static constexpr bool const_=true;
	static constexpr bool static_=false;
};

template <typename _Return,typename ..._Args>
struct function_traits<_Return(*)(_Args...)> {
	using class_type=void;
	using return_type=_Return;
	using args_tuple=std::tuple<_Args...>;
	static constexpr bool const_=false;
	static constexpr bool static_=true;
};

template <typename _Tp,std::size_t _Index>
using tuple_element_t=typename std::tuple_element<_Index,_Tp>::type;

template <typename _MethodType,_MethodType _Method,access_kind _Access,std::size_t _AttrCount>
struct member_method_descriptor;

template <typename _Class,typename _Return,typename... _Args,_Return(_Class::*_Method)(_Args...),access_kind _Access,std::size_t _AttrCount>
struct member_method_descriptor<_Return(_Class::*)(_Args...),_Method,_Access,_AttrCount> {
	std::string_view name;
	std::array<attribute,_AttrCount> attributes;

	constexpr member_method_descriptor(std::string_view name,std::array<attribute,_AttrCount> attributes) noexcept : name(name) , attributes(attributes) { }

	static type return_type_get() noexcept {
		return descriptor<_Return>::get();
	}
	static inline std::array<type,sizeof...(_Args)> parameter_types={descriptor<_Args>::get()...};
	template <std::size_t..._Index>
	static std::any invoke_impl(_Class* obj,const std::any* args,std::index_sequence<_Index...>) {
		if constexpr (std::is_void<_Return>::value) {
			(obj->*_Method)(std::any_cast<remove_cvref_t<_Args>>(args[_Index])...);
			return std::any{};
		} else {
			return std::any((obj->*_Method)(std::any_cast<remove_cvref_t<_Args>>(args[_Index])...));
		}
	}
	static std::any invoke_mut(void* obj,const std::any* args,std::size_t count) {
		if (count!=sizeof...(_Args)) throw std::runtime_error("argument count mismatch");
		return invoke_impl(static_cast<_Class*>(obj),args,std::index_sequence_for<_Args...>{});
	}
	static std::any invoke_const(const void*,const std::any*,std::size_t) {
		throw std::runtime_error("method is non-const");
	}
	static std::any invoke_static(const std::any*,std::size_t) {
		throw std::runtime_error("method is not static");
	}
	method make_runtime() const noexcept {
		return method{name,&return_type_get,parameter_types.data(),parameter_types.size(),&invoke_mut,&invoke_const,&invoke_static,_Access,false,false,attributes.data(),attributes.size()};
	}
};

template <typename _MethodType,_MethodType _Method,access_kind _Access,std::size_t _AttrCount>
struct const_method_descriptor;

template <typename _Class,typename _Return,typename... _Args,_Return(_Class::*_Method)(_Args...) const,access_kind _Access,std::size_t _AttrCount>
struct const_method_descriptor<_Return(_Class::*)(_Args...) const,_Method,_Access,_AttrCount> {
	std::string_view name;
	std::array<attribute,_AttrCount> attributes;

	constexpr const_method_descriptor(std::string_view name,std::array<attribute,_AttrCount> attributes) noexcept : name(name) , attributes(attributes) { }
	static type return_type_get() noexcept {
		return descriptor<_Return>::get();
	}
	static inline std::array<type,sizeof...(_Args)> parameter_types={descriptor<_Args>::get()...};
	template <std::size_t..._Index>
	static std::any invoke_impl(const _Class* obj,const std::any* args,std::index_sequence<_Index...>) {
		if constexpr (std::is_void<_Return>::value) {
			(obj->*_Method)(std::any_cast<remove_cvref_t<_Args>>(args[_Index])...);
			return std::any{};
		} else {
			return std::any((obj->*_Method)(std::any_cast<remove_cvref_t<_Args>>(args[_Index])...));
		}
	}
	static std::any invoke_mut(void* obj,const std::any* args,std::size_t count) {
		if (count!=sizeof...(_Args)) throw std::runtime_error("argument count mismatch");
		return invoke_impl(static_cast<const _Class*>(obj),args,std::index_sequence_for<_Args...>{});
	}
	static std::any invoke_const(const void* obj,const std::any* args,std::size_t count) {
		if (count!=sizeof...(_Args)) throw std::runtime_error("argument count mismatch");
		return invoke_impl(static_cast<const _Class*>(obj),args,std::index_sequence_for<_Args...>{});
	}
	static std::any invoke_static(const std::any*,std::size_t) {
		throw std::runtime_error("method is not static");
	}
	method make_runtime() const noexcept {
		return method{
			name,&return_type_get,parameter_types.data(),parameter_types.size(),&invoke_mut,&invoke_const,&invoke_static,_Access,true,false,attributes.data(),attributes.size()};
	}
};

template <typename _MethodType,_MethodType _Method,access_kind _Access,std::size_t _AttrCount>
struct static_method_descriptor;

template <typename _Return,typename... _Args,_Return(*_Method)(_Args...),access_kind _Access,std::size_t _AttrCount>
struct static_method_descriptor<_Return(*)(_Args...),_Method,_Access,_AttrCount> {
	std::string_view name;
	std::array<attribute,_AttrCount> attributes;

	constexpr static_method_descriptor(std::string_view name,std::array<attribute,_AttrCount> attributes) noexcept : name(name) , attributes(attributes) { }

	static type return_type_get() noexcept {
		return descriptor<_Return>::get();
	}
	static inline std::array<type,sizeof...(_Args)> parameter_types={descriptor<_Args>::get()...};
	template <std::size_t..._Index>
	static std::any invoke_impl(const std::any* args,std::index_sequence<_Index...>) {
		if constexpr (std::is_void<_Return>::value) {
			(*_Method)(std::any_cast<remove_cvref_t<_Args>>(args[_Index])...);
			return std::any{};
		} else {
			return std::any((*_Method)(std::any_cast<remove_cvref_t<_Args>>(args[_Index])...));
		}
	}
	static std::any invoke_mut(void*,const std::any* args,std::size_t count) {
		if (count!=sizeof...(_Args)) throw std::runtime_error("argument count mismatch");
		return invoke_impl(args,std::index_sequence_for<_Args...>{});
	}
	static std::any invoke_const(const void*,const std::any* args,std::size_t count) {
		if (count!=sizeof...(_Args)) throw std::runtime_error("argument count mismatch");
		return invoke_impl(args,std::index_sequence_for<_Args...>{});
	}
	static std::any invoke_static(const std::any* args,std::size_t count) {
		if (count!=sizeof...(_Args)) throw std::runtime_error("argument count mismatch");
		return invoke_impl(args,std::index_sequence_for<_Args...>{});
	}
	method make_runtime() const noexcept {
		return method{name,&return_type_get,parameter_types.data(),parameter_types.size(),&invoke_mut,&invoke_const,&invoke_static,_Access,false,true,attributes.data(),attributes.size()};
	}
};

template <auto _Method,access_kind _Access=AK_PUBLIC,std::size_t _AttrCount=0>
struct auto_method_descriptor;

template <typename _Class,typename _Return,typename... _Args,_Return(_Class::*_Method)(_Args...),access_kind _Access,std::size_t _AttrCount>
struct auto_method_descriptor<_Method,_Access,_AttrCount> {
	using wrapped_type=member_method_descriptor<_Return(_Class::*)(_Args...),_Method,_Access,_AttrCount>;

	static constexpr auto make(std::string_view name,const std::array<attribute,_AttrCount>& attributes=std::array<attribute,_AttrCount>{}) noexcept {
		return wrapped_type{name,attributes};
	}
};

template <typename _Class,typename _Return,typename... _Args,_Return(_Class::*_Method)(_Args...) const,access_kind _Access,std::size_t _AttrCount>
struct auto_method_descriptor<_Method,_Access,_AttrCount> {
	using wrapped_type=const_method_descriptor<_Return(_Class::*)(_Args...) const,_Method,_Access,_AttrCount>;

	static constexpr auto make(std::string_view name,const std::array<attribute,_AttrCount>& attributes=std::array<attribute,_AttrCount>{}) noexcept {
		return wrapped_type{name,attributes};
	}
};

template <typename _Return,typename... _Args,_Return(*_Method)(_Args...),access_kind _Access,std::size_t _AttrCount>
struct auto_method_descriptor<_Method,_Access,_AttrCount> {
	using wrapped_type=static_method_descriptor<_Return(*)(_Args...),_Method,_Access,_AttrCount>;

	static constexpr auto make(std::string_view name,const std::array<attribute,_AttrCount>& attributes=std::array<attribute,_AttrCount>{}) noexcept {
		return wrapped_type{name,attributes};
	}
};

template <auto _Method,access_kind _Access=AK_PUBLIC,std::size_t _AttrCount=0>
constexpr auto make_method(std::string_view name,const std::array<attribute,_AttrCount>& attributes=std::array<attribute,_AttrCount>{}) noexcept {
	return auto_method_descriptor<_Method,_Access,_AttrCount>::make(name,attributes);
}

template <typename... _Args>
struct fields_pack {
	static constexpr auto items=std::make_tuple(_Args{}...);
};

template <typename... _Args>
struct methods_pack {
	static constexpr auto items=std::make_tuple(_Args{}...);
};

template <typename... _Args>
struct ctors_pack {
	static constexpr auto items=std::make_tuple(_Args{}...);
};

template <typename... _Args>
struct bases_pack {
	static constexpr auto items=std::make_tuple(_Args{}...);
};

template <typename... _Args>
struct attrs_pack {
	static constexpr auto items=std::make_tuple(_Args{}...);
};

template <typename... _Args>
constexpr auto pack_fields(_Args... args) {
	return std::make_tuple(args...);
}

template <typename... _Args>
constexpr auto pack_methods(_Args... args) {
	return std::make_tuple(args...);
}

template <typename... _Args>
constexpr auto pack_ctors(_Args... args) {
	return std::make_tuple(args...);
}

template <typename... _Args>
constexpr auto pack_bases(_Args... args) {
	return std::make_tuple(args...);
}

template <typename... _Args>
constexpr auto pack_attrs(_Args... args) {
	return std::make_tuple(args...);
}

template <typename _Tuple,std::size_t... _Index>
inline constexpr auto make_runtime_fields(const _Tuple& tuple,std::index_sequence<_Index...>) noexcept {
	return std::array<field,sizeof...(_Index)>{std::get<_Index>(tuple).make_runtime()...};
}

template <typename _Tuple,std::size_t... _Index>
inline constexpr auto make_runtime_methods(const _Tuple& tuple,std::index_sequence<_Index...>) noexcept {
	return std::array<method,sizeof...(_Index)>{std::get<_Index>(tuple).make_runtime()...};
}

template <typename _Tuple,std::size_t... _Index>
inline constexpr auto make_runtime_ctors(const _Tuple& tuple,std::index_sequence<_Index...>) noexcept {
	return std::array<constructor,sizeof...(_Index)>{std::get<_Index>(tuple).make_runtime()...};
}

template <typename _Tuple,std::size_t... _Index>
inline constexpr auto make_runtime_bases(const _Tuple&,std::index_sequence<_Index...>) noexcept {
	return std::array<base,sizeof...(_Index)>{std::tuple_element<_Index,_Tuple>::type::make_runtime()...};
}

template <typename _Tuple,std::size_t... _Index>
inline constexpr auto make_runtime_attrs(const _Tuple& tuple,std::index_sequence<_Index...>) {
	return std::array<attribute,sizeof...(_Index)>{std::get<_Index>(tuple).make_runtime()...};
}

template <typename _Tuple,std::size_t... _Index>
inline constexpr auto make_runtime_enum_values(const _Tuple& tuple,std::index_sequence<_Index...>) noexcept {
	return std::array<enum_value,sizeof...(_Index)>{std::get<_Index>(tuple).make_runtime()...};
}

template <typename _Member,typename _Tp>
_Member& get(_Tp& obj,const field& f) noexcept {
	return *static_cast<_Member*>(f.get(static_cast<void*>(&obj)));
}

template <typename _Member,typename _Tp>
const _Member& get(const _Tp& obj,const field& f) noexcept {
	return *static_cast<const _Member*>(f.get(static_cast<const void*>(&obj)));
}

inline constexpr const field* find_field(const type& t,std::string_view name) noexcept {
	for (const auto& v:t.fields()) {
		if (v.name()==name) return &v;
	}
	return nullptr;
}

inline constexpr const method* find_method(const type& t,std::string_view name) noexcept {
	for (const auto& v:t.methods()) {
		if (v.name()==name) return &v;
	}
	return nullptr;
}

inline constexpr bool method_parameter_types_equal(const method& m,const type* types,std::size_t count) noexcept {
	if (m.parameter_count()!=count) return false;
	for (std::size_t i=0;i<count;i++) {
		if (m.parameter_types()[i]!=types[i]) return false;
	}
	return true;
}

inline constexpr const method* find_method(const type& t,std::string_view name,const type& return_type,const type* parameter_types,std::size_t parameter_count) noexcept {
	for (const auto& v:t.methods()) {
		if (v.name()==name && v.return_type()==return_type && method_parameter_types_equal(v,parameter_types,parameter_count)) return &v;
	}
	return nullptr;
}

inline constexpr const method* find_method(const type& t,std::string_view name,std::size_t parameter_count) noexcept {
	for (const auto& v:t.methods()) {
		if (v.name()==name && v.parameter_count()==parameter_count) return &v;
	}
	return nullptr;
}

inline constexpr const constructor* find_constructor(const type& t,std::size_t parameter_count) noexcept {
	for (const auto& v:t.constructors()) {
		if (v.parameter_count()==parameter_count) return &v;
	}
	return nullptr;
}

inline constexpr bool constructor_parameter_types_equal(const constructor& c,const type* types,std::size_t count) noexcept {
	if (c.parameter_count()!=count) return false;
	for (std::size_t i=0;i<count;i++) {
		if (c.parameter_types()[i]!=types[i]) return false;
	}
	return true;
}

inline constexpr const constructor* find_constructor(const type& t,const type* parameter_types,std::size_t parameter_count) noexcept {
	for (const auto& v:t.constructors()) {
		if (constructor_parameter_types_equal(v,parameter_types,parameter_count)) return &v;
	}
	return nullptr;
}

inline constexpr const enum_value* find_enum_value(const type& t,std::string_view name) noexcept {
	for (const auto& v:t.enum_values()) {
		if (v.name()==name) return &v;
	}
	return nullptr;
}

inline constexpr const attribute* find_attribute(attribute_list attrs,std::string_view name) noexcept {
	for (const auto& v:attrs) {
		if (v.name()==name) return &v;
	}
	return nullptr;
}

inline constexpr bool has_attribute(attribute_list attrs,std::string_view name) noexcept {
	return find_attribute(attrs,name)!=nullptr;
}

template <typename... _Types>
inline constexpr bool field_value_type_is_any_of(const field& f) noexcept {
	return ((f.value_type()==reflect<_Types>()) || ...);
}

template <typename... _ReturnTypes>
inline constexpr bool method_return_type_is_any_of(const method& m) noexcept {
	return ((m.return_type()==reflect<_ReturnTypes>()) || ...);
}

template <typename... _BaseTypes>
inline constexpr bool base_type_is_any_of(const base& b) noexcept {
	return ((b.value_type()==reflect<_BaseTypes>()) || ...);
}

inline constexpr bool constructor_arity_is(const constructor& c,std::size_t parameter_count) noexcept {
	return c.parameter_count()==parameter_count;
}

template <typename _Tp,_Tp... _Values>
inline constexpr bool enum_value_is_any_of(const enum_value& v) noexcept {
	static_assert(std::is_enum_v<_Tp>,"_Tp must be an enum type.");

	return ((v.value()==static_cast<long long>(_Values)) || ...);
}

template <typename _Tp>
const _Tp* attribute_value(const attribute& a) noexcept {
	return std::any_cast<_Tp>(&a.value_);
}

template <typename _Tp,typename _Func>
void for_each_field(_Func&& func) {
	static_assert(std::is_invocable<_Func&,const field&>::value,"_Func must be invocable with(const field&).");

	const auto t=reflect<_Tp>();
	for (const auto& v:t.fields()) func(v);
}

template <typename _Tp,typename... _Types,typename _Func>
void for_each_field_of(_Func&& func) {
	const auto t=reflect<_Tp>();
	for (const auto& v:t.fields()) {
		if (field_value_type_is_any_of<_Types...>(v)) func(v);
	}
}

template <typename _Tp,typename _Func>
void for_each_method(_Func&& func) {
	static_assert(std::is_invocable<_Func&,const method&>::value,"_Func must be invocable with(const method&).");

	const auto t=reflect<_Tp>();
	for (const auto& v:t.methods()) func(v);
}

template <typename _Tp,typename _Func>
void for_each_const_method(_Func&& func) {
	static_assert(std::is_invocable<_Func&,const method&>::value,"_Func must be invocable with(const method&).");

	const auto t=reflect<_Tp>();
	for (const auto& v:t.methods()) {
		if(v.is_const()) func(v);
	}
}

template <typename _Tp,typename _Func>
void for_each_static_method(_Func&& func) {
	static_assert(std::is_invocable<_Func&,const method&>::value,"_Func must be invocable with(const method&).");

	const auto t=reflect<_Tp>();
	for (const auto& v:t.methods()) {
		if (v.is_static()) func(v);
	}
}

template <typename _Tp,typename... _ReturnTypes,typename _Func>
void for_each_method_of_return(_Func&& func) {
	const auto t=reflect<_Tp>();
	for (const auto& v:t.methods()) {
		if (((v.return_type()==reflect<_ReturnTypes>()) || ...)) func(v);
	}
}

template <typename _Tp,typename _Func>
void for_each_method_of_arity(std::size_t parameter_count,_Func&& func) {
	const auto t=reflect<_Tp>();
	for (const auto& v:t.methods()) {
		if (v.parameter_count()==parameter_count) func(v);
	}
}

template <typename _Tp,typename _Func>
void for_each_method_view(_Tp& obj,_Func&& func) {
	visit_methods(obj,std::forward<_Func>(func));
}

template <typename _Tp,typename _Func>
void for_each_base(_Func&& func) {
	static_assert(std::is_invocable<_Func&,const base&>::value,"_Func must be invocable with(const base&).");

	const auto t=reflect<_Tp>();
	for (const auto& v:t.bases()) func(v);
}

template <typename _Tp,typename..._BaseTypes,typename _Func>
void for_each_base_of(_Func&& func) {
	const auto t=reflect<_Tp>();
	for (const auto& v:t.bases()) {
		if (((v.value_type()==reflect<_BaseTypes>()) || ...)) func(v);
	}
}

template <typename _Tp,typename _Func>
void for_each_virtual_base(_Func&& func) {
	const auto t=reflect<_Tp>();
	for (const auto& v:t.bases()) {
		if (v.is_virtual()) func(v);
	}
}

template <typename _Tp,typename _Func>
void for_each_base_view(_Tp& obj,_Func&& func) {
	visit_bases(obj,std::forward<_Func>(func));
}

template <typename _Tp,typename _Func>
void for_each_constructor(_Func&& func) {
	static_assert(std::is_invocable<_Func&,const constructor&>::value,"_Func must be invocable with(const constructor&).");

	const auto t=reflect<_Tp>();
	for (const auto& v:t.constructors()) func(v);
}

template <typename _Tp,typename _Func>
void for_each_constructor_of_arity(std::size_t parameter_count,_Func&& func) {
	const auto t=reflect<_Tp>();
	for (const auto& v:t.constructors()) {
		if (constructor_arity_is(v,parameter_count)) func(v);
	}
}

template <typename _Tp,typename _Func>
void for_each_enum_value(_Func&& func) {
	static_assert(std::is_invocable<_Func&,const enum_value&>::value,"_Func must be invocable with(const enum_value&).");

	const auto t=reflect<_Tp>();
	for (const auto& v:t.enum_values()) func(v);
}

template <typename _Tp,typename _Enum,_Enum..._Values,typename _Func>
void for_each_enum_value_of(_Func&& func) {
	const auto t=reflect<_Tp>();
	for (const auto& v:t.enum_values()) {
		if (enum_value_is_any_of<_Enum,_Values...>(v)) func(v);
	}
}

template <typename _Tp,typename _Func>
void for_each_named_enum_value(std::string_view name,_Func&& func) {
	const auto t=reflect<_Tp>();
	for (const auto& v:t.enum_values()) {
		if (v.name()==name) func(v);
	}
}

template <typename _Tp,typename _Func>
void visit_fields(_Tp& obj,_Func&& func) {
	static_assert(std::is_invocable<_Func&,const field&,void*>::value,"_Func must be invocable with(const field&,void*).");

	const auto t=reflect<_Tp>();
	for (const auto& v:t.fields()) func(v,v.get(static_cast<void*>(&obj)));
}

template <typename _Tp,typename... _Types,typename _Func>
void visit_fields_of(_Tp& obj,_Func&& func) {
	const auto t=reflect<_Tp>();
	for (const auto& v:t.fields()) {
		if (field_value_type_is_any_of<_Types...>(v)) func(v,v.get(static_cast<void*>(&obj)));
	}
}

template <typename _Tp,typename _Func>
void visit_fields(const _Tp& obj,_Func&& func) {
	static_assert(std::is_invocable<_Func&,const field&,const void*>::value,"_Func must be invocable with(const field&,const void*).");

	const auto t=reflect<_Tp>();
	for (const auto& v:t.fields()) func(v,v.get(static_cast<const void*>(&obj)));
}

template <typename _Tp,typename... _Types,typename _Func>
void visit_fields_of(const _Tp& obj,_Func&& func) {
	const auto t=reflect<_Tp>();
	for (const auto& v:t.fields()) {
		if (field_value_type_is_any_of<_Types...>(v)) func(v,v.get(static_cast<const void*>(&obj)));
	}
}

template <typename _FieldType,typename _Func>
void invoke_typed_field_visitor(_Func&& func,const field& f,void* p) {
	func(f,*static_cast<_FieldType*>(p));
}

template <typename _FieldType,typename _Func>
void invoke_typed_field_visitor(_Func&& func,const field& f,const void* p) {
	func(f,*static_cast<const _FieldType*>(p));
}

template <typename... _Types,typename _Func>
bool try_invoke_typed_field_visitor(_Func&& func,const field& f,void* p) {
	bool matched=false;
	((f.value_type()==reflect<_Types>()?(invoke_typed_field_visitor<_Types>(std::forward<_Func>(func),f,p),matched=true,true):false) || ...);
	return matched;
}

template <typename... _Types,typename _Func>
bool try_invoke_typed_field_visitor(_Func&& func,const field& f,const void* p) {
	bool matched=false;
	((f.value_type()==reflect<_Types>()?(invoke_typed_field_visitor<_Types>(std::forward<_Func>(func),f,p),matched=true,true):false) || ...);
	return matched;
}

template <typename _Tp,typename... _Types,typename _Func>
void visit_fields_as(_Tp& obj,_Func&& func) {
	const auto t=reflect<_Tp>();
	for (const auto& v:t.fields()) {
		void* p=v.get(static_cast<void*>(&obj));
		try_invoke_typed_field_visitor<_Types...>(std::forward<_Func>(func),v,p);
	}
}

template <typename _Tp,typename... _Types,typename _Func>
void visit_fields_as(const _Tp& obj,_Func&& func) {
	const auto t=reflect<_Tp>();
	for (const auto& v:t.fields()) {
		const void* p=v.get(static_cast<const void*>(&obj));
		try_invoke_typed_field_visitor<_Types...>(std::forward<_Func>(func),v,p);
	}
}

template <typename _Tp,typename _Func>
void visit_methods(_Tp& obj,_Func&& func) {
	const auto t=reflect<_Tp>();
	for (const auto& v:t.methods()) func(v,static_cast<void*>(&obj));
}

template <typename _Tp,typename _Func>
void visit_methods(const _Tp& obj,_Func&& func) {
	const auto t=reflect<_Tp>();
	for (const auto& v:t.methods()) func(v,static_cast<const void*>(&obj));
}

template <typename _Tp,typename _Func>
void visit_const_methods(_Tp& obj,_Func&& func) {
	const auto t=reflect<_Tp>();
	for (const auto& v:t.methods()) {
		if (v.is_const()) func(v,static_cast<void*>(&obj));
	}
}

template <typename _Tp,typename _Func>
void visit_const_methods(const _Tp& obj,_Func&& func) {
	const auto t=reflect<_Tp>();
	for (const auto& v:t.methods()) {
		if (v.is_const()) func(v,static_cast<const void*>(&obj));
	}
}

template <typename _Tp,typename _Func>
void visit_static_methods(_Tp& obj,_Func&& func) {
	const auto t=reflect<_Tp>();
	for (const auto& v:t.methods()) {
		if (v.is_static()) func(v,static_cast<void*>(&obj));
	}
}

template <typename _Tp,typename _Func>
void visit_static_methods(const _Tp& obj,_Func&& func) {
	const auto t=reflect<_Tp>();
	for (const auto& v:t.methods()) {
		if (v.is_static()) func(v,static_cast<const void*>(&obj));
	}
}

template <typename _Tp,typename..._ReturnTypes,typename _Func>
void visit_methods_of_return(_Tp& obj,_Func&& func) {
	const auto t=reflect<_Tp>();
	for (const auto& v:t.methods()) {
		if (((v.return_type()==reflect<_ReturnTypes>()) || ...)) func(v,static_cast<void*>(&obj));
	}
}

template <typename _Tp,typename..._ReturnTypes,typename _Func>
void visit_methods_of_return(const _Tp& obj,_Func&& func) {
	const auto t=reflect<_Tp>();
	for (const auto& v:t.methods()) {
		if (((v.return_type()==reflect<_ReturnTypes>()) || ...)) func(v,static_cast<const void*>(&obj));
	}
}

template <typename _Tp,typename _Func>
void visit_methods_of_arity(_Tp& obj,std::size_t parameter_count,_Func&& func) {
	const auto t=reflect<_Tp>();
	for (const auto& v:t.methods()) {
		if (v.parameter_count()==parameter_count) func(v,static_cast<void*>(&obj));
	}
}

template <typename _Tp,typename _Func>
void visit_methods_of_arity(const _Tp& obj,std::size_t parameter_count,_Func&& func) {
	const auto t=reflect<_Tp>();
	for (const auto& v:t.methods()) {
		if (v.parameter_count()==parameter_count) func(v,static_cast<const void*>(&obj));
	}
}

template <typename _Tp,typename _Func>
void visit_bases(_Tp& obj,_Func&& func) {
	const auto t=reflect<_Tp>();
	for (const auto& v:t.bases()) func(v,v.get(static_cast<void*>(&obj)));
}

template <typename _Tp,typename _Func>
void visit_bases(const _Tp& obj,_Func&& func) {
	const auto t=reflect<_Tp>();
	for (const auto& v:t.bases()) func(v,v.get(static_cast<const void*>(&obj)));
}

template <typename _Tp,typename..._BaseTypes,typename _Func>
void visit_bases_of(_Tp& obj,_Func&& func) {
	const auto t=reflect<_Tp>();
	for (const auto& v:t.bases()) {
		if (((v.value_type()==reflect<_BaseTypes>()) || ...)) func(v,v.get(static_cast<void*>(&obj)));
	}
}

template <typename _Tp,typename..._BaseTypes,typename _Func>
void visit_bases_of(const _Tp& obj,_Func&& func) {
	const auto t=reflect<_Tp>();
	for (const auto& v:t.bases()) {
		if (((v.value_type()==reflect<_BaseTypes>()) || ...)) func(v,v.get(static_cast<const void*>(&obj)));
	}
}

template <typename _Tp>
std::string_view enum_to_string(_Tp value) noexcept {
	static_assert(std::is_enum<_Tp>::value,"_Tp must be an enum type.");
	const auto t=reflect<_Tp>();
	for (const auto& v:t.enum_values()) {
		if (v.value()==static_cast<long long>(value)) return v.name();
	}
	return std::string_view{};
}

template <typename _Tp>
bool enum_from_string(std::string_view name,_Tp& value) noexcept {
	static_assert(std::is_enum<_Tp>::value,"_Tp must be an enum type.");
	const auto t=reflect<_Tp>();
	for (const auto& v:t.enum_values()) {
		if (v.name()==name) {
			value=static_cast<_Tp>(v.value());
			return true;
		}
	}
	return false;
}

template <typename _Tp>
bool enum_is_valid(_Tp value) noexcept {
	static_assert(std::is_enum<_Tp>::value,"_Tp must be an enum type.");
	const auto t=reflect<_Tp>();
	for (const auto& v:t.enum_values()) {
		if (v.value()==static_cast<long long>(value)) return true;
	}
	return false;
}

template <typename _Tp>
std::any get_field_any(const _Tp& obj,const field& f) {
	return f.get_any(static_cast<const void*>(&obj));
}

template <typename _Tp>
bool set_field_any(_Tp& obj,const field& f,const std::any& value) noexcept {
	return f.set(static_cast<void*>(&obj),value);
}

template <typename _Tp>
std::any invoke_method_any(_Tp& obj,const method& m,const std::initializer_list<std::any>& args) {
	return m.invoke(static_cast<void*>(&obj),args.begin(),args.size());
}

template <typename _Tp>
std::any invoke_method_any(const _Tp& obj,const method& m,const std::initializer_list<std::any>& args) {
	return m.invoke(static_cast<const void*>(&obj),args.begin(),args.size());
}

inline std::any invoke_static_method_any(const method& m,const std::initializer_list<std::any>& args) {
	return m.invoke(static_cast<void*>(nullptr),args.begin(),args.size());
}

template <typename _Tp>
std::any invoke_by_name(_Tp& obj,std::string_view name,const std::initializer_list<std::any>& args) {
	const auto t=reflect<_Tp>();
	const method* m=find_method(t,name,args.size());
	if (!m) throw std::runtime_error("method not found");
	return m->invoke(static_cast<void*>(&obj),args.begin(),args.size());
}

template <typename _Tp>
std::any invoke_by_name(const _Tp& obj,std::string_view name,const std::initializer_list<std::any>& args) {
	const auto t=reflect<_Tp>();
	const method* m=find_method(t,name,args.size());
	if (!m) throw std::runtime_error("method not found");
	return m->invoke(static_cast<const void*>(&obj),args.begin(),args.size());
}

template <typename _Tp>
bool set_field_by_name(_Tp& obj,std::string_view name,const std::any& value) noexcept {
	const auto t=reflect<_Tp>();
	const field* f=find_field(t,name);
	return f?f->set(static_cast<void*>(&obj),value):false;
}

template <typename _Tp>
std::any get_field_by_name(const _Tp& obj,std::string_view name) {
	const auto t=reflect<_Tp>();
	const field* f=find_field(t,name);
	return f?f->get_any(static_cast<const void*>(&obj)):std::any{};
}

template <typename _Tp>
std::any construct_by_arity(std::size_t parameter_count,const std::initializer_list<std::any>& args) {
	const auto t=reflect<_Tp>();
	const constructor* c=find_constructor(t,parameter_count);
	if (!c) throw std::runtime_error("constructor not found");
	return c->create(args.begin(),args.size());
}

#define _STDEX_DEFINE_FUNDAMENTAL_DESCRIPTOR_SINGLE(_type,name) \
template <> \
struct descriptor<_type> { \
	static constexpr bool reflectable=true; \
	static constexpr bool class_reflectable=false; \
	static constexpr bool enum_reflectable=false; \
	static constexpr type get() noexcept { \
		return type{name,TK_FUNDAMENTAL,field_list{nullptr,0},method_list{nullptr,0},constructor_list{nullptr,0},base_list{nullptr,0},enum_value_list{nullptr,0},attribute_list{nullptr,0},sizeof(_type),alignof(_type),false,false,false,std::is_pointer<_type>::value,std::is_array<_type>::value,std::is_default_constructible<_type>::value,std::is_copy_constructible<_type>::value,std::is_move_constructible<_type>::value,std::is_copy_assignable<_type>::value,std::is_move_assignable<_type>::value}; \
	} \
};

#define _STDEX_DEFINE_FUNDAMENTAL_DESCRIPTOR(_type,name) \
_STDEX_DEFINE_FUNDAMENTAL_DESCRIPTOR_SINGLE(_type,name) \
_STDEX_DEFINE_FUNDAMENTAL_DESCRIPTOR_SINGLE(const _type,"const " name) \
_STDEX_DEFINE_FUNDAMENTAL_DESCRIPTOR_SINGLE(_type&,name"&") \
_STDEX_DEFINE_FUNDAMENTAL_DESCRIPTOR_SINGLE(const _type&,"const " name"&")

_STDEX_DEFINE_FUNDAMENTAL_DESCRIPTOR(bool,"bool")
_STDEX_DEFINE_FUNDAMENTAL_DESCRIPTOR(char,"char")
_STDEX_DEFINE_FUNDAMENTAL_DESCRIPTOR(signed char,"signed char")
_STDEX_DEFINE_FUNDAMENTAL_DESCRIPTOR(unsigned char,"unsigned char")
_STDEX_DEFINE_FUNDAMENTAL_DESCRIPTOR(short,"short")
_STDEX_DEFINE_FUNDAMENTAL_DESCRIPTOR(unsigned short,"unsigned short")
_STDEX_DEFINE_FUNDAMENTAL_DESCRIPTOR(int,"int")
_STDEX_DEFINE_FUNDAMENTAL_DESCRIPTOR(unsigned int,"unsigned int")
_STDEX_DEFINE_FUNDAMENTAL_DESCRIPTOR(long,"long")
_STDEX_DEFINE_FUNDAMENTAL_DESCRIPTOR(unsigned long,"unsigned long")
_STDEX_DEFINE_FUNDAMENTAL_DESCRIPTOR(long long,"long long")
_STDEX_DEFINE_FUNDAMENTAL_DESCRIPTOR(unsigned long long,"unsigned long long")
_STDEX_DEFINE_FUNDAMENTAL_DESCRIPTOR(float,"float")
_STDEX_DEFINE_FUNDAMENTAL_DESCRIPTOR(double,"double")
_STDEX_DEFINE_FUNDAMENTAL_DESCRIPTOR(long double,"long double")
_STDEX_DEFINE_FUNDAMENTAL_DESCRIPTOR(wchar_t,"wchar_t")
_STDEX_DEFINE_FUNDAMENTAL_DESCRIPTOR(char16_t,"char16_t")
_STDEX_DEFINE_FUNDAMENTAL_DESCRIPTOR(char32_t,"char32_t")
#if __cplusplus>=_STDEX_CPP20_VERSION
_STDEX_DEFINE_FUNDAMENTAL_DESCRIPTOR(char8_t,"char8_t")
#endif

template <>
struct descriptor<std::nullptr_t> {
	static constexpr bool reflectable=true;
	static constexpr bool class_reflectable=false;
	static constexpr bool enum_reflectable=false;
	static constexpr type get() noexcept {
		return type{"nullptr_t",TK_FUNDAMENTAL,{nullptr,0},{nullptr,0},{nullptr,0},{nullptr,0},{nullptr,0},{nullptr,0},sizeof(std::nullptr_t),alignof(std::nullptr_t),false,false,false,false,false,true,true,true,true,true};
	}
};

template <>
struct descriptor<std::string> {
	static constexpr bool reflectable=true;
	static constexpr bool class_reflectable=false;
	static constexpr bool enum_reflectable=false;
	static constexpr type get() noexcept {
		return type{"std::string",TK_CLASS,{nullptr,0},{nullptr,0},{nullptr,0},{nullptr,0},{nullptr,0},{nullptr,0},sizeof(std::string),alignof(std::string),false,false,false,false,false,std::is_default_constructible<std::string>::value,std::is_copy_constructible<std::string>::value,std::is_move_constructible<std::string>::value,std::is_copy_assignable<std::string>::value,std::is_move_assignable<std::string>::value};
	}
};

template <>
struct descriptor<std::string_view> {
	static constexpr bool reflectable=true;
	static constexpr bool class_reflectable=false;
	static constexpr bool enum_reflectable=false;
	static constexpr type get() noexcept {
		return type{"std::string_view",TK_CLASS,{nullptr,0},{nullptr,0},{nullptr,0},{nullptr,0},{nullptr,0},{nullptr,0},sizeof(std::string_view),alignof(std::string_view),false,false,false,false,false,std::is_default_constructible<std::string_view>::value,std::is_copy_constructible<std::string_view>::value,std::is_move_constructible<std::string_view>::value,std::is_copy_assignable<std::string_view>::value,std::is_move_assignable<std::string_view>::value};
	}
};

template <typename _Tp>
struct descriptor<_Tp*> {
	static constexpr bool reflectable=true;
	static constexpr bool class_reflectable=false;
	static constexpr bool enum_reflectable=false;
	static constexpr type get() noexcept {
		return type{"pointer",TK_FUNDAMENTAL,{nullptr,0},{nullptr,0},{nullptr,0},{nullptr,0},{nullptr,0},{nullptr,0},sizeof(_Tp*),alignof(_Tp*),false,false,false,true,false,true,true,true,true,true};
	}
};

#if __cplusplus>=_STDEX_CPP20_VERSION
template <typename _Tp,std::size_t _Extent>
struct descriptor<std::span<_Tp,_Extent>> {
	static constexpr bool reflectable=true;
	static constexpr bool class_reflectable=false;
	static constexpr bool enum_reflectable=false;
	static constexpr type get() noexcept {
		return type{"std::span",TK_CLASS,{nullptr,0},{nullptr,0},{nullptr,0},{nullptr,0},{nullptr,0},{nullptr,0},sizeof(std::span<_Tp,_Extent>),alignof(std::span<_Tp,_Extent>),false,false,false,false,false,std::is_default_constructible<std::span<_Tp,_Extent>>::value,std::is_copy_constructible<std::span<_Tp,_Extent>>::value,std::is_move_constructible<std::span<_Tp,_Extent>>::value,std::is_copy_assignable<std::span<_Tp,_Extent>>::value,std::is_move_assignable<std::span<_Tp,_Extent>>::value};
	}
};
#endif

}

}

}

#define _STDEX_REFLECT_ATTR(name,value) stdex::meta::reflect::make_attribute(#name,value)
#define _STDEX_REFLECT_ATTRS(...) stdex::meta::reflect::make_attributes(__VA_ARGS__)
#define _STDEX_REFLECT_FIELD(name) stdex::meta::reflect::make_field<stdex_meta_self,decltype(stdex_meta_self::name),&stdex_meta_self::name>(#name)
#define _STDEX_REFLECT_FIELD_ATTR(name,attrs) stdex::meta::reflect::make_field<stdex_meta_self,decltype(stdex_meta_self::name),&stdex_meta_self::name>(#name,attrs)
#define _STDEX_REFLECT_FIELD_EX(name,access_v,const_v) stdex::meta::reflect::make_field<stdex_meta_self,decltype(stdex_meta_self::name),&stdex_meta_self::name,stdex::meta::reflect::access_kind::access_v,const_v>(#name)
#define _STDEX_REFLECT_FIELD_EX_ATTR(name,access_v,const_v,attrs) stdex::meta::reflect::make_field<stdex_meta_self,decltype(stdex_meta_self::name),&stdex_meta_self::name,stdex::meta::reflect::access_kind::access_v,const_v>(#name,attrs)
#define _STDEX_REFLECT_STATIC_FIELD(name) stdex::meta::reflect::make_static_field<stdex_meta_self,decltype(stdex_meta_self::name),&stdex_meta_self::name>(#name)
#define _STDEX_REFLECT_STATIC_FIELD_ATTR(name,attrs) stdex::meta::reflect::make_static_field<stdex_meta_self,decltype(stdex_meta_self::name),&stdex_meta_self::name>(#name,attrs)
#define _STDEX_REFLECT_STATIC_FIELD_EX(name,access_v,const_v) stdex::meta::reflect::make_static_field<stdex_meta_self,decltype(stdex_meta_self::name),&stdex_meta_self::name,stdex::meta::reflect::access_kind::access_v,const_v>(#name)
#define _STDEX_REFLECT_STATIC_FIELD_EX_ATTR(name,access_v,const_v,attrs) stdex::meta::reflect::make_static_field<stdex_meta_self,decltype(stdex_meta_self::name),&stdex_meta_self::name,stdex::meta::reflect::access_kind::access_v,const_v>(#name,attrs)
#define _STDEX_REFLECT_BASE(base_type,access_v,virtual_v) stdex::meta::reflect::make_base<stdex_meta_self,base_type,stdex::meta::reflect::access_kind::access_v,virtual_v>()
#define _STDEX_REFLECT_METHOD(name) stdex::meta::reflect::make_method<&stdex_meta_self::name>(#name)
#define _STDEX_REFLECT_METHOD_ATTR(name,attrs) stdex::meta::reflect::make_method<&stdex_meta_self::name>(#name,attrs)
#define _STDEX_REFLECT_METHOD_EX(name,access_v) stdex::meta::reflect::make_method<&stdex_meta_self::name,stdex::meta::reflect::access_kind::access_v>(#name)
#define _STDEX_REFLECT_METHOD_EX_ATTR(name,access_v,attrs) stdex::meta::reflect::make_method<&stdex_meta_self::name,stdex::meta::reflect::access_kind::access_v>(#name,attrs)
#define _STDEX_REFLECT_STATIC_METHOD(func) stdex::meta::reflect::make_method<&func>(#func)
#define _STDEX_REFLECT_STATIC_METHOD_ATTR(func,attrs) stdex::meta::reflect::make_method<&func>(#func,attrs)
#define _STDEX_REFLECT_CTOR0() stdex::meta::reflect::make_constructor<stdex_meta_self,stdex::meta::reflect::AK_PUBLIC>()
#if __cplusplus>=_STDEX_CPP20_VERSION
	#define _STDEX_REFLECT_CTOR(...) stdex::meta::reflect::make_constructor<stdex_meta_self,stdex::meta::reflect::AK_PUBLIC __VA_OPT__(,) __VA_ARGS__>()
#else
	#define _STDEX_REFLECT_CTOR(...) stdex::meta::reflect::make_constructor<stdex_meta_self,stdex::meta::reflect::AK_PUBLIC,__VA_ARGS__>()
#endif
#define _STDEX_REFLECT_CTOR_ATTR(attrs,...) stdex::meta::reflect::make_constructor<stdex_meta_self,stdex::meta::reflect::AK_PUBLIC,__VA_ARGS__>(attrs)
#define _STDEX_REFLECT_FIELDS(...) std::make_tuple(__VA_ARGS__)
#define _STDEX_REFLECT_METHODS(...) std::make_tuple(__VA_ARGS__)
#define _STDEX_REFLECT_CTORS(...) std::make_tuple(__VA_ARGS__)
#define _STDEX_REFLECT_BASES(...) std::make_tuple(__VA_ARGS__)
#define _STDEX_REFLECT_ATTRIBUTES(...) std::make_tuple(__VA_ARGS__)
#define _STDEX_REFLECT_REGISTER_CLASS_EX(_type,fields_pack,methods_pack,ctors_pack,bases_pack,attrs_pack) \
template <> \
struct stdex::meta::reflect::descriptor<_type> { \
private: \
	using stdex_meta_self=_type; \
	static inline auto fields_desc_=fields_pack; \
	static inline auto methods_desc_=methods_pack; \
	static inline auto ctors_desc_=ctors_pack; \
	static inline auto bases_desc_=bases_pack; \
	static inline auto attrs_desc_=attrs_pack; \
	static inline auto fields_=stdex::meta::reflect::make_runtime_fields(fields_desc_,std::make_index_sequence<std::tuple_size<decltype(fields_desc_)>::value>{}); \
	static inline auto methods_=stdex::meta::reflect::make_runtime_methods(methods_desc_,std::make_index_sequence<std::tuple_size<decltype(methods_desc_)>::value>{}); \
	static inline auto ctors_=stdex::meta::reflect::make_runtime_ctors(ctors_desc_,std::make_index_sequence<std::tuple_size<decltype(ctors_desc_)>::value>{}); \
	static inline auto bases_=stdex::meta::reflect::make_runtime_bases<decltype(bases_desc_)>(bases_desc_,std::make_index_sequence<std::tuple_size<decltype(bases_desc_)>::value>{}); \
	static inline auto attrs_=stdex::meta::reflect::make_runtime_attrs(attrs_desc_,std::make_index_sequence<std::tuple_size<decltype(attrs_desc_)>::value>{}); \
public: \
	static constexpr bool reflectable=true; \
	static constexpr bool class_reflectable=true; \
	static constexpr bool enum_reflectable=false; \
	static stdex::meta::reflect::type get() noexcept { \
		return stdex::meta::reflect::type{#_type,stdex::meta::reflect::TK_CLASS,{fields_.data(),fields_.size()},{methods_.data(),methods_.size()},{ctors_.data(),ctors_.size()},{bases_.data(),bases_.size()},{nullptr,0},{attrs_.data(),attrs_.size()},sizeof(_type),alignof(_type),false,false,false,false,false,std::is_default_constructible<_type>::value,std::is_copy_constructible<_type>::value,std::is_move_constructible<_type>::value,std::is_copy_assignable<_type>::value,std::is_move_assignable<_type>::value}; \
	} \
};

#define _STDEX_REFLECT_ENUM_REGISTER_ONE(name) stdex::meta::reflect::make_enum_value<stdex_meta_self,stdex_meta_self::name>(#name),
#define _STDEX_REFLECT_ENUM_REGISTER_ONE_ATTR(name,attrs) stdex::meta::reflect::make_enum_value<stdex_meta_self,stdex_meta_self::name>(#name,attrs),
#define _STDEX_REFLECT_REGISTER_ENUM(_type,items_macro) \
template <> \
struct stdex::meta::reflect::descriptor<_type> { \
private: \
	using stdex_meta_self=_type; \
	static inline auto enum_desc_=std::make_tuple(items_macro(_STDEX_REFLECT_ENUM_REGISTER_ONE)stdex::meta::reflect::make_enum_value<stdex_meta_self,static_cast<stdex_meta_self>(0)>("")); \
	template <std::size_t... _Index> \
	static std::array<stdex::meta::reflect::enum_value,sizeof...(_Index)> make_enum_values_(std::index_sequence<_Index...>) noexcept { \
		return std::array<stdex::meta::reflect::enum_value,sizeof...(_Index)>{std::get<_Index>(enum_desc_).make_runtime()...}; \
	} \
	static inline auto enum_values_=make_enum_values_(std::make_index_sequence<std::tuple_size<decltype(enum_desc_)>::value-1>{}); \
	static inline auto attrs_desc_=std::make_tuple(); \
	static inline auto attrs_=stdex::meta::reflect::make_runtime_attrs(attrs_desc_,std::make_index_sequence<0>{}); \
public: \
	static constexpr bool reflectable=true; \
	static constexpr bool class_reflectable=false; \
	static constexpr bool enum_reflectable=true; \
	static stdex::meta::reflect::type get() noexcept { \
		return stdex::meta::reflect::type{#_type,stdex::meta::reflect::TK_ENUM,{nullptr,0},{nullptr,0},{nullptr,0},{nullptr,0},{enum_values_.data(),enum_values_.size()},{attrs_.data(),attrs_.size()},sizeof(_type),alignof(_type),false,false,false,false,false,std::is_default_constructible<_type>::value,std::is_copy_constructible<_type>::value,std::is_move_constructible<_type>::value,std::is_copy_assignable<_type>::value,std::is_move_assignable<_type>::value}; \
	} \
};

#define _STDEX_REFLECT_REGISTER_ENUM_EX(_type,items_macro,attrs_pack) \
template <> \
struct stdex::meta::reflect::descriptor<_type> { \
private: \
	using stdex_meta_self=_type; \
	static inline auto enum_desc_=std::make_tuple(items_macro(_STDEX_REFLECT_ENUM_REGISTER_ONE)stdex::meta::reflect::make_enum_value<stdex_meta_self,static_cast<stdex_meta_self>(0)>("")); \
	template <std::size_t... _Index> \
	static std::array<stdex::meta::reflect::enum_value,sizeof...(_Index)> make_enum_values_(std::index_sequence<_Index...>) noexcept { \
		return std::array<stdex::meta::reflect::enum_value,sizeof...(_Index)>{std::get<_Index>(enum_desc_).make_runtime()...}; \
	} \
	static inline auto enum_values_=make_enum_values_(std::make_index_sequence<std::tuple_size<decltype(enum_desc_)>::value-1>{}); \
	static inline auto attrs_desc_=attrs_pack; \
	static inline auto attrs_=stdex::meta::reflect::make_runtime_attrs(attrs_desc_,std::make_index_sequence<std::tuple_size<decltype(attrs_desc_)>::value>{}); \
public: \
	static constexpr bool reflectable=true; \
	static constexpr bool class_reflectable=false; \
	static constexpr bool enum_reflectable=true; \
	static stdex::meta::reflect::type get() noexcept { \
		return stdex::meta::reflect::type{#_type,stdex::meta::reflect::TK_ENUM,{nullptr,0},{nullptr,0},{nullptr,0},{nullptr,0},{enum_values_.data(),enum_values_.size()},{attrs_.data(),attrs_.size()},sizeof(_type),alignof(_type),false,false,false,false,false,std::is_default_constructible<_type>::value,std::is_copy_constructible<_type>::value,std::is_move_constructible<_type>::value,std::is_copy_assignable<_type>::value,std::is_move_assignable<_type>::value}; \
	} \
};

#define _STDEX_REFLECT_ENUM_DECLARE_ONE(name,value) name=value,
#define _STDEX_REFLECT_ENUM_REFLECT_ONE(name,value) stdex::meta::reflect::make_enum_value<stdex_meta_self,stdex_meta_self::name>(#name),
#define _STDEX_REFLECT_ENUM(_type,base,items_macro) \
enum _type : base { \
	items_macro(_STDEX_REFLECT_ENUM_DECLARE_ONE) \
}; \
template <> \
struct stdex::meta::reflect::descriptor<_type> { \
private: \
	using stdex_meta_self=_type; \
	static inline auto enum_desc_=std::make_tuple(items_macro(_STDEX_REFLECT_ENUM_REFLECT_ONE)stdex::meta::reflect::make_enum_value<stdex_meta_self,static_cast<stdex_meta_self>(0)>("")); \
	template <std::size_t... _Index> \
	static std::array<stdex::meta::reflect::enum_value,sizeof...(_Index)> make_enum_values_(std::index_sequence<_Index...>) noexcept { \
		return std::array<stdex::meta::reflect::enum_value,sizeof...(_Index)>{ \
			std::get<_Index>(enum_desc_).make_runtime()... \
		}; \
	} \
	static inline auto enum_values_=make_enum_values_(std::make_index_sequence<std::tuple_size<decltype(enum_desc_)>::value-1>{}); \
	static inline auto attrs_desc_=std::make_tuple(); \
	static inline auto attrs_=stdex::meta::reflect::make_runtime_attrs(attrs_desc_,std::make_index_sequence<0>{}); \
public: \
	static constexpr bool reflectable=true; \
	static constexpr bool class_reflectable=false; \
	static constexpr bool enum_reflectable=true; \
	static stdex::meta::reflect::type get() noexcept { \
		return stdex::meta::reflect::type{#_type,stdex::meta::reflect::TK_ENUM,{nullptr,0},{nullptr,0},{nullptr,0},{nullptr,0},{enum_values_.data(),enum_values_.size()},{attrs_.data(),attrs_.size()},sizeof(_type),alignof(_type),false,false,false,false,false,std::is_default_constructible<_type>::value,std::is_copy_constructible<_type>::value,std::is_move_constructible<_type>::value,std::is_copy_assignable<_type>::value,std::is_move_assignable<_type>::value}; \
	} \
};

#define _STDEX_REFLECT_ENUM_DECLARE_ONE_EX(name) name,
#define _STDEX_REFLECT_ENUM_DECLARE_ONE_V_EX(name,value) name = value,
#define _STDEX_REFLECT_ENUM_REFLECT_ONE_EX(name) stdex::meta::reflect::make_enum_value<stdex_meta_self,stdex_meta_self::name>(#name),
#define _STDEX_REFLECT_ENUM_REFLECT_ONE_V_EX(name,value) stdex::meta::reflect::make_enum_value<stdex_meta_self,stdex_meta_self::name>(#name),
#define _STDEX_REFLECT_ENUM_EX(_type,base,items_macro) \
enum _type : base { \
	items_macro(_STDEX_REFLECT_ENUM_DECLARE_ONE_EX,_STDEX_REFLECT_ENUM_DECLARE_ONE_V_EX) \
}; \
template <> \
struct stdex::meta::reflect::descriptor<_type> { \
private: \
	using stdex_meta_self=_type; \
	static inline auto enum_desc_=std::make_tuple( \
		items_macro(_STDEX_REFLECT_ENUM_REFLECT_ONE_EX,_STDEX_REFLECT_ENUM_REFLECT_ONE_V_EX) \
		stdex::meta::reflect::make_enum_value<stdex_meta_self,static_cast<stdex_meta_self>(0)>("") \
	); \
	template <std::size_t... _Index> \
	static std::array<stdex::meta::reflect::enum_value,sizeof...(_Index)> make_enum_values_(std::index_sequence<_Index...>) noexcept { \
		return { std::get<_Index>(enum_desc_).make_runtime()... }; \
	} \
	static inline auto enum_values_=make_enum_values_( \
		std::make_index_sequence<std::tuple_size<decltype(enum_desc_)>::value-1>{} \
	); \
	static inline auto attrs_desc_=std::make_tuple(); \
	static inline auto attrs_=stdex::meta::reflect::make_runtime_attrs(attrs_desc_,std::make_index_sequence<0>{}); \
public: \
	static constexpr bool reflectable=true; \
	static constexpr bool class_reflectable=false; \
	static constexpr bool enum_reflectable=true; \
	static stdex::meta::reflect::type get() noexcept { \
		return stdex::meta::reflect::type{#_type,stdex::meta::reflect::TK_ENUM,{nullptr,0},{nullptr,0},{nullptr,0},{nullptr,0},{enum_values_.data(),enum_values_.size()},{attrs_.data(),attrs_.size()},sizeof(_type),alignof(_type),false,false,false,false,false,std::is_default_constructible<_type>::value,std::is_copy_constructible<_type>::value,std::is_move_constructible<_type>::value,std::is_copy_assignable<_type>::value,std::is_move_assignable<_type>::value}; \
	} \
};

#define _STDEX_REFLECT_ENUM_LIGHT_REFLECT_ONE(name,value) \
	stdex::meta::reflect::enum_value{#name,static_cast<long long>(stdex_meta_self::name),nullptr,0},
#define _STDEX_REFLECT_ENUM_LIGHT(_type,base,items_macro) \
enum _type : base { \
	items_macro(_STDEX_REFLECT_ENUM_DECLARE_ONE) \
}; \
template <> \
struct stdex::meta::reflect::descriptor<_type>; \
namespace stdex { namespace meta { namespace reflect { \
template <> \
struct descriptor<_type> { \
private: \
	using stdex_meta_self=_type; \
	static inline constexpr enum_value enum_values_[]={ \
		items_macro(_STDEX_REFLECT_ENUM_LIGHT_REFLECT_ONE) \
	}; \
	static inline constexpr std::size_t enum_count_=sizeof(enum_values_)/sizeof(enum_values_[0]); \
public: \
	static constexpr bool reflectable=true; \
	static constexpr bool class_reflectable=false; \
	static constexpr bool enum_reflectable=true; \
	static stdex::meta::reflect::type get() noexcept { \
		return stdex::meta::reflect::type{#_type,stdex::meta::reflect::TK_ENUM,{nullptr,0},{nullptr,0},{nullptr,0},{nullptr,0},{enum_values_,enum_count_},{nullptr,0},sizeof(_type),alignof(_type),false,false,false,false,false,std::is_default_constructible<_type>::value,std::is_copy_constructible<_type>::value,std::is_move_constructible<_type>::value,std::is_copy_assignable<_type>::value,std::is_move_assignable<_type>::value}; \
	} \
}; \
}}}

#define _STDEX_REFLECT_ENUM_EX_LIGHT_REFLECT_ONE_EX(name) \
	stdex::meta::reflect::enum_value{#name,static_cast<long long>(stdex_meta_self::name),nullptr,0},
#define _STDEX_REFLECT_ENUM_EX_LIGHT_REFLECT_ONE_V_EX(name,value) \
	stdex::meta::reflect::enum_value{#name,static_cast<long long>(stdex_meta_self::name),nullptr,0},
#define _STDEX_REFLECT_ENUM_EX_LIGHT(_type,base,items_macro) \
enum _type : base { \
	items_macro(_STDEX_REFLECT_ENUM_DECLARE_ONE_EX,_STDEX_REFLECT_ENUM_DECLARE_ONE_V_EX) \
}; \
template <> \
struct stdex::meta::reflect::descriptor<_type>; \
namespace stdex { namespace meta { namespace reflect { \
template <> \
struct descriptor<_type> { \
private: \
	using stdex_meta_self=_type; \
	static inline constexpr enum_value enum_values_[]={ \
		items_macro(_STDEX_REFLECT_ENUM_EX_LIGHT_REFLECT_ONE_EX,_STDEX_REFLECT_ENUM_EX_LIGHT_REFLECT_ONE_V_EX) \
	}; \
	static inline constexpr std::size_t enum_count_=sizeof(enum_values_)/sizeof(enum_values_[0]); \
public: \
	static constexpr bool reflectable=true; \
	static constexpr bool class_reflectable=false; \
	static constexpr bool enum_reflectable=true; \
	static stdex::meta::reflect::type get() noexcept { \
		return stdex::meta::reflect::type{#_type,stdex::meta::reflect::TK_ENUM,{nullptr,0},{nullptr,0},{nullptr,0},{nullptr,0},{enum_values_,enum_count_},{nullptr,0},sizeof(_type),alignof(_type),false,false,false,false,false,std::is_default_constructible<_type>::value,std::is_copy_constructible<_type>::value,std::is_move_constructible<_type>::value,std::is_copy_assignable<_type>::value,std::is_move_assignable<_type>::value}; \
	} \
}; \
}}}

#endif