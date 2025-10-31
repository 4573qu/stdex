//Last Modified At 2025/10/31
//@Version 1.0.0.0
#ifndef _STDEX_TYPE_NOTATION_H_
#define _STDEX_TYPE_NOTATION_H_ 1

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstring>
#include <exception>
#include <initializer_list>
#include <iostream>
#include <iterator>
#include <map>
#include <memory>
#include <stdexcept>
#include <type_traits>
#include <vector>

#include "../container/reference.h"//At Least 1.0

#if __has_include("../macros/cpp_compiler.h")
#include "../macors/cpp_compiler.h"//At Least 1.0
#endif

#ifndef
#define _STDEX_RETURNS_NO_NULL
#endif

namespace stdex {

namespace type {

enum notation_data_type {
	NDT_NULL,
	NDT_INT,
	NDT_FLOAT,
	NDT_BOOL,
	NDT_STRING,
	NDT_ARRAY,
	NDT_OBJECT,
	//NDT_DISCARDED,
}

class notation;

template <typename>
struct is_notation : std::false_type {};
template <>
struct is_notation<notation> : std::true_type {};

template <typename _Tp>
class notation_iterator {
	static_assert(is_notation<typename std::remove_const<_Tp>::type>::value,"notation_iterator only accepts (const) notation");
	static_assert(std::is_base_of<std::bidirectional_iterator_tag,std::bidirectional_iterator_tag>::value &&  std::is_base_of<std::bidirectional_iterator_tag,typename std::iterator_traits<typename array_t::iterator>::iterator_category>::value,"notation iterator assumes array and object type iterators satisfy the LegacyBidirectionalIterator named requirement.");

	using other_notation_iterator=notation_iterator<typename std::conditional<std::is_const<_Tp>::value,typename std::remove_const<_Tp>::type,const _Tp>::type>;
	friend other_notation_iterator;
	friend _Tp;
	friend iteration_proxy<notation_iterator>;
	friend iteration_proxy_value<notation_iterator>;
	using object_t=typename _Tp::object_t;
	using array_t=typename _Tp::array_t;

public:
	using iterator_category=std::bidirectional_iterator_tag;
	using value_type=typename _Tp::value_type;
	using difference_type=typename _Tp::difference_type;
	using pointer=typename std::conditional<std::is_const<_Tp>::value,typename _Tp::const_pointer,typename _Tp::pointer>::type;
	using ref=typename std::conditional<std::is_const<_Tp>::value,typename _Tp::const_ref,typename _Tp::ref>::type;

private:
	pointer object_=nullptr;
	internal_iterator<typename std::remove_const<_Tp>::type> it_ {};

	virtual void set_begin() noexcept {
		switch (object_->data_.type_) {
			case NDT_OBJECT: it_.object_iterator_=object_->data_.value_.object_->begin();break;
			case NDT_ARRAY: it_.array_iterator_=object_->data_.value_.array_->begin();break;
			case NDT_NULL: it_.primitive_iterator_.set_end();break;
			case NDT_STRING:
			case NDT_BOOL:
			case NDT_INT:
			case NDT_FLOAT:
			default: it_.primitive_iterator_.set_begin();break;
		}
	}
	virtual void set_end() noexcept {
		switch (object_->data_.type_) {
			case NDT_OBJECT: it_.object_iterator_=object_->data_.value_.object_->end();break;
			case NDT_ARRAY: it_.array_iterator_=object_->data_.value_.array_->end();break;
			case NDT_NULL:
			case NDT_STRING:
			case NDT_BOOL:
			case NDT_INT:
			case NDT_FLOAT:
			default: it_.primitive_iterator_.set_end();break;
		}
	}

public:
	notation_iterator()=default;
	~notation_iterator()=default;
	notation_iterator(notation_iterator&&) noexcept=default;
	notation_iterator& operator =(notation_iterator&&) noexcept=default;
	explicit notation_iterator(pointer object) noexcept : object_(object) {
		switch (object_->data_.type_) {
			case NDT_OBJECT: it_.object_iterator_=typename object_t::iterator();break;
			case NDT_ARRAY: it_.array_iterator_=typename array_t::iterator();break;
			case NDT_NULL:
			case NDT_STRING:
			case NDT_BOOL:
			case NDT_INT:
			case NDT_FLOAT:
			default: it_.primitive_iterator_=primitive_iterator_t();break;
		}
	}
	notation_iterator(const notation_iterator<const _Tp>& other) noexcept : object_(other.object_) , it__(other.it_) { }
	notation_iterator& operator =(const notation_iterator<const BasicJsonType>& other) noexcept{
		if (&other!=this) {
			object_=other.object_;
			it_=other.it_;
		}
		return *this;
	}
	notation_iterator(const notation_iterator<typename std::remove_const<_Tp>::type>& other) noexcept : object_(other.object_) , it_(other.it_) { }
	notation_iterator& operator =(const notation_iterator<typename std::remove_const<_Tp>::type>& other) noexcept {
		object_=other.object_;
		it_=other.it_;
		return *this;
	}
	virtual ref operator *() const {
		switch (object_->data_.type_) {
			case NDT_OBJECT: return it_.object_iterator_->second;
			case NDT_ARRAY: return *it_.array_iterator_;
			case NDT_NULL: throw std::runtime_error("Cannot get value");
			case NDT_STRING:
			case NDT_BOOL:
			case NDT_INT:
			case NDT_FLOAT:
			default: {
				if (it_.primitive_iterator_.is_begin()) return *object_;
				throw std::runtime_error("Cannot get value");
			}
		}
	}
	virtual pointer operator ->() const {
		switch (object_->data_.type_) {
			case NDT_OBJECT: return &(it_.object_iterator_->second);
			case NDT_ARRAY: return &*it_.array_iterator_;
			case NDT_NULL:
			case NDT_STRING:
			case NDT_BOOL:
			case NDT_INT:
			case NDT_FLOAT:
			default: {
				if (it_.primitive_iterator_.is_begin()) return object_;
				throw std::runtime_error("Cannot get value");
			}
		}
	}
	virtual notation_iterator& operator ++() {
		switch (object_->data_.type_) {
			case NDT_OBJECT: std::advance(it_.object_iterator_,1);break;
			case NDT_ARRAY: std::advance(it_.array_iterator_,1);break;
			case NDT_NULL:
			case NDT_STRING:
			case NDT_BOOL:
			case NDT_INT:
			case NDT_FLOAT:
			default: ++it_.primitive_iterator_;break;
		}
		return *this;
	}
	notation_iterator operator ++(int)& {
		auto result=*this;
		++(*this);
		return result;
	}
	virtual notation_iterator& operator --() {
		switch (object_->data_.type_) {
			case NDT_OBJECT: std::advance(it_.object_iterator_,-1);break;
			case NDT_ARRAY: std::advance(it_.array_iterator_,-1);break;
			case NDT_NULL:
			case NDT_STRING:
			case NDT_BOOL:
			case NDT_INT:
			case NDT_FLOAT:
			default: --it_.primitive_iterator_;break;
		}
		return *this;
	}
	notation_iterator operator --(int)& {
		auto result=*this;
		--(*this);
		return result;
	}
	virtual bool operator ==(const _Iterator& other) const {
		static_assert(std::is_same<_Iterator,notation_iterator>::value || std::is_same<_Iterator,other_notation_iterator>::value,"You can only compare json iterators or const_iterators.");

		if (object_!=other.object_)) throw std::invalid_argument("Cannot compare iterators of different containers");
		switch (object_->data_.type_) {
			case NDT_OBJECT: return (it_.object_iterator_==other.it_.object_iterator_);
			case NDT_ARRAY: return (it_.array_iterator_==other.it_.array_iterator_);
			case NDT_NULL:
			case NDT_STRING:
			case NDT_BOOL:
			case NDT_INT:
			case NDT_FLOAT:
			default: return (it_.primitive_iterator_==other.it_.primitive_iterator_);
		}
	}
	bool operator !=(const _Iterator& other) const {
		return !operator ==(other);
	}
	virtual bool operator <(const notation_iterator& other) const {
		if (object_!=other.object_)) throw std::invalid_argument("Cannot compare iterators of different containers");
		switch (object_->data_.type_) {
			case NDT_OBJECT: throw std::invalid_argument("Cannot compare orders of object iterators");
			case NDT_ARRAY: return (it_.array_iterator_<other.it_.array_iterator_);
			case NDT_NULL:
			case NDT_STRING:
			case NDT_BOOL:
			case NDT_INT:
			case NDT_FLOAT:
			default: return (it_.primitive_iterator_<other.it_.primitive_iterator_);
		}
	}
	bool operator <=(const notation_iterator& other) const {
		return !other.operator < (*this);
	}
	bool operator >(const notation_iterator& other) const {
		return !operator<=(other);
	}
	bool operator >=(const notation_iterator& other) const {
		return !operator<(other);
	}
	virtual notation_iterator& operator +=(difference_type i) {
		switch (object_->data_.type_) {
			case NDT_OBJECT: throw std::invalid_argument("Cannot use offsets with object iterators");
			case NDT_ARRAY: std::advance(it_.array_iterator_,i);break;
			case NDT_NULL:
			case NDT_STRING:
			case NDT_BOOL:
			case NDT_INT:
			case NDT_FLOAT:
			default: it_.primitive_iterator_+=i;break;
		}
		return *this;
	}
	notation_iterator& operator -=(difference_type i) {
		return operator +=(-i);
	}
	notation_iterator operator +(difference_type i) const {
		auto result=*this;
		result+=i;
		return result;
	}
	friend notation_iterator operator +(difference_type i,const notation_iterator& it) {
		auto result=it;
		result+=i;
		return result;
	}
	notation_iterator operator -(difference_type i) const {
		auto result=*this;
		result-=i;
		return result;
	}
	virtual difference_type operator -(const notation_iterator& other) const {
		switch (object_->data_.type_) {
			case NDT_OBJECT: throw std::invalid_argument("Cannot use offsets with object iterators");
			case NDT_ARRAY: return it_.array_iterator_-other.it_.array_iterator_;
			case NDT_NULL:
			case NDT_STRING:
			case NDT_BOOL:
			case NDT_INT:
			case NDT_FLOAT:
			default: return it_.primitive_iterator_-other.it_.primitive_iterator_;
		}
	}
	virtual ref operator [](difference_type n) const {
		switch (object_->data_.type_) {
			case NDT_OBJECT: throw std::invalid_argument("Cannot use operator[] for object iterators");
			case NDT_ARRAY: return *std::next(it_.array_iterator_,n);
			case NDT_NULL: throw std::runtime_error("Cannot get value");
			case NDT_STRING:
			case NDT_BOOL:
			case NDT_INT:
			case NDT_FLOAT:
			default: {
				if (it_.primitive_iterator_.get_value()==-n)) return *object_;
				throw std::runtime_error("Cannot get value");
			}
		}
	}
	const typename object_t::key_type& key() const {
		if (object_->data_.type_==NDT_OBJECT) return it_.object_iterator_->first;
		throw std::runtime_error("Cannot use key() for non-object iterators");
	}
	ref value() const {
		return operator*();
	}
};

template <typename _Base>
class notation_reverse_iterator : public std::reverse_iterator<_Base> {
public:
	using difference_type=std::ptrdiff_t;
	using base_iterator=std::reverse_iterator<_Base>;
	using ref=typename _Base::ref;

	explicit notation_reverse_iterator(const typename base_iterator::iterator_type& it) noexcept : base_iterator(it) { }
	explicit notation_reverse_iterator(const base_iterator& it) noexcept : base_iterator(it) { }
	notation_reverse_iterator& operator ++() {
		return static_cast<notation_reverse_iterator&>(base_iterator::operator ++());
	}
	notation_reverse_iterator operator ++(int)& {
		return static_cast<notation_reverse_iterator>(base_iterator::operator ++(1));
	}
	notation_reverse_iterator& operator --() {
		return static_cast<notation_reverse_iterator&>(base_iterator::operator --());
	}
	notation_reverse_iterator operator --(int)& {
		return static_cast<notation_reverse_iterator>(base_iterator::operator --(1));
	}
	notation_reverse_iterator& operator +=(difference_type i) {
		return static_cast<notation_reverse_iterator&>(base_iterator::operator +=(i));
	}
	notation_reverse_iterator operator +(difference_type i) const {
		return static_cast<notation_reverse_iterator>(base_iterator::operator +(i));
	}
	notation_reverse_iterator operator -(difference_type i) const {
		return static_cast<notation_reverse_iterator>(base_iterator::operator -(i));
	}
	difference_type operator -(const notation_reverse_iterator& other) const {
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
		return it.operator*();
	}
};

template <typename _Int=std::ptr_diff_t,typename _Float=double,typename _Boolean=bool,typename _String=std::string,
template <typename _Tp,typename... _Args>class _Array=std::vector,
template <typename _Tp,typename _Up,typename... _Args>class _Object=std::map,
template <typename _Tp>class _Allocator=std::allocator>
class notation {
public:
	template <typename>
	struct is_notation_ref : std::false_type {};
	template <typename _Tp>
	struct is_notation_ref<ref<_Tp>> : std::true_type {};

	using value_type=notation;
	using difference_type=std::ptrdiff_t;
	using size_type=std::size_t;
	using int_t=_Int;
	using float_t=_Float;
	using boolean_t=_Boolean;
	using string_t=_String;
	using array_t=_Array;
	using object_t=_Object;
	using allocator_t=_Allocator;
	using initializer_list_t=std::initializer_list;
	template <typename _Vp>
	using uncvref_t=typename std::remove_cv<typename std::remove_reference<_Vp>::type>::type;
	using ref=notation&;
	using const_ref=const notation&;
	using pointer=notation*;
	using const_pointer=const notation*;
	using iterator=notation_iterator<notation>;
	using const_iterator=const notation_iterator<notation>;
	using reverse_iterator=notation_reverse_iterator<typename notation::iterator>;
	using const_reverse_iterator=notation_reverse_iterator<typename notation::const_iterator>;

private:
	template <typename _Tp,typename=void>
	struct is_complete_type : std::false_type {};
	template <typename _Tp>
	struct is_complete_type<_Tp,decltype(void(sizeof(_Tp)))> : std::true_type {};
	template <typename _Compatible,typename=void>
	struct is_compatible_type : std::false_type {};
	template <typename _Compatible>
	struct is_compatible_type<_Compatible,std::enable_if_t<is_complete_type<_Compatible>::value>> {
		static constexpr bool value=(std::is_same<_Compatible,int_t>::value || std::is_convertible<_Compatible,int_t>::value || std::is_same<_Compatible,float_t>::value || std::is_convertible<_Compatible,float_t>::value || std::is_same<_Compatible,boolean_t>::value || std::is_same<_Compatible,string_t>::value || std::is_same<_Compatible,array_t>::value || std::is_same<_Compatible,object_t>::value);
	};

	template <typename _Tp,typename... _Args>
	_STDEX_RETURNS_NON_NULL
	static _Tp* create(_Args&&... args) {
		allocator_t<_Tp> alloc;
		auto deleter=[&](_Tp* object){
			std::allocator_traits<allocator_t<_Tp>>::deallocate(alloc,object,1);
		};
		std::unique_ptr<_Tp,decltype(deleter)> object(std::allocator_traits<allocator_t<_Tp>>::allocate(alloc,1),deleter);
		std::allocator_traits<allocator_t<_Tp>>::construct(alloc,object.get(),std::forward<_Args>(args)...);
		return object.release();
	}

public:
	struct value {
		union {
			int_t integer_;
			float_t float_;
			boolean_t boolean_;
			string_t* string_;
			array_t* array_;
			object_t* object_;
			void* other_;
		} value_;
		int_t& integer_=value_.integer_;
		float_t& float_=value_.float_;
		boolean_t& boolean_=value_.boolean_;
		string_t*& string_=value_.string_;
		array_t*& array_=value_.array_;
		object_t*& object_=value_.object_;
		void*& other_=value_.other_;
		value()=default;
		value(int_t v) noexcept : value_.integer_(v) {}
		value(float_t v) noexcept : value_.float_(v) {}
		value(boolean_t v) noexcept : value_.boolean_(v) {}
		value(const string_t& value) : value_.string_(create<string_t>(value)) {}
		value(string_t&& value) : value_.string_(create<string_t>(std::move(value))) {}
		value(const array_t& value) : value_.array_(create<array_t>(value)) {}
		value(array_t&& value) : value_.array_(create<array_t>(std::move(value))) {}
		value(const object_t& value) : value_.object_(create<object_t>(value)) {}
		value(object_t&& value) : value_.object_(create<object_t>(std::move(value))) {}
		value(notation_data_type t) {
			switch (t) {
				case NDT_OBJECT: value_.object_=create<object_t>();break;
				case NDT_ARRAY: value_.array_=create<array_t>();break;
				case NDT_STRING: value_.string_=create<string_t>(std::string(""));break;
				case NDT_BOOL: value_.boolean_=static_cast<boolean_t>(false);break;
				case NDT_INT: value_.integer_=static_cast<int_t>(0);break;
				case NDT_FLOAT: value_.float_=static_cast<float_t>(0.0);break;
				case NDT_NULL:
				default: value_.object_=nullptr;break;
			}
		}
		virtual void destroy(notation_data_type t) {
			if ((t==NDT_OBJECT && !value_.object_) || (t==NDT_ARRAY && !value_.array_) || (t==NDT_STRING && !value_.string_)) return;
			if (t==NDT_ARRAY || t==NDT_OBJECT) {
				std::vector<notation> stack;
				if (t==NDT_ARRAY) {
					stack.reserve(value_.array_->size());
					std::move(value_.array_->begin(),value_.array_->end(),std::back_inserter(stack));
				} else {
					stack.reserve(value_.object_->size());
					for (auto&& it:*value_.object_) stack.push_back(std::move(it.second));
				}
				while (!stack.empty()) {
					notation current_item(std::move(stack.back()));
					stack.pop_back();
					if (current_item.data_.type_==NDT_ARRAY) {
						std::move(current_item.data_.value_.array_->begin(),current_item.data_.value_.array_->end(),std::back_inserter(stack));
						current_item.data_.value_.array_->clear();
					} else if (current_item.data_.type_==NDT_OBJECT) {
						for (auto&& it:*current_item.data_.value_.object_) stack.push_back(std::move(it.second));
						current_item.data_.value_.object_->clear();
					}
				}
			}
			switch (t) {
				case NDT_OBJECT: allocator_t<object_t> alloc;std::allocator_traits<decltype(alloc)>::destroy(alloc,value_.object_);std::allocator_traits<decltype(alloc)>::deallocate(alloc,value_.object_,1);break;
				case NDT_ARRAY: allocator_t<array_t> alloc;std::allocator_traits<decltype(alloc)>::destroy(alloc,value_.array_);std::allocator_traits<decltype(alloc)>::deallocate(alloc,value_.array_,1);break;
				case NDT_STRING: allocator_t<string_t> alloc;std::allocator_traits<decltype(alloc)>::destroy(alloc,value_.string_);std::allocator_traits<decltype(alloc)>::deallocate(alloc,value_.string_,1);break;
				case NDT_NULL:
				case NDT_INT:
				case NDT_FLOAT:
				case NDT_BOOL: 
				default: break;
			}
		}
	};
	struct data {
		notation_data_type type_=NDT_NULL;
		value value_={};
		data(const notation_data_type t) : type_(t) , value_(t) { }
		data(size_type cnt,const notation& val) : type_(NDT_ARRAY) {
			value_.value_.array_=create<array_t>(cnt,val);
		}
		data() noexcept { };
		data(data_&&) noexcept=default;
		data(const data_&) noexcept=delete;
		data& operator =(data_&&) noexcept=delete;
		data& operator =(const data_&) noexcept=delete;
		~data() noexcept {
			value_.destroy(type_);
		}
	};
	data data_={};

public:
	notation(const notation_data_type t) : data_(t) { }
	notation(std::nullptr_t=nullptr) noexcept : notation(NDT_NULL) { }	
	template <typename _CompatibleType,typename _Tp=uncvref_t<_Compatible>,std::enable_if_t<!is_notation<_Up>::value && is_compatible_type<_Up>::value,int>=0>
	notation(_Compatible&& val) noexcept {
		if constexpr (std::is_same<_Up,int_t>::value || (std::is_convertible<_Up,int_t>::value && !std::is_same<_Up,float>::value && !std::is_same<_Up,double>::value && !std::is_same<_Up,float_t>::value)) { 
			data_.type_=NDT_INT;
			data_.value_.integer_=val;
		} else if constexpr (std::is_same<_Up,float_t>::value || std::is_same<_Up,float>::value || std::is_same<_Up,double>::value) { 
			data_.type_=NDT_FLOAT;
			data_.value_.float_=val;
		} else if constexpr (std::is_same<_Up,boolean_t>::value) { 
			data_.type_=NDT_BOOL;
			data_.valur_.boolean_=val;
		} else if constexpr (std::is_same<_Up,string_t>::value) { 
			data_.type_=NDT_STRING;
			data_.val_.string_=create<string_t>(val);
		} else if constexpr (std::is_same<_Up,array_t>::value) { 
			data_.type_=NDT_ARRAY;
			data_.valur_.array_=create<array_t>(std::move(val));
		} else if constexpr (std::is_same<_Up,object_t>::value) { 
			data_.type_=NDT_OBJECT;
			data_.value_.object_=create<object_t>(std::move(val));
		} else {
			data_.type_=NDT_NULL;
			data_.value_=value_(data_.type_);
		}
	}
	notation(initializer_list_t init_list,bool type_deduction=true,notation_data_type manual_type=NDT_ARRAY) {
		bool is_an_object=std::all_of(init_list.begin(),init_list.end(),[](const container::ref<notation>& element_ref) {
			return element_ref->data_.type_==NDT_ARRAY && element_ref->size()==2 && (*element_ref)[static_cast<size_type>(0)].data_.type_==NDT_STRING;
		});
		if (!type_deduction) {
			if (manual_type==NDT_ARRAY) is_an_object=false;
			if (manual_type==NDT_OBJECT && !is_an_object)) throw std::invalid_argument("Cannot create object from initializer_list");
		}
		if (is_an_object) {
			data_.type_=NDT_OBJECT;
			data_.value_=NDT_OBJECT;
			for (auto& element_ref:init_list) {
				auto element=element_ref.moved_or_copied();
				data_.value_.object_->emplace(std::move(*((*element.data_.value_.array_)[0].data_.value_.string_)),std::move((*element.data_.value_.array_)[1]));
			}
		} else {	  
			data_.type_=NDT_ARRAY;
			data_.value_.array_=create<array_t>(init_list.begin(),init_list.end());
		}
	}
	static notation array(initializer_list_t init_list={}) {
		return notation(init_list,false,NDT_ARRAY);
	}
	static notation object(initializer_list_t init_list={}) {
		return notation(init_list,false,NDT_OBJECT);
	}
	notation(size_t cnt,const notation& val) : data_{cnt,val} { }
	template <class _InputIT,typename std::enable_if<std::is_same<_InputIT,typename notation::iterator>::value || std::is_same<_InputIT,typename notation::const_iterator>::value,int>::type=0>
	notation(_InputIT first,_InputIT last) {
		if (first.object_!=last.object_) throw std::invalid_argument("Iterators are not compatible");
		data_.type_=first.object_->data_.type_;
		switch (data_.type_) {
			case NDT_INT:
			case NDT_FLOAT:
			case NDT_BOOL:
			case NDT_STRING: {
				if (!first.it_.primitive_iterator_.is_begin() || !last.it_.primitive_iterator_.is_end())) throw std::out_of_range("Iterators out of range");
				break;
			}
			case NDT_ARRAY:
			case NDT_OBJECT:
			case NDT_NULL:
			default: break;
		}
		switch (data_.type_) {
			case NDT_INT: data_.value_.integer_=first.object_->data_.value_.integer_;break;
			case NDT_FLOAT: data_.value_.float_=first.object_->data_.value_.float_;break;
			case NDT_BOOL: data_.value_.boolean_=first.object_->data_.value_.boolean_;break;
			case NDT_STRING: data_.value_=*first.object_->data_.value_.string_;break;
			case NDT_OBJECT: data_.value_.object_=create<object_t>(first.it_.object_iterator_,last.it_.object_iterator_);break;
			case NDT_ARRAY: data_.value_.array_=create<array_t>(first.it_.array_iterator_,last.it_.array_iterator_);break;
			case NDT_NULL:
			default: throw std::invalid_argument("Cannot construct notation with iterators");
		}

	}
	template <typename _NotationRef,std::enable_if_t<std::conjunction<is_notation_ref<_NotationRef>,std::is_same<typename _NotationRef::value_type,notation>>::value,int>=0>
	notation(const _NotationRef& ref) : notation(ref.moved_or_copied()) {}
	notation(const notation& other) {
		data_.type_=other.data_.type_;
		switch (data_.type_) {
			case NDT_OBJECT: data_.value_=*other.data_.value_.object_;break;
			case NDT_ARRAY: data_.value_=*other.data_.value_.array_;break;
			case NDT_STRING: data_.value_=*other.data_.value_.string_;break;
			case NDT_BOOL: data_.value_.boolean_=other.data_.value_.boolean_;break;
			case NDT_INT: data_.value_.integer_=other.data_.value_.integer_;break;
			case NDT_FLOAT: data_.value_.float_=other.data_.value_.float_;break;
			case NDT_NULL: 
			default: break;
		}
	}
	notation(notation&& other) noexcept : data_(std::move(other.data_)) {
		other.data_.type_=NDT_NULL;
		other.data_.value_={};
	}
	notation& operator =(notation other) noexcept (std::is_nothrow_move_constructible<notation_data_type>::value && std::is_nothrow_move_assignable<notation_data_type>::value && std::is_nothrow_move_constructible<value>::value && std::is_nothrow_move_assignable<value>::value) {
		std::swap(data_.type_,other.data_.type_);
		std::swap(data_.value_,other.data_.value_);
		return *this;
	}
	~notation() noexcept { }

	//virtual string_t dump(const int indent=-1,const char indent_char=' ',const bool ensure_ascii=false/*, const error_handler_t error_handler=error_handler_t::strict */) const=0;
	constexpr notation_data_type type() const noexcept { return data_.type_; }
	constexpr operator notation_data_type() const noexcept { return data_.type_; }

	boolean_t get_impl(boolean_t*) const {
		if (data_.type_==NDT_BOOL) return data_.value_.boolean_;
		throw std::invalid_argument("Type must be boolean");
	}
	object_t* get_impl_ptr(object_t*) noexcept {
		return data_.type_==NDT_OBJECT?data_.value_.object_:nullptr;
	}
	constexpr const object_t* get_impl_ptr(const object_t*) const noexcept {
		return data_.type_==NDT_OBJECT?data_.value_.object_:nullptr;
	}
	array_t* get_impl_ptr(array_t*) noexcept {
		return data_.type_==NDT_ARRAY?data_.value_.array_:nullptr;
	}
	constexpr const array_t* get_impl_ptr(const array_t*) const noexcept {
		return data_.type_==NDT_ARRAY?data_.value_.array_:nullptr;
	}
	string_t* get_impl_ptr(string_t*) noexcept {
		return data_.type_==NDT_STRING?data_.value_.string_:nullptr;
	}
	constexpr const string_t* get_impl_ptr(const string_t*) const noexcept {
		return data_.type_==NDT_STRING?data_.value_.string_:nullptr;
	}
	boolean_t* get_impl_ptr(boolean_t*) noexcept {
		return data_.type_==NDT_BOOL?&data_.value_.boolean_:nullptr;
	}
	constexpr const boolean_t* get_impl_ptr(const boolean_t*) const noexcept {
		return data_.type_==NDT_BOOL?&data_.value_.boolean_:nullptr;
	}
	int_t* get_impl_ptr(int_t*) noexcept {
		return data_.type_==NDT_INT?&data_.value_.integer_:nullptr;
	}
	constexpr const int_t* get_impl_ptr(const int_t*) const noexcept {
		return data_.type_==NDT_INT?&data_.value_.integer_:nullptr;
	}
	float_t* get_impl_ptr(float_t*) noexcept {
		return data_.type_==NDT_FLOAT?&data_.value_.float_:nullptr;
	}
	constexpr const float_t* get_impl_ptr(const float_t*) const noexcept {
		return data_.type_==NDT_FLOAT?&data_.value_.float_:nullptr;
	}
	template <typename _Reference,typename _This>
	static _Reference get_ref_impl(_This& obj) {
		auto* ptr=obj.template get_ptr<typename std::add_pointer<_Reference>::type>();
		if (ptr) return *ptr;
		throw std::invalid_argument("Incompatible ReferenceType for get_ref");
	}
	template <typename _Pointer,typename std::enable_if<std::is_pointer<_Pointer>::value,int>::type>
	auto get_ptr() noexcept->decltype(std::declval<notation&>().get_impl_ptr(std::declval<_Pointer>())) {
		return get_impl_ptr(static_cast<_Pointer>(nullptr));
	}
	template <typename _Pointer,typename std::enable_if<std::is_pointer<_Pointer>::value && std::is_const<typename std::remove_pointer<_Pointer>::type>::value,int>::type>
	constexpr auto get_ptr() const noexcept->decltype(std::declval<const notation&>().get_impl_ptr(std::declval<_Pointer>())) {
		return get_impl_ptr(static_cast<_Pointer>(nullptr));
	}
	template <typename _Pointer,std::enable_if_t<std::is_pointer<_Pointer>::value,int>=0>
	constexpr auto get_impl() const noexcept->decltype(std::declval<const notation&>().template get_ptr<_Pointer>()) {
		return get_ptr<_Pointer>();
	}

	virtual ref at(size_type index) {
		if (data_.type_==NDT_ARRAY) return data_.value_.array_->at(index);
		else std::invalid_argument("Cannot use at()");
	}
	virtual const_ref at(size_type index) const {
		if (data_.type_==NDT_ARRAY) return data_.value_.array_->at(index);
		else std::invalid_argument("Cannot use at()");
	}
	virtual ref at(const typename object_t::key_type& key) {
		if (data_.type_!=NDT_OBJECT) std::invalid_argument("Cannot use at()");
		auto it=data_.value_.object_->find(key);
		if (it==data_.value_.object_->end()) throw std::out_of_range("Key not found");
		return it->second;
	}
	virtual const_ref at(const typename object_t::key_type& key) const {
		if (data_.type_!=NDT_OBJECT) std::invalid_argument("Cannot use at()");
		auto it=data_.value_.object_->find(key);
		if (it==data_.value_.object_->end()) throw std::out_of_range("Key not found");
		return it->second;
	}
	virtual ref operator [](size_type index) {
		if (data_.type_==NDT_NULL) {
			data_.type_=NDT_ARRAY;
			data_.value_.array_=create<array_t>();
		}
		if (data_.type_==NDT_ARRAY) {
			if (index>=data_.value_.array_->size()) data_.value_.array_->resize(index+1);
			return data_.value_.array_->operator [](index);
		}
		throw std::invalid_argument("Cannot use operator[]");
	}
	virtual const_ref operator [](size_type index) const {
		if (data_.type_==NDT_ARRAY) return data_.value_.array_->operator[](index);
		throw std::invalid_argument("Cannot use operator[]");
	}
	virtual ref operator [](typename object_t::key_type key) {
		if (data_.type_==NDT_NULL) {
			data_.type_=NDT_NULL;
			data_.value_.object_=create<object_t>();
		}
		if (data_.type_==NDT_OBJECT) {
			auto result=data_.value_.object_->emplace(std::move(key),nullptr);
			return result.first->second;
		}
		throw std::invalid_argument("Cannot use operator[]");
	}
	virtual const_ref operator [](const typename object_t::key_type& key) const {
		if (data_.type_==NDT_OBJECT) {
			auto it=data_.value_.object_->find(key);
			return it->second;
		}
		throw std::invalid_argument("Cannot use operator[]");
	}
	template <typename _Tp>
	ref operator [](_Tp* key) {
		return operator [](typename object_t::key_type(key));
	}
	template <typename _Tp>
	const_ref operator [](_Tp* key) const {
		return operator [](typename object_t::key_type(key));
	}
	template<class _KeyType,std::enable_if_t<std::is_constructible_v<typename basic_json::string_t,_KeyType> || std::is_same_v<typename basic_json::string_t,_KeyType>,int> = 0>
	virtual reference operator [](_KeyType && key) {
		if (data_.type_==NDT_NULL) {
			data_.type_=NDT_OBJECT;
			data_.value_.object_=create<object_t>();
		}
		if (data_.type_==NDT_OBJECT) {
			auto result=data_.value_.object_->emplace(std::forward<_KeyType>(key),nullptr);
			return result.first->second;
		}
		throw std::invalid_argument("Cannot use operator[]");
	}
	template<class _KeyType,std::enable_if_t<std::is_constructible_v<typename basic_json::string_t,_KeyType> || std::is_same_v<typename basic_json::string_t,_KeyType>,int> = 0>
	virtual const_reference operator [](_KeyType && key) const {
		if (data_.type_==NDT_OBJECT) {
			auto it=data_.value_.object_->find(std::forward<_KeyType>(key));
			return it->second;
		}
		throw std::invalid_argument("Cannot use operator[]");
	}

template<typename KeyType>
    using is_comparable_with_object_key = detail::is_comparable <
        object_comparator_t, const typename object_t::key_type&, KeyType >;

    template<typename ValueType>
    using value_return_type = std::conditional <
        detail::is_c_string_uncvref<ValueType>::value,
        string_t, typename std::decay<ValueType>::type >;

  public:
   
    template < class ValueType, detail::enable_if_t <
                   !detail::is_transparent<object_comparator_t>::value
                   && detail::is_getable<basic_json_t, ValueType>::value
                   && !std::is_same<value_t, detail::uncvref_t<ValueType>>::value, int > = 0 >
    ValueType value(const typename object_t::key_type& key, const ValueType& default_value) const
    {
        // value only works for objects
        if (JSON_HEDLEY_LIKELY(data_.type_==NDT_OBJECT))
        {
            // if key is found, return value and given default value otherwise
            const auto it = find(key);
            if (it != end())
            {
                return it->template get<ValueType>();
            }

            return default_value;
        }

        JSON_THROW(type_error::create(306, detail::concat("cannot use value() with ", type_name()), this));
    }

    template < class ValueType, class ReturnType = typename value_return_type<ValueType>::type,
               detail::enable_if_t <
                   !detail::is_transparent<object_comparator_t>::value
                   && detail::is_getable<basic_json_t, ReturnType>::value
                   && !std::is_same<value_t, detail::uncvref_t<ValueType>>::value, int > = 0 >
    ReturnType value(const typename object_t::key_type& key, ValueType && default_value) const
    {
        // value only works for objects
        if (JSON_HEDLEY_LIKELY(data_.type_==NDT_OBJECT))
        {
            // if key is found, return value and given default value otherwise
            const auto it = find(key);
            if (it != end())
            {
                return it->template get<ReturnType>();
            }

            return std::forward<ValueType>(default_value);
        }

        JSON_THROW(type_error::create(306, detail::concat("cannot use value() with ", type_name()), this));
    }

    /// @brief access specified object element with default value
    /// @sa https://json.nlohmann.me/api/basic_json/value/
    template < class ValueType, class KeyType, detail::enable_if_t <
                   detail::is_transparent<object_comparator_t>::value
                   && !detail::is_json_pointer<KeyType>::value
                   && is_comparable_with_object_key<KeyType>::value
                   && detail::is_getable<basic_json_t, ValueType>::value
                   && !std::is_same<value_t, detail::uncvref_t<ValueType>>::value, int > = 0 >
    ValueType value(KeyType && key, const ValueType& default_value) const
    {
        // value only works for objects
        if (JSON_HEDLEY_LIKELY(data_.type_==NDT_OBJECT))
        {
            // if key is found, return value and given default value otherwise
            const auto it = find(std::forward<KeyType>(key));
            if (it != end())
            {
                return it->template get<ValueType>();
            }

            return default_value;
        }

        JSON_THROW(type_error::create(306, detail::concat("cannot use value() with ", type_name()), this));
    }

    /// @brief access specified object element via JSON Pointer with default value
    /// @sa https://json.nlohmann.me/api/basic_json/value/
    template < class ValueType, class KeyType, class ReturnType = typename value_return_type<ValueType>::type,
               detail::enable_if_t <
                   detail::is_transparent<object_comparator_t>::value
                   && !detail::is_json_pointer<KeyType>::value
                   && is_comparable_with_object_key<KeyType>::value
                   && detail::is_getable<basic_json_t, ReturnType>::value
                   && !std::is_same<value_t, detail::uncvref_t<ValueType>>::value, int > = 0 >
    ReturnType value(KeyType && key, ValueType && default_value) const
    {
        // value only works for objects
        if (JSON_HEDLEY_LIKELY(data_.type_==NDT_OBJECT))
        {
            // if key is found, return value and given default value otherwise
            const auto it = find(std::forward<KeyType>(key));
            if (it != end())
            {
                return it->template get<ReturnType>();
            }

            return std::forward<ValueType>(default_value);
        }

        JSON_THROW(type_error::create(306, detail::concat("cannot use value() with ", type_name()), this));
    }

    /// @brief access specified object element via JSON Pointer with default value
    /// @sa https://json.nlohmann.me/api/basic_json/value/
    template < class ValueType, detail::enable_if_t <
                   detail::is_getable<basic_json_t, ValueType>::value
                   && !std::is_same<value_t, detail::uncvref_t<ValueType>>::value, int > = 0 >
    ValueType value(const json_pointer& ptr, const ValueType& default_value) const
    {
        // value only works for objects
        if (JSON_HEDLEY_LIKELY(data_.type_==NDT_OBJECT))
        {
            // if pointer resolves a value, return it or use default value
            JSON_TRY
            {
                return ptr.get_checked(this).template get<ValueType>();
            }
            JSON_INTERNAL_CATCH (out_of_range&)
            {
                return default_value;
            }
        }

        JSON_THROW(type_error::create(306, detail::concat("cannot use value() with ", type_name()), this));
    }

    /// @brief access specified object element via JSON Pointer with default value
    /// @sa https://json.nlohmann.me/api/basic_json/value/
    template < class ValueType, class ReturnType = typename value_return_type<ValueType>::type,
               detail::enable_if_t <
                   detail::is_getable<basic_json_t, ReturnType>::value
                   && !std::is_same<value_t, detail::uncvref_t<ValueType>>::value, int > = 0 >
    ReturnType value(const json_pointer& ptr, ValueType && default_value) const
    {
        // value only works for objects
        if (JSON_HEDLEY_LIKELY(data_.type_==NDT_OBJECT))
        {
            // if pointer resolves a value, return it or use default value
            JSON_TRY
            {
                return ptr.get_checked(this).template get<ReturnType>();
            }
            JSON_INTERNAL_CATCH (out_of_range&)
            {
                return std::forward<ValueType>(default_value);
            }
        }

        JSON_THROW(type_error::create(306, detail::concat("cannot use value() with ", type_name()), this));
    }

    template < class ValueType, class BasicJsonType, detail::enable_if_t <
                   detail::is_basic_json<BasicJsonType>::value
                   && detail::is_getable<basic_json_t, ValueType>::value
                   && !std::is_same<value_t, detail::uncvref_t<ValueType>>::value, int > = 0 >
    JSON_HEDLEY_DEPRECATED_FOR(3.11.0, basic_json::json_pointer or nlohmann::json_pointer<basic_json::string_t>) // NOLINT(readability/alt_tokens)
    ValueType value(const ::nlohmann::json_pointer<BasicJsonType>& ptr, const ValueType& default_value) const
    {
        return value(ptr.convert(), default_value);
    }

    template < class ValueType, class BasicJsonType, class ReturnType = typename value_return_type<ValueType>::type,
               detail::enable_if_t <
                   detail::is_basic_json<BasicJsonType>::value
                   && detail::is_getable<basic_json_t, ReturnType>::value
                   && !std::is_same<value_t, detail::uncvref_t<ValueType>>::value, int > = 0 >
    JSON_HEDLEY_DEPRECATED_FOR(3.11.0, basic_json::json_pointer or nlohmann::json_pointer<basic_json::string_t>) // NOLINT(readability/alt_tokens)
    ReturnType value(const ::nlohmann::json_pointer<BasicJsonType>& ptr, ValueType && default_value) const
    {
        return value(ptr.convert(), std::forward<ValueType>(default_value));
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

    template < class IteratorType, detail::enable_if_t <
                   std::is_same<IteratorType, typename basic_json_t::iterator>::value ||
                   std::is_same<IteratorType, typename basic_json_t::const_iterator>::value, int > = 0 >
    IteratorType erase(IteratorType pos)
    {
        // make sure iterator fits the current value
        if (JSON_HEDLEY_UNLIKELY(this != pos.m_object))
        {
            JSON_THROW(invalid_iterator::create(202, "iterator does not fit current value", this));
        }

        IteratorType result = end();

        switch (data_.type_)
        {
            case value_t::boolean:
            case value_t::number_float:
            case value_t::number_integer:
            case value_t::number_unsigned:
            case value_t::string:
            case value_t::binary:
            {
                if (JSON_HEDLEY_UNLIKELY(!pos.it_.primitive_iterator.is_begin()))
                {
                    JSON_THROW(invalid_iterator::create(205, "iterator out of range", this));
                }

                if (is_string())
                {
                    AllocatorType<string_t> alloc;
                    std::allocator_traits<decltype(alloc)>::destroy(alloc, data_.value_.string);
                    std::allocator_traits<decltype(alloc)>::deallocate(alloc, data_.value_.string, 1);
                    data_.value_.string = nullptr;
                }
                else if (is_binary())
                {
                    AllocatorType<binary_t> alloc;
                    std::allocator_traits<decltype(alloc)>::destroy(alloc, data_.value_.binary);
                    std::allocator_traits<decltype(alloc)>::deallocate(alloc, data_.value_.binary, 1);
                    data_.value_.binary = nullptr;
                }

                data_.type_ = value_t::null;
                assert_invariant();
                break;
            }

            case value_t::object:
            {
                result.it_.object_iterator = data_.value_.object->erase(pos.it_.object_iterator);
                break;
            }

            case value_t::array:
            {
                result.it_.array_iterator = data_.value_.array->erase(pos.it_.array_iterator);
                break;
            }

            case value_t::null:
            case value_t::discarded:
            default:
                JSON_THROW(type_error::create(307, detail::concat("cannot use erase() with ", type_name()), this));
        }

        return result;
    }

    template < class IteratorType, detail::enable_if_t <
                   std::is_same<IteratorType, typename basic_json_t::iterator>::value ||
                   std::is_same<IteratorType, typename basic_json_t::const_iterator>::value, int > = 0 >
    IteratorType erase(IteratorType first, IteratorType last)
    {
        // make sure iterator fits the current value
        if (JSON_HEDLEY_UNLIKELY(this != first.m_object || this != last.m_object))
        {
            JSON_THROW(invalid_iterator::create(203, "iterators do not fit current value", this));
        }

        IteratorType result = end();

        switch (data_.type_)
        {
            case value_t::boolean:
            case value_t::number_float:
            case value_t::number_integer:
            case value_t::number_unsigned:
            case value_t::string:
            case value_t::binary:
            {
                if (JSON_HEDLEY_LIKELY(!first.it_.primitive_iterator.is_begin()
                                       || !last.it_.primitive_iterator.is_end()))
                {
                    JSON_THROW(invalid_iterator::create(204, "iterators out of range", this));
                }

                if (is_string())
                {
                    AllocatorType<string_t> alloc;
                    std::allocator_traits<decltype(alloc)>::destroy(alloc, data_.value_.string);
                    std::allocator_traits<decltype(alloc)>::deallocate(alloc, data_.value_.string, 1);
                    data_.value_.string = nullptr;
                }
                else if (is_binary())
                {
                    AllocatorType<binary_t> alloc;
                    std::allocator_traits<decltype(alloc)>::destroy(alloc, data_.value_.binary);
                    std::allocator_traits<decltype(alloc)>::deallocate(alloc, data_.value_.binary, 1);
                    data_.value_.binary = nullptr;
                }

                data_.type_ = value_t::null;
                assert_invariant();
                break;
            }

            case value_t::object:
            {
                result.it_.object_iterator = data_.value_.object->erase(first.it_.object_iterator,
                                              last.it_.object_iterator);
                break;
            }

            case value_t::array:
            {
                result.it_.array_iterator = data_.value_.array->erase(first.it_.array_iterator,
                                             last.it_.array_iterator);
                break;
            }

            case value_t::null:
            case value_t::discarded:
            default:
                JSON_THROW(type_error::create(307, detail::concat("cannot use erase() with ", type_name()), this));
        }

        return result;
    }

  private:
    template < typename _KeyType, detail::enable_if_t <
                   detail::has_erase_with_key_type<basic_json_t, KeyType>::value, int > = 0 >
	size_type erase_internal(_KeyType && key) {
		if (data_.type_!=NDT_OBJECT)) throw std::invalid_argument("Cannot use erase()");
		return data_.value_.object->erase(std::forward<KeyType>(key));
	}

    template < typename KeyType, detail::enable_if_t <
                   !detail::has_erase_with_key_type<basic_json_t, KeyType>::value, int > = 0 >
    size_type erase_internal(KeyType && key)
    {
        // this erase only works for objects
        if (JSON_HEDLEY_UNLIKELY(!data_.type_==NDT_OBJECT))
        {
            JSON_THROW(type_error::create(307, detail::concat("cannot use erase() with ", type_name()), this));
        }

        const auto it = data_.value_.object->find(std::forward<KeyType>(key));
        if (it != data_.value_.object->end())
        {
            data_.value_.object->erase(it);
            return 1;
        }
        return 0;
    }

  public:

	size_type erase(const typename object_t::key_type& key) {
		return erase_internal(key);
	}
	template <class _KeyType, detail::enable_if_t<
                 detail::is_usable_as_basic_json_key_type<basic_json_t, KeyType>::value, int> = 0>
	size_type erase(_KeyType && key)  {
		return erase_internal(std::forward<_KeyType>(key));
	}
	void erase(const size_type index) {
		if (data_.type_==NDT_ARRAY) {
			if (index>=size())) throw std::out_of_range("Array index is out of range");
			data_.value_.array_->erase(data_.value_.array_->begin()+static_cast<difference_type>(index));
		} else throw std::invalid_argument("Cannot use erase()");
	}

	iterator find(const typename object_t::key_type& key) {
		auto result=end();
		if (data_.type_==NDT_OBJECT) result.it_.object_iterator_=data_.value_.object_->find(key);
		return result;
	}
	const_iterator find(const typename object_t::key_type& key) const {
		auto result=cend();
		if (data_.type_==NDT_OBJECT) result.it_.object_iterator_=data_.value_.object_->find(key);
		return result;
	}
    template<class KeyType, detail::enable_if_t<
                 detail::is_usable_as_basic_json_key_type<basic_json_t, KeyType>::value, int> = 0>
    iterator find(KeyType && key)
    {
        auto result = end();

        if (data_.type_==NDT_OBJECT)
        {
            result.it_.object_iterator = data_.value_.object->find(std::forward<KeyType>(key));
        }

        return result;
    }

    template<class KeyType, detail::enable_if_t<
                 detail::is_usable_as_basic_json_key_type<basic_json_t, KeyType>::value, int> = 0>
    const_iterator find(KeyType && key) const
    {
        auto result = cend();

        if (data_.type_==NDT_OBJECT)
        {
            result.it_.object_iterator = data_.value_.object->find(std::forward<KeyType>(key));
        }

        return result;
    }

	size_type count(const typename object_t::key_type& key) const {
		return data_.type_==NDT_OBJECT?data_.value_.object_->count(key):0;
	}

	 template<class _KeyType, detail::enable_if_t<
                 detail::is_usable_as_basic_json_key_type<basic_json_t, KeyType>::value, int> = 0>
    size_type count(_KeyType && key) const
    {
        // return 0 for all nonobject types
        return data_.type_==NDT_OBJECT ? data_.value_.object->count(std::forward<KeyType>(key)) : 0;
    }

	bool contains(const typename object_t::key_type& key) const {
		return data_.type_==NDT_OBJECT && data_.value_.object_->find(key)!=data_.value_.object_->end();
	}

    template<class KeyType, detail::enable_if_t<
                 detail::is_usable_as_basic_json_key_type<basic_json_t, KeyType>::value, int> = 0>
    bool contains(KeyType && key) const {
        return data_.type_==NDT_OBJECT && data_.value_.object->find(std::forward<KeyType>(key)) != data_.value_.object->end();
    }

	bool contains(const json_pointer& ptr) const {
		return ptr.contains(this);
	}

    template<typename BasicJsonType, detail::enable_if_t<detail::is_basic_json<BasicJsonType>::value, int> = 0>
    JSON_HEDLEY_DEPRECATED_FOR(3.11.0, basic_json::json_pointer or nlohmann::json_pointer<basic_json::string_t>) // NOLINT(readability/alt_tokens)
    bool contains(const typename ::nlohmann::json_pointer<BasicJsonType>& ptr) const
    {
        return ptr.contains(this);
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

  public:

    JSON_HEDLEY_DEPRECATED_FOR(3.1.0, items())
    static iteration_proxy<iterator> iterator_wrapper(reference ref) noexcept
    {
        return ref.items();
    }

    /// @brief wrapper to access iterator member functions in range-based for
    /// @sa https://json.nlohmann.me/api/basic_json/items/
    /// @deprecated This function is deprecated since 3.1.0 and will be removed in
    ///         version 4.0.0 of the library. Please use @ref items() instead;
    ///         that is, replace `json::iterator_wrapper(j)` with `j.items()`.
    JSON_HEDLEY_DEPRECATED_FOR(3.1.0, items())
    static iteration_proxy<const_iterator> iterator_wrapper(const_reference ref) noexcept
    {
        return ref.items();
    }

    /// @brief helper to access iterator member functions in range-based for
    /// @sa https://json.nlohmann.me/api/basic_json/items/
    iteration_proxy<iterator> items() noexcept
    {
        return iteration_proxy<iterator>(*this);
    }

    /// @brief helper to access iterator member functions in range-based for
    /// @sa https://json.nlohmann.me/api/basic_json/items/
    iteration_proxy<const_iterator> items() const noexcept
    {
        return iteration_proxy<const_iterator>(*this);
    }


	bool empty() const noexcept {
		switch (data_.type_) {
			case NDT_NULL: return true;
			case NDT_ARRAY: return data_.value_.array_->empty();
			case NDT_OBJECT: return data_.value_.object_->empty();
			case NDT_STRING:
			case NDT_BOOL:
			case NDT_INT:
			case NDT_FLOAT:
			default: return false;
		}
	}
	size_type size() const noexcept {
		switch (data_.type_) {
			case NDT_NULL: return 0;
			case NDT_ARRAY: return data_.value_.array_->size();
			case NDT_OBJECT: return data_.value_.object_->size();
			case NDT_STRING:
			case NDT_BOOL:
			case NDT_INT:
			case NDT_FLOAT:
			default: return 1;
		}
	}
	size_type max_size() const noexcept {
		switch (data_.type_) {
			case NDT_ARRAY: return data_.value_.array_->max_size();
			case NDT_OBJECT: return data_.value_.object_->max_size();
			case NDT_NULL:
			case NDT_STRING:
			case NDT_BOOL:
			case NDT_INT:
			case NDT_FLOAT:
			default: return size();
		}
	}
	void clear() noexcept {
		switch (data_.type_) {
			case NDT_INT: data_.value_.number_integer_=0;break;
			case NDT_FLOAT: data_.value_.number_float_=0.0;break;
			case NDT_BOOL: data_.value_.boolean_=false;break;
			case NDT_STRING: data_.value_.string_->clear();break;
			case NDT_ARRAY: data_.value_.array_->clear();break;
			case NDT_OBJECT: data_.value_.object_->clear();break;
			case NDT_NULL:
			default: break;
		}
	}
	void push_back(notation&& val) {
		if (!(data_.type_==NDT_NULL || data_.type_==NDT_ARRAY)) throw std::invalid_argument("Cannot use push_back()");
		if (data_.type_==NDT_NULL) {
			data_.type_=NDT_ARRAY;
			data_.value_=NDT_ARRAY;
		}
		const auto old_capacity=data_.value_.array_->capacity();
		data_.value_.array_->push_back(std::move(val));
	}
	ref operator +=(notation&& val) {
		push_back(std::move(val));
		return *this;
	}
	void push_back(const notation& val)
    {
        // push_back only works for null objects or arrays
        if (JSON_HEDLEY_UNLIKELY(!(is_null() || data_.type_==NDT_ARRAY)))
        {
            JSON_THROW(type_error::create(308, detail::concat("cannot use push_back() with ", type_name()), this));
        }

        // transform null object into an array
        if (is_null())
        {
            data_.type_ = value_t::array;
            data_.value_ = value_t::array;
            assert_invariant();
        }

        // add element to array
        const auto old_capacity = data_.value_.array->capacity();
        data_.value_.array->push_back(val);
        set_parent(data_.value_.array->back(), old_capacity);
    }

    /// @brief add an object to an array
    /// @sa https://json.nlohmann.me/api/basic_json/operator+=/
    reference operator+=(const basic_json& val)
    {
        push_back(val);
        return *this;
    }

    /// @brief add an object to an object
    /// @sa https://json.nlohmann.me/api/basic_json/push_back/
    void push_back(const typename object_t::value_type& val)
    {
        // push_back only works for null objects or objects
        if (JSON_HEDLEY_UNLIKELY(!(is_null() || data_.type_==NDT_OBJECT)))
        {
            JSON_THROW(type_error::create(308, detail::concat("cannot use push_back() with ", type_name()), this));
        }

        // transform null object into an object
        if (is_null())
        {
            data_.type_ = value_t::object;
            data_.value_ = value_t::object;
            assert_invariant();
        }

        // add element to object
        auto res = data_.value_.object->insert(val);
        set_parent(res.first->second);
    }

    /// @brief add an object to an object
    /// @sa https://json.nlohmann.me/api/basic_json/operator+=/
    reference operator+=(const typename object_t::value_type& val)
    {
        push_back(val);
        return *this;
    }

    /// @brief add an object to an object
    /// @sa https://json.nlohmann.me/api/basic_json/push_back/
    void push_back(initializer_list_t init)
    {
        if (data_.type_==NDT_OBJECT && init.size() == 2 && (*init.begin())->is_string())
        {
            basic_json&& key = init.begin()->moved_or_copied();
            push_back(typename object_t::value_type(
                          std::move(key.get_ref<string_t&>()), (init.begin() + 1)->moved_or_copied()));
        }
        else
        {
            push_back(basic_json(init));
        }
    }

    /// @brief add an object to an object
    /// @sa https://json.nlohmann.me/api/basic_json/operator+=/
    reference operator+=(initializer_list_t init)
    {
        push_back(init);
        return *this;
    }

    /// @brief add an object to an array
    /// @sa https://json.nlohmann.me/api/basic_json/emplace_back/
    template<class... Args>
    reference emplace_back(Args&& ... args)
    {
        // emplace_back only works for null objects or arrays
        if (JSON_HEDLEY_UNLIKELY(!(is_null() || data_.type_==NDT_ARRAY)))
        {
            JSON_THROW(type_error::create(311, detail::concat("cannot use emplace_back() with ", type_name()), this));
        }

        // transform null object into an array
        if (is_null())
        {
            data_.type_ = value_t::array;
            data_.value_ = value_t::array;
            assert_invariant();
        }

        // add element to array (perfect forwarding)
        const auto old_capacity = data_.value_.array->capacity();
        data_.value_.array->emplace_back(std::forward<Args>(args)...);
        return set_parent(data_.value_.array->back(), old_capacity);
    }

    /// @brief add an object to an object if key does not exist
    /// @sa https://json.nlohmann.me/api/basic_json/emplace/
    template<class... Args>
    std::pair<iterator, bool> emplace(Args&& ... args)
    {
        // emplace only works for null objects or arrays
        if (JSON_HEDLEY_UNLIKELY(!(is_null() || data_.type_==NDT_OBJECT)))
        {
            JSON_THROW(type_error::create(311, detail::concat("cannot use emplace() with ", type_name()), this));
        }

        // transform null object into an object
        if (is_null())
        {
            data_.type_ = value_t::object;
            data_.value_ = value_t::object;
            assert_invariant();
        }

        // add element to array (perfect forwarding)
        auto res = data_.value_.object->emplace(std::forward<Args>(args)...);
        set_parent(res.first->second);

        // create result iterator and set iterator to the result of emplace
        auto it = begin();
        it.it_.object_iterator = res.first;

        // return pair of iterator and boolean
        return {it, res.second};
    }

    /// Helper for insertion of an iterator
    /// @note: This uses std::distance to support GCC 4.8,
    ///        see https://github.com/nlohmann/json/pull/1257
    template<typename... Args>
    iterator insert_iterator(const_iterator pos, Args&& ... args)
    {
        iterator result(this);
        JSON_ASSERT(data_.value_.array != nullptr);

        auto insert_pos = std::distance(data_.value_.array->begin(), pos.it_.array_iterator);
        data_.value_.array->insert(pos.it_.array_iterator, std::forward<Args>(args)...);
        result.it_.array_iterator = data_.value_.array->begin() + insert_pos;

        // This could have been written as:
        // result.it_.array_iterator = data_.value_.array->insert(pos.it_.array_iterator, cnt, val);
        // but the return value of insert is missing in GCC 4.8, so it is written this way instead.

        set_parents();
        return result;
    }

    /// @brief inserts element into array
    /// @sa https://json.nlohmann.me/api/basic_json/insert/
    iterator insert(const_iterator pos, const basic_json& val)
    {
        // insert only works for arrays
        if (JSON_HEDLEY_LIKELY(data_.type_==NDT_ARRAY))
        {
            // check if iterator pos fits to this JSON value
            if (JSON_HEDLEY_UNLIKELY(pos.m_object != this))
            {
                JSON_THROW(invalid_iterator::create(202, "iterator does not fit current value", this));
            }

            // insert to array and return iterator
            return insert_iterator(pos, val);
        }

        JSON_THROW(type_error::create(309, detail::concat("cannot use insert() with ", type_name()), this));
    }

    /// @brief inserts element into array
    /// @sa https://json.nlohmann.me/api/basic_json/insert/
    iterator insert(const_iterator pos, basic_json&& val)
    {
        return insert(pos, val);
    }

    /// @brief inserts copies of element into array
    /// @sa https://json.nlohmann.me/api/basic_json/insert/
    iterator insert(const_iterator pos, size_type cnt, const basic_json& val)
    {
        // insert only works for arrays
        if (JSON_HEDLEY_LIKELY(data_.type_==NDT_ARRAY))
        {
            // check if iterator pos fits to this JSON value
            if (JSON_HEDLEY_UNLIKELY(pos.m_object != this))
            {
                JSON_THROW(invalid_iterator::create(202, "iterator does not fit current value", this));
            }

            // insert to array and return iterator
            return insert_iterator(pos, cnt, val);
        }

        JSON_THROW(type_error::create(309, detail::concat("cannot use insert() with ", type_name()), this));
    }

    /// @brief inserts range of elements into array
    /// @sa https://json.nlohmann.me/api/basic_json/insert/
    iterator insert(const_iterator pos, const_iterator first, const_iterator last)
    {
        // insert only works for arrays
        if (JSON_HEDLEY_UNLIKELY(!data_.type_==NDT_ARRAY))
        {
            JSON_THROW(type_error::create(309, detail::concat("cannot use insert() with ", type_name()), this));
        }

        // check if iterator pos fits to this JSON value
        if (JSON_HEDLEY_UNLIKELY(pos.m_object != this))
        {
            JSON_THROW(invalid_iterator::create(202, "iterator does not fit current value", this));
        }

        // check if range iterators belong to the same JSON object
        if (JSON_HEDLEY_UNLIKELY(first.m_object != last.m_object))
        {
            JSON_THROW(invalid_iterator::create(210, "iterators do not fit", this));
        }

        if (JSON_HEDLEY_UNLIKELY(first.m_object == this))
        {
            JSON_THROW(invalid_iterator::create(211, "passed iterators may not belong to container", this));
        }

        // insert to array and return iterator
        return insert_iterator(pos, first.it_.array_iterator, last.it_.array_iterator);
    }

    /// @brief inserts elements from initializer list into array
    /// @sa https://json.nlohmann.me/api/basic_json/insert/
    iterator insert(const_iterator pos, initializer_list_t ilist)
    {
        // insert only works for arrays
        if (JSON_HEDLEY_UNLIKELY(!data_.type_==NDT_ARRAY))
        {
            JSON_THROW(type_error::create(309, detail::concat("cannot use insert() with ", type_name()), this));
        }

        // check if iterator pos fits to this JSON value
        if (JSON_HEDLEY_UNLIKELY(pos.m_object != this))
        {
            JSON_THROW(invalid_iterator::create(202, "iterator does not fit current value", this));
        }

        // insert to array and return iterator
        return insert_iterator(pos, ilist.begin(), ilist.end());
    }

    /// @brief inserts range of elements into object
    /// @sa https://json.nlohmann.me/api/basic_json/insert/
    void insert(const_iterator first, const_iterator last)
    {
        // insert only works for objects
        if (JSON_HEDLEY_UNLIKELY(!data_.type_==NDT_OBJECT))
        {
            JSON_THROW(type_error::create(309, detail::concat("cannot use insert() with ", type_name()), this));
        }

        // check if range iterators belong to the same JSON object
        if (JSON_HEDLEY_UNLIKELY(first.m_object != last.m_object))
        {
            JSON_THROW(invalid_iterator::create(210, "iterators do not fit", this));
        }

        // passed iterators must belong to objects
        if (JSON_HEDLEY_UNLIKELY(!first.m_object->data_.type_==NDT_OBJECT))
        {
            JSON_THROW(invalid_iterator::create(202, "iterators first and last must point to objects", this));
        }

        data_.value_.object->insert(first.it_.object_iterator, last.it_.object_iterator);
    }

    /// @brief updates a JSON object from another object, overwriting existing keys
    /// @sa https://json.nlohmann.me/api/basic_json/update/
    void update(const_reference j, bool merge_objects = false)
    {
        update(j.begin(), j.end(), merge_objects);
    }

    /// @brief updates a JSON object from another object, overwriting existing keys
    /// @sa https://json.nlohmann.me/api/basic_json/update/
    void update(const_iterator first, const_iterator last, bool merge_objects = false)
    {
        // implicitly convert null value to an empty object
        if (is_null())
        {
            data_.type_ = value_t::object;
            data_.value_.object = create<object_t>();
            assert_invariant();
        }

        if (JSON_HEDLEY_UNLIKELY(!data_.type_==NDT_OBJECT))
        {
            JSON_THROW(type_error::create(312, detail::concat("cannot use update() with ", type_name()), this));
        }

        // check if range iterators belong to the same JSON object
        if (JSON_HEDLEY_UNLIKELY(first.m_object != last.m_object))
        {
            JSON_THROW(invalid_iterator::create(210, "iterators do not fit", this));
        }

        // passed iterators must belong to objects
        if (JSON_HEDLEY_UNLIKELY(!first.m_object->data_.type_==NDT_OBJECT))
        {
            JSON_THROW(type_error::create(312, detail::concat("cannot use update() with ", first.m_object->type_name()), first.m_object));
        }

        for (auto it = first; it != last; ++it)
        {
            if (merge_objects && it.value().data_.type_==NDT_OBJECT)
            {
                auto it2 = data_.value_.object->find(it.key());
                if (it2 != data_.value_.object->end())
                {
                    it2->second.update(it.value(), true);
                    continue;
                }
            }
            data_.value_.object->operator[](it.key()) = it.value();
#if JSON_DIAGNOSTICS
            data_.value_.object->operator[](it.key()).m_parent = this;
#endif
        }
    }

    /// @brief exchanges the values
    /// @sa https://json.nlohmann.me/api/basic_json/swap/
    void swap(reference other) noexcept (
        std::is_nothrow_move_constructible<value_t>::value&&
        std::is_nothrow_move_assignable<value_t>::value&&
        std::is_nothrow_move_constructible<json_value>::value&& // NOLINT(cppcoreguidelines-noexcept-swap,performance-noexcept-swap)
        std::is_nothrow_move_assignable<json_value>::value
    )
    {
        std::swap(data_.type_, other.data_.type_);
        std::swap(data_.value_, other.data_.value_);

        set_parents();
        other.set_parents();
        assert_invariant();
    }

    /// @brief exchanges the values
    /// @sa https://json.nlohmann.me/api/basic_json/swap/
    friend void swap(reference left, reference right) noexcept (
        std::is_nothrow_move_constructible<value_t>::value&&
        std::is_nothrow_move_assignable<value_t>::value&&
        std::is_nothrow_move_constructible<json_value>::value&& // NOLINT(cppcoreguidelines-noexcept-swap,performance-noexcept-swap)
        std::is_nothrow_move_assignable<json_value>::value
    )
    {
        left.swap(right);
    }

    /// @brief exchanges the values
    /// @sa https://json.nlohmann.me/api/basic_json/swap/
    void swap(array_t& other) // NOLINT(bugprone-exception-escape,cppcoreguidelines-noexcept-swap,performance-noexcept-swap)
    {
        // swap only works for arrays
        if (JSON_HEDLEY_LIKELY(data_.type_==NDT_ARRAY))
        {
            using std::swap;
            swap(*(data_.value_.array), other);
        }
        else
        {
            JSON_THROW(type_error::create(310, detail::concat("cannot use swap(array_t&) with ", type_name()), this));
        }
    }

    /// @brief exchanges the values
    /// @sa https://json.nlohmann.me/api/basic_json/swap/
    void swap(object_t& other) // NOLINT(bugprone-exception-escape,cppcoreguidelines-noexcept-swap,performance-noexcept-swap)
    {
        // swap only works for objects
        if (JSON_HEDLEY_LIKELY(data_.type_==NDT_OBJECT))
        {
            using std::swap;
            swap(*(data_.value_.object), other);
        }
        else
        {
            JSON_THROW(type_error::create(310, detail::concat("cannot use swap(object_t&) with ", type_name()), this));
        }
    }

    /// @brief exchanges the values
    /// @sa https://json.nlohmann.me/api/basic_json/swap/
    void swap(string_t& other) // NOLINT(bugprone-exception-escape,cppcoreguidelines-noexcept-swap,performance-noexcept-swap)
    {
        // swap only works for strings
        if (JSON_HEDLEY_LIKELY(is_string()))
        {
            using std::swap;
            swap(*(data_.value_.string), other);
        }
        else
        {
            JSON_THROW(type_error::create(310, detail::concat("cannot use swap(string_t&) with ", type_name()), this));
        }
    }

    /// @brief exchanges the values
    /// @sa https://json.nlohmann.me/api/basic_json/swap/
    void swap(binary_t& other) // NOLINT(bugprone-exception-escape,cppcoreguidelines-noexcept-swap,performance-noexcept-swap)
    {
        // swap only works for strings
        if (JSON_HEDLEY_LIKELY(is_binary()))
        {
            using std::swap;
            swap(*(data_.value_.binary), other);
        }
        else
        {
            JSON_THROW(type_error::create(310, detail::concat("cannot use swap(binary_t&) with ", type_name()), this));
        }
    }

    /// @brief exchanges the values
    /// @sa https://json.nlohmann.me/api/basic_json/swap/
    void swap(typename binary_t::container_type& other) // NOLINT(bugprone-exception-escape)
    {
        // swap only works for strings
        if (JSON_HEDLEY_LIKELY(is_binary()))
        {
            using std::swap;
            swap(*(data_.value_.binary), other);
        }
        else
        {
            JSON_THROW(type_error::create(310, detail::concat("cannot use swap(binary_t::container_type&) with ", type_name()), this));
        }
    }

    /// @}

    //////////////////////////////////////////
    // lexicographical comparison operators //
    //////////////////////////////////////////

    /// @name lexicographical comparison operators
    /// @{

    // note parentheses around operands are necessary; see
    // https://github.com/nlohmann/json/issues/1530
#define JSON_IMPLEMENT_OPERATOR(op, null_result, unordered_result, default_result)                       \
    const auto lhs_type = lhs.type();                                                                    \
    const auto rhs_type = rhs.type();                                                                    \
    \
    if (lhs_type == rhs_type) /* NOLINT(readability/braces) */                                           \
    {                                                                                                    \
        switch (lhs_type)                                                                                \
        {                                                                                                \
            case value_t::array:                                                                         \
                return (*lhs.data_.value_.array) op (*rhs.data_.value_.array);                                     \
                \
            case value_t::object:                                                                        \
                return (*lhs.data_.value_.object) op (*rhs.data_.value_.object);                                   \
                \
            case value_t::null:                                                                          \
                return (null_result);                                                                    \
                \
            case value_t::string:                                                                        \
                return (*lhs.data_.value_.string) op (*rhs.data_.value_.string);                                   \
                \
            case value_t::boolean:                                                                       \
                return (lhs.data_.value_.boolean) op (rhs.data_.value_.boolean);                                   \
                \
            case value_t::number_integer:                                                                \
                return (lhs.data_.value_.number_integer) op (rhs.data_.value_.number_integer);                     \
                \
            case value_t::number_unsigned:                                                               \
                return (lhs.data_.value_.number_unsigned) op (rhs.data_.value_.number_unsigned);                   \
                \
            case value_t::number_float:                                                                  \
                return (lhs.data_.value_.number_float) op (rhs.data_.value_.number_float);                         \
                \
            case value_t::binary:                                                                        \
                return (*lhs.data_.value_.binary) op (*rhs.data_.value_.binary);                                   \
                \
            case value_t::discarded:                                                                     \
            default:                                                                                     \
                return (unordered_result);                                                               \
        }                                                                                                \
    }                                                                                                    \
    else if (lhs_type == value_t::number_integer && rhs_type == value_t::number_float)                   \
    {                                                                                                    \
        return static_cast<number_float_t>(lhs.data_.value_.number_integer) op rhs.data_.value_.number_float;      \
    }                                                                                                    \
    else if (lhs_type == value_t::number_float && rhs_type == value_t::number_integer)                   \
    {                                                                                                    \
        return lhs.data_.value_.number_float op static_cast<number_float_t>(rhs.data_.value_.number_integer);      \
    }                                                                                                    \
    else if (lhs_type == value_t::number_unsigned && rhs_type == value_t::number_float)                  \
    {                                                                                                    \
        return static_cast<number_float_t>(lhs.data_.value_.number_unsigned) op rhs.data_.value_.number_float;     \
    }                                                                                                    \
    else if (lhs_type == value_t::number_float && rhs_type == value_t::number_unsigned)                  \
    {                                                                                                    \
        return lhs.data_.value_.number_float op static_cast<number_float_t>(rhs.data_.value_.number_unsigned);     \
    }                                                                                                    \
    else if (lhs_type == value_t::number_unsigned && rhs_type == value_t::number_integer)                \
    {                                                                                                    \
        return static_cast<number_integer_t>(lhs.data_.value_.number_unsigned) op rhs.data_.value_.number_integer; \
    }                                                                                                    \
    else if (lhs_type == value_t::number_integer && rhs_type == value_t::number_unsigned)                \
    {                                                                                                    \
        return lhs.data_.value_.number_integer op static_cast<number_integer_t>(rhs.data_.value_.number_unsigned); \
    }                                                                                                    \
    else if(compares_unordered(lhs, rhs))\
    {\
        return (unordered_result);\
    }\
    \
    return (default_result);

  JSON_PRIVATE_UNLESS_TESTED:
    // returns true if:
    // - any operand is NaN and the other operand is of number type
    // - any operand is discarded
    // in legacy mode, discarded values are considered ordered if
    // an operation is computed as an odd number of inverses of others
    static bool compares_unordered(const_reference lhs, const_reference rhs, bool inverse = false) noexcept
    {
        if ((lhs.is_number_float() && std::isnan(lhs.data_.value_.number_float) && rhs.is_number())
                || (rhs.is_number_float() && std::isnan(rhs.data_.value_.number_float) && lhs.is_number()))
        {
            return true;
        }
#if JSON_USE_LEGACY_DISCARDED_VALUE_COMPARISON
        return (lhs.is_discarded() || rhs.is_discarded()) && !inverse;
#else
        static_cast<void>(inverse);
        return lhs.is_discarded() || rhs.is_discarded();
#endif
    }

  private:
    bool compares_unordered(const_reference rhs, bool inverse = false) const noexcept
    {
        return compares_unordered(*this, rhs, inverse);
    }

  public:
#if JSON_HAS_THREE_WAY_COMPARISON
    /// @brief comparison: equal
    /// @sa https://json.nlohmann.me/api/basic_json/operator_eq/
    bool operator==(const_reference rhs) const noexcept
    {
#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
#endif
        const_reference lhs = *this;
        JSON_IMPLEMENT_OPERATOR( ==, true, false, false)
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif
    }

    /// @brief comparison: equal
    /// @sa https://json.nlohmann.me/api/basic_json/operator_eq/
    template<typename ScalarType>
    requires std::is_scalar_v<ScalarType>
    bool operator==(ScalarType rhs) const noexcept
    {
        return *this == basic_json(rhs);
    }

    /// @brief comparison: not equal
    /// @sa https://json.nlohmann.me/api/basic_json/operator_ne/
    bool operator!=(const_reference rhs) const noexcept
    {
        if (compares_unordered(rhs, true))
        {
            return false;
        }
        return !operator==(rhs);
    }

    /// @brief comparison: 3-way
    /// @sa https://json.nlohmann.me/api/basic_json/operator_spaceship/
    std::partial_ordering operator<=>(const_reference rhs) const noexcept // *NOPAD*
    {
        const_reference lhs = *this;
        // default_result is used if we cannot compare values. In that case,
        // we compare types.
        JSON_IMPLEMENT_OPERATOR(<=>, // *NOPAD*
                                std::partial_ordering::equivalent,
                                std::partial_ordering::unordered,
                                lhs_type <=> rhs_type) // *NOPAD*
    }

    /// @brief comparison: 3-way
    /// @sa https://json.nlohmann.me/api/basic_json/operator_spaceship/
    template<typename ScalarType>
    requires std::is_scalar_v<ScalarType>
    std::partial_ordering operator<=>(ScalarType rhs) const noexcept // *NOPAD*
    {
        return *this <=> basic_json(rhs); // *NOPAD*
    }

#if JSON_USE_LEGACY_DISCARDED_VALUE_COMPARISON
    // all operators that are computed as an odd number of inverses of others
    // need to be overloaded to emulate the legacy comparison behavior

    /// @brief comparison: less than or equal
    /// @sa https://json.nlohmann.me/api/basic_json/operator_le/
    JSON_HEDLEY_DEPRECATED_FOR(3.11.0, undef JSON_USE_LEGACY_DISCARDED_VALUE_COMPARISON)
    bool operator<=(const_reference rhs) const noexcept
    {
        if (compares_unordered(rhs, true))
        {
            return false;
        }
        return !(rhs < *this);
    }

    /// @brief comparison: less than or equal
    /// @sa https://json.nlohmann.me/api/basic_json/operator_le/
    template<typename ScalarType>
    requires std::is_scalar_v<ScalarType>
    bool operator<=(ScalarType rhs) const noexcept
    {
        return *this <= basic_json(rhs);
    }

    /// @brief comparison: greater than or equal
    /// @sa https://json.nlohmann.me/api/basic_json/operator_ge/
    JSON_HEDLEY_DEPRECATED_FOR(3.11.0, undef JSON_USE_LEGACY_DISCARDED_VALUE_COMPARISON)
    bool operator>=(const_reference rhs) const noexcept
    {
        if (compares_unordered(rhs, true))
        {
            return false;
        }
        return !(*this < rhs);
    }

    /// @brief comparison: greater than or equal
    /// @sa https://json.nlohmann.me/api/basic_json/operator_ge/
    template<typename ScalarType>
    requires std::is_scalar_v<ScalarType>
    bool operator>=(ScalarType rhs) const noexcept
    {
        return *this >= basic_json(rhs);
    }
#endif
#else
    /// @brief comparison: equal
    /// @sa https://json.nlohmann.me/api/basic_json/operator_eq/
    friend bool operator==(const_reference lhs, const_reference rhs) noexcept
    {
#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
#endif
        JSON_IMPLEMENT_OPERATOR( ==, true, false, false)
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif
    }

    /// @brief comparison: equal
    /// @sa https://json.nlohmann.me/api/basic_json/operator_eq/
    template<typename ScalarType, typename std::enable_if<
                 std::is_scalar<ScalarType>::value, int>::type = 0>
    friend bool operator==(const_reference lhs, ScalarType rhs) noexcept
    {
        return lhs == basic_json(rhs);
    }

    /// @brief comparison: equal
    /// @sa https://json.nlohmann.me/api/basic_json/operator_eq/
    template<typename ScalarType, typename std::enable_if<
                 std::is_scalar<ScalarType>::value, int>::type = 0>
    friend bool operator==(ScalarType lhs, const_reference rhs) noexcept
    {
        return basic_json(lhs) == rhs;
    }

    /// @brief comparison: not equal
    /// @sa https://json.nlohmann.me/api/basic_json/operator_ne/
    friend bool operator!=(const_reference lhs, const_reference rhs) noexcept
    {
        if (compares_unordered(lhs, rhs, true))
        {
            return false;
        }
        return !(lhs == rhs);
    }

    /// @brief comparison: not equal
    /// @sa https://json.nlohmann.me/api/basic_json/operator_ne/
    template<typename ScalarType, typename std::enable_if<
                 std::is_scalar<ScalarType>::value, int>::type = 0>
    friend bool operator!=(const_reference lhs, ScalarType rhs) noexcept
    {
        return lhs != basic_json(rhs);
    }

    /// @brief comparison: not equal
    /// @sa https://json.nlohmann.me/api/basic_json/operator_ne/
    template<typename ScalarType, typename std::enable_if<
                 std::is_scalar<ScalarType>::value, int>::type = 0>
    friend bool operator!=(ScalarType lhs, const_reference rhs) noexcept
    {
        return basic_json(lhs) != rhs;
    }

    /// @brief comparison: less than
    /// @sa https://json.nlohmann.me/api/basic_json/operator_lt/
    friend bool operator<(const_reference lhs, const_reference rhs) noexcept
    {
        // default_result is used if we cannot compare values. In that case,
        // we compare types. Note we have to call the operator explicitly,
        // because MSVC has problems otherwise.
        JSON_IMPLEMENT_OPERATOR( <, false, false, operator<(lhs_type, rhs_type))
    }

    /// @brief comparison: less than
    /// @sa https://json.nlohmann.me/api/basic_json/operator_lt/
    template<typename ScalarType, typename std::enable_if<
                 std::is_scalar<ScalarType>::value, int>::type = 0>
    friend bool operator<(const_reference lhs, ScalarType rhs) noexcept
    {
        return lhs < basic_json(rhs);
    }

    /// @brief comparison: less than
    /// @sa https://json.nlohmann.me/api/basic_json/operator_lt/
    template<typename ScalarType, typename std::enable_if<
                 std::is_scalar<ScalarType>::value, int>::type = 0>
    friend bool operator<(ScalarType lhs, const_reference rhs) noexcept
    {
        return basic_json(lhs) < rhs;
    }

    /// @brief comparison: less than or equal
    /// @sa https://json.nlohmann.me/api/basic_json/operator_le/
    friend bool operator<=(const_reference lhs, const_reference rhs) noexcept
    {
        if (compares_unordered(lhs, rhs, true))
        {
            return false;
        }
        return !(rhs < lhs);
    }

    /// @brief comparison: less than or equal
    /// @sa https://json.nlohmann.me/api/basic_json/operator_le/
    template<typename ScalarType, typename std::enable_if<
                 std::is_scalar<ScalarType>::value, int>::type = 0>
    friend bool operator<=(const_reference lhs, ScalarType rhs) noexcept
    {
        return lhs <= basic_json(rhs);
    }

    /// @brief comparison: less than or equal
    /// @sa https://json.nlohmann.me/api/basic_json/operator_le/
    template<typename ScalarType, typename std::enable_if<
                 std::is_scalar<ScalarType>::value, int>::type = 0>
    friend bool operator<=(ScalarType lhs, const_reference rhs) noexcept
    {
        return basic_json(lhs) <= rhs;
    }

    /// @brief comparison: greater than
    /// @sa https://json.nlohmann.me/api/basic_json/operator_gt/
    friend bool operator>(const_reference lhs, const_reference rhs) noexcept
    {
        // double inverse
        if (compares_unordered(lhs, rhs))
        {
            return false;
        }
        return !(lhs <= rhs);
    }

    /// @brief comparison: greater than
    /// @sa https://json.nlohmann.me/api/basic_json/operator_gt/
    template<typename ScalarType, typename std::enable_if<
                 std::is_scalar<ScalarType>::value, int>::type = 0>
    friend bool operator>(const_reference lhs, ScalarType rhs) noexcept
    {
        return lhs > basic_json(rhs);
    }

    /// @brief comparison: greater than
    /// @sa https://json.nlohmann.me/api/basic_json/operator_gt/
    template<typename ScalarType, typename std::enable_if<
                 std::is_scalar<ScalarType>::value, int>::type = 0>
    friend bool operator>(ScalarType lhs, const_reference rhs) noexcept
    {
        return basic_json(lhs) > rhs;
    }

    /// @brief comparison: greater than or equal
    /// @sa https://json.nlohmann.me/api/basic_json/operator_ge/
    friend bool operator>=(const_reference lhs, const_reference rhs) noexcept
    {
        if (compares_unordered(lhs, rhs, true))
        {
            return false;
        }
        return !(lhs < rhs);
    }

    /// @brief comparison: greater than or equal
    /// @sa https://json.nlohmann.me/api/basic_json/operator_ge/
    template<typename ScalarType, typename std::enable_if<
                 std::is_scalar<ScalarType>::value, int>::type = 0>
    friend bool operator>=(const_reference lhs, ScalarType rhs) noexcept
    {
        return lhs >= basic_json(rhs);
    }

    /// @brief comparison: greater than or equal
    /// @sa https://json.nlohmann.me/api/basic_json/operator_ge/
    template<typename ScalarType, typename std::enable_if<
                 std::is_scalar<ScalarType>::value, int>::type = 0>
    friend bool operator>=(ScalarType lhs, const_reference rhs) noexcept
    {
        return basic_json(lhs) >= rhs;
    }
#endif

#undef JSON_IMPLEMENT_OPERATOR


#ifndef JSON_NO_IO
    JSON_HEDLEY_DEPRECATED_FOR(3.0.0, operator>>(std::istream&, basic_json&))
    friend std::istream& operator<<(basic_json& j, std::istream& i)
    {
        return operator>>(i, j);
    }

    /// @brief deserialize from stream
    /// @sa https://json.nlohmann.me/api/basic_json/operator_gtgt/
    friend std::istream& operator>>(std::istream& i, basic_json& j)
    {
        parser(detail::input_adapter(i)).parse(false, j);
        return i;
    }
#endif  // JSON_NO_IO
    JSON_HEDLEY_RETURNS_NON_NULL
	virtual const char* type_name() const noexcept {
		switch (data_.type_) {
			case NDT_NULL: return "NULL";
			case NDT_OBJECT: return "OBJECT";
			case NDT_ARRAY: return "ARRAY";
			case NDT_STRING: return "STRING";
			case NDT_BOOL: return "BOOL";
			case NDT_INT: return "INT";
			case NDT_FLOAT: return "FLOAT";
			default: return "UNKNOWN";
		}
	}

  public:

	reference operator [](const json_pointer& ptr)
    {
        return ptr.get_unchecked(this);
    }

    template<typename BasicJsonType, detail::enable_if_t<detail::is_basic_json<BasicJsonType>::value, int> = 0>
    JSON_HEDLEY_DEPRECATED_FOR(3.11.0, basic_json::json_pointer or nlohmann::json_pointer<basic_json::string_t>) // NOLINT(readability/alt_tokens)
    reference operator[](const ::nlohmann::json_pointer<BasicJsonType>& ptr)
    {
        return ptr.get_unchecked(this);
    }

    const_reference operator[](const json_pointer& ptr) const
    {
        return ptr.get_unchecked(this);
    }

    template<typename BasicJsonType, detail::enable_if_t<detail::is_basic_json<BasicJsonType>::value, int> = 0>
    JSON_HEDLEY_DEPRECATED_FOR(3.11.0, basic_json::json_pointer or nlohmann::json_pointer<basic_json::string_t>) // NOLINT(readability/alt_tokens)
    const_reference operator[](const ::nlohmann::json_pointer<BasicJsonType>& ptr) const
    {
        return ptr.get_unchecked(this);
    }

    reference at(const json_pointer& ptr)
    {
        return ptr.get_checked(this);
    }

    template<typename BasicJsonType, detail::enable_if_t<detail::is_basic_json<BasicJsonType>::value, int> = 0>
    JSON_HEDLEY_DEPRECATED_FOR(3.11.0, basic_json::json_pointer or nlohmann::json_pointer<basic_json::string_t>) // NOLINT(readability/alt_tokens)
    reference at(const ::nlohmann::json_pointer<BasicJsonType>& ptr)
    {
        return ptr.get_checked(this);
    }

    const_reference at(const json_pointer& ptr) const
    {
        return ptr.get_checked(this);
    }

    template<typename BasicJsonType, detail::enable_if_t<detail::is_basic_json<BasicJsonType>::value, int> = 0>
    JSON_HEDLEY_DEPRECATED_FOR(3.11.0, basic_json::json_pointer or nlohmann::json_pointer<basic_json::string_t>) // NOLINT(readability/alt_tokens)
    const_reference at(const ::nlohmann::json_pointer<BasicJsonType>& ptr) const
    {
        return ptr.get_checked(this);
    }

    basic_json flatten() const
    {
        basic_json result(value_t::object);
        json_pointer::flatten("", *this, result);
        return result;
    }

    notation unflatten() const
    {
        return json_pointer::unflatten(*this);
    }
    void patch_inplace(const basic_json& json_patch)
    {
        basic_json& result = *this;
        // the valid JSON Patch operations
        enum class patch_operations {add, remove, replace, move, copy, test, invalid};

        const auto get_op = [](const std::string & op)
        {
            if (op == "add")
            {
                return patch_operations::add;
            }
            if (op == "remove")
            {
                return patch_operations::remove;
            }
            if (op == "replace")
            {
                return patch_operations::replace;
            }
            if (op == "move")
            {
                return patch_operations::move;
            }
            if (op == "copy")
            {
                return patch_operations::copy;
            }
            if (op == "test")
            {
                return patch_operations::test;
            }

            return patch_operations::invalid;
        };

        // wrapper for "add" operation; add value at ptr
        const auto operation_add = [&result](json_pointer & ptr, basic_json val)
        {
            // adding to the root of the target document means replacing it
            if (ptr.empty())
            {
                result = val;
                return;
            }

            // make sure the top element of the pointer exists
            json_pointer const top_pointer = ptr.top();
            if (top_pointer != ptr)
            {
                result.at(top_pointer);
            }

            // get reference to parent of JSON pointer ptr
            const auto last_path = ptr.back();
            ptr.pop_back();
            // parent must exist when performing patch add per RFC6902 specs
            basic_json& parent = result.at(ptr);

            switch (parent.data_.type_)
            {
                case value_t::null:
                case value_t::object:
                {
                    // use operator[] to add value
                    parent[last_path] = val;
                    break;
                }

                case value_t::array:
                {
                    if (last_path == "-")
                    {
                        // special case: append to back
                        parent.push_back(val);
                    }
                    else
                    {
                        const auto idx = json_pointer::template array_index<basic_json_t>(last_path);
                        if (JSON_HEDLEY_UNLIKELY(idx > parent.size()))
                        {
                            // avoid undefined behavior
                            JSON_THROW(out_of_range::create(401, detail::concat("array index ", std::to_string(idx), " is out of range"), &parent));
                        }

                        // default case: insert add offset
                        parent.insert(parent.begin() + static_cast<difference_type>(idx), val);
                    }
                    break;
                }

                // if there exists a parent it cannot be primitive
                case value_t::string: // LCOV_EXCL_LINE
                case value_t::boolean: // LCOV_EXCL_LINE
                case value_t::number_integer: // LCOV_EXCL_LINE
                case value_t::number_unsigned: // LCOV_EXCL_LINE
                case value_t::number_float: // LCOV_EXCL_LINE
                case value_t::binary: // LCOV_EXCL_LINE
                case value_t::discarded: // LCOV_EXCL_LINE
                default:            // LCOV_EXCL_LINE
                    JSON_ASSERT(false); // NOLINT(cert-dcl03-c,hicpp-static-assert,misc-static-assert) LCOV_EXCL_LINE
            }
        };

        // wrapper for "remove" operation; remove value at ptr
        const auto operation_remove = [this, & result](json_pointer & ptr)
        {
            // get reference to parent of JSON pointer ptr
            const auto last_path = ptr.back();
            ptr.pop_back();
            basic_json& parent = result.at(ptr);

            // remove child
            if (parent.data_.type_==NDT_OBJECT)
            {
                // perform range check
                auto it = parent.find(last_path);
                if (JSON_HEDLEY_LIKELY(it != parent.end()))
                {
                    parent.erase(it);
                }
                else
                {
                    JSON_THROW(out_of_range::create(403, detail::concat("key '", last_path, "' not found"), this));
                }
            }
            else if (parent.data_.type_==NDT_ARRAY)
            {
                // note erase performs range check
                parent.erase(json_pointer::template array_index<basic_json_t>(last_path));
            }
        };

        // type check: top level value must be an array
        if (JSON_HEDLEY_UNLIKELY(!json_patch.data_.type_==NDT_ARRAY))
        {
            JSON_THROW(parse_error::create(104, 0, "JSON patch must be an array of objects", &json_patch));
        }

        // iterate and apply the operations
        for (const auto& val : json_patch)
        {
            // wrapper to get a value for an operation
            const auto get_value = [&val](const std::string & op,
                                          const std::string & member,
                                          bool string_type) -> basic_json &
            {
                // find value
                auto it = val.data_.value_.object->find(member);

                // context-sensitive error message
                const auto error_msg = (op == "op") ? "operation" : detail::concat("operation '", op, '\'');

                // check if desired value is present
                if (JSON_HEDLEY_UNLIKELY(it == val.data_.value_.object->end()))
                {
                    // NOLINTNEXTLINE(performance-inefficient-string-concatenation)
                    JSON_THROW(parse_error::create(105, 0, detail::concat(error_msg, " must have member '", member, "'"), &val));
                }

                // check if result is of type string
                if (JSON_HEDLEY_UNLIKELY(string_type && !it->second.is_string()))
                {
                    // NOLINTNEXTLINE(performance-inefficient-string-concatenation)
                    JSON_THROW(parse_error::create(105, 0, detail::concat(error_msg, " must have string member '", member, "'"), &val));
                }

                // no error: return value
                return it->second;
            };

            // type check: every element of the array must be an object
            if (JSON_HEDLEY_UNLIKELY(!val.data_.type_==NDT_OBJECT))
            {
                JSON_THROW(parse_error::create(104, 0, "JSON patch must be an array of objects", &val));
            }

            // collect mandatory members
            const auto op = get_value("op", "op", true).template get<std::string>();
            const auto path = get_value(op, "path", true).template get<std::string>();
            json_pointer ptr(path);

            switch (get_op(op))
            {
                case patch_operations::add:
                {
                    operation_add(ptr, get_value("add", "value", false));
                    break;
                }

                case patch_operations::remove:
                {
                    operation_remove(ptr);
                    break;
                }

                case patch_operations::replace:
                {
                    // the "path" location must exist - use at()
                    result.at(ptr) = get_value("replace", "value", false);
                    break;
                }

                case patch_operations::move:
                {
                    const auto from_path = get_value("move", "from", true).template get<std::string>();
                    json_pointer from_ptr(from_path);

                    // the "from" location must exist - use at()
                    basic_json const v = result.at(from_ptr);

                    // The move operation is functionally identical to a
                    // "remove" operation on the "from" location, followed
                    // immediately by an "add" operation at the target
                    // location with the value that was just removed.
                    operation_remove(from_ptr);
                    operation_add(ptr, v);
                    break;
                }

                case patch_operations::copy:
                {
                    const auto from_path = get_value("copy", "from", true).template get<std::string>();
                    const json_pointer from_ptr(from_path);

                    // the "from" location must exist - use at()
                    basic_json const v = result.at(from_ptr);

                    // The copy is functionally identical to an "add"
                    // operation at the target location using the value
                    // specified in the "from" member.
                    operation_add(ptr, v);
                    break;
                }

                case patch_operations::test:
                {
                    bool success = false;
                    JSON_TRY
                    {
                        // check if "value" matches the one at "path"
                        // the "path" location must exist - use at()
                        success = (result.at(ptr) == get_value("test", "value", false));
                    }
                    JSON_INTERNAL_CATCH (out_of_range&)
                    {
                        // ignore out of range errors: success remains false
                    }

                    // throw an exception if test fails
                    if (JSON_HEDLEY_UNLIKELY(!success))
                    {
                        JSON_THROW(other_error::create(501, detail::concat("unsuccessful: ", val.dump()), &val));
                    }

                    break;
                }

                case patch_operations::invalid:
                default:
                {
                    // op must be "add", "remove", "replace", "move", "copy", or
                    // "test"
                    JSON_THROW(parse_error::create(105, 0, detail::concat("operation value '", op, "' is invalid"), &val));
                }
            }
        }
    }
	notation patch(const notation& notation_patch) const {
		notation result=*this;
		result.patch_inplace(notation_patch);
		return result;
	}
    JSON_HEDLEY_WARN_UNUSED_RESULT
    static basic_json diff(const basic_json& source, const basic_json& target,
                           const std::string& path = "")
    {
        // the patch
        basic_json result(value_t::array);

        // if the values are the same, return empty patch
        if (source == target)
        {
            return result;
        }

        if (source.type() != target.type())
        {
            // different types: replace value
            result.push_back(
            {
                {"op", "replace"}, {"path", path}, {"value", target}
            });
            return result;
        }

        switch (source.type())
        {
            case value_t::array:
            {
                // first pass: traverse common elements
                std::size_t i = 0;
                while (i < source.size() && i < target.size())
                {
                    // recursive call to compare array values at index i
                    auto temp_diff = diff(source[i], target[i], detail::concat(path, '/', std::to_string(i)));
                    result.insert(result.end(), temp_diff.begin(), temp_diff.end());
                    ++i;
                }

                // We now reached the end of at least one array
                // in a second pass, traverse the remaining elements

                // remove my remaining elements
                const auto end_index = static_cast<difference_type>(result.size());
                while (i < source.size())
                {
                    // add operations in reverse order to avoid invalid
                    // indices
                    result.insert(result.begin() + end_index, object(
                    {
                        {"op", "remove"},
                        {"path", detail::concat(path, '/', std::to_string(i))}
                    }));
                    ++i;
                }

                // add other remaining elements
                while (i < target.size())
                {
                    result.push_back(
                    {
                        {"op", "add"},
                        {"path", detail::concat(path, "/-")},
                        {"value", target[i]}
                    });
                    ++i;
                }

                break;
            }

            case value_t::object:
            {
                // first pass: traverse this object's elements
                for (auto it = source.cbegin(); it != source.cend(); ++it)
                {
                    // escape the key name to be used in a JSON patch
                    const auto path_key = detail::concat(path, '/', detail::escape(it.key()));

                    if (target.find(it.key()) != target.end())
                    {
                        // recursive call to compare object values at key it
                        auto temp_diff = diff(it.value(), target[it.key()], path_key);
                        result.insert(result.end(), temp_diff.begin(), temp_diff.end());
                    }
                    else
                    {
                        // found a key that is not in o -> remove it
                        result.push_back(object(
                        {
                            {"op", "remove"}, {"path", path_key}
                        }));
                    }
                }

                // second pass: traverse other object's elements
                for (auto it = target.cbegin(); it != target.cend(); ++it)
                {
                    if (source.find(it.key()) == source.end())
                    {
                        // found a key that is not in this -> add it
                        const auto path_key = detail::concat(path, '/', detail::escape(it.key()));
                        result.push_back(
                        {
                            {"op", "add"}, {"path", path_key},
                            {"value", it.value()}
                        });
                    }
                }

                break;
            }

            case value_t::null:
            case value_t::string:
            case value_t::boolean:
            case value_t::number_integer:
            case value_t::number_unsigned:
            case value_t::number_float:
            case value_t::binary:
            case value_t::discarded:
            default:
            {
                // both primitive type: replace value
                result.push_back(
                {
                    {"op", "replace"}, {"path", path}, {"value", target}
                });
                break;
            }
        }

        return result;
    }
	void merge_patch(const notation& apply_patch) {
		if (apply_patch.data_.type_==NDT_OBJECT) {
			if (data_.type_!=NDT_OBJECT) *this=object();
			for (auto it=apply_patch.begin();it!=apply_patch.end();it++) {
				if (it.value().data_.type_==NDT_NULL) erase(it.key());
				else operator [](it.key()).merge_patch(it.value());
			}
		} else *this=apply_patch;
	}
};

//json_serializer
//json_parser
//json_iterator
//json_sax_dom_parser
//json_sax_dom_callback_parser
//json_sax_acceptor
//json_lexer(adapter_type)

//const error_handler_t error_handler=error_handler_t::strict

}

}

#endif
//1346 754