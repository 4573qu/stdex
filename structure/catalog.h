//Last Modified At 2026/05/11
//@Version 1.0.0.0
#ifndef _STDEX_STRUCTURE_CATALOG_H_
#define _STDEX_STRUCTURE_CATALOG_H_ 1

#include <algorithm>
#include <atomic>
#include <cstddef>
#include <functional>
#include <initializer_list>
#include <memory>
#include <optional>
#include <shared_mutex>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

#include "../memory/observer_ptr.h"//At Least 1.0

#if __has_include("../macros/cpp_version.h")
#include "../macros/cpp_version.h"//At Least 1.0
#endif

#ifndef _STDEX_CPP20_VERSION
#define _STDEX_CPP20_VERSION 202002L
#endif

#if __cplusplus>=_STDEX_CPP20_VERSION
#include <ranges>
#endif

namespace stdex {

namespace structure {

template <typename _Tp,bool _Syn>
class catalog_view;

template <typename _Tp=std::string,bool _Syn=false>
class catalog {
public:
	using value_type=_Tp;
	using key_type=std::string;
	using layer_type=std::unordered_map<std::string,_Tp>;
	using size_type=std::size_t;
	using lookup_policy_type=std::function<std::optional<_Tp>(const std::vector<layer_type>&,std::string_view)>;
	using missing_key_handler_type=std::function<_Tp(std::string_view)>;

private:
	struct noop_mutex {
		void lock() noexcept { }
		void unlock() noexcept { }
		void lock_shared() noexcept { }
		void unlock_shared() noexcept { }
	};

	using mutex_type=std::conditional_t<_Syn,std::shared_mutex,noop_mutex>;
	using read_lock=std::shared_lock<mutex_type>;
	using write_lock=std::unique_lock<mutex_type>;

	struct layer_observer_entry {
		size_type id;
		std::function<void(size_type)> callback;
	};

	struct reorder_observer_entry {
		size_type id;
		std::function<void()> callback;
	};

	std::vector<layer_type> layers_;
	std::vector<std::pair<std::string,size_type>> named_layers_;
	lookup_policy_type lookup_policy_;
	missing_key_handler_type missing_key_handler_;
	mutable mutex_type mutex_;
	std::vector<layer_observer_entry> layer_observers_;
	std::vector<reorder_observer_entry> reorder_observers_;
	std::atomic<size_type> next_id_{0};

	static std::optional<_Tp> default_lookup(const std::vector<layer_type>& layers,std::string_view key){
		for (const auto& it:layers) {
			auto jt=it.find(std::string(key));
			if (jt!=it.end()) return jt->second;
		}
		return std::nullopt;
	}

	void notify_layer_changed(size_type idx) {
		for (const auto& it:layer_observers_) {
			if (it.callback) it.callback(idx);
		}
	}

	void notify_reordered() {
		for (const auto& it:reorder_observers_) {
			if (it.callback) it.callback();
		}
	}

	std::optional<size_type> find_layer_index(std::string_view name) const {
		for (const auto& [n,idx]:named_layers_) {
			if (n==name) return idx;
		}
		return std::nullopt;
	}

	size_type next_id() {
		return next_id_.fetch_add(1,std::memory_order_relaxed);
	}

public:
	catalog()=default;
	explicit catalog(layer_type layer) {
		layers_.push_back(std::move(layer));
	}
	catalog(std::initializer_list<layer_type> init_list) {
		for (const auto& it:init_list) layers_.push_back(it);
	}
	template <typename _It,typename=std::enable_if_t<std::is_convertible_v<typename std::iterator_traits<_It>::value_type,std::pair<const std::string,_Tp>>>>
	catalog(_It first,_It last) {
		layers_.emplace_back(first,last);
	}
	~catalog()=default;

	catalog(const catalog&)=delete;
	catalog(catalog&& other) noexcept {
		write_lock lk(other.mutex_);
		layers_=std::move(other.layers_);
		named_layers_=std::move(other.named_layers_);
		lookup_policy_=std::move(other.lookup_policy_);
		missing_key_handler_=std::move(other.missing_key_handler_);
		layer_observers_=std::move(other.layer_observers_);
		reorder_observers_=std::move(other.reorder_observers_);
		next_id_.store(other.next_id_.load(std::memory_order_relaxed),std::memory_order_relaxed);
	}

	catalog& operator =(const catalog&)=delete;
	catalog& operator =(catalog&& other) noexcept {
		if (this!=&other) {
			write_lock lk1(mutex_,std::defer_lock);
			write_lock lk2(other.mutex_,std::defer_lock);
			std::lock(lk1,lk2);
			layers_=std::move(other.layers_);
			named_layers_=std::move(other.named_layers_);
			lookup_policy_=std::move(other.lookup_policy_);
			missing_key_handler_=std::move(other.missing_key_handler_);
			layer_observers_=std::move(other.layer_observers_);
			reorder_observers_=std::move(other.reorder_observers_);
			next_id_.store(other.next_id_.load(std::memory_order_relaxed),std::memory_order_relaxed);
		}
		return *this;
	}

	void push_layer(layer_type layer) {
		write_lock lk(mutex_);
		for (auto& [n,idx]:named_layers_) idx++;
		layers_.insert(layers_.begin(),std::move(layer));
		notify_reordered();
	}
	void push_layer(std::string name,layer_type layer) {
		write_lock lk(mutex_);
		for (auto& [n,idx]:named_layers_) idx++;
		layers_.insert(layers_.begin(),std::move(layer));
		named_layers_.push_back({std::move(name),0});
		notify_reordered();
	}

	void append_layer(layer_type layer) {
		write_lock lk(mutex_);
		layers_.push_back(std::move(layer));
		notify_reordered();
	}
	void append_layer(std::string name,layer_type layer) {
		write_lock lk(mutex_);
		size_type idx=layers_.size();
		layers_.push_back(std::move(layer));
		named_layers_.push_back({std::move(name),idx});
		notify_reordered();
	}

	void insert_layer(size_type index,layer_type layer) {
		write_lock lk(mutex_);
		index=std::min(index,layers_.size());
		for (auto& [n,idx]:named_layers_) {
			if (idx>=index) idx++;
		}
		layers_.insert(layers_.begin()+static_cast<std::ptrdiff_t>(index),std::move(layer));
		notify_reordered();
	}
	void insert_layer(size_type index,std::string name,layer_type layer) {
		write_lock lk(mutex_);
		index=std::min(index,layers_.size());
		for (auto& [n,idx]:named_layers_) {
			if (idx>=index) idx++;
		}
		layers_.insert(layers_.begin()+static_cast<std::ptrdiff_t>(index),std::move(layer));
		named_layers_.push_back({std::move(name),index});
		notify_reordered();
	}

	template <typename _InputIt>
	void push_layer_range(_InputIt first,_InputIt last) {
		push_layer(layer_type(first,last));
	}
	template <typename _InputIt>
	void push_layer_range(std::string name,_InputIt first,_InputIt last) {
		push_layer(std::move(name),layer_type(first,last));
	}

	template <typename _InputIt>
	void append_layer_range(_InputIt first,_InputIt last) {
		append_layer(layer_type(first,last));
	}
	template <typename _InputIt>
	void append_layer_range(std::string name,_InputIt first,_InputIt last) {
		append_layer(std::move(name),layer_type(first,last));
	}

	std::optional<layer_type> pop_layer() {
		write_lock lk(mutex_);
		if (layers_.empty()) return std::nullopt;
		layer_type top=std::move(layers_.front());
		layers_.erase(layers_.begin());
		named_layers_.erase(std::remove_if(named_layers_.begin(),named_layers_.end(),[](const auto& p){ return p.second==0; }),named_layers_.end());
		for (auto& [n,idx]:named_layers_) idx--;
		notify_reordered();
		return top;
	}

	bool remove_layer(size_type index) {
		write_lock lk(mutex_);
		if (index>=layers_.size()) return false;
		layers_.erase(layers_.begin()+static_cast<std::ptrdiff_t>(index));
		named_layers_.erase(std::remove_if(named_layers_.begin(),named_layers_.end(),[index](const auto& p){ return p.second==index; }),named_layers_.end());
		for (auto& [n,idx]:named_layers_) {
			if (idx>index) idx--;
		}
		notify_reordered();
		return true;
	}
	bool remove_layer(std::string_view name) {
		write_lock lk(mutex_);
		auto it=std::find_if(named_layers_.begin(),named_layers_.end(),[name](const auto& p){ return p.first==name; });
		if (it==named_layers_.end()) return false;
		size_type index=it->second;
		layers_.erase(layers_.begin()+static_cast<std::ptrdiff_t>(index));
		named_layers_.erase(it);
		for (auto& [n,idx] : named_layers_) {
			if (idx>index) idx--;
		}
		notify_reordered();
		return true;
	}

	layer_type& layer(size_type index) {
		read_lock lk(mutex_);
		if (index>=layers_.size()) throw std::out_of_range("index out of range");
		return layers_[index];
	}
	const layer_type& layer(size_type index) const {
		read_lock lk(mutex_);
		if (index>=layers_.size()) throw std::out_of_range("index out of range");
		return layers_[index];
	}

	std::optional<std::reference_wrapper<layer_type>> layer(std::string_view name) {
		read_lock lk(mutex_);
		auto idx=find_layer_index(name);
		if (!idx) return std::nullopt;
		return std::ref(layers_[*idx]);
	}
	std::optional<std::reference_wrapper<const layer_type>> layer(std::string_view name) const {
		read_lock lk(mutex_);
		auto idx=find_layer_index(name);
		if (!idx) return std::nullopt;
		return std::cref(layers_[*idx]);
	}

	bool replace_layer(size_type index,layer_type new_layer) {
		write_lock lk(mutex_);
		if (index>=layers_.size()) return false;
		layers_[index]=std::move(new_layer);
		notify_layer_changed(index);
		return true;
	}
	bool replace_layer(std::string_view name,layer_type new_layer) {
		write_lock lk(mutex_);
		auto idx=find_layer_index_(name);
		if (!idx) return false;
		layers_[*idx]=std::move(new_layer);
		notify_layer_changed(*idx);
		return true;
	}

	bool swap_layers(size_type i,size_type j) {
		write_lock lk(mutex_);
		if (i>=layers_.size() || j>=layers_.size()) return false;
		std::swap(layers_[i],layers_[j]);
		for (auto& [n,idx]:named_layers_) {
			if (idx==i) idx=j;
			else if (idx==j) idx=i;
		}
		notify_reordered();
		return true;
	}

	bool reorder_layers(const std::vector<size_type>& new_order) {
		write_lock lk(mutex_);
		if (new_order.size()!=layers_.size()) return false;
		std::vector<layer_type> reordered;
		reordered.reserve(layers_.size());
		for (size_type i:new_order) {
			if (i>=layers_.size()) return false;
			reordered.push_back(std::move(layers_[i]));
		}
		std::vector<std::pair<std::string,size_type>> new_named;
		for (const auto& [name,old_idx]:named_layers_) {
			for (size_type new_idx=0;new_idx<new_order.size();new_idx++) {
				if (new_order[new_idx]==old_idx) {
					new_named.push_back({name,new_idx});
					break;
				}
			}
		}
		layers_=std::move(reordered);
		named_layers_=std::move(new_named);
		notify_reordered();
		return true;
	}

	size_type layer_count() const noexcept {
		read_lock lk(mutex_);
		return layers_.size();
	}

	bool has_layer(std::string_view name) const {
		read_lock lk(mutex_);
		return find_layer_index(name).has_value();
	}

	void clear_layers() {
		write_lock lk(mutex_);
		layers_.clear();
		named_layers_.clear();
		notify_reordered();
	}

	std::vector<std::string> layer_names() const {
		read_lock lk(mutex_);
		std::vector<std::string> result(layers_.size());
		for (const auto& [name,idx]:named_layers_) {
			if (idx<result.size()) result[idx]=name;
		}
		return result;
	}

	std::optional<_Tp> find(std::string_view key) const {
		read_lock lk(mutex_);
		if (lookup_policy_) return lookup_policy_(layers_,key);
		return default_lookup_(layers_,key);
	}

	std::optional<_Tp> find_in(size_type layer_index,std::string_view key) const {
		read_lock lk(mutex_);
		if (layer_index>=layers_.size()) return std::nullopt;
		auto it=layers_[layer_index].find(std::string(key));
		if (it!=layers_[layer_index].end()) return it->second;
		return std::nullopt;
	}

	std::optional<_Tp> find_in(std::string_view layer_name,std::string_view key) const {
		read_lock lk(mutex_);
		auto idx=find_layer_index(layer_name);
		if (!idx) return std::nullopt;
		auto it=layers_[*idx].find(std::string(key));
		if (it!=layers_[*idx].end()) return it->second;
		return std::nullopt;
	}

	bool contains(std::string_view key) const {
		return find(key).has_value();
	}

	bool contains_in(size_type layer_index,std::string_view key) const {
		return find_in(layer_index,key).has_value();
	}

	bool contains_in(std::string_view layer_name,std::string_view key) const {
		return find_in(layer_name,key).has_value();
	}

	_Tp at(std::string_view key) const {
		auto v=find(key);
		if (v) return *std::move(v);
		if (missing_key_handler_) return missing_key_handler_(key);
		throw std::out_of_range("key not found");
	}

	std::optional<_Tp> operator [](std::string_view key) const {
		return find(key);
	}

	_Tp find_or(std::string_view key,const _Tp& fallback) const {
		auto v=find(key);
		return v?*std::move(v):fallback;
	}

	_Tp find_or(std::string_view key,_Tp&& fallback) const {
		auto v=find(key);
		return v?*std::move(v):std::move(fallback);
	}

	bool insert_into(size_type layer_index,std::string key,_Tp value) {
		write_lock lk(mutex_);
		if (layer_index>=layers_.size()) return false;
		auto [it,ok]=layers_[layer_index].emplace(std::move(key),std::move(value));
		if (ok) notify_layer_changed(layer_index);
		return ok;
	}
	bool insert_into(std::string_view layer_name,std::string key,_Tp value) {
		write_lock lk(mutex_);
		auto idx=find_layer_index(layer_name);
		if (!idx) return false;
		auto [it,ok]=layers_[*idx].emplace(std::move(key),std::move(value));
		if (ok) notify_layer_changed(*idx);
		return ok;
	}

	void assign_into(size_type layer_index,std::string key,_Tp value) {
		write_lock lk(mutex_);
		if (layer_index>=layers_.size()) return;
		layers_[layer_index][std::move(key)]=std::move(value);
		notify_layer_changed(layer_index);
	}
	void assign_into(std::string_view layer_name,std::string key,_Tp value) {
		write_lock lk(mutex_);
		auto idx=find_layer_index(layer_name);
		if (!idx) return;
		layers_[*idx][std::move(key)]=std::move(value);
		notify_layer_changed(*idx);
	}

	bool erase_from(size_type layer_index,std::string_view key) {
		write_lock lk(mutex_);
		if (layer_index>=layers_.size()) return false;
		auto it=layers_[layer_index].find(std::string(key));
		if (it==layers_[layer_index].end()) return false;
		layers_[layer_index].erase(it);
		notify_layer_changed(layer_index);
		return true;
	}
	bool erase_from(std::string_view layer_name,std::string_view key) {
		write_lock lk(mutex_);
		auto idx=find_layer_index_(layer_name);
		if (!idx) return false;
		auto it=layers_[*idx].find(std::string(key));
		if (it==layers_[*idx].end()) return false;
		layers_[*idx].erase(it);
		notify_layer_changed(*idx);
		return true;
	}

	template <typename _InputIt>
	void insert_range_into(size_type layer_index,_InputIt first,_InputIt last) {
		write_lock lk(mutex_);
		if (layer_index>=layers_.size()) return;
		for (auto it=first;it!=last;it++) layers_[layer_index].emplace(it->first,it->second);
		notify_layer_changed(layer_index);
	}
	template <typename _InputIt>
	void assign_range_into(size_type layer_index,_InputIt first,_InputIt last) {
		write_lock lk(mutex_);
		if (layer_index>=layers_.size()) return;
		for (auto it=first;it!=last;it++) layers_[layer_index][it->first]=it->second;
		notify_layer_changed(layer_index);
	}

	void set_lookup_policy(lookup_policy_type policy) {
		write_lock lk(mutex_);
		lookup_policy_=std::move(policy);
	}

	void reset_lookup_policy() {
		write_lock lk(mutex_);
		lookup_policy_=nullptr;
	}

	void set_missing_key_handler(missing_key_handler_type handler) {
		write_lock lk(mutex_);
		missing_key_handler_=std::move(handler);
	}

	void clear_missing_key_handler() {
		write_lock lk(mutex_);
		missing_key_handler_=nullptr;
	}

	layer_type flatten() const {
		read_lock lk(mutex_);
		layer_type result;
		for (auto it=layers_.rbegin();it!=layers_.rend();it++) {
			for (const auto& [k,v]:*it) result[k]=v;
		}
		return result;
	}
	void flatten_into(layer_type& target,bool overwrite=true) const {
		read_lock lk(mutex_);
		for (auto it=layers_.rbegin();it!=layers_.rend();it++) {
			for (const auto& [k,v]:*it) {
				if (overwrite) target[k]=v;
				else target.emplace(k,v);
			}
		}
	}

	size_type total_entries() const {
		read_lock lk(mutex_);
		size_type n=0;
		for (const auto& it:layers_) n+=it.size();
		return n;
	}

	size_type size() const {
		return flatten().size();
	}

	bool empty() const {
		read_lock lk(mutex_);
		for (const auto& it:layers_) {
			if (!it.empty()) return false;
		}
		return true;
	}

	size_type on_layer_changed(std::function<void(size_type)> callback) {
		write_lock lk(mutex_);
		size_type id=next_id();
		layer_observers_.push_back({id,std::move(callback)});
		return id;
	}

	size_type on_layers_reordered(std::function<void()> callback) {
		write_lock lk(mutex_);
		size_type id=next_id();
		reorder_observers_.push_back({id,std::move(callback)});
		return id;
	}

	bool remove_observer(size_type id) {
		write_lock lk(mutex_);
		auto it=std::find_if(layer_observers_.begin(),layer_observers_.end(),[id](const layer_observer_entry_& e){ return e.id==id; });
		if (it!=layer_observers_.end()) {
			layer_observers_.erase(it);
			return true;
		}
		auto it2=std::find_if(reorder_observers_.begin(),reorder_observers_.end(),[id](const reorder_observer_entry_& e){ return e.id==id; });
		if (it2!=reorder_observers_.end()) {
			reorder_observers_.erase(it2);
			return true;
		}
		return false;
	}

	bool has_observer(size_type id) const {
		read_lock lk(mutex_);
		for (const auto& it:layer_observers_) {
			if (it.id==id) return true;
		}
		for (const auto& it:reorder_observers_) {
			if (it.id==id) return true;
		}
		return false;
	}

	void clear_observers() {
		write_lock lk(mutex_);
		layer_observers_.clear();
		reorder_observers_.clear();
	}

	catalog_view<_Tp,_Syn> view() const noexcept;
};

template <typename _Tp=std::string,bool _Syn=false>
class catalog_view {
public:
	using value_type=_Tp;
	using key_type=std::string;
	using layer_type=typename catalog<_Tp,_Syn>::layer_type;
	using size_type=std::size_t;

private:
	stdex::memory::observer_ptr<const catalog<_Tp,_Syn>> cat_;

public:
	catalog_view()=default;
	catalog_view(const catalog<_Tp,_Syn>& c) noexcept : cat_(stdex::memory::make_observer(&c)) { }
	~catalog_view()=default;

	catalog_view(const catalog_view&)=default;
	catalog_view(catalog_view&&) noexcept=default;

	catalog_view& operator =(const catalog_view&)=default;
	catalog_view& operator =(catalog_view&&) noexcept=default;


	bool valid() const noexcept { return cat_!=nullptr; }
	explicit operator bool() const noexcept { return valid(); }

	std::optional<_Tp> find(std::string_view key) const {
		if (!cat_) return std::nullopt;
		return cat_->find(key);
	}

	std::optional<_Tp> find_in(size_type layer_index,std::string_view key) const {
		if (!cat_) return std::nullopt;
		return cat_->find_in(layer_index,key);
	}

	std::optional<_Tp> find_in(std::string_view layer_name,std::string_view key) const {
		if (!cat_) return std::nullopt;
		return cat_->find_in(layer_name,key);
	}

	bool contains(std::string_view key) const {
		if (!cat_) return false;
		return cat_->contains(key);
	}

	bool contains_in(size_type layer_index,std::string_view key) const {
		if (!cat_) return false;
		return cat_->contains_in(layer_index,key);
	}

	bool contains_in(std::string_view layer_name,std::string_view key) const {
		if (!cat_) return false;
		return cat_->contains_in(layer_name,key);
	}

	_Tp at(std::string_view key) const {
		if (!cat_) throw std::runtime_error("null view");
		return cat_->at(key);
	}

	std::optional<_Tp> operator [](std::string_view key) const {
		return find(key);
	}

	_Tp find_or(std::string_view key,const _Tp& fallback) const {
		if (!cat_) return fallback;
		return cat_->find_or(key,fallback);
	}

	_Tp find_or(std::string_view key,_Tp&& fallback) const {
		if (!cat_) return std::move(fallback);
		return cat_->find_or(key,std::move(fallback));
	}

	size_type layer_count() const noexcept {
		if (!cat_) return 0;
		return cat_->layer_count();
	}

	bool has_layer(std::string_view name) const {
		if (!cat_) return false;
		return cat_->has_layer(name);
	}

	bool empty() const {
		if (!cat_) return true;
		return cat_->empty();
	}

	size_type size() const {
		if (!cat_) return 0;
		return cat_->size();
	}

	layer_type flatten() const {
		if (!cat_) return {};
		return cat_->flatten();
	}

	std::vector<std::string> layer_names() const {
		if (!cat_) return {};
		return cat_->layer_names();
	}

	const catalog<_Tp,_Syn>* get() const noexcept { return cat_.get(); }
};

template <typename _Tp,bool _Syn>
catalog_view<_Tp,_Syn> catalog<_Tp,_Syn>::view() const noexcept {
	return catalog_view<_Tp,_Syn>(*this);
}

template <typename _Tp=std::string>
class catalog_builder {
public:
	using value_type=_Tp;
	using layer_type=std::unordered_map<std::string,_Tp>;

private:
	struct named_layer_entry {
		std::string name;
		layer_type layer;
	};

	std::vector<named_layer_entry> layers_;
	typename catalog<_Tp,false>::lookup_policy_type lookup_policy_;
	typename catalog<_Tp,false>::missing_key_handler_type missing_key_handler_;

public:
	catalog_builder()=default;
	~catalog_builder()=default;

	catalog_builder(const catalog_builder&)=default;
	catalog_builder(catalog_builder&&)=default;

	catalog_builder& operator =(const catalog_builder&)=default;
	catalog_builder& operator =(catalog_builder&&)=default;


	catalog_builder& push_layer(std::string name,layer_type layer) & {
		layers_.insert(layers_.begin(),{std::move(name),std::move(layer)});
		return *this;
	}

	catalog_builder& push_layer(layer_type layer) & {
		layers_.insert(layers_.begin(),{"",std::move(layer)});
		return *this;
	}

	catalog_builder& append_layer(std::string name,layer_type layer) & {
		layers_.push_back({std::move(name),std::move(layer)});
		return *this;
	}

	catalog_builder& append_layer(layer_type layer) & {
		layers_.push_back({"",std::move(layer)});
		return *this;
	}

	template <typename _InputIt>
	catalog_builder& push_layer_range(std::string name,_InputIt first,_InputIt last) & {
		return push_layer(std::move(name),layer_type(first,last));
	}

	template <typename _InputIt>
	catalog_builder& append_layer_range(std::string name,_InputIt first,_InputIt last) & {
		return append_layer(std::move(name),layer_type(first,last));
	}

	catalog_builder& with_lookup_policy(typename catalog<_Tp,false>::lookup_policy_type policy) & {
		lookup_policy_=std::move(policy);
		return *this;
	}

	catalog_builder& with_missing_key_handler(typename catalog<_Tp,false>::missing_key_handler_type handler) & {
		missing_key_handler_=std::move(handler);
		return *this;
	}

	catalog_builder&& push_layer(std::string name,layer_type layer) && {
		return std::move(push_layer(std::move(name),std::move(layer)));
	}
	catalog_builder&& push_layer(layer_type layer) && {
		return std::move(push_layer(std::move(layer)));
	}

	catalog_builder&& append_layer(std::string name,layer_type layer) && {
		return std::move(append_layer(std::move(name),std::move(layer)));
	}
	catalog_builder&& append_layer(layer_type layer) && {
		return std::move(append_layer(std::move(layer)));
	}

	catalog_builder&& with_lookup_policy(typename catalog<_Tp,false>::lookup_policy_type policy) && {
		return std::move(with_lookup_policy(std::move(policy)));
	}
	catalog_builder&& with_missing_key_handler(typename catalog<_Tp,false>::missing_key_handler_type handler) && {
		return std::move(with_missing_key_handler(std::move(handler)));
	}

	catalog<_Tp,false> build() && {
		catalog<_Tp,false> result;
		for (auto& it:layers_) {
			if (it.name.empty()) result.append_layer(std::move(it.layer));
			else result.append_layer(std::move(it.name),std::move(it.layer));
		}
		if (lookup_policy_) result.set_lookup_policy(std::move(lookup_policy_));
		if (missing_key_handler_) result.set_missing_key_handler(std::move(missing_key_handler_));
		return result;
	}

	catalog<_Tp,true> build_sync() && {
		catalog<_Tp,true> result;
		for (auto& it:layers_) {
			if (it.name.empty()) result.append_layer(std::move(it.layer));
			else result.append_layer(std::move(it.name),std::move(it.layer));
		}
		if (lookup_policy_) result.set_lookup_policy(std::move(lookup_policy_));
		if (missing_key_handler_) result.set_missing_key_handler(std::move(missing_key_handler_));
		return result;
	}

	void reset() {
		layers_.clear();
		lookup_policy_=nullptr;
		missing_key_handler_=nullptr;
	}

	size_t layer_count() const noexcept { return layers_.size(); }
	bool empty() const noexcept { return layers_.empty(); }
};

inline std::string lookup(const catalog<std::string>& cat,std::string_view key) {
	auto v=cat.find(key);
	return v?*v:std::string(key);
}

inline std::string lookup(catalog_view<std::string> view,std::string_view key) {
	auto v=view.find(key);
	return v?*v:std::string(key);
}

template <typename _Tp,bool _Syn>
_Tp lookup_or(const catalog<_Tp,_Syn>& cat,std::string_view key,const _Tp& fallback){
	return cat.find_or(key,fallback);
}

template <typename _Tp,bool _Syn>
_Tp lookup_or(catalog_view<_Tp,_Syn> view,std::string_view key,const _Tp& fallback) {
	return view.find_or(key,fallback);
}

template <typename _Tp,bool _Syn>
catalog<_Tp,false> merge_catalogs(const catalog<_Tp,_Syn>& high,const catalog<_Tp,_Syn>& low) {
	auto result=low.flatten();
	auto high_flat=high.flatten();
	for (const auto& [k,v]:high_flat) result[k]=v;
	return catalog<_Tp,false>(std::move(result));
}

}

}

#endif