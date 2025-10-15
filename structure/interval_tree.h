//Last Modified At 2025/10/15
//@Version 1.0.0.0
#ifndef _STDEX_STRUCTURE_INTERVAL_TREE_H_
#define _STDEX_STRUCTURE_INTERVAL_TREE_H_ 1

#include <utility>

#include "avl_tree.h"

namespace stdex {

namespace structure {

template <typename _Tp,typename _Key=_Tp,typename _Compare=std::less<std::pair<_Key,_Key>>,typename _Allocator=std::allocator<std::pair<const std::pair<_Key,_Key>,_Tp>>>
class interval_tree : public avl_tree<_Tp,std::pair<_Key,_Key>,_Compare,_Allocator> {
protected:
	using key_type=std::pair<_Key,_Key>;
	using base_type=avl_tree<_Tp,std::pair<_Key,_Key>,_Compare,_Allocator>;
	using typename base_type::binary_tree_node;
	using base_type::root_;
	using base_type::alloc_;
	using base_type::comp_;
	using base_type::size_;
	using base_type::create_node;
	using base_type::destroy_node;

	struct interval_node : public base_type::avl_node {
		_Key max_end_;
		template <typename... _Args>
		interval_node(_Args&&... args) : base_type::avl_node(std::forward<_Args>(args)...) {
			max_end_=this->data_.first.second;
		}
		const _Key& low() const noexcept { return this->data_.first.first; }
		const _Key& high() const noexcept { return this->data_.first.second; }
		const key_type& key() const noexcept override { return base_type::avl_node::data_.first; }
		_Tp& value() noexcept { return this->data_.second; }
		const _Tp& value() const noexcept { return this->data_.second; }
		virtual binary_tree_node* clone(binary_tree_node* parent=nullptr) override {
			interval_node* result=new interval_node;
			binary_tree_node::sync_node(this,result,nullptr);
			result->base_type::avl_node::height_=base_type::avl_node::height_;
			result->max_end_=max_end_;
			return result;
		}
	};

public:
	using key_base_type=_Key;
	using mapped_type=_Tp;
	using value_type=std::pair<key_type,_Tp>;
	using key_compare=_Compare;
	using allocator_type=_Allocator;
	using size_type=std::size_t;
	using difference_type=std::ptrdiff_t;
	using reference=value_type&;
	using const_reference=const value_type&;

	using base_type::begin;
	using base_type::end;
	using base_type::cbegin;
	using base_type::cend;

	interval_tree()=default;
	explicit interval_tree(const key_compare& comp,const allocator_type& alloc=allocator_type()) : base_type(comp,alloc) { }
	explicit interval_tree(const allocator_type& alloc) : base_type(alloc) { }
	template <typename _InputIt>
	interval_tree(_InputIt first,_InputIt last,const key_compare& comp=key_compare(),const allocator_type& alloc=allocator_type()) : base_type(comp,alloc) {
		insert(first,last);
	}
	interval_tree(std::initializer_list<value_type> init_list,const key_compare& comp=key_compare(),const allocator_type& alloc=allocator_type()) : base_type(comp,alloc) {
		insert(init_list.begin(),init_list.end());
	}
	virtual ~interval_tree()=default;

	interval_tree(const interval_tree& other) : base_type(other) { }
	interval_tree(interval_tree&& other) noexcept : base_type(std::move(other)) { }
	interval_tree& operator =(const interval_tree& other) {
		base_type::operator =(other);
		return *this;
	}
	interval_tree& operator =(interval_tree&& other) noexcept {
		base_type::operator =(std::move(other));
		return *this;
	}

	mapped_type& at(const key_type& interval) {
		interval_node* node=find_interval_impl(interval);
		if (!node) throw std::out_of_range("interval_tree::at: interval not found");
		return node->value();
	}
	const mapped_type& at(const key_type& interval) const {
		interval_node* node=find_interval_impl(interval);
		if (!node) throw std::out_of_range("interval_tree::at: interval not found");
		return node->value();
	}

	mapped_type& operator [](const key_type& interval) {
		auto result=insert({interval,mapped_type()});
		return result.first->second;
	}

	virtual std::pair<typename base_type::iterator,bool> insert(const value_type& value) override {
		auto result=base_type::insert(value);
		if (result.second) update_max_end(static_cast<interval_node*>(result.first.node()));
		return result;
	}
	std::vector<value_type> find_overlapping(const key_base_type& low,const key_base_type& high) const {
		std::vector<value_type> result;
		search_overlapping(static_cast<interval_node*>(root_),low,high,result);
		return result;
	}
	std::vector<value_type> find_contained(const key_base_type& low,const key_base_type& high) const {
		std::vector<value_type> result;
		search_contained(static_cast<interval_node*>(root_),low,high,result);
		return result;
	}
	bool has_overlapping(const key_base_type& low,const key_base_type& high) const {
		return !find_overlapping(low,high).empty();
	}
	bool has_contained(const key_base_type& low,const key_base_type& high) const {
		return !find_contained(low,high).empty();
	}

protected:
	binary_tree_node* create_node(const value_type& value) override {
		return new interval_node(value);
	}
	template <typename... _Args>
	binary_tree_node* create_node(_Args&&... args) {
		return new interval_node(std::forward<_Args>(args)...);
	}
	key_base_type max_end(interval_node* node) const {
		return node?node->max_end_:key_base_type();
	}
	void update_node(interval_node* node) {
		if (node) {
			base_type::update_height(node);
			node->max_end_=std::max({key_type(node->high(),node->high()),key_type(max_end(static_cast<interval_node*>(node->left_)),max_end(static_cast<interval_node*>(node->left_))),key_type(max_end(static_cast<interval_node*>(node->right_)),max_end(static_cast<interval_node*>(node->right_)))},comp_).first;
		}
	}
	interval_node* find_interval_impl(const key_type& interval) const {
		interval_node* current=static_cast<interval_node*>(root_);
		while (current) {
			key_type lhs=interval;
			key_type rhs={current->low(),current->high()};
			if (comp_(lhs,rhs)) current=static_cast<interval_node*>(current->left_);
			else if (comp_(rhs,lhs)) current=static_cast<interval_node*>(current->right_);
			else if (interval.second==current->high()) return current;
			else current=static_cast<interval_node*>(current->right_);
		}
		return nullptr;
	}
	void update_max_end(interval_node* node) {
		while (node) {
			update_node(node);
			node=static_cast<interval_node*>(node->parent_);
		}
	}
	bool overlaps(const key_base_type& low1,const key_base_type& high1,const key_base_type& low2,const key_base_type& high2) const {
		return !(comp_(key_type(high1,high1),key_type(low2,low2)) || comp_(key_type(high2,high2),key_type(low1,low1)));
	}
	void search_overlapping(interval_node* node,const key_base_type& low,const key_base_type& high,std::vector<value_type>& result) const {
		if (!node) return;
		if (overlaps(low,high,node->low(),node->high())) result.emplace_back(std::make_pair(node->low(),node->high()),node->value());
		if (node->left_) {
			auto left_max=static_cast<interval_node*>(node->left_)->max_end_;
			if (!comp_(key_type(left_max,left_max),key_type(low,low))) search_overlapping(static_cast<interval_node*>(node->left_),low,high,result);
		}
		if (node->right_) {
			if (!comp_(key_type(high,high),key_type(node->low(),node->high()))) search_overlapping(static_cast<interval_node*>(node->right_),low,high,result);
		}
	}
	void search_contained(interval_node* node,const key_base_type& low,const key_base_type& high,std::vector<value_type>& result) const {
		if (!node) return;
		if (!comp_(key_type(node->low(),node->low()),key_type(low,low)) && !comp_(key_type(high,high),key_type(node->high(),node->high()))) result.emplace_back(std::make_pair(node->low(),node->high()),node->value());
		if (node->left_) {
			auto left_max=static_cast<interval_node*>(node->left_)->max_end_;
			if (!comp_(key_type(left_max,left_max),key_type(low,low))) search_contained(static_cast<interval_node*>(node->left_),low,high,result);
		}
		if (node->right_) {
			if (!comp_(key_type(high,high),key_type(node->low(),node->high()))) search_contained(static_cast<interval_node*>(node->right_),low,high,result);
		}
	}
};

template <typename _Tp,typename _Key,typename _Compare,typename _Allocator>
void swap(interval_tree<_Tp,_Key,_Compare,_Allocator>& lhs,interval_tree<_Tp,_Key,_Compare,_Allocator>& rhs) noexcept {
	lhs.swap(rhs);
}

}

}

#endif