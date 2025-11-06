//Last Modified At 2025/11/06
//@Version 1.5.0.0
#ifndef _STDEX_SYNTAX_LEXER_H_
#define _STDEX_SYNTAX_LEXER_H_ 1

#include <algorithm>
#include <climits>
#include <cstddef>
#include <initializer_list>
#include <iomanip>
#include <map>
#include <memory>
#include <set>
#include <stdexcept>
#include <string>
#include <typeinfo>
#include <utility>
#include <variant>
#include <vector>

#include "../structure/disjoint_set.h"//At Least 1.0

namespace stdex {
	
namespace syntax {

template <typename _Tp,typename _Str=std::string>
struct lexer_node {
	std::map<_Str::value_type,lexer_node<_Tp,_Str>*> edge_;
	std::vector<lexer_node<_Tp,_Str>*> epsilons_;
	_Tp token_;
	bool end_;
	int fuzzy_;
	lexer_node(_Tp token=(_Tp)-1) {
		end_=false;
		token_=token;
		edge_.clear();
		epsilons_.clear();
		fuzzy_=-1;
	}
#ifdef _STDEX_OUTPUT_LEXER
	void print() {
		_STDEX_OUTPUT_LEXER<<((std::size_t)this);
		_STDEX_OUTPUT_LEXER<<" token:"<<((int)token_)<<" end:"<<(end_?"true":"false")<<"\nedges:\n";
		for (auto& it:edge_) result<<"    "<<it.first<<" "<<((uint64_t)it.second)<<"\n";
		_STDEX_OUTPUT_LEXER<<"epsilons:\n";
		for (auto& it:epsilons_) result<<"    ε "<<((uint64_t)it)+"\n";
	}
#endif
	lexer_node(const lexer_node& other) {
		end_=other.end_;
		token_=other.token_;
		edge_=other.edge_;
		epsilons_=other.epsilons_;
		fuzzy_=other.fuzzy_;
	}
	lexer_node(lexer_node&& other) {
		if (this!=&other) {
			end_=other.end_;
			token_=other.token_;
			edge_=other.edge_;
			epsilons_=other.epsilons_;
			fuzzy_=other.fuzzy_;
			other.end_=false;
			other.token_=(_Tp)-1;
			other.edge_.clear();
			other.epsilons_.clear();
			other.fuzzy_=-1;
		}
	}
	bool equal_to(const lexer_node& other,structure::disjoint_set<void*>& equal_set) {
		if (this==&other) return true;
		//std::pair<void*,void*> curr_pair;
		//if ((std::size_t)this<(std::size_t)&other) curr_pair=std::make_pair((void*)this,(void*)&other);
		//else curr_pair=std::make_pair((void*)&other,(void*)this);
		//if (equal_map.count(curr_pair)) return equal_map[curr_pair];
		if (equal_set.is_same((void*)this,(void*)&other) return true;
		if (token_!=other.token_ || end_!=other.end_) return false;
		if (edge_.size()!=other.edge_.size() || epsilon_.size()!=other.epsilon_.size()) return false;
		for (auto& it=edge_.begin(),jt=other.edge_.begin();it!=edge_.end();it++,jt++) {
			if (!it->second->equal_to(*jt->second,equal_map)) return false;
		}
		auto v1=epsilon_,v2=other.epsilon_;
		std::sort(v1.begin(),v1.end());
		std::sort(v2.begin(),v2.end());
		for (int i=0;i<v1.size();i++) {
			if (!v1[i]->equal_to(*v2[i],equal_map)) return false;
		}
		//equal_map[curr_pair]=true;
		equal_set.merge((void*)this,(void*)&other);
		return true;
	}
};

template <typename _Tp,typename _Str=std::string>
class lexer_unit {
	using node=lexer_node<_Tp,_Str>;
public:
	_Str word_;
	_Tp result_;
	enum expression_type {
		ET_NORMAL,
		ET_ONCE,
		ET_TIMES,
		ET_TIMES_WITH_ZERO,
	};
	lexer_unit() : word_("") , result_((_Tp)0) { }
	lexer_unit(_Str word,_Tp result) : word_(word) , result_(result) { }
	lexer_unit(_Str::value_type* word,_Tp result) : word_(word) , result_(result) { }
	lexer_unit(std::initializer_list<std::variant<_Str,_Str::value_type*,_Tp>> init_list) {
		if (init_list.size()!=2) throw std::invalid_argument("The number of the initializer arguments for lexer_unit must be 2!");
		auto it=init_list.begin();
		if (std::holds_alternative<_Str::value_type*>(*it)) word_=_Str(std::get<_Str::value_type*>(*it++));
		else if (std::holds_alternative<_Str>(*it)) word_=std::get<_Str>(*it++);
		else throw std::invalid_argument(std::string("The first argument for lexer_unit must be _Str(")+std::string(typeid(_Str).name())+std::string(") or _Ch(")+std::string(typeid(_Str::value_type).name())+std::string(")*!"));
		if (std::holds_alternative<_Tp>(*it)) result_=std::get<_Tp>(*it);
		else throw std::invalid_argument(std::string("The second argument for lexer_unit must be _Tp(")+std::string(typeid(_Tp).name())+std::string(")!"));
	}
	struct expression_unit {
		union unit_detail {
			std::vector<std::shared_ptr<expression_unit>>* units_;
			std::vector<_Str::value_type>* letters_;
		} word_;
		bool is_units_;
		int length_;
		expression_type exp_type_;
		node* start_node_;
		std::vector<node*> end_nodes_;
		lexer_unit* lexer_unit_;
		int fuzzy_;
		bool is_or_;
		expression_unit(bool is_units=false) {
			is_units_=is_units;
			fuzzy_=-1;
			if (is_units) word_.units_=new std::vector<std::shared_ptr<expression_unit>>();
			else word_.letters_=new std::vector<_Str::value_type>();
			is_or_=false;
		}
		expression_unit(const expression_unit& other) {
			if (other.is_units_) {
				word_.units_=new std::vector<std::shared_ptr<expression_unit>>();
				for (const auto& unit:*other.word_.units_) word_.units_->push_back(std::make_shared<expression_unit>(*unit));
			} else word_.letters_=new std::vector<_Str::value_type>(*other.word_.letters_);
			is_units_=other.is_units_;
			exp_type_=other.exp_type_;
			length_=other.length_;
			start_node_=other.start_node_;
			end_nodes_=other.end_nodes_;
			lexer_unit_=other.lexer_unit_;
			fuzzy_=other.fuzzy_;
			is_or_=other.is_or_;
		}
		~expression_unit() {
			if (is_units_) delete word_.units_;
			else delete word_.letters_;
		}
	};
	enum word_type {
		WT_LETTER,
		WT_PLUS,
		WT_STAR,
		WT_QUESTION,
		WT_LEFT_SQUARE,
		WT_RIGHT_SQUARE,
		WT_LEFT_BRACKET,
		WT_RIGHT_BRACKET,
		WT_CONNECT,
		WT_NOT,
		WT_OR,
	};
	struct word_unit {
		_Str::value_type letter_;
		word_type type_;
	};
	word_unit get_next_char(_Str& str) {
		word_unit result;
		if (!str.size()) return result;
		if (str[0]!='\\') {
			result.letter_=str[0];
			result.type_=WT_LETTER;
			str=str.substr(1);
			return result;
		} else {
			if (str.size()==1) throw std::invalid_argument("Get invalid \'\\\' at the end of "+word_);
			switch (str[1]) {
				case '\\': {
					result.letter_='\\';
					result.type_=WT_LETTER;
					break;
				}
				case '+': {
					result.type_=WT_PLUS;
					break;
				}
				case '*': {
					result.type_=WT_STAR;
					break;
				}
				case '?': {
					result.type_=WT_QUESTION;
					break;
				}
				case '[': {
					result.type_=WT_LEFT_SQUARE;
					break;
				}
				case ']': {
					result.type_=WT_RIGHT_SQUARE;
					break;
				}
				case '(': {
					result.type_=WT_LEFT_BRACKET;
					break;
				}
				case ')': {
					result.type_=WT_RIGHT_BRACKET;
					break;
				}
				case '-': {
					result.type_=WT_CONNECT;
					break;
				}
				case '^': {
					result.type_=WT_NOT;
					break;
				}
				case 'n': {
					result.letter_='\n';
					result.type_=WT_LETTER;
					break;
				}
				case 'r': {
					result.letter_='\r';
					result.type_=WT_LETTER;
					break;
				}
				case 't': {
					result.letter_='\t';
					result.type_=WT_LETTER;
					break;
				}
				case 'c': {
					for (int i=2;i<str.size()-1;i++) {
						if (str[i]=='\\' && str[i+1]=='c') {
							try {
								result.letter_=(_Str::value_type)std::stoi(str.substr(2,i-2));
							} catch (const std::invalid_argument& e) {
								throw std::invalid_argument("Get invalid letter with \'\\c\' at "+word_);
							}
							result.type_=WT_LETTER;
							str=str.substr(i+2);
							return result;
						}
					}
					throw std::invalid_argument("No end \'\\c\' at "+word_);
					break;
				}
				case '|': {
					result.type_=WT_OR;
					break;
				}
				default: {
					throw std::invalid_argument("Get invalid \'"+_Str(1,str[1])+"\'after \'\\\' at "+word_);
					break;
				}
			}
			str=str.substr(2);
			return result;
		}
	}
	expression_unit calc_units(std::vector<word_unit> temp_unit) {
		expression_unit result(true);
		result.lexer_unit_=this;
		result.length_=0;
		result.fuzzy_=0;	
		for (int i=0;i<temp_unit.size();i++) {
			switch (temp_unit[i].type_) {
				case WT_LETTER: {
					expression_unit curr(false);
					curr.lexer_unit_=this;
					curr.word_.letters_->push_back(temp_unit[i].letter_);
					curr.exp_type_=ET_NORMAL;
					if (i!=temp_unit.size()-1) {
						switch (temp_unit[i+1].type_) {
							case WT_PLUS: {
								curr.exp_type_=ET_TIMES;
								result.fuzzy_++;
								i++;
								break;
							}
							case WT_STAR: {
								curr.exp_type_=ET_TIMES_WITH_ZERO;
								result.fuzzy_++;
								i++;
								break;
							}
							case WT_QUESTION: {
								curr.exp_type_=ET_ONCE;
								result.fuzzy_++;
								i++;
								break;
							}
						}
					}
					curr.length_=1;
					result.length_++;
					result.word_.units_->push_back(std::make_shared<expression_unit>(curr));
					break;
				}
				case WT_LEFT_SQUARE: {
					int connect=-2000;
					bool deleted=false;
					expression_unit curr(false);
					curr.lexer_unit_=this;
					while (1) {
						if (i>=temp_unit.size()) throw std::invalid_argument("No ] to fulfill [] at "+word_);
						if (temp_unit[i].type_==WT_RIGHT_SQUARE) break;
						if (i==temp_unit.size()-1) throw std::invalid_argument("No ] to fulfill [] at "+word_);
						if (connect>-2000) {
							if ((int)(temp_unit[i].letter_)<connect) 
								throw std::invalid_argument("Invalid "+_Str((_Str::value_type)connect)+"-"+_Str(temp_unit[i].letter_)+" at "+word_);
							if (!deleted) {
								for (int j=connect;j<=((int)temp_unit[i].letter_);j++) curr.word_.letters_->push_back((_Str::value_type)j);
							} else {
								for (auto it=curr.word_.letters_->begin();it!=curr.word_.letters_->end();) {
									_Str::value_type curr_letter=*it;
									if (curr_letter>=(_Str::value_type)connect && curr_letter<=temp_unit[i].letter_) it=curr.word_.letters_->erase(it);
									else it++;
								}
							}
							connect=-2000;
							deleted=false;
						} else {
							if (temp_unit[i].type_==WT_NOT) {
								if (temp_unit[i+1].type_!=WT_LETTER) throw std::invalid_argument("Not letter after ^ at "+word_);
								deleted=true;	
							} else if (temp_unit[i+1].type_==WT_CONNECT) {
								connect=(int)temp_unit[i].letter_;
								i++;	
							} else {
								if (!deleted) {
									curr.word_.letters_->push_back(temp_unit[i].letter_);
								} else {
									for (auto it=curr.word_.letters_->begin();it!=curr.word_.letters_->end();) {
										_Str::value_type curr_letter=*it;
										if (curr_letter==temp_unit[i].letter_) it=curr.word_.letters_->erase(it);
										else it++;
									}
									deleted=false;
								}
							}
						}
						i++;
					}
					if (connect!=-2000) throw std::invalid_argument("Invalid - at the end of [] at "+word_);
					curr.exp_type_=ET_NORMAL;
					if (i!=temp_unit.size()-1) {
						switch (temp_unit[i+1].type_) {
							case WT_PLUS: {
								curr.exp_type_=ET_TIMES;
								result.fuzzy_++;
								i++;
								break;
							}
							case WT_STAR: {
								curr.exp_type_=ET_TIMES_WITH_ZERO;
								result.fuzzy_++;
								i++;
								break;
							}
							case WT_QUESTION: {
								curr.exp_type_=ET_ONCE;
								result.fuzzy_++;
								i++;
								break;
							}
						}
					}
					curr.length_=1;
					std::sort(curr.word_.letters_->begin(),curr.word_.letters_->end());
					curr.word_.letters_->erase(std::unique(curr.word_.letters_->begin(),curr.word_.letters_->end()),curr.word_.letters_->end());
					result.length_++;
					result.fuzzy_+=curr.word_.letters_->size();
					result.word_.units_->push_back(std::make_shared<expression_unit>(curr));
					break;
				}
				case WT_LEFT_BRACKET: {
					std::vector<word_unit> temp_bracket;
					temp_bracket.clear();
					expression_unit curr(true);
					curr.lexer_unit_=this;
					int jx=0;
					while (1) {
						i++;
						if (i>=temp_unit.size()) throw std::invalid_argument("No ) to fulfill () at "+word_);
						WORD_TYPE type=temp_unit[i].type_;
						if (type==WT_RIGHT_BRACKET) {
							if (jx==0) break;
							else jx++;
						} else if (type==WT_LEFT_BRACKET) {
							jx--;
						}
						if (i==temp_unit.size()-1) throw std::invalid_argument("No ) to fulfill () at "+word_);
						temp_bracket.push_back(temp_unit[i]);	
					}
					auto temp_curr=calc_units(temp_bracket);
					curr.length_=temp_curr.length_;
					curr.fuzzy_=temp_curr.fuzzy_;
					for (auto it:(*temp_curr.word_.units_)) {
						curr.word_.units_->push_back(std::make_shared<expression_unit>(*it));
					}
					curr.exp_type_=ET_NORMAL;
					if (i<temp_unit.size()-1) {
						switch (temp_unit[i+1].type_) {
							case WT_PLUS: {
								curr.exp_type_=ET_TIMES;
								result.fuzzy_++;
								i++;
								break;
							}
							case WT_STAR: {
								curr.exp_type_=ET_TIMES_WITH_ZERO;
								result.fuzzy_++;
								i++;
								break;
							}
							case WT_QUESTION: {
								curr.exp_type_=ET_ONCE;
								result.fuzzy_++;
								i++;
								break;
							}
						}
					}
					result.length_+=curr.length_;
					result.fuzzy_+=curr.fuzzy_;
					result.word_.units_->push_back(std::make_shared<expression_unit>(curr));
					break;
				}
				case WT_OR: {
					if (result.word_.units_->size()==0) throw std::invalid_argument("| at first place is invalid!");
					if ((*result.word_.units_)[result.word_.units_->size()-1]->is_or_) throw std::invalid_argument("Continuous | is invalid!");
					expression_unit curr(false);
					curr.is_or_=true;
					curr.lexer_unit_=this;
					curr.word_.letters_->push_back('|');
					curr.exp_type_=ET_NORMAL;
					curr.length_=1;
					result.length_++;
					result.word_.units_->push_back(std::make_shared<expression_unit>(curr));
					break;
				}
				case WT_PLUS:
				case WT_STAR:
				case WT_QUESTION:
				case WT_RIGHT_SQUARE:
				case WT_RIGHT_BRACKET:
				case WT_CONNECT:
				case WT_NOT:
				default: {
					throw std::invalid_argument("Invalid syntax at "+word_);
					break;
				}	
			}
		}
		return result;
	}
	expression_unit split_to_units(_Str str) {
		std::vector<word_unit> temp_unit;
		_Str temp_str=str;
		while (temp_str.size()) {
			try {
				temp_unit.push_back(get_next_char(temp_str));
			} catch (const std::exception& e) {
				throw;
			}	
		}
		expression_unit result=calc_units(temp_unit);
		result.exp_type_=ET_NORMAL;
		return result;
	}
};

template <typename _Tp,typename _Str=std::string>
class lexer {
	using node=lexer_node<_Tp,_Str>;
	using unit=lexer_unit<_Tp,_Str>;

public:
	struct resolution {
		_Str word_;
		_Tp token_;
		int row_=-1;
		int col_=-1;
		int status_=0;
	};
private:
	struct graph {
		node* start_node_;
		std::vector<node*> nodes_;
		graph() {
			start_node_=nullptr;
			nodes_.clear();
		}
		graph(const graph& other) {
			std::map<node*,node*> mp;
			start_node_=new node(*other.start_node_);
			for (auto it:other.nodes_) {
				node* temp=new node(*it);
				nodes_.push_back(temp);
				mp[it]=temp;
			}
			for (auto it:nodes_) {
				for (auto jt=it->edges_.begin();jt!=it->edges_.end();jt++) {
					if (mp.count(it->edges_[jt->second])) it->edges_[jt->second]=mp[it->edges_[jt->second]];
				}
				for (int i=0;i<it->epsilon_.size();i++) {
					if (mp.count(it->epsilon_[i])) it->epsilon_[i]=mp[it->epsilon_[i]];
				}
			}
		}
		~graph() {
			for (int i=0;i<nodes_.size();i++) delete nodes_[i];
		}
#ifdef _STDEX_OUTPUT_LEXER
		void print() {
			_STDEX_OUTPUT_LEXER<<"start node:"<<start_node_->print()<<"\n\n";
			for (auto& it:nodes_) {
				it->print();
				_STDEX_OUTPUT_LEXER<<"\n";
			}
		}
#endif
	};
public:
	_Tp temporal_;
	_Tp error_;
	std::vector<unit> units_;
private:
	std::map<int,std::map<_Str::value_type,int>> jtable_;
	graph* dfa_map_;
public:
	lexer() {
		temporal_=(_Tp)-1;
		error_=(_Tp)-1;
		dfa_map_=nullptr;
	}
	lexer(_Tp temporal_token,_Tp error_token=(_Tp)-1) : temporal_(temporal_token) , error_(error_token) {
		dfa_map_=nullptr;
	}
	lexer(_Tp temporal_token,_Tp error_token,std::vector<unit> units) : temporal_(temporal_token) , error_(error_token) , units_(units) {
		dfa_map_=nullptr;
	}
	lexer(const lexer& other) {
		temporal_=other.temporal_;
		error_=other.error_;
		units_=other.units_;
		jtable_=other.jtable_;
	}
	lexer(lexer&& other) noexcept {
		temporal_=other.temporal_;
		other.temporal_=(_Tp)-1;
		error_=other.error_;
		other.error_=(_Tp)-1;
		units_=other.units_;
		other.units_.clear();
		jtable_=other.jtable_;
		other.jtable_.clear();
		dfa_map_=other.dfa_map_;
		other.dfa_map_=nullptr;
	}
	~lexer() {
		delete dfa_map_;
	}
private:
	void generate_single_graph(graph* temp_graph,typename unit::expression_unit& unit) {
		if (!unit.is_units_) {
			node* start=new node(temporal_);
			node* end1=new node(temporal_);
			node* end2=new node(temporal_);
			for (auto it:*(unit.word_.letters_)) {
				start->edge_[it]=end1;
				end1->edge_[it]=end2;
				end2->edge_[it]=end2;
			}
			unit.start_node_=start;
			unit.end_nodes_.clear();
			switch (unit.exp_type_) {
				case unit::ET_ONCE: {
					unit.end_nodes_.push_back(start);
					unit.end_nodes_.push_back(end1);
					break;
				}
				case unit::ET_TIMES: {
					unit.end_nodes_.push_back(end1);
					unit.end_nodes_.push_back(end2);
					break;
				}
				case unit::ET_TIMES_WITH_ZERO: {
					unit.end_nodes_.push_back(start);
					unit.end_nodes_.push_back(end1);
					unit.end_nodes_.push_back(end2);
					break;
				}
				case unit::ET_NORMAL:
				default: {
					unit.end_nodes_.push_back(end1);
					break;
				}
			}
			temp_graph->nodes_.push_back(start);
			temp_graph->nodes_.push_back(end1);
			temp_graph->nodes_.push_back(end2);
		} else {
			if (!unit.word_.units_->size()) return;
			if ((*unit.word_.units_)[unit.word_.units_->size()-1]->is_or_) throw std::invalid_argument("| at last place is invalid!");
			for (int i=0;i<unit.word_.units_->size();i++) {
				if (!(*unit.word_.units_)[i]->is_or_) generate_single_graph(temp_graph,*((*unit.word_.units_)[i]));
			}
			std::vector<std::vector<std::shared_ptr<typename unit::expression_unit>>> splited_units;
			for (int i=0;i<unit.word_.units_->size();i++) {
				if ((*unit.word_.units_)[i]->is_or_) continue;
				if (i==unit.word_.units_->size()-1 || !(*unit.word_.units_)[i+1]->is_or_) {
					std::vector<std::shared_ptr<typename unit::expression_unit>> temp={(*unit.word_.units_)[i]};
					splited_units.push_back(temp);
					continue;
				}
				std::vector<std::shared_ptr<typename unit::expression_unit>> temp={(*unit.word_.units_)[i]};
				while (i+2<unit.word_.units_->size()) {
					i+=2;
					if (i==unit.word_.units_->size()-1 || !(*unit.word_.units_)[i+1]->is_or_) {
						temp.push_back((*unit.word_.units_)[i]);
						break;
					}
					temp.push_back((*unit.word_.units_)[i]);
				}
				splited_units.push_back(temp);
			}
			
			for (int i=0;i<splited_units.size()-1;i++) {
				for (auto& it:splited_units[i]) {
					for (int j=0;j<it->end_nodes_.size();j++) {
						for (auto& jt:splited_units[i+1]) it->end_nodes_[j]->epsilons_.push_back(jt->start_node_);
					}
				}
			}
			int end_id=splited_units.size()-1;
			switch (unit.exp_type_) {
				case unit::ET_ONCE: {
					for (auto& it:splited_units[0]) {
						for (auto& jt:splited_units[end_id]) {
							for (int i=0;i<jt->end_nodes_.size();i++) {
								it->start_node_->epsilons_.push_back(jt->end_nodes_[i]);
							}
						}
					}
					break;
				}
				case unit::ET_TIMES: {
					for (auto& it:splited_units[0]) {
						for (auto& jt:splited_units[end_id]) {
							for (int i=0;i<jt->end_nodes_.size();i++) {
								jt->end_nodes_[i]->epsilons_.push_back(it->start_node_);
							}
						}
					}
					break;
				}
				case unit::ET_TIMES_WITH_ZERO: {
					for (auto& it:splited_units[0]) {
						for (auto& jt:splited_units[end_id]) {
							for (int i=0;i<jt->end_nodes_.size();i++) {
								it->start_node_->epsilons_.push_back(jt->end_nodes_[i]);
								jt->end_nodes_[i]->epsilons_.push_back(it->start_node_);
							}
						}
					}
					break;
				}
				case unit::ET_NORMAL:
				default: {
					break;
				}
			}
			node* temp_start=new node(temporal_);
			node* temp_end=new node(temporal_);
			for (auto& it:splited_units[0]) temp_start->epsilons_.push_back(it->start_node_);
			for (auto& it:splited_units[end_id]) {
				for (int i=0;i<it->end_nodes_.size();i++) it->end_nodes_[i]->epsilons_.push_back(temp_end);
			}
			unit.start_node_=temp_start;
			unit.end_nodes_.clear();
			unit.end_nodes_.push_back(temp_end);
			temp_graph->nodes_.push_back(temp_start);
			temp_graph->nodes_.push_back(temp_end);
			//-(epsilon)->result-(epsilon)->
		}	
	}
	void get_equivalence(node* root,std::set<node*>& visited) {
		visited.insert(root);
		for (int i=0;i<root->epsilons_.size();i++) {
			if(visited.find(root->epsilons_[i])==visited.end()) get_equivalence(root->epsilons_[i],visited);
		}
	}
	graph* convert_to_dfa(graph* temp_graph) {
		graph* result=new graph();
		std::vector<std::set<node*>> dfa_queue;
		std::set<node*> temp_set;
		temp_set.clear();
		get_equivalence(temp_graph->start_node_,temp_set);
		dfa_queue.push_back(temp_set);
		node* start=new node(temporal_);
		result->start_node_=start;
		result->nodes_.push_back(start);
		for (auto it:temp_set) {
			if (it->end_) {
				start->end_=true;
				start->token_=it->token_;
			}
		}
		int i=0;
		while (i<dfa_queue.size()) {
			//Get All Output
			std::set<_Str::value_type> out_edges;
			for (auto it:dfa_queue[i]) {
				for (auto jt:it->edge_) {
					out_edges.insert(jt.first);
				}
			}
			//Get Out Array
			for (auto it:out_edges) {
				temp_set.clear();
				for (auto jt=dfa_queue[i].begin();jt!=dfa_queue[i].end();jt++) {
					auto kt=(*jt)->edge_.find(it);
					if (kt!=(*jt)->edge_.end()) {
						get_equivalence(kt->second,temp_set);
					}
				}
				auto lt=std::find(dfa_queue.begin(),dfa_queue.end(),temp_set);
				if (lt!=dfa_queue.end()) result->nodes_[i]->edge_[it]=result->nodes_[std::distance(dfa_queue.begin(),lt)];	
				else {
					//new node
					dfa_queue.push_back(temp_set);
					node* new_node=new node(temporal_);
					int curr_fuzzy=INT_MAX;
					for (auto mt:temp_set) {
						if (mt->end_ && mt->fuzzy_<curr_fuzzy) {
							new_node->end_=true;
							new_node->token_=mt->token_;
							curr_fuzzy=mt->fuzzy_;
						}
					}
					result->nodes_.push_back(new_node);
					result->nodes_[i]->edge_[it]=new_node;
				}
			}
			temp_set.clear();
			i++;	
		}
		return result;
	}
	void skip_useless_nodes(graph* dfa_graph) {
		std::set<node*> delete_list;
		for (auto& it=dfa_graph->nodes_.begin();it!=dfa_graph->nodes_.end();) {
			bool deleted=true;
			for (auto& jt:(*it)->edge_) {
				if (jt.second!=*it) deleted=false;
			}
			if ((*it)->end_) deleted=false;
			if (deleted) {
				delete_list.insert(*it);
				it=dfa_graph->nodes_.erase(it); 	
			}
			else it++;
		}
		for (auto& it:delete_list) {
			for (auto& jt=dfa_graph->nodes_.begin();jt!=dfa_graph->nodes_.end();jt++) {
				for (auto& kt=(*jt)->edge_.begin();kt!=(*jt)->edge_.end();) {
					if (kt->second==it) kt=(*jt)->edge_.erase(kt);
					else kt++;	
				}
			}	
		}
	}
	void merge_nodes(graph* dfa_graph) {
		//std::map<std::pair<void*,void*>,bool> equal_map;
		structure::disjoint_set<void*> equal_set;
		for (int i=0;i<dfa_graph->nodes_.size()-1;i++) {
			for (int j=i+1;j<dfa_graph->nodes_.size();) {
				node* node1=dfa_graph->nodes_[i];
				node* node2=dfa_graph->nodes_[j];
				if (node1->equal_to(*node2,equal_set)) {
					//adjust j->i
					for (auto it:dfa_graph->nodes_) {
						for (auto jt:it->edge_) {
							if (jt.second==node2) jt.second=node1;
						}
					}
					//delete self
					dfa_graph->nodes_.erase(dfa_graph->nodes_.begin()+j);
				} else j++;
			}
		}
	}
	void clear_nodes(graph* dfa_graph) {
		structure::disjoint_set<node*> graph_set;
		for (auto& it=dfa_graph->nodes_.begin();it!=dfa_graph->nodes_.end();it++) graph_set.emplace(*it);
		for (auto& it=dfa_graph->nodes_.begin();it!=dfa_graph->nodes_.end();it++) {
			for (auto& jt:(*it)->edge_) graph_set.merge(*it,jt.second,false);
		}
		std::set<node*> delete_list;
		auto sets=graph_set.sets();
		for (auto& it:sets) {
			bool delete=true;
			for (auto& jt:it) {
				if (jt==start_node || jt->end_) {
					delete=false;
					break;
				}
			}
			if (delete) {
				for (auto& jt:it) delete_list.insert(jt);
			}
		}
		for (auto& it:delete_list) {
			delete it;
			dfa_graph->nodes_.erase(std::remove(dfa_graph->nodes_.begin(),dfa_graph->nodes_.end(),it),dfa_graph->nodes_.end());
		}
		for (auto& it:dfa_graph->nodes_) {
			for (auto jt=it->edges_.begin();jt!=it->edges_.end();) {
				if (delete_list.count(jt->second)) jt=it->edges_.erase(jt);
				else jt++;
			}
		}	
	}
	int get_node_id(graph* dfa_graph,node* the_node) {
		auto it=std::find(dfa_graph->nodes_.begin(),dfa_graph->nodes_.end(),the_node);
		if (it!=dfa_graph->nodes_.end()) return std::distance(dfa_graph->nodes_.begin(),it);
		return -1;
	}
	void construct_jtable(graph* dfa_graph) {
		jtable_.clear();
		for (int i=0;i<dfa_graph->nodes_.size();i++) {
			std::map<node*,int> temp_map;
			for (auto it:dfa_graph->nodes_[i]->edge_) {
				if (!temp_map.count(it.second)) temp_map[it.second]=get_node_id(dfa_graph,it.second);
			}
			for (auto it:dfa_graph->nodes_[i]->edge_) {
				if (temp_map.count(it.second) && temp_map[it.second]!=-1) {
					//if (!jtable_.count(i))
					jtable_[i][it.first]=temp_map[it.second];
				}
			}
		}
	}
	int get_next_state(int curr_state,_Str::value_type letter) {
		if (!jtable_.count(curr_state)) return -1;
		if (!jtable_[curr_state].count(letter)) return -1;
		return jtable_[curr_state][letter];
	}
	bool get_end(int curr_state) {
		if(curr_state>=dfa_map_->nodes_.size()) throw std::out_of_range("Unrecognized state!");
		return dfa_map_->nodes_[curr_state]->end_;
	}
	_Tp get_category(int curr_state){
		if(curr_state>=dfa_map_->nodes_.size()) return error_;
		return dfa_map_->nodes_[curr_state]->token_;
	}
public:
	void generate_lexer() {
		delete dfa_map_;
		std::vector<typename unit::expression_unit> temp_units;
		graph* temp_graph=new graph();
		node* temp_start=new node(temporal_);
		//temp_start->start_=true;
		temp_graph->nodes_.push_back(temp_start);
		temp_graph->start_node_=temp_start;
		for (int i=0;i<units_.size();i++) {
			temp_units.push_back(units_[i].split_to_units(units_[i].word_));
		}
		//Construct NFA graph
		for (int i=0;i<temp_units.size();i++) {
			//construct graph
			generate_single_graph(temp_graph,temp_units[i]);
			//add end node
			node* temp_end=new node(temporal_);
			temp_end->token_=temp_units[i].lexer_unit_->result_;
			temp_end->end_=true;
			for (int j=0;j<temp_units[i].end_nodes_.size();j++) {
				temp_units[i].end_nodes_[j]->epsilons_.push_back(temp_end);
			}
			temp_graph->nodes_.push_back(temp_end);
			temp_end->fuzzy_=temp_units[i].fuzzy_;
			//merge graphs
			temp_start->epsilons_.push_back(temp_units[i].start_node_);
		}
#ifdef _STDEX_OUTPUT_LEXER
		_STDEX_OUTPUT_LEXER<<"NFA Map:\n";
		temp_graph->print();
#endif
		//Transform NFA graph to DFA graph
		graph* result_graph=convert_to_dfa(temp_graph);
		delete temp_graph;
		//Delete Useless Nodes in DFA graph
		skip_useless_nodes(result_graph);
		//Merge Equal Nodes
		merge_nodes(result_graph);
		//Clear Useless Nodes
		clear_nodes(result_graph);
		//Transform DFA graph to JumpTable
		construct_jtable(result_graph);
#ifdef _STDEX_OUTPUT_LEXER
		_STDEX_OUTPUT_LEXER<<"DFA Map:\n";
		result_graph->print();
#endif
		dfa_map_=result_graph;
	}
	std::vector<resolution> resolve(_Str text) {
		bool temp;
		return resolve(text,temp);
	}
	std::vector<resolution> resolve(_Str text,bool& success) {
		success=true;
		if (!jtable_.size()) throw std::invalid_argument("Lexer is not constructed! Please call generate_lexer() first.");
		std::vector<resolution> result;
		int current_state=0,start_index=0,current_index=0,curr_line=0,curr_column=0,line=0,column=0;
		while (current_index<text.size() || (current_index==text.size() && current_state!=0)) {
			int next_state=(current_index==text.size()?-1:get_next_state(current_state,text[current_index]));
			if (next_state==-1) {
				if (get_end(current_state)) {
					_Tp curr_token=get_category(current_state);
					if (curr_token!=error_) {
						_Str temp_word=text.substr(start_index,current_index-start_index);
#ifdef _STDEX_OUTPUT_LEXER
						_STDEX_OUTPUT_LEXER<<"Detected Word:\n'"<<temp_word;
#endif
						resolution res;
						res.token_=curr_token;
						res.word_=temp_word;
						res.row_=curr_line;
						res.col_=curr_column;
						res.status_=0;
#ifdef _STDEX_OUTPUT_LEXER
						_STDEX_OUTPUT_LEXER<<"'\nDFAnodeid="<<std::setw(3)<<std::setfill('0')<<current_state<<"->DFAtokenid="<<static_cast<int>(get_category(current_state))<<" at line="<<std::setw(2)<<std::setfill('0')<<curr_line<<"/col="<<std::setw(2)<<std::setfill('0')<<curr_column<<"\n\n";		
#endif
						curr_line=line;
						curr_column=column;
						result.push_back(res);
						start_index=current_index;
						current_state=0;
					} else {
						_Str temp_word=text.substr(start_index,current_index-start_index);
#ifdef _STDEX_OUTPUT_LEXER
						#ifdef _STDEX_OUTPUT_PARSER<<"Undefined Word!\n\n'"<<temp_word<<"\n";//if raise_err? Or raise In getCategoryInGraph? I like the first one.
#endif
						resolution res;
						res.token_=curr_token;
						res.word_=temp_word;
						res.row_=curr_line;
						res.col_=curr_column;
						res.status_=1;
						result.push_back(res);
						success=false;
						start_index=current_index;
						current_state=0;
					}
				} else {
					if (current_index<text.size()) {
#ifdef _STDEX_OUTPUT_LEXER
			    			_STDEX_OUTPUT_LEXER<<"Err:State at "<<current_state<<" with word as '"<<text[current_index]<<"' at line "<<line<<" & col "<<column<<"\n";
#endif
						_Str temp_word=text.substr(start_index,current_index-start_index);
						resolution res;
						res.token_=error_;
						res.word_=temp_word;
						res.row_=curr_line;
						res.col_=curr_column;
						res.status_=2;
						result.push_back(res);
						success=false;
						return result;
					} else current_state=0;
					break;
				}
			} else {
				current_state=next_state;
				if(text[current_index]=='\n'){
					line++;
					column=-1;
				}
				current_index++;
				column++;
			}
		}
		return result;
	}
};
	
}

}

#endif