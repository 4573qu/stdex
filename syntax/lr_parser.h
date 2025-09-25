//Last Modified At 2025/09/25
//@Version 1.0.0.1
#ifndef _STDEX_SYNTAX_LR_PARSER_H_
#define _STDEX_SYNTAX_LR_PARSER_H_ 1

#include <cstddef>
#include <memory>
#include <queue>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "parser.h"//At Least 3.2.1

namespace stdex {
	
namespace syntax {

template <typename _Tp,typename _SentenceEnum=int>
class lalr_parser : public parser<_Tp,_SentenceEnum> {
	using base=parser<_Tp,_SentenceEnum>;
	using unit_type=typename base::unit_type;
	using lr_node=typename base::lr_node;
	struct lalr_item {
		uintptr_t index_;
		std::unordered_set<_Tp> lookaheads_;
	};
	std::unordered_map<lr_node*,std::vector<lalr_item>> lalr_items_;
public:
	lalr_parser(std::initializer_list<std::variant<_Tp,std::vector<unit_type>,std::map<_Tp,bool>>> init_list) : parser<_Tp,_SentenceEnum>(init_list) { }
private:
	std::unordered_set<_Tp> compute_first(const std::vector<_Tp>& sequence) {
		std::unordered_set<_Tp> result;
		for (auto& it:sequence) {
			for (auto& jt:base::first_set_[it]) {
				if (jt!=base::epsilon_) result.insert(jt);
			}
			if (base::first_set_[it].find(base::epsilon_)==base::first_set_[it].end()) return result;
		}
		result.insert(base::epsilon_);
		return result;
	}
	void propagate(lr_node* start) {
		for (auto& it:base::lr_node_list_) {
			for (uintptr_t i=0;i<it->unit_list_.size();i++) {
				lalr_item temp{i,{}};
				lalr_items_[it].push_back(temp);
			}
		}
		std::queue<std::pair<lr_node*,int>> q;
		for (int i=0;i<start->unit_list_.size();i++) {
			if (start->unit_list_[i].left_op_==base::start_ && start->unit_list_[i].dot_==0) {
				lalr_items_[start][i].lookaheads_.insert(base::eof_);
				q.push(std::make_pair(start,i));
			}
		}
		while (q.size()) {
			auto current=q.front();
			q.pop();
			unit_type& item=current.first->unit_list_[current.second];
			auto& lookaheads=lalr_items_[current.first][current.second].lookaheads_;
			if (item.dot_<item.right_ops_.size()) {
				_Tp ptr=item.right_ops_[item.dot_];
				std::vector<_Tp> beta;
				beta.insert(beta.end(),item.right_ops_.begin()+item.dot_+1,item.right_ops_.end());
				if (base::ptrs_[ptr]) {
					auto firsts=compute_first(beta);
					if (firsts.find(base::epsilon_)!=firsts.end()) firsts.insert(lookaheads.begin(),lookaheads.end());
					for (int i=0;i<current.first->unit_list_.size();i++) {
						if (current.first->unit_list_[i].left_op_==ptr && current.first->unit_list_[i].dot_==0) {
							if (firsts.find(base::epsilon_)!=firsts.end()) q.push(std::make_pair(current.first,i));
							else {
								uintptr_t size=lalr_items_[current.first][i].lookaheads_.size();
								lalr_items_[current.first][i].lookaheads_.insert(firsts.begin(),firsts.end());
								if (lalr_items_[current.first][i].lookaheads_.size()>size) q.push(std::make_pair(current.first,i));
							}
						}
					}
				}
				if (current.first->edges_.count(ptr) && current.first->edges_[ptr]) {
					unit_type temp;
					temp.left_op_=item.left_op_;
					temp.right_ops_=item.right_ops_;
					temp.dot_=item.dot_+1;
					for (int i=0;i<current.first->edges_[ptr]->unit_list_.size();i++) {
						if (current.first->edges_[ptr]->unit_list_[i]==temp) {
							//auto next_firsts=compute_first(beta);
							uintptr_t size=lalr_items_[current.first->edges_[ptr]][i].lookaheads_.size();
							lalr_items_[current.first->edges_[ptr]][i].lookaheads_.insert(lookaheads.begin(),lookaheads.end());
							if (lalr_items_[current.first->edges_[ptr]][i].lookaheads_.size()>size) q.push(std::make_pair(current.first->edges_[ptr],i));
						}
					}
				}
			}
		}
	}
	void construct_table() override {
		auto it=base::units_by_lhs_.find(base::start_);
		std::vector<unit_type*> start_units;
		if (it!=base::units_by_lhs_.end()) {
			for (auto& jt:it->second) {
				if (jt->right_ops_.size() && jt->right_ops_[jt->right_ops_.size()-1]==base::eof_) start_units.push_back(const_cast<unit_type*>(jt));
			}
		}
		for (auto& it:base::lr_node_list_) {
			for (auto jt:it->edges_) {
				base::lr_sheet_[std::make_pair(jt.first,it->id_)].next_.lr_ptr_=new std::shared_ptr<lr_node>(std::make_shared<lr_node>(*jt.second));
				if (!base::ptrs_[jt.first]) base::lr_sheet_[std::make_pair(jt.first,it->id_)].type_=ST_SHIFT;
			}
		}
		for (auto& it:base::lr_node_list_) {
			uintptr_t i=0;
			for (auto& jt:lalr_items_[it]) {
				if (it->unit_list_[jt.index_].dot_==it->unit_list_[jt.index_].right_ops_.size()) {
					unit_type temp_unit;
					temp_unit.left_op_=it->unit_list_[jt.index_].left_op_;
					temp_unit.right_ops_=it->unit_list_[jt.index_].right_ops_;
					temp_unit.dot_=-1;
					temp_unit.id_=it->unit_list_[jt.index_].id_;
					for (auto kt:base::ptrs_) {
						_Tp current_ptr=kt.first;
						if (jt.lookaheads_.find(current_ptr)!=jt.lookaheads_.end()) {
							if (base::lr_sheet_[std::make_pair(current_ptr,it->id_)].next_.lr_ptr_ && base::lr_sheet_[std::make_pair(current_ptr,it->id_)].type_!=ST_ERROR) {
								if (base::ptrs_[current_ptr]) throw std::logic_error("Conflict GOTO and REDUCTION at production "+std::to_string(i)+"("+it->unit_list_[jt.index_].to_string()+") with GT("+std::to_string(it->id_)+","+std::to_string(current_ptr)+")");
								else {
									if (base::lr_sheet_[std::make_pair(current_ptr,it->id_)].type_==ST_SHIFT) throw std::logic_error("Conflict SHIFT and REDUCTION at production "+std::to_string(i)+"("+it->unit_list_[jt.index_].to_string()+") with SHIFT("+std::to_string(it->id_)+","+std::to_string(current_ptr)+")");
									else throw std::logic_error("Conflict REDUCTION and REDUCTION at production "+std::to_string(i)+"("+it->unit_list_[jt.index_].to_string()+") with REDUCTION("+std::to_string(it->id_)+","+std::to_string(current_ptr)+")");
								}
							}
							base::lr_sheet_[std::make_pair(current_ptr,it->id_)].type_=ST_REDUCTION;
						}
						if (!base::lr_sheet_[std::make_pair(current_ptr,it->id_)].next_.unit_ptr_) base::lr_sheet_[std::make_pair(current_ptr,it->id_)].next_.unit_ptr_=new std::shared_ptr<unit_type>(std::make_shared<unit_type>(temp_unit));
					}
				}
				i++;
			}
			if (it->unit_list_.size()==1 && it->unit_list_[0].right_ops_.size() && it->unit_list_[0].right_ops_[it->unit_list_[0].right_ops_.size()-1]==base::eof_) {
				unit_type* temp_unit=new unit_type;
				temp_unit->left_op_=it->unit_list_[0].left_op_;
				for (auto jt:it->unit_list_[0].right_ops_) temp_unit->right_ops_.push_back(jt);
				temp_unit->dot_=-1;
				for (auto& jt:start_units) {
					if (*temp_unit==*jt) {
						for (auto& kt:base::lr_node_list_) {
							for (auto& lt:kt->edges_) {
								if (lt.second->id_==it->id_) base::lr_sheet_[std::make_pair(base::eof_,kt->id_)].type_=ST_ACCEPT;
							}
						}
					}
				}
			}
		}
    }

public:
	void generate_parser(bool auto_ptr=true) override {
		base::ptrs_.clear();
		uintptr_t node_amount=0;
		if (auto_ptr) {
			base::ptrs_.clear();
			for (auto it:base::units_) {
				base::ptrs_[it.left_op_]|=true;
				for (auto jt:it.right_ops_) base::ptrs_[jt]|=false;
			}
		}
		base::generate_initialize();
		auto it=base::units_by_lhs_.find(base::start_);
		if (it==base::units_by_lhs_.end()) return;
		std::vector<unit_type> start_units;
		for (auto jt:it->second) start_units.push_back(*jt);
		auto I0=base::generate_lr_node(start_units,node_amount);
		base::calculate_first();
		lalr_items_.clear();
		propagate(I0);
#ifdef _STDEX_OUTPUT_PARSER
		auto lr_sort=[](lr_node* lhs,lr_node* rhs) {
			return lhs->id_<rhs->id_;
		};
		std::set<lr_node*,decltype(lr_sort)> lr_output(lr_sort);
		for (auto& it:base::lr_node_list_) lr_output.insert(it);
		for (auto& it:lr_output) {
			_STDEX_OUTPUT_PARSER<<it->id_<<":"<<std::endl;
			_STDEX_OUTPUT_PARSER<<"  units:"<<std::endl;
			for (int i=0;i<it->unit_list_.size();i++) {
				_STDEX_OUTPUT_PARSER<<"    "<<it->unit_list_[i].left_op_<<"->";
				for (int j=0;j<it->unit_list_[i].right_ops_.size();j++) {
					if (j==it->unit_list_[i].dot_) _STDEX_OUTPUT_PARSER<<"· ";
					_STDEX_OUTPUT_PARSER<<it->unit_list_[i].right_ops_[j]<<" ";
				}
				if (it->unit_list_[i].dot_==it->unit_list_[i].right_ops_.size()) _STDEX_OUTPUT_PARSER<<"· ";
				_STDEX_OUTPUT_PARSER<<"{";
				std::string temp_lookahead="";
				for (auto jt:lalr_items_[it][i].lookaheads_) temp_lookahead+=std::to_string((int)jt)+",";
				if (temp_lookahead.size()) temp_lookahead.pop_back();
				_STDEX_OUTPUT_PARSER<<temp_lookahead<<"}"<<std::endl;
			}
			_STDEX_OUTPUT_PARSER<<"  edges:"<<std::endl;
			for (auto jt:it->edges_) _STDEX_OUTPUT_PARSER<<"    "<<jt.first<<"->"<<jt.second->id_<<std::endl;
			_STDEX_OUTPUT_PARSER<<std::endl;
		}
#endif
		construct_table();
#ifdef _STDEX_OUTPUT_PARSER
		_STDEX_OUTPUT_PARSER<<"\n";
		for (auto& it:base::lr_sheet_) {
			std::vector<std::string> get_type={"e","r","s","a"};
			intptr_t id=-1;
			if (it.second.type_==ST_SHIFT || (it.second.type_==ST_ERROR && it.second.next_.lr_ptr_)) id=(*it.second.next_.lr_ptr_)->id_;
			else if (it.second.type_==ST_REDUCTION) {
				for (uintptr_t i=0;i<base::units_.size();i++) {
					if (base::units_[i]==**it.second.next_.unit_ptr_) id=i;
				}
			}
			_STDEX_OUTPUT_PARSER<<it.first.second<<"-"<<it.first.first<<"->"<<get_type[(int)it.second.type_]<<id<<std::endl;
		}
#endif
	}
};

}

}

#endif