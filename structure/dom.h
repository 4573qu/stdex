//Last Modified At 2026/09/03
//@Version 1.1.0.0
#ifndef _STDEX_STRUCTURE_DOM_H_
#define _STDEX_STRUCTURE_DOM_H_ 1

#include <algorithm>
#include <cctype>
#include <cerrno>
#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <functional>
#include <initializer_list>
#include <iostream>
#include <iterator>
#include <limits>
#include <map>
#include <memory>
#include <numeric>
#include <optional>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "../container/reference.h"//At Least 1.0
#include "../utility/kind.h"//At Least 2.0

#if __has_include("../macros/cpp_compiler.h")
#include "../macros/cpp_compiler.h"//At Least 1.0
#endif

#if __has_include("../macros/cpp_version.h")
#include "../macros/cpp_version.h"//At Least 1.0
#endif

#ifndef _STDEX_GNU_COMPILER
#if defined(__GNUC__)
#define _STDEX_GNU_COMPILER 1
#else
#define _STDEX_GNU_COMPILER 0
#endif
#endif
#ifndef _STDEX_CLANG_COMPILER
#if defined(__clang__)
#define _STDEX_CLANG_COMPILER 1
#else
#define _STDEX_CLANG_COMPILER 0
#endif
#endif

#ifndef _STDEX_RETURNS_NON_NULL
#if defined(_Ret_notnull_)
#define _STDEX_RETURNS_NON_NULL _Ret_notnull_
#else
#define _STDEX_RETURNS_NON_NULL
#endif
#endif

#ifndef _STDEX_CPP20_VERSION
#define _STDEX_CPP20_VERSION 202002L
#endif

#if __cplusplus>=_STDEX_CPP20_VERSION
#include <compare>
#endif

namespace stdex {

namespace structure {

_STDEX_KIND(dom_data_type,int,
	_STDEX_KIND_VALUE_AUTO(DDT_NULL)
	_STDEX_KIND_VALUE_AUTO(DDT_INT)
	_STDEX_KIND_VALUE_AUTO(DDT_FLOAT)
	_STDEX_KIND_VALUE_AUTO(DDT_BOOL)
	_STDEX_KIND_VALUE_AUTO(DDT_STRING)
	_STDEX_KIND_VALUE_AUTO(DDT_ARRAY)
	_STDEX_KIND_VALUE_AUTO(DDT_OBJECT)
)

enum dom_convert_policy : int {
	DCP_STRICT,
	DCP_DISCARD,
};

#define _STDEX_DOM_TPL_DEFAULT_DECLARATION \
template <typename _Int=std::ptrdiff_t,typename _Float=double,typename _Boolean=bool,typename _String=std::string, \
	template <typename _Tp,typename... _Args> \
	class _Array=std::vector, \
	template <typename _Tp,typename _Up,typename... _Args> \
	class _Object=std::map, \
	template <typename _Tp> \
	class _Allocator=std::allocator \
> 
#define _STDEX_DOM_TPL_DECLARATION template <typename _Int,typename _Float,typename _Boolean,typename _String, \
	template <typename _Tp,typename... _Args> \
	class _Array, \
	template <typename _Tp,typename _Up,typename... _Args> \
	class _Object, \
	template <typename _Tp> \
	class _Allocator \
>
#define _STDEX_DOM_DEF dom<_Int,_Float,_Boolean,_String,_Array,_Object,_Allocator>

_STDEX_DOM_TPL_DEFAULT_DECLARATION
class dom;

template <typename _RefType>
class dom_pointer;

template <typename _Tp>
class dom_iterator;

template <typename _IteratorType>
class dom_iteration_proxy;

template <typename _IteratorType>
class dom_iteration_proxy_value;

template <typename _Tp,typename=void>
struct is_dom : std::false_type { };
template <typename _Tp>
struct is_dom<_Tp,std::void_t<typename _Tp::dom_base_t>> : std::is_base_of<typename _Tp::dom_base_t,_Tp> { };

template <typename _Tp>
struct is_dom_pointer : std::false_type { };
template <typename _RefType>
struct is_dom_pointer<dom_pointer<_RefType>> : std::true_type { };

template <typename _Tp,typename=void>
struct dom_string_type {
	using type=_Tp;
};

template <typename _Tp>
struct dom_string_type<_Tp,std::enable_if_t<is_dom<_Tp>::value>> {
	using type=typename _Tp::string_t;
};

class primitive_iterator_t {
public:
	using difference_type=std::ptrdiff_t;

private:
	static constexpr difference_type begin_value_=0;
	static constexpr difference_type end_value_=begin_value_+1;
	difference_type it_=(std::numeric_limits<difference_type>::min)();

public:
	constexpr difference_type get_value() const noexcept { return it_; }
	void set_begin() noexcept { it_=begin_value_; }
	void set_end() noexcept { it_=end_value_; }
	constexpr bool is_begin() const noexcept { return it_==begin_value_; }
	constexpr bool is_end() const noexcept { return it_==end_value_; }

	friend constexpr bool operator ==(primitive_iterator_t lhs,primitive_iterator_t rhs) noexcept {
		return lhs.it_==rhs.it_;
	}
	friend constexpr bool operator !=(primitive_iterator_t lhs,primitive_iterator_t rhs) noexcept {
		return lhs.it_!=rhs.it_;
	}
	friend constexpr bool operator <(primitive_iterator_t lhs,primitive_iterator_t rhs) noexcept {
		return lhs.it_<rhs.it_;
	}
	friend constexpr bool operator <=(primitive_iterator_t lhs,primitive_iterator_t rhs) noexcept {
		return lhs.it_<=rhs.it_;
	}
	friend constexpr bool operator >(primitive_iterator_t lhs,primitive_iterator_t rhs) noexcept {
		return lhs.it_>rhs.it_;
	}
	friend constexpr bool operator >=(primitive_iterator_t lhs,primitive_iterator_t rhs) noexcept {
		return lhs.it_>=rhs.it_;
	}
	friend constexpr difference_type operator -(primitive_iterator_t lhs,primitive_iterator_t rhs) noexcept {
		return lhs.it_-rhs.it_;
	}

	primitive_iterator_t& operator ++() noexcept {
		it_++;
		return *this;
	}
	primitive_iterator_t operator ++(int)& noexcept {
		auto result=*this;
		it_++;
		return result;
	}
	primitive_iterator_t& operator --() noexcept {
		it_--;
		return *this;
	}
	primitive_iterator_t operator --(int)& noexcept {
		auto result=*this;
		it_--;
		return result;
	}
	primitive_iterator_t& operator +=(difference_type n) noexcept {
		it_+=n;
		return *this;
	}
	primitive_iterator_t& operator -=(difference_type n) noexcept {
		it_-=n;
		return *this;
	}
	primitive_iterator_t operator +(difference_type n) const noexcept {
		auto result=*this;
		result+=n;
		return result;
	}
	primitive_iterator_t operator -(difference_type n) const noexcept {
		auto result=*this;
		result-=n;
		return result;
	}
};

template <typename _Tp>
struct internal_iterator {
	typename _Tp::object_t::iterator object_iterator_{};
	typename _Tp::array_t::iterator array_iterator_{};
	primitive_iterator_t primitive_iterator_{};
};

template <typename _IteratorType>
class dom_iteration_proxy_value {
public:
	using difference_type=std::ptrdiff_t;
	using value_type=dom_iteration_proxy_value;
	using pointer=value_type*;
	using ref=value_type&;
	using iterator_category=std::input_iterator_tag;
	using string_t=typename std::remove_cv<typename std::remove_reference<decltype(std::declval<_IteratorType>().key())>::type>::type;

private:
	_IteratorType anchor_{};
	std::size_t array_index_=0;
	mutable std::size_t array_index_last_=(std::numeric_limits<std::size_t>::max)();
	mutable string_t array_index_str_{};
	string_t empty_str_{};

public:
	dom_iteration_proxy_value()=default;
	explicit dom_iteration_proxy_value(_IteratorType it,std::size_t array_index=0) noexcept(std::is_nothrow_move_constructible<_IteratorType>::value) : anchor_(std::move(it)) , array_index_(array_index) { }
	~dom_iteration_proxy_value()=default;

	dom_iteration_proxy_value(const dom_iteration_proxy_value&)=default;
	dom_iteration_proxy_value(dom_iteration_proxy_value&&) noexcept(std::is_nothrow_move_constructible<_IteratorType>::value)=default;

	dom_iteration_proxy_value& operator =(const dom_iteration_proxy_value&)=default;
	dom_iteration_proxy_value& operator =(dom_iteration_proxy_value&&) noexcept(std::is_nothrow_move_constructible<_IteratorType>::value &&  std::is_nothrow_move_assignable<_IteratorType>::value)=default;

	const dom_iteration_proxy_value& operator *() const noexcept { return *this; }
	dom_iteration_proxy_value& operator ++() {
		anchor_++;
		array_index_++;
		return *this;
	}
	dom_iteration_proxy_value operator ++(int)& {
		auto result=*this;
		++(*this);
		return result;
	}
	bool operator ==(const dom_iteration_proxy_value& other) const {
		return anchor_==other.anchor_;
	}
	bool operator !=(const dom_iteration_proxy_value& other) const {
		return anchor_!=other.anchor_;
	}

	const string_t& key() const {
		switch (anchor_.object_->type()) {
			case DDT_ARRAY: {
				if (array_index_!=array_index_last_) {
					const std::string index_string=std::to_string(array_index_);
					array_index_str_=string_t(index_string.begin(),index_string.end());
					array_index_last_=array_index_;
				}
				return array_index_str_;
			}
			case DDT_OBJECT: {
				return anchor_.key();
			}
			case DDT_NULL:
			case DDT_STRING:
			case DDT_BOOL:
			case DDT_INT:
			case DDT_FLOAT:
			default: {
				return empty_str_;
			}
		}
	}
	decltype(auto) value() const {
		return anchor_.value();
	}
};

template <typename _IteratorType>
class dom_iteration_proxy {
private:
	typename _IteratorType::pointer container_=nullptr;

public:
	dom_iteration_proxy()=default;
	explicit dom_iteration_proxy(typename _IteratorType::ref container) noexcept : container_(&container) { }
	~dom_iteration_proxy()=default;

	dom_iteration_proxy(const dom_iteration_proxy&)=default;
	dom_iteration_proxy(dom_iteration_proxy&&) noexcept=default;

	dom_iteration_proxy& operator =(const dom_iteration_proxy&)=default;
	dom_iteration_proxy& operator =(dom_iteration_proxy&&) noexcept=default;

	dom_iteration_proxy_value<_IteratorType> begin() const noexcept {
		return dom_iteration_proxy_value<_IteratorType>(container_->begin());
	}
	dom_iteration_proxy_value<_IteratorType> end() const noexcept {
		return dom_iteration_proxy_value<_IteratorType>(container_->end());
	}
};

template <std::size_t _Np,typename _IteratorType,std::enable_if_t<_Np==0,int> =0>
auto get(const dom_iteration_proxy_value<_IteratorType>& proxy_value)->decltype(proxy_value.key()) {
	return proxy_value.key();
}
template <std::size_t _Np,typename _IteratorType,std::enable_if_t<_Np==1,int> =0>
auto get(const dom_iteration_proxy_value<_IteratorType>& proxy_value)->decltype(proxy_value.value()) {
	return proxy_value.value();
}

template <typename _Tp>
class dom_iterator {
	using nonconst_dom_t=typename std::remove_const<_Tp>::type;

	static_assert(is_dom<nonconst_dom_t>::value,"dom_iterator only accepts (const) dom.");

	using other_dom_iterator=dom_iterator<typename std::conditional<std::is_const<_Tp>::value,nonconst_dom_t,const _Tp>::type>;
	friend other_dom_iterator;
	friend _Tp;
	friend nonconst_dom_t;
	friend dom_iteration_proxy<dom_iterator>;
	friend dom_iteration_proxy_value<dom_iterator>;
	using object_t=typename nonconst_dom_t::object_t;
	using array_t=typename nonconst_dom_t::array_t;

	static_assert(std::is_base_of<std::bidirectional_iterator_tag,typename std::iterator_traits<typename object_t::iterator>::iterator_category>::value &&  std::is_base_of<std::bidirectional_iterator_tag,typename std::iterator_traits<typename array_t::iterator>::iterator_category>::value,"dom_iterator assumes array and object type iterators satisfy the LegacyBidirectionalIterator named requirement.");

public:
	using iterator_category=std::bidirectional_iterator_tag;
	using value_type=typename _Tp::value_type;
	using difference_type=typename _Tp::difference_type;
	using pointer=typename std::conditional<std::is_const<_Tp>::value,typename _Tp::const_pointer,typename _Tp::pointer>::type;
	using ref=typename std::conditional<std::is_const<_Tp>::value,typename _Tp::const_ref,typename _Tp::ref>::type;
	using reference=ref;

private:
	pointer object_=nullptr;
	internal_iterator<nonconst_dom_t> it_ { };

	virtual void set_begin() noexcept {
		switch (object_->type()) {
			case DDT_OBJECT: it_.object_iterator_=object_->value().object->begin();break;
			case DDT_ARRAY: it_.array_iterator_=object_->value().array->begin();break;
			case DDT_NULL: it_.primitive_iterator_.set_end();break;
			case DDT_STRING:
			case DDT_BOOL:
			case DDT_INT:
			case DDT_FLOAT:
			default: it_.primitive_iterator_.set_begin();break;
		}
	}
	virtual void set_end() noexcept {
		switch (object_->type()) {
			case DDT_OBJECT: it_.object_iterator_=object_->value().object->end();break;
			case DDT_ARRAY: it_.array_iterator_=object_->value().array->end();break;
			case DDT_NULL:
			case DDT_STRING:
			case DDT_BOOL:
			case DDT_INT:
			case DDT_FLOAT:
			default: it_.primitive_iterator_.set_end();break;
		}
	}

	template <typename _Iterator>
	bool equal_to(const _Iterator& other) const {
		if (object_!=other.object_) throw std::invalid_argument("Cannot compare iterators of different containers");
		switch (object_->type()) {
			case DDT_OBJECT: return it_.object_iterator_==other.it_.object_iterator_;
			case DDT_ARRAY: return it_.array_iterator_==other.it_.array_iterator_;
			case DDT_NULL:
			case DDT_STRING:
			case DDT_BOOL:
			case DDT_INT:
			case DDT_FLOAT:
			default: return it_.primitive_iterator_==other.it_.primitive_iterator_;
		}
	}

public:
	dom_iterator()=default;
	explicit dom_iterator(pointer object) noexcept : object_(object) { }
	virtual ~dom_iterator()=default;

	dom_iterator(const dom_iterator&)=default;	
	dom_iterator(dom_iterator&&) noexcept=default;
	template <typename _Up,std::enable_if_t<std::is_const<_Tp>::value && std::is_same<_Up,nonconst_dom_t>::value,int> =0>
	dom_iterator(const dom_iterator<_Up>& other) noexcept : object_(other.object_) , it_(other.it_) { }

	dom_iterator& operator =(const dom_iterator&)=default;
	dom_iterator& operator =(dom_iterator&&) noexcept=default;
	template <typename _Up,std::enable_if_t<std::is_const<_Tp>::value && std::is_same<_Up,nonconst_dom_t>::value,int> =0>
	dom_iterator& operator =(const dom_iterator<_Up>& other) noexcept {
		object_=other.object_;
		it_=other.it_;
		return *this;
	}

	virtual ref operator *() const {
		switch (object_->type()) {
			case DDT_OBJECT: return it_.object_iterator_->second;
			case DDT_ARRAY: return *it_.array_iterator_;
			case DDT_NULL: throw std::runtime_error("Cannot get value");
			case DDT_STRING:
			case DDT_BOOL:
			case DDT_INT:
			case DDT_FLOAT:
			default: {
				if (it_.primitive_iterator_.is_begin()) return *object_;
				throw std::runtime_error("Cannot get value");
			}
		}
	}
	virtual pointer operator ->() const {
		switch (object_->type()) {
			case DDT_OBJECT: return &(it_.object_iterator_->second);
			case DDT_ARRAY: return &*it_.array_iterator_;
			case DDT_NULL:
			case DDT_STRING:
			case DDT_BOOL:
			case DDT_INT:
			case DDT_FLOAT:
			default: {
				if (it_.primitive_iterator_.is_begin()) return object_;
				throw std::runtime_error("Cannot get value");
			}
		}
	}
	virtual dom_iterator& operator ++() {
		switch (object_->type()) {
			case DDT_OBJECT: std::advance(it_.object_iterator_,1);break;
			case DDT_ARRAY: std::advance(it_.array_iterator_,1);break;
			case DDT_NULL:
			case DDT_STRING:
			case DDT_BOOL:
			case DDT_INT:
			case DDT_FLOAT:
			default: ++it_.primitive_iterator_;break;
		}
		return *this;
	}
	dom_iterator operator ++(int)& {
		auto result=*this;
		++(*this);
		return result;
	}
	virtual dom_iterator& operator --() {
		switch (object_->type()) {
			case DDT_OBJECT: std::advance(it_.object_iterator_,-1);break;
			case DDT_ARRAY: std::advance(it_.array_iterator_,-1);break;
			case DDT_NULL:
			case DDT_STRING:
			case DDT_BOOL:
			case DDT_INT:
			case DDT_FLOAT:
			default: --it_.primitive_iterator_;break;
		}
		return *this;
	}
	dom_iterator operator --(int)& {
		auto result=*this;
		--(*this);
		return result;
	}

	virtual bool operator ==(const dom_iterator& other) const {
		return equal_to(other);
	}
	virtual bool operator ==(const other_dom_iterator& other) const {
		return equal_to(other);
	}
	bool operator !=(const dom_iterator& other) const {
		return !operator ==(other);
	}
	bool operator !=(const other_dom_iterator& other) const {
		return !operator ==(other);
	}
	virtual bool operator <(const dom_iterator& other) const {
		if (object_!=other.object_) throw std::invalid_argument("Cannot compare iterators of different containers");
		switch (object_->type()) {
			case DDT_OBJECT: throw std::invalid_argument("Cannot compare orders of object iterators");
			case DDT_ARRAY: return it_.array_iterator_<other.it_.array_iterator_;
			case DDT_NULL:
			case DDT_STRING:
			case DDT_BOOL:
			case DDT_INT:
			case DDT_FLOAT:
			default: return it_.primitive_iterator_<other.it_.primitive_iterator_;
		}
	}
	bool operator <=(const dom_iterator& other) const {
		return !other.operator <(*this);
	}
	bool operator >(const dom_iterator& other) const {
		return !operator <=(other);
	}
	bool operator >=(const dom_iterator& other) const {
		return !operator <(other);
	}
	virtual dom_iterator& operator +=(difference_type i) {
		switch (object_->type()) {
			case DDT_OBJECT: throw std::invalid_argument("Cannot use offsets with object iterators");
			case DDT_ARRAY: std::advance(it_.array_iterator_,i);break;
			case DDT_NULL:
			case DDT_STRING:
			case DDT_BOOL:
			case DDT_INT:
			case DDT_FLOAT:
			default: it_.primitive_iterator_+=i;break;
		}
		return *this;
	}
	dom_iterator& operator -=(difference_type i) {
		return operator +=(-i);
	}
	dom_iterator operator +(difference_type i) const {
		auto result=*this;
		result+=i;
		return result;
	}
	friend dom_iterator operator +(difference_type i,const dom_iterator& it) {
		auto result=it;
		result+=i;
		return result;
	}
	dom_iterator operator -(difference_type i) const {
		auto result=*this;
		result-=i;
		return result;
	}
	virtual difference_type operator -(const dom_iterator& other) const {
		switch (object_->type()) {
			case DDT_OBJECT: throw std::invalid_argument("Cannot use offsets with object iterators");
			case DDT_ARRAY: return it_.array_iterator_-other.it_.array_iterator_;
			case DDT_NULL:
			case DDT_STRING:
			case DDT_BOOL:
			case DDT_INT:
			case DDT_FLOAT:
			default: return it_.primitive_iterator_-other.it_.primitive_iterator_;
		}
	}
	virtual ref operator [](difference_type n) const {
		switch (object_->type()) {
			case DDT_OBJECT: throw std::invalid_argument("Cannot use operator[] for object iterators");
			case DDT_ARRAY: return *std::next(it_.array_iterator_,n);
			case DDT_NULL: throw std::runtime_error("Cannot get value");
			case DDT_STRING:
			case DDT_BOOL:
			case DDT_INT:
			case DDT_FLOAT:
			default: {
				if (it_.primitive_iterator_.get_value()==-n) return *object_;
				throw std::runtime_error("Cannot get value");
			}
		}
	}
	const typename object_t::key_type& key() const {
		if (object_->is_object()) return it_.object_iterator_->first;
		throw std::runtime_error("Cannot use key() for non-object iterators");
	}
	ref value() const {
		return operator *();
	}
};

template <typename _Base>
class dom_reverse_iterator : public std::reverse_iterator<_Base> {
public:
	using difference_type=std::ptrdiff_t;
	using base_iterator=std::reverse_iterator<_Base>;
	using ref=typename _Base::ref;

	explicit dom_reverse_iterator(const typename base_iterator::iterator_type& it) noexcept : base_iterator(it) { }
	explicit dom_reverse_iterator(const base_iterator& it) noexcept : base_iterator(it) { }
	dom_reverse_iterator& operator ++() {
		return static_cast<dom_reverse_iterator&>(base_iterator::operator ++());
	}
	dom_reverse_iterator operator ++(int)& {
		return static_cast<dom_reverse_iterator>(base_iterator::operator ++(1));
	}
	dom_reverse_iterator& operator --() {
		return static_cast<dom_reverse_iterator&>(base_iterator::operator --());
	}
	dom_reverse_iterator operator --(int)& {
		return static_cast<dom_reverse_iterator>(base_iterator::operator --(1));
	}
	dom_reverse_iterator& operator +=(difference_type i) {
		return static_cast<dom_reverse_iterator&>(base_iterator::operator +=(i));
	}
	dom_reverse_iterator operator +(difference_type i) const {
		return static_cast<dom_reverse_iterator>(base_iterator::operator +(i));
	}
	dom_reverse_iterator operator -(difference_type i) const {
		return static_cast<dom_reverse_iterator>(base_iterator::operator -(i));
	}
	difference_type operator -(const dom_reverse_iterator& other) const {
		return base_iterator(*this)-base_iterator(other);
	}
	ref operator [](difference_type n) const {
		return *(this->operator +(n));
	}
	auto key() const->decltype(std::declval<_Base>().key()) {
		auto it=--this->base();
		return it.key();
	}
	ref value() const {
		auto it=--this->base();
		return it.operator *();
	}
};

template <typename _Tp>
inline _Tp dom_path_escape(_Tp s) {
	auto replace_substring=[](_Tp& str,const _Tp& format,const _Tp& target){
		for (auto it=str.find(format);it!=_Tp::npos;str.replace(it,format.size(),target),it=str.find(format,it+target.size())) { }
	};
	replace_substring(s,_Tp{"~"},_Tp{"~0"});
	replace_substring(s,_Tp{"/"},_Tp{"~1"});
	return s;
}

template <typename _Tp>
inline _Tp dom_path_unescape(_Tp s) {
	auto replace_substring=[](_Tp& str,const _Tp& format,const _Tp& target){
		for (auto it=str.find(format);it!=_Tp::npos;str.replace(it,format.size(),target),it=str.find(format,it+target.size())) { }
	};
	replace_substring(s,_Tp{"~1"},_Tp{"/"});
	replace_substring(s,_Tp{"~0"},_Tp{"~"});
	return s;
}

template <typename _RefType>
class dom_pointer {
public:
	using string_t=typename dom_string_type<_RefType>::type;

private:
	_STDEX_DOM_TPL_DECLARATION
	friend class dom;
	template <typename>
	friend class dom_pointer;

	std::vector<string_t> ref_tokens_;

	static typename string_t::value_type separator() noexcept {
		return static_cast<typename string_t::value_type>('/');
	}
	static bool token_is_dash(const string_t& token) {
		return token.size()==1 && token[0]==static_cast<typename string_t::value_type>('-');
	}
	static bool char_is_digit(typename string_t::value_type c) noexcept {
		return c>=static_cast<typename string_t::value_type>('0') && c<=static_cast<typename string_t::value_type>('9');
	}

	template <typename _Tp>
	static typename _Tp::size_type array_index(const string_t& s) {
		using size_type=typename _Tp::size_type;
		if (s.size()>1 && s[0]==static_cast<typename string_t::value_type>('0')) throw std::invalid_argument("Array index must not begin with '0'");
		if (s.size()>1 && !char_is_digit(s[0])) throw std::invalid_argument("Array index is not a number");
		const char* p=s.c_str();
		char* p_end=nullptr;
		errno=0;
		const unsigned long long result=std::strtoull(p,&p_end,10);
		if (p==p_end || static_cast<std::size_t>(p_end-p)!=s.size()) throw std::invalid_argument("Unresolved reference token");
		if (errno==ERANGE || result>=static_cast<unsigned long long>((std::numeric_limits<size_type>::max)())) throw std::out_of_range("Array index exceeds size_type");
		return static_cast<size_type>(result);
	}

	dom_pointer top() const {
		if (empty()) throw std::out_of_range("Dom pointer has no parent");
		dom_pointer result=*this;
		result.ref_tokens_={ref_tokens_[0]};
		return result;
	}

	template <typename _Tp>
	_Tp& get_and_create(_Tp& j) const {
		auto* result=&j;
		for (const auto& it:ref_tokens_) {
			switch (result->type()) {
				case DDT_NULL: {
					if (it.size()==1 && it[0]==static_cast<typename string_t::value_type>('0')) result=&result->operator [](static_cast<typename _Tp::size_type>(0));
					else result=&result->operator [](it);
					break;
				}
				case DDT_OBJECT: {
					result=&result->operator [](it);
					break;
				}
				case DDT_ARRAY: {
					result=&result->operator [](array_index<_Tp>(it));
					break;
				}
				case DDT_STRING:
				case DDT_BOOL:
				case DDT_INT:
				case DDT_FLOAT:
				default: throw std::invalid_argument("Invalid value to unflatten");
			}
		}
		return *result;
	}

	template <typename _Tp>
	_Tp& get_unchecked(_Tp* ptr) const {
		for (const auto& it:ref_tokens_) {
			if (ptr->is_null()) {
				const bool nums=!it.empty() && std::all_of(it.begin(),it.end(),[](const typename string_t::value_type x) {
					return char_is_digit(x);
				});
				*ptr=(nums || token_is_dash(it))?DDT_ARRAY:DDT_OBJECT;
			}
			switch (ptr->type()) {
				case DDT_OBJECT: {
					ptr=&ptr->operator [](it);
					break;
				}
				case DDT_ARRAY: {
					if (token_is_dash(it)) ptr=&ptr->operator [](ptr->size());
					else ptr=&ptr->operator [](array_index<_Tp>(it));
					break;
				}
				case DDT_NULL:
				case DDT_STRING:
				case DDT_BOOL:
				case DDT_INT:
				case DDT_FLOAT:
				default: throw std::out_of_range("Unresolved reference token");
			}
		}
		return *ptr;
	}

	template <typename _Tp>
	_Tp& get_checked(_Tp* ptr) const {
		for (const auto& it:ref_tokens_) {
			switch (ptr->type()) {
				case DDT_OBJECT: {
					ptr=&ptr->at(it);
					break;
				}
				case DDT_ARRAY: {
					if (token_is_dash(it)) throw std::out_of_range("Array index is out of range");
					ptr=&ptr->at(array_index<_Tp>(it));
					break;
				}
				case DDT_NULL:
				case DDT_STRING:
				case DDT_BOOL:
				case DDT_INT:
				case DDT_FLOAT:
				default: throw std::out_of_range("Unresolved reference token");
			}
		}
		return *ptr;
	}

	template <typename _Tp>
	const _Tp& get_unchecked(const _Tp* ptr) const {
		for (const auto& it:ref_tokens_) {
			switch (ptr->type()) {
				case DDT_OBJECT: {
					ptr=&ptr->operator [](it);
					break;
				}
				case DDT_ARRAY: {
					if (token_is_dash(it)) throw std::out_of_range("Array index is out of range");
					ptr=&ptr->operator [](array_index<_Tp>(it));
					break;
				}
				case DDT_NULL:
				case DDT_STRING:
				case DDT_BOOL:
				case DDT_INT:
				case DDT_FLOAT:
				default: throw std::out_of_range("Unresolved reference token");
			}
		}
		return *ptr;
	}

	template <typename _Tp>
	const _Tp& get_checked(const _Tp* ptr) const {
		for (const auto& it:ref_tokens_) {
			switch (ptr->type()) {
				case DDT_OBJECT: {
					ptr=&ptr->at(it);
					break;
				}
				case DDT_ARRAY: {
					if (token_is_dash(it)) throw std::out_of_range("Array index is out of range");
					ptr=&ptr->at(array_index<_Tp>(it));
					break;
				}
				case DDT_NULL:
				case DDT_STRING:
				case DDT_BOOL:
				case DDT_INT:
				case DDT_FLOAT:
				default: throw std::out_of_range("Unresolved reference token");
			}
		}
		return *ptr;
	}

	template <typename _Tp>
	bool contains(const _Tp* ptr) const {
		for (const auto& it:ref_tokens_) {
			switch (ptr->type()) {
				case DDT_OBJECT: {
					if (!ptr->contains(it)) return false;
					ptr=&ptr->operator [](it);
					break;
				}
				case DDT_ARRAY: {
					if (token_is_dash(it) || it.empty()) return false;
					if (it.size()==1 && !char_is_digit(it[0])) return false;
					if (it.size()>1) {
						if (!(it[0]>=static_cast<typename string_t::value_type>('1') && it[0]<=static_cast<typename string_t::value_type>('9'))) return false;
						for (std::size_t i=1;i<it.size();i++) {
							if (!char_is_digit(it[i])) return false;
						}
					}
					const auto index=array_index<_Tp>(it);
					if (index>=ptr->size()) return false;
					ptr=&ptr->operator [](index);
					break;
				}
				case DDT_NULL:
				case DDT_STRING:
				case DDT_BOOL:
				case DDT_INT:
				case DDT_FLOAT:
				default: return false;
			}
		}
		return true;
	}

	static std::vector<string_t> split(const string_t& ref_string) {
		std::vector<string_t> result;
		if (ref_string.empty()) return result;
		if (ref_string[0]!=separator()) throw std::invalid_argument("Dom pointer must be empty or begin with '/'");
		for (std::size_t slash=ref_string.find_first_of(separator(),1),start=1;start!=0;start=(slash==string_t::npos)?0:slash+1,slash=ref_string.find_first_of(separator(),start)) {
			auto ref_token=ref_string.substr(start,slash-start);
			for (std::size_t pos=ref_token.find_first_of(static_cast<typename string_t::value_type>('~'));pos!=string_t::npos;pos=ref_token.find_first_of(static_cast<typename string_t::value_type>('~'),pos+1)) {
				if (pos==ref_token.size()-1 || (ref_token[pos+1]!=static_cast<typename string_t::value_type>('0') && ref_token[pos+1]!=static_cast<typename string_t::value_type>('1'))) throw std::invalid_argument("Escape character '~' must be followed with '0' or '1'");
			}
			result.push_back(dom_path_unescape(std::move(ref_token)));
		}
		return result;
	}

	template <typename _Tp>
	static void flatten(const string_t& ref_string,const _Tp& val,_Tp& result) {
		switch (val.type()) {
			case DDT_ARRAY: {
				if (val.empty()) result[ref_string]=nullptr;
				else {
					for (std::size_t i=0;i<val.size();i++) {
						const std::string index_string=std::to_string(i);
						flatten(ref_string+separator()+string_t(index_string.begin(),index_string.end()),val[static_cast<typename _Tp::size_type>(i)],result);
					}
				}
				break;
			}
			case DDT_OBJECT: {
				if (val.empty()) result[ref_string]=nullptr;
				else {
					for (auto it=val.cbegin();it!=val.cend();it++) flatten(ref_string+separator()+dom_path_escape(it.key()),it.value(),result);
				}
				break;
			}
			case DDT_NULL:
			case DDT_STRING:
			case DDT_BOOL:
			case DDT_INT:
			case DDT_FLOAT:
			default: {
				result[ref_string]=val;
				break;
			}
		}
	}

	template <typename _Tp>
	static _Tp unflatten(const _Tp& val) {
		if (!val.is_object()) throw std::invalid_argument("Only objects can be unflattened");
		_Tp result;
		for (auto it=val.cbegin();it!=val.cend();it++) {
			if (!it.value().is_primitive()) throw std::invalid_argument("Values in object must be primitive");
			dom_pointer(it.key()).get_and_create(result)=it.value();
		}
		return result;
	}

	dom_pointer<string_t> convert() const& {
		dom_pointer<string_t> result;
		result.ref_tokens_=ref_tokens_;
		return result;
	}
	dom_pointer<string_t> convert()&& {
		dom_pointer<string_t> result;
		result.ref_tokens_=std::move(ref_tokens_);
		return result;
	}

public:
	explicit dom_pointer(const string_t& s=string_t()) : ref_tokens_(split(s)) { }

	string_t to_string() const {
		return std::accumulate(ref_tokens_.begin(),ref_tokens_.end(),string_t{},[](const string_t& a,const string_t& b){
			return a+separator()+dom_path_escape(b);
		});
	}

	friend std::ostream& operator <<(std::ostream& o,const dom_pointer& ptr) {
		o<<ptr.to_string();
		return o;
	}

	dom_pointer& operator /=(const dom_pointer& other) {
		ref_tokens_.insert(ref_tokens_.end(),other.ref_tokens_.begin(),other.ref_tokens_.end());
		return *this;
	}
	dom_pointer& operator /=(string_t token) {
		push_back(std::move(token));
		return *this;
	}
	dom_pointer& operator /=(std::size_t array_index) {
		const std::string index_string=std::to_string(array_index);
		return *this/=string_t(index_string.begin(),index_string.end());
	}
	friend dom_pointer operator /(const dom_pointer& lhs,const dom_pointer& rhs) {
		return dom_pointer(lhs)/=rhs;
	}
	friend dom_pointer operator /(const dom_pointer& lhs,string_t token) {
		return dom_pointer(lhs)/=std::move(token);
	}
	friend dom_pointer operator /(const dom_pointer& lhs,std::size_t array_index) {
		return dom_pointer(lhs)/=array_index;
	}

	dom_pointer parent_pointer() const {
		if (empty()) return *this;
		dom_pointer result=*this;
		result.pop_back();
		return result;
	}

	void pop_back() {
		if (empty()) throw std::out_of_range("Dom pointer has no parent");
		ref_tokens_.pop_back();
	}

	const string_t& back() const {
		if (empty()) throw std::out_of_range("Dom pointer has no parent");
		return ref_tokens_.back();
	}

	void push_back(const string_t& token) {
		ref_tokens_.push_back(token);
	}
	void push_back(string_t&& token) {
		ref_tokens_.push_back(std::move(token));
	}

	bool empty() const noexcept {
		return ref_tokens_.empty();
	}

#if __cplusplus>=_STDEX_CPP20_VERSION
	template <typename _RefTypeRhs>
	bool operator ==(const dom_pointer<_RefTypeRhs>& rhs) const noexcept {
		return ref_tokens_==rhs.ref_tokens_;
	}

	template <typename _RefTypeRhs>
	std::strong_ordering operator <=>(const dom_pointer<_RefTypeRhs>& rhs) const noexcept {
		return ref_tokens_<=>rhs.ref_tokens_;
	}
#else
	template <typename _RefTypeLhs,typename _RefTypeRhs>
	friend bool operator ==(const dom_pointer<_RefTypeLhs>& lhs,const dom_pointer<_RefTypeRhs>& rhs) noexcept;

	template <typename _RefTypeLhs,typename _RefTypeRhs>
	friend bool operator !=(const dom_pointer<_RefTypeLhs>& lhs,const dom_pointer<_RefTypeRhs>& rhs) noexcept;

	template <typename _RefTypeLhs,typename _RefTypeRhs>
	friend bool operator <(const dom_pointer<_RefTypeLhs>& lhs,const dom_pointer<_RefTypeRhs>& rhs) noexcept;
#endif
};

#if __cplusplus<_STDEX_CPP20_VERSION
template <typename _RefTypeLhs,typename _RefTypeRhs>
inline bool operator ==(const dom_pointer<_RefTypeLhs>& lhs,const dom_pointer<_RefTypeRhs>& rhs) noexcept {
	return lhs.ref_tokens_==rhs.ref_tokens_;
}

template <typename _RefTypeLhs,typename _RefTypeRhs>
inline bool operator !=(const dom_pointer<_RefTypeLhs>& lhs,const dom_pointer<_RefTypeRhs>& rhs) noexcept {
	return !(lhs==rhs);
}

template <typename _RefTypeLhs,typename _RefTypeRhs>
inline bool operator <(const dom_pointer<_RefTypeLhs>& lhs,const dom_pointer<_RefTypeRhs>& rhs) noexcept {
	return lhs.ref_tokens_<rhs.ref_tokens_;
}
#endif

_STDEX_DOM_TPL_DECLARATION
class dom {
public:
	template <typename>
	struct is_dom_ref : std::false_type { };
	template <typename _Vp>
	struct is_dom_ref<container::reference<_Vp>> : std::is_same<_Vp,dom> { };

	using dom_base_t=dom;
	using self_t=dom;
	using value_type=dom;
	using difference_type=std::ptrdiff_t;
	using size_type=std::size_t;
	using int_t=_Int;
	using float_t=_Float;
	using boolean_t=_Boolean;
	using string_t=_String;
	using const_char_t=const typename string_t::value_type*;
	using array_t=_Array<dom,_Allocator<dom>>;
	using object_t=_Object<string_t,dom,std::less<string_t>,_Allocator<std::pair<const string_t,dom>>>;
	using object_comparator_t=typename object_t::key_compare;
	template <typename _Vp>
	using allocator_t=_Allocator<_Vp>;
	using initializer_list_t=std::initializer_list<container::reference<dom>>;
	template <typename _Vp>
	using uncvref_t=typename std::remove_cv<typename std::remove_reference<_Vp>::type>::type;
	using ref=dom&;
	using const_ref=const dom&;
	using pointer=dom*;
	using const_pointer=const dom*;
	using iterator=dom_iterator<dom>;
	using const_iterator=dom_iterator<const dom>;
	using reverse_iterator=dom_reverse_iterator<typename dom::iterator>;
	using const_reverse_iterator=dom_reverse_iterator<typename dom::const_iterator>;
	using dom_pointer_t=structure::dom_pointer<string_t>;

	template <typename>
	friend class structure::dom_pointer;
	template <typename>
	friend class structure::dom_iterator;
	template <typename>
	friend class structure::dom_iteration_proxy;
	template <typename>
	friend class structure::dom_iteration_proxy_value;

private:
	template <typename _Vp,typename=void>
	struct is_complete_type : std::false_type { };
	template <typename _Vp>
	struct is_complete_type<_Vp,decltype(void(sizeof(_Vp)))> : std::true_type { };
	
	template <typename _Vp>
	struct is_dom_scalar : std::integral_constant<bool,std::is_scalar<_Vp>::value && !std::is_same<typename std::remove_cv<typename std::remove_reference<_Vp>::type>::type,dom_data_type::enumeration_type>::value> { };

	template <typename _Compatible,typename=void>
	struct is_compatible_type : std::false_type { };
	template <typename _Compatible>
	struct is_compatible_type<_Compatible,std::enable_if_t<is_complete_type<_Compatible>::value>> {
		static constexpr bool value=!std::is_same<_Compatible,dom_data_type>::value && !std::is_base_of<dom_data_type,_Compatible>::value && !std::is_same<_Compatible,dom_data_type::enumeration_type>::value && !is_dom<_Compatible>::value && (std::is_same<_Compatible,boolean_t>::value || std::is_same<_Compatible,int_t>::value || std::is_same<_Compatible,float_t>::value || std::is_arithmetic<_Compatible>::value || std::is_constructible<string_t,_Compatible>::value || std::is_same<_Compatible,array_t>::value || std::is_same<_Compatible,object_t>::value);
	};

	template <typename _Vp,typename... _Args>
	_STDEX_RETURNS_NON_NULL
	static _Vp* create(_Args&&... args) {
		allocator_t<_Vp> alloc;
		auto deleter=[&](_Vp* object){
			std::allocator_traits<allocator_t<_Vp>>::deallocate(alloc,object,1);
		};
		std::unique_ptr<_Vp,decltype(deleter)> object(std::allocator_traits<allocator_t<_Vp>>::allocate(alloc,1),deleter);
		std::allocator_traits<allocator_t<_Vp>>::construct(alloc,object.get(),std::forward<_Args>(args)...);
		return object.release();
	}

public:
	template <typename _Comparator,typename _Lhs,typename _Rhs,typename=void>
	struct is_comparable : std::false_type { };
	template <typename _Comparator,typename _Lhs,typename _Rhs>
	struct is_comparable<_Comparator,_Lhs,_Rhs,std::void_t<decltype(std::declval<_Comparator>()(std::declval<_Lhs>(),std::declval<_Rhs>()))>> : std::true_type { };
	template <typename _KeyType>
	using is_comparable_with_object_key=is_comparable<object_comparator_t,const typename object_t::key_type&,_KeyType>;

	template <typename _Vp>
	struct value_return_type {
		using type=std::conditional_t<std::is_convertible<std::decay_t<_Vp>,const_char_t>::value,string_t,std::decay_t<_Vp>>;
	};

	template <typename _Vp,typename=void>
	struct is_transparent : std::false_type { };
	template <typename _Vp>
	struct is_transparent<_Vp,std::void_t<typename _Vp::is_transparent>> : std::true_type { };

	template <typename _Dom,typename _Vp,typename=void>
	struct is_getable : std::false_type { };
	template <typename _Dom,typename _Vp>
	struct is_getable<_Dom,_Vp,std::void_t<decltype(std::declval<const _Dom&>().template get<_Vp>())>> : std::true_type { };

	template <typename _ObjectType,typename _KeyType,typename=void>
	struct has_erase_with_key_type : std::false_type { };
	template <typename _ObjectType,typename _KeyType>
	struct has_erase_with_key_type<_ObjectType,_KeyType,std::void_t<decltype(std::declval<_ObjectType&>().erase(std::declval<_KeyType>()))>> : std::true_type { };

	template <typename _KeyType>
	struct is_usable_as_key_type {
		static constexpr bool value=is_transparent<object_comparator_t>::value && !std::is_same<uncvref_t<_KeyType>,typename object_t::key_type>::value && is_comparable_with_object_key<_KeyType>::value && !is_dom<uncvref_t<_KeyType>>::value && !is_dom_pointer<uncvref_t<_KeyType>>::value && !std::is_base_of<dom_data_type,uncvref_t<_KeyType>>::value && !std::is_same<uncvref_t<_KeyType>,iterator>::value && !std::is_same<uncvref_t<_KeyType>,const_iterator>::value;
	};

public:
	struct value_t {
		union {
			int_t integer;
			float_t floating;
			boolean_t boolean;
			string_t* string;
			array_t* array;
			object_t* object;
			void* other;
		};

		value_t() noexcept : object(nullptr) { }
		value_t(int_t v) noexcept : integer(v) { }
		value_t(float_t v) noexcept : floating(v) { }
		value_t(boolean_t v) noexcept : boolean(v) { }
		value_t(const string_t& v) : string(create<string_t>(v)) { }
		value_t(string_t&& v) : string(create<string_t>(std::move(v))) { }
		value_t(const array_t& v) : array(create<array_t>(v)) { }
		value_t(array_t&& v) : array(create<array_t>(std::move(v))) { }
		value_t(const object_t& v) : object(create<object_t>(v)) { }
		value_t(object_t&& v) : object(create<object_t>(std::move(v))) { }
		value_t(size_type cnt,const dom& val) : array(create<array_t>(cnt,val)) { }
		value_t(dom_data_type t) {
			switch (t) {
				case DDT_OBJECT: object=create<object_t>();break;
				case DDT_ARRAY: array=create<array_t>();break;
				case DDT_STRING: string=create<string_t>();break;
				case DDT_BOOL: boolean=static_cast<boolean_t>(false);break;
				case DDT_INT: integer=static_cast<int_t>(0);break;
				case DDT_FLOAT: floating=static_cast<float_t>(0.0);break;
				case DDT_NULL:
				default: object=nullptr;break;
			}
		}
		virtual ~value_t()=default;

		value_t(const value_t&)=delete;
		value_t(value_t&&)=delete;

		value_t& operator =(const value_t&)=delete;
		value_t& operator =(value_t&&)=delete;


		virtual void destroy(dom_data_type t) {
			if ((t==DDT_OBJECT && !object) || (t==DDT_ARRAY && !array) || (t==DDT_STRING && !string)) return;
			if (t==DDT_ARRAY || t==DDT_OBJECT) {
				std::vector<dom> stack;
				if (t==DDT_ARRAY) {
					stack.reserve(array->size());
					std::move(array->begin(),array->end(),std::back_inserter(stack));
				} else {
					stack.reserve(object->size());
					for (auto&& it:*object) stack.push_back(std::move(it.second));
				}
				while (!stack.empty()) {
					dom current_item(std::move(stack.back()));
					stack.pop_back();
					if (current_item.is_array()) {
						std::move(current_item.value().array->begin(),current_item.value().array->end(),std::back_inserter(stack));
						current_item.value().array->clear();
					} else if (current_item.is_object()) {
						for (auto&& it:*current_item.value().object) stack.push_back(std::move(it.second));
						current_item.value().object->clear();
					}
				}
			}
			switch (t) {
				case DDT_OBJECT: {
					allocator_t<object_t> alloc;
					std::allocator_traits<allocator_t<object_t>>::destroy(alloc,object);
					std::allocator_traits<allocator_t<object_t>>::deallocate(alloc,object,1);
					object=nullptr;
					break;
				}
				case DDT_ARRAY: {
					allocator_t<array_t> alloc;
					std::allocator_traits<allocator_t<array_t>>::destroy(alloc,array);
					std::allocator_traits<allocator_t<array_t>>::deallocate(alloc,array,1);
					array=nullptr;
					break;
				}
				case DDT_STRING: {
					allocator_t<string_t> alloc;
					std::allocator_traits<allocator_t<string_t>>::destroy(alloc,string);
					std::allocator_traits<allocator_t<string_t>>::deallocate(alloc,string,1);
					string=nullptr;
					break;
				}
				case DDT_NULL:
				case DDT_INT:
				case DDT_FLOAT:
				case DDT_BOOL:
				default: break;
			}
		}
		virtual void destroy_self(dom_data_type t) {
			destroy(t);
			allocator_t<value_t> alloc;
			std::allocator_traits<allocator_t<value_t>>::destroy(alloc,this);
			std::allocator_traits<allocator_t<value_t>>::deallocate(alloc,this,1);
		}
		virtual value_t* clone(dom_data_type t) const {
			value_t* result=create<value_t>();
			switch (t) {
				case DDT_OBJECT: result->object=create<object_t>(*object);break;
				case DDT_ARRAY: result->array=create<array_t>(*array);break;
				case DDT_STRING: result->string=create<string_t>(*string);break;
				case DDT_BOOL: result->boolean=boolean;break;
				case DDT_INT: result->integer=integer;break;
				case DDT_FLOAT: result->floating=floating;break;
				case DDT_NULL:
				default: break;
			}
			return result;
		}
	};
	struct data_t {
		dom_data_type type=DDT_NULL;
		value_t* value=nullptr;

		data_t() noexcept=default;
		data_t(dom_data_type t,value_t* v) noexcept : type(t) , value(v) { }
		data_t(dom_data_type t) : type(t) , value(t==DDT_NULL?nullptr:create<value_t>(t)) { }
		data_t(size_type cnt,const dom& val) : type(DDT_ARRAY) , value(create<value_t>(cnt,val)) { }
		~data_t() noexcept {
			reset();
		}

		data_t(const data_t& other) : type(other.type) , value(other.value?other.value->clone(other.type):nullptr) { }
		data_t(data_t&& other) noexcept : type(other.type) , value(other.value) {
			other.type=DDT_NULL;
			other.value=nullptr;
		}

		data_t& operator =(const data_t&)=delete;
		data_t& operator =(data_t&& other) noexcept {
			if (this!=&other) {
				reset();
				type=other.type;
				value=other.value;
				other.type=DDT_NULL;
				other.value=nullptr;
			}
			return *this;
		}

		void reset() noexcept {
			if (value) value->destroy_self(type);
			value=nullptr;
			type=DDT_NULL;
		}
	};

private:
	data_t data_{};

protected:
	virtual value_t& morph(dom_data_type t) {
		if (!data_.value) data_.value=create<value_t>(t);
		else {
			switch (t) {
				case DDT_OBJECT: data_.value->object=create<object_t>();break;
				case DDT_ARRAY: data_.value->array=create<array_t>();break;
				case DDT_STRING: data_.value->string=create<string_t>();break;
				case DDT_BOOL: data_.value->boolean=static_cast<boolean_t>(false);break;
				case DDT_INT: data_.value->integer=static_cast<int_t>(0);break;
				case DDT_FLOAT: data_.value->floating=static_cast<float_t>(0.0);break;
				case DDT_NULL:
				default: break;
			}
		}
		data_.type=t;
		return *data_.value;
	}

public:
	dom(dom_data_type t) : data_(t) { }
	dom(dom_data_type::enumeration_type t) : data_(dom_data_type(t)) { }
	dom(std::nullptr_t=nullptr) noexcept { }
	template <typename _Compatible,typename _Up=uncvref_t<_Compatible>,std::enable_if_t<!is_dom<_Up>::value && is_compatible_type<_Up>::value,int> =0>
	dom(_Compatible&& val) {
		if constexpr (std::is_same<_Up,boolean_t>::value) {
			data_=data_t(DDT_BOOL,create<value_t>(static_cast<boolean_t>(val)));
		} else if constexpr (std::is_same<_Up,float_t>::value || std::is_same<_Up,float>::value || std::is_same<_Up,double>::value || std::is_floating_point<_Up>::value) {
			data_=data_t(DDT_FLOAT,create<value_t>(static_cast<float_t>(val)));
		} else if constexpr (std::is_same<_Up,int_t>::value || ((std::is_integral<_Up>::value || std::is_enum<_Up>::value || std::is_convertible<_Up,int_t>::value) && !std::is_constructible<string_t,_Up>::value)) {
			data_=data_t(DDT_INT,create<value_t>(static_cast<int_t>(val)));
		} else if constexpr (std::is_same<_Up,string_t>::value || std::is_constructible<string_t,_Compatible>::value) {
			data_=data_t(DDT_STRING,create<value_t>(string_t(std::forward<_Compatible>(val))));
		} else if constexpr (std::is_same<_Up,array_t>::value) {
			data_=data_t(DDT_ARRAY,create<value_t>(std::forward<_Compatible>(val)));
		} else if constexpr (std::is_same<_Up,object_t>::value) {
			data_=data_t(DDT_OBJECT,create<value_t>(std::forward<_Compatible>(val)));
		} else {
			data_=data_t(DDT_NULL);
		}
	}
	dom(initializer_list_t init_list,bool type_deduction=true,dom_data_type manual_type=DDT_ARRAY) {
		bool is_an_object=std::all_of(init_list.begin(),init_list.end(),[](const container::reference<dom>& element_ref){
			return element_ref->is_array() && element_ref->size()==2 && (*element_ref)[static_cast<size_type>(0)].is_string();
		});
		if (!type_deduction) {
			if (manual_type==DDT_ARRAY) is_an_object=false;
			if (manual_type==DDT_OBJECT && !is_an_object) throw std::invalid_argument("Cannot create object from initializer_list");
		}
		if (is_an_object) {
			morph(DDT_OBJECT);
			for (auto& element_ref:init_list) {
				auto element=element_ref.moved_or_copied();
				value().object->emplace(std::move(*((*element.value().array)[0].value().string)),std::move((*element.value().array)[1]));
			}
		} else {
			morph(DDT_ARRAY);
			value().array->insert(value().array->end(),init_list.begin(),init_list.end());
		}
	}
	static dom array(initializer_list_t init_list={}) {
		return dom(init_list,false,DDT_ARRAY);
	}
	static dom object(initializer_list_t init_list={}) {
		return dom(init_list,false,DDT_OBJECT);
	}
	dom(size_type cnt,const dom& val) : data_(cnt,val) { }
	template <typename _InputIt,std::enable_if_t<std::is_same<_InputIt,typename dom::iterator>::value || std::is_same<_InputIt,typename dom::const_iterator>::value,int> =0>
	dom(_InputIt first,_InputIt last) {
		if (first.object_!=last.object_) throw std::invalid_argument("Iterators are not compatible");
		const dom_data_type t=first.object_->type();
		switch (t) {
			case DDT_INT:
			case DDT_FLOAT:
			case DDT_BOOL:
			case DDT_STRING: {
				if (!first.it_.primitive_iterator_.is_begin()||!last.it_.primitive_iterator_.is_end()) throw std::out_of_range("Iterators out of range");
				break;
			}
			case DDT_ARRAY:
			case DDT_OBJECT:
			case DDT_NULL:
			default: break;
		}
		switch (t) {
			case DDT_INT: data_=data_t(DDT_INT,create<value_t>(first.object_->value().integer));break;
			case DDT_FLOAT: data_=data_t(DDT_FLOAT,create<value_t>(first.object_->value().floating));break;
			case DDT_BOOL: data_=data_t(DDT_BOOL,create<value_t>(first.object_->value().boolean));break;
			case DDT_STRING: data_=data_t(DDT_STRING,create<value_t>(*first.object_->value().string));break;
			case DDT_OBJECT: data_=data_t(DDT_OBJECT,create<value_t>(object_t(first.it_.object_iterator_,last.it_.object_iterator_)));break;
			case DDT_ARRAY: data_=data_t(DDT_ARRAY,create<value_t>(array_t(first.it_.array_iterator_,last.it_.array_iterator_)));break;
			case DDT_NULL:
			default: throw std::invalid_argument("Cannot construct dom with iterators");
		}
	}
	virtual ~dom() noexcept=default;

	template <typename _DomRef,std::enable_if_t<is_dom_ref<_DomRef>::value,int> =0>
	dom(const _DomRef& ref_value) : dom(ref_value.moved_or_copied()) { }
	dom(const dom& other) : data_(other.data_) { }
	dom(dom&& other) noexcept : data_(std::move(other.data_)) { }

	dom& operator =(const dom& other) {
		if (this==&other) return *this;
		if (!support(other.type()) && !other.support(other.type())) {
			dom temp(other);
			assign_unsupported(std::move(temp));
			return *this;
		}
		data_t temp(other.data_);
		std::swap(data_.type,temp.type);
		std::swap(data_.value,temp.value);
		return *this;
	}

	dom& operator =(dom&& other) {
		if (this==&other) return *this;
		if (!support(other.type()) && !other.support(other.type())) {
			assign_unsupported(std::move(other));
			return *this;
		}
		std::swap(data_.type,other.data_.type);
		std::swap(data_.value,other.data_.value);
		return *this;
	}

	virtual bool support(dom_data_type t) const noexcept {
		return t>=DDT_NULL && t<=DDT_OBJECT;
	}

protected:
	virtual void assign_unsupported(dom&& other) {
		static_cast<void>(other);
		throw std::invalid_argument("Unsupported data type for this container");
	}

	virtual bool degrade_unsupported(const dom& source,dom& replacement) const {
		static_cast<void>(source);
		static_cast<void>(replacement);
		return false;
	}

	virtual bool convert_unsupported(const dom& source,dom& replacement) const {
		static_cast<void>(source);
		static_cast<void>(replacement);
		return false;
	}

public:
	data_t& data() noexcept {
		return data_;
	}
	const data_t& data() const noexcept {
		return data_;
	}
	value_t& value() noexcept {
		return *data_.value;
	}
	const value_t& value() const noexcept {
		return *data_.value;
	}

	//virtual string_t dump(const int indent=-1,const char indent_char=' ',const bool ensure_ascii=false/*, const error_handler_t error_handler=error_handler_t::strict */) const=0;

	dom_data_type type() const noexcept { return data_.type; }
	operator dom_data_type() const noexcept { return data_.type; }

	using convert_handler_t=std::function<bool(const dom&,dom&)>;

	dom& assign_converted(const dom& other,dom_convert_policy policy=DCP_STRICT,const convert_handler_t& handler=convert_handler_t()) {
		dom result;
		if (convert_node(other,*this,other,policy,handler,result)) *this=std::move(result);
		else *this=dom();
		return *this;
	}

	template <typename _Target,std::enable_if_t<std::is_base_of<dom,_Target>::value,int> =0>
	_Target convert_to(dom_convert_policy policy=DCP_STRICT,const convert_handler_t& handler=convert_handler_t()) const {
		_Target result;
		result.assign_converted(*this,policy,handler);
		return result;
	}

private:
	static bool convert_node(const dom& node,const dom& target,const dom& source_root,dom_convert_policy policy,const convert_handler_t& handler,dom& out) {
		if (target.support(node.type())) {
			switch (node.type()) {
				case DDT_OBJECT: {
					out=dom(DDT_OBJECT);
					object_t& object=*out.data_.value->object;
					for (auto it=node.cbegin();it!=node.cend();it++) {
						dom child;
						if (convert_node(*it,target,source_root,policy,handler,child)) object.emplace(it.key(),std::move(child));
					}
					return true;
				}
				case DDT_ARRAY: {
					out=dom(DDT_ARRAY);
					array_t& array=*out.data_.value->array;
					for (auto it=node.cbegin();it!=node.cend();it++) {
						dom child;
						if (convert_node(*it,target,source_root,policy,handler,child)) array.push_back(std::move(child));
					}
					return true;
				}
				default: {
					out.data_=data_t(node.data_);
					return true;
				}
			}
		}
		dom replacement;
		if (handler && handler(node,replacement)) return convert_node(replacement,target,source_root,policy,handler,out);
		if (source_root.degrade_unsupported(node,replacement)) return convert_node(replacement,target,source_root,policy,handler,out);
		if (target.convert_unsupported(node,replacement)) return convert_node(replacement,target,source_root,policy,handler,out);
		if (policy==DCP_STRICT) throw std::invalid_argument("Cannot convert node of type "+std::to_string(static_cast<long long>(static_cast<int>(node.type())))+" to the target notation");
		return false;
	}
	
public:
	virtual bool is_null() const noexcept {
		return type()==DDT_NULL;
	}
	virtual bool is_boolean() const noexcept {
		return type()==DDT_BOOL;
	}
	virtual bool is_string() const noexcept {
		return type()==DDT_STRING;
	}
	virtual bool is_integer() const noexcept {
		return type()==DDT_INT;
	}
	virtual bool is_float() const noexcept {
		return type()==DDT_FLOAT;
	}
	virtual bool is_number() const noexcept {
		return is_integer() || is_float();
	}
	virtual bool is_array() const noexcept {
		return type()==DDT_ARRAY;
	}
	virtual bool is_object() const noexcept {
		return type()==DDT_OBJECT;
	}
	virtual bool is_structured() const noexcept {
		return is_array() || is_object();
	}
	virtual bool is_primitive() const noexcept {
		return !is_structured();
	}

private:
	boolean_t get_impl(boolean_t*) const {
		if (is_boolean()) return value().boolean;
		throw std::invalid_argument("Type must be boolean");
	}
	string_t get_impl(string_t*) const {
		if (is_string()) return *value().string;
		throw std::invalid_argument("Type must be string");
	}
	array_t get_impl(array_t*) const {
		if (is_array()) return *value().array;
		throw std::invalid_argument("Type must be array");
	}
	object_t get_impl(object_t*) const {
		if (is_object()) return *value().object;
		throw std::invalid_argument("Type must be object");
	}
	dom get_impl(dom*) const {
		return *this;
	}
	std::nullptr_t get_impl(std::nullptr_t*) const {
		if (is_null()) return nullptr;
		throw std::invalid_argument("Type must be null");
	}

	template <typename _Arithmetic,std::enable_if_t<std::is_arithmetic<_Arithmetic>::value && !std::is_same<_Arithmetic,boolean_t>::value && !std::is_same<_Arithmetic,string_t>::value,int> =0>
	_Arithmetic get_impl(_Arithmetic*) const {
		switch (type()) {
			case DDT_INT: return static_cast<_Arithmetic>(value().integer);
			case DDT_FLOAT: return static_cast<_Arithmetic>(value().floating);
			case DDT_BOOL: return static_cast<_Arithmetic>(value().boolean);
			case DDT_NULL:
			case DDT_STRING:
			case DDT_ARRAY:
			case DDT_OBJECT:
			default: throw std::invalid_argument("Type must be number");
		}
	}

	object_t* get_impl_ptr(object_t*) noexcept {
		return is_object()?value().object:nullptr;
	}
	const object_t* get_impl_ptr(const object_t*) const noexcept {
		return is_object()?value().object:nullptr;
	}
	array_t* get_impl_ptr(array_t*) noexcept {
		return is_array()?value().array:nullptr;
	}
	const array_t* get_impl_ptr(const array_t*) const noexcept {
		return is_array()?value().array:nullptr;
	}
	string_t* get_impl_ptr(string_t*) noexcept {
		return is_string()?value().string:nullptr;
	}
	const string_t* get_impl_ptr(const string_t*) const noexcept {
		return is_string()?value().string:nullptr;
	}
	boolean_t* get_impl_ptr(boolean_t*) noexcept {
		return is_boolean()?&value().boolean:nullptr;
	}
	const boolean_t* get_impl_ptr(const boolean_t*) const noexcept {
		return is_boolean()?&value().boolean:nullptr;
	}
	int_t* get_impl_ptr(int_t*) noexcept {
		return is_integer()?&value().integer:nullptr;
	}
	const int_t* get_impl_ptr(const int_t*) const noexcept {
		return is_integer()?&value().integer:nullptr;
	}
	float_t* get_impl_ptr(float_t*) noexcept {
		return is_float()?&value().floating:nullptr;
	}
	const float_t* get_impl_ptr(const float_t*) const noexcept {
		return is_float()?&value().floating:nullptr;
	}
	template <typename _Reference,typename _This>
	static _Reference get_ref_impl(_This& obj) {
		auto* ptr=obj.template get_ptr<typename std::add_pointer<_Reference>::type>();
		if (ptr) return *ptr;
		throw std::invalid_argument("Incompatible reference type for get_ref");
	}

public:
	template <typename _Pointer,std::enable_if_t<std::is_pointer<_Pointer>::value,int> =0>
	auto get_ptr() noexcept->decltype(std::declval<dom&>().get_impl_ptr(std::declval<_Pointer>())) {
		return get_impl_ptr(static_cast<_Pointer>(nullptr));
	}
	template <typename _Pointer,std::enable_if_t<std::is_pointer<_Pointer>::value && std::is_const<typename std::remove_pointer<_Pointer>::type>::value,int> =0>
	auto get_ptr() const noexcept->decltype(std::declval<const dom&>().get_impl_ptr(std::declval<_Pointer>())) {
		return get_impl_ptr(static_cast<_Pointer>(nullptr));
	}

	template <typename _ValueType,std::enable_if_t<!std::is_pointer<_ValueType>::value,int> =0>
	auto get() const->decltype(std::declval<const dom&>().get_impl(static_cast<_ValueType*>(nullptr))) {
		return get_impl(static_cast<_ValueType*>(nullptr));
	}
	template <typename _Pointer,std::enable_if_t<std::is_pointer<_Pointer>::value,int> =0>
	auto get() noexcept->decltype(std::declval<dom&>().template get_ptr<_Pointer>()) {
		return get_ptr<_Pointer>();
	}
	template <typename _Pointer,std::enable_if_t<std::is_pointer<_Pointer>::value && std::is_const<typename std::remove_pointer<_Pointer>::type>::value,int> =0>
	auto get() const noexcept->decltype(std::declval<const dom&>().template get_ptr<_Pointer>()) {
		return get_ptr<_Pointer>();
	}

	template <typename _Reference,std::enable_if_t<std::is_reference<_Reference>::value,int> =0>
	_Reference get_ref() {
		return get_ref_impl<_Reference>(*this);
	}
	template <typename _Reference,std::enable_if_t<std::is_reference<_Reference>::value && std::is_const<typename std::remove_reference<_Reference>::type>::value,int> =0>
	_Reference get_ref() const {
		return get_ref_impl<_Reference>(*this);
	}

	template <typename _ValueType,std::enable_if_t<!std::is_pointer<_ValueType>::value && is_getable<self_t,_ValueType>::value,int> =0>
	_ValueType& get_to(_ValueType& v) const {
		v=get<_ValueType>();
		return v;
	}

	template <typename _ValueType,std::enable_if_t<!std::is_pointer<_ValueType>::value && !is_dom<_ValueType>::value && !std::is_same<_ValueType,dom_data_type>::value && !std::is_same<_ValueType,std::nullptr_t>::value && is_getable<self_t,_ValueType>::value,int> =0>
	operator _ValueType() const {
		return get<_ValueType>();
	}

	virtual ref at(size_type index) {
		if (is_array()) return value().array->at(index);
		throw std::invalid_argument("Cannot use at()");
	}
	virtual const_ref at(size_type index) const {
		if (is_array()) return value().array->at(index);
		throw std::invalid_argument("Cannot use at()");
	}
	virtual ref at(const typename object_t::key_type& key) {
		if (!is_object()) throw std::invalid_argument("Cannot use at()");
		auto it=value().object->find(key);
		if (it==value().object->end()) throw std::out_of_range("Key not found");
		return it->second;
	}
	virtual const_ref at(const typename object_t::key_type& key) const {
		if (!is_object()) throw std::invalid_argument("Cannot use at()");
		auto it=value().object->find(key);
		if (it==value().object->end()) throw std::out_of_range("Key not found");
		return it->second;
	}

	virtual ref operator [](size_type index) {
		if (is_null()) morph(DDT_ARRAY);
		if (is_array()) {
			if (index>=value().array->size()) value().array->resize(index+1);
			return value().array->operator [](index);
		}
		throw std::invalid_argument("Cannot use operator[]");
	}
	virtual const_ref operator [](size_type index) const {
		if (is_array()) return value().array->operator [](index);
		throw std::invalid_argument("Cannot use operator[]");
	}

	virtual ref operator [](typename object_t::key_type key) {
		if (is_null()) morph(DDT_OBJECT);
		if (is_object()) {
			auto result=value().object->emplace(std::move(key),nullptr);
			return result.first->second;
		}
		throw std::invalid_argument("Cannot use operator[]");
	}
	virtual const_ref operator [](const typename object_t::key_type& key) const {
		if (is_object()) {
			auto it=value().object->find(key);
			if (it==value().object->end()) throw std::out_of_range("Key not found");
			return it->second;
		}
		throw std::invalid_argument("Cannot use operator[]");
	}

	template <typename _CharType>
	ref operator [](_CharType* key) {
		return operator [](typename object_t::key_type(key));
	}
	template <typename _CharType>
	const_ref operator [](_CharType* key) const {
		return operator [](typename object_t::key_type(key));
	}

	template <typename _KeyType,std::enable_if_t<is_usable_as_key_type<_KeyType>::value,int> =0>
	ref operator [](_KeyType&& key) {
		if (is_null()) morph(DDT_OBJECT);
		if (is_object()) {
			auto result=value().object->emplace(std::forward<_KeyType>(key),nullptr);
			return result.first->second;
		}
		throw std::invalid_argument("Cannot use operator[]");
	}
	template <typename _KeyType,std::enable_if_t<is_usable_as_key_type<_KeyType>::value,int> =0>
	const_ref operator [](_KeyType&& key) const {
		if (is_object()) {
			auto it=value().object->find(std::forward<_KeyType>(key));
			if (it==value().object->end()) throw std::out_of_range("Key not found");
			return it->second;
		}
		throw std::invalid_argument("Cannot use operator[]");
	}

	template <typename _ValueType,std::enable_if_t<!is_transparent<object_comparator_t>::value && is_getable<self_t,_ValueType>::value && !std::is_base_of<dom_data_type,uncvref_t<_ValueType>>::value,int> =0>
	_ValueType value(const typename object_t::key_type& key,const _ValueType& default_value) const {
		if (is_object()) {
			const auto it=find(key);
			if (it!=cend()) return it->template get<_ValueType>();
			return default_value;
		}
		throw std::invalid_argument("Cannot use value()");
	}
	template <typename _ValueType,typename _Return=typename value_return_type<_ValueType>::type,std::enable_if_t<!is_transparent<object_comparator_t>::value && is_getable<self_t,_Return>::value && !std::is_base_of<dom_data_type,uncvref_t<_ValueType>>::value,int> =0>
	_Return value(const typename object_t::key_type& key,_ValueType&& default_value) const {
		if (is_object()) {
			const auto it=find(key);
			if (it!=cend()) return it->template get<_Return>();
			return std::forward<_ValueType>(default_value);
		}
		throw std::invalid_argument("Cannot use value()");
	}
	template <typename _ValueType,typename _KeyType,std::enable_if_t<is_usable_as_key_type<_KeyType>::value && is_getable<self_t,_ValueType>::value && !std::is_base_of<dom_data_type,uncvref_t<_ValueType>>::value,int> =0>
	_ValueType value(_KeyType&& key,const _ValueType& default_value) const {
		if (is_object()) {
			const auto it=find(std::forward<_KeyType>(key));
			if (it!=cend()) return it->template get<_ValueType>();
			return default_value;
		}
		throw std::invalid_argument("Cannot use value()");
	}
	template <typename _ValueType,typename _KeyType,typename _Return=typename value_return_type<_ValueType>::type,std::enable_if_t<is_usable_as_key_type<_KeyType>::value && is_getable<self_t,_Return>::value && !std::is_base_of<dom_data_type,uncvref_t<_ValueType>>::value,int> =0>
	_Return value(_KeyType&& key,_ValueType&& default_value) const {
		if (is_object()) {
			const auto it=find(std::forward<_KeyType>(key));
			if (it!=cend()) return it->template get<_Return>();
			return std::forward<_ValueType>(default_value);
		}
		throw std::invalid_argument("Cannot use value()");
	}
	template <typename _ValueType,std::enable_if_t<is_getable<self_t,_ValueType>::value && !std::is_base_of<dom_data_type,uncvref_t<_ValueType>>::value,int> =0>
	_ValueType value(const dom_pointer_t& ptr,const _ValueType& default_value) const {
		if (is_object()) {
			try {
				return ptr.get_checked(this).template get<_ValueType>();
			} catch (std::out_of_range&) {
				return default_value;
			}
		}
		throw std::invalid_argument("Cannot use value()");
	}
	template <typename _ValueType,typename _Return=typename value_return_type<_ValueType>::type,std::enable_if_t<is_getable<self_t,_Return>::value && !std::is_base_of<dom_data_type,uncvref_t<_ValueType>>::value,int> =0>
	_Return value(const dom_pointer_t& ptr,_ValueType&& default_value) const {
		if (is_object()) {
			try {
				return ptr.get_checked(this).template get<_Return>();
			} catch (std::out_of_range&) {
				return std::forward<_ValueType>(default_value);
			}
		}
		throw std::invalid_argument("Cannot use value()");
	}

	template <typename _ValueType,typename _RefType,std::enable_if_t<is_dom<_RefType>::value && std::is_same<typename _RefType::string_t,string_t>::value && is_getable<self_t,_ValueType>::value && !std::is_base_of<dom_data_type,uncvref_t<_ValueType>>::value,int> =0>
	_ValueType value(const structure::dom_pointer<_RefType>& ptr,const _ValueType& default_value) const {
		return value(ptr.convert(),default_value);
	}
	template <typename _ValueType,typename _RefType,typename _Return=typename value_return_type<_ValueType>::type,std::enable_if_t<is_dom<_RefType>::value && std::is_same<typename _RefType::string_t,string_t>::value && is_getable<self_t,_Return>::value && !std::is_base_of<dom_data_type,uncvref_t<_ValueType>>::value,int> =0>
	_Return value(const structure::dom_pointer<_RefType>& ptr,_ValueType&& default_value) const {
		return value(ptr.convert(),std::forward<_ValueType>(default_value));
	}

	template <typename _ValueType,typename _Up=uncvref_t<_ValueType>>
	bool can_get() const noexcept {
		if constexpr (std::is_same<_Up,dom>::value) {
			return true;
		} else if constexpr (std::is_same<_Up,std::nullptr_t>::value) {
			return is_null();
		} else if constexpr (std::is_same<_Up,boolean_t>::value) {
			return is_boolean();
		} else if constexpr (std::is_same<_Up,object_t>::value) {
			return is_object();
		} else if constexpr (std::is_same<_Up,array_t>::value) {
			return is_array();
		} else if constexpr (std::is_same<_Up,string_t>::value) {
			return is_string();
		} else if constexpr (std::is_arithmetic<_Up>::value) {
			return is_number() || is_boolean();
		} else {
			return false;
		}
	}

	template <typename _ValueType,typename _Return=typename value_return_type<_ValueType>::type,std::enable_if_t<is_getable<self_t,_Return>::value && !std::is_base_of<dom_data_type,uncvref_t<_ValueType>>::value,int> =0>
	_Return get_or(_ValueType&& default_value) const {
		if (can_get<_Return>()) return get<_Return>();
		return _Return(std::forward<_ValueType>(default_value));
	}

	template <typename _ValueType,std::enable_if_t<is_getable<self_t,_ValueType>::value && !std::is_base_of<dom_data_type,uncvref_t<_ValueType>>::value,int> =0>
	std::optional<_ValueType> try_get() const {
		if (can_get<_ValueType>()) return get<_ValueType>();
		return std::nullopt;
	}

	template <typename _ValueType,typename _Return=typename value_return_type<_ValueType>::type,std::enable_if_t<is_getable<self_t,_Return>::value && !std::is_base_of<dom_data_type,uncvref_t<_ValueType>>::value,int> =0>
	_Return value_or(const typename object_t::key_type& key,_ValueType&& default_value) const {
		if (!is_object()) return _Return(std::forward<_ValueType>(default_value));
		const auto it=find(key);
		if (it==cend()) return _Return(std::forward<_ValueType>(default_value));
		return it->get_or(std::forward<_ValueType>(default_value));
	}

	template <typename _ValueType,typename _KeyType,typename _Return=typename value_return_type<_ValueType>::type,std::enable_if_t<is_usable_as_key_type<_KeyType>::value && is_getable<self_t,_Return>::value && !std::is_base_of<dom_data_type,uncvref_t<_ValueType>>::value,int> =0>
	_Return value_or(_KeyType&& key,_ValueType&& default_value) const {
		if (!is_object()) return _Return(std::forward<_ValueType>(default_value));
		const auto it=find(std::forward<_KeyType>(key));
		if (it==cend()) return _Return(std::forward<_ValueType>(default_value));
		return it->get_or(std::forward<_ValueType>(default_value));
	}

	template <typename _ValueType,typename _Return=typename value_return_type<_ValueType>::type,std::enable_if_t<is_getable<self_t,_Return>::value && !std::is_base_of<dom_data_type,uncvref_t<_ValueType>>::value,int> =0>
	_Return value_or(const dom_pointer_t& ptr,_ValueType&& default_value) const {
		if (!ptr.contains(this)) return _Return(std::forward<_ValueType>(default_value));
		return ptr.get_unchecked(static_cast<const dom*>(this)).get_or(std::forward<_ValueType>(default_value));
	}

	template <typename _ValueType,typename _RefType,typename _Return=typename value_return_type<_ValueType>::type,std::enable_if_t<is_dom<_RefType>::value && std::is_same<typename _RefType::string_t,string_t>::value && is_getable<self_t,_Return>::value && !std::is_base_of<dom_data_type,uncvref_t<_ValueType>>::value,int> =0>
	_Return value_or(const structure::dom_pointer<_RefType>& ptr,_ValueType&& default_value) const {
		return value_or(ptr.convert(),std::forward<_ValueType>(default_value));
	}

	ref front() {
		return *begin();
	}
	const_ref front() const {
		return *cbegin();
	}
	ref back() {
		auto temp=end();
		temp--;
		return *temp;
	}
	const_ref back() const {
		auto temp=cend();
		temp--;
		return *temp;
	}

	template <typename _IteratorType,std::enable_if_t<std::is_same<_IteratorType,typename self_t::iterator>::value || std::is_same<_IteratorType,typename self_t::const_iterator>::value,int> =0>
	_IteratorType erase(_IteratorType pos) {
		if (this!=pos.object_) throw std::invalid_argument("Iterator does not fit current value");
		_IteratorType result=end();
		switch (type()) {
			case DDT_BOOL:
			case DDT_FLOAT:
			case DDT_INT:
			case DDT_STRING: {
				if (!pos.it_.primitive_iterator_.is_begin()) throw std::out_of_range("Iterator out of range");
				if (is_string()) {
					allocator_t<string_t> alloc;
					std::allocator_traits<allocator_t<string_t>>::destroy(alloc,value().string);
					std::allocator_traits<allocator_t<string_t>>::deallocate(alloc,value().string,1);
					value().string=nullptr;
				}
				data_.type=DDT_NULL;
				break;
			}
			case DDT_OBJECT: {
				result.it_.object_iterator_=value().object->erase(pos.it_.object_iterator_);
				break;
			}
			case DDT_ARRAY: {
				result.it_.array_iterator_=value().array->erase(pos.it_.array_iterator_);
				break;
			}
			case DDT_NULL:
			default: throw std::invalid_argument("Cannot use erase()");
		}
		return result;
	}
	template <typename _IteratorType,std::enable_if_t<std::is_same<_IteratorType,typename self_t::iterator>::value || std::is_same<_IteratorType,typename self_t::const_iterator>::value,int> =0>
	_IteratorType erase(_IteratorType first,_IteratorType last) {
		if (this!=first.object_ || this!=last.object_) throw std::invalid_argument("Iterators do not fit current value");
		_IteratorType result=end();
		switch (type()) {
			case DDT_BOOL:
			case DDT_FLOAT:
			case DDT_INT:
			case DDT_STRING: {
				if (!first.it_.primitive_iterator_.is_begin() || !last.it_.primitive_iterator_.is_end()) throw std::out_of_range("Iterators out of range");
				if (is_string()) {
					allocator_t<string_t> alloc;
					std::allocator_traits<allocator_t<string_t>>::destroy(alloc,value().string);
					std::allocator_traits<allocator_t<string_t>>::deallocate(alloc,value().string,1);
					value().string=nullptr;
				}
				data_.type=DDT_NULL;
				break;
			}
			case DDT_OBJECT: {
				result.it_.object_iterator_=value().object->erase(first.it_.object_iterator_,last.it_.object_iterator_);
				break;
			}
			case DDT_ARRAY: {
				result.it_.array_iterator_=value().array->erase(first.it_.array_iterator_,last.it_.array_iterator_);
				break;
			}
			case DDT_NULL:
			default: throw std::invalid_argument("Cannot use erase()");
		}
		return result;
	}

private:
	template <typename _KeyType,std::enable_if_t<has_erase_with_key_type<object_t,_KeyType>::value,int> =0>
	size_type erase_internal(_KeyType&& key) {
		if (!is_object()) throw std::invalid_argument("Cannot use erase()");
		return value().object->erase(std::forward<_KeyType>(key));
	}
	template <typename _KeyType,std::enable_if_t<!has_erase_with_key_type<object_t,_KeyType>::value,int> =0>
	size_type erase_internal(_KeyType&& key) {
		if (!is_object()) throw std::invalid_argument("Cannot use erase()");
		const auto it=value().object->find(std::forward<_KeyType>(key));
		if (it!=value().object->end()) {
			value().object->erase(it);
			return 1;
		}
		return 0;
	}

public:
	size_type erase(const typename object_t::key_type& key) {
		return erase_internal(key);
	}
	template <typename _KeyType,std::enable_if_t<is_usable_as_key_type<_KeyType>::value,int> =0>
	size_type erase(_KeyType&& key) {
		return erase_internal(std::forward<_KeyType>(key));
	}
	void erase(const size_type index) {
		if (is_array()) {
			if (index>=size()) throw std::out_of_range("Array index is out of range");
			value().array->erase(value().array->begin()+static_cast<difference_type>(index));
		} else throw std::invalid_argument("Cannot use erase()");
	}

	iterator find(const typename object_t::key_type& key) {
		auto result=end();
		if (is_object()) result.it_.object_iterator_=value().object->find(key);
		return result;
	}
	const_iterator find(const typename object_t::key_type& key) const {
		auto result=cend();
		if (is_object()) result.it_.object_iterator_=value().object->find(key);
		return result;
	}
	template <typename _KeyType,std::enable_if_t<is_usable_as_key_type<_KeyType>::value,int> =0>
	iterator find(_KeyType&& key) {
		auto result=end();
		if (is_object()) result.it_.object_iterator_=value().object->find(std::forward<_KeyType>(key));
		return result;
	}
	template <typename _KeyType,std::enable_if_t<is_usable_as_key_type<_KeyType>::value,int> =0>
	const_iterator find(_KeyType&& key) const {
		auto result=cend();
		if (is_object()) result.it_.object_iterator_=value().object->find(std::forward<_KeyType>(key));
		return result;
	}

	size_type count(const typename object_t::key_type& key) const {
		return is_object()?value().object->count(key):0;
	}
	template <typename _KeyType,std::enable_if_t<is_usable_as_key_type<_KeyType>::value,int> =0>
	size_type count(_KeyType&& key) const {
		return is_object()?value().object->count(std::forward<_KeyType>(key)):0;
	}

	bool contains(const typename object_t::key_type& key) const {
		return is_object() && value().object->find(key)!=value().object->end();
	}
	template <typename _KeyType,std::enable_if_t<is_usable_as_key_type<_KeyType>::value,int> =0>
	bool contains(_KeyType&& key) const {
		return is_object() && value().object->find(std::forward<_KeyType>(key))!=value().object->end();
	}
	bool contains(const dom_pointer_t& ptr) const {
		return ptr.contains(this);
	}
	template <typename _RefType,std::enable_if_t<is_dom<_RefType>::value && std::is_same<typename _RefType::string_t,string_t>::value,int> =0>
	bool contains(const structure::dom_pointer<_RefType>& ptr) const {
		return ptr.convert().contains(this);
	}

	iterator begin() noexcept {
		iterator result(this);
		result.set_begin();
		return result;
	}
	const_iterator begin() const noexcept {
		return cbegin();
	}
	const_iterator cbegin() const noexcept {
		const_iterator result(this);
		result.set_begin();
		return result;
	}
	iterator end() noexcept {
		iterator result(this);
		result.set_end();
		return result;
	}
	const_iterator end() const noexcept {
		return cend();
	}
	const_iterator cend() const noexcept {
		const_iterator result(this);
		result.set_end();
		return result;
	}
	reverse_iterator rbegin() noexcept {
		return reverse_iterator(end());
	}
	const_reverse_iterator rbegin() const noexcept {
		return crbegin();
	}
	reverse_iterator rend() noexcept {
		return reverse_iterator(begin());
	}
	const_reverse_iterator rend() const noexcept {
		return crend();
	}
	const_reverse_iterator crbegin() const noexcept {
		return const_reverse_iterator(cend());
	}
	const_reverse_iterator crend() const noexcept {
		return const_reverse_iterator(cbegin());
	}

	dom_iteration_proxy<iterator> items() noexcept {
		return dom_iteration_proxy<iterator>(*this);
	}
	dom_iteration_proxy<const_iterator> items() const noexcept {
		return dom_iteration_proxy<const_iterator>(*this);
	}

	virtual bool empty() const noexcept {
		switch (type()) {
			case DDT_NULL: return true;
			case DDT_ARRAY: return value().array->empty();
			case DDT_OBJECT: return value().object->empty();
			case DDT_STRING:
			case DDT_BOOL:
			case DDT_INT:
			case DDT_FLOAT:
			default: return false;
		}
	}
	virtual size_type size() const noexcept {
		switch (type()) {
			case DDT_NULL: return 0;
			case DDT_ARRAY: return value().array->size();
			case DDT_OBJECT: return value().object->size();
			case DDT_STRING:
			case DDT_BOOL:
			case DDT_INT:
			case DDT_FLOAT:
			default: return 1;
		}
	}
	virtual size_type max_size() const noexcept {
		switch (type()) {
			case DDT_ARRAY: return value().array->max_size();
			case DDT_OBJECT: return value().object->max_size();
			case DDT_NULL:
			case DDT_STRING:
			case DDT_BOOL:
			case DDT_INT:
			case DDT_FLOAT:
			default: return size();
		}
	}
	virtual void clear() noexcept {
		switch (type()) {
			case DDT_INT: value().integer=static_cast<int_t>(0);break;
			case DDT_FLOAT: value().floating=static_cast<float_t>(0.0);break;
			case DDT_BOOL: value().boolean=static_cast<boolean_t>(false);break;
			case DDT_STRING: value().string->clear();break;
			case DDT_ARRAY: value().array->clear();break;
			case DDT_OBJECT: value().object->clear();break;
			case DDT_NULL:
			default: break;
		}
	}

	void push_back(dom&& val) {
		if (!(is_null() || is_array())) throw std::invalid_argument("Cannot use push_back()");
		if (is_null()) morph(DDT_ARRAY);
		value().array->push_back(std::move(val));
	}
	ref operator +=(dom&& val) {
		push_back(std::move(val));
		return *this;
	}
	void push_back(const dom& val) {
		if (!(is_null() || is_array())) throw std::invalid_argument("Cannot use push_back()");
		if (is_null()) morph(DDT_ARRAY);
		value().array->push_back(val);
	}
	ref operator +=(const dom& val) {
		push_back(val);
		return *this;
	}
	void push_back(const typename object_t::value_type& val) {
		if (!(is_null() || is_object())) throw std::invalid_argument("Cannot use push_back()");
		if (is_null()) morph(DDT_OBJECT);
		value().object->insert(val);
	}
	ref operator +=(const typename object_t::value_type& val) {
		push_back(val);
		return *this;
	}
	void push_back(initializer_list_t init_list) {
		if (is_object() && init_list.size()==2 && (*init_list.begin())->is_string()) {
			dom&& key=init_list.begin()->moved_or_copied();
			push_back(typename object_t::value_type(std::move(key.get_ref<string_t&>()),(init_list.begin()+1)->moved_or_copied()));
		} else push_back(dom(init_list));
	}
	ref operator +=(initializer_list_t init_list) {
		push_back(init_list);
		return *this;
	}
	template <typename... _Args>
	ref emplace_back(_Args&&... args) {
		if (!(is_null() || is_array())) throw std::invalid_argument("Cannot use emplace_back()");
		if (is_null()) morph(DDT_ARRAY);
		value().array->emplace_back(std::forward<_Args>(args)...);
		return value().array->back();
	}
	template <typename... _Args>
	std::pair<iterator,bool> emplace(_Args&&... args) {
		if (!(is_null() || is_object())) throw std::invalid_argument("Cannot use emplace()");
		if (is_null()) morph(DDT_OBJECT);
		auto res=value().object->emplace(std::forward<_Args>(args)...);
		auto it=begin();
		it.it_.object_iterator_=res.first;
		return {it,res.second};
	}

private:
	template <typename... _Args>
	iterator insert_iterator(const_iterator pos,_Args&&... args) {
		iterator result(this);
		auto insert_pos=std::distance(value().array->begin(),pos.it_.array_iterator_);
		value().array->insert(pos.it_.array_iterator_,std::forward<_Args>(args)...);
		result.it_.array_iterator_=value().array->begin()+insert_pos;
		return result;
	}

public:
	iterator insert(const_iterator pos,const dom& val) {
		if (is_array()) {
			if (pos.object_!=this) throw std::invalid_argument("Iterator does not fit current value");
			return insert_iterator(pos,val);
		}
		throw std::invalid_argument("Cannot use insert()");
	}
	iterator insert(const_iterator pos,dom&& val) {
		if (is_array()) {
			if (pos.object_!=this) throw std::invalid_argument("Iterator does not fit current value");
			return insert_iterator(pos,std::move(val));
		}
		throw std::invalid_argument("Cannot use insert()");
	}
	iterator insert(const_iterator pos,size_type cnt,const dom& val) {
		if (is_array()) {
			if (pos.object_!=this) throw std::invalid_argument("Iterator does not fit current value");
			return insert_iterator(pos,cnt,val);
		}
		throw std::invalid_argument("Cannot use insert()");
	}
	iterator insert(const_iterator pos,const_iterator first,const_iterator last) {
		if (!is_array()) throw std::invalid_argument("Cannot use insert()");
		if (pos.object_!=this) throw std::invalid_argument("Iterator does not fit current value");
		if (first.object_!=last.object_) throw std::invalid_argument("Iterators do not fit");
		if (first.object_==this) throw std::invalid_argument("Passed iterators may not belong to container");
		return insert_iterator(pos,first.it_.array_iterator_,last.it_.array_iterator_);
	}
	iterator insert(const_iterator pos,initializer_list_t init_list) {
		if (!is_array()) throw std::invalid_argument("Cannot use insert()");
		if (pos.object_!=this) throw std::invalid_argument("Iterator does not fit current value");
		return insert_iterator(pos,init_list.begin(),init_list.end());
	}
	void insert(const_iterator first,const_iterator last) {
		if (!is_object()) throw std::invalid_argument("Cannot use insert()");
		if (first.object_!=last.object_) throw std::invalid_argument("Iterators do not fit");
		if (first.object_->type()!=DDT_OBJECT) throw std::invalid_argument("Iterators first and last must point to objects");
		value().object->insert(first.it_.object_iterator_,last.it_.object_iterator_);
	}

	void update(const_ref j,bool merge_objects=false) {
		update(j.begin(),j.end(),merge_objects);
	}
	void update(const_iterator first,const_iterator last,bool merge_objects=false) {
		if (is_null()) morph(DDT_OBJECT);
		if (!is_object()) throw std::invalid_argument("Cannot use update()");
		if (first.object_!=last.object_) throw std::invalid_argument("Iterators do not fit");
		if (first.object_->type()!=DDT_OBJECT) throw std::invalid_argument("Cannot use update()");
		for (auto it=first;it!=last;it++) {
			if (merge_objects && it.value().type()==DDT_OBJECT) {
				auto jt=value().object->find(it.key());
				if (jt!=value().object->end()) {
					jt->second.update(it.value(),true);
					continue;
				}
			}
			value().object->operator [](it.key())=it.value();
		}
	}

	void swap(ref other) noexcept {
		std::swap(data_.type,other.data_.type);
		std::swap(data_.value,other.data_.value);
	}
	friend void swap(ref left,ref right) noexcept {
		left.swap(right);
	}
	void swap(array_t& other) {
		if (is_array()) std::swap(*(value().array),other);
		else throw std::invalid_argument("Cannot use swap(array_t&)");
	}
	void swap(object_t& other) {
		if (is_object()) std::swap(*(value().object),other);
		else throw std::invalid_argument("Cannot use swap(object_t&)");
	}
	void swap(string_t& other) {
		if (is_string()) std::swap(*(value().string),other);
		else throw std::invalid_argument("Cannot use swap(string_t&)");
	}

private:
	static bool compares_unordered(const_ref lhs,const_ref rhs,bool inverse=false) noexcept {
		static_cast<void>(inverse);
		return (lhs.is_float() && std::isnan(lhs.value().floating) && rhs.is_number()) || (rhs.is_float() && std::isnan(rhs.value().floating) && lhs.is_number());
	}
	bool compares_unordered(const_ref rhs,bool inverse=false) const noexcept {
		return compares_unordered(*this,rhs,inverse);
	}
#if _STDEX_GNU_COMPILER
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
#endif
	static bool equal_impl(const_ref lhs,const_ref rhs) noexcept {
		const auto lhs_type=lhs.type();
		const auto rhs_type=rhs.type();
		if (lhs_type==rhs_type) {
			switch (lhs_type) {
				case DDT_ARRAY: return *lhs.value().array==*rhs.value().array;
				case DDT_OBJECT: return *lhs.value().object==*rhs.value().object;
				case DDT_NULL: return true;
				case DDT_STRING: return *lhs.value().string==*rhs.value().string;
				case DDT_BOOL: return lhs.value().boolean==rhs.value().boolean;
				case DDT_INT: return lhs.value().integer==rhs.value().integer;
				case DDT_FLOAT: return lhs.value().floating==rhs.value().floating;
				default: return false;
			}
		} else if (lhs_type==DDT_INT && rhs_type==DDT_FLOAT) return static_cast<float_t>(lhs.value().integer)==rhs.value().floating;
		else if (lhs_type==DDT_FLOAT && rhs_type==DDT_INT) return lhs.value().floating==static_cast<float_t>(rhs.value().integer);
		return false;
	}
#if _STDEX_GNU_COMPILER
#pragma GCC diagnostic pop
#endif
	static bool less_impl(const_ref lhs,const_ref rhs) noexcept {
		const auto lhs_type=lhs.type();
		const auto rhs_type=rhs.type();
		if (lhs_type==rhs_type) {
			switch (lhs_type) {
				case DDT_ARRAY: return *lhs.value().array<*rhs.value().array;
				case DDT_OBJECT: return *lhs.value().object<*rhs.value().object;
				case DDT_NULL: return false;
				case DDT_STRING: return *lhs.value().string<*rhs.value().string;
				case DDT_BOOL: return lhs.value().boolean<rhs.value().boolean;
				case DDT_INT: return lhs.value().integer<rhs.value().integer;
				case DDT_FLOAT: return lhs.value().floating<rhs.value().floating;
				default: return false;
			}
		} else if (lhs_type==DDT_INT && rhs_type==DDT_FLOAT) return static_cast<float_t>(lhs.value().integer)<rhs.value().floating;
		else if (lhs_type==DDT_FLOAT && rhs_type==DDT_INT) return lhs.value().floating<static_cast<float_t>(rhs.value().integer);
		if (compares_unordered(lhs,rhs)) return false;
		return lhs_type<rhs_type;
	}

public:
#if __cplusplus>=_STDEX_CPP20_VERSION
	bool operator ==(const_ref rhs) const noexcept {
		return equal_impl(*this,rhs);
	}
	bool operator ==(dom_data_type rhs) const noexcept {
		return type()==rhs;
	}
	bool operator ==(dom_data_type::enumeration_type rhs) const noexcept {
		return type()==rhs;
	}
	template <typename _Scalar>
	requires is_dom_scalar<_Scalar>::value
	bool operator ==(_Scalar rhs) const noexcept {
		return *this==dom(rhs);
	}

	std::partial_ordering operator <=>(const_ref rhs) const noexcept {
		if (equal_impl(*this,rhs)) return std::partial_ordering::equivalent;
		if (compares_unordered(*this,rhs)) return std::partial_ordering::unordered;
		if (less_impl(*this,rhs)) return std::partial_ordering::less;
		return std::partial_ordering::greater;
	}
	template <typename _Scalar>
	requires is_dom_scalar<_Scalar>::value
	std::partial_ordering operator <=>(_Scalar rhs) const noexcept {
		return *this<=>dom(rhs);
	}
#else
	friend bool operator ==(const_ref lhs,const_ref rhs) noexcept {
		return equal_impl(lhs,rhs);
	}
	friend bool operator ==(const_ref lhs,dom_data_type rhs) noexcept {
		return lhs.type()==rhs;
	}
	friend bool operator ==(dom_data_type lhs,const_ref rhs) noexcept {
		return lhs==rhs.type();
	}
	friend bool operator !=(const_ref lhs,dom_data_type rhs) noexcept {
		return !(lhs==rhs);
	}
	friend bool operator !=(dom_data_type lhs,const_ref rhs) noexcept {
		return !(lhs==rhs);
	}
	friend bool operator ==(const_ref lhs,dom_data_type::enumeration_type rhs) noexcept {
		return lhs.type()==rhs;
	}
	friend bool operator ==(dom_data_type::enumeration_type lhs,const_ref rhs) noexcept {
		return rhs.type()==lhs;
	}
	friend bool operator !=(const_ref lhs,dom_data_type::enumeration_type rhs) noexcept {
		return !(lhs==rhs);
	}
	friend bool operator !=(dom_data_type::enumeration_type lhs,const_ref rhs) noexcept {
		return !(lhs==rhs);
	}
	template <typename _Scalar,std::enable_if_t<is_dom_scalar<_Scalar>::value,int> =0>
	friend bool operator ==(const_ref lhs,_Scalar rhs) noexcept {
		return lhs==dom(rhs);
	}
	template <typename _Scalar,std::enable_if_t<is_dom_scalar<_Scalar>::value,int> =0>
	friend bool operator ==(_Scalar lhs,const_ref rhs) noexcept {
		return dom(lhs)==rhs;
	}
	friend bool operator !=(const_ref lhs,const_ref rhs) noexcept {
		return !(lhs==rhs);
	}
	template <typename _Scalar,std::enable_if_t<is_dom_scalar<_Scalar>::value,int> =0>
	friend bool operator !=(const_ref lhs,_Scalar rhs) noexcept {
		return lhs!=dom(rhs);
	}
	template <typename _Scalar,std::enable_if_t<is_dom_scalar<_Scalar>::value,int> =0>
	friend bool operator !=(_Scalar lhs,const_ref rhs) noexcept {
		return dom(lhs)!=rhs;
	}
	friend bool operator <(const_ref lhs,const_ref rhs) noexcept {
		if (compares_unordered(lhs,rhs)) return false;
		return less_impl(lhs,rhs);
	}
	template <typename _Scalar,std::enable_if_t<is_dom_scalar<_Scalar>::value,int> =0>
	friend bool operator <(const_ref lhs,_Scalar rhs) noexcept {
		return lhs<dom(rhs);
	}
	template <typename _Scalar,std::enable_if_t<is_dom_scalar<_Scalar>::value,int> =0>
	friend bool operator <(_Scalar lhs,const_ref rhs) noexcept {
		return dom(lhs)<rhs;
	}
	friend bool operator <=(const_ref lhs,const_ref rhs) noexcept {
		if (compares_unordered(lhs,rhs,true)) return false;
		return !(rhs<lhs);
	}
	template <typename _Scalar,std::enable_if_t<is_dom_scalar<_Scalar>::value,int> =0>
	friend bool operator <=(const_ref lhs,_Scalar rhs) noexcept {
		return lhs<=dom(rhs);
	}
	template <typename _Scalar,std::enable_if_t<is_dom_scalar<_Scalar>::value,int> =0>
	friend bool operator <=(_Scalar lhs,const_ref rhs) noexcept {
		return dom(lhs)<=rhs;
	}
	friend bool operator >(const_ref lhs,const_ref rhs) noexcept {
		if (compares_unordered(lhs,rhs)) return false;
		return !(lhs<=rhs);
	}
	template <typename _Scalar,std::enable_if_t<is_dom_scalar<_Scalar>::value,int> =0>
	friend bool operator >(const_ref lhs,_Scalar rhs) noexcept {
		return lhs>dom(rhs);
	}
	template <typename _Scalar,std::enable_if_t<is_dom_scalar<_Scalar>::value,int> =0>
	friend bool operator >(_Scalar lhs,const_ref rhs) noexcept {
		return dom(lhs)>rhs;
	}
	friend bool operator >=(const_ref lhs,const_ref rhs) noexcept {
		if (compares_unordered(lhs,rhs,true)) return false;
		return !(lhs<rhs);
	}
	template <typename _Scalar,std::enable_if_t<is_dom_scalar<_Scalar>::value,int> =0>
	friend bool operator >=(const_ref lhs,_Scalar rhs) noexcept {
		return lhs>=dom(rhs);
	}
	template <typename _Scalar,std::enable_if_t<is_dom_scalar<_Scalar>::value,int> =0>
	friend bool operator >=(_Scalar lhs,const_ref rhs) noexcept {
		return dom(lhs)>=rhs;
	}
#endif

	_STDEX_RETURNS_NON_NULL
	virtual const char* type_name() const noexcept {
		switch (type()) {
			case DDT_NULL: return "NULL";
			case DDT_OBJECT: return "OBJECT";
			case DDT_ARRAY: return "ARRAY";
			case DDT_STRING: return "STRING";
			case DDT_BOOL: return "BOOL";
			case DDT_INT: return "INT";
			case DDT_FLOAT: return "FLOAT";
			default: return "UNKNOWN";
		}
	}

	ref operator [](const dom_pointer_t& ptr) {
		return ptr.get_unchecked(this);
	}
	const_ref operator [](const dom_pointer_t& ptr) const {
		return ptr.get_unchecked(this);
	}
	template <typename _RefType,std::enable_if_t<is_dom<_RefType>::value && std::is_same<typename _RefType::string_t,string_t>::value,int> =0>
	ref operator [](const structure::dom_pointer<_RefType>& ptr) {
		return ptr.convert().get_unchecked(this);
	}
	template <typename _RefType,std::enable_if_t<is_dom<_RefType>::value && std::is_same<typename _RefType::string_t,string_t>::value,int> =0>
	const_ref operator [](const structure::dom_pointer<_RefType>& ptr) const {
		return ptr.convert().get_unchecked(this);
	}
	ref at(const dom_pointer_t& ptr) {
		return ptr.get_checked(this);
	}
	const_ref at(const dom_pointer_t& ptr) const {
		return ptr.get_checked(this);
	}
	template <typename _RefType,std::enable_if_t<is_dom<_RefType>::value && std::is_same<typename _RefType::string_t,string_t>::value,int> =0>
	ref at(const structure::dom_pointer<_RefType>& ptr) {
		return ptr.convert().get_checked(this);
	}
	template <typename _RefType,std::enable_if_t<is_dom<_RefType>::value && std::is_same<typename _RefType::string_t,string_t>::value,int> =0>
	const_ref at(const structure::dom_pointer<_RefType>& ptr) const {
		return ptr.convert().get_checked(this);
	}

	[[nodiscard]]
	dom flatten() const {
		dom result(DDT_OBJECT);
		dom_pointer_t::flatten(string_t(),*this,result);
		return result;
	}
	[[nodiscard]]
	dom unflatten() const {
		return dom_pointer_t::unflatten(*this);
	}

	template <typename _Vp>
	static _Vp path_escape(_Vp s) {
		return dom_path_escape(std::move(s));
	}
	template <typename _Vp>
	static _Vp path_unescape(_Vp s) {
		return dom_path_unescape(std::move(s));
	}

private:
	static string_t index_to_key(std::size_t index) {
		const std::string index_string=std::to_string(index);
		return string_t(index_string.begin(),index_string.end());
	}

public:
	virtual void patch_inplace(const dom& dom_patch) {
		dom& result=*this;
		enum patch_operations {
			PO_ADD,
			PO_REMOVE,
			PO_REPLACE,
			PO_MOVE,
			PO_COPY,
			PO_TEST,
			PO_INVALID,
		};
		const auto get_op=[](const string_t& op){
			if (op==string_t{"add"}) return PO_ADD;
			if (op==string_t{"remove"}) return PO_REMOVE;
			if (op==string_t{"replace"}) return PO_REPLACE;
			if (op==string_t{"move"}) return PO_MOVE;
			if (op==string_t{"copy"}) return PO_COPY;
			if (op==string_t{"test"}) return PO_TEST;
			return PO_INVALID;
		};
		const auto operation_add=[&result](dom_pointer_t& ptr,dom val){
			if (ptr.empty()) {
				result=std::move(val);
				return;
			}
			const dom_pointer_t top_pointer=ptr.top();
			if (top_pointer!=ptr) result.at(top_pointer);
			const auto last_path=ptr.back();
			ptr.pop_back();
			dom& parent=result.at(ptr);
			switch (parent.type()) {
				case DDT_NULL:
				case DDT_OBJECT: parent[last_path]=std::move(val);break;
				case DDT_ARRAY: {
					if (dom_pointer_t::token_is_dash(last_path)) parent.push_back(std::move(val));
					else {
						const auto index=dom_pointer_t::template array_index<self_t>(last_path);
						if (index>parent.size()) throw std::out_of_range("Array index is out of range");
						parent.insert(parent.begin()+static_cast<difference_type>(index),std::move(val));
					}
					break;
				}
				case DDT_STRING:
				case DDT_BOOL:
				case DDT_INT:
				case DDT_FLOAT:
				default: break;
			}
		};
		const auto operation_remove=[&result](dom_pointer_t& ptr){
			const auto last_path=ptr.back();
			ptr.pop_back();
			dom& parent=result.at(ptr);
			if (parent.type()==DDT_OBJECT) {
				auto it=parent.find(last_path);
				if (it!=parent.end()) parent.erase(it);
				else throw std::out_of_range("Key not found");
			} else if (parent.type()==DDT_ARRAY) parent.erase(dom_pointer_t::template array_index<self_t>(last_path));
		};
		if (dom_patch.type()!=DDT_ARRAY) throw std::invalid_argument("Patch must be an array of objects");
		for (auto patch_it=dom_patch.cbegin();patch_it!=dom_patch.cend();patch_it++) {
			const dom& val=patch_it.value();
			const auto get_value=[&val](const string_t& op,const string_t& member,bool string_type)->const dom& {
				auto jt=val.value().object->find(member);
				if (jt==val.value().object->end()) throw std::invalid_argument("Patch operation must have member");
				if (string_type && jt->second.type()!=DDT_STRING) throw std::invalid_argument("Patch operation must have string member");
				static_cast<void>(op);
				return jt->second;
			};
			if (val.type()!=DDT_OBJECT) throw std::invalid_argument("Patch must be an array of objects");
			const auto op=get_value(string_t{"op"},string_t{"op"},true).template get<string_t>();
			const auto path=get_value(op,string_t{"path"},true).template get<string_t>();
			dom_pointer_t ptr(path);
			switch (get_op(op)) {
				case PO_ADD: operation_add(ptr,get_value(string_t{"add"},string_t{"value"},false));break;
				case PO_REMOVE: operation_remove(ptr);break;
				case PO_REPLACE: result.at(ptr)=get_value(string_t{"replace"},string_t{"value"},false);break;
				case PO_MOVE: {
					const auto from_path=get_value(string_t{"move"},string_t{"from"},true).template get<string_t>();
					dom_pointer_t from_ptr(from_path);
					dom const v=result.at(from_ptr);
					operation_remove(from_ptr);
					operation_add(ptr,v);
					break;
				}
				case PO_COPY: {
					const auto from_path=get_value(string_t{"copy"},string_t{"from"},true).template get<string_t>();
					const dom_pointer_t from_ptr(from_path);
					dom const v=result.at(from_ptr);
					operation_add(ptr,v);
					break;
				}
				case PO_TEST: {
					bool success=false;
					try {
						success=equal_impl(result.at(ptr),get_value(string_t{"test"},string_t{"value"},false));
					} catch (std::out_of_range&) {
					}
					if (!success) throw std::runtime_error("Unsuccessful patch test operation");
					break;
				}
				case PO_INVALID:
				default: throw std::invalid_argument("Operation value is invalid");
			}
		}
	}
	[[nodiscard]]
	dom patch(const dom& dom_patch) const {
		dom result=*this;
		result.patch_inplace(dom_patch);
		return result;
	}
	[[nodiscard]]
	static dom diff(const dom& source,const dom& target,const string_t& path=string_t()) {
		dom result(DDT_ARRAY);
		if (equal_impl(source,target)) return result;
		const auto sep=static_cast<typename string_t::value_type>('/');
		if (source.type()!=target.type()) {
			result.push_back({{"op","replace"},{"path",path},{"value",target}});
			return result;
		}
		switch (source.type()) {
			case DDT_ARRAY: {
				std::size_t i=0;
				while (i<source.size() && i<target.size()) {
					auto temp_diff=diff(source[static_cast<size_type>(i)],target[static_cast<size_type>(i)],path+sep+index_to_key(i));
					result.insert(result.end(),temp_diff.begin(),temp_diff.end());
					i++;
				}
				const auto end_index=static_cast<difference_type>(result.size());
				while (i<source.size()) {
					result.insert(result.begin()+end_index,object({{"op","remove"},{"path",path+sep+index_to_key(i)}}));
					i++;
				}
				while (i<target.size()) {
					result.push_back({{"op","add"},{"path",path+string_t{"/-"}},{"value",target[static_cast<size_type>(i)]}});
					i++;
				}
				break;
			}
			case DDT_OBJECT: {
				for (auto it=source.cbegin();it!=source.cend();it++) {
					const auto path_key=path+sep+path_escape(it.key());
					if (target.find(it.key())!=target.cend()) {
						auto temp_diff=diff(it.value(),target[it.key()],path_key);
						result.insert(result.end(),temp_diff.begin(),temp_diff.end());
					} else result.push_back(object({{"op","remove"},{"path",path_key}}));
				}
				for (auto it=target.cbegin();it!=target.cend();it++) {
					if (source.find(it.key())==source.cend()) {
						const auto path_key=path+sep+path_escape(it.key());
						result.push_back({{"op","add"},{"path",path_key},{"value",it.value()}});
					}
				}
				break;
			}
			case DDT_NULL:
			case DDT_STRING:
			case DDT_BOOL:
			case DDT_INT:
			case DDT_FLOAT:
			default: {
				result.push_back({{"op","replace"},{"path",path},{"value",target}});
				break;
			}
		}
		return result;
	}
	void merge_patch(const dom& apply_patch) {
		if (apply_patch.type()==DDT_OBJECT) {
			if (!is_object()) *this=object();
			for (auto it=apply_patch.cbegin();it!=apply_patch.cend();it++) {
				if (it.value().type()==DDT_NULL) erase(it.key());
				else operator [](it.key()).merge_patch(it.value());
			}
		} else *this=apply_patch;
	}
};

}

}

namespace std {

#if _STDEX_CLANG_COMPILER
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmismatched-tags"
#endif
template <typename _IteratorType>
class tuple_size<stdex::structure::dom_iteration_proxy_value<_IteratorType>> : public std::integral_constant<std::size_t,2> { };

template <std::size_t _Np,typename _IteratorType>
class tuple_element<_Np,stdex::structure::dom_iteration_proxy_value<_IteratorType>> {
public:
	using type=decltype(get<_Np>(std::declval<stdex::structure::dom_iteration_proxy_value<_IteratorType>>()));
};
#if _STDEX_CLANG_COMPILER
#pragma clang diagnostic pop
#endif

}

#endif