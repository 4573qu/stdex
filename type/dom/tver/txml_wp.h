//Last Modified At 2026/06/12
//@Version 1.0.0.0
#ifndef _STDEX_TYPE_DOM_XML_H_
#define _STDEX_TYPE_DOM_XML_H_ 1

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <istream>
#include <limits>
#include <memory>
#include <mutex>
#include <ostream>
#include <regex>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "../../structure/dom.h"//At Least 1.0
#include "../../syntax/parser.h"//At Least 3.4
#include "../../utility/kind.h"//At Least 1.4

namespace stdex {

namespace type {

namespace basic_xml {

_STDEX_DERIVED_KIND(xml_data_type,structure::dom_data_type,_STDEX_KIND_AUTO_START,
	_STDEX_KIND_VALUE_AUTO(XDT_ELEMENT)
	_STDEX_KIND_VALUE_AUTO(XDT_CDATA)
	_STDEX_KIND_VALUE_AUTO(XDT_COMMENT)
	_STDEX_KIND_VALUE_AUTO(XDT_PROCINST)
)

enum xml_symbol : int {
	XS_EPSILON,
	XS_EOF,
	XS_LT,
	XS_ETAG_OPEN,
	XS_GT,
	XS_EMPTY_CLOSE,
	XS_NAME,
	XS_EQ,
	XS_ATTVALUE,
	XS_TEXT,
	XS_CDATA,
	XS_COMMENT,
	XS_PI,
	XS_XMLDECL,
	XS_DOCTYPE,
	XS_START,
	XS_DOCUMENT,
	XS_ITEM_SEQ,
	XS_DOC_ITEM,
	XS_ELEMENT,
	XS_EMPTY_ELEM,
	XS_STAG,
	XS_ETAG,
	XS_ATTR_SEQ,
	XS_ATTRIBUTE,
	XS_CONTENT,
	XS_CONTENT_ITEM,
};

enum xml_production : int {
	XP_START,
	XP_DOCUMENT,
	XP_ITEMS_FIRST,
	XP_ITEMS_APPEND,
	XP_ITEM_DECL,
	XP_ITEM_DOCTYPE,
	XP_ITEM_COMMENT,
	XP_ITEM_PI,
	XP_ITEM_TEXT,
	XP_ITEM_ELEMENT,
	XP_ELEMENT_EMPTY,
	XP_ELEMENT_BLANK,
	XP_ELEMENT,
	XP_EMPTY_ELEM_PLAIN,
	XP_EMPTY_ELEM_ATTRS,
	XP_STAG_PLAIN,
	XP_STAG_ATTRS,
	XP_ETAG,
	XP_ATTR_FIRST,
	XP_ATTR_APPEND,
	XP_ATTRIBUTE,
	XP_CONTENT_FIRST,
	XP_CONTENT_APPEND,
	XP_CITEM_ELEMENT,
	XP_CITEM_TEXT,
	XP_CITEM_CDATA,
	XP_CITEM_COMMENT,
	XP_CITEM_PI,
};

template <typename _String>
struct basic_xml_document_info {
	_String version{};
	_String encoding{};
	int standalone=-1;
	_String doctype{};

	bool has_declaration() const noexcept {
		return !version.empty();
	}
	bool has_doctype() const noexcept {
		return !doctype.empty();
	}
};

template <typename _Xml>
struct xml_sax {
	using string_t=typename _Xml::string_t;

	virtual bool declaration(string_t& version,string_t& encoding,int standalone)=0;
	virtual bool doctype(string_t& text)=0;
	virtual bool start_element(string_t& name)=0;
	virtual bool attribute(string_t& name,string_t& value)=0;
	virtual bool end_element()=0;
	virtual bool characters(string_t& text)=0;
	virtual bool cdata(string_t& text)=0;
	virtual bool comment(string_t& text)=0;
	virtual bool processing_instruction(string_t& target,string_t& data)=0;
	virtual bool parse_error(std::size_t position,const std::string& last_token,const std::string& message)=0;
	virtual ~xml_sax()=default;
};

template <typename _Xml>
class xml_sax_dom_builder : public xml_sax<_Xml> {
public:
	using string_t=typename _Xml::string_t;
	using base_t=typename _Xml::base_t;
	using document_info_t=typename _Xml::document_info_t;

private:
	_Xml& root_;
	document_info_t* info_=nullptr;
	std::vector<base_t*> ref_stack_;
	bool preserve_whitespace_=false;
	bool root_set_=false;
	bool errored_=false;
	std::size_t error_position_=0;
	std::string error_message_;

	static bool whitespace_only(const string_t& text) noexcept {
		for (auto it:text) {
			if (it!=' ' && it!='\t' && it!='\n' && it!='\r') return false;
		}
		return true;
	}
	base_t* handle_node(base_t&& node) {
		if (ref_stack_.empty()) {
			if (node.type()!=xml_data_type(XDT_ELEMENT)) return nullptr;
			root_=_Xml(std::move(node));
			root_set_=true;
			return static_cast<base_t*>(&root_);
		}
		typename base_t::array_t& parent=_Xml::children(*ref_stack_.back());
		parent.push_back(std::move(node));
		return &parent.back();
	}

public:
	explicit xml_sax_dom_builder(_Xml& root,document_info_t* info=nullptr,bool preserve_whitespace=false) : root_(root) , info_(info) , preserve_whitespace_(preserve_whitespace) { }
	bool declaration(string_t& version,string_t& encoding,int standalone) override {
		if (info_) {
			info_->version=std::move(version);
			info_->encoding=std::move(encoding);
			info_->standalone=standalone;
		}
		return true;
	}
	bool doctype(string_t& text) override {
		if (info_) info_->doctype=std::move(text);
		return true;
	}
	bool start_element(string_t& name) override {
		base_t* node=handle_node(_Xml::make_element(std::move(name)));
		if (!node) return true;
		ref_stack_.push_back(node);
		return true;
	}
	bool attribute(string_t& name,string_t& value) override {
		if (ref_stack_.empty()) return true;
		_Xml::attributes(*ref_stack_.back()).emplace(std::move(name),base_t(std::move(value)));
		return true;
	}
	bool end_element() override {
		if (!ref_stack_.empty()) ref_stack_.pop_back();
		return true;
	}
	bool characters(string_t& text) override {
		if (ref_stack_.empty()) return true;
		if (!preserve_whitespace_ && whitespace_only(text)) return true;
		handle_node(base_t(std::move(text)));
		return true;
	}
	bool cdata(string_t& text) override {
		if (ref_stack_.empty()) return true;
		handle_node(_Xml::make_cdata(std::move(text)));
		return true;
	}
	bool comment(string_t& text) override {
		if (ref_stack_.empty()) return true;
		handle_node(_Xml::make_comment(std::move(text)));
		return true;
	}
	bool processing_instruction(string_t& target,string_t& data) override {
		if (ref_stack_.empty()) return true;
		handle_node(_Xml::make_processing_instruction(std::move(target),std::move(data)));
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

template <typename _Xml>
class xml_sax_acceptor : public xml_sax<_Xml> {
public:
	using string_t=typename _Xml::string_t;

	bool declaration(string_t&,string_t&,int) override { return true; }
	bool doctype(string_t&) override { return true; }
	bool start_element(string_t&) override { return true; }
	bool attribute(string_t&,string_t&) override { return true; }
	bool end_element() override { return true; }
	bool characters(string_t&) override { return true; }
	bool cdata(string_t&) override { return true; }
	bool comment(string_t&) override { return true; }
	bool processing_instruction(string_t&,string_t&) override { return true; }
	bool parse_error(std::size_t,const std::string&,const std::string&) override { return false; }
};

//XmlPath(XPath 1.0缩略语法子集)编译后查询对象。谱系与json_path一致:查询小语言用手工
//递归下降编译成不可变AST,对dom反复求值,不走文档级SLR机器;词法细粒度沿用std::regex。
//数据模型绑定xml.h的落点:文档序=children数组序;CDATA按XPath数据模型计为文本;属性不在child轴;
//string-value(元素)=后代文本递归拼接(排除注释/PI)。dom无父指针,父轴".."由求值期携带的祖先链
//提供(每个结果条目附祖先链,内存成本与深度成正比);没有文档节点对象,以虚文档条目(node=nullptr)
//统一"/"与根上".."的语义,select输出时虚文档落地为传入的根节点。
//名字测试为词法名保真(前缀原样,不解析namespace作用域,与xml.h口径一致);XPath字符串字面量按
//规范不支持转义。编译错误抛std::invalid_argument;求值不抛,select*对非节点集结果抛
//std::invalid_argument,evaluate_boolean/number/string按XPath转换规则永不抛。
//编译后对象不可变,const求值可并发。
template <typename _Xml>
class xml_path {
public:
	using base_t=typename _Xml::base_t;
	using int_t=typename _Xml::int_t;
	using float_t=typename _Xml::float_t;
	using boolean_t=typename _Xml::boolean_t;
	using string_t=typename _Xml::string_t;
	using size_type=typename _Xml::size_type;

private:
	enum axis_kind : int {
		AX_CHILD,
		AX_ATTRIBUTE,
		AX_SELF,
		AX_PARENT,
	};
	enum node_test_kind : int {
		TK_NAME,
		TK_WILDCARD,
		TK_TEXT,
		TK_COMMENT,
		TK_PI,
		TK_NODE,
	};
	enum expr_kind : int {
		EK_OR,
		EK_AND,
		EK_COMPARISON,
		EK_ARITHMETIC,
		EK_NEGATE,
		EK_UNION,
		EK_NUMBER,
		EK_STRING,
		EK_PATH,
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
	enum arithmetic_op : int {
		AO_ADD,
		AO_SUBTRACT,
		AO_MULTIPLY,
		AO_DIVIDE,
		AO_MODULO,
	};
	enum function_kind : int {
		FK_POSITION,
		FK_LAST,
		FK_COUNT,
		FK_NAME,
		FK_STRING,
		FK_NUMBER,
		FK_BOOLEAN,
		FK_NOT,
		FK_TRUE,
		FK_FALSE,
		FK_CONTAINS,
		FK_STARTS_WITH,
		FK_STRING_LENGTH,
		FK_NORMALIZE_SPACE,
		FK_CONCAT,
	};
	//XPath 1.0四类型系统。
	enum value_kind : int {
		VK_NODESET,
		VK_BOOLEAN,
		VK_NUMBER,
		VK_STRING,
	};

	struct step {
		bool descendant=false;//前置"//":descendant-or-self::node()展开
		axis_kind axis=AX_CHILD;
		node_test_kind test=TK_NAME;
		string_t name{};//名字测试或processing-instruction目标
		bool has_target=false;
		std::vector<int> predicates;
	};
	struct location_path {
		bool absolute=false;
		std::vector<step> steps;
	};
	struct expr_node {
		expr_kind kind=EK_NUMBER;
		comparison_op comparison=CO_EQUAL;
		arithmetic_op arithmetic=AO_ADD;
		function_kind function=FK_TRUE;
		std::vector<int> children;
		double number=0;
		string_t text{};
		location_path path{};
	};
	//结果条目:node=nullptr表示虚文档节点;attribute条目指向attributes映射中的值dom并携带键名;
	//ancestors自外向内(front为虚文档nullptr),父轴与string-value都依赖它。
	struct entry {
		const base_t* node=nullptr;
		bool attribute=false;
		string_t attribute_name{};
		std::vector<const base_t*> ancestors{};
	};
	struct xpath_value {
		value_kind kind=VK_BOOLEAN;
		std::vector<entry> nodes{};
		bool truth=false;
		double number=0;
		string_t text{};
	};

	std::string expression_;
	std::vector<expr_node> pool_;
	int root_expr_=-1;

	//---词法细粒度(名字正则与xml.h词法同一条口径:XML NameChar的ASCII子集近似)---
	static const std::regex& name_regex() {
		static const std::regex result(R"([:A-Za-z_][:A-Za-z0-9._\-]*)",std::regex::optimize);
		return result;
	}
	static const std::regex& number_regex() {
		static const std::regex result(R"([0-9]+(?:\.[0-9]*)?|\.[0-9]+)",std::regex::optimize);
		return result;
	}
	//XPath的number()文法:可选空白与负号+十进制(无指数)。
	static const std::regex& numeric_string_regex() {
		static const std::regex result(R"([ \t\r\n]*-?(?:[0-9]+(?:\.[0-9]*)?|\.[0-9]+)[ \t\r\n]*)",std::regex::optimize);
		return result;
	}

	[[noreturn]]
	void fail(std::size_t position,const std::string& message) const {
		throw std::invalid_argument(message+" at index "+std::to_string(position)+" in XmlPath '"+expression_+"'");
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
	static bool is_name_char(char c) noexcept {
		return (c>='A' && c<='Z') || (c>='a' && c<='z') || (c>='0' && c<='9') || c=='_' || c=='.' || c=='-' || c==':';
	}
	//关键字算符(or/and/div/mod)须整词匹配。
	bool match_keyword(std::size_t& pos,const char* text) const noexcept {
		const std::size_t length=std::strlen(text);
		if (expression_.size()-pos<length || std::memcmp(expression_.data()+pos,text,length)!=0) return false;
		if (expression_.size()-pos>length && is_name_char(expression_[pos+length])) return false;
		pos+=length;
		return true;
	}
	bool match_literal(std::size_t& pos,const char* text) const noexcept {
		const std::size_t length=std::strlen(text);
		if (expression_.size()-pos<length || std::memcmp(expression_.data()+pos,text,length)!=0) return false;
		pos+=length;
		return true;
	}
	//XPath 1.0字符串字面量:到同型引号为止,规范不含转义。
	string_t parse_string_literal(std::size_t& pos) const {
		const char quote=expression_[pos];
		const std::size_t start=pos;
		pos++;
		string_t result;
		while (true) {
			if (pos>=expression_.size()) fail(start,"Unterminated string literal");
			const char c=expression_[pos++];
			if (c==quote) return result;
			result.push_back(c);
		}
	}

	int make_node(expr_node&& node) {
		pool_.push_back(std::move(node));
		return static_cast<int>(pool_.size()-1);
	}
	void require_node_set(int index,std::size_t position,const char* what) const {
		const expr_kind kind=pool_[index].kind;
		if (kind!=EK_PATH && kind!=EK_UNION) fail(position,std::string(what)+" requires a node set operand");
	}

	//---表达式文法:or<and<等值<关系<加减<乘除模<一元负<联合<初等式/路径---
	void compile() {
		if (expression_.empty()) throw std::invalid_argument("XmlPath must not be empty");
		std::size_t pos=0;
		root_expr_=parse_or(pos);
		skip_blank(pos);
		if (pos!=expression_.size()) fail(pos,std::string("Unexpected character '")+expression_[pos]+"'");
	}
	int parse_or(std::size_t& pos) {
		int left=parse_and(pos);
		while (true) {
			skip_blank(pos);
			if (!match_keyword(pos,"or")) return left;
			const int right=parse_and(pos);
			expr_node node;
			node.kind=EK_OR;
			node.children={left,right};
			left=make_node(std::move(node));
		}
	}
	int parse_and(std::size_t& pos) {
		int left=parse_equality(pos);
		while (true) {
			skip_blank(pos);
			if (!match_keyword(pos,"and")) return left;
			const int right=parse_equality(pos);
			expr_node node;
			node.kind=EK_AND;
			node.children={left,right};
			left=make_node(std::move(node));
		}
	}
	int parse_equality(std::size_t& pos) {
		int left=parse_relational(pos);
		while (true) {
			skip_blank(pos);
			comparison_op op;
			if (match_literal(pos,"!=")) op=CO_NOT_EQUAL;
			else if (match_literal(pos,"=")) op=CO_EQUAL;
			else return left;
			const int right=parse_relational(pos);
			expr_node node;
			node.kind=EK_COMPARISON;
			node.comparison=op;
			node.children={left,right};
			left=make_node(std::move(node));
		}
	}
	int parse_relational(std::size_t& pos) {
		int left=parse_additive(pos);
		while (true) {
			skip_blank(pos);
			comparison_op op;
			if (match_literal(pos,"<=")) op=CO_LESS_EQUAL;
			else if (match_literal(pos,">=")) op=CO_GREATER_EQUAL;
			else if (match_literal(pos,"<")) op=CO_LESS;
			else if (match_literal(pos,">")) op=CO_GREATER;
			else return left;
			const int right=parse_additive(pos);
			expr_node node;
			node.kind=EK_COMPARISON;
			node.comparison=op;
			node.children={left,right};
			left=make_node(std::move(node));
		}
	}
	int parse_additive(std::size_t& pos) {
		int left=parse_multiplicative(pos);
		while (true) {
			skip_blank(pos);
			arithmetic_op op;
			if (match_literal(pos,"+")) op=AO_ADD;
			else if (match_literal(pos,"-")) op=AO_SUBTRACT;
			else return left;
			const int right=parse_multiplicative(pos);
			expr_node node;
			node.kind=EK_ARITHMETIC;
			node.arithmetic=op;
			node.children={left,right};
			left=make_node(std::move(node));
		}
	}
	int parse_multiplicative(std::size_t& pos) {
		int left=parse_unary(pos);
		while (true) {
			skip_blank(pos);
			arithmetic_op op;
			if (match_literal(pos,"*")) op=AO_MULTIPLY;
			else if (match_keyword(pos,"div")) op=AO_DIVIDE;
			else if (match_keyword(pos,"mod")) op=AO_MODULO;
			else return left;
			const int right=parse_unary(pos);
			expr_node node;
			node.kind=EK_ARITHMETIC;
			node.arithmetic=op;
			node.children={left,right};
			left=make_node(std::move(node));
		}
	}
	int parse_unary(std::size_t& pos) {
		skip_blank(pos);
		if (pos<expression_.size() && expression_[pos]=='-') {
			pos++;
			const int child=parse_unary(pos);
			expr_node node;
			node.kind=EK_NEGATE;
			node.children={child};
			return make_node(std::move(node));
		}
		return parse_union(pos);
	}
	int parse_union(std::size_t& pos) {
		const std::size_t left_position=pos;
		int left=parse_primary(pos);
		while (true) {
			skip_blank(pos);
			if (pos>=expression_.size() || expression_[pos]!='|') return left;
			require_node_set(left,left_position,"'|'");
			pos++;
			const std::size_t right_position=pos;
			const int right=parse_primary(pos);
			require_node_set(right,right_position,"'|'");
			expr_node node;
			node.kind=EK_UNION;
			node.children={left,right};
			left=make_node(std::move(node));
		}
	}
	static bool is_node_type_name(const std::string& name) noexcept {
		return name=="text" || name=="comment" || name=="node" || name=="processing-instruction";
	}
	int parse_primary(std::size_t& pos) {
		skip_blank(pos);
		if (pos>=expression_.size()) fail(pos,"Unexpected end of expression");
		const char c=expression_[pos];
		if (c=='(') {//括号表达式(子集边界:括号后不支持继续路径步)
			pos++;
			const int inner=parse_or(pos);
			skip_blank(pos);
			if (pos>=expression_.size() || expression_[pos]!=')') fail(pos,"Expected ')'");
			pos++;
			return inner;
		}
		if (c=='\'' || c=='"') {
			expr_node node;
			node.kind=EK_STRING;
			node.text=parse_string_literal(pos);
			return make_node(std::move(node));
		}
		const bool leading_digit=(c>='0' && c<='9');
		const bool leading_point_digit=(c=='.' && pos+1<expression_.size() && expression_[pos+1]>='0' && expression_[pos+1]<='9');
		if (leading_digit || leading_point_digit) {
			std::string text;
			match_regex(pos,number_regex(),text);
			expr_node node;
			node.kind=EK_NUMBER;
			node.number=std::strtod(text.c_str(),nullptr);
			return make_node(std::move(node));
		}
		if (c=='/' || c=='@' || c=='.' || c=='*') return parse_path_node(pos);
		{
			const std::size_t mark=pos;
			std::string name;
			if (match_regex(pos,name_regex(),name)) {
				std::size_t probe=pos;
				skip_blank(probe);
				const bool call=probe<expression_.size() && expression_[probe]=='(';
				pos=mark;
				if (call && !is_node_type_name(name)) return parse_function(pos);
				return parse_path_node(pos);//名字测试步(含节点类型测试)
			}
		}
		fail(pos,std::string("Unexpected character '")+c+"'");
	}
	int parse_path_node(std::size_t& pos) {
		expr_node node;
		node.kind=EK_PATH;
		node.path=parse_location_path(pos);
		return make_node(std::move(node));
	}
	location_path parse_location_path(std::size_t& pos) {
		location_path result;
		bool pending_descendant=false;
		if (match_literal(pos,"//")) {
			result.absolute=true;
			pending_descendant=true;
		} else if (match_literal(pos,"/")) {
			result.absolute=true;
			skip_blank(pos);
			if (pos>=expression_.size() || !is_step_start(expression_[pos])) return result;//"/"单独:文档根
		}
		while (true) {
			result.steps.push_back(parse_step(pos,pending_descendant));
			pending_descendant=false;
			const std::size_t mark=pos;
			skip_blank(pos);
			if (match_literal(pos,"//")) {
				pending_descendant=true;
				continue;
			}
			if (match_literal(pos,"/")) continue;
			pos=mark;
			return result;
		}
	}
	static bool is_step_start(char c) noexcept {
		return c=='@' || c=='.' || c=='*' || c==':' || c=='_' || (c>='A' && c<='Z') || (c>='a' && c<='z');
	}
	step parse_step(std::size_t& pos,bool descendant) {
		skip_blank(pos);
		if (pos>=expression_.size()) fail(pos,"Expected a location step");
		step result;
		result.descendant=descendant;
		const char c=expression_[pos];
		if (c=='.') {
			pos++;
			if (pos<expression_.size() && expression_[pos]=='.') {
				pos++;
				result.axis=AX_PARENT;
			} else result.axis=AX_SELF;
			result.test=TK_NODE;
		} else if (c=='@') {
			pos++;
			skip_blank(pos);
			result.axis=AX_ATTRIBUTE;
			if (pos<expression_.size() && expression_[pos]=='*') {
				pos++;
				result.test=TK_WILDCARD;
			} else {
				std::string name;
				if (!match_regex(pos,name_regex(),name)) fail(pos,"Expected an attribute name or '*' after '@'");
				result.test=TK_NAME;
				result.name=string_t(name.begin(),name.end());
			}
		} else if (c=='*') {
			pos++;
			result.test=TK_WILDCARD;
		} else {
			std::string name;
			if (!match_regex(pos,name_regex(),name)) fail(pos,std::string("Unexpected character '")+c+"' in location step");
			std::size_t probe=pos;
			skip_blank(probe);
			if (is_node_type_name(name) && probe<expression_.size() && expression_[probe]=='(') {
				pos=probe+1;
				skip_blank(pos);
				if (name=="processing-instruction") {
					if (pos<expression_.size() && (expression_[pos]=='\'' || expression_[pos]=='"')) {
						result.name=parse_string_literal(pos);
						result.has_target=true;
						skip_blank(pos);
					}
					result.test=TK_PI;
				} else if (name=="text") result.test=TK_TEXT;
				else if (name=="comment") result.test=TK_COMMENT;
				else result.test=TK_NODE;
				if (pos>=expression_.size() || expression_[pos]!=')') fail(pos,"Expected ')' in node type test");
				pos++;
			} else {
				result.test=TK_NAME;
				result.name=string_t(name.begin(),name.end());
			}
		}
		while (true) {//谓词序列
			const std::size_t mark=pos;
			skip_blank(pos);
			if (pos>=expression_.size() || expression_[pos]!='[') {
				pos=mark;
				return result;
			}
			pos++;
			result.predicates.push_back(parse_or(pos));
			skip_blank(pos);
			if (pos>=expression_.size() || expression_[pos]!=']') fail(pos,"Expected ']'");
			pos++;
		}
	}
	int parse_function(std::size_t& pos) {
		skip_blank(pos);
		const std::size_t name_position=pos;
		std::string name;
		match_regex(pos,name_regex(),name);
		function_kind function;
		std::size_t minimum_arguments=0;
		std::size_t maximum_arguments=0;
		if (name=="position") function=FK_POSITION;
		else if (name=="last") function=FK_LAST;
		else if (name=="count") {
			function=FK_COUNT;
			minimum_arguments=1;
			maximum_arguments=1;
		} else if (name=="name") {
			function=FK_NAME;
			maximum_arguments=1;
		} else if (name=="string") {
			function=FK_STRING;
			maximum_arguments=1;
		} else if (name=="number") {
			function=FK_NUMBER;
			maximum_arguments=1;
		} else if (name=="boolean") {
			function=FK_BOOLEAN;
			minimum_arguments=1;
			maximum_arguments=1;
		} else if (name=="not") {
			function=FK_NOT;
			minimum_arguments=1;
			maximum_arguments=1;
		} else if (name=="true") function=FK_TRUE;
		else if (name=="false") function=FK_FALSE;
		else if (name=="contains") {
			function=FK_CONTAINS;
			minimum_arguments=2;
			maximum_arguments=2;
		} else if (name=="starts-with") {
			function=FK_STARTS_WITH;
			minimum_arguments=2;
			maximum_arguments=2;
		} else if (name=="string-length") {
			function=FK_STRING_LENGTH;
			maximum_arguments=1;
		} else if (name=="normalize-space") {
			function=FK_NORMALIZE_SPACE;
			maximum_arguments=1;
		} else if (name=="concat") {
			function=FK_CONCAT;
			minimum_arguments=2;
			maximum_arguments=static_cast<std::size_t>(-1);
		} else fail(name_position,"Unknown function '"+name+"'");
		skip_blank(pos);
		if (pos>=expression_.size() || expression_[pos]!='(') fail(pos,"Expected '(' after function name");
		pos++;
		std::vector<int> arguments;
		skip_blank(pos);
		if (pos<expression_.size() && expression_[pos]==')') pos++;
		else {
			while (true) {
				arguments.push_back(parse_or(pos));
				skip_blank(pos);
				if (pos>=expression_.size()) fail(pos,"Unterminated function argument list");
				const char c=expression_[pos];
				pos++;
				if (c==')') break;
				if (c!=',') fail(pos-1,std::string("Expected ',' or ')' but found '")+c+"'");
			}
		}
		if (arguments.size()<minimum_arguments || arguments.size()>maximum_arguments) fail(name_position,name+"() argument count is invalid");
		if (function==FK_COUNT) require_node_set(arguments[0],name_position,"count()");
		expr_node node;
		node.kind=EK_FUNCTION;
		node.function=function;
		node.children=std::move(arguments);
		return make_node(std::move(node));
	}

	//---XPath数据模型与转换---
	static entry document_entry() {
		return entry();
	}
	static entry root_entry(const base_t& root) {
		entry result;
		result.node=&root;
		result.ancestors.push_back(nullptr);
		return result;
	}
	static bool is_text_node(const base_t& node) noexcept {
		return node.type()==structure::DDT_STRING || _Xml::is_cdata(node);
	}
	static void scalar_text(const base_t& node,string_t& out) {
		switch (static_cast<int>(node.type().get())) {
			case static_cast<int>(structure::DDT_STRING): out+=*node.template get_ptr<const string_t*>();break;
			case static_cast<int>(structure::DDT_INT): {
				const std::string text=std::to_string(static_cast<long long>(*node.template get_ptr<const int_t*>()));
				out.append(text.begin(),text.end());
				break;
			}
			case static_cast<int>(structure::DDT_FLOAT): out+=format_number(static_cast<double>(*node.template get_ptr<const float_t*>()));break;
			case static_cast<int>(structure::DDT_BOOL): {
				if (*node.template get_ptr<const boolean_t*>()) out.append({'t','r','u','e'});
				else out.append({'f','a','l','s','e'});
				break;
			}
			default: break;
		}
	}
	static void element_text(const base_t& node,string_t& out) {
		for (const auto& it:_Xml::children(node)) {
			if (it.type()==structure::DDT_STRING) out+=*it.template get_ptr<const string_t*>();
			else if (_Xml::is_cdata(it)) out+=_Xml::text_content(it);
			else if (_Xml::is_element(it)) element_text(it,out);
		}
	}
	string_t string_value(const entry& item,const base_t& root) const {
		string_t result;
		if (!item.node) {
			if (_Xml::is_element(root)) element_text(root,result);
			else scalar_text(root,result);
			return result;
		}
		const base_t& node=*item.node;
		if (item.attribute) scalar_text(node,result);
		else if (_Xml::is_element(node)) element_text(node,result);
		else if (_Xml::is_cdata(node) || _Xml::is_comment(node)) result=_Xml::text_content(node);
		else if (_Xml::is_processing_instruction(node)) result=_Xml::pi_data(node);
		else scalar_text(node,result);
		return result;
	}
	static double string_to_number(const string_t& text) {
		const std::string narrow(text.begin(),text.end());
		if (!std::regex_match(narrow,numeric_string_regex())) return std::numeric_limits<double>::quiet_NaN();
		return std::strtod(narrow.c_str(),nullptr);
	}
	//XPath的string(number):NaN/±Infinity字面,整数不带小数点,其余最短往返(%.15g/%.17g)。
	static string_t format_number(double value) {
		string_t result;
		if (std::isnan(value)) {
			result.append({'N','a','N'});
			return result;
		}
		if (std::isinf(value)) {
			if (value<0) result.push_back('-');
			result.append({'I','n','f','i','n','i','t','y'});
			return result;
		}
		if (value==0) {
			result.push_back('0');
			return result;
		}
		if (value==static_cast<double>(static_cast<long long>(value))) {
			const std::string text=std::to_string(static_cast<long long>(value));
			result.append(text.begin(),text.end());
			return result;
		}
		char buffer[64];
		int length=std::snprintf(buffer,sizeof(buffer),"%.15g",value);
		if (std::strtod(buffer,nullptr)!=value) length=std::snprintf(buffer,sizeof(buffer),"%.17g",value);
		result.append(buffer,buffer+length);
		return result;
	}
	double to_number(const xpath_value& value,const base_t& root) const {
		switch (value.kind) {
			case VK_NUMBER: return value.number;
			case VK_BOOLEAN: return value.truth?1.0:0.0;
			case VK_STRING: return string_to_number(value.text);
			case VK_NODESET: {
				if (value.nodes.empty()) return std::numeric_limits<double>::quiet_NaN();
				return string_to_number(string_value(value.nodes[0],root));
			}
			default: return std::numeric_limits<double>::quiet_NaN();
		}
	}
	static bool to_boolean(const xpath_value& value) noexcept {
		switch (value.kind) {
			case VK_BOOLEAN: return value.truth;
			case VK_NUMBER: return value.number!=0 && !std::isnan(value.number);
			case VK_STRING: return !value.text.empty();
			case VK_NODESET: return !value.nodes.empty();
			default: return false;
		}
	}
	string_t to_string_value(const xpath_value& value,const base_t& root) const {
		switch (value.kind) {
			case VK_STRING: return value.text;
			case VK_NUMBER: return format_number(value.number);
			case VK_BOOLEAN: {
				string_t result;
				if (value.truth) result.append({'t','r','u','e'});
				else result.append({'f','a','l','s','e'});
				return result;
			}
			case VK_NODESET: {
				if (value.nodes.empty()) return string_t();
				return string_value(value.nodes[0],root);
			}
			default: return string_t();
		}
	}

	//---轴与步求值---
	std::vector<entry> children_entries(const entry& parent,const base_t& root) const {
		std::vector<entry> result;
		if (!parent.node) {//虚文档节点:唯一子=根
			result.push_back(root_entry(root));
			return result;
		}
		if (parent.attribute || !_Xml::is_element(*parent.node)) return result;
		std::vector<const base_t*> chain=parent.ancestors;
		chain.push_back(parent.node);
		for (const auto& it:_Xml::children(*parent.node)) {
			entry child;
			child.node=&it;
			child.ancestors=chain;
			result.push_back(std::move(child));
		}
		return result;
	}
	std::vector<entry> attribute_entries(const entry& parent) const {
		std::vector<entry> result;
		if (!parent.node || parent.attribute || !_Xml::is_element(*parent.node)) return result;
		std::vector<const base_t*> chain=parent.ancestors;
		chain.push_back(parent.node);
		for (const auto& it:_Xml::attributes(*parent.node)) {
			entry attribute;
			attribute.node=&it.second;
			attribute.attribute=true;
			attribute.attribute_name=it.first;
			attribute.ancestors=chain;
			result.push_back(std::move(attribute));
		}
		return result;
	}
	void collect_descendants(const entry& item,const base_t& root,std::vector<entry>& out) const {
		out.push_back(item);
		for (auto& it:children_entries(item,root)) collect_descendants(it,root,out);
	}
	bool node_test(const entry& item,const step& current) const {
		if (current.axis==AX_ATTRIBUTE) {
			if (!item.attribute) return false;
			if (current.test==TK_WILDCARD) return true;
			return current.test==TK_NAME && item.attribute_name==current.name;
		}
		switch (current.test) {
			case TK_NODE: return true;
			case TK_NAME: return item.node && !item.attribute && _Xml::is_element(*item.node) && _Xml::name(*item.node)==current.name;
			case TK_WILDCARD: return item.node && !item.attribute && _Xml::is_element(*item.node);
			case TK_TEXT: return item.node && !item.attribute && is_text_node(*item.node);
			case TK_COMMENT: return item.node && !item.attribute && _Xml::is_comment(*item.node);
			case TK_PI: {
				if (!item.node || item.attribute || !_Xml::is_processing_instruction(*item.node)) return false;
				return !current.has_target || _Xml::pi_target(*item.node)==current.name;
			}
			default: return false;
		}
	}
	std::vector<entry> axis_results(const entry& context,const step& current,const base_t& root) const {
		std::vector<entry> result;
		switch (current.axis) {
			case AX_CHILD: {
				for (auto& it:children_entries(context,root)) {
					if (node_test(it,current)) result.push_back(std::move(it));
				}
				break;
			}
			case AX_ATTRIBUTE: {
				for (auto& it:attribute_entries(context)) {
					if (node_test(it,current)) result.push_back(std::move(it));
				}
				break;
			}
			case AX_SELF: {
				if (node_test(context,current)) result.push_back(context);
				break;
			}
			case AX_PARENT: {
				if (!context.ancestors.empty()) {
					entry parent;
					parent.node=context.ancestors.back();
					parent.ancestors.assign(context.ancestors.begin(),context.ancestors.end()-1);
					if (node_test(parent,current)) result.push_back(std::move(parent));
				}
				break;
			}
			default: break;
		}
		return result;
	}
	//谓词按XPath语义"逐上下文节点"应用:position/size取自同一上下文节点产出的候选列表。
	std::vector<entry> apply_step(const entry& context,const step& current,const base_t& root) const {
		std::vector<entry> contexts;
		if (current.descendant) collect_descendants(context,root,contexts);
		else contexts.push_back(context);
		std::vector<entry> result;
		for (const auto& it:contexts) {
			std::vector<entry> selected=axis_results(it,current,root);
			for (const int predicate:current.predicates) {
				std::vector<entry> kept;
				const std::size_t size=selected.size();
				for (std::size_t i=0;i<size;i++) {
					const xpath_value value=eval_expr(predicate,selected[i],i+1,size,root);
					const bool keep=(value.kind==VK_NUMBER)?(value.number==static_cast<double>(i+1)):to_boolean(value);
					if (keep) kept.push_back(selected[i]);
				}
				selected=std::move(kept);
			}
			for (auto& jt:selected) result.push_back(std::move(jt));
		}
		return result;
	}
	//节点集语义:按指针身份去重,保留首次出现序(前向轴下即文档序)。
	static void deduplicate(std::vector<entry>& nodes) {
		std::vector<entry> result;
		result.reserve(nodes.size());
		std::vector<const void*> seen;
		seen.reserve(nodes.size());
		for (auto& it:nodes) {
			const void* key=it.node;
			bool duplicate=false;
			for (const void* jt:seen) {
				if (jt==key) {
					duplicate=true;
					break;
				}
			}
			if (duplicate) continue;
			seen.push_back(key);
			result.push_back(std::move(it));
		}
		nodes=std::move(result);
	}
	std::vector<entry> eval_path(const location_path& path,const entry& context,const base_t& root) const {
		std::vector<entry> nodes;
		if (path.absolute) nodes.push_back(document_entry());
		else nodes.push_back(context);
		for (const auto& current:path.steps) {
			std::vector<entry> next;
			for (const auto& it:nodes) {
				for (auto& jt:apply_step(it,current,root)) next.push_back(std::move(jt));
			}
			deduplicate(next);
			nodes=std::move(next);
		}
		return nodes;
	}

	//---表达式求值(XPath 1.0比较/转换规则)---
	bool compare_strings(comparison_op op,const string_t& lhs,const string_t& rhs) const noexcept {
		switch (op) {
			case CO_EQUAL: return lhs==rhs;
			case CO_NOT_EQUAL: return lhs!=rhs;
			default: return compare_numbers(op,string_to_number(lhs),string_to_number(rhs));
		}
	}
	static bool compare_numbers(comparison_op op,double lhs,double rhs) noexcept {
		switch (op) {
			case CO_EQUAL: return lhs==rhs;
			case CO_NOT_EQUAL: return lhs!=rhs;
			case CO_LESS: return lhs<rhs;
			case CO_LESS_EQUAL: return lhs<=rhs;
			case CO_GREATER: return lhs>rhs;
			case CO_GREATER_EQUAL: return lhs>=rhs;
			default: return false;
		}
	}
	bool compare_values(comparison_op op,const xpath_value& lhs,const xpath_value& rhs,const base_t& root) const {
		if (lhs.kind==VK_NODESET && rhs.kind==VK_NODESET) {//存在语义
			for (const auto& it:lhs.nodes) {
				const string_t left=string_value(it,root);
				for (const auto& jt:rhs.nodes) {
					if (compare_strings(op,left,string_value(jt,root))) return true;
				}
			}
			return false;
		}
		if (lhs.kind==VK_NODESET || rhs.kind==VK_NODESET) {
			const bool nodeset_left=(lhs.kind==VK_NODESET);
			const xpath_value& nodeset=nodeset_left?lhs:rhs;
			const xpath_value& other=nodeset_left?rhs:lhs;
			if (other.kind==VK_BOOLEAN && (op==CO_EQUAL || op==CO_NOT_EQUAL)) {
				const bool left=nodeset_left?to_boolean(nodeset):other.truth;
				const bool right=nodeset_left?other.truth:to_boolean(nodeset);
				return compare_numbers(op,left?1.0:0.0,right?1.0:0.0);
			}
			for (const auto& it:nodeset.nodes) {
				const string_t text=string_value(it,root);
				bool matched;
				if (other.kind==VK_STRING && (op==CO_EQUAL || op==CO_NOT_EQUAL)) matched=nodeset_left?compare_strings(op,text,other.text):compare_strings(op,other.text,text);
				else {
					const double node_number=string_to_number(text);
					const double other_number=to_number(other,root);
					matched=nodeset_left?compare_numbers(op,node_number,other_number):compare_numbers(op,other_number,node_number);
				}
				if (matched) return true;
			}
			return false;
		}
		if (op==CO_EQUAL || op==CO_NOT_EQUAL) {
			if (lhs.kind==VK_BOOLEAN || rhs.kind==VK_BOOLEAN) return compare_numbers(op,to_boolean(lhs)?1.0:0.0,to_boolean(rhs)?1.0:0.0);
			if (lhs.kind==VK_NUMBER || rhs.kind==VK_NUMBER) return compare_numbers(op,to_number(lhs,root),to_number(rhs,root));
			return compare_strings(op,lhs.text,rhs.text);
		}
		return compare_numbers(op,to_number(lhs,root),to_number(rhs,root));
	}
	static std::size_t codepoint_count(const string_t& text) noexcept {
		std::size_t result=0;
		for (auto it:text) {
			if ((static_cast<unsigned char>(it)&0xC0)!=0x80) result++;
		}
		return result;
	}
	static string_t normalize_space_text(const string_t& text) {
		string_t result;
		bool pending=false;
		for (auto it:text) {
			if (it==' ' || it=='\t' || it=='\n' || it=='\r') {
				if (!result.empty()) pending=true;
				continue;
			}
			if (pending) {
				result.push_back(' ');
				pending=false;
			}
			result.push_back(it);
		}
		return result;
	}
	xpath_value eval_expr(int index,const entry& context,std::size_t position,std::size_t size,const base_t& root) const {
		const expr_node& node=pool_[index];
		xpath_value result;
		switch (node.kind) {
			case EK_OR: {
				result.kind=VK_BOOLEAN;
				result.truth=to_boolean(eval_expr(node.children[0],context,position,size,root)) || to_boolean(eval_expr(node.children[1],context,position,size,root));
				break;
			}
			case EK_AND: {
				result.kind=VK_BOOLEAN;
				result.truth=to_boolean(eval_expr(node.children[0],context,position,size,root)) && to_boolean(eval_expr(node.children[1],context,position,size,root));
				break;
			}
			case EK_COMPARISON: {
				result.kind=VK_BOOLEAN;
				result.truth=compare_values(node.comparison,eval_expr(node.children[0],context,position,size,root),eval_expr(node.children[1],context,position,size,root),root);
				break;
			}
			case EK_ARITHMETIC: {
				const double lhs=to_number(eval_expr(node.children[0],context,position,size,root),root);
				const double rhs=to_number(eval_expr(node.children[1],context,position,size,root),root);
				result.kind=VK_NUMBER;
				switch (node.arithmetic) {
					case AO_ADD: result.number=lhs+rhs;break;
					case AO_SUBTRACT: result.number=lhs-rhs;break;
					case AO_MULTIPLY: result.number=lhs*rhs;break;
					case AO_DIVIDE: result.number=lhs/rhs;break;
					case AO_MODULO: result.number=std::fmod(lhs,rhs);break;
					default: break;
				}
				break;
			}
			case EK_NEGATE: {
				result.kind=VK_NUMBER;
				result.number=-to_number(eval_expr(node.children[0],context,position,size,root),root);
				break;
			}
			case EK_UNION: {
				xpath_value left=eval_expr(node.children[0],context,position,size,root);
				xpath_value right=eval_expr(node.children[1],context,position,size,root);
				result.kind=VK_NODESET;
				result.nodes=std::move(left.nodes);
				for (auto& it:right.nodes) result.nodes.push_back(std::move(it));
				deduplicate(result.nodes);
				break;
			}
			case EK_NUMBER: {
				result.kind=VK_NUMBER;
				result.number=node.number;
				break;
			}
			case EK_STRING: {
				result.kind=VK_STRING;
				result.text=node.text;
				break;
			}
			case EK_PATH: {
				result.kind=VK_NODESET;
				result.nodes=eval_path(node.path,context,root);
				break;
			}
			case EK_FUNCTION: {
				switch (node.function) {
					case FK_POSITION: {
						result.kind=VK_NUMBER;
						result.number=static_cast<double>(position);
						break;
					}
					case FK_LAST: {
						result.kind=VK_NUMBER;
						result.number=static_cast<double>(size);
						break;
					}
					case FK_COUNT: {
						result.kind=VK_NUMBER;
						result.number=static_cast<double>(eval_expr(node.children[0],context,position,size,root).nodes.size());
						break;
					}
					case FK_NAME: {
						result.kind=VK_STRING;
						entry target=context;
						bool has_target=true;
						if (!node.children.empty()) {
							xpath_value argument=eval_expr(node.children[0],context,position,size,root);
							if (argument.nodes.empty()) has_target=false;
							else target=argument.nodes[0];
						}
						if (has_target && target.node) {
							if (target.attribute) result.text=target.attribute_name;
							else if (_Xml::is_element(*target.node)) result.text=_Xml::name(*target.node);
							else if (_Xml::is_processing_instruction(*target.node)) result.text=_Xml::pi_target(*target.node);
						}
						break;
					}
					case FK_STRING: {
						result.kind=VK_STRING;
						if (node.children.empty()) result.text=string_value(context,root);
						else result.text=to_string_value(eval_expr(node.children[0],context,position,size,root),root);
						break;
					}
					case FK_NUMBER: {
						result.kind=VK_NUMBER;
						if (node.children.empty()) result.number=string_to_number(string_value(context,root));
						else result.number=to_number(eval_expr(node.children[0],context,position,size,root),root);
						break;
					}
					case FK_BOOLEAN: {
						result.kind=VK_BOOLEAN;
						result.truth=to_boolean(eval_expr(node.children[0],context,position,size,root));
						break;
					}
					case FK_NOT: {
						result.kind=VK_BOOLEAN;
						result.truth=!to_boolean(eval_expr(node.children[0],context,position,size,root));
						break;
					}
					case FK_TRUE: {
						result.kind=VK_BOOLEAN;
						result.truth=true;
						break;
					}
					case FK_FALSE: {
						result.kind=VK_BOOLEAN;
						result.truth=false;
						break;
					}
					case FK_CONTAINS:
					case FK_STARTS_WITH: {
						const string_t haystack=to_string_value(eval_expr(node.children[0],context,position,size,root),root);
						const string_t needle=to_string_value(eval_expr(node.children[1],context,position,size,root),root);
						result.kind=VK_BOOLEAN;
						if (node.function==FK_CONTAINS) result.truth=haystack.find(needle)!=string_t::npos;
						else result.truth=haystack.size()>=needle.size() && haystack.compare(0,needle.size(),needle)==0;
						break;
					}
					case FK_STRING_LENGTH: {
						result.kind=VK_NUMBER;
						if (node.children.empty()) result.number=static_cast<double>(codepoint_count(string_value(context,root)));
						else result.number=static_cast<double>(codepoint_count(to_string_value(eval_expr(node.children[0],context,position,size,root),root)));
						break;
					}
					case FK_NORMALIZE_SPACE: {
						result.kind=VK_STRING;
						if (node.children.empty()) result.text=normalize_space_text(string_value(context,root));
						else result.text=normalize_space_text(to_string_value(eval_expr(node.children[0],context,position,size,root),root));
						break;
					}
					case FK_CONCAT: {
						result.kind=VK_STRING;
						for (const int it:node.children) result.text+=to_string_value(eval_expr(it,context,position,size,root),root);
						break;
					}
					default: break;
				}
				break;
			}
			default: break;
		}
		return result;
	}
	xpath_value evaluate_value(const base_t& root) const {
		if (root_expr_<0) return xpath_value();
		return eval_expr(root_expr_,root_entry(root),1,1,root);
	}
	//虚文档条目落地为根节点指针("/"的select结果)。
	std::vector<const base_t*> materialize(const std::vector<entry>& nodes,const base_t& root) const {
		std::vector<const base_t*> result;
		result.reserve(nodes.size());
		for (const auto& it:nodes) result.push_back(it.node?it.node:&root);
		return result;
	}

public:
	xml_path()=default;
	explicit xml_path(std::string_view expression) : expression_(expression) {
		compile();
	}

	static xml_path compile(std::string_view expression) {
		return xml_path(expression);
	}
	static bool try_compile(std::string_view expression,xml_path& out) {
		try {
			out=xml_path(expression);
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
		const xpath_value value=evaluate_value(root);
		if (value.kind!=VK_NODESET) throw std::invalid_argument("Expression does not evaluate to a node set");
		return materialize(value.nodes,root);
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
	std::vector<_Xml> select_values(const base_t& root) const {
		const std::vector<const base_t*> nodes=select(root);
		std::vector<_Xml> result;
		result.reserve(nodes.size());
		for (const auto* it:nodes) result.push_back(_Xml(*it));
		return result;
	}
	//结果的XPath字符串值:节点集逐项string-value,非节点集为单元素转换结果。
	std::vector<string_t> string_values(const base_t& root) const {
		const xpath_value value=evaluate_value(root);
		std::vector<string_t> result;
		if (value.kind==VK_NODESET) {
			result.reserve(value.nodes.size());
			for (const auto& it:value.nodes) result.push_back(string_value(it,root));
		} else result.push_back(to_string_value(value,root));
		return result;
	}
	bool exists(const base_t& root) const {
		const xpath_value value=evaluate_value(root);
		if (value.kind!=VK_NODESET) throw std::invalid_argument("Expression does not evaluate to a node set");
		return !value.nodes.empty();
	}
	bool evaluate_boolean(const base_t& root) const {
		return to_boolean(evaluate_value(root));
	}
	double evaluate_number(const base_t& root) const {
		return to_number(evaluate_value(root),root);
	}
	string_t evaluate_string(const base_t& root) const {
		return to_string_value(evaluate_value(root),root);
	}
};

_STDEX_DOM_TPL_DECLARATION
class xml : public structure::_STDEX_DOM_DEF {
public:
	using base_t=structure::_STDEX_DOM_DEF;
	using int_t=typename base_t::int_t;
	using float_t=typename base_t::float_t;
	using boolean_t=typename base_t::boolean_t;
	using string_t=typename base_t::string_t;
	using array_t=typename base_t::array_t;
	using object_t=typename base_t::object_t;
	using size_type=typename base_t::size_type;
	using sax_t=xml_sax<xml>;
	using document_info_t=basic_xml_document_info<string_t>;

	static_assert(sizeof(typename string_t::value_type)==1,"xml serializer assumes a byte-oriented (UTF-8) string_t.");

protected:
	struct element_value : base_t::value_t {
		string_t name{};
		object_t attributes{};
		array_t children{};

		element_value()=default;
		explicit element_value(string_t element_name) : name(std::move(element_name)) { }
		~element_value() override=default;

		element_value(const element_value& other) : base_t::value_t() , name(other.name) , attributes(other.attributes) , children(other.children) { }

		typename base_t::value_t* clone(structure::dom_data_type t) const override {
			if (t==XDT_ELEMENT) return create_value<element_value>(*this);
			return base_t::value_t::clone(t);
		}
		void destroy(structure::dom_data_type t) override {
			if (t==XDT_ELEMENT) return;
			base_t::value_t::destroy(t);
		}
		void destroy_self(structure::dom_data_type t) override {
			this->destroy(t);
			_Allocator<element_value> alloc;
			std::allocator_traits<_Allocator<element_value>>::destroy(alloc,this);
			std::allocator_traits<_Allocator<element_value>>::deallocate(alloc,this,1);
		}
	};
	struct text_value : base_t::value_t {
		string_t text{};

		text_value()=default;
		explicit text_value(string_t content) : text(std::move(content)) { }
		~text_value() override=default;

		text_value(const text_value& other) : base_t::value_t() , text(other.text) { }

		typename base_t::value_t* clone(structure::dom_data_type t) const override {
			if (t==XDT_CDATA || t==XDT_COMMENT) return create_value<text_value>(*this);
			return base_t::value_t::clone(t);
		}
		void destroy(structure::dom_data_type t) override {
			if (t==XDT_CDATA || t==XDT_COMMENT) return;
			base_t::value_t::destroy(t);
		}
		void destroy_self(structure::dom_data_type t) override {
			this->destroy(t);
			_Allocator<text_value> alloc;
			std::allocator_traits<_Allocator<text_value>>::destroy(alloc,this);
			std::allocator_traits<_Allocator<text_value>>::deallocate(alloc,this,1);
		}
	};
	struct procinst_value : base_t::value_t {
		string_t target{};
		string_t content{};

		procinst_value()=default;
		procinst_value(string_t pi_target,string_t pi_content) : target(std::move(pi_target)) , content(std::move(pi_content)) { }
		~procinst_value() override=default;

		procinst_value(const procinst_value& other) : base_t::value_t() , target(other.target) , content(other.content) { }

		typename base_t::value_t* clone(structure::dom_data_type t) const override {
			if (t==XDT_PROCINST) return create_value<procinst_value>(*this);
			return base_t::value_t::clone(t);
		}
		void destroy(structure::dom_data_type t) override {
			if (t==XDT_PROCINST) return;
			base_t::value_t::destroy(t);
		}
		void destroy_self(structure::dom_data_type t) override {
			this->destroy(t);
			_Allocator<procinst_value> alloc;
			std::allocator_traits<_Allocator<procinst_value>>::destroy(alloc,this);
			std::allocator_traits<_Allocator<procinst_value>>::deallocate(alloc,this,1);
		}
	};

	template <typename _Vp,typename... _Args>
	static _Vp* create_value(_Args&&... args) {
		_Allocator<_Vp> alloc;
		_Vp* result=std::allocator_traits<_Allocator<_Vp>>::allocate(alloc,1);
		try {
			std::allocator_traits<_Allocator<_Vp>>::construct(alloc,result,std::forward<_Args>(args)...);
		} catch (...) {
			std::allocator_traits<_Allocator<_Vp>>::deallocate(alloc,result,1);
			throw;
		}
		return result;
	}
	static element_value* element_payload(const base_t& node) {
		element_value* payload=node.data().value?dynamic_cast<element_value*>(node.data().value):nullptr;
		if (node.type()!=xml_data_type(XDT_ELEMENT) || !payload) throw std::invalid_argument("Node is not an xml element");
		return payload;
	}
	static text_value* text_payload(const base_t& node) {
		text_value* payload=node.data().value?dynamic_cast<text_value*>(node.data().value):nullptr;
		if ((node.type()!=xml_data_type(XDT_CDATA) && node.type()!=xml_data_type(XDT_COMMENT)) || !payload) throw std::invalid_argument("Node is not an xml cdata section or comment");
		return payload;
	}
	static procinst_value* procinst_payload(const base_t& node) {
		procinst_value* payload=node.data().value?dynamic_cast<procinst_value*>(node.data().value):nullptr;
		if (node.type()!=xml_data_type(XDT_PROCINST) || !payload) throw std::invalid_argument("Node is not an xml processing instruction");
		return payload;
	}
	static string_t make_string(const char* text) {
		return string_t(text,text+std::strlen(text));
	}

public:
	using base_t::base_t;
	using base_t::operator =;

	xml()=default;
	~xml() override=default;

	xml(const xml&)=default;
	xml(xml&&) noexcept=default;

	xml& operator =(const xml&)=default;
	xml& operator =(xml&&)=default;

	xml(const base_t& other) : base_t(other) { }
	xml(base_t&& other) noexcept : base_t(std::move(other)) { }

	bool support(structure::dom_data_type t) const noexcept override {
		return base_t::support(t) || t==XDT_ELEMENT || t==XDT_CDATA || t==XDT_COMMENT || t==XDT_PROCINST;
	}

	static base_t make_element(string_t name) {
		base_t node;
		node.data()=typename base_t::data_t(structure::dom_data_type(XDT_ELEMENT),create_value<element_value>(std::move(name)));
		return node;
	}
	static base_t make_cdata(string_t text) {
		base_t node;
		node.data()=typename base_t::data_t(structure::dom_data_type(XDT_CDATA),create_value<text_value>(std::move(text)));
		return node;
	}
	static base_t make_comment(string_t text) {
		base_t node;
		node.data()=typename base_t::data_t(structure::dom_data_type(XDT_COMMENT),create_value<text_value>(std::move(text)));
		return node;
	}
	static base_t make_processing_instruction(string_t target,string_t data) {
		base_t node;
		node.data()=typename base_t::data_t(structure::dom_data_type(XDT_PROCINST),create_value<procinst_value>(std::move(target),std::move(data)));
		return node;
	}
	static xml element(string_t name) {
		return xml(make_element(std::move(name)));
	}
	static xml cdata(string_t text) {
		return xml(make_cdata(std::move(text)));
	}
	static xml comment(string_t text) {
		return xml(make_comment(std::move(text)));
	}
	static xml processing_instruction(string_t target,string_t data) {
		return xml(make_processing_instruction(std::move(target),std::move(data)));
	}
	static xml text(string_t content) {
		return xml(base_t(std::move(content)));
	}

	static bool is_element(const base_t& node) noexcept {
		return node.type()==xml_data_type(XDT_ELEMENT);
	}
	static bool is_cdata(const base_t& node) noexcept {
		return node.type()==xml_data_type(XDT_CDATA);
	}
	static bool is_comment(const base_t& node) noexcept {
		return node.type()==xml_data_type(XDT_COMMENT);
	}
	static bool is_processing_instruction(const base_t& node) noexcept {
		return node.type()==xml_data_type(XDT_PROCINST);
	}
	static bool is_text(const base_t& node) noexcept {
		return node.type()==structure::DDT_STRING;
	}
	bool is_element() const noexcept {
		return is_element(*this);
	}
	bool is_cdata() const noexcept {
		return is_cdata(*this);
	}
	bool is_comment() const noexcept {
		return is_comment(*this);
	}
	bool is_processing_instruction() const noexcept {
		return is_processing_instruction(*this);
	}
	bool is_text() const noexcept {
		return is_text(*this);
	}

	static string_t& name(const base_t& node) {
		return element_payload(node)->name;
	}
	static object_t& attributes(const base_t& node) {
		return element_payload(node)->attributes;
	}
	static array_t& children(const base_t& node) {
		return element_payload(node)->children;
	}
	static string_t& text_content(const base_t& node) {
		return text_payload(node)->text;
	}
	static string_t& pi_target(const base_t& node) {
		return procinst_payload(node)->target;
	}
	static string_t& pi_data(const base_t& node) {
		return procinst_payload(node)->content;
	}
	string_t& name() {
		return name(*this);
	}
	const string_t& name() const {
		return name(*this);
	}
	void set_name(string_t value) {
		name(*this)=std::move(value);
	}
	object_t& attributes() {
		return attributes(*this);
	}
	const object_t& attributes() const {
		return attributes(*this);
	}
	array_t& children() {
		return children(*this);
	}
	const array_t& children() const {
		return children(*this);
	}
	string_t& text_content() {
		return text_content(*this);
	}
	const string_t& text_content() const {
		return text_content(*this);
	}
	string_t& pi_target() {
		return pi_target(*this);
	}
	const string_t& pi_target() const {
		return pi_target(*this);
	}
	string_t& pi_data() {
		return pi_data(*this);
	}
	const string_t& pi_data() const {
		return pi_data(*this);
	}

	static bool has_attribute(const base_t& node,const string_t& key) {
		const object_t& table=attributes(node);
		return table.find(key)!=table.end();
	}
	bool has_attribute(const string_t& key) const {
		return has_attribute(*this,key);
	}
	static base_t& attribute(const base_t& node,const string_t& key) {
		object_t& table=attributes(node);
		auto it=table.find(key);
		if (it==table.end()) throw std::out_of_range("Attribute not found");
		return it->second;
	}
	base_t& attribute(const string_t& key) {
		return attribute(*this,key);
	}
	const base_t& attribute(const string_t& key) const {
		return attribute(*this,key);
	}
	static string_t attribute_or(const base_t& node,const string_t& key,string_t default_value) {
		if (!is_element(node)) return default_value;
		const object_t& table=element_payload(node)->attributes;
		auto it=table.find(key);
		if (it==table.end() || it->second.type()!=structure::DDT_STRING) return default_value;
		return *it->second.template get_ptr<const string_t*>();
	}
	string_t attribute_or(const string_t& key,string_t default_value) const {
		return attribute_or(*this,key,std::move(default_value));
	}
	static void set_attribute(base_t& node,string_t key,string_t value) {
		object_t& table=attributes(node);
		auto it=table.find(key);
		if (it==table.end()) table.emplace(std::move(key),base_t(std::move(value)));
		else it->second=base_t(std::move(value));
	}
	void set_attribute(string_t key,string_t value) {
		set_attribute(*this,std::move(key),std::move(value));
	}
	static bool remove_attribute(base_t& node,const string_t& key) {
		return attributes(node).erase(key)>0;
	}
	bool remove_attribute(const string_t& key) {
		return remove_attribute(*this,key);
	}

	static base_t& append_child(base_t& node,base_t child) {
		array_t& sequence=children(node);
		sequence.push_back(std::move(child));
		return sequence.back();
	}
	base_t& append_child(base_t child) {
		return append_child(*this,std::move(child));
	}
	static size_type child_count(const base_t& node) {
		return children(node).size();
	}
	size_type child_count() const {
		return child_count(*this);
	}
	static base_t* find_child(base_t& node,const string_t& child_name) {
		for (auto& it:children(node)) {
			if (is_element(it) && element_payload(it)->name==child_name) return &it;
		}
		return nullptr;
	}
	static const base_t* find_child(const base_t& node,const string_t& child_name) {
		return find_child(const_cast<base_t&>(node),child_name);
	}
	base_t* find_child(const string_t& child_name) {
		return find_child(*this,child_name);
	}
	const base_t* find_child(const string_t& child_name) const {
		return find_child(*this,child_name);
	}
	static string_t inner_text(const base_t& node) {
		string_t result;
		for (const auto& it:children(node)) {
			if (it.type()==structure::DDT_STRING) result+=*it.template get_ptr<const string_t*>();
			else if (is_cdata(it)) result+=text_payload(it)->text;
		}
		return result;
	}
	string_t inner_text() const {
		return inner_text(*this);
	}

	//XmlPath便利接口:string_view版即编即查,重复求值请预编译xml_path<xml>。
	std::vector<const base_t*> select(const xml_path<xml>& path) const {
		return path.select(static_cast<const base_t&>(*this));
	}
	std::vector<base_t*> select(const xml_path<xml>& path) {
		return path.select(static_cast<base_t&>(*this));
	}
	std::vector<const base_t*> select(std::string_view expression) const {
		return xml_path<xml>(expression).select(static_cast<const base_t&>(*this));
	}
	std::vector<base_t*> select(std::string_view expression) {
		return xml_path<xml>(expression).select(static_cast<base_t&>(*this));
	}
	const base_t* select_first(const xml_path<xml>& path) const {
		return path.select_first(static_cast<const base_t&>(*this));
	}
	base_t* select_first(const xml_path<xml>& path) {
		return path.select_first(static_cast<base_t&>(*this));
	}
	const base_t* select_first(std::string_view expression) const {
		return xml_path<xml>(expression).select_first(static_cast<const base_t&>(*this));
	}
	base_t* select_first(std::string_view expression) {
		return xml_path<xml>(expression).select_first(static_cast<base_t&>(*this));
	}
	std::vector<xml> select_values(const xml_path<xml>& path) const {
		return path.select_values(static_cast<const base_t&>(*this));
	}
	std::vector<xml> select_values(std::string_view expression) const {
		return xml_path<xml>(expression).select_values(static_cast<const base_t&>(*this));
	}
	std::vector<string_t> string_values(const xml_path<xml>& path) const {
		return path.string_values(static_cast<const base_t&>(*this));
	}
	std::vector<string_t> string_values(std::string_view expression) const {
		return xml_path<xml>(expression).string_values(static_cast<const base_t&>(*this));
	}
	bool exists(const xml_path<xml>& path) const {
		return path.exists(static_cast<const base_t&>(*this));
	}
	bool exists(std::string_view expression) const {
		return xml_path<xml>(expression).exists(static_cast<const base_t&>(*this));
	}
	bool evaluate_boolean(const xml_path<xml>& path) const {
		return path.evaluate_boolean(static_cast<const base_t&>(*this));
	}
	bool evaluate_boolean(std::string_view expression) const {
		return xml_path<xml>(expression).evaluate_boolean(static_cast<const base_t&>(*this));
	}
	double evaluate_number(const xml_path<xml>& path) const {
		return path.evaluate_number(static_cast<const base_t&>(*this));
	}
	double evaluate_number(std::string_view expression) const {
		return xml_path<xml>(expression).evaluate_number(static_cast<const base_t&>(*this));
	}
	string_t evaluate_string(const xml_path<xml>& path) const {
		return path.evaluate_string(static_cast<const base_t&>(*this));
	}
	string_t evaluate_string(std::string_view expression) const {
		return xml_path<xml>(expression).evaluate_string(static_cast<const base_t&>(*this));
	}

protected:
	//记法转换协议·源侧降级:xml专有节点降级为纯dom的等价结构(有损去记法,数据保全):
	//元素→{"name","attributes","children"};CDATA→string;注释→string;处理指令→{"target","data"}。
	//如需其他形态(如丢弃注释)请用convert_handler_t定制。
	bool degrade_unsupported(const base_t& source,base_t& replacement) const override {
		if (source.type()==xml_data_type(XDT_CDATA) || source.type()==xml_data_type(XDT_COMMENT)) {
			replacement=base_t(text_payload(source)->text);
			return true;
		}
		if (source.type()==xml_data_type(XDT_PROCINST)) {
			replacement=base_t(structure::DDT_OBJECT);
			replacement.value().object->emplace(make_string("target"),base_t(procinst_payload(source)->target));
			replacement.value().object->emplace(make_string("data"),base_t(procinst_payload(source)->content));
			return true;
		}
		if (source.type()==xml_data_type(XDT_ELEMENT)) {
			const element_value* payload=element_payload(source);
			replacement=base_t(structure::DDT_OBJECT);
			replacement.value().object->emplace(make_string("name"),base_t(payload->name));
			base_t attribute_table(structure::DDT_OBJECT);
			for (const auto& it:payload->attributes) attribute_table.value().object->emplace(it.first,it.second);
			base_t child_sequence(structure::DDT_ARRAY);
			for (const auto& it:payload->children) child_sequence.value().array->push_back(it);
			replacement.value().object->emplace(make_string("attributes"),std::move(attribute_table));
			replacement.value().object->emplace(make_string("children"),std::move(child_sequence));
			return true;
		}
		return false;
	}

private:
	struct xml_token {
		xml_symbol symbol;
		std::size_t position;
		string_t text{};
		string_t aux{};
		int extra=0;
	};

	static const std::regex& tag_whitespace_regex() {
		static const std::regex result(R"([ \t\r\n]+)",std::regex::optimize);
		return result;
	}
	static const std::regex& name_regex() {
		//XML NameChar的ASCII子集近似;完整Unicode区间超出char正则字符类的可移植范围,留待charset层。
		//this method is discarded
		//static const std::regex result(R"([:A-Za-z_][:A-Za-z0-9._\-]*)",std::regex::optimize);
		constexpr char u8_char[]=R"((?:[\xC2-\xDF][\x80-\xBF]|\xE0[\xA0-\xBF][\x80-\xBF]|[\xE1-\xEC][\x80-\xBF]{2}|\xED[\x80-\x9F][\x80-\xBF]|[\xEE-\xEF][\x80-\xBF]{2}|\xF0[\x90-\xBF][\x80-\xBF]{2}|[\xF1-\xF3][\x80-\xBF]{3}|\xF4[\x80-\x8F][\x80-\xBF]{2}))";
		static const std::string start=std::string("(?:[:A-Za-z0-9_]|")+u8_char+")";
		static const std::string follow=std::string("(?:[:A-Za-z0-9._\\-]|")+u8_char+")*";
		static const std::regex result(start+follow,std::regex::optimize);
		//static const std::regex result(R"([:A-Za-z_\x80-\xFF][:A-Za-z0-9._\-\x80-\xFF]*)",std::regex::optimize);
		return result;
	}
	static const std::regex& attvalue_regex() {
		static const std::regex result(R"("[^<"]*"|'[^<']*')",std::regex::optimize);
		return result;
	}
	static const std::regex& text_regex() {
		static const std::regex result(R"([^<]+)",std::regex::optimize);
		return result;
	}
	static const std::regex& decl_version_regex() {
		static const std::regex result(R"re(version[ \t\r\n]*=[ \t\r\n]*(?:"(1\.[0-9]+)"|'(1\.[0-9]+)'))re",std::regex::optimize);
		return result;
	}
	static const std::regex& decl_encoding_regex() {
		static const std::regex result(R"re(encoding[ \t\r\n]*=[ \t\r\n]*(?:"([A-Za-z][A-Za-z0-9._\-]*)"|'([A-Za-z][A-Za-z0-9._\-]*)'))re",std::regex::optimize);
		return result;
	}
	static const std::regex& decl_standalone_regex() {
		static const std::regex result(R"re(standalone[ \t\r\n]*=[ \t\r\n]*(?:"(yes|no)"|'(yes|no)'))re",std::regex::optimize);
		return result;
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
	//实体引用与字符引用展开+换行规范化("\r\n"/"\r"→"\n");attribute_mode额外做属性值空白规范化。
	//五个预定义实体与字符引用在词法层无条件展开;自定义实体无DTD支持,按良构性要求报错。
	static bool decode_text(const char* first,const char* last,string_t& out,bool attribute_mode,std::string& error_message) {
		while (first<last) {
			const char c=*first;
			if (c=='\r') {
				first++;
				if (first<last && *first=='\n') first++;
				out.push_back(attribute_mode?' ':'\n');
				continue;
			}
			if (attribute_mode && (c=='\t' || c=='\n')) {
				out.push_back(' ');
				first++;
				continue;
			}
			if (c!='&') {
				out.push_back(c);
				first++;
				continue;
			}
			const char* entity_end=first+1;
			while (entity_end<last && *entity_end!=';' && entity_end-first<=10) entity_end++;
			if (entity_end>=last || *entity_end!=';') {
				error_message="unterminated entity reference";
				return false;
			}
			const std::string entity(first+1,entity_end);
			if (entity=="lt") out.push_back('<');
			else if (entity=="gt") out.push_back('>');
			else if (entity=="amp") out.push_back('&');
			else if (entity=="apos") out.push_back('\'');
			else if (entity=="quot") out.push_back('"');
			else if (entity.size()>1 && entity[0]=='#') {
				char* parse_end=nullptr;
				const bool hexadecimal=entity.size()>2 && (entity[1]=='x' || entity[1]=='X');
				const unsigned long cp=std::strtoul(entity.c_str()+(hexadecimal?2:1),&parse_end,hexadecimal?16:10);
				if (!parse_end || *parse_end!='\0' || cp==0 || cp>0x10FFFF || (cp>=0xD800 && cp<=0xDFFF)) {
					error_message="invalid character reference '&"+entity+";'";
					return false;
				}
				append_codepoint(out,cp);
			} else {
				error_message="undefined entity '&"+entity+";' (no DTD support)";
				return false;
			}
			first=entity_end+1;
		}
		return true;
	}
	static bool decode_declaration(const std::string& body,xml_token& token,std::string& error_message) {
		std::smatch match;
		if (!std::regex_search(body,match,decl_version_regex())) {
			error_message="xml declaration requires a version pseudo-attribute";
			return false;
		}
		const std::string version=match[1].matched?match[1].str():match[2].str();
		token.text=string_t(version.begin(),version.end());
		if (std::regex_search(body,match,decl_encoding_regex())) {
			const std::string encoding=match[1].matched?match[1].str():match[2].str();
			token.aux=string_t(encoding.begin(),encoding.end());
		}
		token.extra=-1;
		if (std::regex_search(body,match,decl_standalone_regex())) token.extra=((match[1].matched?match[1].str():match[2].str())=="yes")?1:0;
		return true;
	}
	//DOCTYPE:计数内部子集的[],引号内忽略,直到配对的">";原文整体作不透明载荷(归属结论:
	//实体定义属词法层而本实现无DTD、结构验证属并联listener、数据模型不为DTD增加节点)。
	static bool scan_doctype(const char* first,const char* last,const char*& next,std::string& error_message) {
		const char* it=first+9;
		int depth=0;
		char quote=0;
		while (it<last) {
			const char c=*it;
			if (quote) {
				if (c==quote) quote=0;
			} else if (c=='"' || c=='\'') quote=c;
			else if (c=='[') depth++;
			else if (c==']') depth--;
			else if (c=='>' && depth==0) {
				next=it+1;
				return true;
			}
			it++;
		}
		error_message="unterminated DOCTYPE declaration";
		return false;
	}

	static bool tokenize(std::string_view input,std::vector<xml_token>& tokens,std::size_t& error_position,std::string& error_message) {
		const char* first=input.data();
		const char* const last=input.data()+input.size();
		if (last-first>=3 && static_cast<unsigned char>(first[0])==0xEF && static_cast<unsigned char>(first[1])==0xBB && static_cast<unsigned char>(first[2])==0xBF) first+=3;
		std::cmatch match;
		const auto flags=std::regex_constants::match_continuous;
		bool tag_mode=false;
		int pending_name_role=0;
		auto fail=[&](const char* where,const std::string& message){
			error_position=static_cast<std::size_t>(where-input.data());
			error_message=message;
			return false;
		};
		while (first<last) {
			xml_token token;
			token.position=static_cast<std::size_t>(first-input.data());
			if (tag_mode) {
				if (std::regex_search(first,last,match,tag_whitespace_regex(),flags)) {
					first=match[0].second;
					continue;
				}
				if (*first=='>') {
					token.symbol=XS_GT;
					first++;
					tag_mode=false;
				} else if (last-first>=2 && first[0]=='/' && first[1]=='>') {
					token.symbol=XS_EMPTY_CLOSE;
					first+=2;
					tag_mode=false;
				} else if (*first=='=') {
					token.symbol=XS_EQ;
					first++;
				} else if (std::regex_search(first,last,match,name_regex(),flags)) {
					token.symbol=XS_NAME;
					token.text=string_t(match[0].first,match[0].second);
					token.extra=pending_name_role;
					pending_name_role=0;
					first=match[0].second;
				} else if (std::regex_search(first,last,match,attvalue_regex(),flags)) {
					token.symbol=XS_ATTVALUE;
					if (!decode_text(match[0].first+1,match[0].second-1,token.text,true,error_message)) return fail(first,error_message);
					first=match[0].second;
				} else return fail(first,std::string("unexpected character '")+*first+"' inside tag");
			} else if (*first=='<') {
				if (last-first>=4 && std::memcmp(first,"<!--",4)==0) {
					const char* end_mark=std::search(first+4,last,"-->","-->"+3);
					if (end_mark==last) return fail(first,"unterminated comment");
					token.symbol=XS_COMMENT;
					token.text=string_t(first+4,end_mark);
					const std::string content(first+4,end_mark);
					if (content.find("--")!=std::string::npos) return fail(first,"'--' is not allowed inside a comment");
					first=end_mark+3;
				} else if (last-first>=9 && std::memcmp(first,"<![CDATA[",9)==0) {
					const char* end_mark=std::search(first+9,last,"]]>","]]>"+3);
					if (end_mark==last) return fail(first,"unterminated CDATA section");
					token.symbol=XS_CDATA;
					token.text=string_t(first+9,end_mark);
					first=end_mark+3;
				} else if (last-first>=9 && std::memcmp(first,"<!DOCTYPE",9)==0) {
					const char* next=nullptr;
					if (!scan_doctype(first,last,next,error_message)) return fail(first,error_message);
					token.symbol=XS_DOCTYPE;
					token.text=string_t(first,next);
					first=next;
				} else if (last-first>=2 && first[1]=='?') {
					const char* target_first=first+2;
					if (!std::regex_search(target_first,last,match,name_regex(),flags)) return fail(first,"processing instruction requires a target name");
					const std::string target(match[0].first,match[0].second);
					const char* end_mark=std::search(match[0].second,last,"?>","?>"+2);
					if (end_mark==last) return fail(first,"unterminated processing instruction");
					const char* data_first=match[0].second;
					while (data_first<end_mark && (*data_first==' ' || *data_first=='\t' || *data_first=='\r' || *data_first=='\n')) data_first++;
					std::string lowered=target;
					for (auto& it:lowered) {
						if (it>='A' && it<='Z') it=static_cast<char>(it-'A'+'a');
					}
					if (lowered=="xml") {
						if (!tokens.empty() || token.position!=0) return fail(first,"xml declaration is only allowed at the very beginning");
						token.symbol=XS_XMLDECL;
						if (!decode_declaration(std::string(data_first,end_mark),token,error_message)) return fail(first,error_message);
					} else {
						token.symbol=XS_PI;
						token.text=string_t(target.begin(),target.end());
						token.aux=string_t(data_first,end_mark);
					}
					first=end_mark+2;
				} else if (last-first>=2 && first[1]=='/') {
					token.symbol=XS_ETAG_OPEN;
					first+=2;
					tag_mode=true;
					pending_name_role=2;
				} else {
					token.symbol=XS_LT;
					first++;
					tag_mode=true;
					pending_name_role=1;
				}
			} else if (std::regex_search(first,last,match,text_regex(),flags)) {
				token.symbol=XS_TEXT;
				const std::string raw(match[0].first,match[0].second);
				if (raw.find("]]>")!=std::string::npos) return fail(first,"']]>' is not allowed in character data");
				if (!decode_text(match[0].first,match[0].second,token.text,false,error_message)) return fail(first,error_message);
				first=match[0].second;
			} else return fail(first,std::string("unexpected character '")+*first+"'");
			tokens.push_back(std::move(token));
		}
		if (tag_mode) return fail(last,"unterminated tag");
		xml_token eof_token;
		eof_token.symbol=XS_EOF;
		eof_token.position=input.size();
		tokens.push_back(std::move(eof_token));
		return true;
	}

	using parser_t=syntax::parser<xml_symbol,xml_production>;

	static bool initialize_grammar(parser_t& target) {
		auto unit=[](xml_symbol left,std::initializer_list<xml_symbol> rights,xml_production id){
			return syntax::single_parser_unit<xml_symbol,xml_production>(left,rights,id);
		};
		target.units={
			unit(XS_START,{XS_DOCUMENT,XS_EOF},XP_START),
			unit(XS_DOCUMENT,{XS_ITEM_SEQ},XP_DOCUMENT),
			unit(XS_ITEM_SEQ,{XS_DOC_ITEM},XP_ITEMS_FIRST),
			unit(XS_ITEM_SEQ,{XS_ITEM_SEQ,XS_DOC_ITEM},XP_ITEMS_APPEND),
			unit(XS_DOC_ITEM,{XS_XMLDECL},XP_ITEM_DECL),
			unit(XS_DOC_ITEM,{XS_DOCTYPE},XP_ITEM_DOCTYPE),
			unit(XS_DOC_ITEM,{XS_COMMENT},XP_ITEM_COMMENT),
			unit(XS_DOC_ITEM,{XS_PI},XP_ITEM_PI),
			unit(XS_DOC_ITEM,{XS_TEXT},XP_ITEM_TEXT),
			unit(XS_DOC_ITEM,{XS_ELEMENT},XP_ITEM_ELEMENT),
			unit(XS_ELEMENT,{XS_EMPTY_ELEM},XP_ELEMENT_EMPTY),
			unit(XS_ELEMENT,{XS_STAG,XS_ETAG},XP_ELEMENT_BLANK),
			unit(XS_ELEMENT,{XS_STAG,XS_CONTENT,XS_ETAG},XP_ELEMENT),
			unit(XS_EMPTY_ELEM,{XS_LT,XS_NAME,XS_EMPTY_CLOSE},XP_EMPTY_ELEM_PLAIN),
			unit(XS_EMPTY_ELEM,{XS_LT,XS_NAME,XS_ATTR_SEQ,XS_EMPTY_CLOSE},XP_EMPTY_ELEM_ATTRS),
			unit(XS_STAG,{XS_LT,XS_NAME,XS_GT},XP_STAG_PLAIN),
			unit(XS_STAG,{XS_LT,XS_NAME,XS_ATTR_SEQ,XS_GT},XP_STAG_ATTRS),
			unit(XS_ETAG,{XS_ETAG_OPEN,XS_NAME,XS_GT},XP_ETAG),
			unit(XS_ATTR_SEQ,{XS_ATTRIBUTE},XP_ATTR_FIRST),
			unit(XS_ATTR_SEQ,{XS_ATTR_SEQ,XS_ATTRIBUTE},XP_ATTR_APPEND),
			unit(XS_ATTRIBUTE,{XS_NAME,XS_EQ,XS_ATTVALUE},XP_ATTRIBUTE),
			unit(XS_CONTENT,{XS_CONTENT_ITEM},XP_CONTENT_FIRST),
			unit(XS_CONTENT,{XS_CONTENT,XS_CONTENT_ITEM},XP_CONTENT_APPEND),
			unit(XS_CONTENT_ITEM,{XS_ELEMENT},XP_CITEM_ELEMENT),
			unit(XS_CONTENT_ITEM,{XS_TEXT},XP_CITEM_TEXT),
			unit(XS_CONTENT_ITEM,{XS_CDATA},XP_CITEM_CDATA),
			unit(XS_CONTENT_ITEM,{XS_COMMENT},XP_CITEM_COMMENT),
			unit(XS_CONTENT_ITEM,{XS_PI},XP_CITEM_PI),
		};
		target.generate_parser();
		return true;
	}
	static parser_t& grammar() {
		static parser_t instance(XS_START,XS_EPSILON,XS_EOF);
		static const bool initialized=initialize_grammar(instance);
		static_cast<void>(initialized);
		return instance;
	}
	static std::mutex& grammar_mutex() {
		static std::mutex instance;
		return instance;
	}

	class xml_listener : public syntax::parser_listener<xml_symbol,xml_production> {
		std::vector<xml_token>* tokens_=nullptr;
		sax_t* sax_=nullptr;
		std::vector<string_t> open_names_;
		std::vector<std::vector<string_t>> open_attrs_;
		bool aborted_=false;
		bool failed_=false;
		bool root_seen_=false;
		bool doctype_seen_=false;
		bool any_item_seen_=false;

		void abort_check(bool keep_going) {
			if (!keep_going) aborted_=true;
		}
		void fail(const xml_token& token,const std::string& message) {
			failed_=true;
			const std::string text(token.text.begin(),token.text.end());
			sax_->parse_error(token.position,text,message);
		}
		static bool whitespace_only(const string_t& text) noexcept {
			for (auto it:text) {
				if (it!=' ' && it!='\t' && it!='\n' && it!='\r') return false;
			}
			return true;
		}

	public:
		void reset(std::vector<xml_token>& tokens,sax_t& sax) {
			tokens_=&tokens;
			sax_=&sax;
			open_names_.clear();
			open_attrs_.clear();
			aborted_=false;
			failed_=false;
			root_seen_=false;
			doctype_seen_=false;
			any_item_seen_=false;
			this->enabled=true;
		}
		bool aborted() const noexcept {
			return aborted_;
		}
		bool failed() const noexcept {
			return failed_;
		}
		intptr_t on_shift(uintptr_t id,int state,xml_symbol word) override {
			static_cast<void>(state);
			if (aborted_ || failed_) return 0;
			xml_token& token=(*tokens_)[id-1];
			if (word==XS_NAME && token.extra==1) {
				open_names_.push_back(token.text);
				open_attrs_.emplace_back();
				abort_check(sax_->start_element(token.text));
			}
			return 0;
		}
		intptr_t on_reduction(uintptr_t id,int state,int next,xml_production sentence_id,int reduction_num) override {
			static_cast<void>(state);
			static_cast<void>(next);
			static_cast<void>(reduction_num);
			if (aborted_ || failed_) return 0;
			switch (sentence_id) {
				case XP_ATTRIBUTE: {
					xml_token& name_token=(*tokens_)[id-4];
					xml_token& value_token=(*tokens_)[id-2];
					for (const auto& it:open_attrs_.back()) {
						if (it==name_token.text) {
							fail(name_token,"duplicate attribute");
							return 0;
						}
					}
					open_attrs_.back().push_back(name_token.text);
					abort_check(sax_->attribute(name_token.text,value_token.text));
					break;
				}
				case XP_EMPTY_ELEM_PLAIN:
				case XP_EMPTY_ELEM_ATTRS: {
					open_names_.pop_back();
					open_attrs_.pop_back();
					abort_check(sax_->end_element());
					break;
				}
				case XP_ETAG: {
					xml_token& name_token=(*tokens_)[id-3];
					if (open_names_.empty() || open_names_.back()!=name_token.text) {
						fail(name_token,"mismatched closing tag");
						return 0;
					}
					open_names_.pop_back();
					open_attrs_.pop_back();
					abort_check(sax_->end_element());
					break;
				}
				case XP_CITEM_TEXT: abort_check(sax_->characters((*tokens_)[id-2].text));break;
				case XP_CITEM_CDATA: abort_check(sax_->cdata((*tokens_)[id-2].text));break;
				case XP_CITEM_COMMENT: abort_check(sax_->comment((*tokens_)[id-2].text));break;
				case XP_CITEM_PI: abort_check(sax_->processing_instruction((*tokens_)[id-2].text,(*tokens_)[id-2].aux));break;
				case XP_ITEM_DECL: {
					xml_token& token=(*tokens_)[id-2];
					if (any_item_seen_) {
						fail(token,"xml declaration must be the first item");
						return 0;
					}
					any_item_seen_=true;
					abort_check(sax_->declaration(token.text,token.aux,token.extra));
					break;
				}
				case XP_ITEM_DOCTYPE: {
					xml_token& token=(*tokens_)[id-2];
					if (doctype_seen_ || root_seen_) {
						fail(token,doctype_seen_?"multiple DOCTYPE declarations":"DOCTYPE must precede the root element");
						return 0;
					}
					doctype_seen_=true;
					any_item_seen_=true;
					abort_check(sax_->doctype(token.text));
					break;
				}
				case XP_ITEM_COMMENT: {
					any_item_seen_=true;
					abort_check(sax_->comment((*tokens_)[id-2].text));
					break;
				}
				case XP_ITEM_PI: {
					any_item_seen_=true;
					abort_check(sax_->processing_instruction((*tokens_)[id-2].text,(*tokens_)[id-2].aux));
					break;
				}
				case XP_ITEM_TEXT: {
					xml_token& token=(*tokens_)[id-2];
					any_item_seen_=true;
					if (!whitespace_only(token.text)) {
						fail(token,"character data is not allowed outside the root element");
						return 0;
					}
					break;
				}
				case XP_ITEM_ELEMENT: {
					any_item_seen_=true;
					if (root_seen_) {
						fail((*tokens_)[id-2],"multiple root elements");
						return 0;
					}
					root_seen_=true;
					break;
				}
				case XP_DOCUMENT: {
					if (!root_seen_) fail((*tokens_)[id-2],"document has no root element");
					break;
				}
				case XP_START:
				case XP_ITEMS_FIRST:
				case XP_ITEMS_APPEND:
				case XP_ELEMENT_EMPTY:
				case XP_ELEMENT_BLANK:
				case XP_ELEMENT:
				case XP_STAG_PLAIN:
				case XP_STAG_ATTRS:
				case XP_ATTR_FIRST:
				case XP_ATTR_APPEND:
				case XP_CONTENT_FIRST:
				case XP_CONTENT_APPEND:
				case XP_CITEM_ELEMENT:
				default: break;
			}
			return 0;
		}
		void on_accept() override { }
		int on_error(uintptr_t id,typename syntax::parser_listener<xml_symbol,xml_production>::error_type type,int state,xml_symbol word) override {
			static_cast<void>(type);
			static_cast<void>(state);
			static_cast<void>(word);
			failed_=true;
			if (sax_ && tokens_ && id!=static_cast<uintptr_t>(-1) && id>=1 && id<=tokens_->size()) {
				const xml_token& token=(*tokens_)[id-1];
				const std::string text(token.text.begin(),token.text.end());
				sax_->parse_error(token.position,text,"Unexpected token");
			} else if (sax_) sax_->parse_error(0,std::string(),"Unexpected end of input");
			return 0;
		}
	};

public:
	static bool sax_parse(std::string_view input,sax_t* sax) {
		std::vector<xml_token> tokens;
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
		static xml_listener listener;
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
	static xml parse(std::string_view input,document_info_t* info=nullptr,bool preserve_whitespace=false,bool allow_exceptions=true) {
		xml result;
		xml_sax_dom_builder<xml> builder(result,info,preserve_whitespace);
		const bool ok=sax_parse(input,&builder) && builder.completed();
		if (!ok) {
			if (allow_exceptions) throw std::runtime_error(std::string("Parse error at byte ")+std::to_string(builder.error_position())+std::string(": ")+(builder.error_message().empty()?std::string("Incomplete document"):builder.error_message()));
			return xml();
		}
		return result;
	}
	static bool try_parse(std::string_view input,xml& out,document_info_t* info=nullptr,bool preserve_whitespace=false) {
		xml result;
		xml_sax_dom_builder<xml> builder(result,info,preserve_whitespace);
		if (!sax_parse(input,&builder) || !builder.completed()) return false;
		out=std::move(result);
		return true;
	}
	static bool accept(std::string_view input) {
		xml_sax_acceptor<xml> acceptor;
		return sax_parse(input,&acceptor);
	}

private:
	static void dump_char_reference(string_t& out,unsigned long cp) {
		static const char digits[]="0123456789ABCDEF";
		char buffer[8];
		int length=0;
		if (cp==0) buffer[length++]='0';
		while (cp) {
			buffer[length++]=digits[cp&0xF];
			cp>>=4;
		}
		out.push_back('&');
		out.push_back('#');
		out.push_back('x');
		for (int i=length-1;i>=0;i--) out.push_back(buffer[i]);
		out.push_back(';');
	}
	//文本/属性值转义。XML 1.0禁止除\t\n\r外的C0控制字符,遇到即抛;ensure_ascii时
	//按UTF-8解码非ASCII序列并写字符引用(与json的ensure_ascii同语义),非法UTF-8抛出。
	static void dump_escaped(string_t& out,const string_t& s,bool attribute_mode,bool ensure_ascii) {
		const unsigned char* first=reinterpret_cast<const unsigned char*>(s.data());
		const unsigned char* const last=first+s.size();
		while (first<last) {
			const unsigned char c=*first;
			switch (c) {
				case '&': out.append({'&','a','m','p',';'});first++;continue;
				case '<': out.append({'&','l','t',';'});first++;continue;
				case '>': out.append({'&','g','t',';'});first++;continue;
				case '"': {
					if (attribute_mode) out.append({'&','q','u','o','t',';'});
					else out.push_back('"');
					first++;
					continue;
				}
				case '\t':
				case '\n': {
					if (attribute_mode) dump_char_reference(out,c);
					else out.push_back(static_cast<typename string_t::value_type>(c));
					first++;
					continue;
				}
				case '\r': {
					dump_char_reference(out,c);
					first++;
					continue;
				}
				default: break;
			}
			if (c<0x20) throw std::invalid_argument("Control character U+"+std::to_string(static_cast<unsigned>(c))+" is not representable in XML 1.0");
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
			} else dump_char_reference(out,cp);
			first+=extra+1;
		}
	}
	static void dump_integer(string_t& out,int_t value) {
		const std::string text=std::to_string(static_cast<long long>(value));
		out.append(text.begin(),text.end());
	}
	static void dump_floating(string_t& out,float_t value) {
		if (std::isnan(value) || std::isinf(value)) throw std::invalid_argument("NaN and infinity are not representable as xml text");
		char buffer[64];
		int length=std::snprintf(buffer,sizeof(buffer),"%.15g",static_cast<double>(value));
		if (std::strtod(buffer,nullptr)!=static_cast<double>(value)) length=std::snprintf(buffer,sizeof(buffer),"%.17g",static_cast<double>(value));
		out.append(buffer,buffer+length);
	}
	static void dump_indent(string_t& out,std::size_t count,typename string_t::value_type indent_char) {
		for (std::size_t i=0;i<count;i++) out.push_back(indent_char);
	}
	//CDATA段不转义,以"]]]]><![CDATA[>"拆分内部的"]]>"实现无损往返。
	static void dump_cdata(string_t& out,const string_t& text) {
		out.append({'<','!','[','C','D','A','T','A','['});
		std::size_t begin=0;
		while (true) {
			std::size_t end_mark=text.find(make_string("]]>"),begin);
			if (end_mark==string_t::npos) break;
			out.append(text,begin,end_mark+2-begin);
			out.append({'<','!','[','C','D','A','T','A','['});
			begin=end_mark+2;
		}
		out.append(text,begin,text.size()-begin);
		out.append({']',']','>'});
	}
	//pretty布局规则:无子→自闭合;唯一子且为文本→内联;否则逐子换行缩进。
	//注意pretty会在混合内容中插入空白从而改变文本语义,需要无损往返请用紧凑模式(indent=-1)。
	static void dump_internal(const base_t& node,string_t& out,int indent_step,typename string_t::value_type indent_char,bool ensure_ascii,std::size_t current_indent) {
		const bool pretty=indent_step>0;
		const std::size_t child_indent=current_indent+(pretty?static_cast<std::size_t>(indent_step):0);
		switch (static_cast<int>(node.type())) {
			case static_cast<int>(structure::DDT_NULL): break;
			case static_cast<int>(structure::DDT_BOOL): {
				if (*node.template get_ptr<const boolean_t*>()) out.append({'t','r','u','e'});
				else out.append({'f','a','l','s','e'});
				break;
			}
			case static_cast<int>(structure::DDT_INT): {
				dump_integer(out,*node.template get_ptr<const int_t*>());
				break;
			}
			case static_cast<int>(structure::DDT_FLOAT): {
				dump_floating(out,*node.template get_ptr<const float_t*>());
				break;
			}
			case static_cast<int>(structure::DDT_STRING): {
				dump_escaped(out,*node.template get_ptr<const string_t*>(),false,ensure_ascii);
				break;
			}
			case static_cast<int>(structure::DDT_ARRAY): {
				for (auto it=node.cbegin();it!=node.cend();it++) {
					if (pretty && it!=node.cbegin()) {
						out.push_back('\n');
						dump_indent(out,current_indent,indent_char);
					}
					dump_internal(*it,out,indent_step,indent_char,ensure_ascii,current_indent);
				}
				break;
			}
			default: {
				if (is_cdata(node)) {
					dump_cdata(out,text_payload(node)->text);
					break;
				}
				if (is_comment(node)) {
					const string_t& text=text_payload(node)->text;
					if (text.find(make_string("--"))!=string_t::npos || (!text.empty() && text[text.size()-1]=='-')) throw std::invalid_argument("Comment content must not contain '--' or end with '-'");
					out.append({'<','!','-','-'});
					out.append(text);
					out.append({'-','-','>'});
					break;
				}
				if (is_processing_instruction(node)) {
					const procinst_value* payload=procinst_payload(node);
					if (payload->content.find(make_string("?>"))!=string_t::npos) throw std::invalid_argument("Processing instruction data must not contain '?>'");
					out.append({'<','?'});
					out.append(payload->target);
					if (!payload->content.empty()) {
						out.push_back(' ');
						out.append(payload->content);
					}
					out.append({'?','>'});
					break;
				}
				if (is_element(node)) {
					const element_value* payload=element_payload(node);
					out.push_back('<');
					out.append(payload->name);
					for (const auto& it:payload->attributes) {
						out.push_back(' ');
						out.append(it.first);
						out.push_back('=');
						out.push_back('"');
						if (it.second.type()==structure::DDT_STRING) dump_escaped(out,*it.second.template get_ptr<const string_t*>(),true,ensure_ascii);
						else dump_internal(it.second,out,-1,indent_char,ensure_ascii,0);
						out.push_back('"');
					}
					if (payload->children.empty()) {
						out.push_back('/');
						out.push_back('>');
						break;
					}
					out.push_back('>');
					const bool inline_content=!pretty || (payload->children.size()==1 && payload->children[0].type()==structure::DDT_STRING);
					for (const auto& it:payload->children) {
						if (!inline_content) {
							out.push_back('\n');
							dump_indent(out,child_indent,indent_char);
						}
						dump_internal(it,out,indent_step,indent_char,ensure_ascii,child_indent);
					}
					if (!inline_content) {
						out.push_back('\n');
						dump_indent(out,current_indent,indent_char);
					}
					out.push_back('<');
					out.push_back('/');
					out.append(payload->name);
					out.push_back('>');
					break;
				}
				throw std::invalid_argument("xml: unsupported node type "+std::to_string(static_cast<long long>(static_cast<int>(node.type())))+" (use the dom conversion protocol first)");
			}
		}
	}

public:
	virtual string_t dump(int indent=-1,typename string_t::value_type indent_char=' ',bool ensure_ascii=false) const {
		string_t result;
		dump_internal(*this,result,indent,indent_char,ensure_ascii,0);
		return result;
	}
	static string_t dump_document(const base_t& root,const document_info_t* info,int indent=-1,typename string_t::value_type indent_char=' ',bool ensure_ascii=false) {
		string_t result;
		if (info && info->has_declaration()) {
			result.append({'<','?','x','m','l',' ','v','e','r','s','i','o','n','=','"'});
			result.append(info->version);
			result.push_back('"');
			if (!info->encoding.empty()) {
				result.append({' ','e','n','c','o','d','i','n','g','=','"'});
				result.append(info->encoding);
				result.push_back('"');
			}
			if (info->standalone>=0) {
				result.append({' ','s','t','a','n','d','a','l','o','n','e','=','"'});
				if (info->standalone) result.append({'y','e','s'});
				else result.append({'n','o'});
				result.push_back('"');
			}
			result.push_back('?');
			result.push_back('>');
			result.push_back('\n');
		}
		if (info && info->has_doctype()) {
			result.append(info->doctype);
			result.push_back('\n');
		}
		dump_internal(root,result,indent,indent_char,ensure_ascii,0);
		return result;
	}

	friend std::ostream& operator <<(std::ostream& os,const xml& value) {
		const int indent_step=static_cast<int>(os.width());
		os.width(0);
		const string_t text=value.dump(indent_step>0?indent_step:-1,static_cast<typename string_t::value_type>(os.fill()));
		os.write(reinterpret_cast<const char*>(text.data()),static_cast<std::streamsize>(text.size()));
		return os;
	}
	friend std::istream& operator >>(std::istream& is,xml& value) {
		std::string content((std::istreambuf_iterator<char>(is)),std::istreambuf_iterator<char>());
		value=parse(content);
		return is;
	}
};

_STDEX_DOM_TPL_DEFAULT_DECLARATION
inline typename xml<_Int,_Float,_Boolean,_String,_Array,_Object,_Allocator>::string_t to_string(const xml<_Int,_Float,_Boolean,_String,_Array,_Object,_Allocator>& value) {
	return value.dump();
}

}

_STDEX_DOM_TPL_DEFAULT_DECLARATION
using xml_t=basic_xml::xml<_Int,_Float,_Boolean,_String,_Array,_Object,_Allocator>;
using xml=xml_t<>;
using basic_xml::basic_xml_document_info;
using basic_xml::xml_sax;
using basic_xml::xml_sax_dom_builder;
using basic_xml::xml_sax_acceptor;
using basic_xml::to_string;
using xml_document_info=basic_xml_document_info<std::string>;
 
_STDEX_DOM_TPL_DEFAULT_DECLARATION
using xml_path_t=basic_xml::xml_path<xml_t<_Int,_Float,_Boolean,_String,_Array,_Object,_Allocator>>;
using xml_path=xml_path_t<>;

inline namespace literals {

inline xml_t<> operator ""_xml(const char* s,std::size_t n) {
	return xml_t<>::parse(std::string_view(s,n));
}

inline xml_path_t<> operator ""_xml_path(const char* s,std::size_t n) {
	return xml_path_t<>(std::string_view(s,n));
}

}

}

}

#endif