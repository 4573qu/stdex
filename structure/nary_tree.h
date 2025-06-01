#ifndef _STL_NARY_TREE_H
#define _STL_NARY_TREE_H 1

#include <vector>
#include <memory>
#include <algorithm>
#include <optional>
#include <map>
#include <stack>
#include <queue>

//namespace my_tree {

template <typename _Tp>
class nary_tree
{
	public:
		template <typename _T=_Tp>
		class Node
		{
			public:
				_Tp value;
				int id;
				std::shared_ptr<Node<_T>> parent;
				std::vector<std::shared_ptr<Node<_T>>> children;
		
				Node()=default;
				Node(const _T& value):value(value){}
				Node(const _T& value,int id):value(value),id(id){}
				
				std::shared_ptr<Node<_T>>& operator[](std::size_t index) {
					if(index>=children.size()) throw std::out_of_range("Index out of range");
					return children[index];
				}
		
				void push_back(std::shared_ptr<Node<_T>> child){
					children.push_back(child);
				}
		
				bool is_leaf() const{
					return children.empty();
				}
		};
		
	private:
		std::map<int,std::shared_ptr<Node<_Tp>>> nodes;
		int next_id=0;
	
	public:
		std::shared_ptr<Node<_Tp>> root;
		nary_tree():root(nullptr),next_id(0){}
		nary_tree(const _Tp& root_value):next_id(0) {
			root=std::make_shared<Node<_Tp>>(root_value);
			nodes[next_id++]=root;
		}

		~nary_tree() {
			nodes.clear();//clear other nodes?
			root=nullptr;
			next_id=0;
		}
		
		nary_tree(const nary_tree& rhs):next_id(0){
			if(rhs.root){
    			root=std::make_shared<Node<_Tp>>(rhs.root->value);
				nodes[next_id++]=root;
				deep_copy_children(root,rhs.root);
			}
		}

		nary_tree& operator=(const nary_tree& rhs){
			if(&rhs==this) return *this;
	  		nodes.clear();
	  		root=nullptr;
			next_id=0;
			if(rhs.root){
				root=std::make_shared<Node<_Tp>>(rhs.root->value);
				nodes[next_id++]=root;
				deep_copy_children(root,rhs.root);
			}
		}
		
	public:
		//using iterator=std::shared_ptr<Node<_Tp>>;
		
		std::shared_ptr<Node<_Tp>> insert(const std::shared_ptr<Node<_Tp>>& parent,const _Tp& value){
	    	auto new_node=std::make_shared<Node<_Tp>>(value);
			new_node->parent=parent;
			parent->children.push_back(new_node);
			return new_node;
		}
		
		std::shared_ptr<Node<_Tp>> insert(const std::shared_ptr<Node<_Tp>>& parent,const std::shared_ptr<Node<_Tp>>& position, const _Tp& value){
			auto new_node=std::make_shared<Node<_Tp>>(value);
			new_node->parent=parent;
			auto it=std::find(parent->children.begin(),parent->children.end(),position);
			if(it==parent->children.end()) throw std::invalid_argument("Invalid position");
		    parent->children.insert(it,new_node);
		    return new_node;
		}
		
		void insert(const std::shared_ptr<Node<_Tp>>& parent,const nary_tree<_Tp>& subtree){
    		subtree->root->parent=parent;
			parent->children.push_back(subtree->root);
			return;
		}
		
		void erase(const std::shared_ptr<Node<_Tp>>& position){
			if(!position->parent) throw std::invalid_argument("Cannot erase root node");
			auto& siblings=position->parent->children;
			siblings.erase(std::remove(siblings.begin(),siblings.end(),position),siblings.end());
		}

		void update(const std::shared_ptr<Node<_Tp>>& position,const _Tp& value){
			position->value=value;
		}
		
		std::vector<std::shared_ptr<Node<_Tp>>> find(const _Tp& __val){
			return find_recursive(root,__val);
		}
		
		int height(const std::shared_ptr<Node<_Tp>>& node) const{
			if (!node) return 0;
			int max_height=0;
			for(const auto& child:node->children) max_height=std::max(max_height,height(child));
			return max_height+1;
		}
		
		int count(const std::shared_ptr<Node<_Tp>>& node) const{
			if (!node) return 0;
			int count=1;
			for(const auto& child:node->children) count+=count(child);
			return count;
		}
		
		std::vector<std::shared_ptr<Node<_Tp>>> preorder(){
			return preorder_traversal(root);
		}
		
		std::vector<std::shared_ptr<Node<_Tp>>> inorder(){
			return inorder_traversal(root);
		}
		
		std::vector<std::shared_ptr<Node<_Tp>>> inorder(int index){
			return inorder_traversal(root,index);
		}
		
		std::vector<std::shared_ptr<Node<_Tp>>> postorder(){
			return postorder_traversal(root);
		}
		
		std::vector<std::shared_ptr<Node<_Tp>>> levelorder(){
			return levelorder_traversal(root);
		}
		
		template <typename _Function>
		void preorder(_Function func){
			preorder_traversal(root,func);
		}
		
		template <typename _Function>
		void inorder(_Function func){
			inorder_traversal(root,func);
		}
		
		template <typename _Function>
		void inorder(int index,_Function func){
			inorder_traversal(root,func,index);
		}
		
		template <typename _Function>
		void postorder(_Function func){
			postorder_traversal(root,func);
		}
		
		template <typename _Function>
		void levelorder(_Function func){
			levelorder_traversal(root,func);
		}
		
		
		template <typename _Function,typename _LevelFunction>
		void levelorder(_Function func,_LevelFunction levelfunc){
			levelorder_traversal(root,func,levelfunc);
		}

	private:
		void clear(const std::shared_ptr<Node<_Tp>>& node){
			nodes.clear();
		}
		
		void deep_copy_children(std::shared_ptr<Node<_Tp>> new_node,std::shared_ptr<Node<_Tp>> original_node){
			for(auto& original_child:original_node->children) {
				auto new_child=std::make_shared<Node<_Tp>>(original_child->value);
				nodes[next_id++]=new_child;
				new_node->children.push_back(new_child);
				deep_copy_children(new_child, original_child);
			}
		}
		
		std::vector<std::shared_ptr<Node<_Tp>>> find_recursive(const std::shared_ptr<Node<_Tp>>& node, const _Tp& value){
			std::vector<std::shared_ptr<Node<_Tp>>> result;
			if(node->value==value) result.push_back(node);
			for(auto& child:node->children){
				auto find_result=find_recursive(child,value);
	   			for(auto it:find_result) result.push_back(it);
			}
			return result;
		}

		std::vector<std::shared_ptr<Node<_Tp>>> preorder_traversal(const std::shared_ptr<Node<_Tp>>& node) const{
			std::vector<std::shared_ptr<Node<_Tp>>> result;
			if (!node) return result;
			result.push_back(node);
			for(const auto& child:node->children){
				auto find_result=preorder_traversal(child);
				for(auto it:find_result) result.push_back(it);
			}
			return result;
		}
		
		std::vector<std::shared_ptr<Node<_Tp>>> inorder_traversal(const std::shared_ptr<Node<_Tp>>& node,int index=0) const{
    		std::vector<std::shared_ptr<Node<_Tp>>> result;
			if(!node) return result;
			if(index<0) result.push_back(node);
		    int child_index=0;
			for(const auto& child:node->children) {
        		auto find_result=inorder_traversal(child,index);
        		result.insert(result.end(),find_result.begin(),find_result.end());
				if(child_index++==index) result.push_back(node);
			}
			if(index>=0 && index>=node->children.size()) result.push_back(node);
			return result;
		}
		
		std::vector<std::shared_ptr<Node<_Tp>>> postorder_traversal(const std::shared_ptr<Node<_Tp>>& node) const{
			std::vector<std::shared_ptr<Node<_Tp>>> result;
			if (!node) return result;
			for(const auto& child:node->children){
				auto find_result=postorder_traversal(child);
				for(auto it:find_result) result.push_back(it);
			}
			result.push_back(node);
			return result;
		}
	
		std::vector<std::shared_ptr<Node<_Tp>>> levelorder_traversal(const std::shared_ptr<Node<_Tp>>& node) const{
			std::vector<std::shared_ptr<Node<_Tp>>> result;
			if (!node) return result;
			std::queue<std::shared_ptr<Node<_Tp>>> queue;
			queue.push(node);
			while(!queue.empty()){
				auto node=queue.front();
				queue.pop();
				result.push_back(node);
				for(const auto& child:node->children) queue.push(child);
			}
			return result;
		}
		
		template <typename _Function>
		void preorder_traversal(const std::shared_ptr<Node<_Tp>>& node,_Function func) const{
			if (!node) return;
			func(node);
			for(const auto& child:node->children) preorder_traversal(child,func);
		}
		
		template <typename _Function>
		void inorder_traversal(const std::shared_ptr<Node<_Tp>>& node,_Function func,int index=0) const{
			if (!node) return;
			if(index<0) func(node);
			int child_index=0;
			for(const auto& child:node->children){
				inorder_traversal(child,func,index);
				if(child_index++==index) func(node);
			}
			if(index>=0 && index>=node->children.size()) func(node);
		}
		
		template <typename _Function>
		void postorder_traversal(const std::shared_ptr<Node<_Tp>>& node,_Function func) const{
			if (!node) return;
			for(const auto& child:node->children) postorder_traversal(child,func);
			func(node);
		}
		
		template <typename _Function>
		void levelorder_traversal(const std::shared_ptr<Node<_Tp>>& node,_Function func) const{
			if (!node) return;
			std::queue<std::shared_ptr<Node<_Tp>>> queue;
			queue.push(node);
			while(!queue.empty()){
				int size=queue.size();
				for(int i=0;i<size;++i){
					auto node=queue.front();
					queue.pop();
					func(node);
					for(const auto& child:node->children) queue.push(child);
				}
			}
		}
		
		template <typename _Function,typename _LevelFunction>
		void levelorder_traversal(const std::shared_ptr<Node<_Tp>>& node,_Function func,_LevelFunction levelfunc) const{
			if (!node) return;
			std::queue<std::shared_ptr<Node<_Tp>>> queue;
			queue.push(node);
			while(!queue.empty()){
				int size=queue.size();
				for(int i=0;i<size;++i){
					auto node=queue.front();
					queue.pop();
					func(node);
					for(const auto& child:node->children) queue.push(child);
				}
				levelfunc();
			}
		}
		
	public:
		class iterator{
			friend class nary_tree;
			private:
				std::stack<std::shared_ptr<Node<_Tp>>> stack;
			public:
				iterator()=default;
				explicit iterator(const std::shared_ptr<Node<_Tp>>& root) {
					if (root) stack.push(root);
				}
				
				iterator& operator++(){
					if (stack.empty()) throw std::runtime_error("Cannot increment past the end");
					auto node=stack.top();
					stack.pop();
					for(auto it=node->children.rbegin();it!=node->children.rend();it++) stack.push(*it);
					return *this;
				}
						
				bool operator==(const iterator& rhs) const{
					return stack==rhs.stack;
				}
				
				bool operator!=(const iterator& rhs) const{
					return !(*this==rhs);
				}
				
				std::shared_ptr<Node<_Tp>>& operator*() const{
					return stack.top();
				}
				
				std::shared_ptr<Node<_Tp>>* operator->() const{
					return &stack.top();
				}
		};
		
		iterator begin(){
			return iterator(root);
		}

		iterator end(){
			return iterator();
		}
};

//}

#endif