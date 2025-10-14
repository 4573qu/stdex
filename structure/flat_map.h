//Last Modified At 2025/10/11
//@Version 1.0.0.0
#ifndef _STDEX_STRUCTURE_FLAT_MAP_H_
#define _STDEX_STRUCTURE_FLAT_MAP_H_ 1

#include <algorithm>
#include <functional>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

#include "../macros/cpp_version.h"//At Least 1.0

#ifndef _STDEX_CPP20_VERSION
#define _STDEX_CPP20_VERSION 202002L
#endif

#if __cplusplus>=_STDEX_CPP20_VERSION
#include <ranges>
#endif

namespace stdex {

namespace structure {

template <typename _Key,typename _Tp,typename _Compare=std::less<_Key>,class _Allocator=std::allocator<std::pair<_Key,_Tp>>>
class flat_map {
private:
	using value_type=std::pair<_Key,_Tp>;
	using container_type=std::vector<value_type>;
	
public:
	using key_type=_Key;
	using mapped_type=_Tp;
	using key_compare=_Compare;
	using allocator_type=_Allocator;
	using size_type=typename container_type::size_type;
	using difference_type=typename container_type::difference_type;
	using reference=value_type&;
	using const_reference=const value_type&;
	using pointer=typename std::allocator_traits<_Allocator>::pointer;
	using const_pointer=typename std::allocator_traits<_Allocator>::const_pointer;

	using iterator=typename container_type::iterator;
	using const_iterator=typename container_type::const_iterator;
	using reverse_iterator=typename container_type::reverse_iterator;
	using const_reverse_iterator=typename container_type::const_reverse_iterator;
	
private:
	container_type data_;
	key_compare comp_;
	
	template <typename _K>
	iterator find_impl(const _K& key) {
		auto it=std::lower_bound(data_.begin(),data_.end(),key,[this](const value_type& elem,const _K& k){
			return comp_(elem.first,k);
		});
		if (it!=data_.end() && !comp_(key,it->first)) return it;
		return data_.end();
	}
	template <typename _K>
	const_iterator find_impl(const _K& key) const {
		auto it=std::lower_bound(data_.begin(),data_.end(),key,[this](const value_type& elem,const _K& k) {
			return comp_(elem.first,k);
		});
		if (it!=data_.end() && !comp_(key,it->first)) return it;
		return data_.end();
	}
	
public:
	flat_map()=default;
	explicit flat_map(const _Compare& comp,const _Allocator& alloc=_Allocator()) : data_(alloc) , comp_(comp) {}
	explicit flat_map(const _Allocator& alloc) : data_(alloc) , comp_(_Compare()) {}
	template <typename _InputIt>
	flat_map(_InputIt first,_InputIt last,const _Compare& comp=_Compare(),const _Allocator& alloc=_Allocator()) : comp_(comp) {
		insert(first,last);
	}
	flat_map(std::initializer_list<value_type> init,const _Compare& comp=_Compare(),const _Allocator& alloc=_Allocator()) : comp_(comp) {
		insert(init.begin(),init.end());
	}
	
	flat_map(const flat_map&)=default;
	flat_map(const flat_map& other,const _Allocator& alloc) : data_(other.data_,alloc) , comp_(other.comp_) {}
	flat_map(flat_map&&) noexcept=default;
	flat_map(flat_map&& other,const _Allocator& alloc) : data_(std::move(other.data_) ,alloc), comp_(std::move(other.comp_)) {}
	~flat_map()=default;

	flat_map& operator =(const flat_map& other) {
		if (this!=&other) {
			data_=other.data_;
 			comp_=other.comp_;
		}
		return *this;
	};
	flat_map& operator =(flat_map&&) noexcept = default;
	/*flat_map& operator =(flat_map&& other) noexcept (std::allocator_traits<_Allocator>::propagate_on_container_move_assignment::value || std::allocator_traits<_Allocator>::is_always_equal::value) {
		if (this!=&other) {
			data_=std::move(other.data_);
			comp_=std::move(other.comp_);
		}
		return *this;
	}*/
	flat_map& operator =(std::initializer_list<value_type> init_list) {
        	clear();
        	insert(init_list.begin(),init_list.end());
		return *this;
	}

	bool operator ==(const flat_map& other) const {
		return data_==other.data_;
	}
	bool operator !=(const flat_map& other) const {
		return !(*this==other);
	}
	bool operator <(const flat_map& other) const {
		return std::lexicographical_compare(data_.begin(),data_.end(),other.data_.begin(),other.data_.end(),[this](const value_type& a,const value_type& b){
			return comp_(a.first, b.first) || (!comp_(b.first, a.first) && a.second < b.second);
		});
	}
	bool operator <=(const flat_map& other) const {
		return !(other<*this);
	}
	bool operator >(const flat_map& other) const {
		return other<*this;
	}
	bool operator >=(const flat_map& other) const {
		return !(*this<other);
	}

	bool empty() const noexcept { return data_.empty(); }
	size_type size() const noexcept { return data_.size(); }
	size_type max_size() const noexcept { return data_.max_size(); }
	size_type capacity() const noexcept { return data_.capacity(); }
	void reserve(size_type new_cap) { data_.reserve(new_cap); }
	void shrink_to_fit() { data_.shrink_to_fit(); }

	iterator begin() noexcept { return data_.begin(); }
	const_iterator begin() const noexcept { return data_.begin(); }
	const_iterator cbegin() const noexcept { return data_.cbegin(); }

	iterator end() noexcept { return data_.end(); }
	const_iterator end() const noexcept { return data_.end(); }
	const_iterator cend() const noexcept { return data_.cend(); }

	reverse_iterator rbegin() noexcept { return data_.rbegin(); }
	const_reverse_iterator rbegin() const noexcept { return data_.rbegin(); }
	const_reverse_iterator crbegin() const noexcept { return data_.crbegin(); }

	reverse_iterator rend() noexcept { return data_.rend(); }
	const_reverse_iterator rend() const noexcept { return data_.rend(); }
	const_reverse_iterator crend() const noexcept { return data_.crend(); }

	template <typename _K=_Key>
	_Tp& at(const _K& key) {
		auto it=find_impl(key);
		if (it==data_.end()) throw std::out_of_range("flat_map::at: key not found");
		return it->second;
	}
	template <typename _K=_Key>
	const _Tp& at(const _K& key) const {
		auto it=find_impl(key);
		if (it==data_.end()) throw std::out_of_range("flat_map::at: key not found");
		return it->second;
	}
	template <typename _K=_Key>
	_Tp& operator [](const _K& key) {
		auto it=find_impl(key);
		if (it==data_.end()) it=data_.insert(it,value_type(key,_Tp{}));
		return it->second;
	}
	template <typename _K=_Key>
	_Tp& operator [](_K&& key) {
		auto it=find_impl(key);
		if (it==data_.end()) it=data_.insert(it,value_type(std::move(key),_Tp{}));
		return it->second;
	}

	template <typename _K=_Key>
	iterator find(const _K& key) {
		return find_impl(key);
	}
	template <typename _K=_Key>
	const_iterator find(const _K& key) const {
		return find_impl(key);
	}
	template <typename _K=_Key>
	bool contains(const _K& key) const {
		return find_impl(key)!=data_.end();
	}
	template <typename _K=_Key>
	size_type count(const _K& key) const {
		return contains(key)?1:0;
	}
	template <typename _K=_Key>
	iterator lower_bound(const _K& key) {
		return std::lower_bound(data_.begin(),data_.end(),key,[this](const value_type& elem,const _K& k){
			return comp_(elem.first, k);
		});
	}
	template <typename _K=_Key>
	const_iterator lower_bound(const _K& key) const {
		return std::lower_bound(data_.begin(),data_.end(),key,[this](const value_type& elem,const _K& k){
			return comp_(elem.first,k);
		});
	}
	template <typename _K=_Key>
	iterator upper_bound(const _K& key) {
		return std::upper_bound(data_.begin(),data_.end(),key,[this](const value_type& elem,const _K& k){
			return comp_(elem.first,k);
		});
	}
	template <typename _K=_Key>
	const_iterator upper_bound(const _K& key) const {
		return std::upper_bound(data_.begin(),data_.end(),key,[this](const value_type& elem,const _K& k) {
			return comp_(elem.first,k);
		});
	}
	template <typename _K=_Key>
	std::pair<iterator,iterator> equal_range(const _K& key) {
		return std::equal_range(data_.begin(),data_.end(),key,[this](const value_type& a,const value_type& b){
			return comp_(a.first,b.first);
		});
	}
	
	template <typename _K=_Key>
	std::pair<const_iterator,const_iterator> equal_range(const _K& key) const {
		return std::equal_range(data_.begin(),data_.end(),key,[this](const value_type& a,const value_type& b){
			return comp_(a.first,b.first);
		});
	}
	
	void clear() noexcept { data_.clear(); }

	template <typename _P=std::pair<_Key,_Tp>>
	std::pair<iterator,bool> insert(const _P& value) {
		auto it=lower_bound(value.first);
		if (it!=data_.end() && !comp_(value.first,it->first)) return {it,false};
		return {data_.insert(it,value),true};
	}
	template <typename _P=std::pair<_Key,_Tp>>
	std::pair<iterator,bool> insert(_P&& value) {
		auto it=lower_bound(value.first);
		if (it!=data_.end() && !comp_(value.first,it->first)) return {it,false};
		return {data_.insert(it,std::forward<_P>(value)),true};
	}
	template <typename _P=std::pair<_Key,_Tp>>
	iterator insert(const_iterator hint,const _P& value) {
		(void)hint;
		return insert(value).first;
	}
	template <typename _P=std::pair<_Key,_Tp>>
	iterator insert(const_iterator hint,_P&& value) {
		(void)hint;
		return insert(std::forward<_P>(value)).first;
	}
	template <typename _InputIt>
	void insert(_InputIt first,_InputIt last) {
		for (auto it=first;it!=last;it++) insert(*it);
	}
	void insert(std::initializer_list<value_type> init_list) {
		insert(init_list.begin(),init_list.end());
	}
	
	template <typename _M>
	std::pair<iterator,bool> insert_or_assign(const _Key& key,_M&& obj) {
		auto it=find_impl(key);
		if (it!=data_.end()) {
			it->second=std::forward<_M>(obj);
			return {it,false};
		} else {
			it=data_.insert(it,value_type(key,std::forward<_M>(obj)));
			return {it,true};
		}
	}
	template <typename _M>
	std::pair<iterator,bool> insert_or_assign(_Key&& key,_M&& obj) {
		auto it=find_impl(key);
		if (it!=data_.end()) {
			it->second=std::forward<_M>(obj);
			return {it,false};
		} else {
			it=data_.insert(it,value_type(std::move(key),std::forward<_M>(obj)));
			return {it,true};
		}
	}
	
	template <typename... _Args>
	std::pair<iterator,bool> emplace(_Args&&... args) {
		value_type value(std::forward<_Args>(args)...);
		return insert(std::move(value));
	}
	template <typename... _Args>
	std::pair<iterator,bool> try_emplace(const _Key& key,_Args&&... args) {
		auto it=find_impl(key);
		if (it!=data_.end()) return {it,false};
		it=data_.insert(it,value_type(key,_Tp(std::forward<_Args>(args)...)));
		return {it,true};
	}
	template <typename... _Args>
	std::pair<iterator,bool> try_emplace(_Key&& key,_Args&&... args) {
		auto it=find_impl(key);
		if (it!=data_.end()) return {it,false};
		it=data_.insert(it,value_type(std::move(key),_Tp(std::forward<_Args>(args)...)));
		return {it,true};
	}

	template <typename _K>
	size_type erase(const _K& key) {
		auto it=find_impl(key);
		if (it!=data_.end()) {
			data_.erase(it);
			return 1;
		}
		return 0;
	}
	iterator erase(const_iterator pos) {
		return data_.erase(pos);
	}
	iterator erase(const_iterator first,const_iterator last) {
		return data_.erase(first,last);
	}
	void swap(flat_map& other) noexcept {
		data_.swap(other.data_);
		std::swap(comp_,other.comp_);
	}


#if __cplusplus>=_STDEX_CPP20_VERSION
	auto keys() const {
		return std::views::transform(*this,[](const auto& pair) -> const _Key& {
			return pair.first;
		});
	}
	auto values() const {
		return std::views::transform(*this,[](const auto& pair) -> const _Tp& {
			return pair.second;
		});
	}
	auto values() {
		return std::views::transform(*this,[](auto& pair) -> _Tp& {
			return pair.second;
		});
	}
#endif

	const container_type& container() const noexcept { return data_; }
	allocator_type get_allocator() const noexcept { 
		return data_.get_allocator(); 
	}
};

template <typename _Key,typename _Tp,typename _Compare,typename _Allocator>
void swap(flat_map<_Key,_Tp,_Compare,_Allocator>& lhs,flat_map<_Key,_Tp,_Compare,_Allocator>& rhs) noexcept {
	lhs.swap(rhs);
}

}

}

#endif