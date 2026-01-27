//Last Modified At 2026/01/27
//@Version 1.1.0.0
#ifndef _STDEX_STRUCTURE_BINARY_TREE_H_
#define _STDEX_STRUCTURE_BINARY_TREE_H_ 1

#include <algorithm>
#include <cstddef>
#include <functional>
#include <iterator>
#include <memory>
#include <queue>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

namespace stdex {

namespace structure {

template <typename _Tp,typename _Key=_Tp,typename _Compare=std::less<_Key>,typename _Allocator=std::allocator<std::pair<const _Key,_Tp>>>
class binary_tree {
public:
	using key_type=_Key;
	using mapped_type=_Tp;
	using value_type=std::pair<_Key,_Tp>;
	using key_compare=_Compare;
	using allocator_type=_Allocator;
	using size_type=std::size_t;
	using difference_type=std::ptrdiff_t;
	using reference=value_type&;
	using const_reference=const value_type&;

protected:
	virtual bool is_exact_binary_tree() const noexcept {
		return typeid(*this)==typeid(std::remove_cv_t<binary_tree<_Tp,_Key,_Compare,_Allocator>>);
		//return dynamic_cast<const binary_tree<_Tp,_Key,_Compare,_Allocator>*>(this) != nullptr && typeid(*this)==typeid(binary_tree<_Tp,_Key,_Compare,_Allocator>);
		//return typeid(std::remove_cv_t<std::remove_reference_t<decltype(*this)>>)==typeid(binary_tree<_Tp,_Key,_Compare,_Allocator>);
		//return std::is_same_v<std::remove_cv_t<std::remove_reference_t<decltype(*this)>>,binary_tree<_Tp,_Key,_Compare,_Allocator>>;
		//return std::is_same_v<std::decay_t<decltype(*this)>,binary_tree>;
	}
	struct binary_tree_node {
		value_type data_;
		binary_tree_node* left_=nullptr;
		binary_tree_node* right_=nullptr;
		binary_tree_node* parent_=nullptr;
		template <typename... _Args>
		binary_tree_node(_Args&&... args) : data_(std::forward<_Args>(args)...) { }
		virtual ~binary_tree_node()=default;
		virtual const key_type& key() const noexcept { return data_.first; }
		_Tp& value() noexcept { return data_.second; }
		const _Tp& value() const noexcept { return data_.second; }
		virtual void sync_node(binary_tree_node* src,binary_tree_node* dst,binary_tree_node* parent=nullptr) {
			if (!src || !dst) return;
			dst->data_=src->data_;
			if (src->left_) dst->left_=src->left_->clone(dst);
			else dst->left_=src->left_;
			if (src->right_) dst->right_=src->right_->clone(dst);
			else dst->right_=src->right_;
			dst->parent_=parent;
		}
		virtual binary_tree_node* clone(binary_tree_node* parent=nullptr) {
			binary_tree_node* result=new binary_tree_node;
			sync_node(this,result,parent);
			return result;
		}
		virtual binary_tree_node* successor() noexcept {
			return binary_tree::successor(this);
		}
		virtual binary_tree_node* predecessor() noexcept {
			return binary_tree::predecessor(this);
		}
	};
	friend struct binary_tree_node;
	binary_tree_node* root_=nullptr;
	allocator_type alloc_;
	key_compare comp_;
	size_type size_=0;
	virtual binary_tree_node* create_node(const value_type& value) {
		return new binary_tree_node(value);
	}
	template <typename... _Args>
	binary_tree_node* create_node(_Args&&... args) {
		return new binary_tree_node(std::forward<_Args>(args)...);
	}
	virtual void destroy_node(binary_tree_node* node) noexcept {
		if (node) delete node;
	}
	virtual binary_tree_node* copy_tree(binary_tree_node* node,binary_tree_node* parent=nullptr) {
		if (!node) return nullptr;
		return node->clone(parent);
	}
	virtual void destroy_tree(binary_tree_node* node) noexcept {
		if (node) {
			destroy_tree(node->left_);
			destroy_tree(node->right_);
			destroy_node(node);
		}
	}
	static binary_tree_node* min_node(binary_tree_node* node) noexcept {
		while (node && node->left_) node=node->left_;
		return node;
	}
	static binary_tree_node* max_node(binary_tree_node* node) noexcept {
		while (node && node->right_) node=node->right_;
		return node;
	}
	static binary_tree_node* successor(binary_tree_node* node) noexcept {
		if (!node) return nullptr;
		if (node->right_) return min_node(node->right_);
		binary_tree_node* parent=node->parent_;
		while (parent && node==parent->right_) {
			node=parent;
			parent=parent->parent_;
		}
		return parent;
	}
	static binary_tree_node* predecessor(binary_tree_node* node) noexcept {
		if (!node) return nullptr;
		if (node->left_) return max_node(node->left_);
		binary_tree_node* parent=node->parent_;
		while (parent && node==parent->left_) {
			node=parent;
			parent=parent->parent_;
		}
		return parent;
	}
	size_type count_nodes(binary_tree_node* node) const noexcept {
		if (!node) return 0;
		return 1+count_nodes(node->left_)+count_nodes(node->right_);
	}
	int height(binary_tree_node* node) const noexcept {
		if (!node) return 0;
		return 1+std::max(height(node->left_),height(node->right_));
	}
	bool is_balanced(binary_tree_node* node) const noexcept {
		if (!node) return true;
		int left_height=height(node->left_);
		int right_height=height(node->right_);
		return std::abs(left_height-right_height)<=1 && is_balanced(node->left_) && is_balanced(node->right_);
	}
	void in_order_traversal(binary_tree_node* node,std::vector<value_type>& result) const {
		if (!node) return;
		in_order_traversal(node->left_,result);
		result.push_back(node->data_);
		in_order_traversal(node->right_,result);
	}
	void pre_order_traversal(binary_tree_node* node,std::vector<value_type>& result) const {
		if (!node) return;
		result.push_back(node->data_);
		pre_order_traversal(node->left_,result);
		pre_order_traversal(node->right_,result);
	}
	void post_order_traversal(binary_tree_node* node,std::vector<value_type>& result) const {
		if (!node) return;
		post_order_traversal(node->left_,result);
		post_order_traversal(node->right_,result);
		result.push_back(node->data_);
	}
	template <typename _Func>
	void traverse_pre_order_impl(binary_tree_node* node,_Func&& func) const {
		if (!node) return;
		if (!func(node->data_)) return;
		traverse_pre_order_impl(node->left_,std::forward<_Func>(func));
		traverse_pre_order_impl(node->right_,std::forward<_Func>(func));
	}
	template <typename _Func>
	void traverse_in_order_impl(binary_tree_node* node,_Func&& func) const {
		if (!node) return;
		traverse_in_order_impl(node->left_,std::forward<_Func>(func));
		if (!func(node->data_)) return;
		traverse_in_order_impl(node->right_,std::forward<_Func>(func));
	}
	template <typename _Func>
	void traverse_post_order_impl(binary_tree_node* node,_Func&& func) const {
		if (!node) return;
		traverse_post_order_impl(node->left_,std::forward<_Func>(func));
		traverse_post_order_impl(node->right_,std::forward<_Func>(func));
		func(node->data_);
	}
	bool is_full_impl(binary_tree_node* node) const {
		if (!node) return true;
		if (!node->left_ && !node->right_) return true;
		if (node->left_ && node->right_) {
			return is_full_impl(node->left_) && is_full_impl(node->right_);
		}
		return false;
	}
	binary_tree_node* erase_node(binary_tree_node* node) {
		if (!node) return nullptr;
		std::queue<binary_tree_node*> q;
		q.push(root_);
		binary_tree_node* last=nullptr;
		while (!q.empty()) {
			last=q.front();
			q.pop();
			if (last->left_) q.push(last->left_);
			if (last->right_) q.push(last->right_);
		}
		if (node!=last) std::swap(node->data_,last->data_);
		if (last->parent_) {
			if (last->parent_->left_==last) last->parent_->left_=nullptr;
			else last->parent_->right_=nullptr;
		} else root_ = nullptr;
		destroy_node(last);
		size_--;
		return node;
	}

public:
	class iterator {
		binary_tree_node* current_=nullptr;
	public:
		using iterator_category=std::bidirectional_iterator_tag;
		using value_type=binary_tree::value_type;
		using difference_type=binary_tree::difference_type;
		using pointer=value_type*;
		using reference=value_type&;
		iterator()=default;
		explicit iterator(binary_tree_node* node) : current_(node) {}
		reference operator *() const { return current_->data_; }
		pointer operator ->() const { return &current_->data_; }
		iterator& operator ++() {
			current_=current_->successor();
			return *this;
		}
		iterator operator ++(int) {
			iterator temp=*this;
			++(*this);
			return temp;
		}
		iterator& operator --() {
			current_=current_->predecessor();
			return *this;
		}
		iterator operator --(int) {
			iterator temp=*this;
			--(*this);
			return temp;
		}
		bool operator ==(const iterator& other) const { return current_==other.current_; }
		bool operator !=(const iterator& other) const { return current_!=other.current_; }
		binary_tree_node* node() const { return current_; }
	};
	using const_iterator=iterator;

public:
	binary_tree()=default;
	explicit binary_tree(const key_compare& comp,const allocator_type& alloc=allocator_type()) : alloc_(alloc) , comp_(comp) { }
	explicit binary_tree(const allocator_type& alloc) : alloc_(alloc) { }
	binary_tree(const binary_tree& other) : alloc_(other.alloc_) , comp_(other.comp_) , size_(other.size_) {
		root_=copy_tree(other.root_);
	}
	binary_tree(binary_tree&& other) noexcept : root_(other.root_) , alloc_(std::move(other.alloc_)) , comp_(std::move(other.comp_)) , size_(other.size_) {
		other.root_=nullptr;
		other.size_=0;
	}
	virtual ~binary_tree() {
		destroy_tree(root_);
	}

	binary_tree& operator =(const binary_tree& other) {
		if (this!=&other) {
			clear();
			alloc_=other.alloc_;
			comp_=other.comp_;
			root_=copy_tree(other.root_);
			size_=other.size_;
		}
		return *this;
	}
	binary_tree& operator =(binary_tree&& other) noexcept {
		if (this!=&other) {
			clear();
			root_=other.root_;
			alloc_=std::move(other.alloc_);
			comp_=std::move(other.comp_);
			size_=other.size_;
			other.root_=nullptr;
			other.size_=0;
		}
		return *this;
	}

	virtual iterator begin() noexcept { return iterator(min_node(root_)); }
	virtual const_iterator begin() const noexcept { return const_iterator(min_node(root_)); }
	virtual const_iterator cbegin() const noexcept { return const_iterator(min_node(root_)); }
	
	iterator end() noexcept { return iterator(nullptr); }
	const_iterator end() const noexcept { return const_iterator(nullptr); }
	const_iterator cend() const noexcept { return const_iterator(nullptr); }

	bool empty() const noexcept { return size_==0; }
	size_type size() const noexcept { return size_; }
	size_type max_size() const noexcept { return std::numeric_limits<size_type>::max(); }

	virtual void clear() noexcept {
		destroy_tree(root_);
		root_=nullptr;
		size_=0;
	}

	virtual std::pair<iterator,bool> insert(const value_type& value) {
		if (!root_) {
			root_=create_node(value);
			size_=1;
			return {iterator(root_),true};
		}
		std::queue<binary_tree_node*> q;
		q.push(root_);
		while (!q.empty()) {
			binary_tree_node* current=q.front();
			q.pop();
			if (!current->left_) {
				current->left_=create_node(value);
				current->left_->parent_=current;
				size_++;
				return {iterator(current->left_),true};
			} else q.push(current->left_);
			if (!current->right_) {
				current->right_=create_node(value);
				current->right_->parent_=current;
				size_++;
				return {iterator(current->right_),true};
			} else q.push(current->right_);
		}
		return {end(),false};
	}
	virtual std::pair<iterator,bool> insert_left(binary_tree_node* node,const value_type& value) {
		if (!is_exact_binary_tree()) throw std::runtime_error("Invalid use of insert_left in non-binary_tree!");
		else {
			if (node->left_) return {iterator(node->left_),false};
			node->left_=create_node(value);
			node->left_->parent_=node;
			size_++;
			return {iterator(node->left_),true};
		}
		return {end(),false};
	}
	virtual std::pair<iterator,bool> insert_right(binary_tree_node* node,const value_type& value) {
		if (!is_exact_binary_tree()) throw std::runtime_error("Invalid use of insert_right in non-binary_tree!");
		else {
			if (node->right_) return {iterator(node->right_),false};
			node->right_=create_node(value);
			node->right_->parent_=node;
			size_++;
			return {iterator(node->right_),true};
		}
		return {end(),false};
	}
	virtual iterator erase(const key_type& key) {
		for (auto it=begin();it!=end();it++) {
			if (!comp_(it->first,key) && !comp_(key,it->first)) {
				binary_tree_node* node=it.node();
				binary_tree_node* next=successor(node);
				erase_node(node);
				return iterator(next);
			}
		}
		return end();
	}
	virtual size_type erase_old(const key_type& key) {
		return erase(key)!=end()?1:0;
	}
	virtual size_type erase_left(binary_tree_node* node) {
		if (!is_exact_binary_tree()) throw std::runtime_error("Invalid use of erase_left in non-binary_tree!");
		else {
			if (node->left_) {
				erase_node(node->left_);
				return 1;
			}
		}
		return 0;
	}
	virtual size_type erase_right(binary_tree_node* node) {
		if (!is_exact_binary_tree()) throw std::runtime_error("Invalid use of erase_right in non-binary_tree!");
		else {
			if (node->right_) {
				erase_node(node->right_);
				return 1;
			}
		}
		return 0;
	}
	virtual iterator find(const key_type& key) {
		for (auto it=begin();it!=end();it++) {
			if (!comp_(it->first,key) && !comp_(key,it->first)) return it;
		}
    		return end();
	}
	virtual const_iterator find(const key_type& key) const {
		for (auto it=begin();it!=end();it++) {
			if (!comp_(it->first,key) && !comp_(key,it->first)) return it;
		}
    		return end();
	}
	size_type count(const key_type& key) const {
		return find(key)!=end()?1:0;
	}
	bool contains(const key_type& key) const {
		return count(key)>0;
	}
	iterator lower_bound(const key_type& key) {
		binary_tree_node* current=root_;
		binary_tree_node* result=nullptr;
		while (current) {
			if (!comp_(current->key(),key)) {
				result=current;
				current=current->left_;
			} else current=current->right_;
		}
		return iterator(result);
	}
	const_iterator lower_bound(const key_type& key) const {
		binary_tree_node* current=root_;
		binary_tree_node* result=nullptr;
		while (current) {
			if (!comp_(current->key(),key)) {
				result=current;
				current=current->left_;
			} else current=current->right_;
		}
		return const_iterator(result);
	}
	iterator upper_bound(const key_type& key) {
		binary_tree_node* current=root_;
		binary_tree_node* result=nullptr;
		while (current) {
			if (comp_(key,current->key())) {
				result=current;
				current=current->left_;
			} else current=current->right_;
		}
		return iterator(result);
	}
	const_iterator upper_bound(const key_type& key) const {
		binary_tree_node* current=root_;
		binary_tree_node* result=nullptr;
		while (current) {
			if (comp_(key,current->key())) {
				result=current;
				current=current->left_;
			} else current=current->right_;
		}
		return const_iterator(result);
	}
	std::vector<value_type> in_order() const {
		std::vector<value_type> result;
		in_order_traversal(root_,result);
		return result;
	}
	std::vector<value_type> pre_order() const {
		std::vector<value_type> result;
		pre_order_traversal(root_,result);
		return result;
	}
	std::vector<value_type> post_order() const {
		std::vector<value_type> result;
		post_order_traversal(root_,result);
		return result;
	}
	std::vector<value_type> level_order() const {
		std::vector<value_type> result;
		if (!root_) return result;
		std::queue<binary_tree_node*> q;
		q.push(root_);
		while (!q.empty()) {
			binary_tree_node* current=q.front();
			q.pop();
			result.push_back(current->data_);
			if (current->left_) q.push(current->left_);
			if (current->right_) q.push(current->right_);
		}
		return result;
	}
	template <typename _Func>
	void traverse_pre_order(_Func&& func) const {
		traverse_pre_order_impl(root_, std::forward<_Func>(func));
	}
	template <typename _Func>
	void traverse_in_order(_Func&& func) const {
		traverse_in_order_impl(root_, std::forward<_Func>(func));
	}
	template <typename _Func>
	void traverse_post_order(_Func&& func) const {
		traverse_post_order_impl(root_, std::forward<_Func>(func));
	}
	template <typename _Func>
	void traverse_level_order(_Func&& func) const {
		if (!root_) return;
		std::queue<binary_tree_node*> q;
		q.push(root_);
		while (!q.empty()) {
			binary_tree_node* current=q.front();
			q.pop();
			if (!func(current->data_)) return;
			if (current->left_) q.push(current->left_);
			if (current->right_) q.push(current->right_);
		}
	}
	bool has_left(binary_tree_node* node) {
		return node->left_;
	}
	bool has_right(binary_tree_node* node) {
		return node->right_;
	}
	bool is_leaf(binary_tree_node* node) {
		return !has_left(node) && !has_right(node);
	}
	int height() const noexcept {
		return height(root_);
	}
	bool is_balanced() const noexcept {
		return is_balanced(root_);
	}
	bool is_complete() const {
		if (!root_) return true;
		std::queue<binary_tree_node*> q;
		q.push(root_);
		bool found_null=false;
		while (!q.empty()) {
			binary_tree_node* current=q.front();
			q.pop();
			if (!current) found_null = true;
			else {
				if (found_null) return false;
				q.push(current->left_);
				q.push(current->right_);
			}
		}
		return true;
	}
	bool is_full() const {
		return is_full_impl(root_);
	}
	void swap(binary_tree& other) noexcept {
		std::swap(root_,other.root_);
		std::swap(alloc_,other.alloc_);
		std::swap(comp_,other.comp_);
		std::swap(size_,other.size_);
	}

protected:
	binary_tree_node* root() const noexcept { return root_; }
	void update_size(size_type new_size) noexcept { size_=new_size; }
	void set_root(binary_tree_node* new_root) noexcept { root_=new_root; }
};

template <typename _Tp,typename _Key,typename _Compare,typename _Allocator>
void swap(binary_tree<_Tp,_Key,_Compare,_Allocator>& lhs,binary_tree<_Tp,_Key,_Compare,_Allocator>& rhs) noexcept {
	lhs.swap(rhs);
}

}

}

#endif