//Last Modified At 2025/10/15
//@Version 1.0.0.0
#ifndef _STDEX_STRUCTURE_MONO_QUEUE_H_
#define _STDEX_STRUCTURE_MONO_QUEUE_H_ 1

#include <cstddef>
#include <deque>
#include <functional>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace stdex {

namespace structure {

template <typename _Tp,typename _Compare=std::less<_Tp>>
class mono_queue {
public:
	using value_type=_Tp;
	using size_type=std::size_t;
	using reference=value_type&;
	using const_reference=const value_type&;
	using compare_type=_Compare;

private:
	struct element {
		value_type value_;
		size_type index_;
		element(value_type&& val,size_type idx) : value_(std::move(val)) , index_(idx) { }
		element(const value_type& val,size_type idx) : value_(val) , index_(idx) { }
	};
	std::deque<element> data_;
	compare_type comp_;
	size_type count_=0;
	size_type window_size_=0;
	bool fixed_window_=false;

	template <typename _ValueT>
	void push_impl(_ValueT&& value) {
		if (fixed_window_ && count_>=window_size_) pop();
		size_type current_index=count_++;
		while (!data_.empty() && comp_(data_.back().value,value)) data_.pop_back();
		data_.emplace_back(std::forward<_ValueT>(value),current_index);
	}

public:
	mono_queue()=default;
	explicit mono_queue(const compare_type& comp) : comp_(comp) { }
	explicit mono_queue(size_type window_size,const compare_type& comp=compare_type()) : comp_(comp), window_size_(window_size), fixed_window_(window_size>0) { }

	void push(const value_type& value) {
		push_impl(value);
	}
	void push(value_type&& value) {
		push_impl(std::move(value));
	}
	template <typename... _Args>
	void emplace(_Args&&... args) {
		push_impl(value_type(std::forward<_Args>(args)...));
	}
	void pop() {
		if (empty()) throw std::runtime_error("mono_queue::pop: queue is empty");
		if (!data_.empty() && data_.front().index==count_-size()) data_.pop_front();
		count_++;
	}
	const value_type& front() const {
		if (empty()) throw std::runtime_error("mono_queue::front: queue is empty");
		return data_.front().value;
	}
	const value_type& back() const {
		if (empty()) throw std::runtime_error("mono_queue::back: queue is empty");
		return data_.back().value;
	}
	const value_type& max() const {
		if (empty()) throw std::runtime_error("mono_queue::max: queue is empty");
		return data_.front().value;
	}
	const value_type& min() const {
		if (empty()) throw std::runtime_error("mono_queue::min: queue is empty");
		return data_.back().value;
	}
	bool empty() const noexcept {
		return data_.empty();
	}
	size_type size() const noexcept {
		if (fixed_window_) return std::min(window_size_, count_);
		return count_;
	}
	size_type window_size() const noexcept {
		return window_size_;
	}
	void set_window_size(size_type new_size) {
		window_size_=new_size;
		fixed_window_=(new_size>0);
		if (fixed_window_ && count_>window_size_) {
			size_type remove_count=count_-window_size_;
			for (size_type i=0;i<remove_count && !data_.empty();i++) {
				if (data_.front().index==count_-size()+i) data_.pop_front();
			}
		}
	}
	void clear() noexcept {
		data_.clear();
		count_=0;
	}
	void swap(mono_queue& other) noexcept {
		data_.swap(other.data_);
		std::swap(comp_,other.comp_);
		std::swap(count_,other.count_);
		std::swap(window_size_,other.window_size_);
		std::swap(fixed_window_,other.fixed_window_);
	}
};

template <typename _Tp,typename _Compare>
void swap(mono_queue<_Tp,_Compare>& lhs,mono_queue<_Tp,_Compare>& rhs) noexcept {
	lhs.swap(rhs);
}

}

}

#endif