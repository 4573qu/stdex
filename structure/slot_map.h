//Last Modified At 2026/03/16
//@Version 1.1.0.0
#ifndef _STDEX_STRUCTURE_SLOT_MAP_H_
#define _STDEX_STRUCTURE_SLOT_MAP_H_ 1

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iterator>
#include <memory>
#include <new>
#include <stdexcept>
#include <type_traits>
#include <utility>

#if __has_include("../macros/cpp_version.h")
#include "../macros/cpp_version.h"//At Least 1.0
#endif

#ifndef _STDEX_CPP20_VERSION
#define _STDEX_CPP20_VERSION 202002L
#endif

#if __cplusplus>=_STDEX_CPP20_VERSION
	#include <concepts>
#endif

namespace stdex {

namespace structure {

inline constexpr int slot_map_index_mask=65535;
inline constexpr int slot_map_key_mask=-65536;
inline constexpr int slot_map_key_shift=16;
inline constexpr int slot_map_max_size=65536;
inline constexpr int slot_map_key_first=1;


#if __cplusplus>=_STDEX_CPP20_VERSION
template <typename _Tp>
concept unsigned_int_like=requires(_Tp t) {
	{ static_cast<unsigned int>(t) }->std::same_as<unsigned int>;
};

template <typename _Tp>
concept exactly_unsigned_int=std::same_as<_Tp,unsigned int>;

template <typename _Tp>
concept handle_id_type=unsigned_int_like<_Tp> || exactly_unsigned_int<_Tp>;
#else
template <typename...>
using void_t=void;

template <typename _Tp,typename=void>
struct is_unsigned_int_like : std::false_type {
};

template <typename _Tp>
struct is_unsigned_int_like<_Tp,void_t<decltype(static_cast<unsigned int>(std::declval<_Tp>()))>> : std::integral_constant<bool,std::is_same<decltype(static_cast<unsigned int>(std::declval<_Tp>())),unsigned int>::value> { };

template <typename _Tp>
struct is_exactly_unsigned_int : std::is_same<_Tp,unsigned int> { };

template <typename _Tp>
struct is_handle_id_type : std::integral_constant<bool,is_unsigned_int_like<_Tp>::value || is_exactly_unsigned_int<_Tp>::value> { };
#endif

#if __cplusplus>=_STDEX_CPP20_VERSION
template <typename _Tp,typename _IDTp=unsigned int,int _IndexMask=slot_map_index_mask,int _KeyMask=slot_map_key_mask,int _KeyShift=slot_map_key_shift,int _MaxSize=slot_map_max_size,int _KeyFirst=slot_map_key_first> requires handle_id_type<_IDTp>
class slot_map {
#else
template <typename _Tp,typename _IDTp=unsigned int,int _IndexMask=slot_map_index_mask,int _KeyMask=slot_map_key_mask,int _KeyShift=slot_map_key_shift,int _MaxSize=slot_map_max_size,int _KeyFirst=slot_map_key_first,typename _Enable=void>
class slot_map;

template <typename _Tp,typename _IDTp,int _IndexMask,int _KeyMask,int _KeyShift,int _MaxSize,int _KeyFirst>
class slot_map<_Tp,_IDTp,_IndexMask,_KeyMask,_KeyShift,_MaxSize,_KeyFirst,typename std::enable_if<is_handle_id_type<_IDTp>::value>::type> {
#endif
public:
	using value_type=_Tp;
	using id_type=_IDTp;
	using size_type=unsigned int;
	using difference_type=std::ptrdiff_t;
	using reference=_Tp&;
	using const_reference=const _Tp&;
	using pointer=_Tp*;
	using const_pointer=const _Tp*;

protected:
	struct slot_type {
		alignas(_Tp) unsigned char storage[sizeof(_Tp)];
		_IDTp id;
		bool constructed;
		pointer ptr() noexcept {
			return std::launder(reinterpret_cast<pointer>(storage));
		}
		const_pointer ptr() const noexcept {
			return std::launder(reinterpret_cast<const_pointer>(storage));
		}
	};

	slot_type* block_;
	size_type max_used_count_;
	size_type max_size_;
	size_type free_list_head_;
	size_type size_;
	size_type next_key_;

	size_type index_of_ptr(const_pointer item) const noexcept {
		if (!block_ || !item) return static_cast<size_type>(-1);
		for (size_type i=0;i<max_used_count_;i++) {
			if (block_[i].constructed && block_[i].ptr()==item) return i;
		}
		return static_cast<size_type>(-1);
	}
	void clear_member() {
		max_used_count_=0;
		max_size_=0;
		free_list_head_=0;
		size_=0;
		next_key_=_KeyFirst;
	}

public:
	class iterator {
	public:
		using iterator_category=std::forward_iterator_tag;
		using value_type=_Tp;
		using difference_type=std::ptrdiff_t;
		using pointer=_Tp*;
		using reference=_Tp&;

	private:
		friend class slot_map;
		const slot_map* owner_;
		size_type index_;
		iterator(slot_map* owner, size_type index) noexcept : owner_(owner) , index_(index) {
			skip_invalid();
		}
		iterator(const slot_map* owner, size_type index) noexcept : owner_(owner) , index_(index) {
			skip_invalid();
		}
		void skip_invalid() noexcept {
			if (!owner_) return;
			while (index_<owner_->max_used_count_) {
				const slot_type& slot=owner_->block_[index_];
				if ((static_cast<unsigned int>(slot.id)&_KeyMask)!=0 && slot.constructed) break;
				index_++;
			}
		}

	public:
		iterator() noexcept : owner_(nullptr) , index_(0) { }

		iterator& operator ++() noexcept {
			index_++;
			skip_invalid();
			return *this;
		}
		iterator operator ++(int) noexcept {
			iterator temp(*this);
			++(*this);
			return temp;
		}
		iterator& operator --()=delete;
		iterator operator --(int)=delete;
		bool operator ==(const iterator& other) const noexcept {
			return owner_==other.owner_ && index_==other.index_;
		}
		bool operator !=(const iterator& other) const noexcept {
			return !(*this==other);
		}
		reference operator *() const noexcept {
			return *const_cast<pointer>(owner_->block_[index_].ptr());
		}

		pointer operator ->() const noexcept {
			return const_cast<pointer>(owner_->block_[index_].ptr());
		}
	
	};
	using const_iterator=iterator;

	slot_map() noexcept : block_(nullptr) , max_used_count_(0) , max_size_(0) , free_list_head_(0) , size_(0) , next_key_(_KeyFirst) { }
	explicit slot_map(size_type capacity) : slot_map() {
		initialize(capacity);
	}
	~slot_map() noexcept (std::is_nothrow_destructible<_Tp>::value) {
		dispose();
	}

	slot_map(const slot_map&)=delete;
	slot_map(slot_map&& other) noexcept : block_(other.block_) , max_used_count_(other.max_used_count_) , max_size_(other.max_size_) , free_list_head_(other.free_list_head_) , size_(other.size_) , next_key_(other.next_key_) {
		other.block_=nullptr;
		other.max_used_count_=0;
		other.max_size_=0;
		other.free_list_head_=0;
		other.size_=0;
		other.next_key_=_KeyFirst;
	}

	slot_map& operator =(const slot_map&)=delete;
	slot_map& operator =(slot_map&& other) noexcept {
		if (this!=&other) {
			dispose();
			block_=other.block_;
			max_used_count_=other.max_used_count_;
			max_size_=other.max_size_;
			free_list_head_=other.free_list_head_;
			size_=other.size_;
			next_key_=other.next_key_;
			other.block_=nullptr;
			other.clear_member();
		}
		return *this;
	}

	void initialize(size_type capacity) {
		if (block_) throw std::logic_error("Block already initialized");
		if (capacity==0) throw std::invalid_argument("Capacity must be greater than zero");
		if (capacity>_MaxSize) throw std::length_error("Capacity exceeds maximum slot count");
		block_=static_cast<slot_type*>(::operator new[](sizeof(slot_type)*capacity));
		for (size_type i=0;i<capacity;i++) {
			block_[i].id=static_cast<_IDTp>(0);
			block_[i].constructed=false;
		}
		clear_member();
		max_size_=capacity;
	}
	void dispose() noexcept (std::is_nothrow_destructible<_Tp>::value) {
		if (!block_) return;
		clear();
		::operator delete[](block_);
		block_=nullptr;
		clear_member();
	}

	template <typename... Args>
	pointer emplace(Args&&... args) {
		if (!block_) throw std::logic_error("Container is not initialized");
		if (full()) throw std::length_error("Container is full");
		size_type next=max_used_count_;
		if (free_list_head_==max_used_count_) free_list_head_=++max_used_count_;
		else {
			next=free_list_head_;
			free_list_head_=static_cast<size_type>(block_[free_list_head_].id);
		}
		slot_type& slot=block_[next];
		slot.id=static_cast<_IDTp>((next_key_<<_KeyShift)|next);
		slot.constructed=false;
		next_key_++;
		if (next_key_==_MaxSize) next_key_=_KeyFirst;
		std::construct_at(slot.ptr(),std::forward<Args>(args)...);
		slot.constructed=true;
		size_++;
		return slot.ptr();
	}

	pointer insert(const _Tp& value) {
		return emplace(value);
	}
	pointer insert(_Tp&& value) {
		return emplace(std::move(value));
	}

	void erase(pointer item) {
		if (!block_) throw std::logic_error("Container is not initialized");
		const size_type index=index_of_ptr(item);
		if (index==static_cast<size_type>(-1)) throw std::invalid_argument("Invalid pointer");
		slot_type& slot=block_[index];
		std::destroy_at(slot.ptr());
		slot.constructed=false;
		slot.id=static_cast<_IDTp>(free_list_head_);
		free_list_head_=index;
		size_--;
	}
	void erase(id_type id) {
		if (!block_) throw std::logic_error("Container is not initialized");
		const unsigned int value=static_cast<unsigned int>(id);
		if (!value) throw std::out_of_range("Invalid id");
		const size_type index=value&_IndexMask;
		if (index>=max_size_) throw std::out_of_range("Invalid id");
		slot_type& slot=block_[index];
		if (slot.id!=id || !slot.constructed) throw std::out_of_range("Invalid id");
		std::destroy_at(slot.ptr());
		slot.constructed=false;
		slot.id=static_cast<_IDTp>(free_list_head_);
		free_list_head_=index;
		size_--;
	}

	void clear() noexcept (std::is_nothrow_destructible<_Tp>::value) {
		if (!block_) return;
		for (size_type i=0;i<max_used_count_;i++) {
			slot_type& slot=block_[i];
			if ((static_cast<unsigned int>(slot.id)&_KeyMask)!=0) {
				if (slot.constructed) std::destroy_at(slot.ptr());
				slot.constructed=false;
				slot.id=static_cast<_IDTp>(0);
			}
		}
		free_list_head_=0;
		max_used_count_=0;
		size_=0;
	}

	pointer get(id_type id) {
		pointer result=try_get(id);
		if (!result) throw std::out_of_range("Invalid id");
		return result;
	}
	const_pointer get(id_type id) const {
		const_pointer result=try_get(id);
		if (!result) throw std::out_of_range("Invalid id");
		return result;
	}

	pointer try_get(id_type id) noexcept {
		if (!block_) return nullptr;
		const unsigned int value=static_cast<unsigned int>(id);
		if (!value) return nullptr;
		const size_type index=value&_IndexMask;
		if (index>=max_size_) return nullptr;
		slot_type& slot=block_[index];
		if (slot.id!=id) return nullptr;
		if (!slot.constructed) return nullptr;
		return slot.ptr();
	}
	const_pointer try_get(id_type id) const noexcept {
		if (!block_) return nullptr;
		const unsigned int value=static_cast<unsigned int>(id);
		if (!value) return nullptr;
		const size_type index=value&_IndexMask;
		if (index>=max_size_) return nullptr;
		const slot_type& slot=block_[index];
		if (slot.id!=id) return nullptr;
		if (!slot.constructed) return nullptr;
		return slot.ptr();
	}

	bool contains(id_type id) const noexcept {
		return try_get(id)!=nullptr;
	}
	id_type id_of(pointer item) const {
		const size_type index=index_of_ptr(item);
#ifndef _STDEX_IGNORE_STRUCTURE_SLOT_MAP_ID_WARNINGS
		if (index==static_cast<size_type>(-1)) return static_cast<id_type>(0);
#else
		if (index==static_cast<size_type>(-1)) throw std::invalid_argument("Invalid pointer");
#endif
		const slot_type& slot=block_[index];
		return slot.id;
	}
#ifndef _STDEX_IGNORE_STRUCTURE_SLOT_MAP_ID_REF_WARNINGS
	[[deprecated("Dangerous: modifying slot_map id by reference may break container invariants")]]
#endif
	id_type& id_ref_of(pointer item) {
		const size_type index=index_of_ptr(item);
#ifndef _STDEX_IGNORE_STRUCTURE_SLOT_MAP_ID_WARNINGS
		if (index==static_cast<size_type>(-1)) return static_cast<id_type>(0);
#else
		if (index==static_cast<size_type>(-1)) throw std::invalid_argument("Invalid pointer");
#endif
		slot_type& slot=block_[index];
		return slot.id;
	}
#ifndef _STDEX_IGNORE_STRUCTURE_SLOT_MAP_ID_REF_WARNINGS
	[[deprecated("Dangerous: modifying slot_map id by reference may break container invariants")]]
#endif
	const id_type& id_ref_of(const_pointer item) const {
		const size_type index=index_of_ptr(item);
#ifndef _STDEX_IGNORE_STRUCTURE_SLOT_MAP_ID_WARNINGS
		if (index==static_cast<size_type>(-1)) return static_cast<id_type>(0);
#else
		if (index==static_cast<size_type>(-1)) throw std::invalid_argument("Invalid pointer");
#endif
		const slot_type& slot=block_[index];
		return slot.id;
	}
	size_type index_of(id_type id) const noexcept {
		return static_cast<unsigned int>(id)&_IndexMask;
	}

	size_type size() const noexcept {
		return size_;
	}
	size_type capacity() const noexcept {
		return max_size_;
	}
	size_type max_used() const noexcept {
		return max_used_count_;
	}
	bool empty() const noexcept {
		return size_==0;
	}
	bool full() const noexcept {
		return size_>=max_size_;
	}

	iterator begin() noexcept {
		return iterator(this,0);
	}
	const_iterator begin() const noexcept {
		return const_iterator(this,0);
	}
	const_iterator cbegin() const noexcept {
		return const_iterator(this,0);
	}
	const_iterator end() const noexcept {
		return const_iterator(this,max_used_count_);
	}
	iterator end() noexcept {
		return iterator(this,max_used_count_);
	}
	const_iterator cend() const noexcept {
		return const_iterator(this,max_used_count_);
	}
};

}

}

#endif