//Last Modified At 2026/06/12
//@Version 1.0.0.0
#ifndef _STDEX_TYPE_DOM_XML_H_
#define _STDEX_TYPE_DOM_XML_H_ 1

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <istream>
#include <memory>
#include <mutex>
#include <ostream>
#include <regex>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "../structure/dom.h"//At Least 1.0
#include "../syntax/parser.h"//At Least 3.4
#include "../utility/kind.h"//At Least 1.4

namespace stdex {

namespace type {

//xml专有节点类型:自dom_data_type::get_next()自动续号。注意kind为开放枚举,并行派生族
//(如binary_data_type)与本族共享同一编号空间,数值可能重叠;跨族混用节点请走dom的转换协议,
//载荷访问器均经dynamic_cast守卫,误判类型会抛出而非未定义行为。
_STDEX_DERIVED_KIND(xml_data_type,structure::dom_data_type,_STDEX_KIND_AUTO_START,
	_STDEX_KIND_VALUE_AUTO(XDT_ELEMENT)
	_STDEX_KIND_VALUE_AUTO(XDT_CDATA)
	_STDEX_KIND_VALUE_AUTO(XDT_COMMENT)
	_STDEX_KIND_VALUE_AUTO(XDT_PROCINST)
)

namespace basic_xml {

enum xml_symbol : int {
	XS_EPSILON,
	XS_EOF,
	XS_LT,//"<"(后随名字,进TAG模式)
	XS_ETAG_OPEN,//"</"(进TAG模式)
	XS_GT,//">"(回CONTENT模式)
	XS_EMPTY_CLOSE,//"/>"(回CONTENT模式)
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
	XP_START,//START→DOCUMENT EOF
	XP_DOCUMENT,//DOCUMENT→ITEM_SEQ
	XP_ITEMS_FIRST,//ITEM_SEQ→DOC_ITEM
	XP_ITEMS_APPEND,//ITEM_SEQ→ITEM_SEQ DOC_ITEM
	XP_ITEM_DECL,//DOC_ITEM→XMLDECL
	XP_ITEM_DOCTYPE,//DOC_ITEM→DOCTYPE
	XP_ITEM_COMMENT,//DOC_ITEM→COMMENT
	XP_ITEM_PI,//DOC_ITEM→PI
	XP_ITEM_TEXT,//DOC_ITEM→TEXT(仅允许空白,listener检查)
	XP_ITEM_ELEMENT,//DOC_ITEM→ELEMENT(根元素,listener检查唯一性)
	XP_ELEMENT_EMPTY,//ELEMENT→EMPTY_ELEM
	XP_ELEMENT_BLANK,//ELEMENT→STAG ETAG
	XP_ELEMENT,//ELEMENT→STAG CONTENT ETAG
	XP_EMPTY_ELEM_PLAIN,//EMPTY_ELEM→LT NAME EMPTY_CLOSE
	XP_EMPTY_ELEM_ATTRS,//EMPTY_ELEM→LT NAME ATTR_SEQ EMPTY_CLOSE
	XP_STAG_PLAIN,//STAG→LT NAME GT
	XP_STAG_ATTRS,//STAG→LT NAME ATTR_SEQ GT
	XP_ETAG,//ETAG→ETAG_OPEN NAME GT
	XP_ATTR_FIRST,//ATTR_SEQ→ATTRIBUTE
	XP_ATTR_APPEND,//ATTR_SEQ→ATTR_SEQ ATTRIBUTE
	XP_ATTRIBUTE,//ATTRIBUTE→NAME EQ ATTVALUE
	XP_CONTENT_FIRST,//CONTENT→CONTENT_ITEM
	XP_CONTENT_APPEND,//CONTENT→CONTENT CONTENT_ITEM
	XP_CITEM_ELEMENT,//CONTENT_ITEM→ELEMENT
	XP_CITEM_TEXT,//CONTENT_ITEM→TEXT
	XP_CITEM_CDATA,//CONTENT_ITEM→CDATA
	XP_CITEM_COMMENT,//CONTENT_ITEM→COMMENT
	XP_CITEM_PI,//CONTENT_ITEM→PI
};

//文档级元信息:XML声明与DOCTYPE不是树中数据,是"关于文档的文法/属性",存于树外。
template <typename _String>
struct basic_xml_document_info {
	_String version{};
	_String encoding{};
	int standalone=-1;//-1未声明,0="no",1="yes"
	_String doctype{};//DOCTYPE原文(不透明载荷,往返保真)

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
			if (node.type()!=xml_data_type(XDT_ELEMENT)) return nullptr;//文档级杂项不进树
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
		if (ref_stack_.empty()) return true;//文档级空白
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
		if (ref_stack_.empty()) return true;//文档级注释不进树(SAX层可得)
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
	//元素载荷:名字+属性表+有序子序列。子节点存为基类dom(切片为dom是既定事实),
	//xml专有子节点的类型与载荷由data_t经virtual clone/destroy保持,虚行为只在根对象生效。
	struct element_value : base_t::value_t {
		string_t name{};
		object_t attributes{};
		array_t children{};

		element_value()=default;
		explicit element_value(string_t element_name) : name(std::move(element_name)) { }
		~element_value() override=default;

		typename base_t::value_t* clone(structure::dom_data_type t) const override {
			if (t==XDT_ELEMENT) return create_value<element_value>(*this);
			return base_t::value_t::clone(t);
		}
		void destroy(structure::dom_data_type t) override {
			if (t==XDT_ELEMENT) return;//成员由destroy_self中的析构回收
			base_t::value_t::destroy(t);
		}
		void destroy_self(structure::dom_data_type t) override {
			this->destroy(t);
			_Allocator<element_value> alloc;
			std::allocator_traits<_Allocator<element_value>>::destroy(alloc,this);
			std::allocator_traits<_Allocator<element_value>>::deallocate(alloc,this,1);
		}
	};
	//文本族载荷:CDATA段与注释共用(语义都是"一段不参与标记解析的原文")。
	struct text_value : base_t::value_t {
		string_t text{};

		text_value()=default;
		explicit text_value(string_t content) : text(std::move(content)) { }
		~text_value() override=default;

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
	//处理指令载荷:目标+数据。
	struct procinst_value : base_t::value_t {
		string_t target{};
		string_t content{};

		procinst_value()=default;
		procinst_value(string_t pi_target,string_t pi_content) : target(std::move(pi_target)) , content(std::move(pi_content)) { }
		~procinst_value() override=default;

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

	//节点工厂:make_*返回基类dom节点(供建树/插入子序列),同名无make_*前缀版本返回xml根变量。
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

	//类型谓词(静态版接受任意树内切片节点)。
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

	//载荷访问(静态版风格与binary_dom::get_binary一致)。
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

	//属性便利接口。
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

	//子节点便利接口。
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
	//首个同名子元素(不存在返回nullptr)。
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
	//直接文本内容:拼接全部文本与CDATA子节点。
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
		int extra=0;//XS_NAME角色:0属性名,1开标签名,2闭标签名;XS_XMLDECL:standalone(-1/0/1)
	};

	//词法层为双模式DFA:CONTENT(元素内容/文档级)与TAG(标签内部),"<"+名字与"</"进TAG,
	//">"与"/>"回CONTENT——这是XML"上下文相关进词法"的标准解法。定长定界块(注释/CDATA/PI/DOCTYPE)
	//以终止串定位扫描(等价于非贪婪正则而无回溯成本),其余token全部std::regex锚定匹配。
	static const std::regex& tag_whitespace_regex() {
		static const std::regex result(R"([ \t\r\n]+)",std::regex::optimize);
		return result;
	}
	static const std::regex& name_regex() {
		//XML NameChar的ASCII子集近似;完整Unicode区间超出char正则字符类的可移植范围,留待charset层。
		static const std::regex result(R"([:A-Za-z_][:A-Za-z0-9._\-]*)",std::regex::optimize);
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
		static const std::regex result(R"(version[ \t\r\n]*=[ \t\r\n]*(?:"(1\.[0-9]+)"|'(1\.[0-9]+)'))",std::regex::optimize);
		return result;
	}
	static const std::regex& decl_encoding_regex() {
		static const std::regex result(R"(encoding[ \t\r\n]*=[ \t\r\n]*(?:"([A-Za-z][A-Za-z0-9._\-]*)"|'([A-Za-z][A-Za-z0-9._\-]*)'))",std::regex::optimize);
		return result;
	}
	static const std::regex& decl_standalone_regex() {
		static const std::regex result(R"(standalone[ \t\r\n]*=[ \t\r\n]*(?:"(yes|no)"|'(yes|no)'))",std::regex::optimize);
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
	//XML声明的伪属性解析(version必选,encoding/standalone可选)。
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
		const char* it=first+9;//"<!DOCTYPE"
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
		if (last-first>=3 && static_cast<unsigned char>(first[0])==0xEF && static_cast<unsigned char>(first[1])==0xBB && static_cast<unsigned char>(first[2])==0xBF) first+=3;//UTF-8 BOM
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

	//上下文相关检查全部落在listener(文法保持上下文无关):①开/闭标签名匹配(名栈)
	//②属性重名 ③声明必须最先 ④DOCTYPE至多一个且先于根 ⑤根元素恰好一个 ⑥根外文本仅限空白。
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
			if (word==XS_NAME && token.extra==1) {//开标签名:start_element必须先于其属性事件
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
				case XP_ATTRIBUTE: {//NAME EQ ATTVALUE:值=id-2,名=id-4
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
				case XP_ETAG: {//ETAG_OPEN NAME GT:名=id-3
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
					if (attribute_mode) dump_char_reference(out,c);//防解析时的属性空白规范化
					else out.push_back(static_cast<typename string_t::value_type>(c));
					first++;
					continue;
				}
				case '\r': {
					dump_char_reference(out,c);//防解析时的换行规范化
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
		switch (static_cast<int>(node.type().get())) {
			case static_cast<int>(structure::DDT_NULL): break;//空内容
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
				//无记法对应的纯数组按文档片段语义顺序序列化其成员。
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
	//整文档序列化:XML声明+DOCTYPE原文+根节点。
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
		std::string content;
		char buffer[4096];
		while (is.read(buffer,sizeof(buffer))) content.append(buffer,sizeof(buffer));
		content.append(buffer,static_cast<std::size_t>(is.gcount()));
		value=parse(content);
		return is;
	}
};

_STDEX_DOM_TPL_DECLARATION
inline typename xml<_Int,_Float,_Boolean,_String,_Array,_Object,_Allocator>::string_t to_string(const xml<_Int,_Float,_Boolean,_String,_Array,_Object,_Allocator>& value) {
	return value.dump();
}

}

_STDEX_DOM_TPL_DECLARATION
using xml_t=basic_xml::xml<_Int,_Float,_Boolean,_String,_Array,_Object,_Allocator>;
using xml=xml_t<>;
using basic_xml::basic_xml_document_info;
using basic_xml::xml_sax;
using basic_xml::xml_sax_dom_builder;
using basic_xml::xml_sax_acceptor;
using basic_xml::to_string;
using xml_document_info=basic_xml_document_info<std::string>;

inline namespace literals {

inline xml_t<> operator ""_xml(const char* s,std::size_t n) {
	return xml_t<>::parse(std::string_view(s,n));
}

}

}

}

#endif
