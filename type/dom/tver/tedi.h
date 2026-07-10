//Last Modified At 2026/07/10
//@Version 1.0.0.0
#ifndef _STDEX_TYPE_DOM_EDI_H_
#define _STDEX_TYPE_DOM_EDI_H_ 1

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

#include "../../structure/dom.h"//At Least 1.0
#include "../../syntax/parser.h"//At Least 3.4
#include "../../utility/kind.h"//At Least 1.4

namespace stdex {

namespace type {

//edi专有节点类型:自dom_data_type::get_next()自动续号。注意kind为开放枚举,并行派生族
//(如binary_data_type/xml_data_type)与本族共享同一编号空间,数值可能重叠;跨族混用节点请走
//dom的转换协议,载荷访问器均经dynamic_cast守卫,误判类型会抛出而非未定义行为。
//数据模型说明:EDI线格式的语法结构=信封三层(交换→功能组→事务)+段的扁平序列+元素/子元素/重复。
//"循环"层级(如X12的HL环)由外部实现指南(schema)定义,属语义层,与DTD在xml中的地位相同,
//不进入语法层数据模型;需要时在事务的段序列上按指南自行成环。
_STDEX_DERIVED_KIND(edi_data_type,structure::dom_data_type,_STDEX_KIND_AUTO_START,
	_STDEX_KIND_VALUE_AUTO(EDT_INTERCHANGE)
	_STDEX_KIND_VALUE_AUTO(EDT_GROUP)
	_STDEX_KIND_VALUE_AUTO(EDT_TRANSACTION)
	_STDEX_KIND_VALUE_AUTO(EDT_SEGMENT)
	_STDEX_KIND_VALUE_AUTO(EDT_COMPOSITE)
)

namespace basic_edi {

//两大EDI方言:ANSI ASC X12与UN/EDIFACT。二者共享信封三层模型,差异在分隔符声明方式
//(X12:ISA定长段的定位字符;EDIFACT:可选UNA服务串)、信封段标签与转义机制(仅EDIFACT有release)。
enum edi_dialect : int {
	ED_X12,
	ED_EDIFACT,
};

//分隔符为实例自声明的数据(不是语法常量),归词法层读取、document_info保存以供往返。
//repetition/release为0表示未启用;decimal仅EDIFACT的UNA声明,不参与结构切分。
struct edi_delimiters {
	char element='*';
	char component=':';
	char segment='~';
	char repetition=0;
	char release=0;
	char decimal='.';

	static edi_delimiters x12_defaults() noexcept {
		edi_delimiters result;
		result.element='*';
		result.component=':';
		result.segment='~';
		result.repetition=0;
		result.release=0;
		result.decimal='.';
		return result;
	}
	static edi_delimiters edifact_defaults() noexcept {
		edi_delimiters result;
		result.element='+';
		result.component=':';
		result.segment='\'';
		result.repetition=0;
		result.release='?';
		result.decimal='.';
		return result;
	}
	bool operator ==(const edi_delimiters& rhs) const noexcept {
		return element==rhs.element && component==rhs.component && segment==rhs.segment && repetition==rhs.repetition && release==rhs.release && decimal==rhs.decimal;
	}
	bool operator !=(const edi_delimiters& rhs) const noexcept {
		return !(*this==rhs);
	}
};

//文档级元信息:方言、分隔符与UNA存在性不是树中数据,是"关于文档的文法/属性",存于树外。
struct edi_document_info {
	edi_dialect dialect=ED_X12;
	edi_delimiters delimiters{};
	bool has_una=false;
};

enum edi_symbol : int {
	ES_EPSILON,
	ES_EOF,
	ES_INTERCHANGE_HEADER,//ISA/UNB
	ES_INTERCHANGE_TRAILER,//IEA/UNZ
	ES_GROUP_HEADER,//GS/UNG
	ES_GROUP_TRAILER,//GE/UNE
	ES_TRANSACTION_HEADER,//ST/UNH
	ES_TRANSACTION_TRAILER,//SE/UNT
	ES_SEGMENT,
	ES_START,
	ES_DOCUMENT,
	ES_INTERCHANGE_SEQ,
	ES_INTERCHANGE,
	ES_ICONTENT,
	ES_GROUP_SEQ,
	ES_GROUP,
	ES_TXN_SEQ,
	ES_TRANSACTION,
	ES_SEG_SEQ,
};

enum edi_production : int {
	EP_START,//START→DOCUMENT EOF
	EP_DOCUMENT,//DOCUMENT→INTERCHANGE_SEQ
	EP_INTERCHANGES_FIRST,//INTERCHANGE_SEQ→INTERCHANGE
	EP_INTERCHANGES_APPEND,//INTERCHANGE_SEQ→INTERCHANGE_SEQ INTERCHANGE
	EP_INTERCHANGE_EMPTY,//INTERCHANGE→IH IT
	EP_INTERCHANGE,//INTERCHANGE→IH ICONTENT IT
	EP_ICONTENT_GROUPS,//ICONTENT→GROUP_SEQ
	EP_ICONTENT_TRANSACTIONS,//ICONTENT→TXN_SEQ
	EP_ICONTENT_SEGMENTS,//ICONTENT→SEG_SEQ(交换级裸段,如X12的TA1确认)
	EP_ICONTENT_SEGMENTS_GROUPS,//ICONTENT→SEG_SEQ GROUP_SEQ
	EP_ICONTENT_SEGMENTS_TRANSACTIONS,//ICONTENT→SEG_SEQ TXN_SEQ
	EP_GROUPS_FIRST,//GROUP_SEQ→GROUP
	EP_GROUPS_APPEND,//GROUP_SEQ→GROUP_SEQ GROUP
	EP_GROUP_EMPTY,//GROUP→GH GT
	EP_GROUP,//GROUP→GH TXN_SEQ GT
	EP_TRANSACTIONS_FIRST,//TXN_SEQ→TRANSACTION
	EP_TRANSACTIONS_APPEND,//TXN_SEQ→TXN_SEQ TRANSACTION
	EP_TRANSACTION_EMPTY,//TRANSACTION→TH TT
	EP_TRANSACTION,//TRANSACTION→TH SEG_SEQ TT
	EP_SEGMENTS_FIRST,//SEG_SEQ→SEGMENT
	EP_SEGMENTS_APPEND,//SEG_SEQ→SEG_SEQ SEGMENT
};

template <typename _Edi>
struct edi_sax {
	using base_t=typename _Edi::base_t;
	using string_t=typename _Edi::string_t;

	virtual bool dialect(edi_dialect kind,const edi_delimiters& delimiters)=0;
	virtual bool start_interchange(base_t& header)=0;
	virtual bool end_interchange(base_t& trailer)=0;
	virtual bool start_group(base_t& header)=0;
	virtual bool end_group(base_t& trailer)=0;
	virtual bool start_transaction(base_t& header)=0;
	virtual bool end_transaction(base_t& trailer)=0;
	virtual bool segment(base_t& node)=0;
	virtual bool parse_error(std::size_t position,const std::string& last_token,const std::string& message)=0;
	virtual ~edi_sax()=default;
};

//建树listener:信封节点持有header/trailer段与有序children;交换级裸段(如TA1)进交换children。
//结果为顶层交换序列(一个EDI流可含多个交换)。
template <typename _Edi>
class edi_sax_dom_builder : public edi_sax<_Edi> {
public:
	using base_t=typename _Edi::base_t;
	using string_t=typename _Edi::string_t;

private:
	std::vector<base_t>& results_;
	edi_document_info* info_=nullptr;
	std::vector<base_t*> ref_stack_;
	bool errored_=false;
	std::size_t error_position_=0;
	std::string error_message_;

	base_t* open_envelope(base_t&& node) {
		if (ref_stack_.empty()) {
			results_.push_back(std::move(node));
			return &results_.back();
		}
		typename _Edi::base_t::array_t& parent=_Edi::children(*ref_stack_.back());
		parent.push_back(std::move(node));
		return &parent.back();
	}

public:
	explicit edi_sax_dom_builder(std::vector<base_t>& results,edi_document_info* info=nullptr) : results_(results) , info_(info) { }
	bool dialect(edi_dialect kind,const edi_delimiters& delimiters) override {
		if (info_) {
			info_->dialect=kind;
			info_->delimiters=delimiters;
			info_->has_una=(kind==ED_EDIFACT && delimiters!=edi_delimiters::edifact_defaults());
		}
		return true;
	}
	bool start_interchange(base_t& header) override {
		ref_stack_.push_back(open_envelope(_Edi::make_interchange(std::move(header))));
		return true;
	}
	bool end_interchange(base_t& trailer) override {
		if (!ref_stack_.empty()) {
			_Edi::trailer(*ref_stack_.back())=std::move(trailer);
			ref_stack_.pop_back();
		}
		return true;
	}
	bool start_group(base_t& header) override {
		ref_stack_.push_back(open_envelope(_Edi::make_group(std::move(header))));
		return true;
	}
	bool end_group(base_t& trailer) override {
		if (!ref_stack_.empty()) {
			_Edi::trailer(*ref_stack_.back())=std::move(trailer);
			ref_stack_.pop_back();
		}
		return true;
	}
	bool start_transaction(base_t& header) override {
		ref_stack_.push_back(open_envelope(_Edi::make_transaction(std::move(header))));
		return true;
	}
	bool end_transaction(base_t& trailer) override {
		if (!ref_stack_.empty()) {
			_Edi::trailer(*ref_stack_.back())=std::move(trailer);
			ref_stack_.pop_back();
		}
		return true;
	}
	bool segment(base_t& node) override {
		if (ref_stack_.empty()) return true;//文法保证不出现;防御性忽略
		_Edi::children(*ref_stack_.back()).push_back(std::move(node));
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
		return !results_.empty() && ref_stack_.empty();
	}
	std::size_t error_position() const noexcept {
		return error_position_;
	}
	const std::string& error_message() const noexcept {
		return error_message_;
	}
};

template <typename _Edi>
class edi_sax_acceptor : public edi_sax<_Edi> {
public:
	using base_t=typename _Edi::base_t;
	using string_t=typename _Edi::string_t;

	bool dialect(edi_dialect,const edi_delimiters&) override { return true; }
	bool start_interchange(base_t&) override { return true; }
	bool end_interchange(base_t&) override { return true; }
	bool start_group(base_t&) override { return true; }
	bool end_group(base_t&) override { return true; }
	bool start_transaction(base_t&) override { return true; }
	bool end_transaction(base_t&) override { return true; }
	bool segment(base_t&) override { return true; }
	bool parse_error(std::size_t,const std::string&,const std::string&) override { return false; }
};

_STDEX_DOM_TPL_DECLARATION
class edi : public structure::_STDEX_DOM_DEF {
public:
	using base_t=structure::_STDEX_DOM_DEF;
	using int_t=typename base_t::int_t;
	using float_t=typename base_t::float_t;
	using boolean_t=typename base_t::boolean_t;
	using string_t=typename base_t::string_t;
	using array_t=typename base_t::array_t;
	using object_t=typename base_t::object_t;
	using size_type=typename base_t::size_type;
	using sax_t=edi_sax<edi>;
	using document_info_t=edi_document_info;

	static_assert(sizeof(typename string_t::value_type)==1,"edi serializer assumes a byte-oriented string_t.");

protected:
	//信封载荷:header/trailer为完整段节点(控制号/计数原文保真),children为有序子序列
	//(交换含组/事务/裸段,组含事务,事务含段)。子节点存为基类dom(切片为dom是既定事实),
	//edi专有子节点的类型与载荷由data_t经virtual clone/destroy保持,虚行为只在根对象生效。
	struct envelope_value : base_t::value_t {
		base_t header{};
		base_t trailer{};
		array_t children{};

		envelope_value()=default;
		explicit envelope_value(base_t header_segment) : header(std::move(header_segment)) { }
		~envelope_value() override=default;

		typename base_t::value_t* clone(structure::dom_data_type t) const override {
			if (t==EDT_INTERCHANGE || t==EDT_GROUP || t==EDT_TRANSACTION) return create_value<envelope_value>(*this);
			return base_t::value_t::clone(t);
		}
		void destroy(structure::dom_data_type t) override {
			if (t==EDT_INTERCHANGE || t==EDT_GROUP || t==EDT_TRANSACTION) return;//成员由destroy_self中的析构回收
			base_t::value_t::destroy(t);
		}
		void destroy_self(structure::dom_data_type t) override {
			this->destroy(t);
			_Allocator<envelope_value> alloc;
			std::allocator_traits<_Allocator<envelope_value>>::destroy(alloc,this);
			std::allocator_traits<_Allocator<envelope_value>>::deallocate(alloc,this,1);
		}
	};
	//段载荷:标签+位置元素序列。元素为string(简单)/EDT_COMPOSITE(子元素)/DDT_ARRAY(重复出现),
	//空位以空字符串占位(位置语义)。
	struct segment_value : base_t::value_t {
		string_t tag{};
		array_t elements{};

		segment_value()=default;
		explicit segment_value(string_t segment_tag) : tag(std::move(segment_tag)) { }
		~segment_value() override=default;

		typename base_t::value_t* clone(structure::dom_data_type t) const override {
			if (t==EDT_SEGMENT) return create_value<segment_value>(*this);
			return base_t::value_t::clone(t);
		}
		void destroy(structure::dom_data_type t) override {
			if (t==EDT_SEGMENT) return;
			base_t::value_t::destroy(t);
		}
		void destroy_self(structure::dom_data_type t) override {
			this->destroy(t);
			_Allocator<segment_value> alloc;
			std::allocator_traits<_Allocator<segment_value>>::destroy(alloc,this);
			std::allocator_traits<_Allocator<segment_value>>::deallocate(alloc,this,1);
		}
	};
	//复合元素载荷:子元素(component)序列。
	struct composite_value : base_t::value_t {
		array_t components{};

		composite_value()=default;
		~composite_value() override=default;

		typename base_t::value_t* clone(structure::dom_data_type t) const override {
			if (t==EDT_COMPOSITE) return create_value<composite_value>(*this);
			return base_t::value_t::clone(t);
		}
		void destroy(structure::dom_data_type t) override {
			if (t==EDT_COMPOSITE) return;
			base_t::value_t::destroy(t);
		}
		void destroy_self(structure::dom_data_type t) override {
			this->destroy(t);
			_Allocator<composite_value> alloc;
			std::allocator_traits<_Allocator<composite_value>>::destroy(alloc,this);
			std::allocator_traits<_Allocator<composite_value>>::deallocate(alloc,this,1);
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
	static envelope_value* envelope_payload(const base_t& node) {
		envelope_value* payload=node.data().value?dynamic_cast<envelope_value*>(node.data().value):nullptr;
		if ((node.type()!=edi_data_type(EDT_INTERCHANGE) && node.type()!=edi_data_type(EDT_GROUP) && node.type()!=edi_data_type(EDT_TRANSACTION)) || !payload) throw std::invalid_argument("Node is not an edi envelope");
		return payload;
	}
	static segment_value* segment_payload(const base_t& node) {
		segment_value* payload=node.data().value?dynamic_cast<segment_value*>(node.data().value):nullptr;
		if (node.type()!=edi_data_type(EDT_SEGMENT) || !payload) throw std::invalid_argument("Node is not an edi segment");
		return payload;
	}
	static composite_value* composite_payload(const base_t& node) {
		composite_value* payload=node.data().value?dynamic_cast<composite_value*>(node.data().value):nullptr;
		if (node.type()!=edi_data_type(EDT_COMPOSITE) || !payload) throw std::invalid_argument("Node is not an edi composite element");
		return payload;
	}
	static string_t make_string(const char* text) {
		return string_t(text,text+std::strlen(text));
	}

public:
	using base_t::base_t;
	using base_t::operator =;

	edi()=default;
	~edi() override=default;

	edi(const edi&)=default;
	edi(edi&&) noexcept=default;

	edi& operator =(const edi&)=default;
	edi& operator =(edi&&)=default;

	edi(const base_t& other) : base_t(other) { }
	edi(base_t&& other) noexcept : base_t(std::move(other)) { }

	bool support(structure::dom_data_type t) const noexcept override {
		return base_t::support(t) || t==EDT_INTERCHANGE || t==EDT_GROUP || t==EDT_TRANSACTION || t==EDT_SEGMENT || t==EDT_COMPOSITE;
	}

	//节点工厂:make_*返回基类dom节点(供建树/插入子序列),同名无make_*前缀版本返回edi根变量。
	static base_t make_interchange(base_t header=base_t()) {
		base_t node;
		node.data()=typename base_t::data_t(structure::dom_data_type(EDT_INTERCHANGE),create_value<envelope_value>(std::move(header)));
		return node;
	}
	static base_t make_group(base_t header=base_t()) {
		base_t node;
		node.data()=typename base_t::data_t(structure::dom_data_type(EDT_GROUP),create_value<envelope_value>(std::move(header)));
		return node;
	}
	static base_t make_transaction(base_t header=base_t()) {
		base_t node;
		node.data()=typename base_t::data_t(structure::dom_data_type(EDT_TRANSACTION),create_value<envelope_value>(std::move(header)));
		return node;
	}
	static base_t make_segment(string_t tag) {
		base_t node;
		node.data()=typename base_t::data_t(structure::dom_data_type(EDT_SEGMENT),create_value<segment_value>(std::move(tag)));
		return node;
	}
	static base_t make_composite() {
		base_t node;
		node.data()=typename base_t::data_t(structure::dom_data_type(EDT_COMPOSITE),create_value<composite_value>());
		return node;
	}
	static edi interchange(base_t header=base_t()) {
		return edi(make_interchange(std::move(header)));
	}
	static edi group(base_t header=base_t()) {
		return edi(make_group(std::move(header)));
	}
	static edi transaction(base_t header=base_t()) {
		return edi(make_transaction(std::move(header)));
	}
	static edi segment(string_t tag) {
		return edi(make_segment(std::move(tag)));
	}
	static edi composite() {
		return edi(make_composite());
	}

	//类型谓词(静态版接受任意树内切片节点)。
	static bool is_interchange(const base_t& node) noexcept {
		return node.type()==edi_data_type(EDT_INTERCHANGE);
	}
	static bool is_group(const base_t& node) noexcept {
		return node.type()==edi_data_type(EDT_GROUP);
	}
	static bool is_transaction(const base_t& node) noexcept {
		return node.type()==edi_data_type(EDT_TRANSACTION);
	}
	static bool is_envelope(const base_t& node) noexcept {
		return is_interchange(node) || is_group(node) || is_transaction(node);
	}
	static bool is_segment(const base_t& node) noexcept {
		return node.type()==edi_data_type(EDT_SEGMENT);
	}
	static bool is_composite(const base_t& node) noexcept {
		return node.type()==edi_data_type(EDT_COMPOSITE);
	}
	bool is_interchange() const noexcept {
		return is_interchange(*this);
	}
	bool is_group() const noexcept {
		return is_group(*this);
	}
	bool is_transaction() const noexcept {
		return is_transaction(*this);
	}
	bool is_envelope() const noexcept {
		return is_envelope(*this);
	}
	bool is_segment() const noexcept {
		return is_segment(*this);
	}
	bool is_composite() const noexcept {
		return is_composite(*this);
	}

	//载荷访问(静态版风格与xml.h一致)。
	static base_t& header(const base_t& node) {
		return envelope_payload(node)->header;
	}
	static base_t& trailer(const base_t& node) {
		return envelope_payload(node)->trailer;
	}
	static array_t& children(const base_t& node) {
		return envelope_payload(node)->children;
	}
	static string_t& tag(const base_t& node) {
		return segment_payload(node)->tag;
	}
	static array_t& elements(const base_t& node) {
		return segment_payload(node)->elements;
	}
	static array_t& components(const base_t& node) {
		return composite_payload(node)->components;
	}
	base_t& header() {
		return header(*this);
	}
	const base_t& header() const {
		return header(*this);
	}
	base_t& trailer() {
		return trailer(*this);
	}
	const base_t& trailer() const {
		return trailer(*this);
	}
	array_t& children() {
		return children(*this);
	}
	const array_t& children() const {
		return children(*this);
	}
	string_t& tag() {
		return tag(*this);
	}
	const string_t& tag() const {
		return tag(*this);
	}
	void set_tag(string_t value) {
		tag(*this)=std::move(value);
	}
	array_t& elements() {
		return elements(*this);
	}
	const array_t& elements() const {
		return elements(*this);
	}
	array_t& components() {
		return components(*this);
	}
	const array_t& components() const {
		return components(*this);
	}

	//元素便利接口(位置1起与标准文档一致:element(node,1)即REF01)。
	static size_type element_count(const base_t& node) {
		return elements(node).size();
	}
	size_type element_count() const {
		return element_count(*this);
	}
	static base_t& element(const base_t& node,size_type position) {
		array_t& sequence=elements(node);
		if (position==0 || position>sequence.size()) throw std::out_of_range("Element position "+std::to_string(static_cast<unsigned long long>(position))+" is out of range");
		return sequence[position-1];
	}
	base_t& element(size_type position) {
		return element(*this,position);
	}
	const base_t& element(size_type position) const {
		return element(*this,position);
	}
	static string_t element_or(const base_t& node,size_type position,string_t default_value) {
		if (!is_segment(node)) return default_value;
		const array_t& sequence=segment_payload(node)->elements;
		if (position==0 || position>sequence.size()) return default_value;
		const base_t& value=sequence[position-1];
		if (value.type()!=structure::DDT_STRING) return default_value;
		return *value.template get_ptr<const string_t*>();
	}
	string_t element_or(size_type position,string_t default_value) const {
		return element_or(*this,position,std::move(default_value));
	}
	static void set_element(base_t& node,size_type position,base_t value) {
		if (position==0) throw std::out_of_range("Element positions start at 1");
		array_t& sequence=elements(node);
		while (sequence.size()<position) sequence.push_back(base_t(string_t()));
		sequence[position-1]=std::move(value);
	}
	void set_element(size_type position,base_t value) {
		set_element(*this,position,std::move(value));
	}
	static base_t& append_element(base_t& node,base_t value) {
		array_t& sequence=elements(node);
		sequence.push_back(std::move(value));
		return sequence.back();
	}
	base_t& append_element(base_t value) {
		return append_element(*this,std::move(value));
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
	//首个同标签子段(不深入信封;不存在返回nullptr)。
	static base_t* find_segment(base_t& node,const string_t& segment_tag) {
		for (auto& it:children(node)) {
			if (is_segment(it) && segment_payload(it)->tag==segment_tag) return &it;
		}
		return nullptr;
	}
	static const base_t* find_segment(const base_t& node,const string_t& segment_tag) {
		return find_segment(const_cast<base_t&>(node),segment_tag);
	}
	base_t* find_segment(const string_t& segment_tag) {
		return find_segment(*this,segment_tag);
	}
	const base_t* find_segment(const string_t& segment_tag) const {
		return find_segment(*this,segment_tag);
	}

protected:
	//记法转换协议·源侧降级:edi专有节点降级为纯dom的等价结构(有损去记法,数据保全):
	//信封→{"kind","header","trailer","children"};段→{"tag","elements"};复合元素→子元素数组。
	//如需其他形态请用convert_handler_t定制。
	bool degrade_unsupported(const base_t& source,base_t& replacement) const override {
		if (source.type()==edi_data_type(EDT_COMPOSITE)) {
			replacement=base_t(structure::DDT_ARRAY);
			for (const auto& it:composite_payload(source)->components) replacement.value().array->push_back(it);
			return true;
		}
		if (source.type()==edi_data_type(EDT_SEGMENT)) {
			const segment_value* payload=segment_payload(source);
			replacement=base_t(structure::DDT_OBJECT);
			replacement.value().object->emplace(make_string("tag"),base_t(payload->tag));
			base_t element_sequence(structure::DDT_ARRAY);
			for (const auto& it:payload->elements) element_sequence.value().array->push_back(it);
			replacement.value().object->emplace(make_string("elements"),std::move(element_sequence));
			return true;
		}
		if (is_envelope(source)) {
			const envelope_value* payload=envelope_payload(source);
			replacement=base_t(structure::DDT_OBJECT);
			const char* kind_name=is_interchange(source)?"interchange":(is_group(source)?"group":"transaction");
			replacement.value().object->emplace(make_string("kind"),base_t(make_string(kind_name)));
			replacement.value().object->emplace(make_string("header"),payload->header);
			replacement.value().object->emplace(make_string("trailer"),payload->trailer);
			base_t child_sequence(structure::DDT_ARRAY);
			for (const auto& it:payload->children) child_sequence.value().array->push_back(it);
			replacement.value().object->emplace(make_string("children"),std::move(child_sequence));
			return true;
		}
		return false;
	}

private:
	struct edi_token {
		edi_symbol symbol;
		std::size_t position;
		base_t segment{};//词素=完整段节点(标签+元素树),信封段与普通段同构,分类只在symbol
	};

	//词法层:分隔符自实例声明(X12:ISA定长段定位字符;EDIFACT:UNA服务串或UNB默认),
	//故词法是"先探方言与分隔符,再定界切分"的两阶段;release转义(仅EDIFACT)在最内层展开。
	//段标签正则:2-3位字母数字且首位字母(覆盖X12的N1/NM1与EDIFACT的三字母段)。
	static const std::regex& tag_regex() {
		static const std::regex result(R"([A-Za-z][A-Za-z0-9]{1,2})",std::regex::optimize);
		return result;
	}
	static bool is_blank(char c) noexcept {
		return c==' ' || c=='\t' || c=='\r' || c=='\n';
	}
	static bool is_delimiter(char c,const edi_delimiters& delimiters) noexcept {
		return c==delimiters.element || c==delimiters.component || c==delimiters.segment || (delimiters.repetition && c==delimiters.repetition) || (delimiters.release && c==delimiters.release);
	}
	//release感知切分:被转义的分隔符不切,release对保持原样交给内层。
	static std::vector<std::string> split_with_release(std::string_view text,char separator,char release) {
		std::vector<std::string> result;
		std::string current;
		for (std::size_t i=0;i<text.size();i++) {
			const char c=text[i];
			if (release && c==release && i+1<text.size()) {
				current.push_back(c);
				current.push_back(text[++i]);
				continue;
			}
			if (c==separator) {
				result.push_back(std::move(current));
				current.clear();
				continue;
			}
			current.push_back(c);
		}
		result.push_back(std::move(current));
		return result;
	}
	//最内层展开release转义。
	static string_t unescape(std::string_view text,char release) {
		string_t result;
		for (std::size_t i=0;i<text.size();i++) {
			const char c=text[i];
			if (release && c==release && i+1<text.size()) {
				result.push_back(text[++i]);
				continue;
			}
			result.push_back(c);
		}
		return result;
	}
	//元素构建:重复出现→DDT_ARRAY;多子元素→EDT_COMPOSITE;否则string。
	static base_t build_component_group(std::string_view occurrence,const edi_delimiters& delimiters) {
		std::vector<std::string> parts=split_with_release(occurrence,delimiters.component,delimiters.release);
		if (parts.size()==1) return base_t(unescape(parts[0],delimiters.release));
		base_t node=make_composite();
		for (const auto& it:parts) components(node).push_back(base_t(unescape(it,delimiters.release)));
		return node;
	}
	static base_t build_element(std::string_view raw,const edi_delimiters& delimiters) {
		if (delimiters.repetition) {
			std::vector<std::string> occurrences=split_with_release(raw,delimiters.repetition,delimiters.release);
			if (occurrences.size()>1) {
				base_t node(structure::DDT_ARRAY);
				for (const auto& it:occurrences) node.value().array->push_back(build_component_group(it,delimiters));
				return node;
			}
		}
		return build_component_group(raw,delimiters);
	}
	static edi_symbol classify(const string_t& tag,edi_dialect dialect) {
		if (dialect==ED_X12) {
			if (tag==make_string("ISA")) return ES_INTERCHANGE_HEADER;
			if (tag==make_string("IEA")) return ES_INTERCHANGE_TRAILER;
			if (tag==make_string("GS")) return ES_GROUP_HEADER;
			if (tag==make_string("GE")) return ES_GROUP_TRAILER;
			if (tag==make_string("ST")) return ES_TRANSACTION_HEADER;
			if (tag==make_string("SE")) return ES_TRANSACTION_TRAILER;
			return ES_SEGMENT;
		}
		if (tag==make_string("UNB")) return ES_INTERCHANGE_HEADER;
		if (tag==make_string("UNZ")) return ES_INTERCHANGE_TRAILER;
		if (tag==make_string("UNG")) return ES_GROUP_HEADER;
		if (tag==make_string("UNE")) return ES_GROUP_TRAILER;
		if (tag==make_string("UNH")) return ES_TRANSACTION_HEADER;
		if (tag==make_string("UNT")) return ES_TRANSACTION_TRAILER;
		return ES_SEGMENT;
	}
	static constexpr std::size_t x12_isa_length=106;
	//段文本→段节点。X12的ISA为定长定位段:元素保持原文(含定界字符本体的ISA11/ISA16),
	//不做子元素/重复切分——这是规范规则,也避免ISA16(其值恰是component字符)被误切。
	static base_t build_segment(std::string_view text,const edi_delimiters& delimiters,edi_dialect dialect,std::size_t position,std::string& error_message) {
		std::vector<std::string> parts=split_with_release(text,delimiters.element,delimiters.release);
		const string_t segment_tag=unescape(parts[0],delimiters.release);
		{
			const std::string narrow(segment_tag.begin(),segment_tag.end());
			if (!std::regex_match(narrow,tag_regex())) {
				error_message="Invalid segment tag at byte "+std::to_string(position);
				return base_t();
			}
		}
		const bool positional=(dialect==ED_X12 && segment_tag==make_string("ISA"));
		base_t node=make_segment(segment_tag);
		array_t& sequence=elements(node);
		for (std::size_t i=1;i<parts.size();i++) {
			if (positional) sequence.push_back(base_t(string_t(parts[i].begin(),parts[i].end())));
			else sequence.push_back(build_element(parts[i],delimiters));
		}
		return node;
	}
	static bool tokenize(std::string_view input,std::vector<edi_token>& tokens,edi_dialect& dialect,edi_delimiters& delimiters,bool& has_una,std::size_t& error_position,std::string& error_message) {
		std::size_t pos=0;
		auto fail=[&](std::size_t where,const std::string& message){
			error_position=where;
			error_message=message;
			return false;
		};
		while (pos<input.size() && is_blank(input[pos])) pos++;
		if (input.size()-pos>=x12_isa_length && input.compare(pos,3,"ISA")==0) {
			dialect=ED_X12;
			has_una=false;
			delimiters=edi_delimiters();
			delimiters.element=input[pos+3];
			delimiters.repetition=input[pos+82];
			delimiters.component=input[pos+104];
			delimiters.segment=input[pos+105];
			delimiters.release=0;
			//ISA11在4010为标准标识'U':字母数字或空格视为未启用重复分隔符
			const char repetition=delimiters.repetition;
			if ((repetition>='A' && repetition<='Z') || (repetition>='a' && repetition<='z') || (repetition>='0' && repetition<='9') || repetition==' ') delimiters.repetition=0;
			//ISA按定长消费(其数据域可能含与段终结符相同的字符)
			edi_token first;
			first.symbol=ES_INTERCHANGE_HEADER;
			first.position=pos;
			first.segment=build_segment(input.substr(pos,x12_isa_length-1),delimiters,dialect,pos,error_message);
			if (!error_message.empty()) return fail(pos,error_message);
			tokens.push_back(std::move(first));
			pos+=x12_isa_length;
		} else if (input.size()-pos>=9 && input.compare(pos,3,"UNA")==0) {
			dialect=ED_EDIFACT;
			has_una=true;
			delimiters=edi_delimiters();
			delimiters.component=input[pos+3];
			delimiters.element=input[pos+4];
			delimiters.decimal=input[pos+5];
			delimiters.release=input[pos+6];
			delimiters.repetition=(input[pos+7]==' ')?0:input[pos+7];
			delimiters.segment=input[pos+8];
			if (delimiters.release==' ') delimiters.release=0;
			pos+=9;
			std::size_t probe=pos;
			while (probe<input.size() && is_blank(input[probe]) && !is_delimiter(input[probe],delimiters)) probe++;
			if (input.size()-probe<3 || input.compare(probe,3,"UNB")!=0) return fail(pos,"UNA must be followed by UNB");
		} else if (input.size()-pos>=3 && input.compare(pos,3,"UNB")==0) {
			dialect=ED_EDIFACT;
			has_una=false;
			delimiters=edi_delimiters::edifact_defaults();
		} else return fail(pos,"Unrecognized EDI dialect (expected ISA, UNA or UNB)");
		while (true) {
			while (pos<input.size() && is_blank(input[pos]) && !is_delimiter(input[pos],delimiters)) pos++;
			if (pos>=input.size()) break;
			const std::size_t start=pos;
			std::size_t end=pos;
			bool terminated=false;
			while (end<input.size()) {
				const char c=input[end];
				if (delimiters.release && c==delimiters.release && end+1<input.size()) {
					end+=2;
					continue;
				}
				if (c==delimiters.segment) {
					terminated=true;
					break;
				}
				end++;
			}
			if (!terminated) return fail(start,"Unterminated segment");
			if (end==start) return fail(start,"Empty segment");
			edi_token token;
			token.position=start;
			token.segment=build_segment(input.substr(start,end-start),delimiters,dialect,start,error_message);
			if (!error_message.empty()) return fail(start,error_message);
			token.symbol=classify(tag(token.segment),dialect);
			tokens.push_back(std::move(token));
			pos=end+1;
		}
		edi_token eof_token;
		eof_token.symbol=ES_EOF;
		eof_token.position=input.size();
		tokens.push_back(std::move(eof_token));
		return true;
	}

	using parser_t=syntax::parser<edi_symbol,edi_production>;

	static bool initialize_grammar(parser_t& target) {
		auto unit=[](edi_symbol left,std::initializer_list<edi_symbol> rights,edi_production id){
			return syntax::single_parser_unit<edi_symbol,edi_production>(left,rights,id);
		};
		target.units={
			unit(ES_START,{ES_DOCUMENT,ES_EOF},EP_START),
			unit(ES_DOCUMENT,{ES_INTERCHANGE_SEQ},EP_DOCUMENT),
			unit(ES_INTERCHANGE_SEQ,{ES_INTERCHANGE},EP_INTERCHANGES_FIRST),
			unit(ES_INTERCHANGE_SEQ,{ES_INTERCHANGE_SEQ,ES_INTERCHANGE},EP_INTERCHANGES_APPEND),
			unit(ES_INTERCHANGE,{ES_INTERCHANGE_HEADER,ES_INTERCHANGE_TRAILER},EP_INTERCHANGE_EMPTY),
			unit(ES_INTERCHANGE,{ES_INTERCHANGE_HEADER,ES_ICONTENT,ES_INTERCHANGE_TRAILER},EP_INTERCHANGE),
			unit(ES_ICONTENT,{ES_GROUP_SEQ},EP_ICONTENT_GROUPS),
			unit(ES_ICONTENT,{ES_TXN_SEQ},EP_ICONTENT_TRANSACTIONS),
			unit(ES_ICONTENT,{ES_SEG_SEQ},EP_ICONTENT_SEGMENTS),
			unit(ES_ICONTENT,{ES_SEG_SEQ,ES_GROUP_SEQ},EP_ICONTENT_SEGMENTS_GROUPS),
			unit(ES_ICONTENT,{ES_SEG_SEQ,ES_TXN_SEQ},EP_ICONTENT_SEGMENTS_TRANSACTIONS),
			unit(ES_GROUP_SEQ,{ES_GROUP},EP_GROUPS_FIRST),
			unit(ES_GROUP_SEQ,{ES_GROUP_SEQ,ES_GROUP},EP_GROUPS_APPEND),
			unit(ES_GROUP,{ES_GROUP_HEADER,ES_GROUP_TRAILER},EP_GROUP_EMPTY),
			unit(ES_GROUP,{ES_GROUP_HEADER,ES_TXN_SEQ,ES_GROUP_TRAILER},EP_GROUP),
			unit(ES_TXN_SEQ,{ES_TRANSACTION},EP_TRANSACTIONS_FIRST),
			unit(ES_TXN_SEQ,{ES_TXN_SEQ,ES_TRANSACTION},EP_TRANSACTIONS_APPEND),
			unit(ES_TRANSACTION,{ES_TRANSACTION_HEADER,ES_TRANSACTION_TRAILER},EP_TRANSACTION_EMPTY),
			unit(ES_TRANSACTION,{ES_TRANSACTION_HEADER,ES_SEG_SEQ,ES_TRANSACTION_TRAILER},EP_TRANSACTION),
			unit(ES_SEG_SEQ,{ES_SEGMENT},EP_SEGMENTS_FIRST),
			unit(ES_SEG_SEQ,{ES_SEG_SEQ,ES_SEGMENT},EP_SEGMENTS_APPEND),
		};
		target.generate_parser();
		return true;
	}
	static parser_t& grammar() {
		static parser_t instance(ES_START,ES_EPSILON,ES_EOF);
		static const bool initialized=initialize_grammar(instance);
		static_cast<void>(initialized);
		return instance;
	}
	static std::mutex& grammar_mutex() {
		static std::mutex instance;
		return instance;
	}

	//上下文相关检查全部落在listener(文法保持上下文无关):控制号匹配(尾段引用首段)与计数校验
	//(SE/UNT=事务内段数含首尾;GE/UNE=组内事务数;IEA=组数;UNZ=组数或未分组时的报文数)。
	//validate_=false时结构照常解析,仅跳过这些校验(实践中残次计数常见)。
	class edi_listener : public syntax::parser_listener<edi_symbol,edi_production> {
		struct frame {
			int level=0;//0=交换,1=组,2=事务
			std::size_t header_index=0;
			std::size_t segment_count=0;
			std::size_t group_count=0;
			std::size_t transaction_count=0;
		};

		std::vector<edi_token>* tokens_=nullptr;
		sax_t* sax_=nullptr;
		edi_dialect dialect_=ED_X12;
		bool validate_=true;
		std::vector<frame> frames_;
		bool aborted_=false;
		bool failed_=false;

		void abort_check(bool keep_going) {
			if (!keep_going) aborted_=true;
		}
		void fail(const edi_token& token,const std::string& message) {
			failed_=true;
			std::string text;
			if (is_segment(token.segment)) {
				const string_t& segment_tag=tag(token.segment);
				text.assign(segment_tag.begin(),segment_tag.end());
			}
			sax_->parse_error(token.position,text,message);
		}
		static std::string trimmed_element(const base_t& segment_node,size_type position) {
			const string_t text=element_or(segment_node,position,string_t());
			std::size_t begin=0;
			std::size_t end=text.size();
			while (begin<end && text[begin]==' ') begin++;
			while (end>begin && text[end-1]==' ') end--;
			return std::string(text.begin()+begin,text.begin()+end);
		}
		void check_control(const edi_token& header,const edi_token& trailer,size_type header_position,size_type trailer_position,const char* what) {
			const std::string expected=trimmed_element(header.segment,header_position);
			const std::string actual=trimmed_element(trailer.segment,trailer_position);
			if (expected!=actual) fail(trailer,std::string("Control number mismatch in ")+what+" (header '"+expected+"', trailer '"+actual+"')");
		}
		void check_count(const edi_token& trailer,size_type position,std::size_t actual,const char* what) {
			const std::string declared=trimmed_element(trailer.segment,position);
			char* parse_end=nullptr;
			const long long value=std::strtoll(declared.c_str(),&parse_end,10);
			if (declared.empty() || !parse_end || *parse_end!='\0' || value<0 || static_cast<unsigned long long>(value)!=actual) fail(trailer,std::string("Count mismatch in ")+what+" (declared '"+declared+"', actual "+std::to_string(actual)+")");
		}

	public:
		void reset(std::vector<edi_token>& tokens,sax_t& sax,edi_dialect dialect,bool validate) {
			tokens_=&tokens;
			sax_=&sax;
			dialect_=dialect;
			validate_=validate;
			frames_.clear();
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
		intptr_t on_shift(uintptr_t id,int state,edi_symbol word) override {
			static_cast<void>(state);
			if (aborted_ || failed_) return 0;
			edi_token& token=(*tokens_)[id-1];
			if (!frames_.empty() && frames_.back().level==2 && (word==ES_SEGMENT || word==ES_TRANSACTION_TRAILER)) frames_.back().segment_count++;
			switch (word) {
				case ES_INTERCHANGE_HEADER: {
					frame opened;
					opened.level=0;
					opened.header_index=id-1;
					frames_.push_back(opened);
					abort_check(sax_->start_interchange(token.segment));
					break;
				}
				case ES_GROUP_HEADER: {
					frame opened;
					opened.level=1;
					opened.header_index=id-1;
					frames_.push_back(opened);
					abort_check(sax_->start_group(token.segment));
					break;
				}
				case ES_TRANSACTION_HEADER: {
					frame opened;
					opened.level=2;
					opened.header_index=id-1;
					opened.segment_count=1;//UNT/SE计数含首段
					frames_.push_back(opened);
					abort_check(sax_->start_transaction(token.segment));
					break;
				}
				case ES_SEGMENT: {
					abort_check(sax_->segment(token.segment));
					break;
				}
				default: break;
			}
			return 0;
		}
		intptr_t on_reduction(uintptr_t id,int state,int next,edi_production sentence_id,int reduction_num) override {
			static_cast<void>(state);
			static_cast<void>(next);
			static_cast<void>(reduction_num);
			if (aborted_ || failed_) return 0;
			switch (sentence_id) {
				case EP_TRANSACTION_EMPTY:
				case EP_TRANSACTION: {
					edi_token& trailer_token=(*tokens_)[id-2];
					frame closed=frames_.back();
					frames_.pop_back();
					if (!frames_.empty()) frames_.back().transaction_count++;
					if (validate_) {
						const edi_token& header_token=(*tokens_)[closed.header_index];
						if (dialect_==ED_X12) {
							check_count(trailer_token,1,closed.segment_count,"SE");
							check_control(header_token,trailer_token,2,2,"SE");
						} else {
							check_count(trailer_token,1,closed.segment_count,"UNT");
							check_control(header_token,trailer_token,1,2,"UNT");
						}
					}
					if (!failed_) abort_check(sax_->end_transaction(trailer_token.segment));
					break;
				}
				case EP_GROUP_EMPTY:
				case EP_GROUP: {
					edi_token& trailer_token=(*tokens_)[id-2];
					frame closed=frames_.back();
					frames_.pop_back();
					if (!frames_.empty()) frames_.back().group_count++;
					if (validate_) {
						const edi_token& header_token=(*tokens_)[closed.header_index];
						if (dialect_==ED_X12) {
							check_count(trailer_token,1,closed.transaction_count,"GE");
							check_control(header_token,trailer_token,6,2,"GE");
						} else {
							check_count(trailer_token,1,closed.transaction_count,"UNE");
							check_control(header_token,trailer_token,5,2,"UNE");
						}
					}
					if (!failed_) abort_check(sax_->end_group(trailer_token.segment));
					break;
				}
				case EP_INTERCHANGE_EMPTY:
				case EP_INTERCHANGE: {
					edi_token& trailer_token=(*tokens_)[id-2];
					frame closed=frames_.back();
					frames_.pop_back();
					if (validate_) {
						const edi_token& header_token=(*tokens_)[closed.header_index];
						if (dialect_==ED_X12) {
							check_count(trailer_token,1,closed.group_count,"IEA");
							check_control(header_token,trailer_token,13,2,"IEA");
						} else {
							check_count(trailer_token,1,closed.group_count?closed.group_count:closed.transaction_count,"UNZ");
							check_control(header_token,trailer_token,5,2,"UNZ");
						}
					}
					if (!failed_) abort_check(sax_->end_interchange(trailer_token.segment));
					break;
				}
				case EP_START:
				case EP_DOCUMENT:
				case EP_INTERCHANGES_FIRST:
				case EP_INTERCHANGES_APPEND:
				case EP_ICONTENT_GROUPS:
				case EP_ICONTENT_TRANSACTIONS:
				case EP_ICONTENT_SEGMENTS:
				case EP_ICONTENT_SEGMENTS_GROUPS:
				case EP_ICONTENT_SEGMENTS_TRANSACTIONS:
				case EP_GROUPS_FIRST:
				case EP_GROUPS_APPEND:
				case EP_TRANSACTIONS_FIRST:
				case EP_TRANSACTIONS_APPEND:
				case EP_SEGMENTS_FIRST:
				case EP_SEGMENTS_APPEND:
				default: break;
			}
			return 0;
		}
		void on_accept() override { }
		int on_error(uintptr_t id,typename syntax::parser_listener<edi_symbol,edi_production>::error_type type,int state,edi_symbol word) override {
			static_cast<void>(type);
			static_cast<void>(state);
			static_cast<void>(word);
			failed_=true;
			if (sax_ && tokens_ && id!=static_cast<uintptr_t>(-1) && id>=1 && id<=tokens_->size()) {
				const edi_token& token=(*tokens_)[id-1];
				std::string text;
				if (is_segment(token.segment)) {
					const string_t& segment_tag=tag(token.segment);
					text.assign(segment_tag.begin(),segment_tag.end());
				}
				sax_->parse_error(token.position,text,"Unexpected segment");
			} else if (sax_) sax_->parse_error(0,std::string(),"Unexpected end of input");
			return 0;
		}
	};

public:
	static bool sax_parse(std::string_view input,sax_t* sax,bool validate_control=true) {
		std::vector<edi_token> tokens;
		edi_dialect dialect=ED_X12;
		edi_delimiters delimiters;
		bool has_una=false;
		std::size_t error_position=0;
		std::string error_message;
		if (!tokenize(input,tokens,dialect,delimiters,has_una,error_position,error_message)) {
			sax->parse_error(error_position,std::string(),error_message);
			return false;
		}
		if (tokens.size()==1) {
			sax->parse_error(0,std::string(),"Attempting to parse an empty input");
			return false;
		}
		if (!sax->dialect(dialect,delimiters)) return false;
		std::vector<typename parser_t::parse_node> nodes;
		nodes.reserve(tokens.size());
		for (const auto& it:tokens) {
			typename parser_t::parse_node node;
			node.op=it.symbol;
			nodes.push_back(std::move(node));
		}
		std::lock_guard<std::mutex> lock(grammar_mutex());
		parser_t& parser=grammar();
		static edi_listener listener;
		listener.reset(tokens,*sax,dialect,validate_control);
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
	//parse:要求恰好一个交换;parse_all:交换序列(DDT_ARRAY),EDI流可含多个交换。
	static edi parse(std::string_view input,document_info_t* info=nullptr,bool validate_control=true,bool allow_exceptions=true) {
		std::vector<base_t> results;
		edi_sax_dom_builder<edi> builder(results,info);
		const bool ok=sax_parse(input,&builder,validate_control) && builder.completed();
		if (!ok) {
			if (allow_exceptions) throw std::runtime_error(std::string("Parse error at byte ")+std::to_string(builder.error_position())+std::string(": ")+(builder.error_message().empty()?std::string("Incomplete document"):builder.error_message()));
			return edi();
		}
		if (results.size()!=1) {
			if (allow_exceptions) throw std::runtime_error("Expected exactly one interchange, found "+std::to_string(results.size())+" (use parse_all)");
			return edi();
		}
		return edi(std::move(results[0]));
	}
	static edi parse_all(std::string_view input,document_info_t* info=nullptr,bool validate_control=true,bool allow_exceptions=true) {
		std::vector<base_t> results;
		edi_sax_dom_builder<edi> builder(results,info);
		const bool ok=sax_parse(input,&builder,validate_control) && builder.completed();
		if (!ok) {
			if (allow_exceptions) throw std::runtime_error(std::string("Parse error at byte ")+std::to_string(builder.error_position())+std::string(": ")+(builder.error_message().empty()?std::string("Incomplete document"):builder.error_message()));
			return edi();
		}
		edi document(structure::DDT_ARRAY);
		for (auto& it:results) document.value().array->push_back(std::move(it));
		return document;
	}
	static bool try_parse(std::string_view input,edi& out,document_info_t* info=nullptr,bool validate_control=true) {
		std::vector<base_t> results;
		edi_sax_dom_builder<edi> builder(results,info);
		if (!sax_parse(input,&builder,validate_control) || !builder.completed() || results.size()!=1) return false;
		out=edi(std::move(results[0]));
		return true;
	}
	static bool try_parse_all(std::string_view input,edi& out,document_info_t* info=nullptr,bool validate_control=true) {
		std::vector<base_t> results;
		edi_sax_dom_builder<edi> builder(results,info);
		if (!sax_parse(input,&builder,validate_control) || !builder.completed()) return false;
		edi document(structure::DDT_ARRAY);
		for (auto& it:results) document.value().array->push_back(std::move(it));
		out=std::move(document);
		return true;
	}
	static bool accept(std::string_view input,bool validate_control=true) {
		edi_sax_acceptor<edi> acceptor;
		return sax_parse(input,&acceptor,validate_control);
	}

private:
	static void dump_integer(string_t& out,int_t value) {
		const std::string text=std::to_string(static_cast<long long>(value));
		out.append(text.begin(),text.end());
	}
	static void dump_floating(string_t& out,float_t value) {
		char buffer[64];
		int length=std::snprintf(buffer,sizeof(buffer),"%.15g",static_cast<double>(value));
		if (std::strtod(buffer,nullptr)!=static_cast<double>(value)) length=std::snprintf(buffer,sizeof(buffer),"%.17g",static_cast<double>(value));
		out.append(buffer,buffer+length);
	}
	static string_t scalar_text(const base_t& node) {
		string_t result;
		switch (static_cast<int>(node.type().get())) {
			case static_cast<int>(structure::DDT_NULL): break;
			case static_cast<int>(structure::DDT_STRING): result=*node.template get_ptr<const string_t*>();break;
			case static_cast<int>(structure::DDT_INT): dump_integer(result,*node.template get_ptr<const int_t*>());break;
			case static_cast<int>(structure::DDT_FLOAT): dump_floating(result,*node.template get_ptr<const float_t*>());break;
			case static_cast<int>(structure::DDT_BOOL): {
				if (*node.template get_ptr<const boolean_t*>()) result.append({'t','r','u','e'});
				else result.append({'f','a','l','s','e'});
				break;
			}
			default: throw std::invalid_argument("Element must be a scalar, composite or repetition");
		}
		return result;
	}
	//数据转义:EDIFACT以release前置任一分隔符;X12无转义机制,数据含分隔符即不可表示(规范如此)。
	static void escape_data(string_t& out,const string_t& text,const edi_delimiters& delimiters,edi_dialect dialect) {
		for (auto c:text) {
			if (is_delimiter(c,delimiters)) {
				if (dialect==ED_X12 || !delimiters.release) throw std::invalid_argument("Data contains a delimiter character and the dialect defines no release mechanism");
				out.push_back(delimiters.release);
			}
			out.push_back(c);
		}
	}
	static void dump_component_group(string_t& out,const base_t& node,const edi_delimiters& delimiters,edi_dialect dialect) {
		if (is_composite(node)) {
			const array_t& parts=composite_payload(node)->components;
			bool first=true;
			for (const auto& it:parts) {
				if (!first) out.push_back(delimiters.component);
				first=false;
				escape_data(out,scalar_text(it),delimiters,dialect);
			}
			return;
		}
		escape_data(out,scalar_text(node),delimiters,dialect);
	}
	static void dump_element_value(string_t& out,const base_t& node,const edi_delimiters& delimiters,edi_dialect dialect) {
		if (node.type()==structure::DDT_ARRAY) {//重复出现
			if (!delimiters.repetition) throw std::invalid_argument("Repeated element requires an enabled repetition separator");
			bool first=true;
			for (auto it=node.cbegin();it!=node.cend();it++) {
				if (!first) out.push_back(delimiters.repetition);
				first=false;
				dump_component_group(out,*it,delimiters,dialect);
			}
			return;
		}
		dump_component_group(out,node,delimiters,dialect);
	}
	static bool element_meaningful(const base_t& node) noexcept {
		if (node.type()==structure::DDT_NULL) return false;
		if (node.type()==structure::DDT_STRING) return !node.template get_ptr<const string_t*>()->empty();
		return true;
	}
	static void dump_segment_body(string_t& out,const base_t& node,const edi_delimiters& delimiters,edi_dialect dialect) {
		const segment_value* payload=segment_payload(node);
		out.append(payload->tag);
		std::size_t last=payload->elements.size();
		while (last>0 && !element_meaningful(payload->elements[last-1])) last--;//尾部空元素不发送
		for (std::size_t i=0;i<last;i++) {
			out.push_back(delimiters.element);
			dump_element_value(out,payload->elements[i],delimiters,dialect);
		}
		out.push_back(delimiters.segment);
	}
	//ISA为定长定位段:16元素逐一空格右填充到规定宽度,ISA11/ISA16承载重复与子元素分隔符本体。
	static void dump_isa(string_t& out,const base_t& node,const edi_delimiters& delimiters) {
		static const std::size_t widths[16]={2,10,2,10,2,15,2,15,6,4,1,5,9,1,1,1};
		const segment_value* payload=segment_payload(node);
		out.append(payload->tag);
		for (std::size_t i=0;i<16;i++) {
			out.push_back(delimiters.element);
			string_t field;
			if (i==10) {//ISA11:重复分隔符(未启用时保留原文,默认'U')
				if (delimiters.repetition) field.push_back(delimiters.repetition);
				else if (i<payload->elements.size() && element_meaningful(payload->elements[i])) field=scalar_text(payload->elements[i]);
				else field.push_back('U');
			} else if (i==15) field.push_back(delimiters.component);//ISA16:子元素分隔符本体
			else if (i<payload->elements.size()) field=scalar_text(payload->elements[i]);
			if (field.size()>widths[i]) throw std::invalid_argument("ISA element "+std::to_string(i+1)+" exceeds its fixed width");
			while (field.size()<widths[i]) field.push_back(' ');
			out.append(field);
		}
		out.push_back(delimiters.segment);
	}
	static string_t header_control(const base_t& header_segment,size_type position) {
		return element_or(header_segment,position,string_t());
	}
	static base_t build_trailer(const char* trailer_tag,std::size_t count,string_t control) {
		base_t node=make_segment(make_string(trailer_tag));
		string_t count_text;
		const std::string narrow=std::to_string(static_cast<unsigned long long>(count));
		count_text.append(narrow.begin(),narrow.end());
		append_element(node,base_t(std::move(count_text)));
		append_element(node,base_t(std::move(control)));
		return node;
	}
	static edi_dialect resolve_dialect(const base_t& node,const document_info_t* info) {
		if (is_envelope(node)) {
			const base_t& header_segment=envelope_payload(node)->header;
			if (is_segment(header_segment)) {
				const string_t& header_tag=segment_payload(header_segment)->tag;
				if (header_tag==make_string("ISA") || header_tag==make_string("GS") || header_tag==make_string("ST")) return ED_X12;
				if (header_tag==make_string("UNB") || header_tag==make_string("UNG") || header_tag==make_string("UNH")) return ED_EDIFACT;
			}
		}
		return info?info->dialect:ED_X12;
	}
	//信封序列化:尾段恒由树推导重算(计数是导出数据,树编辑后原计数必然失效;控制号自首段同步)。
	static void dump_envelope(string_t& out,const base_t& node,const edi_delimiters& delimiters,edi_dialect dialect,bool segment_per_line) {
		const envelope_value* payload=envelope_payload(node);
		if (!is_segment(payload->header)) throw std::invalid_argument("Envelope header segment is missing");
		const bool x12=(dialect==ED_X12);
		if (is_interchange(node) && x12) dump_isa(out,payload->header,delimiters);
		else dump_segment_body(out,payload->header,delimiters,dialect);
		if (segment_per_line) out.push_back('\n');
		std::size_t group_count=0;
		std::size_t transaction_count=0;
		std::size_t segment_count=0;
		for (const auto& it:payload->children) {
			if (is_segment(it)) {
				dump_segment_body(out,it,delimiters,dialect);
				if (segment_per_line) out.push_back('\n');
				segment_count++;
			} else if (is_envelope(it)) {
				dump_envelope(out,it,delimiters,dialect,segment_per_line);
				if (is_group(it)) group_count++;
				else transaction_count++;
			} else throw std::invalid_argument("Envelope children must be segments or envelopes");
		}
		base_t trailer_segment;
		if (is_transaction(node)) trailer_segment=build_trailer(x12?"SE":"UNT",segment_count+2,header_control(payload->header,x12?2:1));
		else if (is_group(node)) trailer_segment=build_trailer(x12?"GE":"UNE",transaction_count,header_control(payload->header,x12?6:5));
		else trailer_segment=build_trailer(x12?"IEA":"UNZ",x12?group_count:(group_count?group_count:transaction_count),header_control(payload->header,x12?13:5));
		dump_segment_body(out,trailer_segment,delimiters,dialect);
		if (segment_per_line) out.push_back('\n');
	}
	static void dump_node(string_t& out,const base_t& node,const document_info_t* info,bool segment_per_line) {
		if (node.type()==structure::DDT_ARRAY) {//交换序列
			for (auto it=node.cbegin();it!=node.cend();it++) dump_node(out,*it,info,segment_per_line);
			return;
		}
		const edi_dialect dialect=resolve_dialect(node,info);
		edi_delimiters delimiters=info?info->delimiters:(dialect==ED_X12?edi_delimiters::x12_defaults():edi_delimiters::edifact_defaults());
		if (is_envelope(node)) {
			if (is_interchange(node) && dialect==ED_EDIFACT) {
				const bool emit_una=(info && info->has_una) || delimiters!=edi_delimiters::edifact_defaults();
				if (emit_una) {
					out.append({'U','N','A'});
					out.push_back(delimiters.component);
					out.push_back(delimiters.element);
					out.push_back(delimiters.decimal);
					out.push_back(delimiters.release?delimiters.release:' ');
					out.push_back(delimiters.repetition?delimiters.repetition:' ');
					out.push_back(delimiters.segment);
					if (segment_per_line) out.push_back('\n');
				}
			}
			dump_envelope(out,node,delimiters,dialect,segment_per_line);
			return;
		}
		if (is_segment(node)) {
			dump_segment_body(out,node,delimiters,dialect);
			if (segment_per_line) out.push_back('\n');
			return;
		}
		throw std::invalid_argument("Unsupported node type "+std::to_string(static_cast<long long>(static_cast<int>(node.type())))+" (use the dom conversion protocol first)");
	}

public:
	string_t dump(const document_info_t* info=nullptr,bool segment_per_line=false) const {
		string_t result;
		dump_node(result,*this,info,segment_per_line);
		return result;
	}
	static string_t dump_document(const base_t& root,const document_info_t* info=nullptr,bool segment_per_line=false) {
		string_t result;
		dump_node(result,root,info,segment_per_line);
		return result;
	}

	friend std::ostream& operator <<(std::ostream& os,const edi& value) {
		const bool segment_per_line=os.width()>0;
		os.width(0);
		const string_t text=value.dump(nullptr,segment_per_line);
		os.write(reinterpret_cast<const char*>(text.data()),static_cast<std::streamsize>(text.size()));
		return os;
	}
	friend std::istream& operator >>(std::istream& is,edi& value) {
		std::string content;
		char buffer[4096];
		while (is.read(buffer,sizeof(buffer))) content.append(buffer,sizeof(buffer));
		content.append(buffer,static_cast<std::size_t>(is.gcount()));
		value=parse(content);
		return is;
	}
};

_STDEX_DOM_TPL_DECLARATION
inline typename edi<_Int,_Float,_Boolean,_String,_Array,_Object,_Allocator>::string_t to_string(const edi<_Int,_Float,_Boolean,_String,_Array,_Object,_Allocator>& value) {
	return value.dump();
}

}

_STDEX_DOM_TPL_DEFAULT_DECLARATION
using edi_t=basic_edi::edi<_Int,_Float,_Boolean,_String,_Array,_Object,_Allocator>;
using edi=edi_t<>;
using basic_edi::edi_dialect;
using basic_edi::ED_X12;
using basic_edi::ED_EDIFACT;
using basic_edi::edi_delimiters;
using basic_edi::edi_document_info;
using basic_edi::edi_sax;
using basic_edi::edi_sax_dom_builder;
using basic_edi::edi_sax_acceptor;
using basic_edi::to_string;

inline namespace literals {

inline edi_t<> operator ""_edi(const char* s,std::size_t n) {
	return edi_t<>::parse(std::string_view(s,n));
}

}

}

}

#endif
