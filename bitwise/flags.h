//Last Modified At 2026/02/13
//@Version 2.0.3.0
#ifndef _STDEX_BITWISE_FLAGS_H_
#define _STDEX_BITWISE_FLAGS_H_ 1

#include <algorithm>
#include <map>
#include <queue>
#include <set>
#include <stack>
#include <stdexcept>
#include <type_traits>
#include <unordered_set>
#include <vector>
#include <utility>

#if __has_include("../macros/cpp_version.h")
#include "../macros/cpp_version.h"//At Least 1.0
#endif

#ifndef _STDEX_CPP20_VERSION
#define _STDEX_CPP20_VERSION 202002L
#endif

#ifndef _STDEX_CONSTEXPR
#if __cplusplus>=_STDEX_CPP20_VERSION
#define _STDEX_CONSTEXPR constexpr
#else
#define _STDEX_CONSTEXPR
#endif
#endif

namespace stdex {
	
namespace bitwise {

template <typename _Tp>
class flags {
	static_assert(std::is_enum_v<_Tp>,"_Tp must be an enum type.");

protected:
	std::underlying_type_t<_Tp> value_{0};
	
public:
	struct _flags_enhanced : std::false_type {};
	constexpr flags()=default;
	constexpr flags(_Tp e) : value_(static_cast<std::underlying_type_t<_Tp>>(e)) {}
	constexpr flags& operator =(_Tp e) noexcept {
		value_=static_cast<std::underlying_type_t<_Tp>>(e);
		return *this;
	}
	virtual _STDEX_CONSTEXPR flags& operator <<=(_Tp e) {
		value_|=static_cast<std::underlying_type_t<_Tp>>(e);
		return *this;
	}
	virtual _STDEX_CONSTEXPR flags operator <<(_Tp e) const {
		auto result=*this;
		result<<=e;
		return result;
	}
	_STDEX_CONSTEXPR flags& operator <<=(flags<_Tp> value) noexcept {
		value.for_each([&](_Tp e){
			this->operator <<=(e);
		});
		return *this;
	}
	_STDEX_CONSTEXPR flags operator <<(flags<_Tp> value) const noexcept {
		auto result=*this;
		value.for_each([&](_Tp e){
			result<<=(e);
		});
		return result;
	} 
	constexpr flags& operator >>=(_Tp e) noexcept {
		value_&=~static_cast<std::underlying_type_t<_Tp>>(e);
		return *this;
	}
	constexpr flags operator >>(_Tp e) const noexcept {
		return flags(*this)>>=e;
	}
	constexpr bool contains(_Tp e) const noexcept {
		return (value_ & static_cast<std::underlying_type_t<_Tp>>(e));
	}
	constexpr operator typename std::underlying_type_t<_Tp>() const noexcept {
		return value_;
	}
	constexpr operator _Tp() const noexcept {
		return (_Tp)value_;
	}
	constexpr void clear() noexcept {
		value_=0;
	}
	constexpr bool empty() const noexcept {
		return !value_;
	}
	template <typename _Func>
	void for_each(_Func func) const {
		std::underlying_type_t<_Tp> temp=value_;
		std::underlying_type_t<_Tp> e=_Tp(1);
		while (temp) {
			if (temp&1) func((_Tp)e);
			temp>>=1;
			e<<=1;
		}
	}
};

enum relation_policy {
	RP_FORCE,
	RP_REJECT,
	RP_EXCEPTION,
};

template <typename _Tp,relation_policy _DefaultPolicy=RP_REJECT>
class exclusive_flags : public flags<_Tp> {
protected:
	std::map<_Tp,flags<_Tp>*> exclusions_;
	relation_policy exclusion_policy_;
public:
	exclusive_flags() {
		exclusion_policy_=_DefaultPolicy;
	}
	template <relation_policy _OtherPolicy>
	exclusive_flags(const exclusive_flags<_Tp,_OtherPolicy>& other) {
		*this=other;
		exclusion_policy_=other.exclusion_policy_;	
	};
	~exclusive_flags() {
		std::set<flags<_Tp>*> exclusions;
		for (auto& it:exclusions_) exclusions.insert(it.second);
		for (auto& it:exclusions) delete it;
	}
#ifndef _STDEX_IGNORE_BITWISE_FLAGS_WARNINGS
	[[deprecated("Direct assignment bypasses ALL relation checks and cannot ensure the relations. If you are not sure what will happen, please use <<= instead")]]
#endif
	constexpr exclusive_flags& operator =(_Tp e) noexcept {
		flags<_Tp>::operator =(e);
		return *this;
	}
	template <relation_policy _OtherPolicy>
	exclusive_flags& operator =(const exclusive_flags<_Tp,_OtherPolicy>& other) {
		if (this==&other) return *this;
		flags<_Tp>::operator =(other);
		for_each(clear_exclusion);
		other.for_each(set_exclusion);
		return *this;
	}
	void set_exclusion_policy(relation_policy policy) {
		if (policy<RP_FORCE || policy>RP_EXCEPTION) throw std::invalid_argument("Invalid policy");
		exclusion_policy_=policy;
	}
	void set_exclusion(_Tp lhs,_Tp rhs) {
		if (lhs==rhs) return;
		if (!exclusions_[lhs] && !exclusions_[rhs]) {
			flags<_Tp>* flag=new flags<_Tp>(lhs);
			*flag<<=rhs;
			exclusions_[lhs]=exclusions_[rhs]=flag;
			return;
		}
		if (!exclusions_[lhs]) std::swap(lhs,rhs);
		if (!exclusions_[rhs]) {
			(*exclusions_[lhs])<<=rhs;
			exclusions_[rhs]=exclusions_[lhs];
			return;
		}
		if (exclusions_[lhs]==exclusions_[rhs]) return;
		(*exclusions_[lhs])<<=(_Tp)(*exclusions_[rhs]);
		delete exclusions_[rhs];
		exclusions_[rhs]=exclusions_[lhs];
	}
	void clear_exclusion(_Tp e) {
		if (!exclusions_[e]) return;
		(*exclusions_[e])>>=e;
		if (exclusions_[e]->empty()) delete exclusions_[e];
		exclusions_[e]=nullptr;
	}
	virtual _STDEX_CONSTEXPR exclusive_flags& operator <<=(_Tp e) override {
		if (exclusion_policy_==RP_FORCE) {
			if (exclusions_[e]) flags<_Tp>::operator >>=((_Tp)(*exclusions_[e]));
		} else {
			bool conflict=false;
			if (exclusions_[e]) {
				if (flags<_Tp>::value_ & (_Tp)(*exclusions_[e])) conflict=true;
			}
			if (conflict) {
				if (exclusion_policy_==RP_FORCE) return *this;
				throw std::invalid_argument("Invalid operation adding element");
			}
		}
		flags<_Tp>::operator <<=(e);
		return *this;
	}
	const std::map<_Tp,flags<_Tp>*>& exclusions() const {
		return exclusions_;
	}
	const relation_policy& exclusion_policy() const {
		return exclusion_policy_;
	}
};

enum consistency_type {
	CT_CYCLE,
	CT_FORBIDDEN_WITH_DEPENDENCY,
	CT_REVERSE_FORBIDDEN_WITH_DEPENDENCY,
	CT_FORBIDDEN_SELF,
};

template <typename _Tp,relation_policy _DefaultExclusionPolicy=RP_REJECT,relation_policy _DefaultForbiddenPolicy=RP_REJECT,relation_policy _DefaultDependencyPolicy=RP_REJECT>
class advanced_flags : public exclusive_flags<_Tp,_DefaultExclusionPolicy> {
	std::map<_Tp,flags<_Tp>> forbiddens_;
	std::map<_Tp,flags<_Tp>> dependencies_;
	relation_policy forbidden_policy_;
	relation_policy dependency_policy_;
	
private:
	bool check_dependencies(_Tp value) const {
		if (!dependencies_.count(value)) return true;
		bool check=true;
		dependencies_.at(value).for_each([this,&check](_Tp e){
			check&=this->contains(e);
		});
 		return check;
	}
	bool handle_dependency_failure(_Tp value) {
		if (dependency_policy_==RP_EXCEPTION) throw std::invalid_argument("Dependency requirement not met");
		else if (dependency_policy_==RP_FORCE) {
			auto result=check_consistency();
			for (auto& it:result) {
				if (it.type_!=CT_CYCLE) continue;
				if (std::find(it.value_.begin(),it.value_.end(),value)!=it.value_.end() || it.extra_value_.contains(value)) return false;
			}
			if (dependencies_.count(value)) {
				dependencies_[value].for_each([this](_Tp e){
					this->operator <<=(e);
				});
				if (check_dependencies(value)) {
					exclusive_flags<_Tp,_DefaultExclusionPolicy>::operator <<=(value);
					return true;
				}
			}  
		}
		return false;
	}
	bool check_forbiddens(_Tp value) const {
		for (const auto& it:forbiddens_) {
			if (this->contains(it.first) && it.second.contains(value)) return false;
		}
		bool check=true;
		if (forbiddens_.count(value)) {
			forbiddens_.at(value).for_each([this,&check](_Tp e){
				check&=!this->contains(e);
			});
		}
		return check;
	}
	bool handle_forbidden_failure(_Tp e,flags<_Tp>& result) {
		if (forbidden_policy_==RP_EXCEPTION) throw std::invalid_argument("Forbidden conflict detected");
		else if (forbidden_policy_==RP_FORCE) {
			for (auto& it:forbiddens_) {
				if (this->contains(it.first) && it.second.contains(e)) result<<=e;
			}
			if (forbiddens_.count(e)) result<<=(_Tp)forbiddens_[e];
 			return true;
		}
		return false;
	}
	void tarjan(_Tp v,std::map<_Tp,flags<_Tp>>& graph,std::map<_Tp,int>& index,std::map<_Tp,int>& lowlink,flags<_Tp>& on_stack,std::stack<_Tp>& s,int& idx,std::vector<std::vector<_Tp>>& sccs) {
		index[v]=idx;
		lowlink[v]=idx++;
		s.push(v);
		on_stack<<=v;
  		graph[v].for_each([&](_Tp e){
			if (!index.count(e)) {
				tarjan(e,graph,index,lowlink,on_stack,s,idx,sccs);
				lowlink[v]=std::min(lowlink[v],lowlink[e]);
			} else if (on_stack.contains(e)) lowlink[v]=std::min(lowlink[v],index[e]);
		});
		if (lowlink[v]==index[v]) {
			std::vector<_Tp> scc;
			_Tp w=(_Tp)-1;
			while (w!=v && !s.empty()) {
				w=s.top();
				s.pop();
				on_stack>>=w;
				scc.push_back(w);
			}
			sccs.push_back(scc);
		}
	}
	std::unordered_set<_Tp> BFS(_Tp start,std::map<_Tp,flags<_Tp>>& graph,flags<_Tp>& deleted_nodes) {
		std::unordered_set<_Tp> visited;
		if (deleted_nodes.contains(start)) return visited;
		std::queue<_Tp> q;
		q.push(start);
		visited.insert(start);
		while (!q.empty()) {
			_Tp temp=q.front();
			q.pop();
			if (graph.count(temp)) {
				bool skip=false;
				graph[temp].for_each([&](_Tp e){
					if (deleted_nodes.contains(e)) skip=true;
					if (!skip && !visited.count(e)) {
						visited.insert(e);
						q.push(e);
					}
				});
			}
		}
		visited.erase(start);
		return visited;
	}
	bool has_path(_Tp from,_Tp to,std::map<_Tp,flags<_Tp>>& graph,flags<_Tp> &deleted_nodes) {
		if (from==to) return true;
		if (deleted_nodes.contains(from) || deleted_nodes.contains(to)) return false;
		auto result=BFS(from,graph,deleted_nodes);
		return result.count(to);
	}
	
public:
	advanced_flags() {
		forbidden_policy_=_DefaultForbiddenPolicy;
		dependency_policy_=_DefaultDependencyPolicy;
	}
	template <relation_policy _OtherPolicy1,relation_policy _OtherPolicy2,relation_policy _OtherPolicy3>
	advanced_flags(const advanced_flags<_Tp,_OtherPolicy1,_OtherPolicy2,_OtherPolicy3>& other) {
		*this=other;
		exclusive_flags<_Tp,_DefaultExclusionPolicy>::exclusion_policy_=other.exclusion_policy_;
		forbidden_policy_=other.forbidden_policy_;
		dependency_policy_=other.dependency_policy_;	
	};
	template <relation_policy _OtherPolicy1,relation_policy _OtherPolicy2,relation_policy _OtherPolicy3>
	advanced_flags& operator =(const advanced_flags<_Tp,_OtherPolicy1,_OtherPolicy2,_OtherPolicy3>& other) {
		if (this==&other) return *this;
		exclusive_flags<_Tp,_DefaultExclusionPolicy>::operator =(other);
		forbiddens_=other.forbiddens_;
		dependencies_=other.dependencies_;
		return *this;
	}
	void set_forbidden_policy(relation_policy policy) {
		forbidden_policy_=policy;
	}
	void set_dependency_policy(relation_policy policy) {
		dependency_policy_=policy;
	}
	void add_dependency(_Tp requirer,_Tp required) {
		dependencies_[requirer]<<=required;
	}
	void remove_dependency(_Tp requirer,_Tp required) {
		dependencies_[requirer]>>=required;
	}
	void clear_dependency(_Tp requirer) {
		dependencies_.erase(requirer);
	}
	void add_forbidden(_Tp element,_Tp forbidden) {
		forbiddens_[element]<<=forbidden;
	}
	void remove_forbidden(_Tp element,_Tp forbidden) {
		forbiddens_[element]>>=forbidden;
	}
	void clear_forbidden(_Tp element) {
		forbiddens_.erase(element);
	}
	_STDEX_CONSTEXPR advanced_flags& operator <<=(_Tp e) override {
		bool check=true;
		if (check && !check_dependencies(e)) check=handle_dependency_failure(e);
		flags<_Tp> result;
		if (check && !check_forbiddens(e)) check=handle_forbidden_failure(e,result);
		if (check) {
			exclusive_flags<_Tp,_DefaultExclusionPolicy>::operator <<=(e);
			result.for_each([this](_Tp e) {
				this->operator >>=(e);
			});
		}
		return *this;
	}
	template <typename _Up=_Tp>
	struct consistency_set {
		static_assert(std::is_same_v<_Tp,_Up>,"the _Up of consistency must be match the _Tp of advanced_flags");
		consistency_type type_;
		std::vector<_Up> value_;
		flags<_Up> extra_value_;
	};
	std::vector<consistency_set<_Tp>> check_consistency() {
		std::vector<consistency_set<_Tp>> inconsistent_groups;
		std::map<_Tp,flags<_Tp>> temp_dependencies;
		for (auto& it:dependencies_) {
			it.second.for_each([&temp_dependencies,&it](_Tp e){
				temp_dependencies[e]<<=it.first;
			});
		}
		std::set<std::pair<_Tp,_Tp>> temp_forbiddens;
		for (auto& it:forbiddens_) {
			it.second.for_each([&temp_forbiddens,&it](_Tp e){
				temp_forbiddens.insert(std::make_pair(e,it.first));
			});
		}
		std::map<_Tp,int> index;
		std::map<_Tp,int> lowlink;
		flags<_Tp> on_stack;
		std::stack<_Tp> s;
		int idx=0;
		std::vector<std::vector<_Tp>> sccs;
		for (auto& it:temp_dependencies) {
			_Tp v=it.first;
			if (!index.count(v)) tarjan(v,temp_dependencies,index,lowlink,on_stack,s,idx,sccs);
		}
		flags<_Tp> deleted_nodes;
		for (auto& it:sccs) {
			if (it.size()>1) {
				consistency_set<_Tp> temp_result;
				for (auto& jt:it) {
					temp_result.value_.push_back(jt);
					deleted_nodes<<=jt;
				}
				for (auto& jt:it) {
					flags<_Tp> null_nodes;
					std::unordered_set<_Tp> temp_successors=BFS(jt,temp_dependencies,null_nodes);
					for (auto& kt:temp_successors) temp_result.extra_value_<<=kt;
					for (auto& kt:temp_result.value_) temp_result.extra_value_>>=kt;
				}
				temp_result.type_=CT_CYCLE;
				inconsistent_groups.push_back(temp_result);
			}
		}
		for (auto& it:temp_forbiddens) {
			_Tp lhs=it.first;
			_Tp rhs=it.second;
			if (lhs==rhs) continue;
			if (deleted_nodes.contains(lhs) || deleted_nodes.contains(rhs)) continue;
			if (has_path(lhs,rhs,temp_dependencies,deleted_nodes)) {
				consistency_set<_Tp> temp_result;
				temp_result.type_=CT_FORBIDDEN_WITH_DEPENDENCY;
				temp_result.value_.push_back(lhs);
				temp_result.value_.push_back(rhs);
				std::unordered_set<_Tp> succ=BFS(rhs,temp_dependencies,deleted_nodes);
				for (auto& jt:succ) temp_result.extra_value_<<=jt;
				inconsistent_groups.push_back(temp_result);
			} else if (has_path(rhs,lhs,temp_dependencies,deleted_nodes)) {
				consistency_set<_Tp> temp_result;
				temp_result.type_=CT_REVERSE_FORBIDDEN_WITH_DEPENDENCY;
				temp_result.value_.push_back(rhs);
				temp_result.value_.push_back(lhs);
				std::unordered_set<_Tp> succ=BFS(lhs,temp_dependencies,deleted_nodes);
				for (auto& jt:succ) temp_result.extra_value_<<=jt;
				inconsistent_groups.push_back(temp_result);
			}
		}
		for (auto& it:temp_forbiddens) {
			if (it.first==it.second && !deleted_nodes.contains(it.first)) {
				consistency_set<_Tp> temp_result;
				temp_result.type_=CT_FORBIDDEN_SELF;
				temp_result.value_.push_back(it.first);
				inconsistent_groups.push_back(temp_result);
			}
		}
		return inconsistent_groups;
	}
	const std::map<_Tp,flags<_Tp>>& forbiddens() const {
		return forbiddens_;
	}
	const std::map<_Tp,flags<_Tp>>& dependencies() const {
		return dependencies_;
	}
	const relation_policy& forbidden_policy() const {
		return forbidden_policy_;
	}
	const relation_policy& dependency_policy() const {
		return dependency_policy_;
	}
};

}

}

#undef _STDEX_CONSTEXPR

#define _STDEX_ENABLE_FLAGS_ENHANCED(EnumType) \
template<> \
struct stdex::bitwise::flags<EnumType>::_flags_enhanced : std::true_type {}; \
constexpr stdex::bitwise::flags<EnumType> operator <<(EnumType lhs,EnumType rhs) noexcept { \
	return stdex::bitwise::flags<EnumType>(lhs)<<rhs; \
} \
constexpr stdex::bitwise::flags<EnumType> operator >>(EnumType lhs,EnumType rhs) noexcept { \
	return stdex::bitwise::flags<EnumType>(lhs)>>rhs; \
}

#endif