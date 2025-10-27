//Last Modified At 2025/10/15
//@Version 1.0.0.0
#ifndef _STDEX_STRUCTURE_RB_TREE_H_
#define _STDEX_STRUCTURE_RB_TREE_H_ 1

#include "bst_tree.h"//At Least 1.0

namespace stdex {

namespace structure {

template <typename _Tp,typename _Key=_Tp,typename _Compare=std::less<_Key>,typename _Allocator=std::allocator<std::pair<const _Key,_Tp>>>
class rb_tree : public bst_tree<_Tp,_Key,_Compare,_Allocator> {
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

	enum rb_tree_node_color {
		RBTNC_RED,
		RBTNC_BLACK
	};
	struct rb_node : public binary_tree_node {
		rb_tree_node_color color_=RBTNC_RED;
		template <typename... _Args>
		rb_node(_Args&&... args) : binary_tree_node(std::forward<_Args>(args)...) {}
		bool is_red() const noexcept { return color_==RBTNC_RED; }
		bool is_black() const noexcept { return color_==RBTNC_BLACK; }
		void set_red() noexcept { color_=RBTNC_RED; }
		void set_black() noexcept { color_=RBTNC_BLACK; }
		binary_tree_node* clone(binary_tree_node* parent=nullptr) {
			rb_node* result=new rb_node;
			sync_node(this,result,parent);
			result.color_=color_;
			return result;
		}
	};

	rb_node* nil_=nullptr;

public:
	using typename base_type::key_type;
	using typename base_type::mapped_type;
	using typename base_type::value_type;
	using typename base_type::key_compare;
	using typename base_type::allocator_type;
	using typename base_type::size_type;
	using typename base_type::iterator;
	using typename base_type::const_iterator;

	rb_tree() {
		init_nil();
	}
	explicit rb_tree(const key_compare& comp,const allocator_type& alloc=allocator_type()) : base_type(comp,alloc) {
		init_nil();
	}
	explicit rb_tree(const allocator_type& alloc) : base_type(alloc) {
		init_nil();
	}
	virtual ~rb_tree() {
		clear();
		if (nil_) destroy_node(nil_);
	}

	rb_tree(const rb_tree& other) : base_type() {
		init_nil();
		for (const auto& it:other) insert(it);
	}
	rb_tree(rb_tree&& other) noexcept : base_type(std::move(other)) {
		nil_=other.nil_;
		other.nil_=nullptr;
	}
	
	rb_tree& operator =(const rb_tree& other) {
		if (this!=&other) {
			clear();
			for (const auto& it:other) insert(it);
		}
		return *this;
	}
	rb_tree& operator =(rb_tree&& other) noexcept {
		if (this!=&other) {
			clear();
			if (nil_) destroy_node(nil_);
			root_=other.root_;
			alloc_=std::move(other.alloc_);
			comp_=std::move(other.comp_);
			size_=other.size_;
			nil_=other.nil_;
			other.root_=nil_;
			other.nil_=nullptr;
			other.size_=0;
		}
		return *this;
	}

	std::pair<iterator,bool> insert(const value_type& value) override {
		rb_node* new_node=static_cast<rb_node*>(create_node(value));
		new_node->left_=nil_;
		new_node->right_=nil_;
		new_node->set_red();
		rb_node* parent=nil_;
		rb_node* current=static_cast<rb_node*>(root_);
		while (current!=nil_) {
			parent=current;
			if (comp_(value.first,current->key())) current=static_cast<rb_node*>(current->left_);
			else if (comp_(current->key(),value.first)) current=static_cast<rb_node*>(current->right_);
			else {
				destroy_node(new_node);
				return {iterator(current),false};
			}
		}
		new_node->parent_=parent;
		if (parent==nil_) root_=new_node;
		else if (comp_(value.first,parent->key())) parent->left_=new_node;
		else parent->right_=new_node;
		size_++;
		insert_fixup(new_node);
		return {iterator(new_node),true};
	}
	size_type erase(const key_type& key) override {
		rb_node* node=static_cast<rb_node*>(this->find_impl(key));
		if (node==nil_) return 0;
		rb_node* y=node;
		rb_node* x=nil_;
		color y_original_color=y->color_;
		if (node->left_==nil_) {
			x=static_cast<rb_node*>(node->right_);
			transplant(node,x);
		} else if (node->right_==nil_) {
			x=static_cast<rb_node*>(node->left_);
			transplant(node,x);
		} else {
			y=static_cast<rb_node*>(this->min_node(node->right_));
			y_original_color=y->color_;
			x=static_cast<rb_node*>(y->right_);
			if (y->parent_==node) x->parent_=y;
			else {
				transplant(y,x);
				y->right_=node->right_;
				y->right_->parent_=y;
			}
			transplant(node,y);
			y->left_=node->left_;
			y->left_->parent_=y;
			y->color_=node->color_;
		}
		destroy_node(node);
		size_--;
		if (y_original_color==RBTNC_BLACK) delete_fixup(x);
		return 1;
	}
	virtual void clear() noexcept override {
		clear_subtree(static_cast<rb_node*>(root_));
		root_=nil_;
		size_=0;
	}

protected:
	void init_nil() {
		nil_=static_cast<rb_node*>(create_node(value_type()));
		nil_->set_black();
		root_=nil_;
	}
	binary_tree_node* create_node(const value_type& value) override {
		return new rb_node(value);
	}
	template <typename... _Args>
	binary_tree_node* create_node(_Args&&... args) {
		return new rb_node(std::forward<_Args>(args)...);
	}
	void clear_subtree(rb_node* node) noexcept {
		if (node!=nil_) {
			clear_subtree(static_cast<rb_node*>(node->left_));
			clear_subtree(static_cast<rb_node*>(node->right_));
			destroy_node(node);
		}
	}
	void left_rotate(rb_node* x) {
		rb_node* y=static_cast<rb_node*>(x->right_);
		x->right_=y->left_;
		if (y->left_!=nil_) y->left_->parent_=x;
		y->parent_=x->parent_;
		if (x->parent_==nil_) root_=y;
		else if (x==x->parent_->left_) x->parent_->left_=y;
		else x->parent_->right_=y;
		y->left_=x;
		x->parent_=y;
	}
	void right_rotate(rb_node* y) {
		rb_node* x=static_cast<rb_node*>(y->left_);
		y->left_=x->right_;
		if (x->right_!=nil_) x->right_->parent_=y;
		x->parent_=y->parent_;
		if (y->parent_==nil_) root_=x;
		else if (y==y->parent_->right_) y->parent_->right_=x;
		else y->parent_->left_=x;
		x->right_=y;
		y->parent_=x;
	}

	void insert_fixup(rb_node* z) {
		while (static_cast<rb_node*>(z->parent_)->is_red()) {
			if (z->parent_==z->parent_->parent_->left_) {
				rb_node* y=static_cast<rb_node*>(z->parent_->parent_->right_);
				if (y->is_red()) {
					static_cast<rb_node*>(z->parent_)->set_black();
					y->set_black();
					static_cast<rb_node*>(z->parent_->parent_)->set_red();
					z=static_cast<rb_node*>(z->parent_->parent_);
				} else {
					if (z==z->parent_->right_) {
						z=static_cast<rb_node*>(z->parent_);
						left_rotate(z);
					}
					static_cast<rb_node*>(z->parent_)->set_black();
					static_cast<rb_node*>(z->parent_->parent_)->set_red();
					right_rotate(static_cast<rb_node*>(z->parent_->parent_));
				}
			} else {
				rb_node* y=static_cast<rb_node*>(z->parent_->parent_->left_);
				if (y->is_red()) {
					static_cast<rb_node*>(z->parent_)->set_black();
					y->set_black();
					static_cast<rb_node*>(z->parent_->parent_)->set_red();
					z=static_cast<rb_node*>(z->parent_->parent_);
				} else {
					if (z==z->parent_->left_) {
						z=static_cast<rb_node*>(z->parent_);
						right_rotate(z);
					}
					static_cast<rb_node*>(z->parent_)->set_black();
					static_cast<rb_node*>(z->parent_->parent_)->set_red();
					left_rotate(static_cast<rb_node*>(z->parent_->parent_));
				}
			}
		}
		static_cast<rb_node*>(root_)->set_black();
	}
	void delete_fixup(rb_node* x) {
		while (x!=root_ && x->is_black()) {
			if (x==x->parent_->left_) {
				rb_node* w=static_cast<rb_node*>(x->parent_->right_);
				if (w->is_red()) {
					w->set_black();
					static_cast<rb_node*>(x->parent_)->set_red();
					left_rotate(static_cast<rb_node*>(x->parent_));
					w=static_cast<rb_node*>(x->parent_->right_);
				}
				if (static_cast<rb_node*>(w->left_)->is_black() && static_cast<rb_node*>(w->right_)->is_black()) {
					w->set_red();
					x=static_cast<rb_node*>(x->parent_);
				} else {
					if (static_cast<rb_node*>(w->right_)->is_black()) {
						static_cast<rb_node*>(w->left_)->set_black();
						w->set_red();
						right_rotate(w);
						w=static_cast<rb_node*>(x->parent_->right_);
					}
					w->color_=static_cast<rb_node*>(x->parent_)->color_;
					static_cast<rb_node*>(x->parent_)->set_black();
					static_cast<rb_node*>(w->right_)->set_black();
					left_rotate(static_cast<rb_node*>(x->parent_));
					x=static_cast<rb_node*>(root_);
				}
			} else {
				rb_node* w=static_cast<rb_node*>(x->parent_->left_);
				if (w->is_red()) {
					w->set_black();
					static_cast<rb_node*>(x->parent_)->set_red();
					right_rotate(static_cast<rb_node*>(x->parent_));
					w=static_cast<rb_node*>(x->parent_->left_);
				}
				if (static_cast<rb_node*>(w->right_)->is_black() && static_cast<rb_node*>(w->left_)->is_black()) {
					w->set_red();
					x=static_cast<rb_node*>(x->parent_);
				} else {
					if (static_cast<rb_node*>(w->left_)->is_black()) {
						static_cast<rb_node*>(w->right_)->set_black();
						w->set_red();
						left_rotate(w);
						w=static_cast<rb_node*>(x->parent_->left_);
					}
					w->color_=static_cast<rb_node*>(x->parent_)->color_;
					static_cast<rb_node*>(x->parent_)->set_black();
					static_cast<rb_node*>(w->left_)->set_black();
					right_rotate(static_cast<rb_node*>(x->parent_));
					x=static_cast<rb_node*>(root_);
				}
			}
		}
		x->set_black();
	}
	void transplant(binary_tree_node* u,binary_tree_node* v) override {
		if (u->parent_==nil_) root_=v;
		else if (u==u->parent_->left_) u->parent_->left_=v;
		else u->parent_->right_=v;
		v->parent_=u->parent_;
	}
};

template <typename _Tp,typename _Key,typename _Compare,typename _Allocator>
void swap(rb_tree<_Tp,_Key,_Compare,_Allocator>& lhs,rb_tree<_Tp,_Key,_Compare,_Allocator>& rhs) noexcept {
	lhs.swap(rhs);
}

}

}

#endif