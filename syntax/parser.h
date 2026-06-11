//Last Modified At 2026/06/11
//@Version 3.4.0.0
#ifndef _STDEX_SYNTAX_PARSER_H_
#define _STDEX_SYNTAX_PARSER_H_ 1

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <initializer_list>
#include <map>
#include <memory>
#include <queue>
#include <set>
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
	_Tp left_op;
	std::vector<_Tp> right_ops;
	_SentenceEnum id;
	int dot;
	parser_unit() {
		dot=-1;
	}
	parser_unit(const parser_unit& other) {
		left_op=other.left_op;
		right_ops=other.right_ops;
		id=other.id;
		dot=other.dot;
	}
	bool operator ==(const parser_unit<_Tp,_SentenceEnum>& other) const {
		return left_op==other.left_op && dot==other.dot && right_ops==other.right_ops;
	}
	bool operator !=(const parser_unit<_Tp,_SentenceEnum>& other) const {
		return !((*this)==other);
	}
	bool operator <(const parser_unit<_Tp,_SentenceEnum>& other) const {
		if (left_op!=other.left_op) return left_op<other.left_op;
		if (right_ops.size()!=other.right_ops.size()) return right_ops.size()<other.right_ops.size();
		for (std::size_t i=0;i<right_ops.size();i++) {
			if (right_ops[i]!=other.right_ops[i]) return right_ops[i]<other.right_ops[i];
		}
		return dot<other.dot;
	}
	std::string to_string() {
		std::string result=std::to_string(id)+":"+std::to_string(left_op)+"->";
		for (std::size_t i=0;i<right_ops.size();i++) {
			if (i==dot) result+="· ";
			result+=std::to_string(right_ops[i])+((i==right_ops.size()-1)?"":" ");
		}
		if (dot==right_ops.size()) result+="·";
		return result;
	}
};

#if __cplusplus>=_STDEX_CPP17_VERSION
template <typename _Tp,typename _SentenceEnum=int>
parser_unit<_Tp,_SentenceEnum> single_parser_unit(_Tp left,std::initializer_list<_Tp> rights,_SentenceEnum id=(_SentenceEnum)0) {
	if (!rights.size()) throw std::invalid_argument("Invalid size of rights");
	parser_unit<_Tp,_SentenceEnum> result;
	result.left_op=left;
	result.right_ops=std::vector<_Tp>(rights);
	result.id=id;
	result.dot=-1;
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
	bool enabled;
	virtual intptr_t on_shift(uintptr_t id,int state,_Tp word)=0;
	virtual intptr_t on_reduction(uintptr_t id,int state,int next,_SentenceEnum sentence_id,int reduction_num)=0;
	virtual void on_accept()=0;
	virtual int on_error(uintptr_t id,error_type type,int state,_Tp word)=0;
};

template <typename _Tp,typename _SentenceEnum=int>
class parser {
#define _STDEX_PARSER_HAS_VALUE(array,value) (std::find(array.begin(),array.end(),value)!=array.end())
#define _STDEX_PARSER_HAS_VALUE_I(array,value,i) (std::find(array.begin()+i,array.end(),value)!=array.end())
protected:
	using unit_type=parser_unit<_Tp,_SentenceEnum>;
public:
	std::vector<unit_type> units;
	std::map<_Tp,std::vector<const unit_type*>> units_by_lhs;
	std::map<_Tp,bool> ptrs;
	_Tp start;
	_Tp epsilon;
	_Tp eof;
	std::vector<parser_listener<_Tp,_SentenceEnum>*> listeners;
	parser() {
		start=(_Tp)-1;
		epsilon=(_Tp)-1;
		eof=(_Tp)-1;
	}
	parser(_Tp start,_Tp epsilon,_Tp eof) : start(start) , epsilon(epsilon) , eof(eof) { }
	parser(_Tp start,_Tp seperator,_Tp epsilon,_Tp eof) : start(start) , epsilon(epsilon) , eof(eof) { }
	parser(_Tp start,_Tp epsilon,_Tp eof,std::vector<unit_type> units) : start(start) , epsilon(epsilon) , eof(eof) , units(units) { }
	parser(_Tp start,_Tp seperator,_Tp epsilon,_Tp eof,std::vector<unit_type> units) : start(start) , epsilon(epsilon) , eof(eof) , units(units) { }
#if __cplusplus>=_STDEX_CPP17_VERSION
	parser(std::initializer_list<std::variant<_Tp,std::vector<unit_type>,std::map<_Tp,bool>>> init_list) {
		if (init_list.size()<4 || init_list.size()>6) throw std::invalid_argument("The amount of the initializer arguments for parser must be 4,5 or 6");
		auto it=init_list.begin();
		bool has_seperator=!(std::holds_alternative<std::vector<unit_type>>(*(init_list.begin()+4)));
		if (has_seperator && init_list.size()==4) throw std::invalid_argument("The amount of the old version of the initializer arguments for parser must be 5 or 6");
		if (!has_seperator && init_list.size()==6) throw std::invalid_argument("The amount of the new version of the initializer arguments for parser must be 4 or 5");
		if (std::holds_alternative<_Tp>(*it)) start=std::get<_Tp>(*it++);
		else throw std::invalid_argument(std::string("The first argument for parser must be _Tp(")+std::string(typeid(_Tp).name())+std::string(") to represent OP_START"));
		if (has_seperator) it++;
		//if (std::holds_alternative<_Tp>(*it)) seperator=std::get<_Tp>(*it++);
		//else throw std::invalid_argument(std::string("The second argument for parser must be _Tp(")+std::string(typeid(_Tp).name())+std::string(") to represent OP_SEPERATOR"));
		if (std::holds_alternative<_Tp>(*it)) epsilon=std::get<_Tp>(*it++);
		else throw std::invalid_argument(std::string("The third argument for parser must be _Tp(")+std::string(typeid(_Tp).name())+std::string(") to represent OP_EPSILON(ε)"));
		if (std::holds_alternative<_Tp>(*it)) eof=std::get<_Tp>(*it++);
		else throw std::invalid_argument(std::string("The fourth argument for parser must be _Tp(")+std::string(typeid(_Tp).name())+std::string(") to represent OP_EOF(end sign)"));
		if (std::holds_alternative<std::vector<unit_type>>(*it)) units=std::get<std::vector<unit_type>>(*it++);
		else throw std::invalid_argument(std::string("The fifth argument for parser must be std:vector<unit_type>(")+std::string(typeid(std::vector<unit_type>).name())+std::string(") to give grammars"));
		if (init_list.size()>(int)has_seperator+4) {
			if (std::holds_alternative<std::map<_Tp,bool>>(*it)) ptrs=std::get<std::map<_Tp,bool>>(*it++);
			else throw std::invalid_argument(std::string("The sixth argument for parser must be std::map<_Tp,bool>(")+std::string(typeid(std::map<_Tp,bool>).name())+std::string(") to give ptr\'s infos"));
		}
	}
#endif
	~parser() {
		for (auto* it:lr_node_list) delete it;
	}

	constexpr static std::size_t magic=0x9E3779B9;
	static void hash_combine(std::size_t& seed,std::size_t value) noexcept {
		seed^=value+magic+(seed<<6)+(seed>>2);
	}
	struct unit_type_hash {
		std::size_t operator ()(unit_type* unit) const {
			std::size_t h=0;
			hash_combine(h,std::hash<intptr_t>{}(unit->left_op));
			for (auto& it:unit->right_ops) hash_combine(h,std::hash<intptr_t>{}(it));
			hash_combine(h,std::hash<int>{}(unit->dot));
			return h;
		}
	};
	struct unit_type_equal {
		bool operator ()(unit_type* lhs,unit_type* rhs) const {
			if (lhs==rhs) return true;
			return *lhs==*rhs;
		}
	};
	class lr_node {
	public:
		uintptr_t id;
		std::vector<unit_type> unit_list;
		std::unordered_map<_Tp,lr_node*> edges;
		lr_node(uintptr_t id) : id(id) { }
		
		bool operator ==(const lr_node& other) const {
			if (unit_list.size()!=other.unit_list.size()) return false;
			for (std::size_t i=0;i<unit_list.size();i++) {
				if (unit_list[i]!=other.unit_list[i]) return false;
			}
			return true;
		}
		bool operator !=(const lr_node& other) const {
			return !((*this)==other);
		}
	};
	struct lr_node_hash {
		std::size_t operator ()(lr_node* node) const {
			return 0;
			std::size_t h=0;
			std::unordered_set<unit_type*,unit_type_hash,unit_type_equal> x0;
			for (auto& it:node->unit_list) x0.insert((unit_type*)&it);
			for (auto& it:x0) hash_combine(h,unit_type_hash{}(it));
			return h;
		}
	};
	struct lr_node_equal {
		bool operator ()(lr_node* lhs,lr_node* rhs) const {
			if (lhs==rhs) return true;
			return *lhs==*rhs;
		}
	};
	std::unordered_set<lr_node*,lr_node_hash,lr_node_equal> lr_node_list;
	std::map<_Tp,std::unordered_set<_Tp>> first_set;
	std::map<_Tp,std::unordered_set<_Tp>> follow_set;
	std::unordered_map<_Tp,std::unordered_set<std::shared_ptr<unit_type>>> closures;
	class sheet_node {
	public:
		sheet_type type=ST_ERROR;
		union node_info {
			std::shared_ptr<lr_node>* lr_ptr;
			std::shared_ptr<unit_type>* unit_ptr;
		} next;
		sheet_node() {
			next.lr_ptr=nullptr;
			type=ST_ERROR;
		}
		~sheet_node() {
			if (next.lr_ptr) {
				if (type==ST_REDUCTION || type==ST_ERROR) delete next.unit_ptr;
				else delete next.lr_ptr;
			}
		}
	};
	std::map<std::pair<_Tp,uintptr_t>,sheet_node> lr_sheet;

private:
	void calculate_closures(_Tp op) {
		if (!ptrs[op]) return;
		if (closures[op].size()) return;
		auto it=units_by_lhs.find(op);
		if (it!=units_by_lhs.end()) {
			std::unordered_set<_Tp> rights;
			for (auto& jt:it->second) {
				auto insert_result=closures[op].insert(std::make_shared<unit_type>(*jt));
				std::size_t i=0;
				while (i<jt->right_ops.size() && jt->right_ops[i]==epsilon) i++;
				if (i<jt->right_ops.size()) rights.insert(jt->right_ops[i]);
				if (insert_result.second) (*insert_result.first)->dot=i;
				//std::vector<_Tp> sep={seperator};
				//p->right_ops.insert(p->right_ops.begin()+i,sep.begin(),sep.end());
			}
			for (auto& jt:rights) {
				calculate_closures(jt);
				closures[op].insert(closures[jt].begin(),closures[jt].end());
			}
		}
	}
protected:
	virtual void generate_initialize() {
		units_by_lhs.clear();
		for (const auto& it:units) units_by_lhs[it.left_op].push_back(&it);
		closures.clear();
		for (auto& it:ptrs) {
			if (it.second) calculate_closures(it.first);
		}
		for (auto* it:lr_node_list) delete it;
		lr_node_list.clear();
		first_set.clear();
		follow_set.clear();
		lr_sheet.clear();
	}
	lr_node* generate_lr_node(std::vector<unit_type> starts,uintptr_t& node_amount) {
		lr_node* curr_node=new lr_node(node_amount++);
		std::unordered_set<std::shared_ptr<unit_type>> temp_set;
		curr_node->unit_list=starts;
		for (auto& it:curr_node->unit_list) {
			it.dot++;
			if (it.dot<it.right_ops.size()) temp_set.insert(closures[it.right_ops[it.dot]].begin(),closures[it.right_ops[it.dot]].end());
		}
		for (auto& it:temp_set) curr_node->unit_list.push_back(*it);
		std::sort(curr_node->unit_list.begin(),curr_node->unit_list.end());
		auto [it,inserted]=lr_node_list.insert(curr_node);
		if (!inserted) {
			delete curr_node;
			node_amount--;
			return *it;
		}
		std::unordered_map<_Tp,std::vector<unit_type>> temp_map;
		for (auto& it:curr_node->unit_list) {
			if (it.dot_<it.right_ops.size()) temp_map[it.right_ops[it.dot]].push_back(it);
		}
		for (auto& it:temp_map) curr_node->edges[it.first]=generate_lr_node(it.second,node_amount);
		return curr_node;
	}
	void calculate_first() {
		std::unordered_map<_Tp,std::unordered_set<_Tp>> ptr_set,nptr_set;
		for (auto& it:units) {
			std::size_t i=0;
			bool skip=false;
			std::size_t length=it.right_ops.size();
			while (i<length && !skip) {
				if (it.right_ops[i]==epsilon) nptr_set[it.left_op].insert(epsilon);
				else {
					skip=true;
					if (ptrs[it.right_ops[i]]) ptr_set[it.left_op].insert(it.right_ops[i]);
					else nptr_set[it.left_op].insert(it.right_ops[i]);
				}
				i++;	
			}
		}
		//auto result=kosaraju_plus<_Tp>(ptr_set);
		//std::unordered_map<std::unordered_set<_Tp>*,std::unordered_set<_Tp>> first_sets;
		/*for (auto& it:result) {
			reverse(it.begin(),it.end());
			for (std::size_t i=0;i<it.size();i++) {
				if (i!=0) first_sets[&it[i]].insert(first_sets[&it[i-1]].begin(),first_sets[&it[i-1]].end());
				for (auto& jt:it[i]) first_sets[&it[i]].insert(nptr_set[jt].begin(),nptr_set[jt].end());
			}
		}*/
		for (auto& it:nptr_set) {
			if (!ptr_set.count(it.first)) ptr_set[it.first].clear();
		}
		auto first_sets=inverse_topology_closure<_Tp>(ptr_set,[&](_Tp node)->std::unordered_set<_Tp>&{
			return nptr_set[node];
		});
		for (auto& it:first_sets) first_set[it.first]=it.second;
		for (auto& it:ptrs) {
			if (!it.second) first_set[it.first].insert(it.first);	
		}
		return;
	}
	bool calculate_follow(unit_type& unit) {
		bool result=false;
		if (ptrs[unit.right_ops[unit.right_ops.size()-1]]) {
			if (unit.right_ops[unit.right_ops.size()-1]!=unit.left_op) {
				std::size_t size=follow_set[unit.right_ops[unit.right_ops.size()-1]].size();
				follow_set[unit.right_ops[unit.right_ops.size()-1]].insert(follow_set[unit.left_op].begin(),follow_set[unit.left_op].end());
				result=size!=follow_set[unit.right_ops[unit.right_ops.size()-1]].size();
			}
		}
		bool e_stand=true;
		int start_index=unit.right_ops.size()-2,end_index=start_index+1;
		while (start_index>=0) {
			if (ptrs[unit.right_ops[start_index]]) {
				for (int i=start_index+1;i<=end_index;i++) {
					for (auto it:first_set[unit.right_ops[i]]) {
						if (it!=epsilon) result=follow_set[unit.right_ops[start_index]].insert(it).second;
					}
				}
				if (first_set[unit.right_ops[start_index]].find(epsilon)==first_set[unit.right_ops[start_index]].end()) end_index=start_index;
				if (e_stand && first_set[unit.right_ops[start_index+1]].find(epsilon)==first_set[unit.right_ops[start_index+1]].end()) e_stand=false;
				if (e_stand) {
					int size=follow_set[unit.right_ops[start_index]].size();
					follow_set[unit.right_ops[start_index]].insert(follow_set[unit.left_op].begin(),follow_set[unit.left_op].end());
					result=size!=follow_set[unit.right_ops[start_index]].size();
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
		auto it=units_by_lhs.find(start);
		std::vector<unit_type*> start_units;
		if (it!=units_by_lhs.end()) {
			for (auto& jt:it->second) {
				if (jt->right_ops.size() && jt->right_ops[jt->right_ops.size()-1]==eof) start_units.push_back(const_cast<unit_type*>(jt));
			}
		}
		for (auto it:ptrs) follow_set[it.first].clear();
		std::queue<_Tp> wait_ptr_list;
		std::unordered_set<_Tp> wait_seen;
		for (auto it:start_units) {
			for (auto jt:it->right_ops) {
				if (ptrs[jt]) {
					wait_ptr_list.push(jt);
					wait_seen.insert(jt);
				}
			}
		}
		//WHEN USING THE ANOTHER METHOD,ONLY PUSH START TO WAIT_PTR_LIST
		//BUT IF REVERSE IS NEEDED,THE CURRENT CODE IS NOT AVAILABLE
		while (wait_ptr_list.size()) {
			_Tp current=wait_ptr_list.front();
			for (auto& it:units) {
				if (!_STDEX_PARSER_HAS_VALUE(it.right_ops,current)) continue;
				bool result=calculate_follow(it);
				for (auto jt:it.right_ops) {
					if (ptrs[jt] && jt!=current) {
						if (wait_seen.find(jt)==wait_seen.end() || result) {
							wait_ptr_list.push(jt);
							wait_seen.insert(jt);
						}
					}
				}	
			}
			for (auto& it:units) {
				if (it.left_op!=current) continue;
				bool result=calculate_follow(it);	
				for (auto jt:it.right_ops) {
					if (ptrs[jt] && jt!=current) {
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
	virtual void construct_table() {
		auto it=units_by_lhs.find(start_);
		std::vector<unit_type*> start_units;
		if (it!=units_by_lhs.end()) {
			for (auto& jt:it->second) {
				if (jt->right_ops.size() && jt->right_ops[jt->right_ops.size()-1]==eof_) start_units.push_back(const_cast<unit_type*>(jt));
			}
		}
		for (auto& it:lr_node_list_) {
			for (auto jt:it->edges) {
				lr_sheet[std::make_pair(jt.first,it->id_)].next.lr_ptr=new std::shared_ptr<lr_node>(std::make_shared<lr_node>(*jt.second));
				if (!ptrs[jt.first]) lr_sheet[std::make_pair(jt.first,it->id)].type=ST_SHIFT;
			}
		}
		for (auto& it:lr_node_list) {
			uintptr_t i=0;
			for (auto jt:it->unit_list) {
				if (jt.right_ops.size()==jt.dot) {
					unit_type temp_unit;
					temp_unit.left_op=jt.left_op;
					for (auto kt:jt.right_ops) temp_unit.right_ops.push_back(kt);
					temp_unit.dot=-1;
					temp_unit.id=jt.id;
					for (auto kt:ptrs) {
						_Tp current_ptr=kt.first;
						if (follow_set[jt.left_op].find(current_ptr)!=follow_set[jt.left_op].end()) {
							if (lr_sheet[std::make_pair(current_ptr,it->id)].next.lr_ptr && lr_sheet[std::make_pair(current_ptr,it->id)].type!=ST_ERROR) {
								if (ptrs[current_ptr]) throw std::logic_error("Conflict GOTO and REDUCTION at production "+std::to_string(i)+"("+jt.to_string()+") with GT("+std::to_string(it->id)+","+std::to_string(current_ptr)+")");
								else {
									if (lr_sheet[std::make_pair(current_ptr,it->id)].type==ST_SHIFT) throw std::logic_error("Conflict SHIFT and REDUCTION at production "+std::to_string(i)+"("+jt.to_string()+") with SHIFT("+std::to_string(it->id)+","+std::to_string(current_ptr)+")");
									else throw std::logic_error("Conflict REDUCTION and REDUCTION at production "+std::to_string(i)+"("+jt.to_string()+") with REDUCTION("+std::to_string(it->id)+","+std::to_string(current_ptr)+")");
								}
							}
							lr_sheet[std::make_pair(current_ptr,it->id)].type=ST_REDUCTION;
						}
						if (!lr_sheet[std::make_pair(current_ptr,it->id)].next.unit_ptr) lr_sheet[std::make_pair(current_ptr,it->id)].next.unit_ptr=new std::shared_ptr<unit_type>(std::make_shared<unit_type>(temp_unit));
					}
				}
				i++;
			}
			if (it->unit_list.size()==1 && it->unit_list[0].right_ops.size() && it->unit_list[0].right_ops[it->unit_list[0].right_ops.size()-1]==eof) {
				unit_type temp_unit=unit_list[0];
				temp_unit->dot=-1;
				for (auto& jt:start_units) {
					if (*jt==temp_unit) {
						for (auto& kt:lr_node_list) {
							for (auto& lt:kt->edges) {
								if (lt.second->id==it->id) lr_sheet[std::make_pair(eof,kt->id)].type=ST_ACCEPT;
							}
						}
					}
				}
			}
		}
	}
public:
	virtual void generate_parser(bool auto_ptr=true) {
		//examine ptr include seperator/start/units
		//ExamineStartSentence();
		uintptr_t node_amount=0;
		if (auto_ptr) {
			ptrs.clear();
			for (auto it:units) {
				ptrs[it.left_op]|=true;
				for (auto jt:it.right_ops) ptrs[jt]|=false;
			}
		}
		generate_initialize();
		auto it=units_by_lhs.find(start);
		if (it==units_by_lhs.end()) return;
		std::vector<unit_type> start_units;
		for (auto jt:it->second) start_units.push_back(*jt);
		generate_lr_node(start_units,node_amount);
#ifdef _STDEX_OUTPUT_PARSER
		auto lr_sort=[](lr_node* lhs,lr_node* rhs){
			return lhs->id<rhs->id;
		};
		std::set<lr_node*,decltype(lr_sort)> lr_output(lr_sort);
		for (auto& it:lr_node_list) lr_output.insert(it);
		for (auto& it:lr_output) {
			_STDEX_OUTPUT_PARSER<<it->id<<":"<<std::endl;
			_STDEX_OUTPUT_PARSER<<"  units:"<<std::endl;
			for (auto jt:it->unit_list) {
				_STDEX_OUTPUT_PARSER<<"    "<<jt.left_op_<<"->";
				for (std::size_t i=0;i<jt.right_ops.size();i++) {
					if (i==jt.dot) _STDEX_OUTPUT_PARSER<<"· ";
					_STDEX_OUTPUT_PARSER<<jt.right_ops[i]<<" ";
				}
				if (jt.dot==jt.right_ops.size()) _STDEX_OUTPUT_PARSER<<"· ";
				_STDEX_OUTPUT_PARSER<<std::endl;
			}
			_STDEX_OUTPUT_PARSER<<"  edges:"<<std::endl;
			for (auto jt:it->edges_) _STDEX_OUTPUT_PARSER<<"    "<<jt.first<<"->"<<jt.second->id<<std::endl;
			_STDEX_OUTPUT_PARSER<<std::endl;
		}
#endif
		calculate_first();
		calculate_follow();
#ifdef _STDEX_OUTPUT_PARSER
		_STDEX_OUTPUT_PARSER<<"\nfirsts:\n\n";
		for (auto& it:ptrs_) {
			_STDEX_OUTPUT_PARSER<<it.first<<":\n  {";
			std::string temp_first;
			for (auto& jt:first_set[it.first]) temp_first+=std::to_string((int)jt)+",";
			if (temp_first.size()) temp_first.pop_back();
			_STDEX_OUTPUT_PARSER<<temp_first<<"}\n";
		}
		_STDEX_OUTPUT_PARSER<<"\nfollows:\n\n";
		for (auto& it:ptrs_) {
			_STDEX_OUTPUT_PARSER<<it.first<<":\n  {";
			std::string temp_follow;
			for (auto& jt:follow_set[it.first]) temp_follow+=std::to_string((int)jt)+",";
			if (temp_follow.size()) temp_follow.pop_back();
			_STDEX_OUTPUT_PARSER<<temp_follow<<"}\n";
		}
#endif
		construct_table();
#ifdef _STDEX_OUTPUT_PARSER
		_STDEX_OUTPUT_PARSER<<"\n";
		for (auto& it:lr_sheet_) {
			std::vector<std::string> get_type={"e","r","s","a"};
			intptr_t id=-1;
			if (it.second.type==ST_SHIFT || (it.second.type==ST_ERROR && it.second.next.lr_ptr)) id=(*it.second.next.lr_ptr)->id;
			else if (it.second.type==ST_REDUCTION) {
				for (uintptr_t i=0;i<units.size();i++) {
					if (units[i]==**it.second.next.unit_ptr) id=i;
				}
			}
			_STDEX_OUTPUT_PARSER<<it.first.second<<"-"<<it.first.first<<"->"<<get_type[(int)it.second.type]<<id<<std::endl;
		}
#endif
	}
public:
	class parse_node {
	public:
		_Tp op;
		std::vector<parse_node> children;
	};
private:
	void on_reduction(sheet_node* current_symbol,std::vector<uintptr_t>& parse_stack,uintptr_t& current_state,std::vector<parser_listener<_Tp,_SentenceEnum>*>& listeners,uintptr_t& current_id,std::vector<parse_node>& nodes,parse_node*& current_word) {
		int reduction_num=0;
		for (std::size_t i=0;i<(*current_symbol->next.unit_ptr)->right_ops.size();i++) {
			if ((*current_symbol->next.unit_ptr)->right_ops[i]!=epsilon) reduction_num++;
		}
		for (int i=0;i<reduction_num && !parse_stack.empty();i++) parse_stack.pop_back();
		current_state=parse_stack.empty()?0:parse_stack.back();
		parse_stack.push_back((*lr_sheet[std::make_pair((*current_symbol->next.unit_ptr)->left_op,current_state)].next.lr_ptr)->id);
		_SentenceEnum id=(*current_symbol->next.unit_ptr)->id;
		intptr_t skips=0;
		for (auto it:listeners) skips=std::max(skips,it->on_reduction(current_id,current_state,(*lr_sheet[std::make_pair((*current_symbol->next.unit_ptr)->left_op,current_state)].next.lr_ptr)->id,id,reduction_num));//move_up?
		current_id+=skips;
		if (skips) current_word=((intptr_t)current_id>nodes.size() || (intptr_t)current_id<1)?nullptr:&nodes[current_id-1];
	}
public:
	bool parse_with_listener(std::vector<parse_node>& nodes) {
		std::vector<parser_listener<_Tp,_SentenceEnum>*> listeners;
		for (auto it:listeners) {
			if (it->enabled) listeners.push_back(it);
		}
		std::vector<uintptr_t> parse_stack;
		std::vector<parse_node> nodes_stack;
		parse_stack.push_back(0);
		uintptr_t current_id=0;
		parse_node* current_word=&nodes[current_id++];
		while (!parse_stack.empty()) {
			uintptr_t current_state=parse_stack.back();
			sheet_node* current_symbol=&lr_sheet[std::make_pair(current_word->op,current_state)];
			switch (current_symbol->type) {
				case ST_SHIFT: {
					parse_stack.push_back((*current_symbol->next.lr_ptr)->id);
					//nodes_stack.push_back(*current_word);
					intptr_t skips=0;
					for (auto it:listeners) skips=std::max(skips,it->on_shift(current_id,(*current_symbol->next.lr_ptr)->id,current_word->op));
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
						int result=it->on_error(current_id,current_symbol->type==ST_ERROR?parser_listener<_Tp,_SentenceEnum>::ET_ERROR:parser_listener<_Tp,_SentenceEnum>::ET_DEFAULT,current_state,current_word->op);
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
	virtual bool validate() {
		try {
			generate_parser();
		} catch (const std::exception& e) {
			return false;
		}
		return true;
	}
	friend std::ostream& operator <<(std::ostream& os,const parser& p) {
		auto write_int=[&](auto v){
			os.write(reinterpret_cast<const char*>(&v),4);
		};
		std::size_t n=p.units.size();
		write_int(n);
		for (const auto& it:p.units) {
			write_int(it.left_op);
			std::size_t size=it.right_ops.size();
			write_int(size);
			for (auto jt:it.right_ops) write_int(jt);
			write_int(it.id);
		}
		std::size_t m=p.lr_sheet.size();
		write_int(m);
		for (const auto& [key,sn]:p.lr_sheet) {
			write_int(key.first);
			write_int(key.second);
			write_int(sn.type);
			if (sn.next.lr_ptr) {
				write_int(true);
				if (sn.type==ST_SHIFT) {
					int nextid=(*sn.next.lr_ptr)->id;
					write_int(nextid);
				} else if (sn.type==ST_REDUCTION || sn.type==ST_ERROR) {
					int uid=(*sn.next.unit_ptr)->id;
					write_int(uid);
				}
			} else write_int(false);
		}
		if (!os) os.setstate(std::ios::failbit);
		return os;
	}
	
	friend std::istream& operator >>(std::istream& is,parser& p) {
		auto read_int=[&](auto& v){
			if (is) is.read(reinterpret_cast<char*>(&v),4);
			if (!is) is.setstate(std::ios::failbit);
		};
		std::size_t n;
		read_int(n);
		p.units.clear();
		for (std::size_t i=0;i<n;i++) {
			unit_type u;
			read_int(u.left_op);
			std::size_t size;
			read_int(size);
			u.right_ops.resize(size);
			for (std::size_t j=0;j<size;j++) {
				read_int(u.right_ops[j]);
			}
			read_int(u.id);
			p.units.push_back(u);
		}
		std::size_t m;
		read_int(m);
		p.lr_sheet.clear();
		for (std::size_t i=0;i<m;i++) {
			_Tp sym;
			int state;
			int type;
			read_int(sym);
			read_int(state);
			read_int(type);
			sheet_node sn;
			sn.type=(sheet_type)type;
			bool has_ptr;
			read_int(has_ptr);
			if (!is) return is;
			if (has_ptr) {
				if (sn.type==ST_SHIFT) {
					int nextid;
					read_int(nextid);
					if (!is) return is;
					sn.next.lr_ptr=new std::shared_ptr<lr_node>(std::make_shared<lr_node>(nextid));
				} else if (sn.type==ST_REDUCTION || sn.type==ST_ERROR) {
					int uid;
					read_int(uid);
					if (!is) return is;
					sn.next.unit_ptr=new std::shared_ptr<unit_type>(std::make_shared<unit_type>(p.units[uid]));
				}	
			}
			p.lr_sheet[{sym,state}]=std::move(sn);
		}
		return is;
	}
#undef _STDEX_PARSER_HAS_VALUE_I
#undef _STDEX_PARSER_HAS_VALUE
};

}

}

#endif