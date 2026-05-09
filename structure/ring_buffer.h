//Last Modified At 2025/10/10
//@Version 1.0.0.0
#ifndef _STDEX_STRUCTURE_RING_BUFFER_H_
#define _STDEX_STRUCTURE_RING_BUFFER_H_ 1

#include <cstddef>
#include <iterator>
#include <memory>
#include <stdexcept>
#include <type_traits>

namespace stdex {

namespace structure {

template <typename _Tp,std::size_t _Capacity>
class ring_buffer {
	_Tp* data_=nullptr;
	std::size_t size_=0;
	std::size_t head_=0;
	std::size_t tail_=0;

public:
	class iterator {
		_Tp* ptr_=nullptr;
		_Tp* begin_=nullptr;
		_Tp* end_=nullptr;

		friend class ring_buffer;
		iterator(_Tp* ptr,_Tp* begin,_Tp* end) : ptr_(ptr) , begin_(begin) , end_(end) { }

	public:
		iterator()=default;
		_Tp& operator *() const { return *ptr_; }
		_Tp* operator ->() const { return ptr_; }
		iterator& operator ++() {
			ptr_++;
			if (ptr_==end_) ptr_=begin_;
			return *this;
		}
		iterator operator ++(int) {
			iterator temp=*this;
			++(*this);
			return temp;
		}
		iterator& operator --() {
			if (ptr_==begin_) ptr_=end_;
			--ptr_;
			return *this;
		}
		iterator operator --(int) {
			iterator temp=*this;
			--(*this);
			return temp;
		}
		bool operator ==(const iterator& other) const { return ptr_==other.ptr_; }
		bool operator !=(const iterator& other) const { return ptr_!=other.ptr_; }
	};
	ring_buffer() : data_(std::allocator<_Tp>().allocate(_Capacity)) { }
	~ring_buffer() {
		clear();
		std::allocator<_Tp>().deallocate(data_,_Capacity);
	}
	ring_buffer(const ring_buffer& other) : data_(std::allocator<_Tp>().allocate(Capacity)) {
		try {
			for (const auto& it:other) push_back(it);
		} catch (...) {
			clear();
			std::allocator<_Tp>().deallocate(data_,_Capacity);
			throw;
		}
	}
	ring_buffer(ring_buffer&& other) noexcept : data_(other.data_) , size_(other.size_) , head_(other.head_) , tail_(other.tail_) {
		other.data_=nullptr;
		other.size_=other.head_=other.tail_=0;
	}
	ring_buffer& operator =(const ring_buffer& other) {
		if (this!=&other) {
			ring_buffer temp(other);
			swap(temp);
		}
		return *this;
	}
	ring_buffer& operator =(ring_buffer&& other) noexcept {
		if (this!=&other) {
			clear();
			std::allocator<_Tp>().deallocate(data_,_Capacity);
			data_=other.data_;
			size_=other.size_;
			head_=other.head_;
			tail_=other.tail_;
			other.data_=nullptr;
			other.size_=other.head_=other.tail_=0;
		}
		return *this;
	}

	void push_back(const _Tp& value) {
		if (size_==_Capacity) throw std::runtime_error("ring_buffer is full");
		new (&data_[tail_]) _Tp(value);
		tail_=(tail_+1)%Capacity;
		size_++;
	}
	void push_back(_Tp&& value) {
		if (size_==_Capacity) throw std::runtime_error("ring_buffer is full");
		new (&data_[tail_]) _Tp(std::move(value));
		tail_=(tail_+1)%Capacity;
		size_++;
	}
	template <typename... _Args>
	void emplace_back(_Args&&... args) {
		if (size_==_Capacity) throw std::runtime_error("ring_buffer is full");
		new (&data_[tail_]) _Tp(std::forward<Args>(args)...);
		tail_=(tail_+1)%Capacity;
		size_++;
	}
	void pop_front() {
		if (empty()) throw std::runtime_error("ring_buffer is empty");
		data_[head_].~_Tp();
		head_=(head_+1)%Capacity;
		size_--;
	}
	_Tp& front() {
		if (empty()) throw std::runtime_error("ring_buffer is empty");
		return data_[head_];
	}
	const _Tp& front() const {
		if (empty()) throw std::runtime_error("ring_buffer is empty");
		return data_[head_];
	}
	_Tp& back() {
		if (empty()) throw std::runtime_error("ring_buffer is empty");
		return data_[(tail_+_Capacity-1)%_Capacity];
	}
	const _Tp& back() const {
		if (empty()) throw std::runtime_error("ring_buffer is empty");
		return data_[(tail_+_Capacity-1)%_Capacity];
	}
	bool empty() const noexcept { return size_==0; }
	bool full() const noexcept { return size_==_Capacity; }
	std::size_t size() const noexcept { return size_; }
	static constexpr std::size_t capacity() noexcept { return _Capacity; }
	void clear() noexcept {
		while (!empty()) pop_front();
	}
	iterator begin() noexcept { 
		return empty()?iterator():iterator(&data_[head_],data_,data_+_Capacity);
	}
	iterator end() noexcept { 
		return empty()?iterator():iterator(&data_[tail_],data_,data_+_Capacity);
	}
	void swap(ring_buffer& other) noexcept {
		std::swap(data_,other.data_);
		std::swap(size_,other.size_);
		std::swap(head_,other.head_);
		std::swap(tail_,other.tail_);
	}
};

}

}

#endif