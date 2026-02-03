//Last Modified At 2026/02/02
//@Version 1.0.0.0
#ifndef _STDEX_STRUCTURE_LRU_CACHE_H_
#define _STDEX_STRUCTURE_LRU_CACHE_H_ 1

#include <cstddef>
#include <memory>
#include <optional>
#include <stdexcept>
#include <unordered_map>

namespace stdex {

namespace structure {

template <typename _Key,typename _Tp>
class lru_cache {
	struct list_node {
		_Key key;
		_Tp value;
		std::shared_ptr<list_node> prev;
		std::shared_ptr<list_node> next;
		list_node(const _Key& k,const _Tp& v) : key(k) , value(v) , prev(nullptr) , next(nullptr) { }
	};
	std::unordered_map<_Key,std::shared_ptr<list_node>> map_;
	std::shared_ptr<list_node> first_;
	std::shared_ptr<list_node> last_;
	std::size_t max_size_;

public:
	explicit lru_cache(std::size_t max_size) : max_size_(max_size) , first_(nullptr) , last_(nullptr) { }
	
	std::size_t size() const { return map_.size(); }
	std::optional<_Tp> get(const _Key& key) {
		auto it=map_.find(key);
		if (it==map_.end()) return std::nullopt;
		auto node=it->second;
		if (node!=first_) {
			if (node==last_) {
				last_=node->prev;
				last_->next=nullptr;
			} else {
				node->prev->next=node->next;
				node->next->prev=node->prev;
			}
			node->next=first_;
			first_->prev=node;
			first_=node;
			node->prev=nullptr;
		}
		return node->value;
	}
	void set(const _Key& key,const _Tp& value) {
		if (max_size_<1) return;
		if (map_.find(key)!=map_.end()) throw std::invalid_argument("Cannot update existing keys in the cache");
		auto node=std::make_shared<list_node>(key,value);
		if (!first_) {
			first_=node;
			last_=node;
		} else {
			node->next=first_;
			first_->prev=node;
			first_=node;
		}
		map_[key]=node;
		while (map_.size()>max_size_) {
			auto last=last_;
			map_.erase(last->key);
			last_=last->prev;
			if (last_) last_->next=nullptr;
			else first_=nullptr;
		}
	}
};

}

}

#endif