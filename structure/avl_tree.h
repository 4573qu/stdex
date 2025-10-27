//Last Modified At 2025/10/15
//@Version 1.0.0.0
#ifndef _STDEX_STRUCTURE_AVL_TREE_H_
#define _STDEX_STRUCTURE_AVL_TREE_H_ 1

#include "bst_tree.h"//At Least 1.0

namespace stdex {

namespace structure {

template <typename _Tp,typename _Key=_Tp,typename _Compare=std::less<_Key>,typename _Allocator=std::allocator<std::pair<const _Key,_Tp>>>
class avl_tree : public bst_tree<_Tp,_Key,_Compare,_Allocator> {
protected:
	using base_type=bst_tree<_Tp,_Key,_Compare,_Allocator>;
	using typename base_type::binary_tree_node;
	using base_type::root_;
	using base_type::alloc_;
	using base_type::comp_;
	using base_type::size_;
	using base_type::create_node;
	using base_type::destroy_node;
	using base_type::transplant;

	struct avl_node : public binary_tree_node {
		int height_=1;
		template <typename... _Args>
		avl_node(_Args&&... args) : binary_tree_node(std::forward<_Args>(args)...) { }
		int balance_factor() const noexcept {
			int left_height=this->left_?static_cast<avl_node*>(this->left_)->height_:0;
			int right_height=this->right_?static_cast<avl_node*>(this->right_)->height_:0;
			return left_height-right_height;
		}
		void update_height() noexcept {
			int left_height=this->left_?static_cast<avl_node*>(this->left_)->height_:0;
			int right_height=this->right_?static_cast<avl_node*>(this->right_)->height_:0;
			height_=1+std::max(left_height,right_height);
		}
		virtual binary_tree_node* clone(binary_tree_node* parent=nullptr) override {
			avl_node* result=new avl_node;
			binary_tree_node::sync_node(this,result,nullptr);
			result->height_=height_;
			return result;
		}
	};

public:
	using typename base_type::key_type;
	using typename base_type::mapped_type;
	using typename base_type::value_type;
	using typename base_type::key_compare;
	using typename base_type::allocator_type;
	using typename base_type::size_type;
	using typename base_type::iterator;
	using typename base_type::const_iterator;

	avl_tree()=default;
	explicit avl_tree(const key_compare& comp,const allocator_type& alloc=allocator_type()) : base_type(comp,alloc) { }
	explicit avl_tree(const allocator_type& alloc) : base_type(alloc) { }
	virtual ~avl_tree()=default;

	avl_tree(const avl_tree& other) : base_type(other) { }
	avl_tree(avl_tree&& other) noexcept : base_type(std::move(other)) { }

	avl_tree& operator =(const avl_tree& other) {
		base_type::operator =(other);
		return *this;
	}
	avl_tree& operator =(avl_tree&& other) noexcept {
		base_type::operator =(std::move(other));
		return *this;
	}
	std::pair<iterator,bool> insert(const value_type& value) override {
		auto result=base_type::insert_impl(value);
		if (result.second) {
			avl_node* node=static_cast<avl_node*>(result.first.node());
			balance(node);
		}
		return result;
	}
	size_type erase(const key_type& key) override {
		binary_tree_node* node=this->find_impl(key);
		if (!node) return 0;
		avl_node* start_balance=static_cast<avl_node*>(node->parent_);
		base_type::erase_node(node);
		if (start_balance) balance(static_cast<avl_node*>(start_balance));
		return 1;
	}

protected:
	virtual binary_tree_node* create_node(const value_type& value) override {
		return new avl_node(value);
	}
	template <typename... _Args>
	binary_tree_node* create_node(_Args&&... args) {
		return new avl_node(std::forward<_Args>(args)...);
	}
	int height(binary_tree_node* node) const noexcept {
		return node?static_cast<avl_node*>(node)->height_:0;
	}
	void update_height(avl_node* node) noexcept {
		if (node) node->update_height();
	}
	avl_node* rotate_left(avl_node* x) {
		avl_node* y=static_cast<avl_node*>(x->right_);
		avl_node* T2=static_cast<avl_node*>(y->left_);
		y->left_=x;
		x->right_=T2;
		if (T2) T2->parent_=x;
		y->parent_=x->parent_;
		x->parent_=y;
		update_height(x);
		update_height(y);
		return y;
	}
	avl_node* rotate_right(avl_node* y) {
		avl_node* x=static_cast<avl_node*>(y->left_);
		avl_node* T2=static_cast<avl_node*>(x->right_);	
		x->right_=y;
		y->left_=T2;
		if (T2) T2->parent_=y;
		x->parent_=y->parent_;
		y->parent_=x;
		update_height(y);
		update_height(x);
		return x;
	}
	int get_balance(avl_node* node) const {
		return node?node->balance_factor():0;
	}
	void balance(avl_node* node) {
		while (node) {
			update_height(node);
			int balance=get_balance(node);
			if (balance>1) {
				if (get_balance(static_cast<avl_node*>(node->left_))>=0) node=rotate_right(node);
				else {
					node->left_=rotate_left(static_cast<avl_node*>(node->left_));
					node=rotate_right(node);
				}
			} else if (balance<-1) {
				if (get_balance(static_cast<avl_node*>(node->right_))<=0) node=rotate_left(node);
				else {
					node->right_=rotate_right(static_cast<avl_node*>(node->right_));
					node=rotate_left(node);
				}
			}
			if (node->parent_) {
				avl_node* parent=static_cast<avl_node*>(node->parent_);
				if (parent->left_==node) parent->left_=node;
				else parent->right_=node;
			} else root_=node;
			node=static_cast<avl_node*>(node->parent_);
		}
	}
	void transplant(binary_tree_node* u,binary_tree_node* v) override {
		base_type::transplant(u,v);
		if (v) balance(static_cast<avl_node*>(v));
	}
};

template <typename _Tp,typename _Key,typename _Compare,typename _Allocator>
void swap(avl_tree<_Tp,_Key,_Compare,_Allocator>& lhs,avl_tree<_Tp,_Key,_Compare,_Allocator>& rhs) noexcept {
	lhs.swap(rhs);
}

}

}

#endif