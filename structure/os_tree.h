//Last Modified At 2026/01/27
//@Version 1.0.0.0
#ifndef _STDEX_STRUCTURE_OS_TREE_H_
#define _STDEX_STRUCTURE_OS_TREE_H_ 1

#include "rb_tree.h"//At Least 1.1

namespace stdex {

namespace structure {

template <typename _Tp,typename _Key=_Tp,typename _Compare=std::less<_Key>,typename _Allocator=std::allocator<std::pair<const _Key,_Tp>>>
class os_tree : public rb_tree<_Tp,_Key,_Compare,_Allocator> {
protected:
	using base_type=rb_tree<_Tp,_Key,_Compare,_Allocator>;
	using typename base_type::binary_tree_node;
	using base_type::root_;
	using base_type::alloc_;
	using base_type::comp_;
	using base_type::size_;
	using base_type::create_node;
	using base_type::destroy_node;
	using base_type::transplant;
	using base_type::nil_;

	struct os_node : public base_type::rb_node {
		std::size_t size_=1;
		template <typename... _Args>
		os_node(os_node* nil,os_tree* tree,_Args &&...args) : base_type::rb_node(nil,tree,std::forward<_Args>(args)...) { }
		virtual ~os_node()=default;
		binary_tree_node *clone(binary_tree_node *parent=nullptr) override {
			os_node *result=new os_node(static_cast<os_node*>(this->nil_),static_cast<os_tree*>(this->tree_));
			binary_tree_node::sync_node(this,result,nullptr);
			result->color_=this->color_;
			result->size_=size_;
			return result;
		}
		void update_size(os_node* nil) noexcept {
			size_=1;
			if (this->left_ && this->left_!=nil) size_+=static_cast<os_node*>(this->left_)->size_;
			if (this->right_ && this->right_!=nil) size_+=static_cast<os_node*>(this->right_)->size_;
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

	os_tree() : base_type() { }
	explicit os_tree(const key_compare &comp,const allocator_type &alloc=allocator_type()) : base_type(comp,alloc) { }
	explicit os_tree(const allocator_type &alloc) : base_type(alloc) { }
	virtual ~os_tree()=default;

	os_tree(const os_tree &other) : base_type(other) { }
	os_tree(os_tree &&other) noexcept : base_type(std::move(other)) { }

	os_tree &operator =(const os_tree &other) {
		base_type::operator =(other);
		return *this;
	}
	os_tree &operator =(os_tree &&other) noexcept {
		base_type::operator =(std::move(other));
		return *this;
	}

	iterator kth_element(size_type k) {
		if (k>=size_) return this->end();
		return iterator(kth_element_impl(static_cast<os_node*>(root_),k));
	}
	const_iterator kth_element(size_type k) const {
		if (k>=size_) return this->cend();
		return const_iterator(kth_element_impl(static_cast<os_node*>(root_),k));
	}

	iterator select(size_type k) { return kth_element(k); }
	const_iterator select(size_type k) const { return kth_element(k); }

	size_type rank(const key_type& key) const {
		os_node* node=static_cast<os_node*>(this->find_impl(key));
		if (!node || node==nil_) return size_;
		return rank_impl(node);
	}
	std::pair<iterator,bool> insert(const value_type& value) override {
		auto result=base_type::insert(value);
		if (result.second) {
			os_node *node=static_cast<os_node*>(result.first.node());
			update_size_upwards(node);
		}
		return result;
	}
	iterator erase(const key_type& key) override {
		os_node* node=static_cast<os_node*>(this->find_impl(key));
		if (!node || node==nil_) return this->end();
		update_size_before_erase(node);
		return base_type::erase(key);
	}
	void clear() noexcept override {
		clear_subtree(static_cast<os_node*>(root_));
		root_=nil_;
		size_=0;
	}

protected:
	void init_nil() override {
		if (!nil_) {
			nil_=static_cast<os_node*>(create_node(value_type()));
			nil_->set_black();
			static_cast<os_node*>(nil_)->size_=0;
		}
		root_=nil_;
	}
	binary_tree_node *create_node(const value_type &value) {
		return new os_node(static_cast<os_node*>(nil_),this,value);
	}
	template <typename... _Args>
	binary_tree_node* create_node(_Args &&...args) {
		return new os_node(static_cast<os_node*>(nil_),this,std::forward<_Args>(args)...);
	}
	void clear_subtree(os_node* node) noexcept {
		if (node!=nil_) {
			clear_subtree(static_cast<os_node*>(node->left_));
			clear_subtree(static_cast<os_node*>(node->right_));
			destroy_node(node);
		}
	}
	void left_rotate(typename base_type::rb_node *x) override {
		base_type::left_rotate(x);
		os_node* y=static_cast<os_node*>(x->parent_);
		y->update_size(static_cast<os_node*>(nil_));
		static_cast<os_node*>(x)->update_size(static_cast<os_node*>(nil_));
	}
	void right_rotate(typename base_type::rb_node* y) override {
		base_type::right_rotate(y);
		os_node* x=static_cast<os_node*>(y->parent_);
		x->update_size(static_cast<os_node*>(nil_));
		static_cast<os_node*>(y)->update_size(static_cast<os_node*>(nil_));
	}
	void update_size_upwards(os_node* node) {
		while (node!=nil_) {
			node->update_size(static_cast<os_node*>(nil_));
			node=static_cast<os_node*>(node->parent_);
		}
	}
	void update_size_before_erase(os_node* node) {
		while (node!=nil_) {
			node->size_--;
			node=static_cast<os_node*>(node->parent_);
		}
	}
	binary_tree_node* kth_element_impl(os_node* node,size_type k) const {
		if (node==nil_) return nullptr;
		size_type left_size=(node->left_!=nil_)?static_cast<os_node*>(node->left_)->size_:0;
		if (k<left_size) return kth_element_impl(static_cast<os_node*>(node->left_),k);
		else if (k==left_size) return node;
		else return kth_element_impl(static_cast<os_node*>(node->right_),k-left_size-1);
		}
	size_type rank_impl(os_node *node) const {
		size_type r=(node->left_!=nil_)?static_cast<os_node*>(node->left_)->size_:0;
		os_node* current=node;
		while (current!=nil_ && current->parent_!=nil_) {
			if (current==current->parent_->right_) r+=(current->parent_->left_!=nil_)?static_cast<os_node*>(current->parent_->left_)->size_+1:1;
			current=static_cast<os_node*>(current->parent_);
		}
		return r;
	}

	void insert_fixup(typename base_type::rb_node* z) override {
		base_type::insert_fixup(z);
		update_size_upwards(static_cast<os_node*>(z));
	}
	void delete_fixup(typename base_type::rb_node* x) override {
		base_type::delete_fixup(x);
		if (x!=nil_) update_size_upwards(static_cast<os_node*>(x));
	}
	void transplant(binary_tree_node* u,binary_tree_node* v) override {
		if (u->parent_!=nil_) static_cast<os_node*>(u->parent_)->update_size(static_cast<os_node*>(nil_));
		base_type::transplant(u,v);
		if (v!=nil_) update_size_upwards(static_cast<os_node*>(v));
	}
};

template <typename _Tp,typename _Key,typename _Compare,typename _Allocator>
void swap(os_tree<_Tp,_Key,_Compare,_Allocator>& lhs,os_tree<_Tp,_Key,_Compare,_Allocator>& rhs) noexcept {
	lhs.swap(rhs);
}

}

}

#endif