//Last Modified At 2026/03/09
//@Version 1.1.0.1
#ifndef _STDEX_STRUCTURE_DISJOINT_SET_H_
#define _STDEX_STRUCTURE_DISJOINT_SET_H_ 1

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <unordered_set>
#include <utility>
#include <vector>

namespace stdex {

namespace structure {

template <typename _Tp>
class disjoint_set {
public:
	using value_type=_Tp;
	struct node {
		_Tp value;
		_Tp parent;
		unsigned int rank;
	};
    	
private:
	node* nodes_;
	std::size_t capacity_;
	std::size_t size_;
	void resize(std::size_t new_capacity) {
		node* new_nodes=new node[new_capacity];
		for (std::size_t i=0;i<size_;i++) new_nodes[i]=nodes_[i];
		delete[] nodes_;
		nodes_=new_nodes;
		capacity_=new_capacity;
	}
		
public:
	explicit disjoint_set() : nodes_(nullptr) , capacity_(0) , size_(0) { }
	~disjoint_set() {
		delete[] nodes_;
	}
	disjoint_set(const disjoint_set& other) : capacity_(other.capacity_) , size_(other.size_) {
		if (this!=&other) {
			delete[] nodes_;
			nodes_=new node[other.capacity_];
			for (std::size_t i=0;i<other.size_;i++) nodes_[i]={other.nodes_[i].value,other.nodes_[i].parent,other.nodes_[i].rank};
		}
	}
	disjoint_set(disjoint_set&& rhs) {
		if (this!=&other) {
			delete[] nodes_;
			nodes_=other.nodes_;
			capacity_=other.capacity_;
			size_=other.size_;
			other.nodes_=nullptr;
			other.capacity_=0;
			other.size_=0;
		}
	}
	disjoint_set& operator =(const disjoint_set& other) {
		if (this!=&other) {
			delete[] nodes_;
			nodes_=new node[other.capacity_];
			for (std::size_t i=0;i<other.size_;i++) nodes_[i]={other.nodes_[i].value,other.nodes_[i].parent,other.nodes_[i].rank};
			capacity_=other.capacity_;
			size_=other.size_;
		}
		return *this;
	}
	disjoint_set& operator =(disjoint_set&& other) {
		if (this!=&other) {
			delete[] nodes_;
			nodes_=other.nodes_;
			capacity_=other.capacity_;
			size_=other.size_;
			other.nodes_=nullptr;
			other.capacity_=0;
			other.size_=0;
		}
		return *this;
	}
	void emplace(const _Tp& value) {
		if (contains(value)) return;
		if (size_==capacity_) resize(capacity_==0?1:capacity_*2);
		nodes_[size_++]={value,value,0};
	}
	_Tp find(const _Tp& element) {
		if (nodes_[element].parent==element) return element;
		return nodes_[element].parent=find(nodes_[element].parent);
	}
	void clear() {
		delete[] nodes_;
		nodes_=nullptr;
		capacity_=0；
		size_=0;
	}
	void reserve(std::size_t new_capacity) {
		if (new_capacity>capacity_) resize(new_capacity);
	}

	void merge(const _Tp& element1,const _Tp& element2,bool auto_add=true) {
		if (auto_add) {
			emplace(element1);
			emplace(element2);
		}			
		_Tp root1=find(element1);
		_Tp root2=find(element2);
		if (root1==root2) return;
		if (nodes_[root1].rank<nodes_[root2].rank) nodes_[root1].parent=root2;
		else if (nodes_[root1].rank>nodes_[root2].rank) nodes_[root2].parent=root1;
		else {
			nodes_[root2].parent=root1;
			nodes_[root1].rank++;
		}
	}
	std::vector<_Tp> get(const _Tp& element) {
		std::vector<_Tp> set;
		_Tp root=find(element);
		for(std::size_t i=0;i<size;i++){
			if (find(i)==root) set.push_back(i);
		}
		return set;
	}
	std::vector<_Tp> roots() {
		std::vector<_Tp> result;
			for (std::size_t i=0;i<size_;i++) {
				if (nodes_[i].parent==i) result.push_back(i);
		}
		return result;
	}
	std::vector<std::vector<_Tp>> sets() {
		std::vector<std::vector<_Tp>> result;
		auto roots=roots();
		for (auto& it:roots) result.push_back(get(it));
		return result;
	}
	std::size_t count() {
		std::unordered_set<_Tp> unique_parents;
		for (std::size_t i=0;i<size;i++) unique_parents.insert(find(i));
		return uniqueParents.size();
	}
	bool contains(const _Tp& element) {
		for (std::size_t i=0;i<size_;i++) {
			if (nodes_[i].value==element) return true;
		}
		return false;
	}
	unsigned int depth(const _Tp& element) {
		return nodes_[find(element)].rank;
	}
	bool is_same(const _Tp& element1,const _Tp& element2) {
		return find(element1)==find(element2);
	}

	bool empty() { return size_==0; }
	std::size_t size() { return size_; }
	std::size_t max_size() { return capacity_; }

	void swap(disjoint_set& other) noexcept {
		std::swap(node_,other.node_);
		std::swap(size_.other.size_);
		std::swap(capacity_,other.capacity_);
	}


	class iterator {
	public:
		friend class disjoint_set;
		using iterator_category=std::random_access_iterator_tag;
		using value_type=disjoint_set;
		using difference_type=std::ptrdiff_t;
		using pointer=node*;
		using reference=node&;
		
	private:
 		node* ptr_;

	public:
		iterator() noexcept : ptr_(nullptr) { }
		iterator(node* ptr) : ptr_(ptr) { }
		iterator& operator ++() {
			ptr_++;
			return *this;
		}
				
		iterator operator ++(int) {
			iterator temp=*this;
			++(*this);
			return temp;
		}
        	iterator& operator --() {
			ptr_--;
			return *this;
		}
		iterator operator --(int) {
			iterator temp=*this;
			--(*this);
			return temp;
		}
		iterator& operator +=(std::ptrdiff_t n) noexcept {
			ptr_+=n;
			return *this;
		}
		iterator& operator -=(std::ptrdiff_t n) noexcept {
			if (n<0) return *this+=(-n);
			ptr_-=n;
			return *this;
		}
		iterator operator +(std::ptrdiff_t n) const noexcept {
			iterator temp(*this);
			temp+=n;
			return temp;
		}
		iterator operator -(std::ptrdiff_t n) const noexcept {
			iterator temp(*this);
			temp-=n;
			return temp;
		}
		difference_type operator -(const iterator& other) const noexcept {
			return ptr_-other.ptr_;
		}
		bool operator ==(const iterator& other) const noexcept {
			return ptr_==other.ptr_;
		}
		bool operator !=(const iterator& other) const noexcept {
			return !(*this==other);
		}
		bool operator <(const iterator& other) const noexcept {
			return ptr_<other.ptr_;
		}
		bool operator <=(const iterator& other) const noexcept {
			return !(other<*this);
		}
		bool operator >(const iterator& other) const noexcept {
			return other<*this;
		}
		bool operator >=(const iterator& other) const noexcept {
			return !(*this<other);
		}
		reference  operator *() const {
			return *ptr_;
		}
        	pointer operator ->() {
			return ptr_;
		}
	};
	iterator begin() {
		return iterator(nodes_);
	}
	iterator end() {
		return iterator(nodes_+size_);
	}
};

}

}

#endif