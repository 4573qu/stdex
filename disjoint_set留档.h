#ifndef _STL_DISJOINTSET_H
#define _STL_DISJOINTSET_H 1

#include <vector>
#include <utility>
#include <unordered_set>

template <typename _Tp>
class disjoint_set
{
	public:
		struct Node{
			_Tp value;
			_Tp parent;
			unsigned int rank;
		};
    	
	private:
		Node* nodes;
		std::size_t capacity;
		std::size_t size;
		void resize(std::size_t new_capacity){
			Node* new_nodes=new Node[new_capacity];
			for(std::size_t i=0;i<size;i++) new_nodes[i]=nodes[i];
			delete[] nodes;
			nodes=new_nodes;
			capacity=new_capacity;
		}
		
	public:
		explicit disjoint_set():nodes(nullptr),capacity(0),size(0){}

		~disjoint_set(){
			delete[] nodes;
		}
		
		disjoint_set(const disjoint_set& rhs):nodes(new Node[rhs.capacity]),capacity(rhs.capacity),size(rhs.size){
			std::copy(rhs.nodes,rhs.nodes+rhs.size,nodes);
		}

		disjoint_set& operator=(const disjoint_set& rhs){
			if(&rhs==this) return *this;
			delete[] nodes;
			nodes=new Node[rhs.capacity];
			capacity=rhs.capacity;
			size=rhs.size;
			std::copy(rhs.nodes,rhs.nodes+rhs.size,nodes);
			return *this;
		}

		void emplace(const _Tp& value){
			if(size==capacity) resize(capacity==0?1:capacity*2);
			nodes[size++]={value,value,0};
		}

		_Tp find(const _Tp& element){
			if(nodes[element].parent==element) return element;
			return nodes[element].parent=find(nodes[element].parent);
		}
		
		_Tp routinefind(const _Tp& element){
			if(nodes[element].parent==element) return element;
			//nodes[element].rank=nodes[routinefind(nodes[element].parent)].rank+1;
			return (nodes[element]=routinefind(nodes[element].parent));
		}

		void merge(const _Tp& element1,const _Tp& element2) {
			_Tp root_of_element1=find(element1);
			_Tp root_of_element2=find(element2);

			if(root_of_element1==root_of_element2) return;

			if(nodes[root_of_element1].rank<nodes[root_of_element2].rank) nodes[root_of_element1].parent=root_of_element2;
			else if(nodes[root_of_element1].rank>nodes[root_of_element2].rank) nodes[root_of_element2].parent=root_of_element1;
			else{
				nodes[root_of_element2].parent=root_of_element1;
				nodes[root_of_element1].rank++;
			}
		}
		
		std::vector<_Tp> get(const _Tp& element){
			std::vector<_Tp> set;
			_Tp root_of_element=find(element);
			for(std::size_t i=0;i<size;i++){
				if(find(i)==root_of_element) set.push_back(i);
			}
			return set;
		}
  
		std::size_t count(){
			std::unordered_set<_Tp> uniqueParents;
			for(std::size_t i=0;i<size;i++) uniqueParents.insert(find(i));
			return uniqueParents.size();
		}
  
		bool contains(const _Tp& element){
			return element<size;
		}
		
		unsigned int depth(const _Tp& element){
			return nodes[find(element)].rank;
		}
		
		bool is_same(const _Tp& element1,const _Tp& element2){
			return find(element1)==find(element2);
		}
    
	public:
		class iterator{
			friend class disjoint_set;
			private:
 				Node* ptr;
				iterator(Node* p):ptr(p){}
			public:
				iterator& operator++(){
					ptr++;
					return *this;
				}
				
				iterator operator++(int) {
					iterator it(*this);
					operator++();
					return it;
				}
        		
				iterator& operator--(){
					ptr--;
					return *this;
				}
				
				iterator operator--(int){
					iterator it(*this);
					operator--();
					return it;
				}
        		
				bool operator!=(const iterator& rhs) const{
					return ptr!=rhs.ptr;
				}
        		
				Node* const& operator*() const{
					return ptr;
				}
        		
				Node** operator->(){
					return &ptr;
				}
		};

		iterator begin(){
			return iterator(nodes);
		}

		iterator end(){
			return iterator(nodes+size);
		}
};

#endif