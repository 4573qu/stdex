//Last Modified At 2025/04/17
//@Version 1.0
//@H_Version 1.06
#include "database.h"
std::meta::core::database::expression::expression() {
	exp_.exps_.exp1_=nullptr;
	exp_.exps_.exp2_=nullptr;
}

std::meta::core::database::expression::expression(const expression& other) : is_expressions_(other.is_expressions_) {
	if (is_expressions_) {
		if (other.exp_.exps_.exp1_) exp_.exps_.exp1_=new expression(*(other.exp_.exps_.exp1_));
		if (other.exp_.exps_.exp2_)  exp_.exps_.exp2_=new expression(*(other.exp_.exps_.exp2_));
		exp_.exps_.op_=other.exp_.exps_.op_;
	} else exp_.an_exp_=other.exp_.an_exp_;
}

std::meta::core::database::expression::~expression() {
	if (is_expressions_) {
		delete exp_.exps_.exp1_;
		delete exp_.exps_.exp2_;
	}
}

void std::meta::core::database::expression::calculate(database* db,type_registry reg) {
	if (!is_expressions_) return;
	if (exp_.exps_.exp1_->is_expressions_) exp_.exps_.exp1_->calculate(db,db->reg_);
	if (exp_.exps_.exp2_->is_expressions_) exp_.exps_.exp2_->calculate(db,db->reg_);
	metadata* m=new metadata(db->reg_,false);
	metadata::field_def fd1,fd2;
	fd1.name_="1";
	fd1.type_=exp_.exps_.exp1_->exp_.an_exp_.type_;
	m->struct_def_.fields_.push_back(fd1);
	fd2.name_="2";
	fd2.type_=exp_.exps_.exp2_->exp_.an_exp_.type_;
	m->struct_def_.fields_.push_back(fd2);
	m->calculate_offsets();
	
	dynamic_struct* ds=new dynamic_struct(m);
	
	db->from_string_func(exp_.exps_.exp1_->exp_.an_exp_.type_)(exp_.exps_.exp1_->exp_.an_exp_.val_,(*ds)["1"].ptr_);
	db->from_string_func(exp_.exps_.exp2_->exp_.an_exp_.type_)(exp_.exps_.exp2_->exp_.an_exp_.val_,(*ds)["2"].ptr_);
	
	std::string result_type="number";
	std::string result=(db->*(db->operator_func(exp_.exps_.exp1_->exp_.an_exp_.type_,exp_.exps_.exp2_->exp_.an_exp_.type_,(std::meta::core::database::operand_type)exp_.exps_.op_)))((*ds)["1"].ptr_,(*ds)["2"].ptr_,result_type);
	is_expressions_=false;
	delete exp_.exps_.exp1_;
	delete exp_.exps_.exp2_;
	exp_.exps_.exp1_=exp_.exps_.exp2_=nullptr;
	exp_.an_exp_.type_=result_type;
	exp_.an_exp_.val_=result;//db->to_string_func(result_type)(result);
	exp_.an_exp_.is_val_=true;
	delete ds;
	delete m;
}

std::string(std::meta::core::database::* std::meta::core::database::operator_func(std::string type1,std::string type2,operand_type op))(void*,void*,std::string&) {
	auto ops=operator_map_[std::make_pair(type1,type2)];
	if (ops.size()<=(int)op) return null_operator;
	if (ops[(int)op]==nullptr) return null_operator;
	return ops[(int)op];
}
	
std::string std::meta::core::database::null_operator(void* val1,void* val2,std::string& type) {
	type="number";
	/*intptr_t* number=new intptr_t();
	*number=0;
	return (void*)number;*/
	return "0";
}

void std::meta::core::database::database_listener::init(std::vector<std::syntax::lexer<token>::resolution> resolutions) {
	if (!db_) throw std::invalid_argument("No database to parse!");
	type_=DBL_STMT_UNKNOWN;
	resolutions_=resolutions;
	stack_.clear();
	curr_=0;
			
	formats_.clear();
	curr_format_.name_=curr_format_.type_="";
	ids_.clear();
	curr_id_="";
	values_.clear();
	curr_value_status_=0;
	curr_value1_=curr_value2_=nullptr;
	//curr_value_.type_=curr_value_.val_.str_="";
	curr_condition_status_=0;
	curr_condition1_=curr_condition2_=nullptr;
	
	current_op_.clear();
			
	message_="";
	success_=true;
	
	temp_result_=nullptr;
	
	order_=0;
	order_id_="";
}

void std::meta::core::database::database_listener::on_shift(int id,int state,std::meta::core::database::token word) {
	stack_.push_back(resolutions_[curr_++]); //cout<<"shift "<<state<<" at "<<id<<endl;
	std::map<token,stmt_type> stmt_map={{TK_CREATE,DBL_STMT_CREATE},{TK_INSERT,DBL_STMT_INSERT},{TK_UPDATE,DBL_STMT_UPDATE},
										{TK_SELECT,DBL_STMT_SELECT},{TK_DELETE,DBL_STMT_DELETE},{TK_SHOW,  DBL_STMT_SHOW  },
										{TK_SHOWTABLES,DBL_STMT_STB},{TK_LOAD, DBL_STMT_LOAD  },{TK_SAVE,  DBL_STMT_SAVE  },
										{TK_HELP,  DBL_STMT_HELP}};
	if (stmt_map.count(word) && type_==DBL_STMT_UNKNOWN) {//INSERT xxx SELECT xxx,SELECT must not override type
		type_=stmt_map[word];
		return;
	}
}

void std::meta::core::database::database_listener::on_reduction(int id,int state,int next,int sentence_id,int reduction_num) {
	if (!db_) throw std::invalid_argument("No database to parse!");
	std::vector<std::syntax::lexer<token>::resolution> curr_nodes(stack_.end()-reduction_num,stack_.end());
	stack_.erase(stack_.end()-reduction_num,stack_.end());
	std::syntax::lexer<token>::resolution result;
	if (!curr_nodes.empty()) {
		result.row_=curr_nodes[0].row_;
		result.col_=curr_nodes[0].col_;
	}
	switch (sentence_id) {
		case  1: case  2: case  3: case  4: case  5: case  6: case  7: case  8: case  9: case 10:
		case 79: {
			//if (type_!=DBL_STMT_UNKNOWN) throw std::logic_error("Get statement type more than once!");
			type_=(stmt_type)sentence_id; break;
		}
		case 11: case 12: {
			std::string table_id=curr_nodes[1].word_;
			metadata m(db_->reg_,false);
			m.struct_def_.name_=table_id;
			for (int i=0;i<formats_.size();i++) {
				metadata::field_def fd;
				if (formats_[i].name_=="*") throw std::invalid_argument("\"*\" cannot be a segment name!");
				fd.name_=formats_[i].name_;
				fd.type_=formats_[i].type_;
				if (!db_->reg_.has_type(fd.type_)) {
					message_="Invalid type name:"+fd.type_+"!";
					success_=false;
					goto switch_end;
				}
				m.struct_def_.fields_.push_back(fd);
			}
			std::vector<std::string> pks;
			if (sentence_id==12) {
				for (int i=0;i<ids_.size();i++) {
					bool find=false;
					for (auto it:m.struct_def_.fields_) {
						if (it.name_==ids_[i]) find=true;
					}
					if (!find) {
						message_+="PK name:"+ids_[i]+" is not existed in table!\n";
						success_=false;
						break;
					}
					if (std::find(pks.begin(),pks.end(),ids_[i])!=pks.end()) {
						message_="Duplicated pk name:"+ids_[i]+"!\n";
						success_=false;
						break;
					}
					pks.push_back(ids_[i]);
				}
			}
			m.calculate_offsets();
			bool success=true;
			if (pks.empty()) db_->create_table(table_id,m);
			else db_->create_table(table_id,m,pks);
			if (!success) {
				message_+="Existed table name!\n";
				success_=false;
				break;
			} else message_="";
			break;
		}
		case 13: { formats_={curr_format_}; break; }
		case 14: { formats_.push_back(curr_format_); break; }
		case 15: {
			if (!temp_result_) success_=false;
			else {
				std::string table_id=curr_nodes[1].word_;
				metadata m(db_->reg_,false);
				m.struct_def_=temp_result_->meta_.struct_def_;
				m.size_=temp_result_->meta_.size_;
				m.strict_==temp_result_->meta_.strict_;
				//m=temp_result_->m
				m.calculate_offsets();
				bool success=db_->create_table(table_id,m);
				if (!success) {
					message_+="Existed table name!\n";
					success_=false;
					break;
				} else message_="";
				break;
			}
			break;
		}
		case 16: { curr_format_.name_=curr_nodes[0].word_; curr_format_.type_=curr_nodes[2].word_; break; }
		case 17: {
			std::string table_id=curr_nodes[1].word_;
			int success=db_->insert(table_id,values_,message_);
			//delete m;
			if (!success) {
				message_="";
				break;
			} else success_=false;
			break;
		}
		case 18: { values_={curr_value1_}; curr_value_status_=0; curr_value1_=nullptr; break; }
		case 19: { values_.push_back(curr_value1_); curr_value_status_=0; curr_value1_=nullptr; break; }
		case 23: case 25: case 27: case 29: case 30: case 32: case 33: case 34: {
			std::map<int,expression_type> op_map={{23,ET_OR},{25,ET_XOR},{27,ET_AND},{29,ET_ADD},{30,ET_SUB},
												  {32,ET_MUL},{33,ET_DIV},{34,ET_MOD}};
			expression* exp=new expression();
			exp->is_expressions_=true;
			exp->exp_.exps_.exp1_=curr_value1_;
			exp->exp_.exps_.exp2_=curr_value2_;
			exp->exp_.exps_.op_=op_map[sentence_id];
			curr_value1_=exp;
			curr_value2_=nullptr;
			curr_value_status_=1;
			break;
		}
		case 39: case 40: case 41: case 42: case 43: {
			std::map<int,std::string> literal_map={{39,"array"},{40,"string"},{41,"bool"},{42,"real"},{43,"number"}};
			expression** an_exp=nullptr;
			if (curr_value_status_==0) {
				an_exp=&curr_value1_;
				curr_value_status_=1;
			} else if (curr_value_status_==1) {
				an_exp=&curr_value2_;
				curr_value_status_=2;
			} else {
				throw std::invalid_argument("Get the third operand at expression!");
				break;
			}
			//delete *an_exp;
			(*an_exp)=new expression();
			(*an_exp)->is_expressions_=false;
			(*an_exp)->exp_.an_exp_.val_=curr_nodes[0].word_;
			(*an_exp)->exp_.an_exp_.type_=literal_map[sentence_id];
			(*an_exp)->exp_.an_exp_.is_val_=true;
			break;
		}
		case 44: case 45: {
			expression** an_exp=nullptr;
			if (curr_value_status_==0) {
				an_exp=&curr_value1_;
				curr_value_status_=1;
			} else if (curr_value_status_==1) {
				an_exp=&curr_value2_;
				curr_value_status_=2;
			} else {
				throw std::invalid_argument("Get the third operand at expression!");
				break;
			}
			//delete *an_exp;
			(*an_exp)=new expression();
			(*an_exp)->is_expressions_=false;
			(*an_exp)->exp_.an_exp_.segment_=curr_nodes[(sentence_id==44)?4:0].word_;
			if (sentence_id==44) (*an_exp)->exp_.an_exp_.table_=curr_nodes[1].word_;
			(*an_exp)->exp_.an_exp_.is_val_=false;
			break;
		}
		case 47: {
			std::string table_name=curr_nodes[1].word_;
			int success=db_->update(table_name,ids_,values_,curr_condition1_);
			if (!success) {
				message_="";
			} else {
				switch (success) {
					case 1: {
						message_="Table does not exist!";
						break;
					}
					case 2: {
						message_="Invalid segment name in UPDATE SENTENCE!";
						break;
					}
					case 3: {
						message_="Invalid segment name in UPDATE SENTENCE!";
						break;
					}
					case 4: {
						message_="Invalid segment name of other table in UPDATE SENTENCE!";
						break;
					}
					case 5: {
						message_="Invalid operator between 2 conditions!";
						break;
					}
					case 6: {
						message_="Invalid expression in conditions!";
						break;
					}
					case 7: {
						message_="Failed to translate expression!";
						break;
					}
					case 8: {
						message_="Mismatch amount of id and value!";
						break;
					}
					case 9: {
						message_="Failed to get the value of expression!";
						break;
					}
					case 10: {
						message_="Failed to update value!";
						break;
					}
					default: {
						message_+="Unknown error of UPDATE SENTENCE!";
						break;
					}
				}
			}
			break;
		}
		case 52: case 54: {
			if (curr_condition_status_!=2) throw std::invalid_argument("Get invalid operand at condition!");
			else {
				condition* an_cond=new condition();
				an_cond->is_conditions_=true;
				an_cond->val1_=curr_condition1_;
				an_cond->val2_=curr_condition2_;
				an_cond->op_.clear();
				an_cond->op_<<=(sentence_id==52)?CT_AND:CT_OR;
				curr_condition_status_=1;
				curr_condition1_=an_cond;
				curr_condition2_=nullptr;
			}
			break;
		}
		case 58: {
			condition** an_cond=nullptr;
			if (curr_condition_status_==0) {
				an_cond=&curr_condition1_;
				curr_condition_status_=1;
			} else if (curr_condition_status_==1) {
				an_cond=&curr_condition2_;
				curr_condition_status_=2;
			} else {
				throw std::invalid_argument("Get the third condition at expression!");
				break;
			}
			if (curr_value_status_!=1) {
				throw std::invalid_argument("Invalid expressions at condition at expression!");
				break;
			}
			if (!(*an_cond)) {
				throw std::logic_error("Lack condition\'s first part!");
				break;
			}
			(*an_cond)->is_conditions_=false;
			(*an_cond)->val2_=(void*)curr_value1_;
			//(*an_cond)->op_=current_op_;
			curr_value1_=nullptr;
			curr_value_status_=0;
			break;
		}
		case 59: case 60: case 61: case 62: case 63: case 64: case 65: {
			if (!current_op_.empty()) {
				throw std::invalid_argument("Too many operand in condition expression!");
				break;
			}
			switch (sentence_id) {
				case 59: current_op_<<=CT_EQUAL; break;
				case 60: current_op_=current_op_<<CT_NOT<<CT_EQUAL; break;
				case 61: current_op_<<=CT_LESS; break;
				case 62: current_op_=current_op_<<CT_LESS<<CT_EQUAL; break;
				case 63: current_op_<<=CT_GREAT; break;
				case 64: current_op_=current_op_<<CT_GREAT<<CT_EQUAL; break;
				case 65: current_op_<<=CT_CONTAINS; break;
				default: {
					throw std::invalid_argument("Invalid ccondition operand!");
					break;
				}
			}
			condition** an_cond=nullptr;
			if (curr_condition_status_==0) an_cond=&curr_condition1_;
			else if (curr_condition_status_==1) an_cond=&curr_condition2_;
			else {
				throw std::invalid_argument("Get the third condition at expression!");
				break;
			}
			if (curr_value_status_!=1) {
				throw std::invalid_argument("Invalid expressions at condition at expression!");
				break;
			}
			(*an_cond)=new condition();
			(*an_cond)->is_conditions_=false;
			(*an_cond)->val1_=(void*)curr_value1_;
			(*an_cond)->op_=current_op_;
			curr_value1_=nullptr;
			curr_value_status_=0;
			current_op_.clear();
			break;
		}
		case 70: { ids_={curr_nodes[0].word_}; break; }
		case 71: { ids_.push_back(curr_nodes[2].word_); break; }
		case 72: { ids_={"*"}; break; }//MAYBE EMPTY IS A GOOD CHOICE
		case 73: {
			//SELECT ids FROM id WHERE
			//2.ForEach in id
			
			//3.Test if fulfill the conditions(also test whether col_name exists)
			//4.Get Format And Make Temp Meta(db_->)
			//5.Meta->Dynamic_struct
			//6.->TempResult
			
			//int select(std::string name,std::vector<std::string> ids,condition* conds,table* result)
			table* res_table=nullptr;
			int result=db_->select(curr_nodes[3].word_,ids_,curr_condition1_,&res_table);
			switch (result) {
				case 0: {
					if (res_table) temp_result_=res_table;
					else {
						message_="SELECT failed.\n";
						success_=false;
					}
					break;
				}
				case 1: {
					message_="Table(select) does not exist!";
					break;
				}
				case 2: {
					message_="Invalid segment name in SELECT SENTENCE!";
					break;
				}
				case 3: {
					message_="Invalid segment name of other table in SELECT SENTENCE!";
					break;
				}
				case 4: {
					message_="Invalid segment name of other table in SELECT SENTENCE!";
					break;
				}
				case 5: {
					message_="Invalid operator between 2 conditions!";
					break;
				}
				case 6: {
					message_="Invalid expression in conditions!";
					break;
				}
				default: {
					message_+="Unknown error of SELECT SENTENCE!";
					break;
				}
			}
			if (result) {
				success_=false;
				temp_result_=nullptr;
			} else {
				result=db_->sort_table(temp_result_,order_id_,order_);
				switch (result) {
					case 1: {
						message_="Table not exists when ordering!";
						break;
					}
					case 2: {
						message_="Invalid type name when ordering!";
						break;
					}
				}
				if (result) {
					success_=false;
					delete temp_result_;
					temp_result_=nullptr;
				}
			}
			break;
		}
		case 74: {
			if (!temp_result_) success_=false;
			else {
				if (!temp_result_->datas_.empty()) message_=db_->show_table(temp_result_);
				delete temp_result_;
				temp_result_=nullptr;
			}
			break;
		}
		case 75: {
			if (!temp_result_) success_=false;
			else {
				std::string table_id=curr_nodes[1].word_;
				metadata* m=new metadata(db_->reg_,false);
				m->struct_def_=temp_result_->meta_.struct_def_;
				m->size_=temp_result_->meta_.size_;
				m->strict_=temp_result_->meta_.strict_;
				//m=temp_result_->m
				m->calculate_offsets();
				bool success=db_->create_table(into_id_,*m);
				if (!success) {
					int i=db_->find_table(into_id_);
					if (i<0) {
						message_+="Existed table name!\n";
						success_=false;
						break;
					} else {
						if ((*m)!=db_->tables_[i]->meta_) {
							message_="Metadata mismatch!";
							success_=false;
							break;
						} else {
							table* temp_table=db_->tables_[i];
							for (auto& it:temp_result_->datas_) {
								dynamic_struct ds(m);
								for (int i=0;i<m->struct_def_.fields_.size();i++) {
									std::string temp_str=db_->to_string_func(m->struct_def_.fields_[i].type_)(it[m->struct_def_.fields_[i].name_].ptr_);
									db_->from_string_func(m->struct_def_.fields_[i].type_)(temp_str,ds[m->struct_def_.fields_[i].name_].ptr_);
								}
								db_->insert(into_id_,ds);
							}
							message_="";
						}
					}
				} else {
					int i=db_->find_table(into_id_);
					if (i<0) {
						message_="Create table failed.\n";
						success_=false;
						break;
					} else {
						table* temp_table=db_->tables_[i];
						for (auto& it:temp_result_->datas_) {
							dynamic_struct ds(m);
							for (int i=0;i<m->struct_def_.fields_.size();i++) {
								std::string temp_str=db_->to_string_func(m->struct_def_.fields_[i].type_)(it[m->struct_def_.fields_[i].name_].ptr_);
								db_->from_string_func(m->struct_def_.fields_[i].type_)(temp_str,ds[m->struct_def_.fields_[i].name_].ptr_);
							}
							temp_table->datas_.push_back(ds);
						}
						message_="";
					}
				}
				//delete m;
				break;
			}
			break;
		}
		case 76: {
			into_id_=curr_nodes[1].word_;
			break;
		}
		case 78: {
			int i=db_->find_table(curr_nodes[1].word_);
			if (i<0) {
				message_="Table does not exist!";
				success_=false;
			} else {
				db_->tables_[i]->datas_.clear();
				//delete_row(tables_[i],nullptr?)
			}
			break;
		}
		case 80: {
			expression** an_exp=nullptr;
			if (curr_value_status_==0) {
				an_exp=&curr_value1_;
				curr_value_status_=1;
			} else if (curr_value_status_==1) {
				an_exp=&curr_value2_;
				curr_value_status_=2;
			} else {
				throw std::invalid_argument("Get the third operand at expression!");
				break;
			}
			//delete *an_exp;
			(*an_exp)=new expression();
			(*an_exp)->is_expressions_=false;
			(*an_exp)->exp_.an_exp_.segment_="<ID>";
			(*an_exp)->exp_.an_exp_.table_="";
			(*an_exp)->exp_.an_exp_.is_val_=false;
			break;
			break;
		}
		case 81: {
			std::string table_name=curr_nodes[2].word_;
			int success=db_->delete_row(table_name,curr_condition1_);
			if (!success) {
				message_="";
			} else {
				switch (success) {
					case 1: {
						message_="Table does not exist!";
						break;
					}
					case 2: {
						message_="Invalid segment name in DELETE SENTENCE!";
						break;
					}
					case 3: {
						message_="Invalid segment name of other table in DELETE SENTENCE!";
						break;
					}
					case 4: {
						message_="Invalid operator between 2 conditions!";
						break;
					}
					case 5: {
						message_="Invalid expression in conditions!";
						break;
					}
					case 6: {
						message_="Failed to translate expression!";
						break;
					}
					default: {
						message_+="Unknown error of DELETE SENTENCE!";
						break;
					}
				}
			}
			break;
		}
		case 82: {
			std::string table_name=curr_nodes[1].word_;
			bool success=db_->delete_table(table_name);
			if (!success) {
				message_="Delete failed!";
				success_=false;
				break;
			}
			break;
		}
		case 83: {
			order_=1;
			order_id_=curr_nodes[1].word_;
			break;
		}
		case 84: {
			order_=2;
			order_id_=curr_nodes[1].word_;
			break;
		}
		case 57: case 66: case 67: case 68: case 69: {
			throw std::logic_error("Current operand is not supported now.");
			break;
		}
		case  0: case 20: case 21: case 22: case 24: case 26: case 28: case 31: 
		case 35: case 36: case 37: case 38: case 49: case 50: case 51: case 53:
		case 55: case 56: case 77: case 85: default: {
			break;
		}
		
	}
switch_end:
	stack_.push_back(result);
	//cout<<"reduction "<<state<<" to "<<next<<" with sentence "<<sentence_id<<":"<<reduction_num<<" out at "<<id<<endl;
}

void std::meta::core::database::set_listener(database_listener* listener) {
	parse_.listeners_.clear();
	if (!listener) parse_.listeners_.push_back(db_listener_);
	else parse_.listeners_.push_back(listener); 
}

int std::meta::core::database::find_table(std::string name) {
	for (int i=0;i<tables_.size();i++) {
		if (tables_[i]->meta_.struct_def_.name_==name) return i;
	}
	return -1;
}

template <typename _Tp>
void std::meta::core::database::register_dbtype(const std::string& name,void (*fromfunc)(std::string,void*)) {
	register_dbtype<_Tp>(name,nullptr,fromfunc,nullptr);
}

template <typename _Tp>
void std::meta::core::database::register_dbtype(const std::string& name,bool (*condfunc)(condition)) {
	register_dbtype<_Tp>(name,nullptr,nullptr,condfunc);
}

template <typename _Tp>
void std::meta::core::database::register_dbtype(const std::string& name,std::string (*tofunc)(void*),bool (*condfunc)(condition)) {
	register_dbtype<_Tp>(name,tofunc,nullptr,condfunc);
}

template <typename _Tp>
void std::meta::core::database::register_dbtype(const std::string& name,void (*fromfunc)(std::string,void*),bool (*condfunc)(condition)) {
	register_dbtype<_Tp>(name,nullptr,fromfunc,condfunc);
}

template <typename _Tp>
void std::meta::core::database::register_dbtype(const std::string& name,std::string (*tofunc)(void*),void (*fromfunc)(std::string,void*),bool (*condfunc)(condition)) {
	//1:type->string//2:string->type//3:condition test
	reg_.register_type<_Tp>(name);
	using to_str=std::string (*)(void*);
	using from_str=void (*)(std::string,void*);
	using cond=bool (*)(condition);
	to_str to_func=tofunc?tofunc:[]{
		if constexpr (std::is_same_v<_Tp,intptr_t>) return calc_int_string;
		else if constexpr (std::is_same_v<_Tp,double>) return calc_real_string;
		else if constexpr (std::is_same_v<_Tp,bool>) return calc_boolean_string;
		else if constexpr (std::is_same_v<_Tp,std::string>) return calc_string_string_to;
		else if constexpr (std::is_same_v<_Tp,std::vector<char*>>) return calc_array_string;
		else return calc_null_string;
	}();
	from_str from_func=fromfunc?fromfunc:[]{
		if constexpr (std::is_same_v<_Tp,intptr_t>) return calc_string_int;
		else if constexpr (std::is_same_v<_Tp,double>) return calc_string_real;
		else if constexpr (std::is_same_v<_Tp,bool>) return calc_string_boolean;
		else if constexpr (std::is_same_v<_Tp,std::string>) return calc_string_string_from;
		else if constexpr (std::is_same_v<_Tp,std::vector<char*>>) return calc_string_array;
		else return calc_string_null;
	}();
	cond cond_func=condfunc?condfunc:[]{
		if constexpr (std::is_same_v<_Tp,intptr_t>) return condition_int;
		else if constexpr (std::is_same_v<_Tp,double>) return condition_real;
		else if constexpr (std::is_same_v<_Tp,bool>) return condition_boolean;
		else if constexpr (std::is_same_v<_Tp,std::string>) return condition_string;
		else if constexpr (std::is_same_v<_Tp,std::vector<char*>>) return condition_array;
		else return condition_null;
	}();
	std::size_t* funcs=new size_t[3];
	funcs[0]=reinterpret_cast<std::size_t>(to_func);
	funcs[1]=reinterpret_cast<std::size_t>(from_func);
	funcs[2]=reinterpret_cast<std::size_t>(cond_func);
	reg_.get_type_info(name).extra_info_=reinterpret_cast<std::size_t>(funcs);
}

std::string(*std::meta::core::database::to_string_func(const std::string& name))(void*) {
	if (!reg_.has_type(name)) return calc_null_string;
	auto info=reg_.get_type_info(name);
	if (info.extra_info_==0) return calc_null_string;
	std::size_t* funcs=reinterpret_cast<std::size_t*>(info.extra_info_);
	return reinterpret_cast<std::string (*)(void*)>(funcs[0]);
}

void(*std::meta::core::database::from_string_func(const std::string& name))(std::string,void*) {
	if (!reg_.has_type(name)) return &calc_string_null;
	auto info=reg_.get_type_info(name);
	if (info.extra_info_==0) return &calc_string_null;
	std::size_t* funcs=reinterpret_cast<std::size_t*>(info.extra_info_);
	return reinterpret_cast<void (*)(std::string,void*)>(funcs[1]);
}

bool(*std::meta::core::database::condition_func(const std::string& name))(condition) {
	if (!reg_.has_type(name)) return &condition_null;
	auto info=reg_.get_type_info(name);
	if (info.extra_info_==0) return &condition_null;
	std::size_t* funcs=reinterpret_cast<std::size_t*>(info.extra_info_);
	return reinterpret_cast<bool (*)(condition)>(funcs[2]);
}