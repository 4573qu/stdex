//Last Modified At 2025/10/15
//@Version 1.0.0.0
#ifndef _STDEX_STRUCTURE_BST_TREE_H_
#define _STDEX_STRUCTURE_BST_TREE_H_ 1

#include "binary_tree.h"//At Least 1.0

namespace stdex {

namespace structure {

template <typename _Tp,typename _Key=_Tp,typename _Compare=std::less<_Key>,typename _Allocator=std::allocator<std::pair<const _Key,_Tp>>>
class bst_tree : public binary_tree<_Tp,_Key,_Compare,_Allocator> {
protected:
	using base_type=binary_tree<_Tp,_Key,_Compare,_Allocator>;
	using typename base_type::binary_tree_node;
	using base_type::root_;
	using base_type::alloc_;
	using base_type::comp_;
	using base_type::size_;
	using base_type::create_node;
	using base_type::destroy_node;

public:
	using typename base_type::key_type;
	using typename base_type::mapped_type;
	using typename base_type::value_type;
	using typename base_type::key_compare;
	using typename base_type::allocator_type;
	using typename base_type::size_type;
	using typename base_type::iterator;
	using typename base_type::const_iterator;

	bst_tree()=default;
	explicit bst_tree(const key_compare& comp,const allocator_type& alloc=allocator_type()) : base_type(comp,alloc) { }
	explicit bst_tree(const allocator_type& alloc) : base_type(alloc) { }
	virtual ~bst_tree()=default;

	bst_tree(const bst_tree& other) : base_type(other) { }
	bst_tree(bst_tree&& other) noexcept : base_type(std::move(other)) { }

	bst_tree& operator =(const bst_tree& other) {
		base_type::operator =(other);
		return *this;
	}
	bst_tree& operator =(bst_tree&& other) noexcept {
		base_type::operator =(std::move(other));
		return *this;
	}
	virtual std::pair<iterator,bool> insert(const value_type& value) override {
		return insert_impl(value);
	}
	virtual size_type erase(const key_type& key) override {
		return erase_impl(key);
	}
	virtual iterator find(const key_type& key) override {
		return iterator(find_impl(key));
	}
	virtual const_iterator find(const key_type& key) const override {
		return const_iterator(find_impl(key));
	}

protected:
	binary_tree_node* find_impl(const key_type& key) const {
		binary_tree_node* current=root_;
		while (current) {
			if (comp_(key,current->key())) current=current->left_;
			else if (comp_(current->key(),key)) current=current->right_;
			else return current;
		}
		return nullptr;
	}
	std::pair<iterator,bool> insert_impl(const value_type& value) {
		if (!root_) {
			root_=create_node(value);
			size_=1;
			return {iterator(root_),true};
		}
		binary_tree_node* current=root_;
		binary_tree_node* parent=nullptr;
		while (current) {
			parent=current;
			if (comp_(value.first,current->key())) current=current->left_;
			else if (comp_(current->key(),value.first)) current=current->right_;
			else return {iterator(current),false};
		}
		binary_tree_node* new_node=create_node(value);
		new_node->parent_=parent;
		if (comp_(value.first,parent->key())) parent->left_=new_node;
		else parent->right_=new_node;
		size_++;
		return {iterator(new_node),true};
	}
	size_type erase_impl(const key_type& key) {
		binary_tree_node* node=find_impl(key);
		if (!node) return 0;
		erase_node(node);
		return 1;
	}
	void erase_node(binary_tree_node* node) {
		if (!node) return;
		if (!node->left_ && !node->right_) transplant(node,nullptr);
		else if (!node->left_) transplant(node,node->right_);
		else if (!node->right_) transplant(node,node->left_);
		else {
			binary_tree_node* successor=this->min_node(node->right_);
			if (successor->parent_!=node) {
				transplant(successor,successor->right_);
				successor->right_=node->right_;
				successor->right_->parent_=successor;
			}
			transplant(node,successor);
			successor->left_=node->left_;
			successor->left_->parent_=successor;
		}
		destroy_node(node);
		size_--;
	}
	virtual void transplant(binary_tree_node* u,binary_tree_node* v) {
		if (!u->parent_) root_=v;
		else if (u==u->parent_->left_) u->parent_->left_=v;
		else u->parent_->right_=v;
		if (v) v->parent_=u->parent_;
	}
};

template <typename _Tp,typename _Key,typename _Compare,typename _Allocator>
void swap(bst_tree<_Tp,_Key,_Compare,_Allocator>& lhs,bst_tree<_Tp,_Key,_Compare,_Allocator>& rhs) noexcept {
	lhs.swap(rhs);
}

}

}

#endif