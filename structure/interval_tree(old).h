//Last Modified At 2025/10/15
//@Version 1.0.0.0
#ifndef _STDEX_STRUCTURE_INTERVAL_TREE_H_
#define _STDEX_STRUCTURE_INTERVAL_TREE_H_ 1

#include <algorithm>
#include <cstddef>
#include <functional>
#include <iterator>
#include <memory>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace stdex {

namespace structure {

template <typename _Key,typename _Tp,typename _Compare=std::less<_Key>,typename _Allocator=std::allocator<std::pair<const std::pair<_Key,_Key>,_Tp>>>
class interval_tree {
public:
	using key_type=_Key;
	using mapped_type=_Tp;
	using value_type=std::pair<const std::pair<_Key, _Key>, _Tp>;
	using key_compare=_Compare;
	using allocator_type=_Allocator;
	using size_type=std::size_t;
	using difference_type=std::ptrdiff_t;
	using reference=value_type&;
	using const_reference=const value_type&;

	struct interval {
		_Key low_;
		_Key high_;
		_Tp value_;
		interval(const _Key& low,const _Key& high,const _Tp& value) : low_(low) , high_(high) , value_(value) { }
		interval(_Key&& low,_Key&& high,_Tp&& value) : low_(std::move(low)) , high_(std::move(high)) , value_(std::move(value)) { }
	};

private:
	struct interval_node {
		interval data_;
		_Key max_end_;
		interval_node* left_=nullptr;
		interval_node* right_=nullptr;
		int height_=1;
		template <typename... _Args>
		interval_node(_Args&&... args) : data(std::forward<_Args>(args)...) , max_end(data.high_) {}
	};
	class iterator {
		std::vector<interval_node*> stack_;
		value_type current_;
		void push_left(interval_node* node) {
			while (node) {
				stack_.push_back(node);
				node=node->left;
			}
		}
	public:
		iterator()=default;
		explicit iterator(interval_node* root) {
			push_left(root);
			if (!stack_.empty()) {
				auto* node=stack_.back();
				current_.first={node->data_.low_,node->data_.high_};
				current_.second=node->data_.value_;
			}
		}
		value_type& operator *() { return current_; }
		value_type* operator ->() { return &current_; }
		iterator& operator ++() {
			if (stack_.empty()) return *this;
			auto* node=stack_.back();
			stack_.pop_back();
			push_left(node->right_);
			if (!stack_.empty()) {
				node=stack_.back();
				current_.first={node->data_.low_,node->data_.high_};
				current_.second=node->data_.value_;
			}
			return *this;
		}
		iterator operator ++(int) {
			iterator temp=*this;
			++(*this);
			return temp;
		}
		bool operator ==(const iterator& other) const {
			return stack_==other.stack_;
		}
		bool operator !=(const iterator& other) const {
			return !(*this==other);
		}
	};
	using const_iterator=iterator;
	interval_node* root_=nullptr;
	allocator_type alloc_;
	key_compare comp_;
	size_type size_=0;
	int height(interval_node* node) const {
		return node?node->height_:0;
	}
	_Key max_end(interval_node* node) const {
		return node?node->max_end_:_Key();
	}
	void update_node(interval_node* node) {
		if (node) {
			node->height_=1+std::max(height(node->left_),height(node->right_));
			node->max_end_=std::max({node->data_.high_,max_end(node->left_),max_end(node->right_)},comp_);
		}
	}
	interval_node* rotate_right(interval_node* y) {
		interval_node* x=y->left_;
		interval_node* T2=x->right_;
		x->right_=y;
		y->left_=T2;
		update_node(y);
		update_node(x);
		return x;
	}
	
	interval_node* rotate_left(interval_node* x) {
		interval_node* y=x->right_;
		interval_node* T2=y->left_;
		y->left_=x;
		x->right_=T2;
		update_node(x);
		update_node(y);
		return y;
	}
	int get_balance(interval_node* node) const {
		return node?height(node->left_)-height(node->right_):0;
	}
	interval_node* insert_node(interval_node* node,const interval& interv) {
		if (!node) {
			size_++;
			return new interval_node(interv);
		}
		if (comp_(interv.low_,node->data_.low_)) node->left_=insert_node(node->left_,interv);
		else node->right_=insert_node(node->right_,interv);
		update_node(node);
		int balance=get_balance(node);
		if (balance>1 && comp_(interv.low_,node->left_->data_.low_)) return rotate_right(node);
		if (balance<-1 && !comp_(interv.low_,node->right_->data_.low_)) return rotate_left(node);
		if (balance>1 && !comp_(interv.low_,node->left_->data_.low_)) {
			node->left_=rotate_left(node->left_);
			return rotate_right(node);
		}
		if (balance<-1 && comp_(interv.low_,node->right_->data_.low_)) {
			node->right_=rotate_right(node->right_);
			return rotate_left(node);
		}
		return node;
	}
	interval_node* min_value_node(interval_node* node) const {
		interval_node* current=node;
		while (current && current->left_) current=current->left_;
		return current;
	}
	interval_node* delete_node(interval_node* node,const _Key& low,const _Key& high) {
		if (!node) return node;
		if (comp_(low_,node->data_.low_)) node->left_=delete_node(node->left_,low,high);
		else if (comp_(node->data_.low_,low)) node->right_=delete_node(node->right_,low,high);
		else if (node->data_.high_==high_) {
			if (!node->left_ || !node->right_) {
				interval_node* temp=node->left_?node->left_:node->right_;
				if (!temp) {
					temp=node;
					node=nullptr;
				} else *node=*temp;
				delete temp;
				size_--;
			} else {
				interval_node* temp=min_value_node(node->right_);
				node->data_=temp->data_;
				node->right_=delete_node(node->right_,temp->data_.low_,temp->data_.high_);
			}
		} else node->right=delete_node(node->right_,low,high);
		if (!node) return node;
		update_node(node);
		int balance=get_balance(node);
		if (balance>1 && get_balance(node->left_)>=0) return rotate_right(node);
		if (balance>1 && get_balance(node->left_)<0) {
			node->left_=rotate_left(node->left_);
			return rotate_right(node);
		}
		if (balance<-1 && get_balance(node->right_)<=0) return rotate_left(node);
		if (balance<-1 && get_balance(node->right)>0) {
			node->right_=rotate_right(node->right_);
			return rotate_left(node);
		}
		return node;
	}
	bool overlaps(const _Key& low1,const _Key& high1,const _Key& low2,const _Key& high2) const {
		return !(comp_(high1,low2) || comp_(high2,low1));
	}
	void search_overlapping(interval_node* node,const _Key& low,const _Key& high,std::vector<value_type>& result) const {
		if (!node) return;
		if (overlaps(low,high,node->data_.low_,node->data_.high_)) result.emplace_back(std::make_pair(node->data_.low_,node->data_.high_),node->data_.value_);
		if (node->left_ && !comp_(node->left_->max_end_,low)) search_overlapping(node->left_,low,high,result);
		if (node->right_ && !comp_(high_,node->data_.low_)) search_overlapping(node->right_,low,high,result);
	}
	void search_contained(interval_node* node,const _Key& low,const _Key& high,std::vector<value_type>& result) const {
		if (!node) return;
		if (!comp_(low,node->data_.low_) && !comp_(node->data_.high_,high)) result.emplace_back(std::make_pair(node->data_.low_,node->data_.high_),node->data_.value_);
		if (node->left_ && !comp_(node->left_->max_end,low)) search_contained(node->left,low,high,result);
		if (node->right_ && !comp_(high,node->data.low_)) search_contained(node->right,low,high,result);
	}
	void destroy_tree(interval_node* node) {
		if (node) {
			destroy_tree(node->left_);
			destroy_tree(node->right_);
			delete node;
		}
	}
	interval_node* copy_tree(interval_node* other) const {
		if (!other) return nullptr;
		interval_node* node=new interval_node(other->data_);
		node->max_end_=other->max_end_;
		node->height_=other->height_;
		node->left_=copy_tree(other->left_);
		node->right_=copy_tree(other->right_);
		return node;
	}

public:
	interval_tree()=default;
	explicit interval_tree(const key_compare& comp,const allocator_type& alloc=allocator_type()) : alloc_(alloc), comp_(comp) {}
	explicit interval_tree(const allocator_type& alloc) : alloc_(alloc) {}
	template <typename _InputIt>
	interval_tree(_InputIt first,_InputIt last,const key_compare& comp=key_compare(),const allocator_type& alloc=allocator_type()) : alloc_(alloc) , comp_(comp) {
		insert(first,last);
	}
	interval_tree(std::initializer_list<value_type> init_list,const key_compare& comp=key_compare(),const allocator_type& alloc=allocator_type()) : alloc_(alloc) , comp_(comp) {
		insert(init_list.begin(),init_list.end());
	}
	interval_tree(const interval_tree& other) : alloc_(other.alloc_) , comp_(other.comp_) , size_(other.size_) {
		root_=copy_tree(other.root_);
	}
	interval_tree(interval_tree&& other) noexcept : root_(other.root_) , alloc_(std::move(other.alloc_)) , comp_(std::move(other.comp_)) , size_(other.size_) {
		other.root_=nullptr;
		other.size_=0;
	}
	~interval_tree() {
		destroy_tree(root_);
	}

	interval_tree& operator =(const interval_tree& other) {
		if (this!=&other) {
			destroy_tree(root_);
			root_=copy_tree(other.root_);
			alloc_=other.alloc_;
			comp_=other.comp_;
			size_=other.size_;
		}
		return *this;
	}
	interval_tree& operator =(interval_tree&& other) noexcept {
		if (this!=&other) {
			destroy_tree(root_);
			root_=other.root_;
			alloc_=std::move(other.alloc_);
			comp_=std::move(other.comp_);
			size_=other.size_;
			other.root_=nullptr;
			other.size_=0;
		}
		return *this;
	}
	interval_tree& operator =(std::initializer_list<value_type> init_list) {
		clear();
		insert(init_list.begin(),init_list.end());
		return *this;
	}
	mapped_type& at(const std::pair<_Key,_Key>& interval) {
		interval_node* current=root_;
		while (current) {
			if (comp_(interval.first,current->data_.low_)) current=current->left_;
			else if (comp_(current->data_.low_,interval.first)) current=current->right_;
			else if (interval.second==current->data_.high_) return current->data_.value_;
			else current=current->right_;
		}
		throw std::out_of_range("interval_tree::at: interval not found");
	}
	const mapped_type& at(const std::pair<_Key,_Key>& interval) const {
		interval_node* current=root_;
		while (current) {
			if (comp_(interval.first,current->data_.low_)) current=current->left_;
			else if (comp_(current->data_.low_,interval.first)) current=current->right_;
			else if (interval.second==current->data_.high_) return current->data_.value_;
			else current=current->right_;
		}
		throw std::out_of_range("interval_tree::at: interval not found");
	}
	mapped_type& operator [](const std::pair<_Key,_Key>& interval) {
		auto result=insert({interval,mapped_type()});
		return result.first->second;
	}

	iterator begin() noexcept { return iterator(root_); }
	const_iterator begin() const noexcept { return const_iterator(root_); }
	const_iterator cbegin() const noexcept { return const_iterator(root_); }
	
	iterator end() noexcept { return iterator(); }
	const_iterator end() const noexcept { return const_iterator(); }
	const_iterator cend() const noexcept { return const_iterator(); }

	bool empty() const noexcept { return size_==0; }
	size_type size() const noexcept { return size_; }
	size_type max_size() const noexcept { return std::numeric_limits<size_type>::max(); }

	void clear() noexcept {
		destroy_tree(root_);
		root_=nullptr;
		size_=0;
	}
	std::pair<iterator, bool> insert(const value_type& value) {
		const auto& interval=value.first;
		auto* current=root_;
		while (current) {
			if (comp_(interval.first,current->data_.low_)) current=current->left_;
			else if (comp_(current->data_.low_,interval.first)) current=current->right_;
			else if (interval.second==current->data_.high_) return {iterator(), false};
			else current=current->right_;
		}
		root_=insert_node(root_,interval(interval.first,interval.second,value.second));
		return {iterator(),true};
	}
	template <typename _InputIt>
	void insert(_InputIt first,_InputIt last) {
		for (auto it=first;it!=last;it++) insert(*it);
	}
	void insert(std::initializer_list<value_type> init_list) {
		insert(init_list.begin(),init_list.end());
	}
	template <typename... _Args>
	std::pair<iterator,bool> emplace(const std::pair<_Key,_Key>& interval,_Args&&... args) {
		auto result=insert({interval,mapped_type(std::forward<_Args>(args)...)});
		return result;
	}
	size_type erase(const std::pair<_Key,_Key>& interval) {
		size_type old_size=size_;
		root_=delete_node(root_,interval.first,interval.second);
		return old_size-size_;
	}
	iterator erase(const_iterator pos) {
		auto interval=pos++->first;
		erase(interval);
		return iterator(root_);
	}
	size_type count(const std::pair<_Key,_Key>& interval) const {
		interval_node* current=root_;
		while (current) {
			if (comp_(interval.first,current->data_.low_)) current=current->left_;
			else if (comp_(current->data_.low_,interval.first)) current=current->right_;
			else if (interval.second==current->data_.high_) return 1;
			else current=current->right_;
		}
		return 0;
	}
	iterator find(const std::pair<_Key,_Key>& interval) {
		interval_node* current=root_;
		while (current) {
			if (comp_(interval.first,current->data_.low_)) current=current->left_;
			else if (comp_(current->data_.low_,interval.first)) current=current->right_;
			else if (interval.second==current->data_.high_) return iterator(current);
			else current=current->right_;
		}
		return end();
	}
	const_iterator find(const std::pair<_Key,_Key>& interval) const {
		interval_node* current=root_;
		while (current) {
			if (comp_(interval.first,current->data_.low_)) current=current->left_;
			else if (comp_(current->data_.low_,interval.first)) current=current->right_;
			else if (interval.second==current->data_.high_) return const_iterator(current);
			else current=current->right_;
		}
		return end();
	}
	bool contains(const std::pair<_Key,_Key>& interval) const {
		return count(interval)>0;
	}
	std::vector<value_type> find_overlapping(const _Key& low,const _Key& high) const {
		std::vector<value_type> result;
		search_overlapping(root_,low,high,result);
		return result;
	}
	std::vector<value_type> find_contained(const _Key& low,const _Key& high) const {
		std::vector<value_type> result;
		search_contained(root_,low,high,result);
		return result;
	}
	bool has_overlapping(const _Key& low,const _Key& high) const {
		return !find_overlapping(low,high).empty();
	}
	bool has_contained(const _Key& low,const _Key& high) const {
		return !find_contained(low,high).empty();
	}
	void swap(interval_tree& other) noexcept {
		std::swap(root_,other.root_);
		std::swap(alloc_,other.alloc_);
		std::swap(comp_,other.comp_);
		std::swap(size_,other.size_);
	}
};

template <typename _Key,typename _Tp,typename _Compare,typename _Allocator>
void swap(interval_tree<_Key,_Tp,_Compare,_Allocator>& lhs,interval_tree<_Key,_Tp,_Compare,_Allocator>& rhs) noexcept {
	lhs.swap(rhs);
}

}

}

#endif