//Last Modified At 2025/10/15
//@Version 1.0.0.0
#ifndef _STDEX_STRUCTURE_TRIE_H_
#define _STDEX_STRUCTURE_TRIE_H_ 1

#include <algorithm>
#include <cstddef>
#include <functional>
#include <iterator>
#include <memory>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "flat_map.h"//At Least 1.0

namespace stdex {

namespace structure {
	
template <typename _Container,typename _CharT,typename _NodePtr,typename=void>
struct is_trie_container : std::false_type {};
template <typename _Container,typename _CharT,typename _NodePtr>
struct is_trie_container<_Container,_CharT,_NodePtr,std::void_t<
	typename _Container::key_type,
	typename _Container::mapped_type,
	decltype(std::declval<_Container>().insert(std::declval<std::pair<_CharT,_NodePtr>>())),
	decltype(std::declval<_Container>().find(std::declval<_CharT>())),
	decltype(std::declval<_Container>().erase(std::declval<_CharT>())),
	decltype(std::declval<_Container>().begin()),
	decltype(std::declval<_Container>().end())>
> : std::conjunction<
	std::is_same<typename _Container::key_type,_CharT>,
	std::is_same<typename _Container::mapped_type,_NodePtr>,
	std::is_convertible<
		decltype(std::declval<_Container>().insert(std::declval<std::pair<_CharT,_NodePtr>>())),
		std::pair<typename _Container::iterator,bool>
	>,
	std::is_convertible<
		decltype(std::declval<_Container>().find(std::declval<_CharT>())),
		typename _Container::iterator
	>
> {};

template <typename _CharT,typename _Tp=void,template <typename...> class _Container=flat_map,typename _Compare=std::less<_CharT>,typename _Allocator=std::allocator<std::pair<const _CharT,void*>>>
class trie {
	static_assert(is_trie_container<_Container<_CharT,void*,_Compare>,_CharT,void*>::value,"Trie container must be a complete map type!");
	struct trie_node {
		_Container<_CharT,trie_node*,_Compare,_Allocator> children_;
		_Tp* value_=nullptr;
		std::size_t count_=0;
		trie_node()=default;
		~trie_node() {
			if (value_) delete value_;
		}
	};
	trie_node* root_=nullptr;
	_Allocator alloc_;
	_Compare comp_;
	std::size_t size_=0;
	trie_node* create_node() {
		return new trie_node();
	}
	void destroy_node(trie_node* node) {
		delete node;
	}
	trie_node* find_node(const std::basic_string<_CharT>& key) const {
		trie_node* current=root_;
		for (_CharT it:key) {
			auto jt=current->children_.find(it);
			if (jt==current->children_.end()) return nullptr;
			current=jt->second;
		}
		return current;
	}

public:
	using char_type=_CharT;
	using key_type=std::basic_string<_CharT>;
	using mapped_type=_Tp;
	using value_type=std::pair<key_type,_Tp>;
	using key_compare=_Compare;
	using allocator_type=_Allocator;
	using size_type=std::size_t;
	using difference_type=std::ptrdiff_t;
	using reference=value_type&;
	using const_reference=const value_type&;
	class iterator {
		struct stack_node {
			trie_node* node_;
			key_type prefix_;
			typename decltype(trie_node::children_)::iterator child_it_;
		};
		std::vector<stack_node> stack_;
		value_type current_;
		bool at_end_=true;
		void build_path_to_target(trie_node* target_node,const key_type& target_key,trie_node* root) {
			if (!target_node || !target_node->value_) {
				at_end_=true;
				return;
			}
			stack_.clear();
			trie_node* current=root;
			key_type prefix;
			for (_CharT it:target_key) {
				auto jt=current->children_.find(it);
				if (jt==current->children_.end()) {
					at_end_=true;
					return;
				}
				stack_.push_back({current,prefix,current->children_.begin()});
				prefix+=it;
				current=jt->second;
				if (current==target_node) {
 					current_.first=prefix;
					current_.second=*(current->value_);
					at_end_=false;
					if (!current->children_.empty()) stack_.push_back({current,prefix,current->children_.begin()});
					return;
				}
			}
			at_end_=true;
		}
		void advance_to_value() {
			while (!stack_.empty()) {
				auto& top=stack_.back();
				if (top.child_it_==top.node_->children_.end()) {
					stack_.pop_back();
					continue;
				}
				auto* current_node=top.child_it_->second;
				key_type new_prefix=top.prefix_+top.child_it_->first;
				top.child_it_++;
				if (current_node->value_) {
					current_.first=new_prefix;
					current_.second=*(current_node->value_);
					at_end_=false;
					stack_.emplace_back(stack_node{current_node,new_prefix, current_node->children_.begin()});
					return;
				}
				stack_.emplace_back(stack_node{current_node,new_prefix,current_node->children_.begin()});
			}
			at_end_=true;
		}
	public:
		iterator()=default;
		iterator(trie_node* target_node,const key_type& target_key,trie_node* root)  : at_end_(target_node==nullptr || target_node->value_==nullptr) {
			if (!at_end_) build_path_to_target(target_node,target_key,root);
		}
		iterator(trie_node* root,bool end=true) : at_end_(end) {
			if (root && !end) {
				stack_.emplace_back(stack_node{root,key_type(),root->children_.begin()});
				advance_to_value();
			}
		}
		value_type& operator *() { return current_; }
		value_type* operator ->() { return &current_; }
		iterator& operator ++() {
			if (!at_end_) advance_to_value();
			return *this;
		}
		
		iterator operator ++(int) {
			iterator temp=*this;
			++(*this);
			return temp;
		}
		bool operator ==(const iterator& other) const {
			if (at_end_!=other.at_end_) return false;
			if (at_end_) return true;
			return current_.first==other.current_.first;
		}
		
		bool operator !=(const iterator& other) const {
			return !(*this==other);
		}
	};
	using const_iterator=iterator;

public:
	trie() : root_(create_node()) {}
	explicit trie(const key_compare& comp,const allocator_type& alloc=allocator_type()) : root_(create_node()) , alloc_(alloc) , comp_(comp) { }
	explicit trie(const allocator_type& alloc) : root_(create_node()) , alloc_(alloc) { }
	template <typename _InputIt>
	trie(_InputIt first,_InputIt last,const key_compare& comp=key_compare(),const allocator_type& alloc=allocator_type()) : root_(create_node()), alloc_(alloc), comp_(comp) {
		insert(first,last);
	}
	trie(std::initializer_list<value_type> init_list,const key_compare& comp=key_compare(),const allocator_type& alloc=allocator_type()) : root_(create_node()) , alloc_(alloc) , comp_(comp) {
		insert(init_list.begin(),init_list.end());
	}
	~trie() {
		clear();
		destroy_node(root_);
	}

	trie(const trie& other) : root_(create_node()) , alloc_(other.alloc_) , comp_(other.comp_) {
		for (const auto& it:other) insert(it);
	}
	trie(trie&& other) noexcept : root_(other.root_) , alloc_(std::move(other.alloc_)) , comp_(std::move(other.comp_)) , size_(other.size_) {
		other.root_=create_node();
		other.size_=0;
	}
	trie& operator =(const trie& other) {
		if (this!=&other) {
			clear();
			for (const auto& it:other) insert(it);
		}
		return *this;
	}
	trie& operator =(trie&& other) noexcept {
		if (this!=&other) {
			clear();
			destroy_node(root_);
			root_=other.root_;
			alloc_=std::move(other.alloc_);
			comp_=std::move(other.comp_);
			size_=other.size_;
			other.root_=create_node();
			other.size_=0;
		}
		return *this;
	}
	trie& operator =(std::initializer_list<value_type> init_list) {
		clear();
		insert(init_list.begin(),init_list.end());
		return *this;
	}

	mapped_type& at(const key_type& key) {
		trie_node* node=find_node(key);
		if (!node || !node->value_) throw std::out_of_range("trie::at: key not found");
		return *(node->value_);
	}
	const mapped_type& at(const key_type& key) const {
		trie_node* node=find_node(key);
		if (!node || !node->value) throw std::out_of_range("trie::at: key not found");
		return *(node->value);
	}
	mapped_type& operator [](const key_type& key) {
		trie_node* current=root_;
		for (_CharT it:key) {
			auto jt=current->children_.find(it);
			if (jt==current->children_.end()) {
				trie_node* new_node=create_node();
				current->children_.insert({it,new_node});
				current=new_node;
			} else current=jt->second;
		}
		if (!current->value_) {
			current->value_=new mapped_type();
			size_++;
		}
		return *(current->value_);
	}
	iterator begin() noexcept {
		if (!root_ || size_==0) return end();
		return iterator(root_,false);
	}
	const_iterator begin() const noexcept {
		if (!root_ || size_==0) return end();
		return const_iterator(root_,false);
	}
	const_iterator cbegin() const noexcept {
		if (!root_ || size_==0) return cend();
		return const_iterator(root_,false);
	}
	
	iterator end() noexcept { return iterator(root_,true); }
	const_iterator end() const noexcept { return const_iterator(root_,true); }
	const_iterator cend() const noexcept { return const_iterator(root_,true); }

	bool empty() const noexcept { return size_==0; }
	size_type size() const noexcept { return size_; }
	size_type max_size() const noexcept { return std::numeric_limits<size_type>::max(); }

	void clear() noexcept {
		if (!root_) return;
		clear_node(root_);
		size_=0;
		root_->children_.clear();
	}
	void clear_node(trie_node* node) noexcept {
		if (!node) return;
		for (auto& it:node->children_) {
			clear_node(it.second);
			destroy_node(it.second);
		}
		node->children_.clear();
		if (node->value_) {
			delete node->value_;
			node->value_=nullptr;
		}
	}

	std::pair<iterator,bool> insert(const value_type& value) {
		auto& key=value.first;
		trie_node* current=root_;
		for (_CharT it:key) {
			auto jt=current->children_.find(it);
			if (jt==current->children_.end()) {
				trie_node* new_node=create_node();
				current->children_.insert({it,new_node});
				current=new_node;
			} else current=jt->second;
		}
		bool inserted=false;
		if (!current->value_) {
			current->value_=new mapped_type(value.second);
			size_++;
			inserted=true;
		} else *(current->value_)=value.second;
		return {iterator(),inserted};
	}
	
	template <typename _InputIt>
	void insert(_InputIt first,_InputIt last) {
		for (auto it=first;it!=last;it++) insert(*it);
	}
	void insert(std::initializer_list<value_type> init_list) {
		insert(init_list.begin(),init_list.end());
	}
	template <typename... _Args>
	std::pair<iterator,bool> emplace(const key_type& key,_Args&&... args) {
		trie_node* current=root_;
		for (_CharT it:key) {
			auto jt=current->children_.find(it);
			if (jt==current->children_.end()) {
				trie_node* new_node=create_node();
				current->children_.insert({it,new_node});
				current=new_node;
			} else current=jt->second;
		}
		bool inserted=false;
		if (!current->value_) {
			current->value_=new mapped_type(std::forward<_Args>(args)...);
			size_++;
			inserted=true;
		}
		return {iterator(),inserted};
	}
	size_type erase(const key_type& key) {
		std::vector<std::pair<trie_node*,_CharT>> path;
		trie_node* current=root_;
		for (_CharT it:key) {
			auto jt=current->children_.find(it);
			if (jt==current->children_.end()) return 0;
			path.emplace_back(current,it);
			current=jt->second;
		}
		if (!current->value_) return 0;
		delete current->value_;
		current->value_=nullptr;
		size_--;
		for (auto it=path.rbegin();it!=path.rend() && current->children_.empty() && !current->value_;it++) {
			it->first->children_.erase(it->second);
			destroy_node(current);
			current=it->first;
		}
		return 1;
	}
	iterator erase(const_iterator pos) {
		auto key=pos->first;
		pos++;
		erase(key);
		return iterator(const_cast<trie_node*>(root_),false);
	}
	size_type count(const key_type& key) const {
		trie_node* node=find_node(key);
		return (node && node->value_)?1:0;
	}
	iterator find(const key_type& key) {
		trie_node* node=find_node(key);
		if (!node || !node->value_) return end();
		return iterator(node,key,root_);
	}
	const_iterator find(const key_type& key) const {
		trie_node* node=find_node(key);
		if (!node || !node->value_) return end();
		return const_iterator(node,key,root_);
	}
	bool contains(const key_type& key) const {
		return count(key)>0;
	}
	iterator lower_bound(const key_type& key) {
		auto it=begin();
		while (it!=end() && it->first<key) it++;
		return it;
	}
	const_iterator lower_bound(const key_type& key) const {
		auto it=begin();
		while (it!=end() && it->first<key) it++;
		return it;
	}
	iterator prefix_find(const key_type& prefix) {
		trie_node* current=root_;
		for (_CharT it:prefix) {
			auto jt=current->children_.find(it);
			if (jt==current->children_.end()) return end();
			current=jt->second;
		}
		if (current->value_) return iterator(current,prefix,root_);
		iterator it(current,false);
		if (it==end()) return end();
		return it;
	}
	const_iterator prefix_find(const key_type& prefix) const {
		trie_node* current=root_;
		for (_CharT it:prefix) {
			auto jt=current->children_.find(it);
			if (jt==current->children_.end()) return end();
			current=jt->second;
		}
		if (current->value_) return iterator(current,prefix,root_);
		iterator it(current,false);
		if (it == end()) return end();
		return it;
	}
	std::vector<value_type> prefix_collect(const key_type& prefix) const {
		std::vector<value_type> results;
		trie_node* current=root_;
		for (_CharT it:prefix) {
			auto jt=current->children_.find(it);
			if (jt==current->children_.end()) return results;
			current=jt->second;
		}
		std::function<void(trie_node*,key_type)> dfs=[&](trie_node* node,key_type path) {
			if (!node) return;
			if (node->value_) results.emplace_back(path,*(node->value_));
			for (auto& it:node->children_) dfs(it.second,path+it.first);
		};
		dfs(current,prefix);
		return results;
	}
	bool prefix_contains(const key_type& prefix,const key_type& key_suffix) const {
		trie_node* current=root_;
		for (_CharT it:prefix) {
			auto jt=current->children_.find(it);
			if (jt==current->children_.end()) return false;
			current=jt->second;
		}
		for (_CharT it:key_suffix) {
			auto jt=current->children_.find(it);
			if (jt==current->children_.end()) return false;
			current=jt->second;
		}
		return current && current->value_;
	}
	size_type public_prefix_length(const key_type& query) const {
		trie_node* current=root_;
		std::size_t count=0;
		for (_CharT it:query) {
			auto jt=current->children_.find(it);
			if (jt==current->children_.end()) break;
			current=jt->second;
			count++;
		}
		return count;
	}
	void swap(trie& other) noexcept {
		std::swap(root_,other.root_);
		std::swap(alloc_,other.alloc_);
		std::swap(comp_,other.comp_);
		std::swap(size_,other.size_);
	}
};

template <typename _CharT,typename _Tp,template <typename...> class _Container,typename _Compare,typename _Allocator>
void swap(trie<_CharT,_Tp,_Container,_Compare,_Allocator>& lhs,trie<_CharT,_Tp,_Container,_Compare,_Allocator>& rhs) noexcept {
	lhs.swap(rhs);
}

template <typename _Str>
std::size_t public_prefix_length(_Str lhs,_Str rhs) {
	std::size_t count=0;
	std::size_t length=std::min(lhs.size(),rhs.size());
	for (;count<length;count++) {
		if (lhs[i]!=rhs[i]) break;
		count++;
	}
	return count;
}

}

}

#endif