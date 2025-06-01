//Last Modified At 2025/04/17
//@Version 1.06
#ifndef _STD4573_META_DATABSE_H_
#define _STD4573_META_DATABSE_H_ 1

//multi-id order
//<COUNT>

#if __cplusplus < 201703L
#error "database.h must run at C++17 or later!"
#endif

#include <cerrno>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <iomanip>
#include <map>
#include <set>
#include <sstream>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>
#include "../bitmask/flags.h"//At Least 1.1
#include "../math/set.h"//At Least 1.0
#include "../syntax/lexer.h"//At Least 1.42
#include "../syntax/parser.h"//At Least 1.21
#include "dynamic_struct.h"//At Least 1.23

namespace std {
	
namespace meta {
	
namespace core {
	
class database {
public:	
	struct table {
		//std::string name_;->meta_.struct_def_.name_
		metadata meta_;
		std::vector<core::dynamic_struct> datas_;
		//bool strict_;
		std::vector<std::string> pks_;
		table(std::string name,metadata meta) : meta_(meta) {
			meta_.struct_def_.name_=name;
		}
		table(std::string name,metadata meta,std::vector<std::string> pks) : meta_(meta) , pks_(pks) {
			meta_.struct_def_.name_=name;
			//(NOP)examine pks exist
		}
	};

public:
	enum token {
		TK_id,
		TK_arrayliteral,
		TK_boolliteral,
		TK_floatliteral,
		TK_numberliteral,
		TK_stringliteral,
		TK_any,
		
		TK_equal,//and others
		TK_not_equal,
		TK_less_than,
		TK_less_equal,
		TK_great_than,
		TK_great_equal,
		TK_above,
		TK_above_equal,
		TK_below,
		TK_below_equal,
		
		TK_add,
		TK_sub,
		TK_div,
		TK_mod,
		TK_and,
		TK_or,
		TK_xor,
		TK_neg,
		TK_not,
		
		TK_comma,
		TK_dot,
		
		TK_lparen,
		TK_rparen,
		TK_lbracket,
		TK_rbracket,
		TK_lsquare,
		TK_rsquare,
		
		TK_AND=100,
		TK_ASCEND,
		TK_CLEAR,
		TK_CONTAINS,
		TK_COUNT,
		TK_CREATE,
		TK_DELETE,
		TK_DESCEND,
		TK_FORMAT,
		TK_FROM,
		TK_HELP,
		TK_ID,
		TK_INSERT,
		TK_INTO,
		TK_LOAD,
		TK_NOT,
		TK_NULL,
		TK_OR,
		TK_PRIMARY,
		TK_SAVE,
		TK_SELECT,
		TK_SHOW,
		TK_SHOWTABLES,
		TK_UPDATE,
		TK_VALUE,
		TK_WHERE,
		
		TK_path,
		TK_filename,
		TK_seperator,
		
		TK_useless,
		TK_error,
		
		TK_OP_S=400,
		TK_OP_Epsilon,
		TK_OP_Seperator,
		TK_OP_Eof,
		
		TK_OP_stmt=1000,
		TK_OP_clear_stmt,
		TK_OP_create_stmt,
		TK_OP_insert_stmt,
		TK_OP_update_stmt,
		TK_OP_select_stmt,
		TK_OP_delete_stmt,
		TK_OP_show_stmt,
		TK_OP_stb_stmt,
		TK_OP_load_stmt,
		TK_OP_save_stmt,
		TK_OP_help_stmt,
		
		TK_OP_formats=2000,
		TK_OP_format,
		
		TK_OP_sel_stmt,
		TK_OP_values,
		TK_OP_value,
		TK_OP_expression,
		TK_OP_e05,
		TK_OP_e04,
		TK_OP_e03,
		TK_OP_e02,
		TK_OP_e01,
		TK_OP_e00,
		TK_OP_literal,
		
		TK_OP_single_update,
		TK_OP_where,
		TK_OP_order,
		TK_OP_conditions,
		TK_OP_condition,
		TK_OP_c02,
		TK_OP_c01,
		TK_OP_c00,
		
		TK_OP_operator,
		
		TK_OP_ids,
		TK_OP_intos,
		
		TK_useless1=10000,
		TK_useless2,
		TK_useless3,
		TK_useless4,
		
		TK_NUM=20000,
	};
	/*bool is_literal(token t) {
		return t==TK_numberliteral || t==TK_stringliteral || t==TK_arrayliteral || t==TK_floatliteral || t==TK_boolliteral || t==TK_filename;
	}*/
protected:
	enum condition_type {
		CT_EQUAL=1,
		CT_LESS=2,
		CT_GREAT=4,
		CT_CONTAINS=8,
		CT_ABOVE=16,
		CT_BELOW=32,
		CT_NOT=128,
		CT_AND=256,
		CT_OR=512,
	};
	enum expression_type {
		ET_ADD,
		ET_SUB,
		ET_MUL,
		ET_DIV,
		ET_MOD,
		ET_AND,
		ET_OR,
		ET_XOR,
		ET_NEG,
		ET_NOT,
	};
	enum operand_type {
		OT_add,
		OT_sub,
		OT_mul,
		OT_div,
		OT_mod,
		OT_and,
		OT_or,
		OT_xor,
		OT_neg,
		OT_not,
		OT_equal,
		OT_not_equal,
		OT_less,
		OT_greater,
		OT_less_equal,
		OT_greater_equal,
		OT_contains,
		OT_NUM,
	};
	struct expression {
		bool is_expressions_;
		struct {
			struct {
				bool is_val_;
				std::string type_;
				std::string val_;
				std::string table_;
				std::string segment_;
			} an_exp_;
			struct {
				expression* exp1_;
				expression_type op_;
				expression* exp2_;
			} exps_;
		} exp_;
		expression();
		expression(const expression& other);
		~expression();
		void calculate(database* db,type_registry reg);
	};
	struct condition {
		bool is_conditions_;
		void* val1_;
		bitmask::flags<condition_type> op_;
		void* val2_;
		/*void destruct(cond_val v) {
			switch (v.type_) {
				case TK_stringliteral:
				case TK_id: {
					delete (string*)(v.val_);
					break;
				}
				case TK_numberliteral: {
					delete (intptr_t*)(v.val_);
					break;
				}
				case TK_floatliteral: {
					delete (double*)(v.val_);
					break;
				}
				default: {
					break;
				}
			}
		}*/
		condition() : val1_(nullptr) , val2_(nullptr) { }
		condition(const condition& other) : is_conditions_(other.is_conditions_) {
			if (is_conditions_) {
				if (other.val1_) val1_=(void*)(new condition(*(condition*)other.val1_));
				if (other.val2_) val2_=(void*)(new condition(*(condition*)other.val2_));
			} else {
				if (other.val1_) val1_=(void*)(new expression(*(expression*)other.val1_));
				if (other.val2_) val2_=(void*)(new expression(*(expression*)other.val2_));
			}
			op_=other.op_;
		}
		~condition() {
			if (is_conditions_) {
				delete (condition*)(val1_);
				delete (condition*)(val2_);
			} else {
				delete (expression*)(val1_);
				delete (expression*)(val2_);
			}
		}
	};
	
private:
	#define CREATE_BINARY_OP_TRAIT(name,op) \
		template <typename _Tp,typename _Up,typename=void> \
		struct has_##name : std::false_type {}; \
		template <typename _Tp,typename _Up> \
		struct has_##name<_Tp,_Up,std::void_t<decltype(std::declval<_Tp>() op std::declval<_Up>())>> : std::true_type {};
	CREATE_BINARY_OP_TRAIT(addition,+)
	CREATE_BINARY_OP_TRAIT(subtract,-)
	CREATE_BINARY_OP_TRAIT(multiply,*)
	CREATE_BINARY_OP_TRAIT(divide  ,/)
	CREATE_BINARY_OP_TRAIT(modulus ,%)
	CREATE_BINARY_OP_TRAIT(bit_and ,&)
	CREATE_BINARY_OP_TRAIT(bit_or  ,|)
	CREATE_BINARY_OP_TRAIT(bit_xor ,^)
	
	CREATE_BINARY_OP_TRAIT(logical_equal,==);
	CREATE_BINARY_OP_TRAIT(logical_not_equal,!=);
	CREATE_BINARY_OP_TRAIT(logical_less,<);
	CREATE_BINARY_OP_TRAIT(logical_greater,>);
	//<= >= contains NO NEDD TRAIT
	#undef CREATE_BINARY_OP_TRAIT
	#define CREATE_UNARY_OP_TRAIT(name,op) \
		template <typename _Tp,typename=void> \
		struct has_unary_##name : std::false_type {}; \
		template <typename _Tp> \
		struct has_unary_##name<_Tp,std::void_t<decltype(op std::declval<_Tp>())>> : std::true_type {};
	CREATE_UNARY_OP_TRAIT(negate ,!);
	CREATE_UNARY_OP_TRAIT(bit_not,~);
	#undef CREATE_UNARY_OP_TRAIT
	template <template<typename,typename> class _Trait,typename _Op,typename _Tp,typename _Up>
	auto generic_binary_operator(_Tp&& val1,_Up&& val2,_Op op) {
		if constexpr (_Trait<_Tp,_Up>::value) return op(std::forward<_Tp>(val1),std::forward<_Up>(val2));
		else {
			std::stringstream ss;
			ss<<std::forward<_Tp>(val1)<<std::forward<_Up>(val2);
			return ss.str();
		}
	}
	template <template<typename> class _Trait,typename _Op,typename _Tp>
	auto generic_unary_operator(_Tp&& val,_Op op) {
		if constexpr (_Trait<_Tp>::value) return op(std::forward<_Tp>(val));
		else {
			std::stringstream ss;
			ss<<std::forward<_Tp>(val);
			return ss.str();
		}
	}
	template <template<typename,typename> class _Trait,typename _Op, typename _Fallback, typename _Tp, typename _Up>
	bool generic_compare(_Tp&& val1,_Up&& val2,_Op op,_Fallback fallback) {
		if constexpr (_Trait<_Tp,_Up>::value) return op(std::forward<_Tp>(val1),std::forward<_Up>(val2));
	    else return fallback(std::forward<_Tp>(val1),std::forward<_Up>(val2));
	}
	static constexpr auto equal_fallback=[](auto&& val1,auto&& val2) {
		std::stringstream ss1, ss2;
		ss1<<val1;
		ss2<<val2;
		return ss1.str()==ss2.str();
	};
	static constexpr auto not_equal_fallback=[](auto&& val1,auto&& val2) {
		std::stringstream ss1, ss2;
		ss1<<val1;
		ss2<<val2;
		return ss1.str()!=ss2.str();
	};
	static constexpr auto false_fallback=[](auto&&,auto&&) { return false; };
	static constexpr auto database_add_impl=[](auto&& val1,auto&& val2) { return val1+val2; };
	static constexpr auto database_sub_impl=[](auto&& val1,auto&& val2) { return val1-val2; };
	static constexpr auto database_mul_impl=[](auto&& val1,auto&& val2) { return val1*val2; };
	static constexpr auto database_div_impl=[](auto&& val1,auto&& val2) { return val1/val2; };
	static constexpr auto database_mod_impl=[](auto&& val1,auto&& val2) { return val1%val2; };
	static constexpr auto database_and_impl=[](auto&& val1,auto&& val2) { return val1&val2; };
	static constexpr auto database_or__impl=[](auto&& val1,auto&& val2) { return val1|val2; };
	static constexpr auto database_xor_impl=[](auto&& val1,auto&& val2) { return val1^val2; };
	static constexpr auto database_neg_impl=[](auto&& val) { return !val; };
	static constexpr auto database_not_impl=[](auto&& val) { return ~val; };
	static constexpr auto equal_op_impl=[](auto&& val1,auto&& val2) { return val1==val2; };
	static constexpr auto not_equal_op_impl=[](auto&& val1,auto&& val2) { return val1!=val2; };
	static constexpr auto less_op_impl=[](auto&& val1,auto&& val2) { return val1<val2; };
	static constexpr auto greater_op_impl=[](auto&& val1,auto&& val2) { return val1>val2; };
	template <typename _Tp,typename _Up>
	auto database_add(_Tp&& val1,_Up&& val2) {
		return generic_binary_operator<has_addition,decltype(database_add_impl)>(std::forward<_Tp>(val1),std::forward<_Up>(val2),database_add_impl);
	}
	template <typename _Tp,typename _Up>
	auto database_sub(_Tp&& val1,_Up&& val2) {
		return generic_binary_operator<has_subtract,decltype(database_sub_impl)>(std::forward<_Tp>(val1),std::forward<_Up>(val2),database_sub_impl);
	}
	template <typename _Tp,typename _Up>
	auto database_mul(_Tp&& val1,_Up&& val2) {
		return generic_binary_operator<has_multiply,decltype(database_mul_impl)>(std::forward<_Tp>(val1),std::forward<_Up>(val2),database_mul_impl);
	}
	template <typename _Tp,typename _Up>
	auto database_div(_Tp&& val1,_Up&& val2) {
		return generic_binary_operator<has_divide  ,decltype(database_div_impl)>(std::forward<_Tp>(val1),std::forward<_Up>(val2),database_div_impl);
	}
	template <typename _Tp,typename _Up>
	auto database_mod(_Tp&& val1,_Up&& val2) {
		return generic_binary_operator<has_modulus ,decltype(database_mod_impl)>(std::forward<_Tp>(val1),std::forward<_Up>(val2),database_mod_impl);
	}
	template <typename _Tp,typename _Up>
	auto database_and(_Tp&& val1,_Up&& val2) {
		return generic_binary_operator<has_bit_and ,decltype(database_and_impl)>(std::forward<_Tp>(val1),std::forward<_Up>(val2),database_and_impl);
	}
	template <typename _Tp,typename _Up>
	auto database_or(_Tp&& val1,_Up&& val2) {
		return generic_binary_operator<has_bit_or  ,decltype(database_or__impl)>(std::forward<_Tp>(val1),std::forward<_Up>(val2),database_or__impl);
	}
	template <typename _Tp,typename _Up>
	auto database_xor(_Tp&& val1,_Up&& val2) {
		return generic_binary_operator<has_bit_xor ,decltype(database_xor_impl)>(std::forward<_Tp>(val1),std::forward<_Up>(val2),database_xor_impl);
	}
	template <typename _Tp>
	auto database_neg(_Tp&& val) {
		return generic_unary_operator<has_unary_negate ,decltype(database_neg_impl)>(std::forward<_Tp>(val),database_neg_impl);
	}
	template <typename _Tp>
	auto database_not(_Tp&& val) {
		return generic_unary_operator<has_unary_bit_not,decltype(database_not_impl)>(std::forward<_Tp>(val),database_not_impl);
	}
	
	template <typename _Tp,typename _Up>
	bool database_equal(_Tp&& val1,_Up&& val2) {
		return generic_compare<has_logical_equal>(std::forward<_Tp>(val1),std::forward<_Up>(val2),equal_op_impl,equal_fallback);
	}
	template <typename _Tp,typename _Up>
	bool database_not_equal(_Tp&& val1,_Up&& val2) {
		return generic_compare<has_logical_not_equal>(std::forward<_Tp>(val1),std::forward<_Up>(val2),not_equal_op_impl,not_equal_fallback);
	}
	template <typename _Tp,typename _Up>
	bool database_less(_Tp&& val1,_Up&& val2) {
		return generic_compare<has_logical_less>(std::forward<_Tp>(val1),std::forward<_Up>(val2),less_op_impl,false_fallback);
	}
	template <typename _Tp,typename _Up>
	bool database_greater(_Tp&& val1,_Up&& val2) {
		return generic_compare<has_logical_greater>(std::forward<_Tp>(val1),std::forward<_Up>(val2),greater_op_impl,false_fallback);
	}
	template <typename _Tp,typename _Up>
	bool database_less_equal(_Tp&& val1,_Up&& val2) {
		return database_equal<_Tp,_Up>(val1,val2)|database_less<_Tp,_Up>(val1,val2);
	}
	template <typename _Tp,typename _Up>
	bool database_greater_equal(_Tp&& val1,_Up&& val2) {
		return database_equal<_Tp,_Up>(val1,val2)|database_greater<_Tp,_Up>(val1,val2);
	}
	template <typename _Tp,typename _Up>
	bool database_contains(_Tp&& val1,_Up&& val2) {
		std::stringstream ss1,ss2;
		ss1<<std::forward<_Tp>(val1);
		ss2<<std::forward<_Up>(val2);
		return ss1.str().find(ss2.str())!=std::string::npos;
	}
	

#define DEF_CALC_GET(type,id) \
type val##id=*reinterpret_cast<type*>(v##id);

#define DEF_CALC_CALC_IMPL(type1,type2,typeres,op) \
std::string calculate_##type1##_##type2##_##op(void* v1,void* v2,std::string& result_type) { \
	using real=double; \
	using string=std::string; \
	using number=intptr_t; \
	DEF_CALC_GET(type1,1) \
	DEF_CALC_GET(type2,2) \
	result_type=#typeres; \
	auto func=to_string_func(result_type); \
	auto result=database_##op(val1,val2); \
	return func((void*)&result); \
}

#define DEF_CALC_LOGICAL_IMPL(type1,type2,op) \
std::string calculate_##type1##_##type2##_##op(void* v1,void* v2,std::string& result_type) { \
	using real=double; \
	using string=std::string; \
	using number=intptr_t; \
	DEF_CALC_GET(type1,1) \
	DEF_CALC_GET(type2,2) \
	bool result=database_##op(val1,val2); \
	return result?"1":"0"; \
}

#define DEF_CALC_CALC(type1,type2,typeres,op) \
DEF_CALC_CALC_IMPL(type1,type2,typeres,op) \
DEF_CALC_CALC_IMPL(type2,type1,typeres,op)

#define DEF_CALC_LOGICAL(type1,type2,op) \
DEF_CALC_LOGICAL_IMPL(type1,type2,op) \
DEF_CALC_LOGICAL_IMPL(type2,type1,op)

#include "database.inc"

#undef DEF_CALC_GET
#undef DEF_CALC_CALC_IMPL
#undef DEF_CALC_LOGICAL_IMPL
#undef DEF_CALC_CALC
#undef DEF_CALC_LOGICAL
	
public:
	type_registry reg_;
	std::vector<table*> tables_;
	std::map<std::pair<std::string,std::string>,std::vector<std::string (std::meta::core::database::*)(void*,void*,std::string&)>> operator_map_;
	
	std::string(database::*operator_func(std::string type1,std::string type2,operand_type op))(void*,void*,std::string&);
	std::string null_operator(void* val1,void* val2,std::string& type);
	
public:
	class database_listener:public std::syntax::parser_listener<token> {
	protected:
		using condition_type=database::condition_type;
		using expression_type=database::expression_type;
		using operand_type=database::operand_type;
		using expression=database::expression;
		using condition=database::condition;
		
	public:
		enum stmt_type {
			DBL_STMT_UNKNOWN	=0,
			DBL_STMT_CREATE		=1,
			DBL_STMT_INSERT		=2,
			DBL_STMT_UPDATE		=3,
			DBL_STMT_SELECT		=4,
			DBL_STMT_DELETE		=5,
			DBL_STMT_SHOW		=6,
			DBL_STMT_STB		=7,
			DBL_STMT_LOAD		=8,
			DBL_STMT_SAVE		=9,
			DBL_STMT_HELP		=10,
			DBL_STMT_CLEAR		=79,
		};
		struct format {
			std::string name_;
			std::string type_;
		};
		database* db_;
		stmt_type type_=DBL_STMT_UNKNOWN;
		std::vector<std::syntax::lexer<token>::resolution> resolutions_;
		std::vector<std::syntax::lexer<token>::resolution> stack_;
		size_t curr_;
		
		std::vector<format> formats_;
		format curr_format_;
		
		std::vector<std::string> ids_;
		std::string curr_id_;
		
		std::vector<expression*> values_;
		expression* curr_value1_;
		expression* curr_value2_;
		int curr_value_status_;
		
		condition* curr_condition1_;
		condition* curr_condition2_;
		int curr_condition_status_;
		
		bitmask::flags<condition_type> current_op_;
		
		std::string	into_id_;
		
		std::string message_;
		bool success_;
		
		table* temp_result_=nullptr;//GET RESULT/MESSAGE(in database public function)
		
		int order_;
		std::string order_id_;
	public:
		database_listener(database* db) : db_(db) , temp_result_(nullptr) {
			enabled_=true;
		}
		~database_listener() {
			if (!temp_result_) delete temp_result_;
			temp_result_=nullptr;
		}
		virtual void init(std::vector<std::syntax::lexer<token>::resolution> resolutions);
		virtual void on_shift(int id,int state,token word) override;
		virtual void on_reduction(int id,int state,int next,int sentence_id,int reduction_num) override;
		virtual void on_accept() override {
			//success_=true;
			//cout<<"accept"<<endl;
		}
		virtual void on_error(typename std::syntax::parser_listener<token>::error_type type,int state,token word) override {
			//cout<<"error:"<<type<<endl;
		}
	};

public:
	std::syntax::lexer<token> lex_;
	std::syntax::parser<token> parse_;
private:
	int float_length_=6;//SAVE/SET SENTENCE
	database_listener* db_listener_;
	
public:
	void set_listener(database_listener* listener=nullptr);
	database_listener* get_listener() { return dynamic_cast<database_listener*>(parse_.listeners_[0]); };
	//database_listener* get_listener(); WARNING

public:
	int find_table(std::string name);
private:
	void generate_lexer() {
		using lu=std::syntax::lexer_unit<token>;
		std::vector<lu> units;
		units.push_back(lu({"\\[a\\-zA\\-Z_\\]\\[a\\-zA\\-Z0\\-9_\\]\\*",TK_id}));
		units.push_back(lu({"\"\\(\\[ \\-~\\c128\\c\\-\\c255\\c\\t\\^\"\\^\\\\\\]\\*\\(\\\\\\[ \\-~\\c128\\c\\-\\c255\\c\\t\\]\\)\\*\\)\\*\"",TK_stringliteral}));
		units.push_back(lu({"{\"\\(\\[ \\-~\\c128\\c\\-\\c255\\c\\t\\^\"\\^\\\\\\]\\*\\(\\\\\\[ \\-~\\c128\\c\\-\\c255\\c\\t\\]\\)\\*\\)\\*\"\\(\"\\(\\[ \\-~\\c128\\c\\-\\c255\\c\\t\\^\"\\^\\\\\\]\\*\\(\\\\\\[ \\-~\\c128\\c\\-\\c255\\c\\t\\]\\)\\*\\)\\*\",\\)\\*}",TK_arrayliteral}));
		units.push_back(lu({"{}",TK_arrayliteral}));
		units.push_back(lu({"\\(\\(\\[1\\-9\\]\\(\'\\?\\[0\\-9\\]\\)\\*\\)\\|\\(0\\(\'\\?\\[0\\-7\\]\\)\\*\\)\\|\\(0\\[xX\\]\\[0\\-9a\\-fA\\-F\\]\\(\'\\?\\[0\\-9a\\-fA\\-F\\]\\)\\*\\)\\|\\(0\\[bB\\]\\[01\\]\\(\'\\?\\[01\\]\\)\\*\\)\\)",TK_numberliteral}));
		units.push_back(lu({"\\(\\[0\\-9\\]\\(\'\\?\\[0\\-9\\]\\)\\*\\)\\?.\\[0\\-9\\]\\(\'\\?\\[0\\-9\\]\\)\\*\\(\\[eE\\]\\[+-\\]\\?\\(\\[0\\-9\\]\\(\'\\?\\[0\\-9\\]\\)\\*\\)\\?\\)\\?\\[flFL\\]\\?",TK_floatliteral}));
		units.push_back(lu({"\\[0\\-9\\]\\(\'\\?\\[0\\-9\\]\\)\\*.\\(\\[eE\\]\\[+-\\]\\?\\(\\[0\\-9\\]\\(\'\\?\\[0\\-9\\]\\)\\*\\)\\?\\)\\?\\[flFL\\]\\?",TK_floatliteral}));
		units.push_back(lu({"\\[0\\-9\\]\\(\'\\?\\[0\\-9\\]\\)\\*\\[eE\\]\\[+-\\]\\?\\(\\[0\\-9\\]\\(\'\\?\\[0\\-9\\]\\)\\*\\)\\?\\[flFL\\]\\?",TK_floatliteral}));
		units.push_back(lu({"true",TK_boolliteral}));
		units.push_back(lu({"false",TK_boolliteral}));
		units.push_back(lu({"*",TK_any}));
		
		units.push_back(lu({"=",TK_equal}));
		units.push_back(lu({"<=",TK_less_equal}));
		units.push_back(lu({">=",TK_great_equal}));
		units.push_back(lu({"!=",TK_not_equal}));
		units.push_back(lu({"<",TK_less_than}));
		units.push_back(lu({">",TK_great_than}));
		
		units.push_back(lu({"+",TK_add}));
		units.push_back(lu({"-",TK_sub}));
		units.push_back(lu({"/",TK_div}));
		units.push_back(lu({"%",TK_mod}));
		units.push_back(lu({"&",TK_and}));
		units.push_back(lu({"|",TK_or}));
		units.push_back(lu({"^",TK_xor}));
		units.push_back(lu({"~",TK_neg}));
		units.push_back(lu({"!",TK_not}));
		
		units.push_back(lu({",",TK_comma}));
		units.push_back(lu({".",TK_dot}));
		
		units.push_back(lu({"(",TK_lparen}));
		units.push_back(lu({")",TK_rparen}));
		units.push_back(lu({"[",TK_lbracket}));
		units.push_back(lu({"]",TK_rbracket}));
		units.push_back(lu({"<<",TK_lsquare}));
		units.push_back(lu({">>",TK_rsquare}));
		
		units.push_back(lu({"AND",TK_AND}));
		units.push_back(lu({"ASCEND",TK_ASCEND}));
		units.push_back(lu({"CLEAR",TK_CLEAR}));
		units.push_back(lu({"CONTAINS",TK_CONTAINS}));
		units.push_back(lu({"CREATE",TK_CREATE}));
		units.push_back(lu({"DELETE",TK_DELETE}));
		units.push_back(lu({"DESCEND",TK_DESCEND}));
		units.push_back(lu({"FORMAT",TK_FORMAT}));
		units.push_back(lu({"FROM",TK_FROM}));
		units.push_back(lu({"HELP",TK_HELP}));
		units.push_back(lu({"<ID>",TK_ID}));
		units.push_back(lu({"INSERT",TK_INSERT}));
		units.push_back(lu({"INTO",TK_INTO}));
		units.push_back(lu({"LOAD",TK_LOAD}));
		units.push_back(lu({"NOT",TK_NOT}));
		units.push_back(lu({"NULL",TK_NULL}));
		units.push_back(lu({"OR",TK_OR}));
		units.push_back(lu({"PRIMARY",TK_PRIMARY}));
		units.push_back(lu({"SAVE",TK_SAVE}));
		units.push_back(lu({"SELECT",TK_SELECT}));
		units.push_back(lu({"SHOW",TK_SHOW}));
		units.push_back(lu({"SHOWTABLES",TK_SHOWTABLES}));
		units.push_back(lu({"UPDATE",TK_UPDATE}));
		units.push_back(lu({"VALUE",TK_VALUE}));
		units.push_back(lu({"WHERE",TK_WHERE}));
		
		//units.push_back(lu({".\\(/\\[ \\-~\\^/\\^\\\\\\^:\\^*\\^?\\^\"\\^<\\^>\\^|\\]\\+\\)\\*/",TK_path}));
		//units.push_back(lu({"..\\(/\\[ \\-~\\^/\\^\\\\\\^:\\^*\\^?\\^\"\\^<\\^>\\^|\\]\\+\\)\\*/",TK_path}));
		//units.push_back(lu({"\\[A\\-Z\\]:\\(/\\[ \\-~\\^/\\^\\\\\\^:\\^*\\^?\\^\"\\^<\\^>\\^|\\]\\+\\)\\*/",TK_path}));
		//units.push_back(lu({"\\[ \\-~\\^ \\^/\\^\\\\\\^:\\^*\\^?\\^\"\\^<\\^>\\^|\\]\\[ \\-~\\^/\\^\\\\\\^:\\^*\\^?\\^\"\\^<\\^>\\^|\\]\\*/\\(\\[ \\-~\\^/\\^\\\\\\^:\\^*\\^?\\^\"\\^<\\^>\\^|\\]\\+/\\)\\*",TK_path}));
		//units.push_back(lu({"\\[ \\-~\\^ \\^/\\^\\\\\\^:\\^*\\^?\\^\"\\^<\\^>\\^|\\]\\+\\(.\\[ \\-~\\^ \\^/\\^\\\\\\^:\\^*\\^?\\^\"\\^<\\^>\\^|\\]\\+\\)\\?",TK_filename}));
		units.push_back(lu({":",TK_seperator}));
		
		units.push_back(lu({"\\[ \\n\\r\\t\\]\\+",TK_useless}));
		new(&lex_) std::syntax::lexer<token>(TK_useless,TK_error,units);
		lex_.generate_lexer();
	}
	void generate_parser() {
		using pu=std::syntax::parser_unit<token>;
		#define psu std::syntax::single_parser_unit<token>
		std::vector<pu> units;
		units.push_back(psu(TK_OP_S,{TK_OP_stmt,TK_OP_Eof}));													//0
		units.push_back(psu(TK_OP_stmt,{TK_OP_create_stmt}));													//1
		units.push_back(psu(TK_OP_stmt,{TK_OP_insert_stmt}));													//2
		units.push_back(psu(TK_OP_stmt,{TK_OP_update_stmt}));													//3
		units.push_back(psu(TK_OP_stmt,{TK_OP_select_stmt}));													//4
		units.push_back(psu(TK_OP_stmt,{TK_OP_delete_stmt}));													//5
		units.push_back(psu(TK_OP_stmt,{TK_OP_show_stmt}));														//6
		units.push_back(psu(TK_OP_stmt,{TK_OP_stb_stmt}));														//7
		units.push_back(psu(TK_OP_stmt,{TK_OP_load_stmt}));														//8
		units.push_back(psu(TK_OP_stmt,{TK_OP_save_stmt}));														//9
		units.push_back(psu(TK_OP_stmt,{TK_OP_help_stmt}));														//10
		units.push_back(psu(TK_OP_create_stmt,{TK_CREATE,TK_id,TK_FORMAT,TK_OP_formats}));						//11
		units.push_back(psu(TK_OP_create_stmt,{TK_CREATE,TK_id,TK_FORMAT,TK_OP_formats,TK_PRIMARY,TK_OP_ids}));	//12
		units.push_back(psu(TK_OP_formats,{TK_OP_format}));														//13
		units.push_back(psu(TK_OP_formats,{TK_OP_formats,TK_comma,TK_OP_format}));								//14
		units.push_back(psu(TK_OP_create_stmt,{TK_CREATE,TK_id,TK_FROM,TK_OP_sel_stmt}));						//15
		units.push_back(psu(TK_OP_format,{TK_id,TK_seperator,TK_id}));											//16
		units.push_back(psu(TK_OP_insert_stmt,{TK_INSERT,TK_id,TK_VALUE,TK_OP_values}));						//17
		units.push_back(psu(TK_OP_values,{TK_OP_value}));														//18
		units.push_back(psu(TK_OP_values,{TK_OP_values,TK_comma,TK_OP_value}));									//19
		units.push_back(psu(TK_OP_value,{TK_OP_expression}));													//20
		units.push_back(psu(TK_OP_expression,{TK_OP_e05}));														//21
		units.push_back(psu(TK_OP_e05,{TK_OP_e04}));															//22
		units.push_back(psu(TK_OP_e05,{TK_OP_e05,TK_or,TK_OP_e04}));											//23
		units.push_back(psu(TK_OP_e04,{TK_OP_e03}));															//24
		units.push_back(psu(TK_OP_e04,{TK_OP_e04,TK_xor,TK_OP_e03}));											//25
		units.push_back(psu(TK_OP_e03,{TK_OP_e02}));															//26
		units.push_back(psu(TK_OP_e03,{TK_OP_e03,TK_and,TK_OP_e02}));											//27
		units.push_back(psu(TK_OP_e02,{TK_OP_e01}));															//28
		units.push_back(psu(TK_OP_e02,{TK_OP_e02,TK_add,TK_OP_e01}));											//29
		units.push_back(psu(TK_OP_e02,{TK_OP_e02,TK_sub,TK_OP_e01}));											//30
		units.push_back(psu(TK_OP_e01,{TK_OP_e00}));															//31
		units.push_back(psu(TK_OP_e01,{TK_OP_e01,TK_any,TK_OP_e00}));											//32
		units.push_back(psu(TK_OP_e01,{TK_OP_e01,TK_div,TK_OP_e00}));											//33
		units.push_back(psu(TK_OP_e01,{TK_OP_e01,TK_mod,TK_OP_e00}));											//34
		units.push_back(psu(TK_OP_e00,{TK_lparen,TK_OP_e05,TK_rparen}));										//35
		units.push_back(psu(TK_OP_e00,{TK_useless1}));														//36
		units.push_back(psu(TK_OP_e00,{TK_useless2}));														//37
		units.push_back(psu(TK_OP_e00,{TK_OP_literal}));														//38
		units.push_back(psu(TK_OP_literal,{TK_arrayliteral}));													//39
		units.push_back(psu(TK_OP_literal,{TK_stringliteral}));													//40
		units.push_back(psu(TK_OP_literal,{TK_boolliteral}));													//41
		units.push_back(psu(TK_OP_literal,{TK_floatliteral}));													//42
		units.push_back(psu(TK_OP_literal,{TK_numberliteral}));													//43
		units.push_back(psu(TK_OP_literal,{TK_lbracket,TK_id,TK_rbracket,TK_dot,TK_id}));						//44
		units.push_back(psu(TK_OP_literal,{TK_id}));															//45
		units.push_back(psu(TK_OP_insert_stmt,{TK_INSERT,TK_id,TK_FROM,TK_OP_sel_stmt}));						//46 SKIPPED->TEMP USE 75
		units.push_back(psu(TK_OP_update_stmt,{TK_UPDATE,TK_id,TK_FROM,TK_OP_ids,TK_VALUE,TK_OP_values,TK_OP_where}));//47
		units.push_back(psu(TK_OP_single_update,{TK_dot}));													//48 CANCELED
		units.push_back(psu(TK_OP_where,{TK_WHERE,TK_OP_conditions}));											//49
		units.push_back(psu(TK_OP_conditions,{TK_OP_c02}));														//50
		units.push_back(psu(TK_OP_c02,{TK_OP_c01}));															//51
		units.push_back(psu(TK_OP_c02,{TK_OP_c02,TK_AND,TK_OP_c01}));											//52
		units.push_back(psu(TK_OP_c01,{TK_OP_c00}));															//53
		units.push_back(psu(TK_OP_c01,{TK_OP_c01,TK_OR,TK_OP_c00}));											//54
		units.push_back(psu(TK_OP_c00,{TK_OP_condition}));														//55
		units.push_back(psu(TK_OP_c00,{TK_useless3}));														//56 CANCELED->(C02)
		units.push_back(psu(TK_OP_c00,{TK_useless4}));														//57 CANCELED
		units.push_back(psu(TK_OP_condition,{TK_OP_expression,TK_OP_operator,TK_OP_expression}));				//58
		units.push_back(psu(TK_OP_operator,{TK_equal}));														//59
		units.push_back(psu(TK_OP_operator,{TK_not_equal}));													//60
		units.push_back(psu(TK_OP_operator,{TK_less_than}));													//61
		units.push_back(psu(TK_OP_operator,{TK_less_equal}));													//62
		units.push_back(psu(TK_OP_operator,{TK_great_than}));													//63
		units.push_back(psu(TK_OP_operator,{TK_great_equal}));													//64
		units.push_back(psu(TK_OP_operator,{TK_CONTAINS}));														//65
		units.push_back(psu(TK_OP_operator,{TK_above}));														//66
		units.push_back(psu(TK_OP_operator,{TK_above_equal}));													//67
		units.push_back(psu(TK_OP_operator,{TK_below}));														//68
		units.push_back(psu(TK_OP_operator,{TK_below_equal}));													//69
		units.push_back(psu(TK_OP_ids,{TK_id}));																//70
		units.push_back(psu(TK_OP_ids,{TK_OP_ids,TK_comma,TK_id}));												//71
		units.push_back(psu(TK_OP_ids,{TK_any}));																//72
		units.push_back(psu(TK_OP_sel_stmt,{TK_SELECT,TK_OP_ids,TK_FROM,TK_id,TK_OP_where,TK_OP_order}));		//73
		units.push_back(psu(TK_OP_select_stmt,{TK_OP_sel_stmt}));												//74
		units.push_back(psu(TK_OP_select_stmt,{TK_OP_sel_stmt,TK_OP_intos}));									//75
		units.push_back(psu(TK_OP_intos,{TK_INTO,TK_id}));														//76
		units.push_back(psu(TK_OP_where,{TK_OP_Epsilon}));														//77
		units.push_back(psu(TK_OP_clear_stmt,{TK_CLEAR,TK_id}));												//78
		units.push_back(psu(TK_OP_stmt,{TK_OP_clear_stmt}));													//79
		units.push_back(psu(TK_OP_literal,{TK_ID}));															//80
		units.push_back(psu(TK_OP_delete_stmt,{TK_DELETE,TK_FROM,TK_id,TK_OP_where}));							//81
		units.push_back(psu(TK_OP_delete_stmt,{TK_DELETE,TK_id}));												//82
		units.push_back(psu(TK_OP_order,{TK_ASCEND,TK_id}));													//83
		units.push_back(psu(TK_OP_order,{TK_DESCEND,TK_id}));													//84
		units.push_back(psu(TK_OP_order,{TK_OP_Epsilon}));														//85
		
		new(&parse_) std::syntax::parser<token>(TK_OP_S,TK_OP_Seperator,TK_OP_Epsilon,TK_OP_Eof,units);
		parse_.start_unit_=0;
		parse_.generate_parser();
		
		if (db_listener_) delete db_listener_;
		db_listener_=new database_listener(this);
		set_listener(nullptr);
		#undef psu
	}
	void generate_operand() {
		//USING LAMBDA TO MAKE SURE WHEN ACCESS MAP,THE VECTOR HAS ENOUGH SIZE
		/*struct {
			template <typename _Key,typename _Value>
			std::vector<_Value>& operator() (std::map<_Key,std::vector<_Value>>& m,const _Key& key,size_t default_size=5) {
				auto [it,inserted]=m.try_emplace(key,default_size);
				return it->second;
			}
		} get_map;*/
#define DEF_CALC_CALC_IMPL(type1,type2,typeres,op) \
if (operator_map_[std::make_pair(#type1,#type2)].size()<=OT_##op) operator_map_[std::make_pair(#type1,#type2)].resize((size_t)OT_NUM); \
operator_map_[std::make_pair(#type1,#type2)][OT_##op]=calculate_##type1##_##type2##_##op;

#define DEF_CALC_LOGICAL_IMPL(type1,type2,op) \
if (operator_map_[std::make_pair(#type1,#type2)].size()<=OT_##op) operator_map_[std::make_pair(#type1,#type2)].resize((size_t)OT_NUM); \
operator_map_[std::make_pair(#type1,#type2)][OT_##op]=calculate_##type1##_##type2##_##op;

#define DEF_CALC_CALC(type1,type2,typeres,op) \
DEF_CALC_CALC_IMPL(type1,type2,typeres,op) \
DEF_CALC_CALC_IMPL(type2,type1,typeres,op)

#define DEF_CALC_LOGICAL(type1,type2,op) \
DEF_CALC_LOGICAL_IMPL(type1,type2,op) \
DEF_CALC_LOGICAL_IMPL(type2,type1,op)

#include "database.inc"

#undef DEF_CALC_CALC_IMPL
#undef DEF_CALC_LOGICAL_IMPL
#undef DEF_CALC_CALC
#undef DEF_CALC_LOGICAL
	}
	
private:
	static std::string calc_int_string(void* ptr) {
		intptr_t i=*reinterpret_cast<intptr_t*>(ptr);
		std::ostringstream os;
		os<<i;
		return os.str();
	}
	static std::string calc_real_string(void* ptr) {
		double f=*reinterpret_cast<double*>(ptr);
		std::ostringstream os;
		os<<std::fixed<<std::setprecision(15/*float_length_*/)<<f;
		std::string str=os.str();
		std::size_t dot_pos=str.find('.');
		if (dot_pos==string::npos) return str+".00";
		std::string int_part=str.substr(0,dot_pos);
		std::string frac_part=str.substr(dot_pos+1);
		std::size_t last_nz=frac_part.find_last_not_of('0');
		if (last_nz!=string::npos) frac_part=frac_part.substr(0,last_nz+1);
		else frac_part="";
		if (frac_part.size()<2) frac_part.resize(2,'0');
		return int_part+"."+frac_part; 
	}
	static std::string calc_string_string_to(void* ptr) {
		std::string s=*reinterpret_cast<std::string*>(ptr);
		return "\""+s+"\"";
	}
	static std::string calc_array_string(void* ptr) {
		std::vector<const char*> v=*reinterpret_cast<std::vector<const char*>*>(ptr);
		std::string result="{";
		for (auto it:v) result+=std::string(it);
		return result+"}";
	}
	static std::string calc_boolean_string(void* ptr) {
		bool b=*reinterpret_cast<bool*>(ptr);
		return b?"true":"false";
	}
	static std::string calc_null_string(void* ptr) {
		return "";
	}
	
	
	static void calc_string_int(std::string s,void* ptr) {
		/*std::istringstream is(s);
		intptr_t i;
		is>>i;
		if (is.fail()) throw std::invalid_argument("Invalid number:"+s+"!");*/
		//reinterpret_cast<intptr_t*>(ptr)=new intptr_t;
		std::string num_str;
		for (char c:s) {
			if (c!='\'') num_str.push_back(c);
		}
		while (!num_str.empty()) {
			char last=num_str.back();
			if (last=='u' || last=='U' || last=='l' || last=='L') s.pop_back();
			else break;
		}
		if (num_str.empty()) std::invalid_argument("Invalid number:"+s+"!");
		char* end_ptr;
		errno=0;
		try {
			*reinterpret_cast<intptr_t*>(ptr)=std::strtoll(num_str.c_str(),&end_ptr,0);
		} catch (const exception& e) {
			throw std::invalid_argument("Invalid number:"+s+"!");
		}
		if (end_ptr!=num_str.c_str()+num_str.size() || errno) throw std::invalid_argument("Invalid number:"+s+"!");
		return;
	}
	static void calc_string_real(std::string s,void* ptr) {
		if (!s.empty()) {
			char last=s.back();
			if (last=='f' || last=='F' || last=='l' || last=='L') s.pop_back();
		}
		try {
			*reinterpret_cast<double*>(ptr)=std::stod(s);
		} catch (const exception& e) {
			throw std::invalid_argument("Invalid real:"+s+"!");
		}
		return;
	}
	static void calc_string_string_from(std::string s,void* ptr) {
		if (s.size()<2) throw std::invalid_argument("String too short:"+s+"!");
		if (s[0]!='\"' || s[s.size()-1]!='\"') throw std::invalid_argument("Invalid string:"+s+"!");
		*reinterpret_cast<std::string*>(ptr)=s.substr(1);
		*reinterpret_cast<std::string*>(ptr)=reinterpret_cast<std::string*>(ptr)->substr(0,reinterpret_cast<std::string*>(ptr)->size()-1);
		return;
	}
	static void calc_string_array(std::string s,void* ptr) {
		//NEED FINISH
		return;
	}
	static void calc_string_boolean(std::string s,void* ptr) {
		int number=-1;
		bool is_number=true;
		char* end_ptr;
		errno=0;
		try {
			number=std::strtoll(s.c_str(),&end_ptr,0);
		} catch (const exception& e) {
			is_number=false;
		}
		if (end_ptr!=s.c_str()+s.size() || errno) is_number=false;
		if (is_number) *reinterpret_cast<bool*>(ptr)=(number!=0);
		else {
			if (s!="true" && s!="false") throw std::invalid_argument("Invalid boolean:"+s+"!");
			*reinterpret_cast<bool*>(ptr)=(s=="true")|(std::strtoll(s.c_str(),nullptr,0)!=0);
		}
		return;
	}
	static void calc_string_null(std::string s,void* ptr) {
		throw std::invalid_argument("Invalid null:"+s+"!");
	}
	
	static bool condition_int(condition c) {
		if (c.is_conditions_) return false;
		bool result=false;
		std::string v1=calc_int_string(c.val1_);
		std::string v2=calc_int_string(c.val2_);
		intptr_t i1=*reinterpret_cast<intptr_t*>(c.val1_);
		intptr_t i2=*reinterpret_cast<intptr_t*>(c.val2_);
		if (c.op_.contains(CT_EQUAL)) {
			if (i1==i2) result=true;
		}
		if (c.op_.contains(CT_LESS)) {
			if (i1<i2) result=true;
		}
		if (c.op_.contains(CT_GREAT)) {
			if (i1>i2) result=true;
		}
		if (c.op_.contains(CT_CONTAINS)) {
			if (v1.find(v2)!=std::string::npos) result=true;
		}
		if (c.op_.contains(CT_ABOVE)) {
			uintptr_t u1=i1;
			uintptr_t u2=i2;
			if (u1>u2) result=true;
		}
		if (c.op_.contains(CT_BELOW)) {
			uintptr_t u1=i1;
			uintptr_t u2=i2;
			if (u1<u2) result=true;
		}
		if (c.op_.contains(CT_NOT)) result=!result;
		return result;
	}
	
	static bool condition_real(condition c) {
		if (c.is_conditions_) return false;
		bool result=false;
		double d1=*reinterpret_cast<double*>(c.val1_);
		double d2=*reinterpret_cast<double*>(c.val2_);
		if (c.op_.contains(CT_EQUAL)) {
			if (d1==d2) result=true;
		}
		if (c.op_.contains(CT_LESS)) {
			if (d1<d2) result=true;
		}
		if (c.op_.contains(CT_GREAT)) {
			if (d1>d2) result=true;
		}
		if (c.op_.contains(CT_ABOVE)) {
			double ud1=std::fabs(d1);
			double ud2=std::fabs(d2);
			if (ud1>ud2) result=true;
		}
		if (c.op_.contains(CT_BELOW)) {
			double ud1=std::fabs(d1);
			double ud2=std::fabs(d2);
			if (ud1<ud2) result=true;
		}
		if (c.op_.contains(CT_NOT)) result=!result;
		return result;
	}
	static bool condition_boolean(condition c) {
		if (c.is_conditions_) return false;
		bool result=false;
		bool b1=*reinterpret_cast<bool*>(c.val1_);
		bool b2=*reinterpret_cast<bool*>(c.val2_);
		if (c.op_.contains(CT_EQUAL)) {
			if (b1==b2) result=true;
		}
		if (c.op_.contains(CT_LESS) || c.op_.contains(CT_BELOW)) {
			if (b1<b2) result=true;
		}
		if (c.op_.contains(CT_GREAT) || c.op_.contains(CT_ABOVE)) {
			if (b1>b2) result=true;
		}
		if (c.op_.contains(CT_NOT)) result=!result;
		return result;
	}
	static bool condition_string(condition c) {
		if (c.is_conditions_) return false;
		bool result=false;
		std::string s1=*reinterpret_cast<std::string*>(c.val1_);
		std::string s2=*reinterpret_cast<std::string*>(c.val2_);
		if (c.op_.contains(CT_EQUAL)) {
			if (s1==s2) result=true;
		}
		if (c.op_.contains(CT_CONTAINS)) {
			if (s1.find(s2)!=std::string::npos) result=true;
		}
		if (c.op_.contains(CT_NOT)) result=!result;
		return result;
	}
	static bool condition_array(condition c) {
		if (c.is_conditions_) return false;
		return true;
	}
	static bool condition_null(condition c) {
		return false;
	}
	
public:
	template <typename _Tp>
	void register_dbtype(const std::string& name,void (*fromfunc)(std::string,void*));
	template <typename _Tp>
	void register_dbtype(const std::string& name,bool (*condfunc)(condition));
	template <typename _Tp>
	void register_dbtype(const std::string& name,std::string (*tofunc)(void*),bool (*condfunc)(condition));
	template <typename _Tp>
	void register_dbtype(const std::string& name,void (*fromfunc)(std::string,void*),bool (*condfunc)(condition));
	template <typename _Tp>
	void register_dbtype(const std::string& name,std::string (*tofunc)(void*)=nullptr,void (*fromfunc)(std::string,void*)=nullptr,bool (*condfunc)(condition)=nullptr);
	std::string(*to_string_func(const std::string& name))(void*);
	void(*from_string_func(const std::string& name))(std::string,void*);
	bool(*condition_func(const std::string& name))(condition);
	
	database() : float_length_(6) {
		register_dbtype<intptr_t>("number");
		register_dbtype<bool>("bool");
		register_dbtype<double>("real");
		register_dbtype<std::string>("string");
		register_dbtype<std::vector<char*>>("array");
		db_listener_=nullptr;
		generate_lexer();
		generate_parser();
		generate_operand();
	}
	database(type_registry reg) : reg_(reg) , float_length_(6) {
		db_listener_=nullptr;
		generate_lexer();
		generate_parser();
		generate_operand();
	}
	~database() {
		while (tables_.size()) {
			delete_table(tables_[0]->meta_.struct_def_.name_);
		}
	}
	
	void write(std::ostream& os) {
		serialization::binary_writer writer(os);
		char* temp=new char[sizeof(std::size_t)];
		sprintf(temp,"%0*d",sizeof(std::size_t),tables_.size());
		os.write(reinterpret_cast<const char*>(temp),sizeof(std::size_t));
		for (auto it:tables_) {
			sprintf(temp,"%0*d",sizeof(std::size_t),it->meta_.struct_def_.name_.size());
			os.write(reinterpret_cast<const char*>(temp),sizeof(std::size_t));
			os.write(it->meta_.struct_def_.name_.c_str(),it->meta_.struct_def_.name_.size());
			std::string str=it->meta_.to_string();
			sprintf(temp,"%0*d",sizeof(std::size_t),str.size());
			os.write(reinterpret_cast<const char*>(temp),sizeof(std::size_t));
			os.write(str.c_str(),str.size());
			sprintf(temp,"%0*d",sizeof(std::size_t),it->datas_.size());
			for (auto jt:it->datas_) writer.write(jt);
			//if (it->strict_) os.write("true ",5);
			//else os.write("false",5);
			sprintf(temp,"%0*d",sizeof(std::size_t),it->pks_.size());
			os.write(reinterpret_cast<const char*>(temp),sizeof(std::size_t));
			for (auto jt:it->pks_) {
				sprintf(temp,"%0*d",sizeof(std::size_t),jt.size());
				os.write(reinterpret_cast<const char*>(temp),sizeof(std::size_t));
				os.write(jt.c_str(),jt.size());
			}
		}
		return;
	}
	
	bool read(std::istream& is) {
		serialization::binary_reader reader(is);
		//using get_size=serialization::binary_reader::get_size;

//examine type_registry
		return true;
	}
	
private:
	bool create_table(std::string name,metadata meta) {
		for (auto it:tables_) {
			if (it->meta_.struct_def_.name_==name) return false;
		}
		table* temp=new table(name,meta);
		tables_.push_back(temp);
		return true;
	}
	
	bool create_table(std::string name,metadata meta,std::vector<std::string> pks) {
		for (auto it:tables_) {
			if (it->meta_.struct_def_.name_==name) return false;
		}
		//try
		table* temp=new table(name,meta,pks);
		//catch
		tables_.push_back(temp);
		return true;
	}
	
	/*//SELECT INTO?
	std::vector<uintptr_t> select(std::string name,condition conds) {
		if (conds.is_conditions_) {
			//
		} else {
			//if ()
		}
	}
	
	bool select_into(std::string name,std::string into_name,condition conds) {
		
	}*/
public:
	struct table_row {
		int table_;
		int row_;
		bool operator ==(const table_row& other) {
			return table_==other.table_ && row_==other.row_;
		}
	};
	bool get_expression_tables_and_rewrite(expression* exp,std::set<std::string>& tables,std::string& table_name) {
		if (exp->is_expressions_) {
			bool result=true;
			if (exp->exp_.exps_.exp1_) result&=get_expression_tables_and_rewrite(exp->exp_.exps_.exp1_,tables,table_name);
			if (exp->exp_.exps_.exp2_) result&=get_expression_tables_and_rewrite(exp->exp_.exps_.exp2_,tables,table_name);
			return result;
		}
		if (exp->exp_.an_exp_.is_val_) return true;
		std::string current_name=(exp->exp_.an_exp_.table_=="")?table_name:(exp->exp_.an_exp_.table_);
		int i=find_table(current_name);
		if (i==-1) return false;
		if (table_name!=current_name) tables.insert(current_name);
		bool find=false;
		for (auto it:tables_[i]->meta_.struct_def_.fields_) {
			if (exp->exp_.an_exp_.segment_=="<ID>") {
				exp->exp_.an_exp_.type_="number";
				exp->exp_.an_exp_.table_=table_name;
				find=true;
			} else if (exp->exp_.an_exp_.segment_==it.name_) {
				exp->exp_.an_exp_.type_=it.type_;
				exp->exp_.an_exp_.table_=current_name;
				find=true;
			}
		}
		if (!find) return false;
		return true;
	}
	bool get_condition_tables(condition* cond,std::set<std::string>& tables,std::string& table_name) {
		if (cond->is_conditions_) {
			bool result=true;
			if (cond->val1_) result&=get_condition_tables((condition*)(cond->val1_),tables,table_name);
			if (cond->val2_) result&=get_condition_tables((condition*)(cond->val2_),tables,table_name);
			return result;
		} else {
			bool result=true;
			if (cond->val1_) result&=get_expression_tables_and_rewrite((expression*)(cond->val1_),tables,table_name);
			if (cond->val2_) result&=get_expression_tables_and_rewrite((expression*)(cond->val2_),tables,table_name);
			return result;
			//when get expression->an_exp->is_val=false need to rewrite its type
		}
		return false;
	}
	bool translate_expression(expression* exp,std::vector<table_row> curr_row) {
		if (exp->is_expressions_) {
			bool result=true;
			if (exp->exp_.exps_.exp1_) result&=translate_expression(exp->exp_.exps_.exp1_,curr_row);
			if (exp->exp_.exps_.exp2_) result&=translate_expression(exp->exp_.exps_.exp2_,curr_row);
			return result;
		} else if (!exp->exp_.an_exp_.is_val_) {
			std::string table_name=exp->exp_.an_exp_.table_;
			table_row* result_row=nullptr;
			for (auto& it:curr_row) {
				if (tables_[it.table_]->meta_.struct_def_.name_==table_name) result_row=&it;
			}
			if (!result_row) return false;
			if (exp->exp_.an_exp_.segment_=="<ID>") {
				intptr_t id=result_row->row_;
				exp->exp_.an_exp_.val_=calc_int_string((void*)(&id));
				exp->exp_.an_exp_.is_val_=true;
				exp->exp_.an_exp_.type_="number";
			} else {
				void* ds=tables_[result_row->table_]->datas_[result_row->row_][exp->exp_.an_exp_.segment_].ptr_;
				auto func=to_string_func(exp->exp_.an_exp_.type_);
				exp->exp_.an_exp_.val_=func(ds);
				exp->exp_.an_exp_.is_val_=true;
			}
			return true;
		} else return true;
		return false;
	}
	bool translate_condition(condition* cond,std::vector<table_row> curr_row) {
		if (cond->is_conditions_) {
			bool result=true;
			if (cond->val1_) result&=translate_condition((condition*)cond->val1_,curr_row);
			if (cond->val2_) result&=translate_condition((condition*)cond->val2_,curr_row);
			return result;
		} else {
			bool result=true;
			if (cond->val1_) result&=translate_expression((expression*)cond->val1_,curr_row);
			if (cond->val2_) result&=translate_expression((expression*)cond->val2_,curr_row);
			return result;
		}
		return false;
	}
	int calculate_condition(condition* cond) {
		if (cond->is_conditions_) {
			bool result1=(!cond->val1_)?false:calculate_condition((condition*)cond->val1_);
			bool result2=(!cond->val2_)?false:calculate_condition((condition*)cond->val2_);
			if (cond->op_.contains(CT_AND) && (cond->op_>>CT_AND).empty()) return result1&result2;
			else if (cond->op_.contains(CT_OR) && (cond->op_>>CT_OR).empty()) return result1|result2;
			else return 2;
		} else {
			if (cond->val1_) ((expression*)cond->val1_)->calculate(this,reg_);
			if (cond->val2_) ((expression*)cond->val2_)->calculate(this,reg_);
			if (!cond->val1_ || !cond->val2_) return 3;
			bool result=false;
			std::map<condition_type,operand_type> mp={{CT_EQUAL,OT_equal},{CT_LESS,OT_less},{CT_GREAT,OT_greater},{CT_CONTAINS,OT_contains}};
			for (auto& it:mp) {
				if (cond->op_.contains(it.first)) {
					metadata* m=new metadata(reg_,false);
					metadata::field_def fd1,fd2;
					fd1.name_="1";
					fd1.type_=((expression*)cond->val1_)->exp_.an_exp_.type_;
					m->struct_def_.fields_.push_back(fd1);
					fd2.name_="2";
					fd2.type_=((expression*)cond->val2_)->exp_.an_exp_.type_;
					m->struct_def_.fields_.push_back(fd2);
					m->calculate_offsets();
					dynamic_struct* ds=new dynamic_struct(m);
					from_string_func(fd1.type_)(((expression*)cond->val1_)->exp_.an_exp_.val_,(*ds)["1"].ptr_);
					from_string_func(fd2.type_)(((expression*)cond->val2_)->exp_.an_exp_.val_,(*ds)["2"].ptr_);
					std::string temp_str;
					auto func=operator_func(((expression*)cond->val1_)->exp_.an_exp_.type_,((expression*)cond->val2_)->exp_.an_exp_.type_,it.second);
					std::string temp=((this->*func)((*ds)["1"].ptr_,(*ds)["2"].ptr_,temp_str));
					result|=(temp=="1");
					delete ds;
					delete m;
				}
			}
			
			if (cond->op_.contains(CT_NOT)) result=!result;
			return result;
		}
	}
	
	bool examine_val(expression* exp) {
		if (exp->is_expressions_) {
			bool result=true;
			if (exp->exp_.exps_.exp1_) result&=examine_val(exp->exp_.exps_.exp1_);
			if (exp->exp_.exps_.exp2_) result&=examine_val(exp->exp_.exps_.exp2_);
			return result;
		}
		return (exp->exp_.an_exp_.is_val_);
	}
		
private:
	int select(std::string name,std::vector<std::string> ids,condition* conds,table** result) {
		int i=find_table(name);
		if (i==-1) return 1;
		metadata m(reg_,false);
		if (ids.empty() || (ids.size()==1 && ids[0]=="*")) {
			ids.clear();
			for (auto& it:tables_[i]->meta_.struct_def_.fields_) {
				metadata::field_def fd;
				fd.name_=it.name_;
				fd.type_=it.type_;
				m.struct_def_.fields_.push_back(fd);
				ids.push_back(it.name_);
			}
		} else {
			for (auto& it:ids) {
				bool find=false;
				for (auto& jt:tables_[i]->meta_.struct_def_.fields_) {
					if (it==jt.name_) {
						find=true;
						metadata::field_def fd;
						fd.name_=it;
						fd.type_=jt.type_;
						m.struct_def_.fields_.push_back(fd);
					}
				}
				if (!find) return 2;
			}
		}
		m.calculate_offsets();
		
		table* temp_table=new table("temp",m);
		if (result) *result=temp_table;//temp_table->~table();
		//new(temp_table) table("temp",m);

		std::set<std::string> temp_names;
		if (conds) {
			bool seg_result=get_condition_tables(conds,temp_names,name);
			if (!seg_result) {
				delete temp_table;
				return 3;
			}
		}
		std::vector<std::string> table_names(temp_names.begin(),temp_names.end());
		std::vector<std::vector<table_row>> rows;
		if (!table_names.empty()) {
			for (auto& it:table_names) {
				int i=find_table(it);
				if (i==-1) {
					delete temp_table;
					return 4;
				}
				std::vector<table_row> temp_row;
				int num=tables_[i]->datas_.size();
				for (int j=0;j<num;j++) temp_row.push_back({i,j});
				rows.push_back(temp_row);
			}
			rows=std::math::cartesian_product<table_row>(rows);
		} else rows.push_back(std::vector<table_row>());
		for (auto& it:rows) {
			auto original_row=it;
			for (int j=0;j<tables_[i]->datas_.size();j++) {
				it.push_back(table_row({i,j}));
				//TEST CONDITIONS
				int calc_result=1;
				condition* temp_cond=nullptr;
				if (conds) {
					//FIRST TRANSLATE EXPRESSION->NOT_EXP->NOT_VAL to VAL
					temp_cond=new condition(*conds);
					bool trans_result=translate_condition(temp_cond,it);
					if (!trans_result) {
						delete temp_cond;
						delete temp_table;
						return 5;
					}
					//SECOND EXPRESSION->EXP->CALCULATE & THIRD CONDITION->CALULATE
					calc_result=calculate_condition(temp_cond);
				}
				if (calc_result>1) {
					if(temp_cond) delete temp_cond;
					delete temp_table;
					return calc_result+4;
				}
				if (calc_result) {
					dynamic_struct ds(&temp_table->meta_);
					for (int k=0;k<ids.size() && k<m.struct_def_.fields_.size();k++) {
						std::string temp_str=to_string_func(m.struct_def_.fields_[k].type_)(tables_[i]->datas_[j][ids[k]].ptr_);
						from_string_func(m.struct_def_.fields_[k].type_)(temp_str,ds[ids[k]].ptr_);
					}
					temp_table->datas_.push_back(ds);
				}
				//WHEN THIRD EXP CAN USE CONTAINS EQUAL ... WHILE COND CAN USE AND OR ...
				if(temp_cond) delete temp_cond;
				it=original_row;
			}
		}
		return 0;
	}
	
	int update(std::string name,std::vector<std::string> ids,std::vector<expression*> values,condition* conds) {
		int i=find_table(name);
		if (i==-1) return 1;
		if (ids.size()!=values.size()) return 8;
		metadata m(reg_,false);
		if (ids.empty() || (ids.size()==1 && ids[0]=="*")) {
			ids.clear();
			for (auto& it:tables_[i]->meta_.struct_def_.fields_) {
				metadata::field_def fd;
				fd.name_=it.name_;
				fd.type_=it.type_;
				m.struct_def_.fields_.push_back(fd);
				ids.push_back(it.name_);
			}
		} else {
			for (auto& it:ids) {
				bool find=false;
				for (auto& jt:tables_[i]->meta_.struct_def_.fields_) {
					if (it==jt.name_) {
						metadata::field_def fd;
						fd.name_=it;
						fd.type_=jt.type_;
						m.struct_def_.fields_.push_back(fd);
						find=true;
					}
				}
				if (!find) return 2;
			}
		}
		m.calculate_offsets();
		std::set<std::string> temp_names;
		if (conds) {
			bool seg_result=get_condition_tables(conds,temp_names,name);
			if (!seg_result) return 3;
		}
		if (!temp_names.empty()) return 4;
		
		for (int j=0;j<tables_[i]->datas_.size();j++) {
			int calc_result=1;
			condition* temp_cond=nullptr;
			std::vector<table_row> it={table_row({i,j})};
			if (conds) {
				temp_cond=new condition(*conds);
				bool trans_result=translate_condition(temp_cond,it);
				if (!trans_result) {
					delete temp_cond;
					return 5;
				}
				calc_result=calculate_condition(temp_cond);
			}
			if (calc_result>1) {
				if(temp_cond) delete temp_cond;
				return calc_result+4;
			}
			bool success=1;
			if (calc_result) {
				std::vector<expression*> curr_exps;
				for (auto& jt:values) {
					expression* curr_exp=new expression(*jt);
					success&=get_expression_tables_and_rewrite(curr_exp,temp_names,name);
					success&=translate_expression(curr_exp,it);
					curr_exps.push_back(curr_exp);
				}
				if (success) {
					for (int k=0;k<ids.size();k++) {
						try {
							if (!examine_val(curr_exps[k])) {
								success=2;
								break;
							}
							curr_exps[k]->calculate(this,reg_);
							from_string_func(m.struct_def_.fields_[k].type_)(curr_exps[k]->exp_.an_exp_.val_,tables_[i]->datas_[j][ids[k]].ptr_);
						} catch (const exception& e) {
							success=3;
							break;
						}
					}
				}
				for (int k=0;k<curr_exps.size();k++) delete curr_exps[k];
			}
			if(temp_cond) delete temp_cond;
			if (success!=1) return 7+success;
		}
		return 0;
	}
	
	int delete_row(std::string name,condition* conds) {
		int i=find_table(name);
		if (i==-1) return 1;
		std::set<std::string> temp_names;
		if (conds) {
			bool seg_result=get_condition_tables(conds,temp_names,name);
			if (!seg_result) return 2;
		}
		if (!temp_names.empty()) return 3;
		int j=0;
		for (auto it=tables_[i]->datas_.begin();it!=tables_[i]->datas_.end();j++) {
			int calc_result=1;
			condition* temp_cond=nullptr;
			std::vector<table_row> jt={table_row({i,j})};
			if (conds) {
				temp_cond=new condition(*conds);
				bool trans_result=translate_condition(temp_cond,jt);
				if (!trans_result) {
					delete temp_cond;
					return 4;
				}
				calc_result=calculate_condition(temp_cond);
			}
			if (calc_result>1) {
				if(temp_cond) delete temp_cond;
				return calc_result+3;
			}
			if (calc_result) it=tables_[i]->datas_.erase(it);
			else it++;
			if(temp_cond) delete temp_cond;
		}
		return 0;
	}
	
	int insert(std::string name,std::vector<expression*> values,std::string& message) {
		int i=find_table(name);
		if (i<0) {
			message="Table does not exist!";
			return 1;
		}
		if (values.size()!=tables_[i]->meta_.struct_def_.fields_.size()) {
			message="Invalid amount of values of INSERT SENTENCE(INSERT name VALUE values)!";
			return 2;
		}
		metadata* m=new metadata(reg_,false);
		m->struct_def_=tables_[i]->meta_.struct_def_;
		m->size_=tables_[i]->meta_.size_;
		m->strict_=tables_[i]->meta_.strict_;
		m->calculate_offsets();
		
		std::set<std::string> temp_names;
		for (auto& it:values) {
			bool seg_result=get_expression_tables_and_rewrite(it,temp_names,name);
			if (!seg_result) {
				message="Invalid segment name in INSERT SENTENCE!";
				return 3;
			}
		}
		std::vector<std::string> table_names(temp_names.begin(),temp_names.end());
		std::vector<std::vector<table_row>> rows;
		if (!table_names.empty()) {
			for (auto& it:table_names) {
				int i=find_table(it);
				if (i==-1) {
					message="Invalid table name in INSERT SENTENCE!";
					return 4;
				}
				std::vector<table_row> temp_row;
				int num=tables_[i]->datas_.size();
				for (int j=0;j<num;j++) temp_row.push_back({i,j});
				rows.push_back(temp_row);
			}
			rows=std::math::cartesian_product<table_row>(rows);
		} else rows.push_back(std::vector<table_row>());
		for (auto& it:rows) {
			std::vector<expression*> curr_expressions;
			bool translate_result=true;
			for (auto& jt:values) {
				expression* curr_exp=new expression(*jt);
				translate_result&=translate_expression(curr_exp,it);
				curr_expressions.push_back(curr_exp);
			}
			if (!translate_result) {
				for (int j=0;j<curr_expressions.size();j++) {
					delete curr_expressions[j];
					curr_expressions[j]=nullptr;
				}
				message="Failed to translate expression in INSERT SENTENCE!";
				return 5;
			}
			dynamic_struct ds(m);
			for (int j=0;j<curr_expressions.size();j++) {
				auto field=tables_[i]->meta_.struct_def_.fields_[j];
				std::string name=field.name_;
				try {
					if (!examine_val(curr_expressions[j])) {
						message="INSERT VALUE stmt cannot insert a [table].segment value!";
						for (int k=0;k<curr_expressions.size();k++) {
							delete curr_expressions[k];
							curr_expressions[k]=nullptr;
						}
						return 6;
					}
					curr_expressions[j]->calculate(this,reg_);
					from_string_func(field.type_)(curr_expressions[j]->exp_.an_exp_.val_,ds[name].ptr_);
				} catch (const exception& e) {
					message="Invalid value:"+curr_expressions[j]->exp_.an_exp_.val_+" with error:"+e.what()+"";
					return 7;
				}
			}
			int success=insert(name,ds);
			for (int j=0;j<curr_expressions.size();j++) {
				delete curr_expressions[j];
				curr_expressions[j]=nullptr;
			}
			if (!success) continue;
			switch (success) {
				case 1: {
					message="Table does not exist!";
					break;
				}
				case 2: {
					message="Duplicated value!";
					break;
				}
				case 3: {
					message="Undefined type name!";
					break;
				}
				case 4: {
					message="Invalid meta!";
					break;
				}
				default: {
					message="Unknown error of INSERT SENTENCE!";
					break;
				}
			}
			return success+7;
		}
		return 0;
	}

	int insert(std::string name,dynamic_struct value) {
		int i=find_table(name);
		if (i==-1) return 1;
		if ((*value.meta_)!=tables_[i]->meta_) return 4;
		if (!tables_[i]->pks_.empty()) {
			for (auto it:tables_[i]->datas_) {
				bool duplicated=true;
				if (tables_[i]->pks_.empty()) duplicated=false;
				for (auto jt:tables_[i]->pks_) {
					std::string type="";
					for (auto kt:tables_[i]->meta_.struct_def_.fields_) {
						if (kt.name_==jt) type=kt.type_;
					}//metadata::find_field(jt).type_;
					if (!reg_.has_type(type)) return 3;
					type_registry::type_info info=reg_.get_type_info(type);
					if (info.is_string_) {
						const auto& p1=it[jt];
						const auto& p2=value[jt];
						std::string* v1=reinterpret_cast<std::string*>(p1.ptr_);
						std::string* v2=reinterpret_cast<std::string*>(p2.ptr_);
						if (*v1!=*v2) duplicated=false;
					} else if (info.is_primitive_) {
						const auto& p1=it[jt];
						const auto& p2=value[jt];
						for (int i=0;i<info.size_;i++) {
							if (*(((char*)p1.ptr_)+i)!=*(((char*)p2.ptr_)+i)) {
								duplicated=false;
								break;
							}
						}
					} else {
						const auto& p1=it[jt];
						const auto& p2=value[jt];
						std::vector<char*>* v1=reinterpret_cast<std::vector<char*>*>(p1.ptr_);
						std::vector<char*>* v2=reinterpret_cast<std::vector<char*>*>(p2.ptr_);
						if (v1->size()!=v2->size()) {
							duplicated=false;
							break;
						}
						for (int i=0;i<v1->size();i++) {
							if (strcmp((*v1)[i],(*v2)[i])) {
								duplicated=false;
								break;
							}
						}
					}
					if (!duplicated) break;
					//if (/*it[jt].value operator!=value[jt].value*/) duplicated=false;
				}
				if (duplicated) return 2;
			}
		}
		tables_[i]->datas_.push_back(value);
		return 0;
	}
	
	bool delete_table(std::string name) {
		for (std::vector<table*>::iterator it=tables_.begin();it!=tables_.end();it++) {
			if ((*it)->meta_.struct_def_.name_==name) {
				it=tables_.erase(it);
				return true;
			}
		}
		return false;
	}
	
	std::string show_table(table* table) {
		std::string result;
		vector<int> length;
		for (auto it:table->meta_.struct_def_.fields_) {
			int curr_length=0;
			auto func=to_string_func(it.type_);
			for (auto jt:table->datas_) curr_length=std::max(curr_length,(int)func(jt[it.name_].ptr_).size());
			curr_length=std::max(curr_length,(int)it.name_.size());
			length.push_back(curr_length);
		}
		int i=0;
		for (auto it:table->meta_.struct_def_.fields_) {
			result+=" "+it.name_+" ";
			for (int j=it.name_.size();j<length[i];j++) result+=" ";
			i++;
		}
		if (table->datas_.size()) result+="\n";
		int temp=0;
		for (auto it:table->datas_) {
			i=0;
			for (auto jt:table->meta_.struct_def_.fields_) {
				auto func=to_string_func(jt.type_);
				result+=" "+func(it[jt.name_].ptr_)+" ";
				for (int j=func(it[jt.name_].ptr_).size();j<length[i];j++) result+=" ";
				i++;
			}
			if (temp++!=table->datas_.size()-1) result+="\n";
		}
		return result;
	}
	
	void sort_datas(table* table,std::string seg_name,std::string (std::meta::core::database::* sort_func)(void*,void*,std::string&)) {
		std::sort(table->datas_.begin(),table->datas_.end(),[=,&sort_func,&seg_name](auto& val1,auto& val2) {
			std::string temp_string;
			return (this->*sort_func)(val1[seg_name].ptr_,val2[seg_name].ptr_,temp_string)=="1";
		});
	}
	
	int sort_table(table* table,std::string seg_name,int order_type) {
		if (!order_type) return 0;
		if (!table) return 1;
		std::string type="";
		for (auto& it:table->meta_.struct_def_.fields_) {
			if (it.name_==seg_name) type=it.type_;
		}
		if (type.empty()) return 2;
		auto func=operator_func(type,type,(order_type==1)?OT_less:OT_greater);
		sort_datas(table,seg_name,func);
		return 0;
	}
	
public:
	//condition get_conditions(std::vector<std::syntax::lexer<token>::resolution> res) { }
	bool execute_sentence(std::string sentence,std::string& message) {
		bool success=true;
		auto resolution=lex_.resolve(sentence,success);
		if (!success || !resolution.size()) {
			message="Invalid grammar!";
			return false;
		}
		for (std::vector<std::syntax::lexer<token>::resolution>::iterator it=resolution.begin();it!=resolution.end();) {
			if (it->token_==TK_useless) it=resolution.erase(it);
			else it++;
		}
	
		std::vector<std::syntax::parser<token>::parse_node<>> parse_list;
		for (auto& it:resolution) {
			std::syntax::parser<token>::parse_node<> node;
			node.op_=it.token_;
			parse_list.push_back(node);
		}
		parse_list.push_back(std::syntax::parser<token>::parse_node<>({TK_OP_Eof,nullptr,{}}));
		dynamic_cast<database_listener*>(parse_.listeners_[0])->init(resolution);
		success=parse_.parse_with_listener(parse_list);
		if (!success && parse_.listeners_.size() && dynamic_cast<database_listener*>(parse_.listeners_[0])->message_=="") dynamic_cast<database_listener*>(parse_.listeners_[0])->message_="Syntax Error!";
		if (parse_.listeners_.size()) success&=dynamic_cast<database_listener*>(parse_.listeners_[0])->success_;
		if (success) {
			if (parse_.listeners_.size()) message=dynamic_cast<database_listener*>(parse_.listeners_[0])->message_;
			return true;
		}
		/*switch (resolution[0].token_) {
			//CREATE name FORMAT name:type name:type name:type PRIMARY name name name
			case TK_CREATE: {
				if (resolution.size()<6) {
					message="Incomplete CREATE SENTENCE!";
					return false;
				}
				if (resolution[1].token_!=TK_id || resolution[2].token_!=TK_FORMAT) {
					message="Invalid grammar of CREATE SENTENCE!(CREATE name FORMAT name:type)";
					return false;
				}
				int i=3;
				metadata m(reg_,false);
				m.struct_def_.name_=resolution[1].word_;
				while (i+2<resolution.size() && resolution[i].token_!=TK_PRIMARY) {
					if (resolution[i].token_!=TK_id || resolution[i+1].token_!=TK_seperator || resolution[i+2].token_!=TK_id) {
						message="Invalid grammar of parameters of CREATE FORMAT!(name:type)";
						return false;
					}
					if (!reg_.has_type(resolution[i+2].word_)) {
						message="Invalid typename:"+resolution[i+2].word_+" in CREATE SENTENCE!";
						return false;
					}
					for (auto it:m.struct_def_.fields_) {
						if (it.name_==resolution[i].word_) {
							message="Existed field name:"+resolution[i].word_+" in CREATE SENTENCE!";
							return false;
						}
					}
					metadata::field_def fd;
					fd.name_=resolution[i].word_;
					fd.type_=resolution[i+2].word_;
					m.struct_def_.fields_.push_back(fd);
					i+=3;
				}
				bool success;
				if (i!=resolution.size()) {
					if (resolution[i].token_!=TK_PRIMARY) {
						message="Unknown extra parameter:"+resolution[i].word_;
						return false;
					} else {
						i++;
						std::vector<std::string> pks;
						while (i<resolution.size()) {
							if (resolution[i].token_!=TK_id) {
								message="Invalid PK name:"+resolution[i].word_;
								return false;
							}
							bool find=false;
							for (auto it:m.struct_def_.fields_) {
								if (it.name_==resolution[i].word_) find=true;
							}
							if (!find) {
								message="PK name:"+resolution[i].word_+" is not existed in table!";
								return false;
							}
							if (std::find(pks.begin(),pks.end(),resolution[i].word_)!=pks.end()) {
								message="Duplicated pk name:"+resolution[i].word_+"!";
								return false;
							}
							pks.push_back(resolution[i].word_);
							i++;
						}
						m.calculate_offsets();
						success=create_table(resolution[1].word_,m,pks);
					}
				} else {
					m.calculate_offsets();
					success=create_table(resolution[1].word_,m);
				}
				if (!success) {
					message="Existed table name!";
					return false;
				}
				return true;
				break;
			}
			//INSERT name VALUE A B C D E F
			case TK_INSERT: {
				if (resolution.size()<=3 || resolution[2].token_!=TK_VALUE) {
					message="Invalid amount of parameters of INSERT SENTENCE(INSERT name VALUE values)!";
					return false;
				}
				if (resolution[1].token_!=TK_id) {
					message="Invalid table name of INSERT SENTENCE!";
					return false;
				}
				int i=find_table(resolution[1].word_);
				if (i<0) {
					message="Table does not exist!";
					return false;
				}
				if (resolution.size()!=3+tables_[i]->meta_.struct_def_.fields_.size()) {
					message="Invalid amount of values of INSERT SENTENCE(INSERT name VALUE values)!";
				}
				int j=3;
				dynamic_struct ds(&tables_[i]->meta_);
				while (j<resolution.size()) {
					auto field=tables_[i]->meta_.struct_def_.fields_[j-3];
					std::string name=field.name_;
					if (is_literal(resolution[j].token_)) {
						try {
							from_string_func(field.type_)(resolution[j].word_,ds[name].ptr_);
							//void* temp=from_string_func(field.type_)(resolution[j].word_);
							//memcpy(ds[name].ptr_,temp,field.size_);
						} catch (const exception& e) {
							message="Invalid value:"+resolution[j].word_+" with error:"+e.what();
							return false;
						}
					} else if (resolution[j].token_==TK_NULL) {
						memset(ds[name].ptr_,'\0',field.size_);
					} else {
						message="Invalid value("+resolution[j].word_+") of INSERT SENTENCE!";
						return false;
					}
					j++;
				}
				int result=insert(resolution[1].word_,ds);
				if (!result) return true;
				switch (result) {
					case 1: {
						message="Table does not exist!";
						return false;
						break;
					}
					case 2: {
						message="Duplicated value!";
						return false;
						break;
					}
					case 3: {
						message="Undefined type name!";
						return false;
						break;
					}
					case 4: {
						message="Invalid meta!";
						return false;
						break;
					}
					default: {
						break;
					}
				}
				message="Unknown error of INSERT SENTENCE!";
				return false;
				break;
			}
			//SHOW name
			case TK_SHOW: {
				if (resolution.size()!=2) {
					message="Invalid amount of parameters of SHOW SENTENCE(SHOW name)!";
					return false;
				}
				if (resolution[1].token_!=TK_id) {
					message="Invalid table name of SHOW SENTENCE(SHOW name)!";
					return false;
				}
				int i=find_table(resolution[1].word_);
				if (i<0) {
					message="Table does not exist!";
					return false;
				}
				vector<int> length;
				for (auto it:tables_[i]->meta_.struct_def_.fields_) {
					int curr_length=0;
					auto func=to_string_func(it.type_);
					for (auto jt:tables_[i]->datas_) curr_length=std::max(curr_length,(int)func(jt[it.name_].ptr_).size());
					curr_length=std::max(curr_length,(int)it.name_.size());
					length.push_back(curr_length);
				}
				int j=0;
				for (auto it:tables_[i]->meta_.struct_def_.fields_) {
					message+=" "+it.name_+" ";
					for (int k=it.name_.size();k<length[j];k++) message+=" ";
					j++;
				}
				if (tables_[i]->datas_.size()) message+="\n";
				int temp=0;
				for (auto it:tables_[i]->datas_) {
					j=0;
					for (auto jt:tables_[i]->meta_.struct_def_.fields_) {
						auto func=to_string_func(jt.type_);
						message+=" "+func(it[jt.name_].ptr_)+" ";
						for (int k=func(it[jt.name_].ptr_).size();k<length[j];k++) message+=" ";
						j++;
					}
					if (temp++!=tables_[i]->datas_.size()-1) message+="\n";
				}
				return true;
				break;
			}
			case TK_SAVE: {
				if (resolution.size()>3 || resolution.size()<2) {
					message="Invalid grammar of SAVE SENTENCE!(SAVE (path) name)!";
					return false;
				}
				std::string filename="";
				if (resolution.size()==2) {
					if (resolution[1].token_!=TK_filename && resolution[1].token_!=TK_stringliteral) {
						message="Invalid filename!";
						return false;
					}
					if (resolution[1].token_==TK_filename) {
						filename=resolution[1].word_;	
					} else {
						if (resolution[1].word_.size()<=2) {
							message="Too short filename!";
							return false;
						}
						filename=resolution[1].word_.substr(1,resolution[1].word_.size()-2);
					}				
				} else {
					if (resolution[1].token_!=TK_path || resolution[2].token_!=TK_filename) {
						message="Invalid filename!";
						return false;
					}
					filename=resolution[1].word_+resolution[2].word_;
				}
				std::ofstream outfile(filename);
			    if (outfile.is_open()) {
					write(outfile);
					outfile.close();
				} else {
					message="Open file failed!";
					return false;
				}
				return true;
				break;
			}
			case TK_SHOWTABLES: {
				for (auto it:tables_) {
					message+=it->meta_.struct_def_.name_+": ";
					for (auto jt:it->meta_.struct_def_.fields_) {
						message+=jt.name_+"("+jt.type_+")";
						if (find(it->pks_.begin(),it->pks_.end(),jt.name_)!=it->pks_.end()) message+="(pk)";
						message+=" ";
					}
					message+="\n";
				}
				std::size_t pos=message.rfind('\n');
				if (pos!=string::npos && pos==message.size()-1) message=message.substr(0,pos);
				return true;
				break;
			}
			case TK_HELP: {
				if (resolution.size()!=1) {
					message="HELP SENTENCE allows only 1 parameters!(HELP)!";
					return false;
				}
				message="(UNFINISHED)";
				return true;
				break;
			}
			//DELETE name
			//DELETE FROM
			case TK_DELETE: {
				if (resolution.size()<2) {
					message="Invalid grammar of DELETE SENTENCE(DELETE name/DELETE FROM name WHERE conditions)!";
					return false;
				}
				if (resolution.size()==2 && resolution[1].token_!=TK_id) {
					message="Invalid table name of DELETE SENTENCE(DELETE name)!";
					return false;
				}
				int i=find_table(resolution[1].word_);
				if (i<0) {
					message="Table does not exist!";
					return false;
				}
				delete_table(resolution[1].word_);
				return true;
				break;
			}
			default: {
				message="Invalid function name:"+resolution[0].word_+"!";
				return false;
			}
		}*/
		if (parse_.listeners_.size() && dynamic_cast<database_listener*>(parse_.listeners_[0])->message_!="") message=dynamic_cast<database_listener*>(parse_.listeners_[0])->message_;
		else message="Unknown error!";
		return false;
	}
	//SQL SENTENCES
	//constructor strict_=false;
};

//using condition=database::condition;
//using cond_value=database::condition_value;
	
}

}

}

#endif