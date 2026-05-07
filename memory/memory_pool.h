//Last Modified At 2026/05/08
//@Version 1.0.0.0
#ifndef _STDEX_MEMORY_MEMORY_POOL_H_
#define _STDEX_MEMORY_MEMORY_POOL_H_ 1

#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <mutex>
#include <memory>
#include <new>
#include <type_traits>
#include <utility>

#include "scope_guard.h"

#if __has_include("../macros/cpp_version.h")
#include "../macros/cpp_version.h"//At Least 1.0
#endif

#ifndef _STDEX_CPP20_VERSION
#define _STDEX_CPP20_VERSION 202002L
#endif

#if __cplusplus>=_STDEX_CPP20_VERSION
#include <concepts>
#include <span>
#endif

namespace stdex {

namespace memory {

namespace pool {

	struct null_mutex {
		void lock() noexcept { }
		void unlock() noexcept { }
		bool try_lock() noexcept { return true; }
	};

	inline constexpr std::size_t align_up(std::size_t n,std::size_t align) noexcept {
		return (n+align-1u)&~(align-1u);
	}

	inline constexpr std::size_t min_block_size(std::size_t obj_size,std::size_t obj_align) noexcept {
		std::size_t sz=(obj_size>sizeof(void*))?obj_size:sizeof(void*);
		std::size_t al=(obj_align>alignof(void*))?obj_align:alignof(void*);
		return align_up(sz, al);
	}

	struct free_node {
		free_node* next;
	};

	struct chunk_header {
		chunk_header* next;
		std::size_t capacity;
	};

	inline char* chunk_data_ptr(chunk_header* c,std::size_t header_sz) noexcept {
		return reinterpret_cast<char*>(c)+header_sz;
	}

	template <typename _Tp,typename=void>
	struct is_pool_like : std::false_type {};
	template <typename _Tp>
	struct is_pool_like<_Tp,std::void_t<std::enable_if_t<std::is_same_v<decltype(std::declval<_Tp&>().allocate(std::declval<std::size_t>())),void*>>,std::enable_if_t<std::is_same_v<decltype(std::declval<_Tp&>().deallocate(std::declval<void*>(),std::declval<std::size_t>())),void>>,std::enable_if_t<std::is_same_v<decltype(std::declval<_Tp&>().reset()),void>>,std::enable_if_t<std::is_same_v<decltype(std::declval<_Tp&>().clear()),void>>>> : std::true_type {};

	template <typename _Tp>
	inline constexpr bool is_pool_like_v=is_pool_like<_Tp>::value;

	template <typename _Tp>
	using require_pool_like=std::enable_if_t<is_pool_like_v<_Tp>>;

}

template <typename _Tp,std::size_t _ChunkObjects=64,typename _Mutex=pool::null_mutex>
class fixed_pool {
public:
	using value_type=_Tp;
	using pointer=_Tp*;
	using const_pointer=const _Tp*;
	using size_type=std::size_t;
	using difference_type=std::ptrdiff_t;
	using mutex_type=_Mutex;

	static constexpr std::size_t chunk_objects=_ChunkObjects;
	static constexpr std::size_t block_align=(alignof(_Tp)>alignof(void*))?alignof(_Tp):alignof(void*);
	static constexpr std::size_t block_size=pool::min_block_size(sizeof(_Tp),alignof(_Tp));

private:
	mutable _Mutex mutex_;
	pool::free_node* free_list_;
	pool::chunk_header* chunk_list_;
	size_type total_capacity_;
	size_type total_allocated_;

	static constexpr std::size_t header_size_=pool::align_up(sizeof(pool::chunk_header),block_align);

	pointer allocate_impl() {
		if (!free_list_) allocate_chunk(_ChunkObjects);
		pool::free_node* node=free_list_;
		free_list_=node->next;
		total_allocated_++;
		return reinterpret_cast<pointer>(node);
	}

	void deallocate_impl(pointer p) noexcept {
		pool::free_node* node=reinterpret_cast<pool::free_node*>(p);
		node->next_=free_list_;
		free_list_=node;
		total_allocated_--;
	}

	void allocate_chunk(size_type n) {
		size_type count=(n>_ChunkObjects)?n:_ChunkObjects;
		std::size_t total_sz=header_size_+count*block_size;
		void* raw=operator new(total_sz,std::align_val_t{block_align});
		pool::chunk_header* c=reinterpret_cast<pool::chunk_header*>(raw);
		c->next=chunk_list_;
		c->capacity=count;
		chunk_list_=c;
		total_capacity_+=count;
		char* data=pool::chunk_data_ptr(c,header_size_);
		for (size_type i=0; i<count; ++i) {
			pool::free_node* node=reinterpret_cast<pool::free_node*>(data+i*block_size);
			node->next=free_list_;
			free_list_=node;
		}
	}

	void clear_impl() noexcept {
		pool::chunk_header* c=chunk_list_;
		while (c) {
			pool::chunk_header* next=c->next;
			operator delete(c,std::align_val_t{block_align});
			c=next;
		}
		chunk_list_=nullptr;
		free_list_=nullptr;
		total_capacity_=0;
		total_allocated_=0;
	}

	void reset_impl() noexcept {
		free_list_=nullptr;
		pool::chunk_header* c=chunk_list_;
		while (c) {
			char* data=pool::chunk_data_ptr(c,header_size_);
			for (size_type i=0; i<c->capacity;i++) {
				pool::free_node* node=reinterpret_cast<pool::free_node*>(data+i*block_size);
				node->next=free_list_;
				free_list_=node;
			}
			c=c->next;
		}
		total_allocated_=0;
	}

public:
	fixed_pool() noexcept : free_list_(nullptr) , chunk_list_(nullptr) , total_capacity_(0) , total_allocated_(0) { }
	~fixed_pool() noexcept {
		clear();
	}
	fixed_pool(const fixed_pool&)=delete;
	fixed_pool(fixed_pool&& other) noexcept : free_list_(other.free_list_) , chunk_list_(other.chunk_list_) , total_capacity_(other.total_capacity_) , total_allocated_(other.total_allocated_) {
		other.free_list_=nullptr;
		other.chunk_list_=nullptr;
		other.total_capacity_=0;
		other.total_allocated_=0;
	}
	fixed_pool& operator =(const fixed_pool&)=delete;
	fixed_pool& operator =(fixed_pool&& other) noexcept {
		if (this!=&other) {
			clear();
			free_list_=other.free_list_;
			chunk_list_=other.chunk_list_;
			total_capacity_=other.total_capacity_;
			total_allocated_=other.total_allocated_;
			other.free_list_=nullptr;
			other.chunk_list_=nullptr;
			other.total_capacity_=0;
			other.total_allocated_=0;
		}
		return *this;
	}

	struct pool_deleter {
		fixed_pool* pool;
		void operator ()(pointer p) const noexcept {
			if (pool && p) pool->destroy(p);
		}
	};

	[[nodiscard]]
	pointer allocate() {
		std::lock_guard<_Mutex> lk(mutex_);
		return allocate_impl();
	}

	void deallocate(pointer p) noexcept {
		if (!p) return;
		std::lock_guard<_Mutex> lk(mutex_);
		deallocate_impl(p);
	}

	template <typename... _Args>
	[[nodiscard]]
	pointer construct(_Args&&... args) {
		pointer p=allocate();
		::new(static_cast<void*>(p)) _Tp(std::forward<_Args>(args)...);
		return p;
	}

	void destroy(pointer p) noexcept {
		if (!p) return;
		p->~_Tp();
		deallocate(p);
	}

	template <typename... _Args>
	[[nodiscard]]
	std::unique_ptr<_Tp,pool_deleter> make(_Args&&... args) {
		pointer p=construct(std::forward<_Args>(args)...);
		return std::unique_ptr<_Tp,pool_deleter>(p,pool_deleter{this});
	}

	template <typename... _Args>
	[[nodiscard]]
	std::shared_ptr<_Tp> make_shared(_Args&&... args) {
		pointer p=construct(std::forward<_Args>(args)...);
		return std::shared_ptr<_Tp>(p,pool_deleter{this});
	}

	void reserve(size_type n) {
		std::lock_guard<_Mutex> lk(mutex_);
		if (n>total_capacity_) allocate_chunk(n-total_capacity_);
	}

	void reset() noexcept {
		std::lock_guard<_Mutex> lk(mutex_);
		reset_impl();
	}

	void clear() noexcept {
		std::lock_guard<_Mutex> lk(mutex_);
		clear_impl();
	}

	[[nodiscard]]
	size_type size() const noexcept {
		std::lock_guard<_Mutex> lk(mutex_);
		return total_allocated_;
	}

	[[nodiscard]]
	size_type capacity() const noexcept {
		std::lock_guard<_Mutex> lk(mutex_);
		return total_capacity_;
	}

	[[nodiscard]]
	bool empty() const noexcept {
		std::lock_guard<_Mutex> lk(mutex_);
		return total_allocated_==0;
	}

	[[nodiscard]]
	static constexpr size_type max_size() noexcept {
		return std::numeric_limits<size_type>::max()/block_size;
	}

#if __cplusplus>=_STDEX_CPP20_VERSION
	[[nodiscard]]
	std::size_t allocate_n(std::span<pointer> result) {
		std::lock_guard<_Mutex> lk(mutex_);
		std::size_t count=0;
		for (auto& it:result) {
			it=allocate_impl();
			count++;
		}
		return count;
	}

	void deallocate_n(std::span<pointer> ptrs) noexcept {
		std::lock_guard<_Mutex> lk(mutex_);
		for (auto it:ptrs) deallocate_impl(it);
	}
#endif
};

template <std::size_t _ChunkSize=4096,typename _Mutex=pool::null_mutex>
class memory_pool {
public:
	using size_type=std::size_t;
	using difference_type=std::ptrdiff_t;
	using mutex_type=_Mutex;

	static constexpr std::size_t alignment=alignof(std::max_align_t);
	static constexpr std::size_t max_small_size=512;
	static constexpr std::size_t bucket_count=max_small_size/alignment;
	static constexpr std::size_t chunk_size=_ChunkSize;

private:
	mutable _Mutex mutex_;

	struct chunk_node {
		chunk_node* next;
		char* data;
		std::size_t capacity;
		std::size_t used;
	};

	std::array<pool::free_node*, bucket_count> buckets_;
	std::array<chunk_node*, bucket_count> chunks_;
	std::array<std::size_t, bucket_count> chunk_cap_;

	static constexpr std::size_t chunk_node_header_size_=pool::align_up(sizeof(chunk_node),alignment);

	static std::ptrdiff_t bucket_index(size_type n) noexcept {
		size_type rounded=pool::align_up(n,alignment);
		if (rounded>max_small_size) return -1;
		return static_cast<std::ptrdiff_t>(rounded/alignment)-1;
	}

	void* allocate_impl(size_type n) {
		std::ptrdiff_t idx=bucket_index(n);
		if (idx<0) return operator new(n);
		if (buckets_[idx]) {
			pool::free_node* node=buckets_[idx];
			buckets_[idx]=node->next;
			return static_cast<void*>(node);
		}
		return alloc_from_chunk(static_cast<std::size_t>(idx));
	}

	void deallocate_impl(void* p,size_type n) noexcept {
		std::ptrdiff_t idx=bucket_index(n);
		if (idx<0) {
			operator delete(p);
			return;
		}
		pool::free_node* node=reinterpret_cast<pool::free_node*>(p);
		node->next=buckets_[idx];
		buckets_[idx]=node;
	}

	void* alloc_from_chunk(std::size_t idx) {
		std::size_t block_sz=alignment*(idx+1);
		chunk_node* c=chunks_[idx];
		if (!c || c->used>=c->capacity) {
			std::size_t data_sz=(chunk_size>=block_sz)?chunk_size:block_sz*8;
			std::size_t cap=data_sz/block_sz;
			if (cap==0) cap=1;
			std::size_t total_sz=chunk_node_header_size_+cap*block_sz;
			void* raw=operator new(total_sz,std::align_val_t{alignment});
			chunk_node* nc=reinterpret_cast<chunk_node*>(raw);
			nc->next=chunks_[idx];
			nc->data=reinterpret_cast<char*>(raw)+chunk_node_header_size_;
			nc->capacity=cap;
			nc->used=0;
			chunks_[idx] =nc;
			chunk_cap_[idx]+=cap;
			c=nc;
		}
		void* p=c->data+c->used*block_sz;
		c->used++;
		return p;
	}

	void clear_impl() noexcept {
		for (std::size_t i=0;i<bucket_count;i++) {
			chunk_node* c=chunks_[i];
			while (c) {
				chunk_node* next=c->next;
				operator delete(c,std::align_val_t{alignment});
				c=next;
			}
			chunks_[i]=nullptr;
			buckets_[i]=nullptr;
			chunk_cap_[i]=0;
		}
	}

	void reset_impl() noexcept {
		for (std::size_t i=0;i<bucket_count;i++) {
			buckets_[i]=nullptr;
			chunk_node* c=chunks_[i];
			while (c) {
				c->used=0;
				c=c->next;
			}
		}
	}

	void move_from(memory_pool& other) noexcept {
		buckets_=other.buckets_;
		chunks_=other.chunks_;
		chunk_cap_=other.chunk_cap_;
		other.buckets_.fill(nullptr);
		other.chunks_.fill(nullptr);
		other.chunk_cap_.fill(0);
	}

public:
	memory_pool() noexcept {
		buckets_.fill(nullptr);
		chunks_.fill(nullptr);
		chunk_cap_.fill(0);
	}
	~memory_pool() noexcept {
		clear();
	}
	memory_pool(const memory_pool&)=delete;
	memory_pool(memory_pool&& other) noexcept {
		move_from(other);
	}
	memory_pool& operator =(const memory_pool&)=delete;
	memory_pool& operator =(memory_pool&& other) noexcept {
		if (this!=&other) {
			clear();
			move_from(other);
		}
		return *this;
	}

	[[nodiscard]]
	void* allocate(size_type n) {
		if (n==0) n=1;
		std::lock_guard<_Mutex> lk(mutex_);
		return allocate_impl(n);
	}

	void deallocate(void* p,size_type n) noexcept {
		if (!p) return;
		if (n==0) n=1;
		std::lock_guard<_Mutex> lk(mutex_);
		deallocate_impl(p, n);
	}

	[[nodiscard]]
	void* allocate_aligned(size_type n,size_type align) {
		if (align<=alignment) return allocate(n);
		return operator new(n, std::align_val_t{align});
	}

	void deallocate_aligned(void* p,size_type n,size_type align) noexcept {
		if (!p) return;
		if (align<=alignment) {
			deallocate(p,n);
			return;
		}
		operator delete(p,std::align_val_t{align});
	}

	void reset() noexcept {
		std::lock_guard<_Mutex> lk(mutex_);
		reset_impl();
	}

	void clear() noexcept {
		std::lock_guard<_Mutex> lk(mutex_);
		clear_impl();
	}

	[[nodiscard]]
	size_type capacity_bytes() const noexcept {
		std::lock_guard<_Mutex> lk(mutex_);
		size_type total=0;
		for (std::size_t i=0;i<bucket_count;i++) total+=chunk_cap_[i]*(alignment*(i+1));
		return total;
	}

	[[nodiscard]]
	static constexpr size_type max_pooled_size() noexcept {
		return max_small_size;
	}

#if __cplusplus>=_STDEX_CPP20_VERSION
	[[nodiscard]]
	std::size_t allocate_n(std::span<void*> result,size_type n) {
		std::lock_guard<_Mutex> lk(mutex_);
		std::size_t count=0;
		for (auto& it:result) {
			it=allocate_impl(n);
			count++;
		}
		return count;
	}

	void deallocate_n(std::span<void*> ptrs,size_type n) noexcept {
		std::lock_guard<_Mutex> lk(mutex_);
		for (auto it:ptrs) deallocate_impl(it,n);
	}
#endif
};


template <typename _Tp,typename _Pool>
class pool_allocator {
public:
	using value_type=_Tp;
	using pointer=_Tp*;
	using const_pointer=const _Tp*;
	using size_type=std::size_t;
	using difference_type=std::ptrdiff_t;
	using pool_type=_Pool;

	using propagate_on_container_copy_assignment=std::false_type;
	using propagate_on_container_move_assignment=std::true_type;
	using propagate_on_container_swap=std::true_type;
	using is_always_equal=std::false_type;

	_Pool* pool;

	template <typename _Up>
	struct rebind {
		using other=pool_allocator<_Up,_Pool>;
	};

	explicit pool_allocator(_Pool& p) noexcept : pool(&p) { }
	template <typename _Up>
	pool_allocator(const pool_allocator<_Up, _Pool>& other) noexcept : pool(other.pool) { }

	[[nodiscard]]
	pointer allocate(size_type n) {
		return reinterpret_cast<pointer>(pool->allocate(n*sizeof(_Tp)));
	}

	void deallocate(pointer p,size_type n) noexcept {
		pool->deallocate(static_cast<void*>(p),n*sizeof(_Tp));
	}

	[[nodiscard]]
	static constexpr size_type max_size() noexcept {
		return std::numeric_limits<size_type>::max()/sizeof(_Tp);
	}

	template <typename _Up,typename... _Args>
	void construct(_Up* p,_Args&&... args) {
		::new(static_cast<void*>(p)) _Up(std::forward<_Args>(args)...);
	}

	template <typename _Up>
	void destroy(_Up* p) noexcept {
		p->~_Up();
	}

	[[nodiscard]]
	bool operator ==(const pool_allocator& other) const noexcept {
		return pool==other.pool;
	}

	[[nodiscard]]
	bool operator !=(const pool_allocator& other) const noexcept {
		return !(*this==other);
	}
};

template <typename _Tp,std::size_t _ChunkObjects,typename _Mutex>
class pool_allocator<_Tp,fixed_pool<_Tp,_ChunkObjects,_Mutex>> {
public:
	using pool_type=fixed_pool<_Tp,_ChunkObjects,_Mutex>;
	using value_type=_Tp;
	using pointer=_Tp*;
	using const_pointer=const _Tp*;
	using size_type=std::size_t;
	using difference_type=std::ptrdiff_t;

	using propagate_on_container_copy_assignment=std::false_type;
	using propagate_on_container_move_assignment=std::true_type;
	using propagate_on_container_swap=std::true_type;
	using is_always_equal=std::false_type;

	pool_type* pool;

	template <typename _Up>
	struct rebind {
		using other=pool_allocator<_Up,pool_type>;
	};
	explicit pool_allocator(pool_type& p) noexcept : pool(&p) { }

	[[nodiscard]]
	pointer allocate(size_type n) {
		if (n!=1) throw std::bad_alloc{};
		return pool->allocate();
	}

	void deallocate(pointer p,size_type) noexcept {
		pool->deallocate(p);
	}

	[[nodiscard]]
	static constexpr size_type max_size() noexcept {
		return 1;
	}

	template <typename _Up,typename... _Args>
	void construct(_Up* p,_Args&&... args) {
		::new(static_cast<void*>(p)) _Up(std::forward<_Args>(args)...);
	}

	template <typename _Up>
	void destroy(_Up* p) noexcept {
		p->~_Up();
	}

	[[nodiscard]]
	bool operator ==(const pool_allocator& other) const noexcept {
		return pool==other.pool;
	}

	[[nodiscard]] bool operator !=(const pool_allocator& other) const noexcept {
		return !(*this==other);
	}
};

template <typename _Pool>
class scoped_pool_resource {
	static auto make_guard_func_(_Pool* p,bool do_clear) noexcept {
		return [p,do_clear]() noexcept {
			if (do_clear) p->clear();
			else p->reset();
		};
	}

	using guard_func_=decltype(make_guard_func_(std::declval<_Pool*>(),bool{}));

	_Pool* pool_;
	scope_guard<guard_func_> guard_;

public:
	explicit scoped_pool_resource(_Pool& p,bool do_clear=false) noexcept : pool_(&p) , guard_(make_guard_func_(&p,do_clear)) { }
	~scoped_pool_resource() noexcept=default;

	scoped_pool_resource(const scoped_pool_resource&)=delete;
	scoped_pool_resource(scoped_pool_resource&&) noexcept=default;

	scoped_pool_resource& operator =(const scoped_pool_resource&)=delete;
	scoped_pool_resource& operator =(scoped_pool_resource&&)=delete;

	void release() noexcept {
		guard_.release();
	}

	[[nodiscard]]
	_Pool& pool() noexcept {
		return *pool_;
	}

	[[nodiscard]]
	const _Pool& pool() const noexcept {
		return *pool_;
	}
};

template <typename _Pool>
scoped_pool_resource(_Pool&)->scoped_pool_resource<_Pool>;

template <typename _Pool>
scoped_pool_resource(_Pool&,bool)->scoped_pool_resource<_Pool>;

template <typename _Tp,typename _Pool>
[[nodiscard]]
auto make_pool_allocator(_Pool& p) noexcept {
	return pool_allocator<_Tp,_Pool>(p);
}

template <typename _Tp,std::size_t _ChunkObjects=64>
using concurrent_fixed_pool=fixed_pool<_Tp,_ChunkObjects,std::mutex>;

template <std::size_t _ChunkSize=4096>
using concurrent_memory_pool=memory_pool<_ChunkSize,std::mutex>;

template <typename _Pool,typename=pool::require_pool_like<_Pool>>
[[nodiscard]]
auto make_scoped_pool_resource(_Pool& p,bool do_clear=false) noexcept {
	return scoped_pool_resource<_Pool>(p,do_clear);
}

template <typename _Tp,typename _Pool,typename=pool::require_pool_like<_Pool>>
[[nodiscard]]
auto make_pool_allocator(_Pool& p) noexcept {
	return pool_allocator<_Tp,_Pool>(p);
}

#if __cplusplus>=_STDEX_CPP20_VERSION
template <typename _Tp>
concept pool_like=pool::is_pool_like_v<_Tp>;
#endif

}

}

#endif