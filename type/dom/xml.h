//Last Modified At 2026/09/04
//@Version 1.1.0.0
#ifndef _STDEX_TYPE_DOM_XML_H_
#define _STDEX_TYPE_DOM_XML_H_ 1

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <istream>
#include <limits>
#include <map>
#include <memory>
#include <ostream>
#include <regex>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "../../structure/dom.h"//At Least 1.1
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
	//A comment or a processing instruction outside the root element has no place in
	//the tree, so the document information carries it instead of dropping it.
	enum item_type {
		XI_COMMENT,
		XI_PROCINST,
	};
	struct item {
		item_type type=XI_COMMENT;
		_String target{};
		_String content{};
		bool after_doctype=false;
	};

	_String version{};
	_String encoding{};
	int standalone=-1;
	_String doctype{};
	std::vector<item> prolog{};
	std::vector<item> epilog{};

	bool has_declaration() const noexcept {
		return !version.empty();
	}
	bool has_doctype() const noexcept {
		return !doctype.empty();
	}
	bool has_outer_items() const noexcept {
		return !prolog.empty() || !epilog.empty();
	}
	void clear_outer_items() noexcept {
		prolog.clear();
		epilog.clear();
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
	void record_outer_item(typename document_info_t::item_type type,string_t&& target,string_t&& content) {
		if (!info_) return;
		typename document_info_t::item entry;
		entry.type=type;
		entry.target=std::move(target);
		entry.content=std::move(content);
		entry.after_doctype=!info_->doctype.empty();
		if (root_set_) info_->epilog.push_back(std::move(entry));
		else info_->prolog.push_back(std::move(entry));
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
		if (ref_stack_.empty()) {
			record_outer_item(document_info_t::XI_COMMENT,string_t(),std::move(text));
			return true;
		}
		handle_node(_Xml::make_comment(std::move(text)));
		return true;
	}
	bool processing_instruction(string_t& target,string_t& data) override {
		if (ref_stack_.empty()) {
			record_outer_item(document_info_t::XI_PROCINST,std::move(target),std::move(data));
			return true;
		}
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

//XmlPath(XPath 1.0)编译后查询对象。谱系与json_path一致:查询小语言用手工递归下降编译成
//不可变AST,对dom反复求值,不走文档级SLR机器;词法细粒度沿用std::regex。
//覆盖XPath 1.0全部十三条轴(含逆向轴的position语义)、非缩略轴语法、FilterExpr(括号/函数结果
//接谓词与后续路径步)、变量引用、核心函数库全集(id以W3C xml:id标准+惯用id属性替代DTD语义),
//节点集恒按文档序排序并按节点身份去重。
//数据模型绑定xml.h的落点:文档序=children数组序;CDATA按XPath数据模型计为文本;属性不在child轴;
//string-value(元素)=后代文本递归拼接(排除注释/PI)。dom无父指针,父系轴由求值期条目携带的
//祖先链提供;没有文档节点对象,以虚文档条目(node=nullptr)作根节点参与全部轴,select输出时
//落地为传入的根节点指针。相邻文本节点不合并:合并结果在dom中没有承载对象,而select返回树内
//指针,身份无从谈起;此口径与保留CDATA身份的往返设计一致。
//namespace:注册前缀绑定(register_namespace)后名字测试按规范的(URI,local)语义匹配
//(未绑定前缀在求值时抛出);不注册则按词法QName匹配(与xml.h词法名保真口径一致)。
//namespace轴自xmlns/xmlns:*声明合成in-scope集(xmlns=""撤销默认,隐式xml前缀恒在)。
//编译错误抛std::invalid_argument;求值仅在用法错误时抛(select*遇非节点集、未知变量、
//未绑定前缀),数据缺失一律空结果。编译后对象不可变(变量与前缀注册除外),const求值可并发。
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
		AX_DESCENDANT,
		AX_DESCENDANT_OR_SELF,
		AX_PARENT,
		AX_ANCESTOR,
		AX_ANCESTOR_OR_SELF,
		AX_SELF,
		AX_ATTRIBUTE,
		AX_NAMESPACE,
		AX_FOLLOWING_SIBLING,
		AX_PRECEDING_SIBLING,
		AX_FOLLOWING,
		AX_PRECEDING,
	};
	enum node_test_kind : int {
		TK_NAME,
		TK_PREFIX_WILDCARD,//"p:*"
		TK_WILDCARD,
		TK_TEXT,
		TK_COMMENT,
		TK_PI,
		TK_NODE,
	};
	enum entry_category : int {
		EC_NODE,//元素/文本/CDATA/注释/PI/虚文档
		EC_ATTRIBUTE,
		EC_NAMESPACE,
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
		EK_FILTER,
		EK_VARIABLE,
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
		FK_ID,
		FK_LOCAL_NAME,
		FK_NAMESPACE_URI,
		FK_NAME,
		FK_STRING,
		FK_CONCAT,
		FK_STARTS_WITH,
		FK_CONTAINS,
		FK_SUBSTRING_BEFORE,
		FK_SUBSTRING_AFTER,
		FK_SUBSTRING,
		FK_STRING_LENGTH,
		FK_NORMALIZE_SPACE,
		FK_TRANSLATE,
		FK_BOOLEAN,
		FK_NOT,
		FK_TRUE,
		FK_FALSE,
		FK_LANG,
		FK_NUMBER,
		FK_SUM,
		FK_FLOOR,
		FK_CEILING,
		FK_ROUND,
	};
	//XPath 1.0四类型系统。
	enum value_kind : int {
		VK_NODESET,
		VK_BOOLEAN,
		VK_NUMBER,
		VK_STRING,
	};

	struct step {
		axis_kind axis=AX_CHILD;
		node_test_kind test=TK_NAME;
		string_t name{};//名字测试/前缀/processing-instruction目标
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
		std::vector<int> predicates;//EK_FILTER
		double number=0;
		string_t text{};
		location_path path{};//EK_PATH:整条路径;EK_FILTER:初等式之后的续接步
	};
	//结果条目:node=nullptr表示虚文档节点;属性条目指向attributes映射中的值dom并携带键名;
	//namespace条目指向声明它的xmlns属性值dom(隐式xml前缀指向静态承载节点)并携带前缀;
	//ancestors自外向内(front为虚文档nullptr),父系轴/文档序/命名空间解析都依赖它。
	struct entry {
		const base_t* node=nullptr;
		entry_category category=EC_NODE;
		string_t item_name{};
		std::vector<const base_t*> ancestors{};
	};
	struct xpath_value {
		value_kind kind=VK_BOOLEAN;
		std::vector<entry> nodes{};
		bool truth=false;
		double number=0;
		string_t text{};
	};
	struct variable_value {
		value_kind kind=VK_BOOLEAN;
		bool truth=false;
		double number=0;
		string_t text{};
	};

	std::string expression_;
	std::vector<expr_node> pool_;
	int root_expr_=-1;
	std::map<string_t,string_t> namespaces_;//前缀→URI;非空即启用规范语义匹配
	std::map<string_t,variable_value> variables_;

	//---词法细粒度(NCName为XML NameChar的ASCII子集近似,QName=NCName(:NCName)?)---
	static const std::regex& ncname_regex() {
		//The document name lexis accepts UTF-8 names, so the query name lexis has to
		//accept the same byte ranges; a NCName differs from a Name in excluding ':'.
		constexpr char u8_char[]=R"((?:[\xC2-\xDF][\x80-\xBF]|\xE0[\xA0-\xBF][\x80-\xBF]|[\xE1-\xEC][\x80-\xBF]{2}|\xED[\x80-\x9F][\x80-\xBF]|[\xEE-\xEF][\x80-\xBF]{2}|\xF0[\x90-\xBF][\x80-\xBF]{2}|[\xF1-\xF3][\x80-\xBF]{3}|\xF4[\x80-\x8F][\x80-\xBF]{2}))";
		static const std::string start=std::string("(?:[A-Za-z_]|")+u8_char+")";
		static const std::string follow=std::string("(?:[A-Za-z0-9._\\-]|")+u8_char+")*";
		static const std::regex result(start+follow,std::regex::optimize);
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
		return (c>='A' && c<='Z') || (c>='a' && c<='z') || (c>='0' && c<='9') || c=='_' || c=='.' || c=='-';
	}
	bool match_keyword(std::size_t& pos,const char* text) const noexcept {
		const std::size_t length=std::strlen(text);
		if (expression_.size()-pos<length || std::memcmp(expression_.data()+pos,text,length)!=0) return false;
		if (expression_.size()-pos>length && (is_name_char(expression_[pos+length]) || expression_[pos+length]==':')) return false;
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
	//静态可判定的节点集表达式:路径/联合/过滤式/id()。
	bool is_node_set_expression(int index) const noexcept {
		const expr_node& node=pool_[index];
		return node.kind==EK_PATH || node.kind==EK_UNION || node.kind==EK_FILTER || (node.kind==EK_FUNCTION && node.function==FK_ID);
	}
	void require_node_set(int index,std::size_t position,const char* what) const {
		if (!is_node_set_expression(index)) fail(position,std::string(what)+" requires a node set operand");
	}

	//---表达式文法:or<and<等值<关系<加减<乘除模<一元负<联合<路径/过滤式/初等式---
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
		int left=parse_path_expression(pos);
		while (true) {
			skip_blank(pos);
			if (pos>=expression_.size() || expression_[pos]!='|') return left;
			require_node_set(left,left_position,"'|'");
			pos++;
			const std::size_t right_position=pos;
			const int right=parse_path_expression(pos);
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
	static bool axis_by_name(const std::string& name,axis_kind& out) noexcept {
		if (name=="child") out=AX_CHILD;
		else if (name=="descendant") out=AX_DESCENDANT;
		else if (name=="descendant-or-self") out=AX_DESCENDANT_OR_SELF;
		else if (name=="parent") out=AX_PARENT;
		else if (name=="ancestor") out=AX_ANCESTOR;
		else if (name=="ancestor-or-self") out=AX_ANCESTOR_OR_SELF;
		else if (name=="self") out=AX_SELF;
		else if (name=="attribute") out=AX_ATTRIBUTE;
		else if (name=="namespace") out=AX_NAMESPACE;
		else if (name=="following-sibling") out=AX_FOLLOWING_SIBLING;
		else if (name=="preceding-sibling") out=AX_PRECEDING_SIBLING;
		else if (name=="following") out=AX_FOLLOWING;
		else if (name=="preceding") out=AX_PRECEDING;
		else return false;
		return true;
	}
	static step descendant_or_self_step() {
		step result;
		result.axis=AX_DESCENDANT_OR_SELF;
		result.test=TK_NODE;
		return result;
	}
	//PathExpr:LocationPath|FilterExpr(('/'|'//')RelativeLocationPath)?。
	int parse_path_expression(std::size_t& pos) {
		skip_blank(pos);
		if (pos>=expression_.size()) fail(pos,"Unexpected end of expression");
		const char c=expression_[pos];
		if (c=='(' || c=='\'' || c=='"' || c=='$') return parse_filter_expression(pos);
		const bool leading_digit=(c>='0' && c<='9');
		const bool leading_point_digit=(c=='.' && pos+1<expression_.size() && expression_[pos+1]>='0' && expression_[pos+1]<='9');
		if (leading_digit || leading_point_digit) return parse_filter_expression(pos);
		if (c!='/' && c!='@' && c!='.' && c!='*') {
			const std::size_t mark=pos;
			std::string name;
			if (match_regex(pos,ncname_regex(),name)) {
				std::size_t probe=pos;
				skip_blank(probe);
				const bool call=probe<expression_.size() && expression_[probe]=='(';
				pos=mark;
				if (call && !is_node_type_name(name)) {
					std::size_t axis_probe=mark+name.size();
					if (!(axis_probe+1<expression_.size() && expression_[axis_probe]==':' && expression_[axis_probe+1]==':')) return parse_filter_expression(pos);
				}
			} else fail(pos,std::string("Unexpected character '")+c+"'");
		}
		expr_node node;
		node.kind=EK_PATH;
		node.path=parse_location_path(pos);
		return make_node(std::move(node));
	}
	//FilterExpr:初等式+谓词序列+可选续接路径步;谓词或续接要求初等式静态为节点集。
	int parse_filter_expression(std::size_t& pos) {
		const std::size_t primary_position=pos;
		const int primary=parse_primary(pos);
		std::vector<int> predicates;
		while (true) {
			const std::size_t mark=pos;
			skip_blank(pos);
			if (pos>=expression_.size() || expression_[pos]!='[') {
				pos=mark;
				break;
			}
			pos++;
			predicates.push_back(parse_or(pos));
			skip_blank(pos);
			if (pos>=expression_.size() || expression_[pos]!=']') fail(pos,"Expected ']'");
			pos++;
		}
		location_path continuation;
		{
			const std::size_t mark=pos;
			skip_blank(pos);
			if (match_literal(pos,"//")) {
				continuation.steps.push_back(descendant_or_self_step());
				parse_relative_steps(continuation,pos);
			} else if (match_literal(pos,"/")) parse_relative_steps(continuation,pos);
			else pos=mark;
		}
		if (predicates.empty() && continuation.steps.empty()) return primary;
		require_node_set(primary,primary_position,"A filter expression");
		expr_node node;
		node.kind=EK_FILTER;
		node.children={primary};
		node.predicates=std::move(predicates);
		node.path=std::move(continuation);
		return make_node(std::move(node));
	}
	int parse_primary(std::size_t& pos) {
		skip_blank(pos);
		const char c=expression_[pos];
		if (c=='(') {
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
		if (c=='$') {
			pos++;
			std::string name;
			if (!match_regex(pos,ncname_regex(),name)) fail(pos,"Expected a variable name after '$'");
			expr_node node;
			node.kind=EK_VARIABLE;
			node.text=string_t(name.begin(),name.end());
			return make_node(std::move(node));
		}
		if ((c>='0' && c<='9') || c=='.') {
			std::string text;
			if (match_regex(pos,number_regex(),text)) {
				expr_node node;
				node.kind=EK_NUMBER;
				node.number=std::strtod(text.c_str(),nullptr);
				return make_node(std::move(node));
			}
		}
		return parse_function(pos);
	}
	location_path parse_location_path(std::size_t& pos) {
		location_path result;
		if (match_literal(pos,"//")) {
			result.absolute=true;
			result.steps.push_back(descendant_or_self_step());
		} else if (match_literal(pos,"/")) {
			result.absolute=true;
			skip_blank(pos);
			if (pos>=expression_.size() || !is_step_start(expression_[pos])) return result;//"/"单独:文档根
		}
		parse_relative_steps(result,pos);
		return result;
	}
	void parse_relative_steps(location_path& result,std::size_t& pos) {
		while (true) {
			result.steps.push_back(parse_step(pos));
			const std::size_t mark=pos;
			skip_blank(pos);
			if (match_literal(pos,"//")) {
				result.steps.push_back(descendant_or_self_step());
				continue;
			}
			if (match_literal(pos,"/")) continue;
			pos=mark;
			return;
		}
	}
	static bool is_step_start(char c) noexcept {
		//ncname_regex accepts the UTF-8 lead bytes of the document name lexis, so
		//the lookahead that decides whether a step follows "/" must accept them too.
		return c=='@' || c=='.' || c=='*' || c=='_' || (c>='A' && c<='Z') || (c>='a' && c<='z') || static_cast<unsigned char>(c)>=0xC2;
	}
	step parse_step(std::size_t& pos) {
		skip_blank(pos);
		if (pos>=expression_.size()) fail(pos,"Expected a location step");
		step result;
		const char c=expression_[pos];
		if (c=='.') {//缩略:"."=self::node(),".."=parent::node()
			pos++;
			if (pos<expression_.size() && expression_[pos]=='.') {
				pos++;
				result.axis=AX_PARENT;
			} else result.axis=AX_SELF;
			result.test=TK_NODE;
			parse_predicates(result,pos);
			return result;
		}
		if (c=='@') {//缩略:"@"=attribute::
			pos++;
			result.axis=AX_ATTRIBUTE;
			parse_node_test(result,pos);
			parse_predicates(result,pos);
			return result;
		}
		if (c!='*') {//探测非缩略轴"axis::"
			const std::size_t mark=pos;
			std::string name;
			if (match_regex(pos,ncname_regex(),name)) {
				if (match_literal(pos,"::")) {
					if (!axis_by_name(name,result.axis)) fail(mark,"Unknown axis '"+name+"'");
				} else pos=mark;
			}
		}
		parse_node_test(result,pos);
		parse_predicates(result,pos);
		return result;
	}
	//NodeTest:'*'|NCName':''*'|QName|节点类型测试。
	void parse_node_test(step& result,std::size_t& pos) {
		skip_blank(pos);
		if (pos>=expression_.size()) fail(pos,"Expected a node test");
		if (expression_[pos]=='*') {
			pos++;
			result.test=TK_WILDCARD;
			return;
		}
		std::string name;
		if (!match_regex(pos,ncname_regex(),name)) fail(pos,std::string("Unexpected character '")+expression_[pos]+"' in node test");
		if (pos<expression_.size() && expression_[pos]==':' && (pos+1>=expression_.size() || expression_[pos+1]!=':')) {
			pos++;
			if (pos<expression_.size() && expression_[pos]=='*') {
				pos++;
				result.test=TK_PREFIX_WILDCARD;
				result.name=string_t(name.begin(),name.end());
				return;
			}
			std::string local;
			if (!match_regex(pos,ncname_regex(),local)) fail(pos,"Expected a local name after ':'");
			result.test=TK_NAME;
			const std::string qualified=name+":"+local;
			result.name=string_t(qualified.begin(),qualified.end());
			return;
		}
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
			return;
		}
		result.test=TK_NAME;
		result.name=string_t(name.begin(),name.end());
	}
	void parse_predicates(step& result,std::size_t& pos) {
		while (true) {
			const std::size_t mark=pos;
			skip_blank(pos);
			if (pos>=expression_.size() || expression_[pos]!='[') {
				pos=mark;
				return;
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
		if (!match_regex(pos,ncname_regex(),name)) fail(pos,std::string("Unexpected character '")+expression_[pos]+"'");
		function_kind function;
		std::size_t minimum_arguments=0;
		std::size_t maximum_arguments=0;
		if (name=="position") function=FK_POSITION;
		else if (name=="last") function=FK_LAST;
		else if (name=="count") {
			function=FK_COUNT;
			minimum_arguments=1;
			maximum_arguments=1;
		} else if (name=="id") {
			function=FK_ID;
			minimum_arguments=1;
			maximum_arguments=1;
		} else if (name=="local-name") {
			function=FK_LOCAL_NAME;
			maximum_arguments=1;
		} else if (name=="namespace-uri") {
			function=FK_NAMESPACE_URI;
			maximum_arguments=1;
		} else if (name=="name") {
			function=FK_NAME;
			maximum_arguments=1;
		} else if (name=="string") {
			function=FK_STRING;
			maximum_arguments=1;
		} else if (name=="concat") {
			function=FK_CONCAT;
			minimum_arguments=2;
			maximum_arguments=static_cast<std::size_t>(-1);
		} else if (name=="starts-with") {
			function=FK_STARTS_WITH;
			minimum_arguments=2;
			maximum_arguments=2;
		} else if (name=="contains") {
			function=FK_CONTAINS;
			minimum_arguments=2;
			maximum_arguments=2;
		} else if (name=="substring-before") {
			function=FK_SUBSTRING_BEFORE;
			minimum_arguments=2;
			maximum_arguments=2;
		} else if (name=="substring-after") {
			function=FK_SUBSTRING_AFTER;
			minimum_arguments=2;
			maximum_arguments=2;
		} else if (name=="substring") {
			function=FK_SUBSTRING;
			minimum_arguments=2;
			maximum_arguments=3;
		} else if (name=="string-length") {
			function=FK_STRING_LENGTH;
			maximum_arguments=1;
		} else if (name=="normalize-space") {
			function=FK_NORMALIZE_SPACE;
			maximum_arguments=1;
		} else if (name=="translate") {
			function=FK_TRANSLATE;
			minimum_arguments=3;
			maximum_arguments=3;
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
		else if (name=="lang") {
			function=FK_LANG;
			minimum_arguments=1;
			maximum_arguments=1;
		} else if (name=="number") {
			function=FK_NUMBER;
			maximum_arguments=1;
		} else if (name=="sum") {
			function=FK_SUM;
			minimum_arguments=1;
			maximum_arguments=1;
		} else if (name=="floor") {
			function=FK_FLOOR;
			minimum_arguments=1;
			maximum_arguments=1;
		} else if (name=="ceiling") {
			function=FK_CEILING;
			minimum_arguments=1;
			maximum_arguments=1;
		} else if (name=="round") {
			function=FK_ROUND;
			minimum_arguments=1;
			maximum_arguments=1;
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
		if (function==FK_SUM) require_node_set(arguments[0],name_position,"sum()");
		expr_node node;
		node.kind=EK_FUNCTION;
		node.function=function;
		node.children=std::move(arguments);
		return make_node(std::move(node));
	}

	//---XPath数据模型---
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
	static string_t xml_namespace_uri() {
		static const char uri[]="http://www.w3.org/XML/1998/namespace";
		return string_t(uri,uri+sizeof(uri)-1);
	}
	//隐式xml前缀的承载节点(namespace轴条目须指向真实dom)。
	static const base_t& xml_namespace_backing() {
		static const base_t instance(xml_namespace_uri());
		return instance;
	}
	static void scalar_text(const base_t& node,string_t& out) {
		switch (static_cast<int>(node.type())) {
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
		if (item.category!=EC_NODE) scalar_text(node,result);
		else if (_Xml::is_element(node)) element_text(node,result);
		else if (_Xml::is_cdata(node) || _Xml::is_comment(node)) result=_Xml::text_content(node);
		else if (_Xml::is_processing_instruction(node)) result=_Xml::pi_data(node);
		else scalar_text(node,result);
		return result;
	}

	//---命名空间解析(经祖先链查xmlns/xmlns:*声明)---
	static string_t make_string(const char* text) {
		return string_t(text,text+std::strlen(text));
	}
	static bool split_qname(const string_t& qualified,string_t& prefix,string_t& local) {
		const std::size_t colon=qualified.find(':');
		if (colon==string_t::npos) {
			prefix.clear();
			local=qualified;
			return false;
		}
		prefix=qualified.substr(0,colon);
		local=qualified.substr(colon+1);
		return true;
	}
	static const base_t* find_attribute(const base_t& element,const string_t& key) {
		const auto& table=_Xml::attributes(element);
		auto it=table.find(key);
		return it==table.end()?nullptr:&it->second;
	}
	static string_t attribute_text(const base_t& value) {
		string_t result;
		scalar_text(value,result);
		return result;
	}
	//沿self→祖先解析前缀:prefix空查默认xmlns;xml前缀恒定;未声明返回false。
	static bool resolve_element_prefix(const base_t* element,const std::vector<const base_t*>& ancestors,const string_t& prefix,string_t& uri) {
		if (prefix==make_string("xml")) {
			uri=xml_namespace_uri();
			return true;
		}
		const string_t key=prefix.empty()?make_string("xmlns"):make_string("xmlns:")+prefix;
		const base_t* current=element;
		std::size_t depth=ancestors.size();
		while (true) {
			if (current && _Xml::is_element(*current)) {
				const base_t* declared=find_attribute(*current,key);
				if (declared) {
					uri=attribute_text(*declared);
					if (prefix.empty() && uri.empty()) return false;//xmlns=""撤销默认
					return true;
				}
			}
			if (depth==0) return false;
			depth--;
			current=ancestors[depth];
		}
	}
	//条目的规范(URI,local):元素受默认命名空间;属性/namespace节点无默认;非元素节点无名。
	bool entry_expanded_name(const entry& item,string_t& uri,string_t& local) const {
		uri.clear();
		local.clear();
		if (!item.node) return false;
		if (item.category==EC_NAMESPACE) {
			local=item.item_name;
			return true;
		}
		if (item.category==EC_ATTRIBUTE) {
			string_t prefix;
			split_qname(item.item_name,prefix,local);
			if (!prefix.empty()) resolve_element_prefix(item.ancestors.empty()?nullptr:item.ancestors.back(),item.ancestors.empty()?std::vector<const base_t*>():std::vector<const base_t*>(item.ancestors.begin(),item.ancestors.end()-1),prefix,uri);
			return true;
		}
		if (_Xml::is_element(*item.node)) {
			string_t prefix;
			split_qname(_Xml::name(*item.node),prefix,local);
			resolve_element_prefix(item.node,item.ancestors,prefix,uri);
			return true;
		}
		if (_Xml::is_processing_instruction(*item.node)) {
			local=_Xml::pi_target(*item.node);
			return true;
		}
		return false;
	}
	//in-scope命名空间合成:self→祖先,内层优先,xmlns=""撤销默认,隐式xml前缀恒在;前缀排序输出。
	std::vector<std::pair<string_t,const base_t*>> in_scope_namespaces(const entry& element) const {
		std::vector<std::pair<string_t,const base_t*>> result;
		std::vector<string_t> seen;
		auto visit=[&](const base_t& node){
			if (!_Xml::is_element(node)) return;
			for (const auto& it:_Xml::attributes(node)) {
				string_t prefix;
				const string_t xmlns=make_string("xmlns");
				if (it.first==xmlns) prefix.clear();
				else if (it.first.size()>6 && it.first.compare(0,6,make_string("xmlns:"))==0) prefix=it.first.substr(6);
				else continue;
				bool duplicate=false;
				for (const auto& jt:seen) {
					if (jt==prefix) {
						duplicate=true;
						break;
					}
				}
				if (duplicate) continue;
				seen.push_back(prefix);
				const string_t uri=attribute_text(it.second);
				if (prefix.empty() && uri.empty()) continue;//撤销默认:占位不输出
				result.emplace_back(prefix,&it.second);
			}
		};
		if (element.node) visit(*element.node);
		for (std::size_t i=element.ancestors.size();i>0;i--) {
			if (element.ancestors[i-1]) visit(*element.ancestors[i-1]);
		}
		{
			const string_t xml_prefix=make_string("xml");
			bool duplicate=false;
			for (const auto& it:seen) {
				if (it==xml_prefix) {
					duplicate=true;
					break;
				}
			}
			if (!duplicate) result.emplace_back(xml_prefix,&xml_namespace_backing());
		}
		std::sort(result.begin(),result.end(),[](const std::pair<string_t,const base_t*>& lhs,const std::pair<string_t,const base_t*>& rhs){
			return lhs.first<rhs.first;
		});
		return result;
	}

	//---数值处理---
	static double string_to_number(const string_t& text) {
		const std::string narrow(text.begin(),text.end());
		if (!std::regex_match(narrow,numeric_string_regex())) return std::numeric_limits<double>::quiet_NaN();
		return std::strtod(narrow.c_str(),nullptr);
	}
	//XPath的string(number):NaN/±Infinity字面,整数不带小数点,其余最短往返。
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
		if (std::strtod(buffer,nullptr)!=value) length=std::snprintf(buffer,sizeof(buffer),"%.16g",value);
		if (std::strtod(buffer,nullptr)!=value) length=std::snprintf(buffer,sizeof(buffer),"%.17g",value);
		result.append(buffer,buffer+length);
		return result;
	}
	//round()按规范=floor(x+0.5),含NaN/±Inf透传与[-0.5,0)→-0。
	static double round_number(double value) noexcept {
		if (std::isnan(value) || std::isinf(value)) return value;
		if (value>=-0.5 && value<0) return -0.0;
		return std::floor(value+0.5);
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

	//---码点级字符串处理(string_t为UTF-8字节串)---
	static std::vector<std::size_t> codepoint_offsets(const string_t& text) {
		std::vector<std::size_t> result;
		for (std::size_t i=0;i<text.size();i++) {
			if ((static_cast<unsigned char>(text[i])&0xC0)!=0x80) result.push_back(i);
		}
		result.push_back(text.size());
		return result;
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
	//substring()按规范:位置1起,起止经round(),NaN比较即失败(得空串),按码点切片。
	static string_t substring_text(const string_t& text,double start,double length,bool has_length) {
		const std::vector<std::size_t> offsets=codepoint_offsets(text);
		const double first=round_number(start);
		double limit=std::numeric_limits<double>::infinity();
		if (has_length) {
			const double rounded=round_number(length);
			limit=first+rounded;
		}
		string_t result;
		const std::size_t total=offsets.size()-1;
		for (std::size_t i=0;i<total;i++) {
			const double position=static_cast<double>(i+1);
			if (position>=first && position<limit) result.append(text,offsets[i],offsets[i+1]-offsets[i]);
		}
		return result;
	}
	//translate()按码点映射:from第一次出现优先,to不足则删除。
	static string_t translate_text(const string_t& text,const string_t& from,const string_t& to) {
		const std::vector<std::size_t> from_offsets=codepoint_offsets(from);
		const std::vector<std::size_t> to_offsets=codepoint_offsets(to);
		const std::size_t from_count=from_offsets.size()-1;
		const std::size_t to_count=to_offsets.size()-1;
		const std::vector<std::size_t> offsets=codepoint_offsets(text);
		string_t result;
		for (std::size_t i=0;i+1<offsets.size();i++) {
			const std::size_t begin=offsets[i];
			const std::size_t size=offsets[i+1]-begin;
			std::size_t found=from_count;
			for (std::size_t j=0;j<from_count;j++) {
				if (from_offsets[j+1]-from_offsets[j]==size && from.compare(from_offsets[j],size,text,begin,size)==0) {
					found=j;
					break;
				}
			}
			if (found==from_count) result.append(text,begin,size);
			else if (found<to_count) result.append(to,to_offsets[found],to_offsets[found+1]-to_offsets[found]);
		}
		return result;
	}
	static char ascii_lower(char c) noexcept {
		return (c>='A' && c<='Z')?static_cast<char>(c-'A'+'a'):c;
	}

	//---轴求值(条目携带祖先链)---
	std::vector<entry> children_entries(const entry& parent,const base_t& root) const {
		std::vector<entry> result;
		if (!parent.node) {//虚文档节点:唯一子=根
			result.push_back(root_entry(root));
			return result;
		}
		if (parent.category!=EC_NODE || !_Xml::is_element(*parent.node)) return result;
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
		if (!parent.node || parent.category!=EC_NODE || !_Xml::is_element(*parent.node)) return result;
		std::vector<const base_t*> chain=parent.ancestors;
		chain.push_back(parent.node);
		for (const auto& it:_Xml::attributes(*parent.node)) {
			entry attribute;
			attribute.node=&it.second;
			attribute.category=EC_ATTRIBUTE;
			attribute.item_name=it.first;
			attribute.ancestors=chain;
			result.push_back(std::move(attribute));
		}
		return result;
	}
	std::vector<entry> namespace_entries(const entry& parent) const {
		std::vector<entry> result;
		if (!parent.node || parent.category!=EC_NODE || !_Xml::is_element(*parent.node)) return result;
		std::vector<const base_t*> chain=parent.ancestors;
		chain.push_back(parent.node);
		for (const auto& it:in_scope_namespaces(parent)) {
			entry item;
			item.node=it.second;
			item.category=EC_NAMESPACE;
			item.item_name=it.first;
			item.ancestors=chain;
			result.push_back(std::move(item));
		}
		return result;
	}
	void collect_descendants(const entry& item,const base_t& root,std::vector<entry>& out,bool include_self) const {
		if (include_self) out.push_back(item);
		for (auto& it:children_entries(item,root)) collect_descendants(it,root,out,true);
	}
	entry parent_entry(const entry& item) const {
		entry result;
		result.node=item.ancestors.back();
		result.ancestors.assign(item.ancestors.begin(),item.ancestors.end()-1);
		return result;
	}
	//同胞定位:经父条目的children按指针身份找到自身下标;属性/namespace节点无同胞。
	bool locate_sibling_context(const entry& item,const base_t& root,std::vector<entry>& siblings,std::size_t& index) const {
		if (item.category!=EC_NODE || item.ancestors.empty()) return false;
		siblings=children_entries(parent_entry(item),root);
		for (std::size_t i=0;i<siblings.size();i++) {
			if (siblings[i].node==item.node) {
				index=i;
				return true;
			}
		}
		return false;
	}
	//文档序键:自根的(种类,下标)对序列;namespace<attribute<child;虚文档为空键。
	std::vector<long long> order_key(const entry& item,const base_t& root) const {
		std::vector<long long> result;
		std::vector<const base_t*> chain=item.ancestors;
		if (item.category==EC_NODE) {
			if (item.node) chain.push_back(item.node);
		} else chain.push_back(nullptr);//占位:属性/namespace的元素链止于ancestors
		const base_t* parent=nullptr;
		bool parent_is_document=true;
		for (std::size_t i=0;i<chain.size();i++) {
			const base_t* current=chain[i];
			if (i==0) {
				if (current==nullptr) {
					parent=nullptr;
					parent_is_document=true;
					continue;
				}
				//链首非虚文档:传入节点即上下文根,视作文档唯一子
				result.push_back(2);
				result.push_back(0);
				parent=current;
				parent_is_document=false;
				continue;
			}
			if (item.category!=EC_NODE && i+1==chain.size()) break;
			if (parent_is_document) {
				result.push_back(2);
				result.push_back(0);
			} else {
				long long index=0;
				const auto& children=_Xml::children(*parent);
				for (auto it=children.begin();it!=children.end();it++,index++) {
					if (&*it==current) break;
				}
				result.push_back(2);
				result.push_back(index);
			}
			parent=current;
			parent_is_document=false;
		}
		if (item.category==EC_ATTRIBUTE) {
			const base_t* element=item.ancestors.back();
			long long index=0;
			for (const auto& it:_Xml::attributes(*element)) {
				if (it.first==item.item_name) break;
				index++;
			}
			result.push_back(1);
			result.push_back(index);
		} else if (item.category==EC_NAMESPACE) {
			entry element;
			element.node=item.ancestors.back();
			element.ancestors.assign(item.ancestors.begin(),item.ancestors.end()-1);
			long long index=0;
			for (const auto& it:in_scope_namespaces(element)) {
				if (it.first==item.item_name) break;
				index++;
			}
			result.push_back(0);
			result.push_back(index);
		}
		return result;
	}
	static bool key_is_prefix(const std::vector<long long>& prefix,const std::vector<long long>& key) noexcept {
		if (prefix.size()>key.size()) return false;
		for (std::size_t i=0;i<prefix.size();i++) {
			if (prefix[i]!=key[i]) return false;
		}
		return true;
	}
	//节点集不变式:文档序+按身份去重(排序后同键即同身份)。
	void sort_document_order(std::vector<entry>& nodes,const base_t& root) const {
		std::vector<std::pair<std::vector<long long>,entry>> keyed;
		keyed.reserve(nodes.size());
		for (auto& it:nodes) keyed.emplace_back(order_key(it,root),std::move(it));
		std::stable_sort(keyed.begin(),keyed.end(),[](const std::pair<std::vector<long long>,entry>& lhs,const std::pair<std::vector<long long>,entry>& rhs){
			return lhs.first<rhs.first;
		});
		nodes.clear();
		for (std::size_t i=0;i<keyed.size();i++) {
			if (i>0 && keyed[i].first==keyed[i-1].first) continue;
			nodes.push_back(std::move(keyed[i].second));
		}
	}
	bool node_test(const entry& item,const step& current) const {
		const bool attribute_axis=(current.axis==AX_ATTRIBUTE);
		const bool namespace_axis=(current.axis==AX_NAMESPACE);
		if (attribute_axis) {
			if (item.category!=EC_ATTRIBUTE) return false;
		} else if (namespace_axis) {
			if (item.category!=EC_NAMESPACE) return false;
		} else if (item.category!=EC_NODE) return false;
		switch (current.test) {
			case TK_NODE: return true;
			case TK_WILDCARD: {//主节点类型:attribute轴=属性,namespace轴=namespace,其余=元素
				if (attribute_axis || namespace_axis) return true;
				return item.node && _Xml::is_element(*item.node);
			}
			case TK_NAME: {
				if (namespace_axis) return item.item_name==current.name;//namespace节点名=前缀
				if (attribute_axis) return match_name(item,current.name,false);
				return item.node && _Xml::is_element(*item.node) && match_name(item,current.name,true);
			}
			case TK_PREFIX_WILDCARD: {
				if (namespace_axis) return false;
				if (attribute_axis) return match_prefix(item,current.name,false);
				return item.node && _Xml::is_element(*item.node) && match_prefix(item,current.name,true);
			}
			case TK_TEXT: return item.category==EC_NODE && item.node && is_text_node(*item.node);
			case TK_COMMENT: return item.category==EC_NODE && item.node && _Xml::is_comment(*item.node);
			case TK_PI: {
				if (item.category!=EC_NODE || !item.node || !_Xml::is_processing_instruction(*item.node)) return false;
				return !current.has_target || _Xml::pi_target(*item.node)==current.name;
			}
			default: return false;
		}
	}
	//名字测试:注册了前缀绑定→规范(URI,local)匹配(未绑定前缀抛出;未加前缀=空命名空间);
	//未注册→词法QName匹配(与xml.h词法名保真口径一致)。
	bool match_name(const entry& item,const string_t& test,bool element) const {
		if (namespaces_.empty()) {
			if (element) return _Xml::name(*item.node)==test;
			return item.item_name==test;
		}
		string_t test_prefix;
		string_t test_local;
		split_qname(test,test_prefix,test_local);
		string_t test_uri;
		if (!test_prefix.empty()) {
			if (!lookup_binding(test_prefix,test_uri)) throw std::invalid_argument("Unbound namespace prefix '"+std::string(test_prefix.begin(),test_prefix.end())+"' in XmlPath '"+expression_+"'");
		}
		string_t uri;
		string_t local;
		entry_expanded_name(item,uri,local);
		if (!element) {//属性无默认命名空间
			return local==test_local && uri==test_uri;
		}
		return local==test_local && uri==test_uri;
	}
	bool match_prefix(const entry& item,const string_t& prefix,bool element) const {
		if (namespaces_.empty()) {//词法:限定名以"prefix:"起始
			const string_t& name=element?_Xml::name(*item.node):item.item_name;
			return name.size()>prefix.size() && name.compare(0,prefix.size(),prefix)==0 && name[prefix.size()]==':';
		}
		string_t test_uri;
		if (!lookup_binding(prefix,test_uri)) throw std::invalid_argument("Unbound namespace prefix '"+std::string(prefix.begin(),prefix.end())+"' in XmlPath '"+expression_+"'");
		string_t uri;
		string_t local;
		entry_expanded_name(item,uri,local);
		return uri==test_uri;
	}
	bool lookup_binding(const string_t& prefix,string_t& uri) const {
		if (prefix==make_string("xml")) {
			uri=xml_namespace_uri();
			return true;
		}
		auto it=namespaces_.find(prefix);
		if (it==namespaces_.end()) return false;
		uri=it->second;
		return true;
	}
	//轴产出按轴序(逆向轴nearest-first),谓词position取轴序;并集后统一回文档序。
	std::vector<entry> axis_results(const entry& context,const step& current,const base_t& root) const {
		std::vector<entry> result;
		switch (current.axis) {
			case AX_CHILD: {
				for (auto& it:children_entries(context,root)) {
					if (node_test(it,current)) result.push_back(std::move(it));
				}
				break;
			}
			case AX_DESCENDANT:
			case AX_DESCENDANT_OR_SELF: {
				std::vector<entry> all;
				collect_descendants(context,root,all,current.axis==AX_DESCENDANT_OR_SELF);
				for (auto& it:all) {
					if (node_test(it,current)) result.push_back(std::move(it));
				}
				break;
			}
			case AX_PARENT: {
				if (!context.ancestors.empty()) {
					entry parent=parent_entry(context);
					if (node_test(parent,current)) result.push_back(std::move(parent));
				}
				break;
			}
			case AX_ANCESTOR:
			case AX_ANCESTOR_OR_SELF: {
				if (current.axis==AX_ANCESTOR_OR_SELF && node_test(context,current)) result.push_back(context);
				entry walker=context;
				while (!walker.ancestors.empty()) {
					walker=parent_entry(walker);
					if (node_test(walker,current)) result.push_back(walker);
				}
				break;
			}
			case AX_SELF: {
				if (node_test(context,current)) result.push_back(context);
				break;
			}
			case AX_ATTRIBUTE: {
				for (auto& it:attribute_entries(context)) {
					if (node_test(it,current)) result.push_back(std::move(it));
				}
				break;
			}
			case AX_NAMESPACE: {
				for (auto& it:namespace_entries(context)) {
					if (node_test(it,current)) result.push_back(std::move(it));
				}
				break;
			}
			case AX_FOLLOWING_SIBLING:
			case AX_PRECEDING_SIBLING: {
				std::vector<entry> siblings;
				std::size_t index=0;
				if (!locate_sibling_context(context,root,siblings,index)) break;
				if (current.axis==AX_FOLLOWING_SIBLING) {
					for (std::size_t i=index+1;i<siblings.size();i++) {
						if (node_test(siblings[i],current)) result.push_back(std::move(siblings[i]));
					}
				} else {
					for (std::size_t i=index;i>0;i--) {
						if (node_test(siblings[i-1],current)) result.push_back(std::move(siblings[i-1]));
					}
				}
				break;
			}
			case AX_FOLLOWING:
			case AX_PRECEDING: {//全树扫描+文档序键过滤(排除后代/祖先),属性与namespace节点不在此二轴
				const std::vector<long long> context_key=order_key(context,root);
				std::vector<entry> all;
				collect_descendants(document_entry(),root,all,true);
				for (auto& it:all) {
					const std::vector<long long> key=order_key(it,root);
					if (current.axis==AX_FOLLOWING) {
						if (key<=context_key || key_is_prefix(context_key,key)) continue;
					} else {
						if (key>=context_key || key_is_prefix(key,context_key)) continue;
					}
					if (node_test(it,current)) result.push_back(std::move(it));
				}
				if (current.axis==AX_PRECEDING) std::reverse(result.begin(),result.end());
				break;
			}
			default: break;
		}
		return result;
	}
	//谓词按"逐上下文节点"应用:position/size取自同一上下文节点的轴序候选列表。
	std::vector<entry> filter_predicates(std::vector<entry> selected,const std::vector<int>& predicates,const base_t& root) const {
		for (const int predicate:predicates) {
			std::vector<entry> kept;
			const std::size_t size=selected.size();
			for (std::size_t i=0;i<size;i++) {
				const xpath_value value=eval_expr(predicate,selected[i],i+1,size,root);
				const bool keep=(value.kind==VK_NUMBER)?(value.number==static_cast<double>(i+1)):to_boolean(value);
				if (keep) kept.push_back(std::move(selected[i]));
			}
			selected=std::move(kept);
		}
		return selected;
	}
	std::vector<entry> eval_steps(const std::vector<step>& steps,std::vector<entry> nodes,const base_t& root) const {
		for (const auto& current:steps) {
			std::vector<entry> next;
			for (const auto& it:nodes) {
				for (auto& jt:filter_predicates(axis_results(it,current,root),current.predicates,root)) next.push_back(std::move(jt));
			}
			sort_document_order(next,root);
			nodes=std::move(next);
		}
		return nodes;
	}
	std::vector<entry> eval_path(const location_path& path,const entry& context,const base_t& root) const {
		std::vector<entry> nodes;
		if (path.absolute) nodes.push_back(document_entry());
		else nodes.push_back(context);
		return eval_steps(path.steps,std::move(nodes),root);
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
	//id():xml.h无DTD(既定结论),按W3C xml:id标准取xml:id,并以惯用id属性作退路;
	//参数为节点集时逐节点string-value,否则整串,按空白切词,全文档序扫描。
	void collect_id_tokens(const string_t& text,std::vector<string_t>& out) const {
		string_t current;
		for (auto it:text) {
			if (it==' ' || it=='\t' || it=='\n' || it=='\r') {
				if (!current.empty()) {
					out.push_back(current);
					current.clear();
				}
				continue;
			}
			current.push_back(it);
		}
		if (!current.empty()) out.push_back(current);
	}
	std::vector<entry> eval_id(const xpath_value& argument,const base_t& root) const {
		std::vector<string_t> tokens;
		if (argument.kind==VK_NODESET) {
			for (const auto& it:argument.nodes) collect_id_tokens(string_value(it,root),tokens);
		} else collect_id_tokens(to_string_value(argument,root),tokens);
		std::vector<entry> result;
		if (tokens.empty()) return result;
		std::vector<entry> all;
		collect_descendants(document_entry(),root,all,true);
		const string_t xml_id_key=make_string("xml:id");
		const string_t id_key=make_string("id");
		for (auto& it:all) {
			if (!it.node || it.category!=EC_NODE || !_Xml::is_element(*it.node)) continue;
			const base_t* value=find_attribute(*it.node,xml_id_key);
			if (!value) value=find_attribute(*it.node,id_key);
			if (!value) continue;
			const string_t text=attribute_text(*value);
			for (const auto& jt:tokens) {
				if (jt==text) {
					result.push_back(std::move(it));
					break;
				}
			}
		}
		return result;
	}
	//lang():nearest xml:lang,大小写不敏感,允许"-"细分前缀匹配。
	bool eval_lang(const entry& context,const string_t& wanted) const {
		const base_t* element=nullptr;
		std::size_t depth=context.ancestors.size();
		if (context.category==EC_NODE && context.node && _Xml::is_element(*context.node)) element=context.node;
		const string_t key=make_string("xml:lang");
		string_t declared;
		bool found=false;
		while (true) {
			if (element) {
				const base_t* value=find_attribute(*element,key);
				if (value) {
					declared=attribute_text(*value);
					found=true;
					break;
				}
			}
			if (depth==0) break;
			depth--;
			element=context.ancestors[depth];
			if (element && !_Xml::is_element(*element)) element=nullptr;
		}
		if (!found) return false;
		if (declared.size()<wanted.size()) return false;
		for (std::size_t i=0;i<wanted.size();i++) {
			if (ascii_lower(declared[i])!=ascii_lower(wanted[i])) return false;
		}
		return declared.size()==wanted.size() || declared[wanted.size()]=='-';
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
				sort_document_order(result.nodes,root);
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
			case EK_FILTER: {//初等式节点集(文档序)→谓词(文档序即child轴口径)→续接步
				xpath_value primary=eval_expr(node.children[0],context,position,size,root);
				result.kind=VK_NODESET;
				result.nodes=filter_predicates(std::move(primary.nodes),node.predicates,root);
				if (!node.path.steps.empty()) result.nodes=eval_steps(node.path.steps,std::move(result.nodes),root);
				break;
			}
			case EK_VARIABLE: {
				auto it=variables_.find(node.text);
				if (it==variables_.end()) throw std::invalid_argument("Unknown variable '$"+std::string(node.text.begin(),node.text.end())+"' in XmlPath '"+expression_+"'");
				result.kind=it->second.kind;
				result.truth=it->second.truth;
				result.number=it->second.number;
				result.text=it->second.text;
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
					case FK_ID: {
						result.kind=VK_NODESET;
						result.nodes=eval_id(eval_expr(node.children[0],context,position,size,root),root);
						break;
					}
					case FK_LOCAL_NAME:
					case FK_NAMESPACE_URI:
					case FK_NAME: {
						result.kind=VK_STRING;
						entry target=context;
						bool has_target=true;
						if (!node.children.empty()) {
							xpath_value argument=eval_expr(node.children[0],context,position,size,root);
							if (argument.nodes.empty()) has_target=false;
							else target=argument.nodes[0];
						}
						if (has_target) {
							if (node.function==FK_NAME) {
								if (target.node) {
									if (target.category!=EC_NODE) result.text=target.item_name;
									else if (_Xml::is_element(*target.node)) result.text=_Xml::name(*target.node);
									else if (_Xml::is_processing_instruction(*target.node)) result.text=_Xml::pi_target(*target.node);
								}
							} else {
								string_t uri;
								string_t local;
								entry_expanded_name(target,uri,local);
								result.text=(node.function==FK_LOCAL_NAME)?local:uri;
							}
						}
						break;
					}
					case FK_STRING: {
						result.kind=VK_STRING;
						if (node.children.empty()) result.text=string_value(context,root);
						else result.text=to_string_value(eval_expr(node.children[0],context,position,size,root),root);
						break;
					}
					case FK_CONCAT: {
						result.kind=VK_STRING;
						for (const int it:node.children) result.text+=to_string_value(eval_expr(it,context,position,size,root),root);
						break;
					}
					case FK_STARTS_WITH:
					case FK_CONTAINS: {
						const string_t haystack=to_string_value(eval_expr(node.children[0],context,position,size,root),root);
						const string_t needle=to_string_value(eval_expr(node.children[1],context,position,size,root),root);
						result.kind=VK_BOOLEAN;
						if (node.function==FK_CONTAINS) result.truth=haystack.find(needle)!=string_t::npos;
						else result.truth=haystack.size()>=needle.size() && haystack.compare(0,needle.size(),needle)==0;
						break;
					}
					case FK_SUBSTRING_BEFORE:
					case FK_SUBSTRING_AFTER: {
						const string_t haystack=to_string_value(eval_expr(node.children[0],context,position,size,root),root);
						const string_t needle=to_string_value(eval_expr(node.children[1],context,position,size,root),root);
						result.kind=VK_STRING;
						const std::size_t found=haystack.find(needle);
						if (found!=string_t::npos) {
							if (node.function==FK_SUBSTRING_BEFORE) result.text=haystack.substr(0,found);
							else result.text=haystack.substr(found+needle.size());
						}
						break;
					}
					case FK_SUBSTRING: {
						const string_t text=to_string_value(eval_expr(node.children[0],context,position,size,root),root);
						const double start=to_number(eval_expr(node.children[1],context,position,size,root),root);
						double length=0;
						const bool has_length=node.children.size()>2;
						if (has_length) length=to_number(eval_expr(node.children[2],context,position,size,root),root);
						result.kind=VK_STRING;
						result.text=substring_text(text,start,length,has_length);
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
					case FK_TRANSLATE: {
						result.kind=VK_STRING;
						result.text=translate_text(to_string_value(eval_expr(node.children[0],context,position,size,root),root),to_string_value(eval_expr(node.children[1],context,position,size,root),root),to_string_value(eval_expr(node.children[2],context,position,size,root),root));
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
					case FK_LANG: {
						result.kind=VK_BOOLEAN;
						result.truth=eval_lang(context,to_string_value(eval_expr(node.children[0],context,position,size,root),root));
						break;
					}
					case FK_NUMBER: {
						result.kind=VK_NUMBER;
						if (node.children.empty()) result.number=string_to_number(string_value(context,root));
						else result.number=to_number(eval_expr(node.children[0],context,position,size,root),root);
						break;
					}
					case FK_SUM: {
						result.kind=VK_NUMBER;
						result.number=0;
						for (const auto& it:eval_expr(node.children[0],context,position,size,root).nodes) result.number+=string_to_number(string_value(it,root));
						break;
					}
					case FK_FLOOR: {
						result.kind=VK_NUMBER;
						result.number=std::floor(to_number(eval_expr(node.children[0],context,position,size,root),root));
						break;
					}
					case FK_CEILING: {
						result.kind=VK_NUMBER;
						result.number=std::ceil(to_number(eval_expr(node.children[0],context,position,size,root),root));
						break;
					}
					case FK_ROUND: {
						result.kind=VK_NUMBER;
						result.number=round_number(to_number(eval_expr(node.children[0],context,position,size,root),root));
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
	//虚文档条目落地为根节点指针("/"的select结果);namespace条目落地为其承载dom。
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
	xml_path(std::string_view expression,std::initializer_list<std::pair<string_t,string_t>> namespace_bindings) : expression_(expression) {
		for (const auto& it:namespace_bindings) namespaces_.emplace(it.first,it.second);
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

	//前缀绑定:注册后名字测试改按规范(URI,local)语义;xml前缀恒定不可改。
	xml_path& register_namespace(string_t prefix,string_t uri) {
		namespaces_[std::move(prefix)]=std::move(uri);
		return *this;
	}
	xml_path& clear_namespaces() {
		namespaces_.clear();
		return *this;
	}
	//变量绑定:boolean/number/string三型(节点集变量不提供:条目含祖先链,公开API无法合法构造;
	//需要节点集时把该路径写进表达式本身)。
	xml_path& set_variable(string_t name,boolean_t value) {
		variable_value stored;
		stored.kind=VK_BOOLEAN;
		stored.truth=static_cast<bool>(value);
		variables_[std::move(name)]=std::move(stored);
		return *this;
	}
	xml_path& set_variable(string_t name,double value) {
		variable_value stored;
		stored.kind=VK_NUMBER;
		stored.number=value;
		variables_[std::move(name)]=std::move(stored);
		return *this;
	}
	xml_path& set_variable(string_t name,string_t value) {
		variable_value stored;
		stored.kind=VK_STRING;
		stored.text=std::move(value);
		variables_[std::move(name)]=std::move(stored);
		return *this;
	}
	xml_path& clear_variables() {
		variables_.clear();
		return *this;
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

		bool equals(structure::dom_data_type t,const typename base_t::value_t& other) const override {
			if (t!=XDT_ELEMENT) return base_t::value_t::equals(t,other);
			const element_value* right=dynamic_cast<const element_value*>(&other);
			if (!right) return false;
			return name==right->name && attributes==right->attributes && children==right->children;
		}

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

		bool equals(structure::dom_data_type t,const typename base_t::value_t& other) const override {
			if (t!=XDT_CDATA && t!=XDT_COMMENT) return base_t::value_t::equals(t,other);
			const text_value* right=dynamic_cast<const text_value*>(&other);
			if (!right) return false;
			return text==right->text;
		}

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

		bool equals(structure::dom_data_type t,const typename base_t::value_t& other) const override {
			if (t!=XDT_PROCINST) return base_t::value_t::equals(t,other);
			const procinst_value* right=dynamic_cast<const procinst_value*>(&other);
			if (!right) return false;
			return target==right->target && content==right->content;
		}

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

	static std::string hexadecimal_code(unsigned long cp) {
		static const char digits[]="0123456789ABCDEF";
		std::string result;
		for (int shift=12;shift>=0;shift-=4) result.push_back(digits[(cp>>shift)&0xF]);
		for (int shift=28;shift>=16;shift-=4) {
			if ((cp>>shift)&0xF) {
				result.clear();
				for (int inner=28;inner>=0;inner-=4) result.push_back(digits[(cp>>inner)&0xF]);
				while (result.size()>4 && result[0]=='0') result.erase(result.begin());
				break;
			}
		}
		return result;
	}
	//Diagnostics helpers. A printable byte keeps the original "character 'x'"
	//wording, anything else is named by its value instead of being embedded raw.
	static std::string describe_byte(char c) {
		const unsigned char byte=static_cast<unsigned char>(c);
		if (byte>=0x20 && byte<0x7F) return std::string("character '")+c+"'";
		static const char digits[]="0123456789ABCDEF";
		std::string result("byte 0x");
		result.push_back(digits[(byte>>4)&0xF]);
		result.push_back(digits[byte&0xF]);
		return result;
	}
	static std::string describe_position(std::string_view input,std::size_t position) {
		std::size_t line=1;
		std::size_t column=1;
		const std::size_t limit=(position<input.size())?position:input.size();
		for (std::size_t i=0;i<limit;i++) {
			if (input[i]=='\n') {
				line++;
				column=1;
			} else column++;
		}
		return std::string("byte ")+std::to_string(position)+" (line "+std::to_string(line)+", column "+std::to_string(column)+")";
	}
	static const char* symbol_name(xml_symbol symbol) noexcept {
		switch (symbol) {
			case XS_EOF: return "end of input";
			case XS_LT: return "'<'";
			case XS_ETAG_OPEN: return "'</'";
			case XS_GT: return "'>'";
			case XS_EMPTY_CLOSE: return "'/>'";
			case XS_NAME: return "a name";
			case XS_EQ: return "'='";
			case XS_ATTVALUE: return "an attribute value";
			case XS_TEXT: return "character data";
			case XS_CDATA: return "a CDATA section";
			case XS_COMMENT: return "a comment";
			case XS_PI: return "a processing instruction";
			case XS_XMLDECL: return "an xml declaration";
			case XS_DOCTYPE: return "a DOCTYPE declaration";
			default: return "a node";
		}
	}

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

	//XML 1.0 Char production plus UTF-8 well formedness, applied to every raw span
	//so that whatever the parser accepts can also be serialised again.
	static bool validate_characters(const char* first,const char* last,const char*& error_at,std::string& error_message) {
		while (first<last) {
			const unsigned char lead=static_cast<unsigned char>(*first);
			if (lead<0x80) {
				if (lead<0x20 && lead!='\t' && lead!='\n' && lead!='\r') {
					error_at=first;
					error_message="Character U+"+hexadecimal_code(lead)+" is not allowed in XML 1.0";
					return false;
				}
				first++;
				continue;
			}
			unsigned long raw=0;
			std::size_t extra=0;
			if ((lead&0xE0)==0xC0) {
				raw=lead&0x1F;
				extra=1;
			} else if ((lead&0xF0)==0xE0) {
				raw=lead&0x0F;
				extra=2;
			} else if ((lead&0xF8)==0xF0) {
				raw=lead&0x07;
				extra=3;
			} else {
				error_at=first;
				error_message="Invalid UTF-8 lead byte";
				return false;
			}
			if (static_cast<std::size_t>(last-first)<extra+1) {
				error_at=first;
				error_message="Truncated UTF-8 sequence";
				return false;
			}
			for (std::size_t i=1;i<=extra;i++) {
				if ((static_cast<unsigned char>(first[i])&0xC0)!=0x80) {
					error_at=first+i;
					error_message="Invalid UTF-8 continuation byte";
					return false;
				}
				raw=(raw<<6)|(static_cast<unsigned char>(first[i])&0x3F);
			}
			if ((extra==1 && raw<0x80) || (extra==2 && raw<0x800) || (extra==3 && raw<0x10000)) {
				error_at=first;
				error_message="Overlong UTF-8 sequence";
				return false;
			}
			if (raw>0x10FFFF || (raw>=0xD800 && raw<=0xDFFF) || raw==0xFFFE || raw==0xFFFF) {
				error_at=first;
				error_message="Character U+"+hexadecimal_code(raw)+" is not allowed in XML 1.0";
				return false;
			}
			first+=extra+1;
		}
		return true;
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
	static bool decode_text(const char* first,const char* last,string_t& out,bool attribute_mode,const char*& error_at,std::string& error_message) {
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
				error_at=first;
				error_message="Unterminated entity reference";
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
					error_at=first;
					error_message="Invalid character reference '&"+entity+";'";
					return false;
				}
				append_codepoint(out,cp);
			} else {
				error_at=first;
				error_message="Undefined entity '&"+entity+";', there is no DTD support";
				return false;
			}
			first=entity_end+1;
		}
		return true;
	}
	static bool decode_declaration(const std::string& body,xml_token& token,std::string& error_message) {
		std::smatch match;
		if (!std::regex_search(body,match,decl_version_regex())) {
			error_message="Xml declaration requires a version pseudo-attribute";
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
		error_message="Unterminated DOCTYPE declaration";
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
		const char* error_at=nullptr;
		auto fail=[&](const char* where,const std::string& message){
			error_position=static_cast<std::size_t>(where-input.data());
			error_message=message;
			return false;
		};
		auto check=[&](const char* span_first,const char* span_last){
			return validate_characters(span_first,span_last,error_at,error_message);
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
					if (!check(match[0].first+1,match[0].second-1)) return fail(error_at,error_message);
					if (!decode_text(match[0].first+1,match[0].second-1,token.text,true,error_at,error_message)) return fail(error_at,error_message);
					first=match[0].second;
				} else return fail(first,"Unexpected "+describe_byte(*first)+" inside tag");
			} else if (*first=='<') {
				if (last-first>=4 && std::memcmp(first,"<!--",4)==0) {
					const char* end_mark=std::search(first+4,last,"-->","-->"+3);
					if (end_mark==last) return fail(first,"Unterminated comment");
					if (!check(first+4,end_mark)) return fail(error_at,error_message);
					token.symbol=XS_COMMENT;
					token.text=string_t(first+4,end_mark);
					const std::string content(first+4,end_mark);
					if (content.find("--")!=std::string::npos) return fail(first,"'--' is not allowed inside a comment");
					first=end_mark+3;
				} else if (last-first>=9 && std::memcmp(first,"<![CDATA[",9)==0) {
					const char* end_mark=std::search(first+9,last,"]]>","]]>"+3);
					if (end_mark==last) return fail(first,"Unterminated CDATA section");
					if (!check(first+9,end_mark)) return fail(error_at,error_message);
					token.symbol=XS_CDATA;
					token.text=string_t(first+9,end_mark);
					first=end_mark+3;
				} else if (last-first>=9 && std::memcmp(first,"<!DOCTYPE",9)==0) {
					const char* next=nullptr;
					if (!scan_doctype(first,last,next,error_message)) return fail(first,error_message);
					if (!check(first,next)) return fail(error_at,error_message);
					token.symbol=XS_DOCTYPE;
					token.text=string_t(first,next);
					first=next;
				} else if (last-first>=2 && first[1]=='?') {
					const char* target_first=first+2;
					if (!std::regex_search(target_first,last,match,name_regex(),flags)) return fail(first,"Processing instruction requires a target name");
					const std::string target(match[0].first,match[0].second);
					const char* end_mark=std::search(match[0].second,last,"?>","?>"+2);
					if (end_mark==last) return fail(first,"Unterminated processing instruction");
					const char* data_first=match[0].second;
					while (data_first<end_mark && (*data_first==' ' || *data_first=='\t' || *data_first=='\r' || *data_first=='\n')) data_first++;
					std::string lowered=target;
					for (auto& it:lowered) {
						if (it>='A' && it<='Z') it=static_cast<char>(it-'A'+'a');
					}
					if (lowered=="xml") {
						if (!tokens.empty() || token.position!=0) return fail(first,"Xml declaration is only allowed at the very beginning");
						token.symbol=XS_XMLDECL;
						if (!decode_declaration(std::string(data_first,end_mark),token,error_message)) return fail(first,error_message);
					} else {
						if (!check(data_first,end_mark)) return fail(error_at,error_message);
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
				if (!check(match[0].first,match[0].second)) return fail(error_at,error_message);
				if (!decode_text(match[0].first,match[0].second,token.text,false,error_at,error_message)) return fail(error_at,error_message);
				first=match[0].second;
			} else return fail(first,"Unexpected "+describe_byte(*first));
			tokens.push_back(std::move(token));
		}
		if (tag_mode) return fail(last,"Unterminated tag");
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
	//The LR machine mutates its own tables while parsing, so every thread owns
	//one. This removes the global lock and makes reentrant parsing possible.
	static parser_t& grammar() {
		static thread_local parser_t instance(XS_START,XS_EPSILON,XS_EOF);
		static thread_local const bool initialized=initialize_grammar(instance);
		static_cast<void>(initialized);
		return instance;
	}

	class xml_listener : public syntax::parser_listener<xml_symbol,xml_production> {
		std::vector<xml_token>* tokens_=nullptr;
		sax_t* sax_=nullptr;
		const parser_t* parser_=nullptr;
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

		//The table already knows which terminals the current state accepts, so the
		//diagnostic reports them instead of a bare "Unexpected token".
		std::string expected_symbols(int state) const {
			if (!parser_) return std::string();
			std::vector<std::string> names;
			for (const auto& it:parser_->lr_sheet) {
				if (it.first.second!=static_cast<uintptr_t>(state) || it.second.type==syntax::ST_ERROR) continue;
				const xml_symbol symbol=it.first.first;
				if (symbol==XS_EPSILON) continue;
				const auto found=parser_->ptrs.find(symbol);
				if (found!=parser_->ptrs.end() && found->second) continue;
				names.push_back(symbol_name(symbol));
			}
			if (names.empty()) return std::string();
			if (names.size()==1) return names[0];
			std::string result((names.size()>2)?"one of ":"");
			for (std::size_t i=0;i<names.size();i++) {
				if (i) result+=(i+1==names.size())?" or ":", ";
				result+=names[i];
			}
			return result;
		}

	public:
		void reset(std::vector<xml_token>& tokens,sax_t& sax,const parser_t& parser) {
			tokens_=&tokens;
			sax_=&sax;
			parser_=&parser;
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
							fail(name_token,"Duplicate attribute");
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
						fail(name_token,"Mismatched closing tag");
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
						fail(token,"Xml declaration must be the first item");
						return 0;
					}
					any_item_seen_=true;
					abort_check(sax_->declaration(token.text,token.aux,token.extra));
					break;
				}
				case XP_ITEM_DOCTYPE: {
					xml_token& token=(*tokens_)[id-2];
					if (doctype_seen_ || root_seen_) {
						fail(token,doctype_seen_?"Multiple DOCTYPE declarations":"DOCTYPE must precede the root element");
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
						fail(token,"Character data is not allowed outside the root element");
						return 0;
					}
					break;
				}
				case XP_ITEM_ELEMENT: {
					any_item_seen_=true;
					if (root_seen_) {
						fail((*tokens_)[id-2],"Multiple root elements");
						return 0;
					}
					root_seen_=true;
					break;
				}
				case XP_DOCUMENT: {
					if (!root_seen_) fail((*tokens_)[id-2],"Document has no root element");
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
			std::string message="Unexpected token";
			const std::string expected=expected_symbols(state);
			if (sax_ && tokens_ && id!=static_cast<uintptr_t>(-1) && id>=1 && id<=tokens_->size()) {
				const xml_token& token=(*tokens_)[id-1];
				const std::string text(token.text.begin(),token.text.end());
				message+=std::string(", found ")+symbol_name(token.symbol);
				if (!expected.empty()) message+=" while expecting "+expected;
				sax_->parse_error(token.position,text,message);
			} else if (sax_) {
				message="Unexpected end of input";
				if (!expected.empty()) message+=" while expecting "+expected;
				sax_->parse_error((tokens_ && !tokens_->empty())?tokens_->back().position:0,std::string(),message);
			}
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
		parser_t& parser=grammar();
		xml_listener listener;
		listener.reset(tokens,*sax,parser);
		std::vector<syntax::parser_listener<xml_symbol,xml_production>*> outer;
		outer.swap(parser.listeners);
		parser.listeners.push_back(&listener);
		bool result=false;
		try {
			result=parser.parse_with_listener(nodes);
		} catch (...) {
			parser.listeners.swap(outer);
			throw;
		}
		parser.listeners.swap(outer);
		return result && !listener.aborted() && !listener.failed();
	}
	static xml parse(std::string_view input,document_info_t* info=nullptr,bool preserve_whitespace=false,bool allow_exceptions=true) {
		xml result;
		xml_sax_dom_builder<xml> builder(result,info,preserve_whitespace);
		const bool ok=sax_parse(input,&builder) && builder.completed();
		if (!ok) {
			if (allow_exceptions) throw std::runtime_error(std::string("Parse error at ")+describe_position(input,builder.error_position())+std::string(": ")+(builder.error_message().empty()?std::string("Incomplete document"):builder.error_message()));
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
			if (c<0x20) throw std::invalid_argument("Control character U+"+hexadecimal_code(c)+" is not representable in XML 1.0");
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
		if (std::strtod(buffer,nullptr)!=static_cast<double>(value)) length=std::snprintf(buffer,sizeof(buffer),"%.16g",static_cast<double>(value));
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
			out.append({']',']','>'});
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
					dump_comment(text_payload(node)->text,out);
					break;
				}
				if (is_processing_instruction(node)) {
					const procinst_value* payload=procinst_payload(node);
					dump_processing_instruction(payload->target,payload->content,out);
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
				throw std::invalid_argument("Unsupported node type "+std::to_string(static_cast<long long>(static_cast<int>(node.type())))+" for xml serialization (use the dom conversion protocol first)");
			}
		}
	}

public:
	//Children of an xml are plain dom nodes, so serialising a subtree through the
	//member dump() would require a deep copy; this entry point does not.
	static string_t dump_node(const base_t& node,int indent=-1,typename string_t::value_type indent_char=' ',bool ensure_ascii=false) {
 		string_t result;
		dump_internal(node,result,indent,indent_char,ensure_ascii,0);
 		return result;
 	}

	virtual string_t dump(int indent=-1,typename string_t::value_type indent_char=' ',bool ensure_ascii=false) const {
		return dump_node(*this,indent,indent_char,ensure_ascii);
	}

	static void dump_comment(const string_t& text,string_t& out) {
		if (text.find(make_string("--"))!=string_t::npos || (!text.empty() && text[text.size()-1]=='-')) throw std::invalid_argument("Comment content must not contain '--' or end with '-'");
		out.append({'<','!','-','-'});
		out.append(text);
		out.append({'-','-','>'});
	}
	static void dump_processing_instruction(const string_t& target,const string_t& content,string_t& out) {
		if (content.find(make_string("?>"))!=string_t::npos) throw std::invalid_argument("Processing instruction data must not contain '?>'");
		out.append({'<','?'});
		out.append(target);
		if (!content.empty()) {
			out.push_back(' ');
			out.append(content);
		}
		out.append({'?','>'});
	}
	static void dump_outer_items(const std::vector<typename document_info_t::item>& items,bool after_doctype,string_t& out) {
		for (const auto& it:items) {
			if (it.after_doctype!=after_doctype) continue;
			if (it.type==document_info_t::XI_COMMENT) dump_comment(it.content,out);
			else dump_processing_instruction(it.target,it.content,out);
			out.push_back('\n');
		}
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
		if (info) dump_outer_items(info->prolog,false,result);
		if (info && info->has_doctype()) {
			result.append(info->doctype);
			result.push_back('\n');
		}
		if (info) dump_outer_items(info->prolog,true,result);
		dump_internal(root,result,indent,indent_char,ensure_ascii,0);
		if (info && info->has_outer_items()) {
			string_t tail;
			dump_outer_items(info->epilog,false,tail);
			dump_outer_items(info->epilog,true,tail);
			if (!tail.empty()) {
				result.push_back('\n');
				result.append(tail);
				while (!result.empty() && result[result.size()-1]=='\n') result.erase(result.size()-1);
			}
		}
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