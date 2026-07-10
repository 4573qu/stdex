//Last Modified At 2026/06/11
//@Version 1.0.0.0
#ifndef _STDEX_TYPE_DOM_JSON_H_
#define _STDEX_TYPE_DOM_JSON_H_ 1

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <istream>
#include <mutex>
#include <ostream>
#include <regex>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include "../../structure/dom.h"//At Least 1.0
#include "../../syntax/parser.h"//At Least 3.4

namespace stdex {

namespace type {

namespace basic_json {

enum json_symbol : int {
	JS_EPSILON,
	JS_EOF,
	JS_LBRACE,
	JS_RBRACE,
	JS_LBRACKET,
	JS_RBRACKET,
	JS_COLON,
	JS_COMMA,
	JS_STRING,
	JS_INT,
	JS_FLOAT,
	JS_TRUE,
	JS_FALSE,
	JS_NULL,
	JS_START,
	JS_VALUE,
	JS_OBJECT,
	JS_MEMBERS,
	JS_MEMBER,
	JS_ARRAY,
	JS_ELEMENTS,
};

enum json_production : int {
	JP_START,
	JP_VALUE_OBJECT,
	JP_VALUE_ARRAY,
	JP_VALUE_STRING,
	JP_VALUE_INT,
	JP_VALUE_FLOAT,
	JP_VALUE_TRUE,
	JP_VALUE_FALSE,
	JP_VALUE_NULL,
	JP_OBJECT_EMPTY,
	JP_OBJECT,
	JP_MEMBERS_FIRST,
	JP_MEMBERS_APPEND,
	JP_MEMBER,
	JP_ARRAY_EMPTY,
	JP_ARRAY,
	JP_ELEMENTS_FIRST,
	JP_ELEMENTS_APPEND,
};

template <typename _Json>
struct json_sax {
	using int_t=typename _Json::int_t;
	using float_t=typename _Json::float_t;
	using boolean_t=typename _Json::boolean_t;
	using string_t=typename _Json::string_t;

	virtual bool null()=0;
	virtual bool boolean(boolean_t value)=0;
	virtual bool number_integer(int_t value)=0;
	virtual bool number_float(float_t value,const string_t& raw)=0;
	virtual bool string(string_t& value)=0;
	virtual bool start_object(std::size_t cnt)=0;
	virtual bool key(string_t& value)=0;
	virtual bool end_object()=0;
	virtual bool start_array(std::size_t cnt)=0;
	virtual bool end_array()=0;
	virtual bool parse_error(std::size_t position,const std::string& last_token,const std::string& message)=0;
	virtual ~json_sax()=default;
};

template <typename _Json>
class json_sax_dom_builder : public json_sax<_Json> {
public:
	using int_t=typename _Json::int_t;
	using float_t=typename _Json::float_t;
	using boolean_t=typename _Json::boolean_t;
	using string_t=typename _Json::string_t;

private:
	_Json& root_;
	std::vector<_Json*> ref_stack_;
	string_t key_;
	bool root_set_=false;
	bool errored_=false;
	std::size_t error_position_=0;
	std::string error_message_;

	template <typename _Vp>
	_Json* handle_value(_Vp&& value) {
		if (ref_stack_.empty()) {
			root_=_Json(std::forward<_Vp>(value));
			root_set_=true;
			return &root_;
		}
		_Json* parent=ref_stack_.back();
		if (parent->is_array()) {
			parent->push_back(std::forward<_Vp>(value));
			return static_cast<_Json*>(&parent->back());
		}
		auto& slot=(*parent)[std::move(key_)];
		slot=std::forward<_Vp>(value);
		return static_cast<_Json*>(&slot);
	}

public:
	explicit json_sax_dom_builder(_Json& root) : root_(root) { }
	bool null() override {
		handle_value(nullptr);
		return true;
	}
	bool boolean(boolean_t value) override {
		handle_value(value);
		return true;
	}
	bool number_integer(int_t value) override {
		handle_value(value);
		return true;
	}
	bool number_float(float_t value,const string_t& raw) override {
		static_cast<void>(raw);
		handle_value(value);
		return true;
	}
	bool string(string_t& value) override {
		handle_value(std::move(value));
		return true;
	}
	bool start_object(std::size_t cnt) override {
		static_cast<void>(cnt);
		ref_stack_.push_back(handle_value(_Json(structure::DDT_OBJECT)));
		return true;
	}
	bool key(string_t& value) override {
		key_=std::move(value);
		return true;
	}
	bool end_object() override {
		ref_stack_.pop_back();
		return true;
	}
	bool start_array(std::size_t cnt) override {
		static_cast<void>(cnt);
		ref_stack_.push_back(handle_value(_Json(structure::DDT_ARRAY)));
		return true;
	}
	bool end_array() override {
		ref_stack_.pop_back();
		return true;
	}
	bool parse_error(std::size_t position,const std::string& last_token,const std::string& message) override {
		errored_=true;
		error_position_=position;
		error_message_=message+(last_token.empty()?std::string():(" near '"+last_token+"'"));
		return false;
	}
	bool errored() const noexcept {
		return errored_;
	}
	bool completed() const noexcept {
		return root_set_ && ref_stack_.empty();
	}
	std::size_t error_position() const noexcept {
		return error_position_;
	}
	const std::string& error_message() const noexcept {
		return error_message_;
	}
};

template <typename _Json>
class json_sax_acceptor : public json_sax<_Json> {
public:
	using int_t=typename _Json::int_t;
	using float_t=typename _Json::float_t;
	using boolean_t=typename _Json::boolean_t;
	using string_t=typename _Json::string_t;

	bool null() override { return true; }
	bool boolean(boolean_t) override { return true; }
	bool number_integer(int_t) override { return true; }
	bool number_float(float_t,const string_t&) override { return true; }
	bool string(string_t&) override { return true; }
	bool start_object(std::size_t) override { return true; }
	bool key(string_t&) override { return true; }
	bool end_object() override { return true; }
	bool start_array(std::size_t) override { return true; }
	bool end_array() override { return true; }
	bool parse_error(std::size_t,const std::string&,const std::string&) override { return false; }
};

//JSONPath(RFC 9535)编译后查询对象。谱系上与structure::dom_pointer(JSON Pointer)同型:
//路径小语言用手工递归下降编译成不可变AST,对dom反复求值——不是文档记法,不产SAX事件,
//故不使用文档级的SLR机器;词法细粒度沿用std::regex。求值面向基类dom(树内子节点是dom切片),
//查不到返回空(RFC语义,不抛),编译错误抛std::invalid_argument;编译后对象不可变,const求值可并发。
//成员名简写为ASCII子集近似(任意名字请走括号引号形式——RFC自身的完备通道);
//match/search的I-Regexp以std::regex ECMAScript近似。
template <typename _Json>
class json_path {
public:
	using base_t=typename _Json::base_t;
	using int_t=typename _Json::int_t;
	using float_t=typename _Json::float_t;
	using boolean_t=typename _Json::boolean_t;
	using string_t=typename _Json::string_t;
	using size_type=typename _Json::size_type;

private:
	enum selector_kind : int {
		SK_NAME,
		SK_WILDCARD,
		SK_INDEX,
		SK_SLICE,
		SK_FILTER,
	};
	enum expr_kind : int {
		EK_OR,
		EK_AND,
		EK_NOT,
		EK_COMPARISON,
		EK_EXISTS,
		EK_LITERAL,
		EK_QUERY,
		EK_FUNCTION,
	};
	enum comparison_op : int {
		CO_EQUAL,
		CO_NOT_EQUAL,
		CO_LESS,
		CO_LESS_EQUAL,
		CO_GREATER,
		CO_GREATER_EQUAL,
	};
	enum function_kind : int {
		FK_LENGTH,
		FK_COUNT,
		FK_MATCH,
		FK_SEARCH,
		FK_VALUE,
	};
	//RFC 9535类型系统:逻辑/值/节点列表;编译期检查,越界即编译错误。
	enum expr_type : int {
		ET_LOGICAL,
		ET_VALUE,
		ET_NODES,
	};

	struct selector {
		selector_kind kind=SK_WILDCARD;
		string_t name{};
		long long index=0;
		long long slice_start=0;
		long long slice_end=0;
		long long slice_step=1;
		bool has_start=false;
		bool has_end=false;
		bool has_step=false;
		int filter=-1;//表达式池下标
	};
	struct segment {
		bool descendant=false;//".."后代段
		std::vector<selector> selectors;
	};
	struct expr_node {
		expr_kind kind=EK_LITERAL;
		expr_type type=ET_VALUE;
		comparison_op op=CO_EQUAL;
		function_kind function=FK_LENGTH;
		std::vector<int> children;
		base_t literal{};
		std::vector<segment> query;
		bool absolute=false;
		bool singular=false;
	};
	struct path_step {
		bool is_index=false;
		size_type index=0;
		string_t name{};
	};
	struct comparable_result {
		bool has_value=false;
		base_t value{};
	};

	std::string expression_;
	std::vector<segment> segments_;
	std::vector<expr_node> pool_;

	//---词法细粒度(与json.h词法同源的std::regex)---
	static const std::regex& shorthand_name_regex() {
		static const std::regex result(R"([A-Za-z_][A-Za-z0-9_]*)",std::regex::optimize);
		return result;
	}
	static const std::regex& int_regex() {
		static const std::regex result(R"(-?(?:0|[1-9][0-9]*))",std::regex::optimize);
		return result;
	}
	static const std::regex& number_regex() {
		static const std::regex result(R"(-?(?:0|[1-9][0-9]*)(\.[0-9]+)?([eE][+-]?[0-9]+)?)",std::regex::optimize);
		return result;
	}

	[[noreturn]]
	void fail(std::size_t position,const std::string& message) const {
		throw std::invalid_argument(message+" at index "+std::to_string(position)+" in JSONPath '"+expression_+"'");
	}
	void skip_blank(std::size_t& pos) const noexcept {
		while (pos<expression_.size()) {
			const char c=expression_[pos];
			if (c!=' ' && c!='\t' && c!='\n' && c!='\r') break;
			pos++;
		}
	}
	bool match_regex(std::size_t& pos,const std::regex& pattern,std::string& text) const {
		std::cmatch match;
		const char* first=expression_.data()+pos;
		const char* const last=expression_.data()+expression_.size();
		if (first>=last || !std::regex_search(first,last,match,pattern,std::regex_constants::match_continuous)) return false;
		text.assign(match[0].first,match[0].second);
		pos+=text.size();
		return true;
	}
	bool match_literal(std::size_t& pos,const char* text) const noexcept {
		const std::size_t length=std::strlen(text);
		if (expression_.size()-pos<length || std::memcmp(expression_.data()+pos,text,length)!=0) return false;
		pos+=length;
		return true;
	}
	static bool is_name_char(char c) noexcept {
		return (c>='A' && c<='Z') || (c>='a' && c<='z') || (c>='0' && c<='9') || c=='_';
	}

	static void append_codepoint(string_t& out,unsigned long cp) {
		if (cp<0x80) out.push_back(static_cast<typename string_t::value_type>(cp));
		else if (cp<0x800) {
			out.push_back(static_cast<typename string_t::value_type>(0xC0|(cp>>6)));
			out.push_back(static_cast<typename string_t::value_type>(0x80|(cp&0x3F)));
		} else if (cp<0x10000) {
			out.push_back(static_cast<typename string_t::value_type>(0xE0|(cp>>12)));
			out.push_back(static_cast<typename string_t::value_type>(0x80|((cp>>6)&0x3F)));
			out.push_back(static_cast<typename string_t::value_type>(0x80|(cp&0x3F)));
		} else {
			out.push_back(static_cast<typename string_t::value_type>(0xF0|(cp>>18)));
			out.push_back(static_cast<typename string_t::value_type>(0x80|((cp>>12)&0x3F)));
			out.push_back(static_cast<typename string_t::value_type>(0x80|((cp>>6)&0x3F)));
			out.push_back(static_cast<typename string_t::value_type>(0x80|(cp&0x3F)));
		}
	}
	//JSONPath字符串字面量:单双引号皆可,转义集与JSON一致外加对应引号自身。
	string_t parse_string_literal(std::size_t& pos) const {
		const char quote=expression_[pos];
		const std::size_t start=pos;
		pos++;
		string_t result;
		auto hex4=[this](std::size_t at)->unsigned long{
			unsigned long value=0;
			for (int i=0;i<4;i++) {
				const char c=expression_[at+i];
				value<<=4;
				if (c>='0' && c<='9') value|=static_cast<unsigned long>(c-'0');
				else if (c>='a' && c<='f') value|=static_cast<unsigned long>(c-'a'+10);
				else if (c>='A' && c<='F') value|=static_cast<unsigned long>(c-'A'+10);
				else fail(at+i,"Invalid hexadecimal digit in \\u escape");
			}
			return value;
		};
		while (true) {
			if (pos>=expression_.size()) fail(start,"Unterminated string literal");
			const char c=expression_[pos];
			if (c==quote) {
				pos++;
				return result;
			}
			if (static_cast<unsigned char>(c)<0x20) fail(pos,"Control character in string literal");
			if (c!='\\') {
				result.push_back(c);
				pos++;
				continue;
			}
			pos++;
			if (pos>=expression_.size()) fail(start,"Unterminated string literal");
			const char escape=expression_[pos++];
			switch (escape) {
				case '\'': result.push_back('\'');break;
				case '"': result.push_back('"');break;
				case '\\': result.push_back('\\');break;
				case '/': result.push_back('/');break;
				case 'b': result.push_back('\b');break;
				case 'f': result.push_back('\f');break;
				case 'n': result.push_back('\n');break;
				case 'r': result.push_back('\r');break;
				case 't': result.push_back('\t');break;
				case 'u': {
					if (expression_.size()-pos<4) fail(pos,"Truncated \\u escape");
					unsigned long cp=hex4(pos);
					pos+=4;
					if (cp>=0xD800 && cp<=0xDBFF) {
						if (expression_.size()-pos>=6 && expression_[pos]=='\\' && expression_[pos+1]=='u') {
							const unsigned long low=hex4(pos+2);
							if (low>=0xDC00 && low<=0xDFFF) {
								cp=0x10000+((cp-0xD800)<<10)+(low-0xDC00);
								pos+=6;
							} else fail(pos,"Invalid low surrogate in \\u escape");
						} else fail(pos,"Lone high surrogate in \\u escape");
					} else if (cp>=0xDC00 && cp<=0xDFFF) fail(pos,"Lone low surrogate in \\u escape");
					append_codepoint(result,cp);
					break;
				}
				default: fail(pos-1,"Invalid escape sequence");
			}
		}
	}

	//---段与选择器解析---
	void compile() {
		if (expression_.empty() || expression_[0]!='$') throw std::invalid_argument("JSONPath must start with '$'");
		std::size_t pos=1;
		parse_segments(segments_,pos,false);
	}
	//embedded=true时用于过滤器内的@/$子查询:遇到非段起始字符停下交还外层。
	void parse_segments(std::vector<segment>& out,std::size_t& pos,bool embedded) {
		while (true) {
			const std::size_t mark=pos;
			skip_blank(pos);
			if (pos>=expression_.size()) {
				if (embedded) pos=mark;
				return;
			}
			const char c=expression_[pos];
			segment current;
			if (c=='.') {
				pos++;
				if (pos<expression_.size() && expression_[pos]=='.') {
					pos++;
					current.descendant=true;
					if (pos<expression_.size() && expression_[pos]=='[') {
						parse_bracket(current,pos);
						out.push_back(std::move(current));
						continue;
					}
				}
				selector picked;
				if (pos<expression_.size() && expression_[pos]=='*') {
					pos++;
					picked.kind=SK_WILDCARD;
				} else {
					std::string text;
					if (!match_regex(pos,shorthand_name_regex(),text)) fail(pos,"Expected a member name or '*' after '.'");
					picked.kind=SK_NAME;
					picked.name=string_t(text.begin(),text.end());
				}
				current.selectors.push_back(std::move(picked));
				out.push_back(std::move(current));
			} else if (c=='[') {
				parse_bracket(current,pos);
				out.push_back(std::move(current));
			} else {
				if (embedded) {
					pos=mark;
					return;
				}
				fail(pos,std::string("Unexpected character '")+c+"'");
			}
		}
	}
	void parse_bracket(segment& out,std::size_t& pos) {
		pos++;//'['
		while (true) {
			skip_blank(pos);
			if (pos>=expression_.size()) fail(pos,"Unterminated bracket segment");
			out.selectors.push_back(parse_selector(pos));
			skip_blank(pos);
			if (pos>=expression_.size()) fail(pos,"Unterminated bracket segment");
			const char c=expression_[pos];
			if (c==']') {
				pos++;
				return;
			}
			if (c==',') {
				pos++;
				continue;
			}
			fail(pos,std::string("Expected ',' or ']' but found '")+c+"'");
		}
	}
	selector parse_selector(std::size_t& pos) {
		selector result;
		const char c=expression_[pos];
		if (c=='\'' || c=='"') {
			result.kind=SK_NAME;
			result.name=parse_string_literal(pos);
			return result;
		}
		if (c=='*') {
			pos++;
			result.kind=SK_WILDCARD;
			return result;
		}
		if (c=='?') {
			pos++;
			result.kind=SK_FILTER;
			result.filter=parse_or(pos);
			return result;
		}
		std::string text;
		bool has_first=false;
		long long first=0;
		if (match_regex(pos,int_regex(),text)) {
			has_first=true;
			first=std::strtoll(text.c_str(),nullptr,10);
		}
		skip_blank(pos);
		if (pos<expression_.size() && expression_[pos]==':') {
			pos++;
			result.kind=SK_SLICE;
			result.has_start=has_first;
			result.slice_start=first;
			skip_blank(pos);
			if (match_regex(pos,int_regex(),text)) {
				result.has_end=true;
				result.slice_end=std::strtoll(text.c_str(),nullptr,10);
			}
			skip_blank(pos);
			if (pos<expression_.size() && expression_[pos]==':') {
				pos++;
				skip_blank(pos);
				if (match_regex(pos,int_regex(),text)) {
					result.has_step=true;
					result.slice_step=std::strtoll(text.c_str(),nullptr,10);
				}
			}
			return result;
		}
		if (has_first) {
			result.kind=SK_INDEX;
			result.index=first;
			return result;
		}
		fail(pos,"Expected a selector");
	}

	//---过滤器表达式(优先级:|| < && < ! < 比较/测试;括号是逻辑初等式)---
	int make_node(expr_node&& node) {
		pool_.push_back(std::move(node));
		return static_cast<int>(pool_.size()-1);
	}
	int parse_or(std::size_t& pos) {
		int left=parse_and(pos);
		while (true) {
			skip_blank(pos);
			if (!match_literal(pos,"||")) return left;
			const int right=parse_and(pos);
			expr_node node;
			node.kind=EK_OR;
			node.type=ET_LOGICAL;
			node.children={left,right};
			left=make_node(std::move(node));
		}
	}
	int parse_and(std::size_t& pos) {
		int left=parse_not(pos);
		while (true) {
			skip_blank(pos);
			if (!match_literal(pos,"&&")) return left;
			const int right=parse_not(pos);
			expr_node node;
			node.kind=EK_AND;
			node.type=ET_LOGICAL;
			node.children={left,right};
			left=make_node(std::move(node));
		}
	}
	int parse_not(std::size_t& pos) {
		skip_blank(pos);
		if (pos<expression_.size() && expression_[pos]=='!') {
			pos++;
			const int child=parse_not(pos);
			if (pool_[child].type!=ET_LOGICAL) fail(pos,"'!' requires a logical operand");
			expr_node node;
			node.kind=EK_NOT;
			node.type=ET_LOGICAL;
			node.children={child};
			return make_node(std::move(node));
		}
		return parse_basic(pos);
	}
	void require_comparable(int index,std::size_t pos) const {
		const expr_node& node=pool_[index];
		if (node.kind==EK_LITERAL) return;
		if (node.kind==EK_QUERY) {
			if (!node.singular) fail(pos,"Comparison requires a singular query");
			return;
		}
		if (node.kind==EK_FUNCTION && node.type==ET_VALUE) return;
		fail(pos,"Expression is not comparable");
	}
	int parse_basic(std::size_t& pos) {
		skip_blank(pos);
		if (pos>=expression_.size()) fail(pos,"Unexpected end of filter expression");
		if (expression_[pos]=='(') {
			pos++;
			const int inner=parse_or(pos);
			skip_blank(pos);
			if (pos>=expression_.size() || expression_[pos]!=')') fail(pos,"Expected ')'");
			pos++;
			return inner;
		}
		const std::size_t operand_position=pos;
		const int left=parse_comparable(pos);
		skip_blank(pos);
		comparison_op op=CO_EQUAL;
		bool has_comparison=true;
		if (match_literal(pos,"==")) op=CO_EQUAL;
		else if (match_literal(pos,"!=")) op=CO_NOT_EQUAL;
		else if (match_literal(pos,"<=")) op=CO_LESS_EQUAL;
		else if (match_literal(pos,">=")) op=CO_GREATER_EQUAL;
		else if (match_literal(pos,"<")) op=CO_LESS;
		else if (match_literal(pos,">")) op=CO_GREATER;
		else has_comparison=false;
		if (has_comparison) {
			require_comparable(left,operand_position);
			const std::size_t right_position=pos;
			const int right=parse_comparable(pos);
			require_comparable(right,right_position);
			expr_node node;
			node.kind=EK_COMPARISON;
			node.type=ET_LOGICAL;
			node.op=op;
			node.children={left,right};
			return make_node(std::move(node));
		}
		if (pool_[left].kind==EK_QUERY) {//裸查询=存在性测试
			expr_node node;
			node.kind=EK_EXISTS;
			node.type=ET_LOGICAL;
			node.children={left};
			return make_node(std::move(node));
		}
		if (pool_[left].kind==EK_FUNCTION && pool_[left].type==ET_LOGICAL) return left;
		fail(operand_position,"Expression is not a valid test (a literal cannot stand alone)");
	}
	int parse_comparable(std::size_t& pos) {
		skip_blank(pos);
		if (pos>=expression_.size()) fail(pos,"Unexpected end of filter expression");
		const char c=expression_[pos];
		if (c=='@' || c=='$') {
			expr_node node;
			node.kind=EK_QUERY;
			node.type=ET_NODES;
			node.absolute=(c=='$');
			pos++;
			parse_segments(node.query,pos,true);
			node.singular=is_singular(node.query);
			return make_node(std::move(node));
		}
		if (c=='\'' || c=='"') {
			expr_node node;
			node.kind=EK_LITERAL;
			node.type=ET_VALUE;
			node.literal=base_t(parse_string_literal(pos));
			return make_node(std::move(node));
		}
		std::string text;
		{
			const std::size_t mark=pos;
			if (match_regex(pos,shorthand_name_regex(),text)) {
				if (text=="true" || text=="false" || text=="null") {
					expr_node node;
					node.kind=EK_LITERAL;
					node.type=ET_VALUE;
					if (text=="null") node.literal=base_t(nullptr);
					else node.literal=base_t(static_cast<boolean_t>(text=="true"));
					return make_node(std::move(node));
				}
				return parse_function(text,mark,pos);
			}
			pos=mark;
		}
		std::cmatch match;
		const char* first=expression_.data()+pos;
		const char* const last=expression_.data()+expression_.size();
		if (std::regex_search(first,last,match,number_regex(),std::regex_constants::match_continuous)) {
			const std::string number(match[0].first,match[0].second);
			pos+=number.size();
			expr_node node;
			node.kind=EK_LITERAL;
			node.type=ET_VALUE;
			if (match[1].matched || match[2].matched) node.literal=base_t(static_cast<float_t>(std::strtod(number.c_str(),nullptr)));
			else {
				errno=0;
				const long long value=std::strtoll(number.c_str(),nullptr,10);
				if (errno==ERANGE || value<static_cast<long long>((std::numeric_limits<int_t>::min)()) || value>static_cast<long long>((std::numeric_limits<int_t>::max)())) node.literal=base_t(static_cast<float_t>(std::strtod(number.c_str(),nullptr)));
				else node.literal=base_t(static_cast<int_t>(value));
			}
			return make_node(std::move(node));
		}
		fail(pos,std::string("Unexpected character '")+c+"' in filter expression");
	}
	int parse_function(const std::string& name,std::size_t name_position,std::size_t& pos) {
		function_kind function;
		if (name=="length") function=FK_LENGTH;
		else if (name=="count") function=FK_COUNT;
		else if (name=="match") function=FK_MATCH;
		else if (name=="search") function=FK_SEARCH;
		else if (name=="value") function=FK_VALUE;
		else fail(name_position,"Unknown function '"+name+"'");
		skip_blank(pos);
		if (pos>=expression_.size() || expression_[pos]!='(') fail(pos,"Expected '(' after function name");
		pos++;
		std::vector<int> arguments;
		skip_blank(pos);
		if (pos<expression_.size() && expression_[pos]==')') pos++;
		else {
			while (true) {
				arguments.push_back(parse_comparable(pos));
				skip_blank(pos);
				if (pos>=expression_.size()) fail(pos,"Unterminated function argument list");
				const char c=expression_[pos];
				pos++;
				if (c==')') break;
				if (c!=',') fail(pos-1,std::string("Expected ',' or ')' but found '")+c+"'");
			}
		}
		//参数类型检查(RFC 9535函数签名):length(Value) count(Nodes) value(Nodes) match/search(Value,Value)
		auto require_value_argument=[this,name_position](int index){
			const expr_node& node=pool_[index];
			if (node.kind==EK_LITERAL || (node.kind==EK_FUNCTION && node.type==ET_VALUE)) return;
			if (node.kind==EK_QUERY && node.singular) return;
			fail(name_position,"Function argument must be a value (literal, singular query or value function)");
		};
		auto require_nodes_argument=[this,name_position](int index){
			if (pool_[index].kind!=EK_QUERY) fail(name_position,"Function argument must be a query");
		};
		expr_node node;
		node.kind=EK_FUNCTION;
		node.function=function;
		switch (function) {
			case FK_LENGTH: {
				if (arguments.size()!=1) fail(name_position,"length() takes exactly one argument");
				require_value_argument(arguments[0]);
				node.type=ET_VALUE;
				break;
			}
			case FK_COUNT: {
				if (arguments.size()!=1) fail(name_position,"count() takes exactly one argument");
				require_nodes_argument(arguments[0]);
				node.type=ET_VALUE;
				break;
			}
			case FK_VALUE: {
				if (arguments.size()!=1) fail(name_position,"value() takes exactly one argument");
				require_nodes_argument(arguments[0]);
				node.type=ET_VALUE;
				break;
			}
			case FK_MATCH:
			case FK_SEARCH: {
				if (arguments.size()!=2) fail(name_position,name+"() takes exactly two arguments");
				require_value_argument(arguments[0]);
				require_value_argument(arguments[1]);
				node.type=ET_LOGICAL;
				break;
			}
		}
		node.children=std::move(arguments);
		return make_node(std::move(node));
	}
	static bool is_singular(const std::vector<segment>& query) noexcept {
		for (const auto& it:query) {
			if (it.descendant || it.selectors.size()!=1) return false;
			const selector_kind kind=it.selectors[0].kind;
			if (kind!=SK_NAME && kind!=SK_INDEX) return false;
		}
		return true;
	}

	//---求值---
	static bool is_number_node(const base_t& node) noexcept {
		return node.type()==structure::DDT_INT || node.type()==structure::DDT_FLOAT;
	}
	static double to_floating(const base_t& node) noexcept {
		if (node.type()==structure::DDT_INT) return static_cast<double>(*node.template get_ptr<const int_t*>());
		return static_cast<double>(*node.template get_ptr<const float_t*>());
	}
	//RFC 9535等价:数值跨Int/Float按数值比较(1==1.0为真),其余同型深比较——
	//dom自身的equal_impl不承诺跨Int/Float数值等价,故此处自实现。
	static bool deep_equal(const base_t& lhs,const base_t& rhs) {
		if (is_number_node(lhs) && is_number_node(rhs)) {
			if (lhs.type()==structure::DDT_INT && rhs.type()==structure::DDT_INT) return *lhs.template get_ptr<const int_t*>()==*rhs.template get_ptr<const int_t*>();
			return to_floating(lhs)==to_floating(rhs);
		}
		if (lhs.type()!=rhs.type()) return false;
		switch (static_cast<int>(lhs.type().get())) {
			case static_cast<int>(structure::DDT_NULL): return true;
			case static_cast<int>(structure::DDT_BOOL): return *lhs.template get_ptr<const boolean_t*>()==*rhs.template get_ptr<const boolean_t*>();
			case static_cast<int>(structure::DDT_STRING): return *lhs.template get_ptr<const string_t*>()==*rhs.template get_ptr<const string_t*>();
			case static_cast<int>(structure::DDT_ARRAY): {
				if (lhs.size()!=rhs.size()) return false;
				auto left=lhs.cbegin();
				auto right=rhs.cbegin();
				for (;left!=lhs.cend();left++,right++) {
					if (!deep_equal(*left,*right)) return false;
				}
				return true;
			}
			case static_cast<int>(structure::DDT_OBJECT): {
				if (lhs.size()!=rhs.size()) return false;
				for (auto it=lhs.cbegin();it!=lhs.cend();it++) {
					auto found=rhs.value().object->find(it.key());
					if (found==rhs.value().object->end() || !deep_equal(*it,found->second)) return false;
				}
				return true;
			}
			default: return false;
		}
	}
	static bool less_than(const base_t& lhs,const base_t& rhs) {
		if (is_number_node(lhs) && is_number_node(rhs)) {
			if (lhs.type()==structure::DDT_INT && rhs.type()==structure::DDT_INT) return *lhs.template get_ptr<const int_t*>()<*rhs.template get_ptr<const int_t*>();
			return to_floating(lhs)<to_floating(rhs);
		}
		if (lhs.type()==structure::DDT_STRING && rhs.type()==structure::DDT_STRING) return *lhs.template get_ptr<const string_t*>()<*rhs.template get_ptr<const string_t*>();
		return false;//其余类型无序(RFC:序比较仅数值与字符串)
	}
	static bool compare(comparison_op op,const comparable_result& lhs,const comparable_result& rhs) {
		switch (op) {
			case CO_EQUAL: {
				if (!lhs.has_value && !rhs.has_value) return true;
				if (lhs.has_value!=rhs.has_value) return false;
				return deep_equal(lhs.value,rhs.value);
			}
			case CO_NOT_EQUAL: return !compare(CO_EQUAL,lhs,rhs);
			case CO_LESS: return lhs.has_value && rhs.has_value && less_than(lhs.value,rhs.value);
			case CO_LESS_EQUAL: return compare(CO_LESS,lhs,rhs) || compare(CO_EQUAL,lhs,rhs);
			case CO_GREATER: return compare(CO_LESS,rhs,lhs);
			case CO_GREATER_EQUAL: return compare(CO_LESS,rhs,lhs) || compare(CO_EQUAL,lhs,rhs);
			default: return false;
		}
	}
	static std::size_t codepoint_count(const string_t& text) noexcept {
		std::size_t result=0;
		for (auto it:text) {
			if ((static_cast<unsigned char>(it)&0xC0)!=0x80) result++;
		}
		return result;
	}

	void collect_descendants(const base_t& node,const std::vector<path_step>* path,std::vector<const base_t*>& nodes,std::vector<std::vector<path_step>>* paths) const {
		nodes.push_back(&node);
		if (paths) paths->push_back(*path);
		if (node.type()==structure::DDT_ARRAY) {
			size_type index=0;
			for (auto it=node.cbegin();it!=node.cend();it++,index++) {
				if (paths) {
					std::vector<path_step> child_path=*path;
					path_step step;
					step.is_index=true;
					step.index=index;
					child_path.push_back(std::move(step));
					collect_descendants(*it,&child_path,nodes,paths);
				} else collect_descendants(*it,nullptr,nodes,paths);
			}
		} else if (node.type()==structure::DDT_OBJECT) {
			for (auto it=node.cbegin();it!=node.cend();it++) {
				if (paths) {
					std::vector<path_step> child_path=*path;
					path_step step;
					step.name=it.key();
					child_path.push_back(std::move(step));
					collect_descendants(*it,&child_path,nodes,paths);
				} else collect_descendants(*it,nullptr,nodes,paths);
			}
		}
	}
	void apply_selector(const selector& picked,const base_t& node,const std::vector<path_step>* path,const base_t& root,std::vector<const base_t*>& nodes,std::vector<std::vector<path_step>>* paths) const {
		auto emit=[&](const base_t& child,bool is_index,size_type index,const string_t* name){
			nodes.push_back(&child);
			if (paths) {
				std::vector<path_step> child_path=*path;
				path_step step;
				step.is_index=is_index;
				if (is_index) step.index=index;
				else step.name=*name;
				child_path.push_back(std::move(step));
				paths->push_back(std::move(child_path));
			}
		};
		switch (picked.kind) {
			case SK_NAME: {
				if (node.type()!=structure::DDT_OBJECT) return;
				auto found=node.value().object->find(picked.name);
				if (found!=node.value().object->end()) emit(found->second,false,0,&picked.name);
				break;
			}
			case SK_WILDCARD: {
				if (node.type()==structure::DDT_ARRAY) {
					size_type index=0;
					for (auto it=node.cbegin();it!=node.cend();it++,index++) emit(*it,true,index,nullptr);
				} else if (node.type()==structure::DDT_OBJECT) {
					for (auto it=node.cbegin();it!=node.cend();it++) {
						const string_t& key=it.key();
						emit(*it,false,0,&key);
					}
				}
				break;
			}
			case SK_INDEX: {
				if (node.type()!=structure::DDT_ARRAY) return;
				const long long length=static_cast<long long>(node.size());
				long long index=picked.index;
				if (index<0) index+=length;
				if (index<0 || index>=length) return;
				auto it=node.cbegin();
				for (long long i=0;i<index;i++) it++;
				emit(*it,true,static_cast<size_type>(index),nullptr);
				break;
			}
			case SK_SLICE: {
				if (node.type()!=structure::DDT_ARRAY) return;
				const long long length=static_cast<long long>(node.size());
				const long long step=picked.has_step?picked.slice_step:1;
				if (step==0 || length==0) return;
				long long start=picked.has_start?picked.slice_start:(step>0?0:length-1);
				long long end=picked.has_end?picked.slice_end:(step>0?length:-length-1);
				if (start<0) start+=length;
				if (picked.has_end && end<0) end+=length;
				std::vector<const base_t*> elements;
				elements.reserve(static_cast<std::size_t>(length));
				for (auto it=node.cbegin();it!=node.cend();it++) elements.push_back(&*it);
				if (step>0) {
					const long long lower=(std::min)((std::max)(start,0LL),length);
					const long long upper=(std::min)((std::max)(end,0LL),length);
					for (long long i=lower;i<upper;i+=step) emit(*elements[static_cast<std::size_t>(i)],true,static_cast<size_type>(i),nullptr);
				} else {
					const long long upper=(std::min)((std::max)(start,-1LL),length-1);
					const long long lower=(std::min)((std::max)(end,-1LL),length-1);
					for (long long i=upper;i>lower;i+=step) emit(*elements[static_cast<std::size_t>(i)],true,static_cast<size_type>(i),nullptr);
				}
				break;
			}
			case SK_FILTER: {
				if (node.type()==structure::DDT_ARRAY) {
					size_type index=0;
					for (auto it=node.cbegin();it!=node.cend();it++,index++) {
						if (eval_logical(picked.filter,*it,root)) emit(*it,true,index,nullptr);
					}
				} else if (node.type()==structure::DDT_OBJECT) {
					for (auto it=node.cbegin();it!=node.cend();it++) {
						if (eval_logical(picked.filter,*it,root)) {
							const string_t& key=it.key();
							emit(*it,false,0,&key);
						}
					}
				}
				break;
			}
			default: break;
		}
	}
	void apply_segments(const std::vector<segment>& query,const base_t& root,std::vector<const base_t*>& nodes,std::vector<std::vector<path_step>>* paths) const {
		for (const auto& current:query) {
			std::vector<const base_t*> next_nodes;
			std::vector<std::vector<path_step>> next_paths;
			for (std::size_t i=0;i<nodes.size();i++) {
				std::vector<const base_t*> candidates;
				std::vector<std::vector<path_step>> candidate_paths;
				if (current.descendant) collect_descendants(*nodes[i],paths?&(*paths)[i]:nullptr,candidates,paths?&candidate_paths:nullptr);
				else {
					candidates.push_back(nodes[i]);
					if (paths) candidate_paths.push_back((*paths)[i]);
				}
				for (std::size_t j=0;j<candidates.size();j++) {
					for (const auto& picked:current.selectors) apply_selector(picked,*candidates[j],paths?&candidate_paths[j]:nullptr,root,next_nodes,paths?&next_paths:nullptr);
				}
			}
			nodes=std::move(next_nodes);
			if (paths) *paths=std::move(next_paths);
		}
	}
	std::vector<const base_t*> eval_query(const std::vector<segment>& query,bool absolute,const base_t& current,const base_t& root) const {
		std::vector<const base_t*> nodes;
		nodes.push_back(absolute?&root:&current);
		apply_segments(query,root,nodes,nullptr);
		return nodes;
	}
	bool eval_logical(int index,const base_t& current,const base_t& root) const {
		const expr_node& node=pool_[index];
		switch (node.kind) {
			case EK_OR: return eval_logical(node.children[0],current,root) || eval_logical(node.children[1],current,root);
			case EK_AND: return eval_logical(node.children[0],current,root) && eval_logical(node.children[1],current,root);
			case EK_NOT: return !eval_logical(node.children[0],current,root);
			case EK_COMPARISON: return compare(node.op,eval_comparable(node.children[0],current,root),eval_comparable(node.children[1],current,root));
			case EK_EXISTS: {
				const expr_node& query=pool_[node.children[0]];
				return !eval_query(query.query,query.absolute,current,root).empty();
			}
			case EK_FUNCTION: {
				const comparable_result pattern=eval_comparable(node.children[1],current,root);
				const comparable_result subject=eval_comparable(node.children[0],current,root);
				if (!subject.has_value || !pattern.has_value) return false;
				if (subject.value.type()!=structure::DDT_STRING || pattern.value.type()!=structure::DDT_STRING) return false;
				const string_t& subject_text=*subject.value.template get_ptr<const string_t*>();
				const string_t& pattern_text=*pattern.value.template get_ptr<const string_t*>();
				try {
					const std::regex expression(std::string(pattern_text.begin(),pattern_text.end()),std::regex::ECMAScript);
					const std::string text(subject_text.begin(),subject_text.end());
					if (node.function==FK_MATCH) return std::regex_match(text,expression);
					return std::regex_search(text,expression);
				} catch (const std::regex_error&) {
					return false;
				}
			}
			default: return false;
		}
	}
	comparable_result eval_comparable(int index,const base_t& current,const base_t& root) const {
		const expr_node& node=pool_[index];
		comparable_result result;
		switch (node.kind) {
			case EK_LITERAL: {
				result.has_value=true;
				result.value=node.literal;
				break;
			}
			case EK_QUERY: {
				std::vector<const base_t*> nodes=eval_query(node.query,node.absolute,current,root);
				if (nodes.size()==1) {
					result.has_value=true;
					result.value=*nodes[0];
				}
				break;
			}
			case EK_FUNCTION: {
				switch (node.function) {
					case FK_LENGTH: {
						const comparable_result argument=eval_comparable(node.children[0],current,root);
						if (!argument.has_value) break;
						if (argument.value.type()==structure::DDT_STRING) {
							result.has_value=true;
							result.value=base_t(static_cast<int_t>(codepoint_count(*argument.value.template get_ptr<const string_t*>())));
						} else if (argument.value.type()==structure::DDT_ARRAY || argument.value.type()==structure::DDT_OBJECT) {
							result.has_value=true;
							result.value=base_t(static_cast<int_t>(argument.value.size()));
						}
						break;
					}
					case FK_COUNT: {
						const expr_node& query=pool_[node.children[0]];
						result.has_value=true;
						result.value=base_t(static_cast<int_t>(eval_query(query.query,query.absolute,current,root).size()));
						break;
					}
					case FK_VALUE: {
						const expr_node& query=pool_[node.children[0]];
						std::vector<const base_t*> nodes=eval_query(query.query,query.absolute,current,root);
						if (nodes.size()==1) {
							result.has_value=true;
							result.value=*nodes[0];
						}
						break;
					}
					case FK_MATCH:
					case FK_SEARCH:
					default: break;//逻辑型函数不可比较,编译期已拦截
				}
				break;
			}
			default: break;
		}
		return result;
	}
	//RFC 9535规范化路径:$['name'][index],名字转义'与\,控制字符用命名转义或\u00xx。
	string_t normalized_path(const std::vector<path_step>& path) const {
		string_t result;
		result.push_back('$');
		for (const auto& it:path) {
			result.push_back('[');
			if (it.is_index) {
				const std::string text=std::to_string(static_cast<unsigned long long>(it.index));
				result.append(text.begin(),text.end());
			} else {
				result.push_back('\'');
				for (auto c:it.name) {
					const unsigned char byte=static_cast<unsigned char>(c);
					if (c=='\'' || c=='\\') {
						result.push_back('\\');
						result.push_back(c);
					} else if (byte>=0x20) result.push_back(c);
					else {
						switch (c) {
							case '\b': result.append({'\\','b'});break;
							case '\f': result.append({'\\','f'});break;
							case '\n': result.append({'\\','n'});break;
							case '\r': result.append({'\\','r'});break;
							case '\t': result.append({'\\','t'});break;
							default: {
								static const char digits[]="0123456789abcdef";
								result.append({'\\','u','0','0'});
								result.push_back(digits[(byte>>4)&0xF]);
								result.push_back(digits[byte&0xF]);
								break;
							}
						}
					}
				}
				result.push_back('\'');
			}
			result.push_back(']');
		}
		return result;
	}

public:
	json_path()=default;
	explicit json_path(std::string_view expression) : expression_(expression) {
		compile();
	}

	static json_path compile(std::string_view expression) {
		return json_path(expression);
	}
	static bool try_compile(std::string_view expression,json_path& out) {
		try {
			out=json_path(expression);
			return true;
		} catch (const std::invalid_argument&) {
			return false;
		}
	}

	const std::string& expression() const noexcept {
		return expression_;
	}
	bool empty() const noexcept {
		return expression_.empty();
	}

	std::vector<const base_t*> select(const base_t& root) const {
		std::vector<const base_t*> nodes;
		nodes.push_back(&root);
		apply_segments(segments_,root,nodes,nullptr);
		return nodes;
	}
	std::vector<base_t*> select(base_t& root) const {
		const std::vector<const base_t*> nodes=select(static_cast<const base_t&>(root));
		std::vector<base_t*> result;
		result.reserve(nodes.size());
		for (const auto* it:nodes) result.push_back(const_cast<base_t*>(it));//非const根保证树可变
		return result;
	}
	const base_t* select_first(const base_t& root) const {
		const std::vector<const base_t*> nodes=select(root);
		return nodes.empty()?nullptr:nodes[0];
	}
	base_t* select_first(base_t& root) const {
		return const_cast<base_t*>(select_first(static_cast<const base_t&>(root)));
	}
	std::vector<_Json> select_values(const base_t& root) const {
		const std::vector<const base_t*> nodes=select(root);
		std::vector<_Json> result;
		result.reserve(nodes.size());
		for (const auto* it:nodes) result.push_back(_Json(*it));
		return result;
	}
	std::vector<string_t> select_paths(const base_t& root) const {
		std::vector<const base_t*> nodes;
		std::vector<std::vector<path_step>> paths;
		nodes.push_back(&root);
		paths.emplace_back();
		apply_segments(segments_,root,nodes,&paths);
		std::vector<string_t> result;
		result.reserve(paths.size());
		for (const auto& it:paths) result.push_back(normalized_path(it));
		return result;
	}
	bool exists(const base_t& root) const {
		return !select(root).empty();
	}
};

_STDEX_DOM_TPL_DECLARATION
class json : public structure::_STDEX_DOM_DEF {
public:
	using base_t=structure::_STDEX_DOM_DEF;
	using int_t=typename base_t::int_t;
	using float_t=typename base_t::float_t;
	using boolean_t=typename base_t::boolean_t;
	using string_t=typename base_t::string_t;
	using size_type=typename base_t::size_type;
	using sax_t=json_sax<json>;

	static_assert(sizeof(typename string_t::value_type)==1,"json serializer assumes a byte-oriented (UTF-8) string_t.");

	using base_t::base_t;
	using base_t::operator =;

	json()=default;
	~json() override=default;

	json(const json&)=default;
	json(json&&) noexcept=default;

	json& operator =(const json&)=default;
	json& operator =(json&&)=default;

	json(const base_t& other) : base_t(other) { }
	json(base_t&& other) noexcept : base_t(std::move(other)) { }

	static json array(typename base_t::initializer_list_t init_list={}) {
		return json(base_t::array(init_list));
	}
	static json object(typename base_t::initializer_list_t init_list={}) {
		return json(base_t::object(init_list));
	}

	//JSONPath便利接口:string_view版即编即查,重复求值请预编译json_path<json>。
	std::vector<const base_t*> select(const json_path<json>& path) const {
		return path.select(static_cast<const base_t&>(*this));
	}
	std::vector<base_t*> select(const json_path<json>& path) {
		return path.select(static_cast<base_t&>(*this));
	}
	std::vector<const base_t*> select(std::string_view expression) const {
		return json_path<json>(expression).select(static_cast<const base_t&>(*this));
	}
	std::vector<base_t*> select(std::string_view expression) {
		return json_path<json>(expression).select(static_cast<base_t&>(*this));
	}
	const base_t* select_first(const json_path<json>& path) const {
		return path.select_first(static_cast<const base_t&>(*this));
	}
	base_t* select_first(const json_path<json>& path) {
		return path.select_first(static_cast<base_t&>(*this));
	}
	const base_t* select_first(std::string_view expression) const {
		return json_path<json>(expression).select_first(static_cast<const base_t&>(*this));
	}
	base_t* select_first(std::string_view expression) {
		return json_path<json>(expression).select_first(static_cast<base_t&>(*this));
	}
	std::vector<json> select_values(const json_path<json>& path) const {
		return path.select_values(static_cast<const base_t&>(*this));
	}
	std::vector<json> select_values(std::string_view expression) const {
		return json_path<json>(expression).select_values(static_cast<const base_t&>(*this));
	}
	std::vector<string_t> select_paths(const json_path<json>& path) const {
		return path.select_paths(static_cast<const base_t&>(*this));
	}
	std::vector<string_t> select_paths(std::string_view expression) const {
		return json_path<json>(expression).select_paths(static_cast<const base_t&>(*this));
	}
	bool exists(const json_path<json>& path) const {
		return path.exists(static_cast<const base_t&>(*this));
	}
	bool exists(std::string_view expression) const {
		return json_path<json>(expression).exists(static_cast<const base_t&>(*this));
	}

private:
	struct json_token {
		json_symbol symbol;
		std::size_t position;
		string_t string{};
		int_t integer=0;
		float_t floating=0;
		bool key=false;
	};

	static const std::regex& whitespace_regex() {
		static const std::regex result(R"([ \t\r\n]+)",std::regex::optimize);
		return result;
	}
	static const std::regex& string_regex() {
		static const std::regex result(R"("(?:[^"\\\x00-\x1F]|\\(?:["\\/bfnrt]|u[0-9A-Fa-f]{4}))*")",std::regex::optimize);
		return result;
	}
	static const std::regex& number_regex() {
		static const std::regex result(R"(-?(?:0|[1-9][0-9]*)(\.[0-9]+)?([eE][+-]?[0-9]+)?)",std::regex::optimize);
		return result;
	}
	static const std::regex& literal_regex() {
		static const std::regex result(R"(true|false|null)",std::regex::optimize);
		return result;
	}
	static const std::regex& punctuation_regex() {
		static const std::regex result(R"([{}\[\]:,])",std::regex::optimize);
		return result;
	}

	static bool decode_string(const char* first,const char* last,string_t& out,std::string& error_message) {
		first++;
		last--;
		auto append_codepoint=[&out](unsigned long cp){
			if (cp<0x80) out.push_back(static_cast<typename string_t::value_type>(cp));
			else if (cp<0x800) {
				out.push_back(static_cast<typename string_t::value_type>(0xC0|(cp>>6)));
				out.push_back(static_cast<typename string_t::value_type>(0x80|(cp&0x3F)));
			} else if (cp<0x10000) {
				out.push_back(static_cast<typename string_t::value_type>(0xE0|(cp>>12)));
				out.push_back(static_cast<typename string_t::value_type>(0x80|((cp>>6)&0x3F)));
				out.push_back(static_cast<typename string_t::value_type>(0x80|(cp&0x3F)));
			} else {
				out.push_back(static_cast<typename string_t::value_type>(0xF0|(cp>>18)));
				out.push_back(static_cast<typename string_t::value_type>(0x80|((cp>>12)&0x3F)));
				out.push_back(static_cast<typename string_t::value_type>(0x80|((cp>>6)&0x3F)));
				out.push_back(static_cast<typename string_t::value_type>(0x80|(cp&0x3F)));
			}
		};
		auto hex4=[](const char* p)->unsigned long{
			unsigned long result=0;
			for (int i=0;i<4;i++) {
				const char c=p[i];
				result<<=4;
				if (c>='0' && c<='9') result|=static_cast<unsigned long>(c-'0');
				else if (c>='a' && c<='f') result|=static_cast<unsigned long>(c-'a'+10);
				else result|=static_cast<unsigned long>(c-'A'+10);
			}
			return result;
		};
		while (first<last) {
			if (*first!='\\') {
				out.push_back(*first++);
				continue;
			}
			first++;
			switch (*first++) {
				case '"': out.push_back('"');break;
				case '\\': out.push_back('\\');break;
				case '/': out.push_back('/');break;
				case 'b': out.push_back('\b');break;
				case 'f': out.push_back('\f');break;
				case 'n': out.push_back('\n');break;
				case 'r': out.push_back('\r');break;
				case 't': out.push_back('\t');break;
				case 'u': {
					unsigned long cp=hex4(first);
					first+=4;
					if (cp>=0xD800 && cp<=0xDBFF) {
						if (last-first>=6 && first[0]=='\\' && first[1]=='u') {
							const unsigned long low=hex4(first+2);
							if (low>=0xDC00 && low<=0xDFFF) {
								cp=0x10000+((cp-0xD800)<<10)+(low-0xDC00);
								first+=6;
							} else {
								error_message="invalid low surrogate in \\u escape";
								return false;
							}
						} else {
							error_message="lone high surrogate in \\u escape";
							return false;
						}
					} else if (cp>=0xDC00 && cp<=0xDFFF) {
						error_message="lone low surrogate in \\u escape";
						return false;
					}
					append_codepoint(cp);
					break;
				}
				default: {
					error_message="invalid escape sequence";
					return false;
				}
			}
		}
		return true;
	}

	static bool tokenize(std::string_view input,std::vector<json_token>& tokens,std::size_t& error_position,std::string& error_message) {
		const char* first=input.data();
		const char* const last=input.data()+input.size();
		std::cmatch match;
		const auto flags=std::regex_constants::match_continuous;
		while (first<last) {
			if (std::regex_search(first,last,match,whitespace_regex(),flags)) {
				first=match[0].second;
				continue;
			}
			json_token token;
			token.position=static_cast<std::size_t>(first-input.data());
			if (std::regex_search(first,last,match,punctuation_regex(),flags)) {
				switch (*first) {
					case '{': token.symbol=JS_LBRACE;break;
					case '}': token.symbol=JS_RBRACE;break;
					case '[': token.symbol=JS_LBRACKET;break;
					case ']': token.symbol=JS_RBRACKET;break;
					case ':': token.symbol=JS_COLON;break;
					default: token.symbol=JS_COMMA;break;
				}
				first=match[0].second;
			} else if (std::regex_search(first,last,match,string_regex(),flags)) {
				token.symbol=JS_STRING;
				if (!decode_string(match[0].first,match[0].second,token.string,error_message)) {
					error_position=token.position;
					return false;
				}
				first=match[0].second;
			} else if (std::regex_search(first,last,match,number_regex(),flags)) {
				const std::string text(match[0].first,match[0].second);
				if (match[1].matched || match[2].matched) {
					token.symbol=JS_FLOAT;
					token.floating=static_cast<float_t>(std::strtod(text.c_str(),nullptr));
				} else {
					errno=0;
					const long long value=std::strtoll(text.c_str(),nullptr,10);
					if (errno==ERANGE || value<static_cast<long long>((std::numeric_limits<int_t>::min)()) || value>static_cast<long long>((std::numeric_limits<int_t>::max)())) {
						token.symbol=JS_FLOAT;
						token.floating=static_cast<float_t>(std::strtod(text.c_str(),nullptr));
					} else {
						token.symbol=JS_INT;
						token.integer=static_cast<int_t>(value);
					}
				}
				token.string=string_t(text.begin(),text.end());
				first=match[0].second;
			} else if (std::regex_search(first,last,match,literal_regex(),flags)) {
				token.symbol=(*first=='t')?JS_TRUE:((*first=='f')?JS_FALSE:JS_NULL);
				first=match[0].second;
			} else {
				error_position=token.position;
				error_message=std::string("unexpected character '")+*first+"'";
				return false;
			}
			tokens.push_back(std::move(token));
		}
		for (std::size_t i=0;i+1<tokens.size();i++) {
			if (tokens[i].symbol==JS_STRING && tokens[i+1].symbol==JS_COLON) tokens[i].key=true;
		}
		json_token eof_token;
		eof_token.symbol=JS_EOF;
		eof_token.position=input.size();
		tokens.push_back(std::move(eof_token));
		return true;
	}

	using parser_t=syntax::parser<json_symbol,json_production>;

	static bool initialize_grammar(parser_t& target) {
		auto unit=[](json_symbol left,std::initializer_list<json_symbol> rights,json_production id){
			return syntax::single_parser_unit<json_symbol,json_production>(left,rights,id);
		};
		target.units={
			unit(JS_START,{JS_VALUE,JS_EOF},JP_START),
			unit(JS_VALUE,{JS_OBJECT},JP_VALUE_OBJECT),
			unit(JS_VALUE,{JS_ARRAY},JP_VALUE_ARRAY),
			unit(JS_VALUE,{JS_STRING},JP_VALUE_STRING),
			unit(JS_VALUE,{JS_INT},JP_VALUE_INT),
			unit(JS_VALUE,{JS_FLOAT},JP_VALUE_FLOAT),
			unit(JS_VALUE,{JS_TRUE},JP_VALUE_TRUE),
			unit(JS_VALUE,{JS_FALSE},JP_VALUE_FALSE),
			unit(JS_VALUE,{JS_NULL},JP_VALUE_NULL),
			unit(JS_OBJECT,{JS_LBRACE,JS_RBRACE},JP_OBJECT_EMPTY),
			unit(JS_OBJECT,{JS_LBRACE,JS_MEMBERS,JS_RBRACE},JP_OBJECT),
			unit(JS_MEMBERS,{JS_MEMBER},JP_MEMBERS_FIRST),
			unit(JS_MEMBERS,{JS_MEMBERS,JS_COMMA,JS_MEMBER},JP_MEMBERS_APPEND),
			unit(JS_MEMBER,{JS_STRING,JS_COLON,JS_VALUE},JP_MEMBER),
			unit(JS_ARRAY,{JS_LBRACKET,JS_RBRACKET},JP_ARRAY_EMPTY),
			unit(JS_ARRAY,{JS_LBRACKET,JS_ELEMENTS,JS_RBRACKET},JP_ARRAY),
			unit(JS_ELEMENTS,{JS_VALUE},JP_ELEMENTS_FIRST),
			unit(JS_ELEMENTS,{JS_ELEMENTS,JS_COMMA,JS_VALUE},JP_ELEMENTS_APPEND),
		};
		target.generate_parser();
		return true;
	}
	static parser_t& grammar() {
		static parser_t instance(JS_START,JS_EPSILON,JS_EOF);
		static const bool initialized=initialize_grammar(instance);
		static_cast<void>(initialized);
		return instance;
	}
	static std::mutex& grammar_mutex() {
		static std::mutex instance;
		return instance;
	}

	class json_listener : public syntax::parser_listener<json_symbol,json_production> {
		std::vector<json_token>* tokens_=nullptr;
		sax_t* sax_=nullptr;
		bool aborted_=false;
		bool failed_=false;

		void abort_check(bool keep_going) {
			if (!keep_going) aborted_=true;
		}

	public:
		void reset(std::vector<json_token>& tokens,sax_t& sax) {
			tokens_=&tokens;
			sax_=&sax;
			aborted_=false;
			failed_=false;
			this->enabled=true;
		}
		bool aborted() const noexcept {
			return aborted_;
		}
		bool failed() const noexcept {
			return failed_;
		}
		intptr_t on_shift(uintptr_t id,int state,json_symbol word) override {
			static_cast<void>(state);
			if (aborted_ || failed_) return 0;
			json_token& token=(*tokens_)[id-1];
			switch (word) {
				case JS_LBRACE: abort_check(sax_->start_object(static_cast<std::size_t>(-1)));break;
				case JS_LBRACKET: abort_check(sax_->start_array(static_cast<std::size_t>(-1)));break;
				case JS_STRING: {
					if (token.key) abort_check(sax_->key(token.string));
					break;
				}
				default: break;
			}
			return 0;
		}
		intptr_t on_reduction(uintptr_t id,int state,int next,json_production sentence_id,int reduction_num) override {
			static_cast<void>(state);
			static_cast<void>(next);
			static_cast<void>(reduction_num);
			if (aborted_ || failed_) return 0;
			json_token& token=(*tokens_)[id-2];
			switch (sentence_id) {
				case JP_VALUE_STRING: abort_check(sax_->string(token.string));break;
				case JP_VALUE_INT: abort_check(sax_->number_integer(token.integer));break;
				case JP_VALUE_FLOAT: abort_check(sax_->number_float(token.floating,token.string));break;
				case JP_VALUE_TRUE: abort_check(sax_->boolean(static_cast<boolean_t>(true)));break;
				case JP_VALUE_FALSE: abort_check(sax_->boolean(static_cast<boolean_t>(false)));break;
				case JP_VALUE_NULL: abort_check(sax_->null());break;
				case JP_OBJECT_EMPTY:
				case JP_OBJECT: abort_check(sax_->end_object());break;
				case JP_ARRAY_EMPTY:
				case JP_ARRAY: abort_check(sax_->end_array());break;
				case JP_START:
				case JP_VALUE_OBJECT:
				case JP_VALUE_ARRAY:
				case JP_MEMBERS_FIRST:
				case JP_MEMBERS_APPEND:
				case JP_MEMBER:
				case JP_ELEMENTS_FIRST:
				case JP_ELEMENTS_APPEND:
				default: break;
			}
			return 0;
		}
		void on_accept() override { }
		int on_error(uintptr_t id,typename syntax::parser_listener<json_symbol,json_production>::error_type type,int state,json_symbol word) override {
			static_cast<void>(type);
			static_cast<void>(state);
			static_cast<void>(word);
			failed_=true;
			if (sax_ && tokens_ && id!=static_cast<uintptr_t>(-1) && id>=1 && id<=tokens_->size()) {
				const json_token& token=(*tokens_)[id-1];
				const std::string text(token.string.begin(),token.string.end());
				sax_->parse_error(token.position,text,"Unexpected token");
			} else if (sax_) sax_->parse_error(0,std::string(),"Unexpected end of input");
			return 0;
		}
	};

public:
	static bool sax_parse(std::string_view input,sax_t* sax) {
		std::vector<json_token> tokens;
		std::size_t error_position=0;
		std::string error_message;
		if (!tokenize(input,tokens,error_position,error_message)) {
			sax->parse_error(error_position,std::string(),error_message);
			return false;
		}
		if (tokens.size()==1) {
			sax->parse_error(0,std::string(),"Attempting to parse an empty input");
			return false;
		}
		std::vector<typename parser_t::parse_node> nodes;
		nodes.reserve(tokens.size());
		for (const auto& it:tokens) {
			typename parser_t::parse_node node;
			node.op=it.symbol;
			nodes.push_back(std::move(node));
		}
		std::lock_guard<std::mutex> lock(grammar_mutex());
		parser_t& parser=grammar();
		static json_listener listener;
		listener.reset(tokens,*sax);
		parser.listeners.push_back(&listener);
		bool result=false;
		try {
			result=parser.parse_with_listener(nodes);
		} catch (...) {
			parser.listeners.pop_back();
			throw;
		}
		parser.listeners.pop_back();
		return result && !listener.aborted() && !listener.failed();
	}
	static json parse(std::string_view input,bool allow_exceptions=true) {
		json result;
		json_sax_dom_builder<json> builder(result);
		const bool ok=sax_parse(input,&builder) && builder.completed();
		if (!ok) {
			if (allow_exceptions) throw std::runtime_error(std::string("Parse error at byte ")+std::to_string(builder.error_position())+std::string(": ")+(builder.error_message().empty()?std::string("Incomplete document"):builder.error_message()));
			return json();
		}
		return result;
	}
	static bool try_parse(std::string_view input,json& out) {
		json result;
		json_sax_dom_builder<json> builder(result);
		if (!sax_parse(input,&builder) || !builder.completed()) return false;
		out=std::move(result);
		return true;
	}
	static bool accept(std::string_view input) {
		json_sax_acceptor<json> acceptor;
		return sax_parse(input,&acceptor);
	}

private:
	static void dump_hex16(string_t& out,unsigned long cp) {
		static const char digits[]="0123456789abcdef";
		out.push_back('\\');
		out.push_back('u');
		out.push_back(digits[(cp>>12)&0xF]);
		out.push_back(digits[(cp>>8)&0xF]);
		out.push_back(digits[(cp>>4)&0xF]);
		out.push_back(digits[cp&0xF]);
	}
	static void dump_escaped(string_t& out,const string_t& s,bool ensure_ascii) {
		const unsigned char* first=reinterpret_cast<const unsigned char*>(s.data());
		const unsigned char* const last=first+s.size();
		while (first<last) {
			const unsigned char c=*first;
			switch (c) {
				case '"': out.push_back('\\');out.push_back('"');first++;continue;
				case '\\': out.push_back('\\');out.push_back('\\');first++;continue;
				case '\b': out.push_back('\\');out.push_back('b');first++;continue;
				case '\f': out.push_back('\\');out.push_back('f');first++;continue;
				case '\n': out.push_back('\\');out.push_back('n');first++;continue;
				case '\r': out.push_back('\\');out.push_back('r');first++;continue;
				case '\t': out.push_back('\\');out.push_back('t');first++;continue;
				default: break;
			}
			if (c<0x20) {
				dump_hex16(out,c);
				first++;
				continue;
			}
			if (c<0x80) {
				out.push_back(static_cast<typename string_t::value_type>(c));
				first++;
				continue;
			}
			unsigned long cp=0;
			std::size_t extra=0;
			if ((c&0xE0)==0xC0) {
				cp=c&0x1F;
				extra=1;
			} else if ((c&0xF0)==0xE0) {
				cp=c&0x0F;
				extra=2;
			} else if ((c&0xF8)==0xF0) {
				cp=c&0x07;
				extra=3;
			} else throw std::invalid_argument("Invalid UTF-8 byte at index "+std::to_string(first-reinterpret_cast<const unsigned char*>(s.data())));
			if (static_cast<std::size_t>(last-first)<extra+1) throw std::invalid_argument("Truncated UTF-8 sequence");
			for (std::size_t i=1;i<=extra;i++) {
				if ((first[i]&0xC0)!=0x80) throw std::invalid_argument("Invalid UTF-8 continuation byte");
				cp=(cp<<6)|(first[i]&0x3F);
			}
			if (!ensure_ascii) {
				for (std::size_t i=0;i<=extra;i++) out.push_back(static_cast<typename string_t::value_type>(first[i]));
			} else if (cp<0x10000) {
				dump_hex16(out,cp);
			} else {
				cp-=0x10000;
				dump_hex16(out,0xD800+(cp>>10));
				dump_hex16(out,0xDC00+(cp&0x3FF));
			}
			first+=extra+1;
		}
	}
	static void dump_integer(string_t& out,int_t value) {
		const std::string text=std::to_string(static_cast<long long>(value));
		out.append(text.begin(),text.end());
	}
	static void dump_floating(string_t& out,float_t value) {
		if (std::isnan(value) || std::isinf(value)) {
			out.append({'n','u','l','l'});
			return;
		}
		char buffer[64];
		int length=std::snprintf(buffer,sizeof(buffer),"%.15g",static_cast<double>(value));
		if (std::strtod(buffer,nullptr)!=static_cast<double>(value)) length=std::snprintf(buffer,sizeof(buffer),"%.17g",static_cast<double>(value));
		bool needs_dot=true;
		for (int i=0;i<length;i++) {
			if (buffer[i]=='.' || buffer[i]=='e' || buffer[i]=='E') {
				needs_dot=false;
				break;
			}
		}
		out.append(buffer,buffer+length);
		if (needs_dot) {
			out.push_back('.');
			out.push_back('0');
		}
	}
	static void dump_indent(string_t& out,std::size_t count,typename string_t::value_type indent_char) {
		for (std::size_t i=0;i<count;i++) out.push_back(indent_char);
	}
	static void dump_internal(const base_t& node,string_t& out,int indent_step,typename string_t::value_type indent_char,bool ensure_ascii,std::size_t current_indent) {
		const bool pretty=indent_step>0;
		const std::size_t child_indent=current_indent+(pretty?static_cast<std::size_t>(indent_step):0);
		switch (node.type()) {
			case structure::DDT_NULL: {
				out.append({'n','u','l','l'});
				break;
			}
			case structure::DDT_BOOL: {
				if (*node.template get_ptr<const boolean_t*>()) out.append({'t','r','u','e'});
				else out.append({'f','a','l','s','e'});
				break;
			}
			case structure::DDT_INT: {
				dump_integer(out,*node.template get_ptr<const int_t*>());
				break;
			}
			case structure::DDT_FLOAT: {
				dump_floating(out,*node.template get_ptr<const float_t*>());
				break;
			}
			case structure::DDT_STRING: {
				out.push_back('"');
				dump_escaped(out,*node.template get_ptr<const string_t*>(),ensure_ascii);
				out.push_back('"');
				break;
			}
			case structure::DDT_ARRAY: {
				if (node.empty()) {
					out.push_back('[');
					out.push_back(']');
					break;
				}
				out.push_back('[');
				if (pretty) out.push_back('\n');
				for (auto it=node.cbegin();it!=node.cend();) {
					if (pretty) dump_indent(out,child_indent,indent_char);
					dump_internal(*it,out,indent_step,indent_char,ensure_ascii,child_indent);
					it++;
					if (it!=node.cend()) out.push_back(',');
					if (pretty) out.push_back('\n');
				}
				if (pretty) dump_indent(out,current_indent,indent_char);
				out.push_back(']');
				break;
			}
			case structure::DDT_OBJECT: {
				if (node.empty()) {
					out.push_back('{');
					out.push_back('}');
					break;
				}
				out.push_back('{');
				if (pretty) out.push_back('\n');
				for (auto it=node.cbegin();it!=node.cend();) {
					if (pretty) dump_indent(out,child_indent,indent_char);
					out.push_back('"');
					dump_escaped(out,it.key(),ensure_ascii);
					out.push_back('"');
					out.push_back(':');
					if (pretty) out.push_back(' ');
					dump_internal(*it,out,indent_step,indent_char,ensure_ascii,child_indent);
					it++;
					if (it!=node.cend()) out.push_back(',');
					if (pretty) out.push_back('\n');
				}
				if (pretty) dump_indent(out,current_indent,indent_char);
				out.push_back('}');
				break;
			}
			default: break;
		}
	}
 
public:
	virtual string_t dump(int indent=-1,typename string_t::value_type indent_char=' ',bool ensure_ascii=false) const {
		string_t result;
		dump_internal(*this,result,indent,indent_char,ensure_ascii,0);
		return result;
	}

	friend std::ostream& operator <<(std::ostream& os,const json& value) {
		const int indent_step=static_cast<int>(os.width());
		os.width(0);
		const string_t text=value.dump(indent_step>0?indent_step:-1,static_cast<typename string_t::value_type>(os.fill()));
		os.write(reinterpret_cast<const char*>(text.data()),static_cast<std::streamsize>(text.size()));
		return os;
	}
	friend std::istream& operator >>(std::istream& is,json& value) {
		std::string content((std::istreambuf_iterator<char>(is)),std::istreambuf_iterator<char>());
		value=parse(content);
		return is;
	}
};
 
_STDEX_DOM_TPL_DEFAULT_DECLARATION
inline typename json<_Int,_Float,_Boolean,_String,_Array,_Object,_Allocator>::string_t to_string(const json<_Int,_Float,_Boolean,_String,_Array,_Object,_Allocator>& value) {
	return value.dump();
}
 
}
 
_STDEX_DOM_TPL_DEFAULT_DECLARATION
using json_t=basic_json::json<_Int,_Float,_Boolean,_String,_Array,_Object,_Allocator>;
using json=json_t<>;
using basic_json::json_sax;
using basic_json::json_sax_dom_builder;
using basic_json::json_sax_acceptor;
using basic_json::to_string;
 
_STDEX_DOM_TPL_DEFAULT_DECLARATION
using json_path_t=basic_json::json_path<json_t<_Int,_Float,_Boolean,_String,_Array,_Object,_Allocator>>;
using json_path=json_path_t<>;
 
inline namespace literals {
 
inline json_t<> operator ""_json(const char* s,std::size_t n) {
	return json_t<>::parse(std::string_view(s,n));
}

inline structure::dom_pointer<std::string> operator ""_json_pointer(const char* s,std::size_t n) {
	return structure::dom_pointer<std::string>(std::string(s,n));
}

inline json_path_t<> operator ""_json_path(const char* s,std::size_t n) {
	return json_path_t<>(std::string_view(s,n));
}
 
}
 
}
 
}
 
#endif