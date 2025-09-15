//Last Modified At 2025/09/15
//@Version 2.0.0.0
#ifndef _STD4573_SYNTAX_PARSER_H_
#define _STD4573_SYNTAX_PARSER_H_ 1

#include <algorithm>
#include <map>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "../macros/cpp_version.h"//At Least 1.0

#ifndef _STDEX_CPP17_VERSION
#define _STDEX_CPP17_VERSION 201703L
#endif

#if __cplusplus>=_STDEX_CPP17_VERSION
#include <initializer_list>
#include <typeinfo>
#include <variant>
#endif

#ifdef _STDEX_OUTPUT_PARSER
#include <iostream>
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
struct parser_unit_base {
	_Tp left_op_;
	std::vector<_Tp> right_ops_;
	std::vector<_Tp> first_set_;
	_SentenceEnum id_;
	bool operator ==(const parser_unit_base<_Tp>& other) const {
		if (right_ops_.size()!=other.right_ops_.size()) return false;
		for (int i=0;i<right_ops_.size();i++) {
			if (right_ops_[i]!=other.right_ops_[i]) return false;
		}
		return left_op_==other.left_op_;
	}
	bool operator !=(const parser_unit_base<_Tp>& other) const {
		return !((*this)==other);
	}
	std::string to_string() {
		std::string result=std::to_string(id_)+":"+std::to_string(left_op_)+"->";
		for (int i=0;i<right_ops_.size()-1;i++) result+=std::to_string(right_ops_[i])+" ";
		if (right_ops_.empty()) return result;
		return result+std::to_string(right_ops_[right_ops_.size()-1]);
	}
};

template <typename _Tp,typename _SentenceEnum=int,typename _Info=void*>
struct parser_unit : public parser_unit_base<_Tp,_SentenceEnum> {
	_Info information_=nullptr;
	bool operator ==(const parser_unit<_Tp,_SentenceEnum,_Info>& other) const {
		return parser_unit_base<_Tp,_SentenceEnum>::operator ==(other) && information_==other.information_;
	}
	bool operator !=(const parser_unit<_Tp,_SentenceEnum,_Info>& other) const {
		return !((*this)==other);
	}
};

#if __cplusplus>=_STDEX_CPP17_VERSION
template <typename _Tp,typename _SentenceEnum=int,typename _Info=void*>
parser_unit<_Tp,_SentenceEnum,_Info> single_parser_unit(_Tp left,std::initializer_list<_Tp> rights,_SentenceEnum id=(_SentenceEnum)0) {
	if (!rights.size()) throw std::invalid_argument("Invalid size of rights");
	parser_unit<_Tp,_Info> result;
	result.left_op_=left;
	result.right_ops_=std::vector<_Tp>(rights);
	result.id_=id;
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
	virtual void on_shift(int id,int state,_Tp word)=0;
	virtual void on_reduction(int id,int state,int next,int sentence_id,int reduction_num)=0;
	virtual void on_accept()=0;
	virtual void on_error(error_type type,int state,_Tp word)=0;
};

template <typename _Tp,typename _SentenceEnum=int,typename _Info=void*>
class parser {
#define _STDEX_PARSER_HAS_VALUE(array,value) (std::find(array.begin(),array.end(),value)!=array.end())
#define _STDEX_PARSER_HAS_VALUE_I(array,value,i) (std::find(array.begin()+i,array.end(),value)!=array.end())
	using unit_type=parser_unit<_Tp,_SentenceEnum,_Info>;
public:
	std::vector<unit_type> units_;
	std::map<_Tp,bool> ptrs_;
	_Tp start_;
	_Tp seperator_;
	_Tp epsilon_;
	_Tp eof_;
	int start_unit_;
	std::vector<parser_listener<_Tp,_SentenceEnum>*> listeners_;
	parser() {
		start_=(_Tp)-1;
		seperator_=(_Tp)-1;
		epsilon_=(_Tp)-1;
		eof_=(_Tp)-1;
	}
	parser(_Tp start,_Tp seperator,_Tp epsilon,_Tp eof) : start_(start) , seperator_(seperator) , epsilon_(epsilon) , eof_(eof) { }
	parser(_Tp start,_Tp seperator,_Tp epsilon,_Tp eof,std::vector<unit_type> units) : start_(start) , seperator_(seperator) , epsilon_(epsilon) , eof_(eof) , units_(units) { }
#if __cplusplus>=_STDEX_CPP17_VERISON
	parser (std::initializer_list<std::variant<_Tp,std::vector<unit_type>,std::map<_Tp,bool>>> init_list) {
		if (init_list.size()!=6) throw std::invalid_argument("The number of the initializer arguments for parser must be 6!");
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
		if (std::holds_alternative<std::map<_Tp,bool>>(*it)) ptrs_=std::get<std::map<_Tp,bool>>(*it++);
		else throw std::invalid_argument(std::string("The sixth argument for parser must be std::map<_Tp,bool>(")+std::string(typeid(std::map<_Tp,bool>).name())+std::string(") to give ptr\'s infos!"));
	}
#endif

private:
	class lr_node {
	public:
		int id_;
		std::vector<unit_type> unit_list_;
		std::map<_Tp,lr_node*> edges_;
		lr_node(int id) : id_(id) {}
		
		bool operator ==(const lr_node& other) const {
			if (unit_list_.size()!=other.unit_list_.size()) return false;
			for (int i=0;i<unit_list_.size();i++) {
				if (unit_list_[i]!=other.unit_list_[i]) return false;
			}
			return true;
			//return edges_==other.edges_;
		}
		bool operator !=(const lr_node& other) const {
			return !((*this)==other);
		}
	};
	std::vector<lr_node*> lr_node_list_;
	std::map<_Tp,std::vector<_Tp>> first_set_;
	std::map<_Tp,std::vector<_Tp>> follow_set_;
	class sheet_node {
	public:
		sheet_type type_=ST_ERROR;
		union node_info {
			lr_node* lr_ptr_;
			unit_type* unit_ptr_;
		} next_;
		sheet_node() {
			next_.lr_ptr_=nullptr;
			type_=ST_ERROR;
		}
		~sheet_node() {
			if (next_.lr_ptr_) {
				if (type_==ST_REDUCTION) delete next_.unit_ptr_;
				else if (type_!=ST_ERROR) delete next_.lr_ptr_;
			}
		}
	};
	std::map<std::pair<_Tp,int>,sheet_node> lr_sheet_;

private:
	lr_node* generate_nexts(_Tp start,lr_node* start_node,std::vector<parser_unit<_Tp,_SentenceEnum,_Info>> units,int& node_amount) {
		lr_node* curr_node=new lr_node(node_amount++);
		std::vector<_Tp> wait_list;
		for (int i=0;i<start_node->unit_list_.size();i++) {
			for (int j=0;j<start_node->unit_list_[i].right_ops_.size()-1;j++) {
				if (start_node->unit_list_[i].right_ops_[j]==seperator_) {
					int k=j+1;
					while (k<start_node->unit_list_[i].right_ops_.size() && start_node->unit_list_[i].right_ops_[k]==epsilon_) k++;
					if (k<start_node->unit_list_[i].right_ops_.size() && start_node->unit_list_[i].right_ops_[k]==start) {
						unit_type temp_unit;
						temp_unit.left_op_=start_node->unit_list_[i].left_op_;
						for (auto it:start_node->unit_list_[i].right_ops_) temp_unit.right_ops_.push_back(it);
						for (int l=j;l<k-1;l++) temp_unit.right_ops_[l]=epsilon_;
						temp_unit.right_ops_[k-1]=start_node->unit_list_[i].right_ops_[k];
						temp_unit.right_ops_[k]=seperator_;
						curr_node->unit_list_.push_back(temp_unit);
						if (k+1<start_node->unit_list_[i].right_ops_.size()) {
							if (!_STDEX_PARSER_HAS_VALUE(wait_list,start_node->unit_list_[i].right_ops_[k+1])) wait_list.push_back(start_node->unit_list_[i].right_ops_[k+1]);
						}
						break;
					}
				}
			}
		}
		for (int i=0;i<wait_list.size();i++) {
			for (int j=0;j<units.size();j++) {
				if (wait_list[i]==units[j].left_op_) {
					unit_type temp_unit;
					temp_unit.left_op_=units[j].left_op_;
					int k=0;
					while (k<units[j].right_ops_.size() && units[j].right_ops_[k]==epsilon_) {
						temp_unit.right_ops_.push_back(epsilon_);
						k++;
					}
					temp_unit.right_ops_.push_back(seperator_);
					for (k;k<units[j].right_ops_.size();k++) temp_unit.right_ops_.push_back(units[j].right_ops_[k]);
					curr_node->unit_list_.push_back(temp_unit);
					if (!_STDEX_PARSER_HAS_VALUE_I(wait_list,temp_unit.right_ops_[1],i)) wait_list.push_back(temp_unit.right_ops_[1]);
				}
			}
		}
		for (auto i:lr_node_list_) {
			if (*i==*curr_node) {
				node_amount--;
				return i;
			}
		}
		lr_node_list_.push_back(curr_node);
		curr_node->edges_.clear();
		if (wait_list.empty()) return curr_node;
		while (!wait_list.empty()) {
			lr_node* next=generate_nexts(wait_list.front(),curr_node,units,node_amount);
			if (!_STDEX_PARSER_HAS_VALUE(lr_node_list_,next)) {
				curr_node->edges_[wait_list.front()]=next;
				wait_list.erase(wait_list.begin());
				lr_node_list_.push_back(next);
			} else {
				curr_node->edges_[wait_list.front()]=next;
				wait_list.erase(wait_list.begin());
			}
		}
		return curr_node;
	}	
	void generate_lr_nodes(int& node_amount) {
		lr_node* curr_node=new lr_node(node_amount++);
		lr_node_list_.push_back(curr_node);
		std::vector<_Tp> wait_list;
		wait_list.push_back(start_);
		for (int i=0;i<wait_list.size();i++) {
			for (auto it:units_) {
				if (wait_list[i]==it.left_op_) {
					unit_type temp_unit;
					temp_unit.left_op_=it.left_op_;
					int j=0;
					while (j<it.right_ops_.size() && it.right_ops_[j]==epsilon_) {
						temp_unit.right_ops_.push_back(epsilon_);
						j++;
					}
					temp_unit.right_ops_.push_back(seperator_);
					for (j;j<it.right_ops_.size();j++) temp_unit.right_ops_.push_back(it.right_ops_[j]);
					curr_node->unit_list_.push_back(temp_unit);
					if (!_STDEX_PARSER_HAS_VALUE_I(wait_list,it.right_ops_[0],i)) wait_list.push_back(it.right_ops_[0]);
				}
			}
		}
		while (!wait_list.empty()) {
			lr_node* next=generate_nexts(wait_list.front(),curr_node,units_,node_amount);
			if (!_STDEX_PARSER_HAS_VALUE(lr_node_list_,next)) {
				curr_node->edges_[wait_list.front()]=next;
				wait_list.erase(wait_list.begin());
				lr_node_list_.push_back(next);
			} else {
				curr_node->edges_[wait_list.front()]=next;
				wait_list.erase(wait_list.begin());
			}
		}
		return;// curr_node;	
	}
	void calculate_first(unit_type& unit) {
		std::vector<_Tp> wait_list;
		wait_list.push_back(unit.left_op_);
		if (ptrs_[unit.right_ops_[0]]) wait_list.push_back(unit.right_ops_[0]);
		else unit.first_set_.push_back(unit.right_ops_[0]);
		for (int i=0;i<wait_list.size();i++) {
			for (auto it:units_) {
				if (it==unit) continue;
				if (it.left_op_==wait_list[i]) {
					if(!ptrs_[it.right_ops_[0]]) unit.first_set_.push_back(it.right_ops_[0]);
					else {
						if(!_STDEX_PARSER_HAS_VALUE_I(wait_list,it.right_ops_[0],i)) wait_list.push_back(it.right_ops_[0]);
					}
				}
			}
		}
	}
	void calculate_first(_Tp op) {
		first_set_[op].clear();
		if (!ptrs_[op]) {
			first_set_[op].push_back(op);
			return;
		}
		for (auto it:units_) {
			if (it.left_op_==op) {
				for (auto jt:it.first_set_) {
					if(!_STDEX_PARSER_HAS_VALUE(first_set_[op],jt)) first_set_[op].push_back(jt);
				}
			}
		}
	}
	bool calculate_follow(unit_type& unit) {
		bool result=false;
		if (ptrs_[unit.right_ops_[unit.right_ops_.size()-1]]) {
			if (unit.right_ops_[unit.right_ops_.size()-1]!=unit.left_op_) {
				for (auto it:follow_set_[unit.left_op_]) {
					if (!_STDEX_PARSER_HAS_VALUE(follow_set_[unit.right_ops_[unit.right_ops_.size()-1]],it)) {
						follow_set_[unit.right_ops_[unit.right_ops_.size()-1]].push_back(it);
						result=true;
					}
				}
			}
		}
		bool e_stand=true;
		int start_index=unit.right_ops_.size()-2,end_index=start_index+1;
		while (start_index>=0) {
			if (ptrs_[unit.right_ops_[start_index]]) {
				for (int i=start_index+1;i<=end_index;i++) {
					for (auto it:first_set_[unit.right_ops_[i]]) {
						if(!_STDEX_PARSER_HAS_VALUE(follow_set_[unit.right_ops_[start_index]],it) && it!=epsilon_) {
							follow_set_[unit.right_ops_[start_index]].push_back(it);
							result=true;
						}
					}
				}
				if (!_STDEX_PARSER_HAS_VALUE(first_set_[unit.right_ops_[start_index]],epsilon_)) end_index=start_index;
				if (e_stand && !_STDEX_PARSER_HAS_VALUE(first_set_[unit.right_ops_[start_index+1]],epsilon_)) e_stand=false;
				if (e_stand) {
					for (auto it:follow_set_[unit.left_op_]) {
						if (!_STDEX_PARSER_HAS_VALUE(follow_set_[unit.right_ops_[start_index]],it)) {
							follow_set_[unit.right_ops_[start_index]].push_back(it);
							result=true;
						}
					}
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
		std::vector<_Tp> wait_ptr_list;
		wait_ptr_list.clear();
		for (auto it:start_unit.right_ops_) {
			if (ptrs_[it]) wait_ptr_list.push_back(it);
		}
		//WHEN USING THE ANOTHER METHOD,ONLY PUSH START TO WAIT_PTR_LIST
		//BUT IF REVERSE IS NEEDED,THE CURRENT CODE IS NOT AVAILABLE
		for (int i=0;i<wait_ptr_list.size();i++) {
			for (auto& it:units_) {
				if(!_STDEX_PARSER_HAS_VALUE(it.right_ops_,wait_ptr_list[i])) continue;
				bool result=calculate_follow(it);
				for (auto jt:it.right_ops_) {
					if (ptrs_[jt] && jt!=wait_ptr_list[i]) {
						if (!_STDEX_PARSER_HAS_VALUE(wait_ptr_list,jt) | result) wait_ptr_list.push_back(jt);
					}
				}	
			}
			for (auto& it:units_) {
				if (it.left_op_!=wait_ptr_list[i]) continue;
				bool result=calculate_follow(it);	
				for (auto jt:it.right_ops_) {
					if (ptrs_[jt] && jt!=wait_ptr_list[i]) {
						if (!_STDEX_PARSER_HAS_VALUE(wait_ptr_list,jt) | result) wait_ptr_list.push_back(jt);
					}
				}
			}
		}
		//for (auto i:wait_ptr_list) cout<<i<<" ";
		//cout<<"\nEND FOLLOW\n";
	}
	void construct_table() {
		for (auto it:lr_node_list_) {
			for (auto jt:it->edges_) {
				lr_sheet_[std::make_pair(jt.first,it->id_)].next_.lr_ptr_=jt.second;
				if (!ptrs_[jt.first]) lr_sheet_[std::make_pair(jt.first,it->id_)].type_=ST_SHIFT;
			}
		}
		for (auto it:lr_node_list_) {
			int i=0;
			for (auto jt:it->unit_list_) {
				if (jt.right_ops_[jt.right_ops_.size()-1]==seperator_) {
					unit_type* temp_unit=new unit_type;
					temp_unit->left_op_=jt.left_op_;
					for (auto kt:jt.right_ops_) {
						if (kt!=seperator_) temp_unit->right_ops_.push_back(kt);
					}
					for (auto kt:follow_set_[jt.left_op_]) {
						if (lr_sheet_[make_pair(kt,it->id_)].next_.lr_ptr_) {
							if (ptrs_[kt]) throw std::logic_error("Conflict GOTO and REDUCTION at production "+std::to_string(i)+"("+jt.to_string()+") with GT("+std::to_string(it->id_)+","+std::to_string(kt)+")");
							else {
								if (lr_sheet_[make_pair(kt,it->id_)].type_==ST_SHIFT) throw std::logic_error("Conflict SHIFT and REDUCTION at production "+std::to_string(i)+"("+jt.to_string()+") with SHIFT("+std::to_string(it->id_)+","+std::to_string(kt)+")");
								else throw std::logic_error("Conflict REDUCTION and REDUCTION at production "+std::to_string(i)+"("+jt.to_string()+") with REDUCTION("+std::to_string(it->id_)+","+std::to_string(kt)+")");
							}
						}
						lr_sheet_[make_pair(kt,it->id_)].type_=ST_REDUCTION;
						lr_sheet_[make_pair(kt,it->id_)].next_.unit_ptr_=temp_unit;
					}
				}
				i++;
			}
			if (it->unit_list_.size()==1) {
				unit_type* temp_unit=new unit_type;
				temp_unit->left_op_=it->unit_list_[0].left_op_;
				for (auto jt:it->unit_list_[0].right_ops_) {
					if (jt!=seperator_) temp_unit->right_ops_.push_back(jt);
				}
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
		int node_amount=0;
		if (auto_ptr) {
			ptrs_.clear();
			for (auto it:units_) {
				ptrs_[it.left_op_]|=true;
				for (auto jt:it.right_ops_) ptrs_[jt]|=false;
			}
		}
		lr_node_list_.clear();
		generate_lr_nodes(node_amount);
#ifdef _STDEX_OUTPUT_PARSER
		for (auto it:lr_node_list_) {
			std::cout<<it->id_<<":"<<endl;
			std::cout<<"  units:"<<endl;
			for (auto jt:it->unit_list_) {
				std::cout<<"    "<<jt.left_op_<<"->";
				for (auto kt:jt.right_ops_) cout<<kt<<" ";
				std::cout<<endl;
			}
			std::cout<<"  edges:"<<endl;
			for (auto jt:it->edges_) std::cout<<"    "<<jt.first<<"->"<<jt.second->id_<<endl;
			std::cout<<endl;
		}
#endif
		first_set_.clear();
		follow_set_.clear();
		for (auto& it:units_) calculate_first(it);
		for (auto& it:ptrs_) calculate_first(it.first);
		calculate_follow();
#ifdef _STDEX_OUTPUT_PARSER
		std::cout<<"\n";
		for (auto& it:ptrs_) {
			std::cout<<it.first<<":\n  {";
			for (int i=0;i<follow_set_[it.first].size();i++) {
				std::cout<<follow_set_[it.first][i];
				if (i!=follow_set_[it.first].size()-1) cout<<",";
			}
			std::cout<<"}\n";
		}
#endif
		lr_sheet_.clear();
		construct_table();
#ifdef _STDEX_OUTPUT_PARSER
		std::cout<<"\n";
		for (auto& it:lr_sheet_) {
			std::vector<std::string> get_type={"e","r","s","a"};
			int id=-1;
			if (it.second.type_==ST_SHIFT || (it.second.type_==ST_ERROR && it.second.next_.lr_ptr_)) id=it.second.next_.lr_ptr_->id_;
			else if (it.second.type_==ST_REDUCTION) {
				for (int i=0;i<units_.size();i++) {
					if (units_[i]==*(it.second.next_.unit_ptr_)) id=i;
				}
			}
			std::cout<<it.first.second<<"-"<<it.first.first<<"->"<<get_type[(int)it.second.type_]<<id<<endl;
		}
#endif
	}
public:
	template <typename _St=void*>
	class parse_node {
	public:
		_Tp op_;
		_St info_=nullptr;
		std::vector<parse_node> children_;
	};
	template <typename _St=void*>
	bool parse_with_listener(std::vector<parse_node<_St>> nodes) {
		std::vector<parser_listener<_Tp>*> listeners;
		for (auto it:listeners_) {
			if (it->enabled_) listeners.push_back(it);
		}
		std::vector<int> parse_stack;
		std::vector<parse_node<_St>> nodes_stack;
		parse_stack.push_back(0);
		int current_id=0;
		parse_node<_St>* current_word=&nodes[current_id++];
		while (!parse_stack.empty()) {
			int current_state=parse_stack.back();
			sheet_node current_symbol=lr_sheet_[std::make_pair(current_word->op_,current_state)];
			switch (current_symbol.type_) {
				case ST_SHIFT: {
					parse_stack.push_back(current_symbol.next_.lr_ptr_->id_);
					//nodes_stack.push_back(*current_word);
					for (auto it:listeners) it->on_shift(current_id,current_symbol.next_.lr_ptr_->id_,current_word->op_);		
					current_word=(current_id==nodes.size())?nullptr:&nodes[current_id++];
					break;
				}
				case ST_REDUCTION: {
					int reduction_num=0;
					for (int i=0;i<current_symbol.next_.unit_ptr_->right_ops_.size();i++) {
						if (current_symbol.next_.unit_ptr_->right_ops_[i]!=epsilon_) reduction_num++;
					}
					for (int i=0;i<reduction_num && !parse_stack.empty();i++) parse_stack.pop_back();
					current_state=parse_stack.empty()?0:parse_stack.back();
					parse_stack.push_back(lr_sheet_[std::make_pair(current_symbol.next_.unit_ptr_->left_op_,current_state)].next_.lr_ptr_->id_);
					int id=current_symbol.next_.unit_ptr_->id_;
					/*for (int i=0;i<units_.size();i++) {
						if (units_[i]==*(current_symbol.next_.unit_ptr_)) id=i;
					}*/
					for (auto it:listeners) it->on_reduction(current_id,current_state,lr_sheet_[std::make_pair(current_symbol.next_.unit_ptr_->left_op_,current_state)].next_.lr_ptr_->id_,id,reduction_num);//move_up?
					break;
				}
				case ST_ACCEPT: {
					for (auto it:listeners) it->on_accept();
					return true;
				}
				case ST_ERROR: {
					for (auto it:listeners) it->on_error(parser_listener<_Tp>::ET_ERROR,current_state,current_word->op_);
					return false;
				}
				default: {
					for (auto it:listeners) it->on_error(parser_listener<_Tp>::ET_DEFAULT,current_state,current_word->op_);
					return false;
				}
			}
		}
		for (auto it:listeners) it->on_error(parser_listener<_Tp>::ET_UNKNOWN,0,(_Tp)-1);
		return false;
	}
#undef _STDEX_PARSER_HAS_VALUE_I
#undef _STDEX_PARSER_HAS_VALUE
};

}

}

#endif