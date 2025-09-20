//Last Modified At 2025/09/20
//@Version 3.1.0.0
#ifndef _STDEX_SYNTAX_PARSER_H_
#define _STDEX_SYNTAX_PARSER_H_ 1

#include <algorithm>
#include <cstddef>
#include <functional>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "../macros/cpp_version.h"//At Least 1.0
#include "../structure/graph_algorithm.h"//At Least 1.0

#ifndef _STDEX_CPP17_VERSION
#define _STDEX_CPP17_VERSION 201703L
#endif

#if __cplusplus>=_STDEX_CPP17_VERSION
#include <initializer_list>
#include <typeinfo>
#include <variant>
#endif

namespace stdex {
	
namespace syntax {
	
enum sheet_type {
	ST_ERROR,
	ST_REDUCTION,
	ST_SHIFT,
	ST_ACCEPT,
};

template <typename _Tp,typename _SentenceEnum=int>
struct parser_unit {
	_Tp left_op_;
	std::vector<_Tp> right_ops_;
	std::unordered_set<_Tp> first_set_;
	_SentenceEnum id_;
	int dot_;
	parser_unit() {
		dot_=-1;
	}
	parser_unit(const parser_unit& other) {
		left_op_=other.left_op_;
		right_ops_=other.right_ops_;
		id_=other.id_;
		dot_=other.dot_;
	}
	bool operator ==(const parser_unit<_Tp,_SentenceEnum>& other) const {
		if (right_ops_.size()!=other.right_ops_.size()) return false;
		for (int i=0;i<right_ops_.size();i++) {
			if (right_ops_[i]!=other.right_ops_[i]) return false;
		}
		if (dot_!=other.dot_) return false;
		return left_op_==other.left_op_;
	}
	bool operator !=(const parser_unit<_Tp,_SentenceEnum>& other) const {
		return !((*this)==other);
	}
	std::string to_string() {
		std::string result=std::to_string(id_)+":"+std::to_string(left_op_)+"->";
		for (int i=0;i<right_ops_.size();i++) {
			if (i==dot_) result+="· ";
			result+=std::to_string(right_ops_[i])+((i==right_ops_.size()-1)?"":" ");
		}
		if (dot_==right_ops_.size()) result+="·";
		return result;
	}
};

#if __cplusplus>=_STDEX_CPP17_VERSION
template <typename _Tp,typename _SentenceEnum=int>
parser_unit<_Tp,_SentenceEnum> single_parser_unit(_Tp left,std::initializer_list<_Tp> rights,_SentenceEnum id=(_SentenceEnum)0) {
	if (!rights.size()) throw std::invalid_argument("Invalid size of rights");
	parser_unit<_Tp,_SentenceEnum> result;
	result.left_op_=left;
	result.right_ops_=std::vector<_Tp>(rights);
	result.id_=id;
	result.dot_=-1;
	return result;
}
#endif

template <typename _Tp,typename _SentenceEnum=int>
class parser_listener {
public:
	enum error_type {
		ET_ERROR,
		ET_DEFAULT,
		ET_UNKNOWN,
	};
	bool enabled_;
	virtual int on_shift(uintptr_t id,int state,_Tp word)=0;
	virtual int on_reduction(uintptr_t id,int state,int next,_SentenceEnum sentence_id,int reduction_num)=0;
	virtual void on_accept()=0;
	virtual int on_error(uintptr_t id,error_type type,int state,_Tp word)=0;
};

template <typename _Tp,typename _SentenceEnum=int>
class parser {
#define _STDEX_PARSER_HAS_VALUE(array,value) (std::find(array.begin(),array.end(),value)!=array.end())
#define _STDEX_PARSER_HAS_VALUE_I(array,value,i) (std::find(array.begin()+i,array.end(),value)!=array.end())
	using unit_type=parser_unit<_Tp,_SentenceEnum>;
public:
	std::vector<unit_type> units_;
	std::map<_Tp,std::vector<const unit_type*>> units_by_lhs_;
	std::map<_Tp,bool> ptrs_;
	_Tp start_;
	_Tp seperator_;
	_Tp epsilon_;
	_Tp eof_;
	uintptr_t start_unit_;
	std::vector<parser_listener<_Tp,_SentenceEnum>*> listeners_;
	parser() {
		start_=(_Tp)-1;
		seperator_=(_Tp)-1;
		epsilon_=(_Tp)-1;
		eof_=(_Tp)-1;
	}
	parser(_Tp start,_Tp seperator,_Tp epsilon,_Tp eof) : start_(start) , seperator_(seperator) , epsilon_(epsilon) , eof_(eof) { }
	parser(_Tp start,_Tp seperator,_Tp epsilon,_Tp eof,std::vector<unit_type> units) : start_(start) , seperator_(seperator) , epsilon_(epsilon) , eof_(eof) , units_(units) { }
#if __cplusplus>=_STDEX_CPP17_VERSION
	parser(std::initializer_list<std::variant<_Tp,std::vector<unit_type>,std::map<_Tp,bool>>> init_list) {
		if (init_list.size()<5 || init_list.size()>6) throw std::invalid_argument("The number of the initializer arguments for parser must be 5 or 6!");
		auto it=init_list.begin();
		if (std::holds_alternative<_Tp>(*it)) start_=std::get<_Tp>(*it++);
		else throw std::invalid_argument(std::string("The first argument for parser must be _Tp(")+std::string(typeid(_Tp).name())+std::string(") to represent OP_START!"));
		if (std::holds_alternative<_Tp>(*it)) seperator_=std::get<_Tp>(*it++);
		else throw std::invalid_argument(std::string("The second argument for parser must be _Tp(")+std::string(typeid(_Tp).name())+std::string(") to represent OP_SEPERATOR!"));
		if (std::holds_alternative<_Tp>(*it)) epsilon_=std::get<_Tp>(*it++);
		else throw std::invalid_argument(std::string("The third argument for parser must be _Tp(")+std::string(typeid(_Tp).name())+std::string(") to represent OP_EPSILON(ε)!"));
		if (std::holds_alternative<_Tp>(*it)) eof_=std::get<_Tp>(*it++);
		else throw std::invalid_argument(std::string("The fourth argument for parser must be _Tp(")+std::string(typeid(_Tp).name())+std::string(") to represent OP_EOF(end sign)!"));
		if (std::holds_alternative<std::vector<unit_type>>(*it)) units_=std::get<std::vector<unit_type>>(*it++);
		else throw std::invalid_argument(std::string("The fifth argument for parser must be std:vector<unit_type>(")+std::string(typeid(std::vector<unit_type>).name())+std::string(") to give grammars!"));
		if (init_list.size()!=5) {
			if (std::holds_alternative<std::map<_Tp,bool>>(*it)) ptrs_=std::get<std::map<_Tp,bool>>(*it++);
			else throw std::invalid_argument(std::string("The sixth argument for parser must be std::map<_Tp,bool>(")+std::string(typeid(std::map<_Tp,bool>).name())+std::string(") to give ptr\'s infos!"));
		}
	}
#endif
	~parser() {
		for (auto* it:lr_node_list_) delete it;
	}

private:
	class lr_node {
	public:
		uintptr_t id_;
		std::vector<unit_type> unit_list_;
		std::unordered_map<_Tp,lr_node*> edges_;
		lr_node(uintptr_t id) : id_(id) {}

		bool operator ==(const lr_node& other) const {
			if (unit_list_.size()!=other.unit_list_.size()) return false;
			for (uintptr_t i=0;i<unit_list_.size();i++) {
				if (unit_list_[i]!=other.unit_list_[i]) return false;
			}
			return true;
			//return edges_==other.edges_;
		}
		bool operator !=(const lr_node& other) const {
			return !((*this)==other);
		}
	};
	struct lr_node_hash {
		std::size_t operator ()(lr_node* node) const {
			std::size_t h=0;
			static uintptr_t magic=0x9E3779B9;
			for (const auto& it:node->unit_list_) {
				h^=std::hash<_Tp>{}(it.left_op_)+magic+(h<<6)+(h>>2);
				for (const auto& jt:it.right_ops_) h^=std::hash<_Tp>{}(jt)+magic+(h<<6)+(h>>2);
				h^=std::hash<_SentenceEnum>{}(it.id_)+magic+(h<<6)+(h>>2);
			}
			return h;
		}
	};
	struct lr_node_equal {
		bool operator ()(lr_node* lhs,lr_node* rhs) const {
			if (lhs==rhs) return true;
			return *lhs==*rhs;
		}
	};
	std::unordered_set<lr_node*,lr_node_hash,lr_node_equal> lr_node_list_;
	std::map<_Tp,std::unordered_set<_Tp>> first_set_;
	std::map<_Tp,std::unordered_set<_Tp>> follow_set_;
	std::unordered_map<_Tp,std::unordered_set<std::shared_ptr<unit_type>>> closures_;
	class sheet_node {
	public:
		sheet_type type_=ST_ERROR;
		union node_info {
			std::shared_ptr<lr_node>* lr_ptr_;
			std::shared_ptr<unit_type>* unit_ptr_;
		} next_;
		sheet_node() {
			next_.lr_ptr_=nullptr;
			type_=ST_ERROR;
		}
		~sheet_node() {
			if (next_.lr_ptr_) {
				if (type_==ST_REDUCTION || type_==ST_ERROR) delete next_.unit_ptr_;
				else delete next_.lr_ptr_;
			}
		}
	};
	std::map<std::pair<_Tp,uintptr_t>,sheet_node> lr_sheet_;

private:
	void calculate_closures(_Tp op) {
		if (!ptrs_[op]) return;
		if (closures_[op].size()) return;
		auto it=units_by_lhs_.find(op);
		if (it!=units_by_lhs_.end()) {
			std::unordered_set<_Tp> rights;
			for (auto& jt:it->second) {
				auto insert_result=closures_[op].insert(std::make_shared<unit_type>(*jt));
				int i=0;
				while (i<jt->right_ops_.size() && jt->right_ops_[i]==epsilon_) i++;
				if (i<jt->right_ops_.size()) rights.insert(jt->right_ops_[i]);
				if (insert_result.second) (*insert_result.first)->dot_=i;
				//std::vector<_Tp> sep={seperator_};
				//p->right_ops_.insert(p->right_ops_.begin()+i,sep.begin(),sep.end());
			}
			for (auto& jt:rights) {
				calculate_closures(jt);
				closures_[op].insert(closures_[jt].begin(),closures_[jt].end());
			}
		}
	}
	lr_node* generate_lr_node(std::vector<unit_type> starts,uintptr_t& node_amount) {
		lr_node* curr_node=new lr_node(node_amount++);
		std::unordered_set<std::shared_ptr<unit_type>> temp_set;
		curr_node->unit_list_=starts;
		for (auto& it:curr_node->unit_list_) {
			it.dot_++;
			if (it.dot_<it.right_ops_.size()) temp_set.insert(closures_[it.right_ops_[it.dot_]].begin(),closures_[it.right_ops_[it.dot_]].end());
		}
		for (auto& it:temp_set) curr_node->unit_list_.push_back(*it);
		auto [it,inserted]=lr_node_list_.insert(curr_node);
		if (!inserted) {
			delete curr_node;
			node_amount--;
			return *it;
		}
		std::unordered_map<_Tp,std::vector<unit_type>> temp_map;
		for (auto& it:curr_node->unit_list_) {
			if (it.dot_<it.right_ops_.size()) temp_map[it.right_ops_[it.dot_]].push_back(it);
		}
		for (auto& it:temp_map) curr_node->edges_[it.first]=generate_lr_node(it.second,node_amount);
		return curr_node;
	}
	void calculate_first() {
		std::unordered_map<_Tp,std::unordered_set<_Tp>> ptr_set,nptr_set;//ptr is feizhong
		for (auto& it:units_) {
			int i=0;
			bool skip=false;
			int length=it.right_ops_.size();
			while (i<length && !skip) {
				if (it.right_ops_[i]==epsilon_) nptr_set[it.left_op_].insert(epsilon_);
				else {
					skip=true;
					if (ptrs_[it.right_ops_[i]]) ptr_set[it.left_op_].insert(it.right_ops_[i]);
					else nptr_set[it.left_op_].insert(it.right_ops_[i]);
				}
				i++;	
			}
		}
		//auto result=kosaraju_plus<_Tp>(ptr_set);
		//std::unordered_map<std::unordered_set<_Tp>*,std::unordered_set<_Tp>> first_sets;
		/*for (auto& it:result) {
			reverse(it.begin(),it.end());
			for (int i=0;i<it.size();i++) {
				if (i!=0) first_sets[&it[i]].insert(first_sets[&it[i-1]].begin(),first_sets[&it[i-1]].end());
				for (auto& jt:it[i]) first_sets[&it[i]].insert(nptr_set[jt].begin(),nptr_set[jt].end());
			}
		}*/
		auto first_sets=inverse_topology_closure<_Tp>(ptr_set,[&](_Tp node) -> std::unordered_set<_Tp>&{
			return nptr_set[node];
		});
		for (auto& it:first_sets) first_set_[it.first]=it.second;
		for (auto& it:ptrs_) {
			if (!it.second) first_set_[it.first].insert(it.first);	
		}
		return;
	}
	bool calculate_follow(unit_type& unit) {
		bool result=false;
		if (ptrs_[unit.right_ops_[unit.right_ops_.size()-1]]) {
			if (unit.right_ops_[unit.right_ops_.size()-1]!=unit.left_op_) {
				int size=follow_set_[unit.right_ops_[unit.right_ops_.size()-1]].size();
				follow_set_[unit.right_ops_[unit.right_ops_.size()-1]].insert(follow_set_[unit.left_op_].begin(),follow_set_[unit.left_op_].end());
				result=size!=follow_set_[unit.right_ops_[unit.right_ops_.size()-1]].size();
			}
		}
		bool e_stand=true;
		int start_index=unit.right_ops_.size()-2,end_index=start_index+1;
		while (start_index>=0) {
			if (ptrs_[unit.right_ops_[start_index]]) {
				for (int i=start_index+1;i<=end_index;i++) {
					for (auto it:first_set_[unit.right_ops_[i]]) {
						if (it!=epsilon_) result=follow_set_[unit.right_ops_[start_index]].insert(it).second;
					}
				}
				if (first_set_[unit.right_ops_[start_index]].find(epsilon_)==first_set_[unit.right_ops_[start_index]].end()) end_index=start_index;
				if (e_stand && first_set_[unit.right_ops_[start_index+1]].find(epsilon_)==first_set_[unit.right_ops_[start_index+1]].end()) e_stand=false;
				if (e_stand) {
					int size=follow_set_[unit.right_ops_[start_index]].size();
					follow_set_[unit.right_ops_[start_index]].insert(follow_set_[unit.left_op_].begin(),follow_set_[unit.left_op_].end());
					result=size!=follow_set_[unit.right_ops_[start_index]].size();
				}
			} else {
				end_index=start_index;
				e_stand=false;
			}
			start_index--;
		}
		return result;
	}
	void calculate_follow() {
		//ANOTHER METHOD IS:
		//FROM START_ UNIT,INSERT EVERY RIGHT_OP TO QUEUE
		//AND EXPAND EVERY UNIT WHERE CURRENT_OP IS ON LEFT
		//CONVERT QUEUE TO VECTOR(NO NEED REVERSE?)
		auto start_unit=units_[start_unit_];
		for (auto it:ptrs_) follow_set_[it.first].clear();
		std::queue<_Tp> wait_ptr_list;
		std::unordered_set<_Tp> wait_seen;
		for (auto it:start_unit.right_ops_) {
			if (ptrs_[it]) {
				wait_ptr_list.push(it);
				wait_seen.insert(it);
			}
		}
		//WHEN USING THE ANOTHER METHOD,ONLY PUSH START TO WAIT_PTR_LIST
		//BUT IF REVERSE IS NEEDED,THE CURRENT CODE IS NOT AVAILABLE
		while (wait_ptr_list.size()) {
			_Tp current=wait_ptr_list.front();
			for (auto& it:units_) {
				if (!_STDEX_PARSER_HAS_VALUE(it.right_ops_,current)) continue;
				bool result=calculate_follow(it);
				for (auto jt:it.right_ops_) {
					if (ptrs_[jt] && jt!=current) {
						if (wait_seen.find(jt)==wait_seen.end() || result) {
							wait_ptr_list.push(jt);
							wait_seen.insert(jt);
						}
					}
				}	
			}
			for (auto& it:units_) {
				if (it.left_op_!=current) continue;
				bool result=calculate_follow(it);	
				for (auto jt:it.right_ops_) {
					if (ptrs_[jt] && jt!=current) {
						if (wait_seen.find(jt)==wait_seen.end() || result) {
							wait_ptr_list.push(jt);
							wait_seen.insert(jt);
						}
					}
				}
			}
			wait_ptr_list.pop();
		}
		//for (auto i:wait_ptr_list) cout<<i<<" ";
		//cout<<"\nEND FOLLOW\n";
	}
	void construct_table() {
		for (auto it:lr_node_list_) {
			for (auto jt:it->edges_) {
				lr_sheet_[std::make_pair(jt.first,it->id_)].next_.lr_ptr_=new std::shared_ptr<lr_node>(std::make_shared<lr_node>(*jt.second));
				if (!ptrs_[jt.first]) lr_sheet_[std::make_pair(jt.first,it->id_)].type_=ST_SHIFT;
			}
		}
		for (auto it:lr_node_list_) {
			uintptr_t i=0;
			for (auto jt:it->unit_list_) {
				if (jt.right_ops_.size()==jt.dot_) {
					unit_type temp_unit;
					temp_unit.left_op_=jt.left_op_;
					for (auto kt:jt.right_ops_) temp_unit.right_ops_.push_back(kt);
					temp_unit.dot_=-1;
					temp_unit.id_=jt.id_;
					for (auto kt:ptrs_) {
						_Tp current_ptr=kt.first;
						if (follow_set_[jt.left_op_].find(current_ptr)!=follow_set_[jt.left_op_].end()) {
							if (lr_sheet_[std::make_pair(current_ptr,it->id_)].next_.lr_ptr_) {
								if (ptrs_[current_ptr]) throw std::logic_error("Conflict GOTO and REDUCTION at production "+std::to_string(i)+"("+jt.to_string()+") with GT("+std::to_string(it->id_)+","+std::to_string(current_ptr)+")");
								else {
									if (lr_sheet_[std::make_pair(current_ptr,it->id_)].type_==ST_SHIFT) throw std::logic_error("Conflict SHIFT and REDUCTION at production "+std::to_string(i)+"("+jt.to_string()+") with SHIFT("+std::to_string(it->id_)+","+std::to_string(current_ptr)+")");
									else throw std::logic_error("Conflict REDUCTION and REDUCTION at production "+std::to_string(i)+"("+jt.to_string()+") with REDUCTION("+std::to_string(it->id_)+","+std::to_string(current_ptr)+")");
								}
							}
							lr_sheet_[std::make_pair(current_ptr,it->id_)].type_=ST_REDUCTION;
						}
						lr_sheet_[std::make_pair(current_ptr,it->id_)].next_.unit_ptr_=new std::shared_ptr<unit_type>(std::make_shared<unit_type>(temp_unit));
					}
				}
				i++;
			}
			if (it->unit_list_.size()==1) {
				unit_type* temp_unit=new unit_type;
				temp_unit->left_op_=it->unit_list_[0].left_op_;
				for (auto jt:it->unit_list_[0].right_ops_) temp_unit->right_ops_.push_back(jt);
				temp_unit->dot_=-1;
				if ((*temp_unit)==(units_[start_unit_])) {
					for (auto jt:lr_node_list_) {
						for (auto kt:jt->edges_) {
							if (kt.second->id_==it->id_) lr_sheet_[std::make_pair(eof_,jt->id_)].type_=ST_ACCEPT;
						}
					}
				}
			}
		}
	}
public:
	void generate_parser(bool auto_ptr=true) {
		//examine ptr include seperator/start/units
		//ExamineStartSentence();
		uintptr_t node_amount=0;
		if (auto_ptr) {
			ptrs_.clear();
			for (auto it:units_) {
				ptrs_[it.left_op_]|=true;
				for (auto jt:it.right_ops_) ptrs_[jt]|=false;
			}
		}
		units_by_lhs_.clear();
		for (const auto& it:units_) units_by_lhs_[it.left_op_].push_back(&it);
		closures_.clear();
		for (auto& it:ptrs_) {
			if (it.second) calculate_closures(it.first);
		}
		for (auto* it:lr_node_list_) delete it;
		lr_node_list_.clear();
		auto it=units_by_lhs_.find(start_);
		if (it==units_by_lhs_.end()) return;
		std::vector<unit_type> start_units;
		for (auto jt:it->second) start_units.push_back(*jt);
		generate_lr_node(start_units,node_amount);
#ifdef _STDEX_OUTPUT_PARSER
		auto lr_sort=[](lr_node* lhs,lr_node* rhs) {
			return lhs->id_<rhs->id_;
		};
		std::set<lr_node*,decltype(lr_sort)> lr_output(lr_sort);
		for (auto it:lr_node_list_) lr_output.insert(it);
		for (auto it:lr_output) {
			_STDEX_OUTPUT_PARSER<<it->id_<<":"<<std::endl;
			_STDEX_OUTPUT_PARSER<<"  units:"<<std::endl;
			for (auto jt:it->unit_list_) {
				_STDEX_OUTPUT_PARSER<<"    "<<jt.left_op_<<"->";
				for (auto kt:jt.right_ops_) _STDEX_OUTPUT_PARSER<<kt<<" ";
				_STDEX_OUTPUT_PARSER<<std::endl;
			}
			_STDEX_OUTPUT_PARSER<<"  edges:"<<std::endl;
			for (auto jt:it->edges_) _STDEX_OUTPUT_PARSER<<"    "<<jt.first<<"->"<<jt.second->id_<<std::endl;
			_STDEX_OUTPUT_PARSER<<std::endl;
		}
#endif
		first_set_.clear();
		calculate_first();
		follow_set_.clear();
		calculate_follow();
#ifdef _STDEX_OUTPUT_PARSER
		_STDEX_OUTPUT_PARSER<<"\nfirsts:\n\n";
		for (auto& it:ptrs_) {
			_STDEX_OUTPUT_PARSER<<it.first<<":\n  {";
			std::string temp_first;
			for (auto& jt:first_set_[it.first]) temp_first+=std::to_string((int)jt)+",";
			if (temp_first.size()) temp_first.pop_back();
			_STDEX_OUTPUT_PARSER<<temp_first<<"}\n";
		}
		_STDEX_OUTPUT_PARSER<<"\nfollows:\n\n";
		for (auto& it:ptrs_) {
			_STDEX_OUTPUT_PARSER<<it.first<<":\n  {";
			std::string temp_follow;
			for (auto& jt:follow_set_[it.first]) temp_follow+=std::to_string((int)jt)+",";
			if (temp_follow.size()) temp_follow.pop_back();
			_STDEX_OUTPUT_PARSER<<temp_follow<<"}\n";
		}
#endif
		lr_sheet_.clear();
		construct_table();
#ifdef _STDEX_OUTPUT_PARSER
		_STDEX_OUTPUT_PARSER<<"\n";
		for (auto& it:lr_sheet_) {
			std::vector<std::string> get_type={"e","r","s","a"};
			intptr_t id=-1;
			if (it.second.type_==ST_SHIFT || (it.second.type_==ST_ERROR && it.second.next_.lr_ptr_)) id=(*it.second.next_.lr_ptr_)->id_;
			else if (it.second.type_==ST_REDUCTION) {
				for (uintptr_t i=0;i<units_.size();i++) {
					if (units_[i]==**it.second.next_.unit_ptr_) id=i;
				}
			}
			_STDEX_OUTPUT_PARSER<<it.first.second<<"-"<<it.first.first<<"->"<<get_type[(int)it.second.type_]<<id<<std::endl;
		}
#endif
	}
public:
	class parse_node {
	public:
		_Tp op_;
		std::vector<parse_node> children_;
	};
private:
	void on_reduction(sheet_node* current_symbol,std::vector<uintptr_t>& parse_stack,uintptr_t& current_state,std::vector<parser_listener<_Tp,_SentenceEnum>*>& listeners,uintptr_t& current_id,std::vector<parse_node>& nodes,parse_node*& current_word) {
		int reduction_num=0;
		for (int i=0;i<(*current_symbol->next_.unit_ptr_)->right_ops_.size();i++) {
			if ((*current_symbol->next_.unit_ptr_)->right_ops_[i]!=epsilon_) reduction_num++;
		}
		for (int i=0;i<reduction_num && !parse_stack.empty();i++) parse_stack.pop_back();
		current_state=parse_stack.empty()?0:parse_stack.back();
		parse_stack.push_back((*lr_sheet_[std::make_pair((*current_symbol->next_.unit_ptr_)->left_op_,current_state)].next_.lr_ptr_)->id_);
		_SentenceEnum id=(*current_symbol->next_.unit_ptr_)->id_;
		intptr_t skips=0;
		for (auto it:listeners) skips=std::max(skips,it->on_reduction(current_id,current_state,(*lr_sheet_[std::make_pair((*current_symbol->next_.unit_ptr_)->left_op_,current_state)].next_.lr_ptr_)->id_,id,reduction_num));//move_up?
		current_id+=skips;
		if (skips) current_word=((intptr_t)current_id>nodes.size()||(intptr_t)current_id<1)?nullptr:&nodes[current_id-1];
	}
public:
	bool parse_with_listener(std::vector<parse_node> nodes) {
		std::vector<parser_listener<_Tp,_SentenceEnum>*> listeners;
		for (auto it:listeners_) {
			if (it->enabled_) listeners.push_back(it);
		}
		std::vector<uintptr_t> parse_stack;
		std::vector<parse_node> nodes_stack;
		parse_stack.push_back(0);
		uintptr_t current_id=0;
		parse_node* current_word=&nodes[current_id++];
		while (!parse_stack.empty()) {
			uintptr_t current_state=parse_stack.back();
			sheet_node* current_symbol=&lr_sheet_[std::make_pair(current_word->op_,current_state)];
			switch (current_symbol->type_) {
				case ST_SHIFT: {
					parse_stack.push_back((*current_symbol->next_.lr_ptr_)->id_);
					//nodes_stack.push_back(*current_word);
					intptr_t skips=0;
					for (auto it:listeners) skips=std::max(skips,it->on_shift(current_id,(*current_symbol->next_.lr_ptr_)->id_,current_word->op_));
					current_id+=skips;
					current_word=((intptr_t)current_id>=nodes.size())?nullptr:&nodes[current_id++];
					break;
				}
				case ST_REDUCTION: {
					on_reduction(current_symbol,parse_stack,current_state,listeners,current_id,nodes,current_word);
					break;
				}
				case ST_ACCEPT: {
					for (auto it:listeners) it->on_accept();
					return true;
				}
				case ST_ERROR:
				default: {
					bool error_continue=true;
					std::vector<parser_listener<_Tp,_SentenceEnum>*> temp_listener;
					for (auto it:listeners) {
						int result=it->on_error(current_id,current_symbol->type_==ST_ERROR?parser_listener<_Tp,_SentenceEnum>::ET_ERROR:parser_listener<_Tp,_SentenceEnum>::ET_DEFAULT,current_state,current_word->op_);
						if (!(result&1)) error_continue=false;
						if (result&2) temp_listener.push_back(it);
					}
					if (temp_listener.size()) on_reduction(current_symbol,parse_stack,current_state,temp_listener,current_id,nodes,current_word);
					if (!error_continue || current_id>=nodes.size()) return false;
					if (temp_listener.empty()) current_word=&nodes[current_id++];
					break;
				}
			}
		}
		for (auto it:listeners) it->on_error((uintptr_t)-1,parser_listener<_Tp,_SentenceEnum>::ET_UNKNOWN,0,(_Tp)-1);
		return false;
	}
	
	friend std::ostream& operator <<(std::ostream& os,const parser& p) {
		auto write_int=[&](auto v){
			os.write(reinterpret_cast<const char*>(&v),4);
		};
	    std::size_t n=p.units_.size();
		write_int(n);
		for (const auto& it:p.units_) {
			write_int(it.left_op_);
			std::size_t size=it.right_ops_.size();
			write_int(size);
			for (auto jt:it.right_ops_) write_int(jt);
			write_int(it.id_);
		}
		std::size_t m=p.lr_sheet_.size();
		write_int(m);
		for (const auto& [key,sn]:p.lr_sheet_) {
			write_int(key.first);
			write_int(key.second);
			write_int(sn.type_);
			if (sn.next_.lr_ptr_) {
				write_int(true);
				if (sn.type_==ST_SHIFT) {
					int nextid=(*sn.next_.lr_ptr_)->id_;
					write_int(nextid);
				} else if (sn.type_==ST_REDUCTION || sn.type_==ST_ERROR) {
					int uid=(*sn.next_.unit_ptr_)->id_;
	            	write_int(uid);
	        	}
			} else write_int(false);
		}
		if (!os) os.setstate(std::ios::failbit);
		return os;
	}
	
	friend std::istream& operator >>(std::istream& is,parser& p) {
		auto read_int=[&](auto& v){
			is.read(reinterpret_cast<char*>(&v),4);
			if (!is) is.setstate(std::ios::failbit);
		};
		std::size_t n;
		read_int(n);
		if (!is) return is;
		p.units_.clear();
		for (std::size_t i=0;i<n;i++) {
			unit_type u;
			read_int(u.left_op_);
			if (!is) return is;
			std::size_t size;
			read_int(size);
			if (!is) return is;
			u.right_ops_.resize(size);
			for (std::size_t j=0;j<size;j++) {
				read_int(u.right_ops_[j]);
				if (!is) return is;
			}
			read_int(u.id_);
			if (!is) return is;
			p.units_.push_back(u);
		}
		std::size_t m;
		read_int(m);
		if (!is) return is;
		p.lr_sheet_.clear();
		for (std::size_t i=0;i<m;i++) {
	        _Tp sym; int state; int type;
	        read_int(sym); read_int(state); read_int(type);
	        sheet_node sn;
	        sn.type_=(sheet_type)type;
	        bool has_ptr;
			read_int(has_ptr);
			if (!is) return is;
			if (has_ptr) {
			    if (sn.type_==ST_SHIFT) {
					int nextid;
					read_int(nextid);
					if (!is) return is;
					sn.next_.lr_ptr_=new std::shared_ptr<lr_node>(std::make_shared<lr_node>(nextid));
				} else if (sn.type_==ST_REDUCTION || sn.type_==ST_ERROR) {
					int uid;
					read_int(uid);
					if (!is) return is;
					sn.next_.unit_ptr_=new std::shared_ptr<unit_type>(std::make_shared<unit_type>(p.units_[uid]));
				}	
			}
			p.lr_sheet_[{sym,state}]=std::move(sn);
		}
		return is;
	}
#undef _STDEX_PARSER_HAS_VALUE_I
#undef _STDEX_PARSER_HAS_VALUE
};

}

}

#endif