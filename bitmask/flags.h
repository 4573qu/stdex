//Last Modified At 2025/09/09
//@Version 2.0.1.0
#ifndef _STD4573_BITMASK_FLAGS_H_
#define _STD4573_BITMASK_FLAGS_H_ 1

#include <algorithm>
#include <set>
#include <stdexcept>
#include <type_traits>
#include <map>

namespace stdex {
	
namespace bitmask {

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
	constexpr flags& operator <<=(_Tp e) noexcept {
		value_|=static_cast<std::underlying_type_t<_Tp>>(e);
		return *this;
	}
	constexpr flags operator <<(_Tp e) const noexcept {
		return flags(*this)<<=e;
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
	~exclusive_flags() {
		std::set<flags<_Tp>*> exclusions;
		for (auto& it:exclusions_) exclusions.insert(it.second);
		for (auto& it:exclusions) delete it;
	}
	template <relation_policy _OtherPolicy>
	exclusive_flags& operator =(const exclusive_flags<_Tp,_OtherPolicy>& other) {
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
	constexpr exclusive_flags& operator <<=(_Tp e) {
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
	constexpr exclusive_flags operator <<(_Tp e) const {
		return exclusive_flags(*this)<<=e;
	}
	const std::map<_Tp,flags<_Tp>*>& exclusions() const {
		return exclusions_;
	}
};

template <typename _Tp,relation_policy _DefaultExclusionPolicy=RP_REJECT,relation_policy _DefaultForbiddenPolicy=RP_REJECT,relation_policy _DefaultDependencyPolicy=RP_REJECT>
class advanced_flags : public exclusive_flags<_Tp,_DefaultExclusionPolicy> {
	std::map<_Tp,flags<_Tp>> forbiddens_;
	std::map<_Tp,flags<_Tp>> dependencies_;
	relation_policy forbidden_policy_;
	relation_policy dependency_policy_;
	
private:
	bool check_dependencies(_Tp e) const {
		if (!dependencies_.count(e)) return true;
		bool check=true;
		dependencies_.at(e).for_each([this,&check](_Tp e){
			check&=this->contains(e);
		});
 		return check;
	}
	bool handle_dependency_failure(_Tp e) {
		if (dependency_policy_==RP_EXCEPTION) throw std::invalid_argument("Dependency requirement not met");
		else if (dependency_policy_==RP_FORCE) {
			if (dependencies_.count(e)) {
				dependencies_[e].for_each([this](_Tp e){
					this->operator<<=(e);
				});
				if (check_dependencies(e)) return true;
			}  
		}
		return false;
	}
	bool check_forbiddens(_Tp e) const {
		for (const auto& it:forbiddens_) {
			if (this->contains(it.first) && it.second.contains(e)) return false;
		}
		bool check=true;
		if (forbiddens_.count(e)) {
			forbiddens_.at(e).for_each([this,&check](_Tp e){
				check&=!this->contains(e);
			});
		}
		return check;
	}
	bool handle_forbidden_failure(_Tp e) {
		if (forbidden_policy_==RP_EXCEPTION) throw std::invalid_argument("Forbidden conflict detected");
		else if (forbidden_policy_==RP_FORCE) {
			for (const auto& it:forbiddens_) {
				if (this->contains(it.first) && it.second.contains(e)) flags<_Tp>::operator>>=(it.first);
			}
			if (forbiddens_.count(e)) flags<_Tp>::operator>>=((_Tp)forbiddens_[e]);
 			return true;
		}
		return false;
	}
	bool check_dependency_cycle(_Tp start,flags<_Tp>& visited) const {
		if (visited.contains(start)) return true; // 发现循环
		visited<<=start;
		if (dependencies_.count(start)) {
			bool cycle_found=false;
			dependencies_[start].for_each([&](_Tp e){
				if (check_dependency_cycle(e,visited)) cycle_found=true;
			});
			if (cycle_found) return true;
		}
		visited>>=start;
		return false;
	}
	bool check_mutual_with_dependency(flags<_Tp> group) const {
		bool has_conflict=false;
		group.for_each([&](_Tp lhs){
			group.for_each([&](_Tp rhs){
				if (lhs!=rhs) {
					if (dependencies_.count(lhs) && dependencies_[lhs].contains(rhs)) has_conflict = true;
				}
			});
		});
		return !has_conflict;
	}
	bool check_forbidden_with_dependency(_Tp e,flags<_Tp> forbidden) const {
		if (dependencies_.count(e)) return !((_Tp)dependencies_[e] & (_Tp)forbidden);
		return true;
	}
    
public:
	advanced_flags() {
		forbidden_policy_=_DefaultForbiddenPolicy;
		dependency_policy_=_DefaultDependencyPolicy;
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
	advanced_flags& operator<<=(_Tp e) {
		bool check=true;
		if (check && !check_dependencies(e)) check=handle_dependency_failure(e);
		if (check && !check_forbiddens(e)) check=handle_forbidden_failure(e);
		if (check) exclusive_flags<_Tp,_DefaultExclusionPolicy>::operator<<=(e);
		return *this;
	}
	advanced_flags operator<<(_Tp e) const {
		return advanced_flags(*this)<<=e;
	}
	std::vector<flags<_Tp>> check_consistency() const {
		std::vector<flags<_Tp>> inconsistent_groups;
		for (const auto& it:dependencies_) {
			flags<_Tp> visited;
			if (check_dependency_cycle(it.first,visited)) inconsistent_groups.push_back(visited);
		}
		for (const auto& it:exclusive_flags<_Tp,_DefaultExclusionPolicy>::exclusions_) {
			if (it.second) {
				if (!check_mutual_with_dependency(*it.second)) inconsistent_groups.push_back(*it.second);
			}
		}
		for (const auto& it:forbiddens_) {
			if (!check_forbidden_with_dependency(it.first,it.second)) inconsistent_groups.push_back(flags<_Tp>(it.first)<<((_Tp)it.second));
		}
		std::map<_Tp,int> duplicates;
		for (const auto& it:inconsistent_groups) {
			_Tp value=(_Tp)it;
			duplicates[value]++;
		}
		inconsistent_groups.erase(std::remove_if(inconsistent_groups.begin(),inconsistent_groups.end(),[&duplicates](const auto& e) {
			if (duplicates[(_Tp)e]>1) {
				duplicates[(_Tp)e]--;
				return true;
			} else return false;
		}),inconsistent_groups.end());
		return inconsistent_groups;
	}
	const std::map<_Tp,flags<_Tp>>& forbiddens() const {
		return forbiddens_;
	}
	const std::map<_Tp,flags<_Tp>>& dependencies() const {
		return dependencies_;
	}
};

}

}

#define _STDEX_ENABLE_FLAGS_ENHANCED(EnumType) \
template<> \
struct stdex::bitmask::flags<EnumType>::_flags_enhanced : std::true_type {}; \
constexpr stdex::bitmask::flags<EnumType> operator <<(EnumType lhs,EnumType rhs) noexcept { \
	return stdex::bitmask::flags<EnumType>(lhs)<<rhs; \
} \
constexpr stdex::bitmask::flags<EnumType> operator >>(EnumType lhs,EnumType rhs) noexcept { \
	return stdex::bitmask::flags<EnumType>(lhs)>>rhs; \
}

#endif