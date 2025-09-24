//Last Modified At 2025/09/24
//@Version 1.0.0.0
#ifndef _STDEX_SYNTAX_LR_PARSER_H_
#define _STDEX_SYNTAX_LR_PARSER_H_ 1

#include <cstddef>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "parser.h"//At Least 3.2

namespace stdex {
	
namespace syntax {

template <typename _Tp,typename _SentenceEnum=int>
class lalr_parser : public parser<_Tp,_SentenceEnum> {
	using base=parser<_Tp,_SentenceEnum>;
	using unit_type=typename base::unit_type;
	using lr_node=typename base::lr_node;
	struct lalr_item {
		unit_type core_;
		std::unordered_set<_Tp> lookaheads_;
	};
	std::unordered_map<lr_node*,std::vector<lalr_item>> lalr_items_;
public:
	lalr_parser(std::initializer_list<std::variant<_Tp,std::vector<unit_type>,std::map<_Tp,bool>>> init_list) : parser<_Tp,_SentenceEnum>(init_list) { }
private:
	std::unordered_set<_Tp> compute_first(const std::vector<_Tp>& suffix,const std::unordered_set<_Tp>& lalrs) {
		std::unordered_set<_Tp> result;
		bool nullable=true;
		for (auto& it:suffix) {
			auto& fs=base::first_set_[it];
			for (auto& jt:fs) {
				if (jt!=base::epsilon_) result.insert(jt);
			}
			if (fs.find(base::epsilon_)==fs.end()) {
				nullable=false;
				break;
			}
		}
		if (nullable) result.insert(lalrs.begin(),lalrs.end());
		return result;
	}
	bool propagate_once() {
		bool changed=false;
		for (auto& [state,items]:lalr_items_) {
			for (auto& it:items) {
				if (it.core_.dot_<it.core_.right_ops_.size()) {
					_Tp current=it.core_.right_ops_[it.core_.dot_];
					if (base::ptrs_[current]) {
						std::vector<_Tp> suffix;
						suffix.insert(suffix.end(),it.core_.right_ops_.begin()+it.core_.dot_+1,it.core_.right_ops_.end());
						if (suffix.empty()) suffix.push_back(base::epsilon_);
						std::unordered_set<_Tp> firsts=compute_first(suffix,it.lookaheads_);
						auto jt=base::units_by_lhs_.find(current);
						if (jt==base::units_by_lhs_.end()) continue;
						for (auto& kt:jt->second) {
							for (auto& lt:items) {
								if (lt.core_.left_op_==kt->left_op_ && lt.core_.right_ops_==kt->right_ops_ && lt.core_.dot_==0) {
									for (auto mt:firsts) {
										if (lt.lookaheads_.insert(mt).second) changed=true;
									}
								}
							}
						}
					}
				}
			}
		}
		return changed;
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
				if (jt.core_.dot_==jt.core_.right_ops_.size()) {
					unit_type temp_unit;
					temp_unit.left_op_=jt.core_.left_op_;
					for (auto kt:jt.core_.right_ops_) temp_unit.right_ops_.push_back(kt);
					temp_unit.dot_=-1;
					temp_unit.id_=jt.core_.id_;
					for (auto kt:base::ptrs_) {
						_Tp current_ptr=kt.first;
						if (jt.lookaheads_.find(current_ptr)!=jt.lookaheads_.end()) {
							if (base::lr_sheet_[std::make_pair(current_ptr,it->id_)].next_.lr_ptr_ && base::lr_sheet_[std::make_pair(current_ptr,it->id_)].type_!=ST_ERROR) {
								if (base::ptrs_[current_ptr]) throw std::logic_error("Conflict GOTO and REDUCTION at production "+std::to_string(i)+"("+jt.core_.to_string()+") with GT("+std::to_string(it->id_)+","+std::to_string(current_ptr)+")");
								else {
									if (base::lr_sheet_[std::make_pair(current_ptr,it->id_)].type_==ST_SHIFT) throw std::logic_error("Conflict SHIFT and REDUCTION at production "+std::to_string(i)+"("+jt.core_.to_string()+") with SHIFT("+std::to_string(it->id_)+","+std::to_string(current_ptr)+")");
									else throw std::logic_error("Conflict REDUCTION and REDUCTION at production "+std::to_string(i)+"("+jt.core_.to_string()+") with REDUCTION("+std::to_string(it->id_)+","+std::to_string(current_ptr)+")");
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
		lalr_items_.clear();
		for (auto& it:I0->unit_list_) {
			lalr_item temp_lalr{it,{}};
			if (it.left_op_==base::start_) temp_lalr.lookaheads_.insert(base::eof_);
			lalr_items_[I0].push_back(temp_lalr);
		}
		base::calculate_first();
		while (propagate_once()) {}
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
};

}

}

#endif