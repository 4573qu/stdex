//Last Modified At 2026/09/05
//@Version 1.1.0.0
#ifndef _STDEX_TYPE_DOM_YAML_H_
#define _STDEX_TYPE_DOM_YAML_H_ 1

#include <cerrno>
#include <cmath>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <istream>
#include <limits>
#include <map>
#include <ostream>
#include <regex>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "../../structure/dom.h"//At Least 1.2
#include "../../syntax/parser.h"//At Least 3.4
#include "../../utility/kind.h"//At Least 1.4

namespace stdex {

namespace type {

namespace basic_yaml {

//以派生kind在树内表达yaml的图语义:YDT_ANCHOR为"命名节点"(包装目标),YDT_ALIAS为"引用边"(仅存锚点名)。
//树+引用与图在可达语义上等价,消除了反序列化把同一锚定数据展开为多份拷贝的失真。
_STDEX_DERIVED_KIND(yaml_data_type,structure::dom_data_type,_STDEX_KIND_AUTO_START,
	_STDEX_KIND_VALUE_AUTO(YDT_ANCHOR)
	_STDEX_KIND_VALUE_AUTO(YDT_ALIAS)
)

enum yaml_symbol : int {
	YS_EPSILON,
	YS_EOF,
	YS_DOC_START,
	YS_DOC_END,
	YS_DIRECTIVE,
	YS_BSEQ_START,
	YS_BSEQ_END,
	YS_BMAP_START,
	YS_BMAP_END,
	YS_ENTRY,
	YS_COLON,
	YS_COMMA,
	YS_FSEQ_START,
	YS_FSEQ_END,
	YS_FMAP_START,
	YS_FMAP_END,
	YS_SCALAR,
	YS_EMPTY,
	YS_ANCHOR,
	YS_ALIAS,
	YS_TAG,
	YS_START,
	YS_STREAM,
	YS_DOCUMENT,
	YS_DIRECTIVE_SEQ,
	YS_NODE,
	YS_BSEQ_ITEMS,
	YS_BMAP_ITEMS,
	YS_BMAP_PAIR,
	YS_FLOW_ITEMS,
	YS_FLOW_PAIRS,
	YS_FLOW_PAIR,
};

enum yaml_production : int {
	YP_START,
	YP_START_EMPTY,
	YP_STREAM_FIRST,
	YP_STREAM_APPEND,
	YP_DOCUMENT_PLAIN,
	YP_DOCUMENT_BLANK,
	YP_DOCUMENT_DIRECTIVE,
	YP_DOCUMENT_DIRECTIVE_BLANK,
	YP_DIRECTIVES_FIRST,
	YP_DIRECTIVES_APPEND,
	YP_NODE_SCALAR,
	YP_NODE_EMPTY,
	YP_NODE_ALIAS,
	YP_NODE_ANCHOR,
	YP_NODE_TAG,
	YP_NODE_BSEQ,
	YP_NODE_BMAP,
	YP_NODE_FSEQ_EMPTY,
	YP_NODE_FSEQ,
	YP_NODE_FMAP_EMPTY,
	YP_NODE_FMAP,
	YP_BSEQ_FIRST,
	YP_BSEQ_APPEND,
	YP_BMAP_FIRST,
	YP_BMAP_APPEND,
	YP_BMAP_PAIR,
	YP_FLOW_ITEMS_FIRST,
	YP_FLOW_ITEMS_APPEND,
	YP_FLOW_PAIRS_FIRST,
	YP_FLOW_PAIRS_APPEND,
	YP_FLOW_PAIR,
	YP_FLOW_PAIR_KEY,
};

enum yaml_scalar_style : int {
	YSS_PLAIN,
	YSS_SINGLE_QUOTED,
	YSS_DOUBLE_QUOTED,
	YSS_LITERAL,
	YSS_FOLDED,
};

//YAML 1.2第10章推荐的三个schema。解析时决定plain标量解析为哪个具体类型,
//序列化时决定一个字符串是否必须加引号才能被同一schema读回为字符串,
enum yaml_schema_type : int {
	YST_FAILSAFE,
	YST_JSON,
	YST_CORE,
};

//10.2要求JSON schema把无法解析的plain标量视为错误,本记法与tprotobuf对未知枚举值、
//未知字段编号的处理一致,统一退化为字符串而不是报错。
//parse行为选项:preserve_references=true时锚点/别名以YDT_ANCHOR/YDT_ALIAS节点保真入树,
//false时按展开语义将别名替换为锚定子树的拷贝;multiline_scalars控制多行plain/引用标量折叠。
struct yaml_parse_options {
	bool preserve_references=true;
	bool multiline_scalars=true;
	yaml_schema_type scalar_schema=YST_JSON;
};

template <typename _String>
struct basic_yaml_document_info {
	_String version{};
	std::vector<std::pair<_String,_String>> tag_directives{};

	bool has_version() const noexcept {
		return !version.empty();
	}
	bool has_tag_directives() const noexcept {
		return !tag_directives.empty();
	}
};

template <typename _Yaml>
struct yaml_sax {
	using int_t=typename _Yaml::int_t;
	using float_t=typename _Yaml::float_t;
	using boolean_t=typename _Yaml::boolean_t;
	using string_t=typename _Yaml::string_t;

	virtual bool start_document()=0;
	virtual bool end_document()=0;
	virtual bool directive(string_t& name,string_t& value)=0;
	virtual bool null()=0;
	virtual bool boolean(boolean_t value)=0;
	virtual bool number_integer(int_t value)=0;
	virtual bool number_float(float_t value,const string_t& raw)=0;
	virtual bool string(string_t& value,yaml_scalar_style style)=0;
	virtual bool start_mapping(std::size_t cnt)=0;
	virtual bool key(string_t& value)=0;
	virtual bool end_mapping()=0;
	virtual bool start_sequence(std::size_t cnt)=0;
	virtual bool end_sequence()=0;
	virtual bool anchor(string_t& name)=0;
	virtual bool alias(string_t& name)=0;
	virtual bool tag(string_t& text)=0;
	//Announces where the next event comes from. Not pure on purpose: a handler that
	//does not need positions is unaffected.
	virtual bool location(std::size_t position) {
		static_cast<void>(position);
		return true;
	}
	virtual bool parse_error(std::size_t position,const std::string& last_token,const std::string& message)=0;
	virtual ~yaml_sax()=default;
};

template <typename _Yaml>
class yaml_sax_dom_builder : public yaml_sax<_Yaml> {
public:
	using int_t=typename _Yaml::int_t;
	using float_t=typename _Yaml::float_t;
	using boolean_t=typename _Yaml::boolean_t;
	using string_t=typename _Yaml::string_t;
	using document_info_t=typename _Yaml::document_info_t;

private:
	std::vector<_Yaml>& documents_;
	document_info_t* info_=nullptr;
	yaml_parse_options options_{};
	using node_t=typename _Yaml::base_t;
	std::vector<node_t*> ref_stack_;
	string_t key_;
	std::map<string_t,_Yaml> anchors_;
	std::vector<std::pair<string_t,std::size_t>> pending_anchors_;
	bool in_document_=false;
	bool document_value_seen_=false;
	bool errored_=false;
	std::size_t error_position_=0;
	std::size_t current_position_=0;
	std::string error_message_;

	static bool text_equals(const string_t& text,const char* literal) noexcept {
		std::size_t i=0;
		for (;literal[i];i++) {
			if (i>=text.size() || text[i]!=static_cast<typename string_t::value_type>(literal[i])) return false;
		}
		return i==text.size();
	}

	template <typename _Vp>
	node_t* handle_value(_Vp&& value) {
		if (ref_stack_.empty()) {
			documents_.push_back(_Yaml(std::forward<_Vp>(value)));
			document_value_seen_=true;
			return &documents_.back();
		}
		node_t* parent=ref_stack_.back();
		if (parent->is_array()) {
			parent->push_back(std::forward<_Vp>(value));
			return &parent->back();
		}
		auto& slot=(*parent)[std::move(key_)];
		slot=std::forward<_Vp>(value);
		return &slot;
	}
	void bind_anchors(std::size_t depth,node_t* node) {
		while (!pending_anchors_.empty() && pending_anchors_.back().second==depth) {
			if (options_.preserve_references) {
				anchors_[pending_anchors_.back().first]=_Yaml();
				*node=_Yaml(_Yaml::make_anchor(std::move(pending_anchors_.back().first),std::move(*node)));
			} else anchors_[std::move(pending_anchors_.back().first)]=*node;
			pending_anchors_.pop_back();
		}
	}

public:
	explicit yaml_sax_dom_builder(std::vector<_Yaml>& documents,document_info_t* info=nullptr,const yaml_parse_options& options=yaml_parse_options()) : documents_(documents) , info_(info) , options_(options) { }
	bool start_document() override {
		in_document_=true;
		document_value_seen_=false;
		return true;
	}
	bool end_document() override {
		if (!document_value_seen_) documents_.push_back(_Yaml(nullptr));
		in_document_=false;
		document_value_seen_=false;
		anchors_.clear();
		pending_anchors_.clear();
		return true;
	}
	bool directive(string_t& name,string_t& value) override {
		if (!info_) return true;
		if (text_equals(name,"YAML")) info_->version=std::move(value);
		else if (text_equals(name,"TAG")) {
			std::size_t split=0;
			while (split<value.size() && value[split]!=' ' && value[split]!='\t') split++;
			string_t handle(value.begin(),value.begin()+split);
			while (split<value.size() && (value[split]==' ' || value[split]=='\t')) split++;
			string_t prefix(value.begin()+split,value.end());
			info_->tag_directives.emplace_back(std::move(handle),std::move(prefix));
		}
		return true;
	}
	bool null() override {
		node_t* node=handle_value(nullptr);
		bind_anchors(ref_stack_.size(),node);
		return true;
	}
	bool boolean(boolean_t value) override {
		node_t* node=handle_value(value);
		bind_anchors(ref_stack_.size(),node);
		return true;
	}
	bool number_integer(int_t value) override {
		node_t* node=handle_value(value);
		bind_anchors(ref_stack_.size(),node);
		return true;
	}
	bool number_float(float_t value,const string_t& raw) override {
		static_cast<void>(raw);
		node_t* node=handle_value(value);
		bind_anchors(ref_stack_.size(),node);
		return true;
	}
	bool string(string_t& value,yaml_scalar_style style) override {
		static_cast<void>(style);
		node_t* node=handle_value(std::move(value));
		bind_anchors(ref_stack_.size(),node);
		return true;
	}
	bool start_mapping(std::size_t cnt) override {
		static_cast<void>(cnt);
		ref_stack_.push_back(handle_value(node_t(structure::DDT_OBJECT)));
		return true;
	}
	bool key(string_t& value) override {
		key_=std::move(value);
		return true;
	}
	bool end_mapping() override {
		if (ref_stack_.empty()) return true;
		node_t* node=ref_stack_.back();
		ref_stack_.pop_back();
		bind_anchors(ref_stack_.size(),node);
		return true;
	}
	bool start_sequence(std::size_t cnt) override {
		static_cast<void>(cnt);
		ref_stack_.push_back(handle_value(node_t(structure::DDT_ARRAY)));
		return true;
	}
	bool end_sequence() override {
		if (ref_stack_.empty()) return true;
		node_t* node=ref_stack_.back();
		ref_stack_.pop_back();
		bind_anchors(ref_stack_.size(),node);
		return true;
	}
	bool anchor(string_t& name) override {
		pending_anchors_.emplace_back(std::move(name),ref_stack_.size());
		return true;
	}
	bool location(std::size_t position) override {
		current_position_=position;
		return true;
	}
	bool alias(string_t& name) override {
		auto it=anchors_.find(name);
		if (it==anchors_.end()) {
			errored_=true;
			error_position_=current_position_;
			error_message_="Unknown alias '"+std::string(name.begin(),name.end())+"'";
			return false;
		}
		node_t* node=nullptr;
		if (options_.preserve_references) node=handle_value(_Yaml(_Yaml::make_alias(std::move(name))));
		else node=handle_value(it->second);
		bind_anchors(ref_stack_.size(),node);
		return true;
	}
	bool tag(string_t& text) override {
		static_cast<void>(text);
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
		return !errored_ && !in_document_ && ref_stack_.empty();
	}
	std::size_t error_position() const noexcept {
		return error_position_;
	}
	const std::string& error_message() const noexcept {
		return error_message_;
	}
};

template <typename _Yaml>
class yaml_sax_acceptor : public yaml_sax<_Yaml> {
public:
	using int_t=typename _Yaml::int_t;
	using float_t=typename _Yaml::float_t;
	using boolean_t=typename _Yaml::boolean_t;
	using string_t=typename _Yaml::string_t;

	bool start_document() override { return true; }
	bool end_document() override { return true; }
	bool directive(string_t&,string_t&) override { return true; }
	bool null() override { return true; }
	bool boolean(boolean_t) override { return true; }
	bool number_integer(int_t) override { return true; }
	bool number_float(float_t,const string_t&) override { return true; }
	bool string(string_t&,yaml_scalar_style) override { return true; }
	bool start_mapping(std::size_t) override { return true; }
	bool key(string_t&) override { return true; }
	bool end_mapping() override { return true; }
	bool start_sequence(std::size_t) override { return true; }
	bool end_sequence() override { return true; }
	bool anchor(string_t&) override { return true; }
	bool alias(string_t&) override { return true; }
	bool tag(string_t&) override { return true; }
	bool parse_error(std::size_t,const std::string&,const std::string&) override { return false; }
};

_STDEX_DOM_TPL_DECLARATION
class yaml : public structure::_STDEX_DOM_DEF {
public:
	using base_t=structure::_STDEX_DOM_DEF;
	using int_t=typename base_t::int_t;
	using float_t=typename base_t::float_t;
	using boolean_t=typename base_t::boolean_t;
	using string_t=typename base_t::string_t;
	using array_t=typename base_t::array_t;
	using object_t=typename base_t::object_t;
	using size_type=typename base_t::size_type;
	using sax_t=yaml_sax<yaml>;
	using document_info_t=basic_yaml_document_info<string_t>;

	static_assert(sizeof(typename string_t::value_type)==1,"yaml serializer assumes a byte-oriented (UTF-8) string_t.");

	using base_t::base_t;
	using base_t::operator =;

	yaml()=default;
	~yaml() override=default;

	yaml(const yaml&)=default;
	yaml(yaml&&) noexcept=default;

	yaml& operator =(const yaml&)=default;
	yaml& operator =(yaml&&)=default;

	yaml(const base_t& other) : base_t(other) { }
	yaml(base_t&& other) noexcept : base_t(std::move(other)) { }

	static yaml sequence(typename base_t::initializer_list_t init_list={}) {
		return yaml(base_t::array(init_list));
	}
	static yaml mapping(typename base_t::initializer_list_t init_list={}) {
		return yaml(base_t::object(init_list));
	}
	static yaml array(typename base_t::initializer_list_t init_list={}) {
		return yaml(base_t::array(init_list));
	}
	static yaml object(typename base_t::initializer_list_t init_list={}) {
		return yaml(base_t::object(init_list));
	}

	bool support(structure::dom_data_type t) const noexcept override {
		return base_t::support(t) || t==yaml_data_type(YDT_ANCHOR) || t==yaml_data_type(YDT_ALIAS);
	}

	static bool is_anchor(const base_t& node) noexcept {
		return node.type()==yaml_data_type(YDT_ANCHOR);
	}
	bool is_anchor() const noexcept {
		return is_anchor(*this);
	}
	static bool is_alias(const base_t& node) noexcept {
		return node.type()==yaml_data_type(YDT_ALIAS);
	}
	bool is_alias() const noexcept {
		return is_alias(*this);
	}
	static base_t make_anchor(string_t name,base_t target) {
		base_t node;
		node.data()=typename base_t::data_t(structure::dom_data_type(YDT_ANCHOR),create_value<anchor_value>(std::move(name),std::move(target)));
		return node;
	}
	static base_t make_alias(string_t name) {
		base_t node;
		node.data()=typename base_t::data_t(structure::dom_data_type(YDT_ALIAS),create_value<alias_value>(std::move(name)));
		return node;
	}
	static yaml anchor(string_t name,base_t target) {
		return yaml(make_anchor(std::move(name),std::move(target)));
	}
	static yaml alias(string_t name) {
		return yaml(make_alias(std::move(name)));
	}
	static string_t& anchor_name(const base_t& node) {
		return anchor_payload(node)->name;
	}
	string_t& anchor_name() {
		return anchor_name(*this);
	}
	const string_t& anchor_name() const {
		return anchor_payload(*this)->name;
	}
	static base_t& anchor_target(const base_t& node) {
		return anchor_payload(node)->target;
	}
	base_t& anchor_target() {
		return anchor_target(*this);
	}
	const base_t& anchor_target() const {
		return anchor_payload(*this)->target;
	}
	static string_t& alias_name(const base_t& node) {
		return alias_payload(node)->name;
	}
	string_t& alias_name() {
		return alias_name(*this);
	}
	const string_t& alias_name() const {
		return alias_payload(*this)->name;
	}
	static void set_anchor(base_t& node,string_t name,base_t target) {
		node.data()=typename base_t::data_t(structure::dom_data_type(YDT_ANCHOR),create_value<anchor_value>(std::move(name),std::move(target)));
	}
	void set_anchor(string_t name,base_t target) {
		set_anchor(*this,std::move(name),std::move(target));
	}
	static void set_alias(base_t& node,string_t name) {
		node.data()=typename base_t::data_t(structure::dom_data_type(YDT_ALIAS),create_value<alias_value>(std::move(name)));
	}
	void set_alias(string_t name) {
		set_alias(*this,std::move(name));
	}
	//解引用:剥去YDT_ANCHOR包装并把YDT_ALIAS展开为锚定目标的深拷贝,得到纯树。
	//未知锚点与环状引用抛invalid_argument(parse按文档序校验不会产出环,环仅可能来自手工构树)。
	static void dereference(base_t& node) {
		std::map<string_t,base_t> anchors;
		collect_anchors(node,anchors);
		std::vector<string_t> stack;
		resolve_aliases(node,anchors,stack);
	}
	yaml& dereference() {
		dereference(static_cast<base_t&>(*this));
		return *this;
	}
	yaml dereferenced() const {
		yaml result(*this);
		result.dereference();
		return result;
	}

protected:
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

	struct anchor_value : base_t::value_t {
		string_t name{};
		base_t target{};

		anchor_value()=default;
		anchor_value(string_t anchor_name,base_t anchor_target) : name(std::move(anchor_name)) , target(std::move(anchor_target)) { }
		~anchor_value() override=default;

		anchor_value(const anchor_value& other) : base_t::value_t() , name(other.name) , target(other.target) { }

		bool equals(structure::dom_data_type t,const typename base_t::value_t& other) const noexcept override {
			if (t!=yaml_data_type(YDT_ANCHOR)) return base_t::value_t::equals(t,other);
			const anchor_value* right=dynamic_cast<const anchor_value*>(&other);
			if (!right) return false;
			return name==right->name && target==right->target;
		}
		bool less(structure::dom_data_type t,const typename base_t::value_t& other) const noexcept override {
			if (t!=yaml_data_type(YDT_ANCHOR)) return base_t::value_t::less(t,other);
			const anchor_value* right=dynamic_cast<const anchor_value*>(&other);
			if (!right) return false;
			if (name!=right->name) return name<right->name;
			return target<right->target;
		}

		typename base_t::value_t* clone(structure::dom_data_type t) const override {
			if (t==yaml_data_type(YDT_ANCHOR)) return create_value<anchor_value>(*this);
			return base_t::value_t::clone(t);
		}
		void destroy(structure::dom_data_type t) override {
			if (t==yaml_data_type(YDT_ANCHOR)) return;
			base_t::value_t::destroy(t);
		}
		void destroy_self(structure::dom_data_type t) override {
			this->destroy(t);
			_Allocator<anchor_value> alloc;
			std::allocator_traits<_Allocator<anchor_value>>::destroy(alloc,this);
			std::allocator_traits<_Allocator<anchor_value>>::deallocate(alloc,this,1);
		}
	};
	struct alias_value : base_t::value_t {
		string_t name{};

		alias_value()=default;
		explicit alias_value(string_t alias_name) : name(std::move(alias_name)) { }
		~alias_value() override=default;

		alias_value(const alias_value& other) : base_t::value_t() , name(other.name) { }

		bool equals(structure::dom_data_type t,const typename base_t::value_t& other) const noexcept override {
			if (t!=yaml_data_type(YDT_ALIAS)) return base_t::value_t::equals(t,other);
			const alias_value* right=dynamic_cast<const alias_value*>(&other);
			if (!right) return false;
			return name==right->name;
		}
		bool less(structure::dom_data_type t,const typename base_t::value_t& other) const noexcept override {
			if (t!=yaml_data_type(YDT_ALIAS)) return base_t::value_t::less(t,other);
			const alias_value* right=dynamic_cast<const alias_value*>(&other);
			if (!right) return false;
			return name<right->name;
		}

		typename base_t::value_t* clone(structure::dom_data_type t) const override {
			if (t==yaml_data_type(YDT_ALIAS)) return create_value<alias_value>(*this);
			return base_t::value_t::clone(t);
		}
		void destroy(structure::dom_data_type t) override {
			if (t==yaml_data_type(YDT_ALIAS)) return;
			base_t::value_t::destroy(t);
		}
		void destroy_self(structure::dom_data_type t) override {
			this->destroy(t);
			_Allocator<alias_value> alloc;
			std::allocator_traits<_Allocator<alias_value>>::destroy(alloc,this);
			std::allocator_traits<_Allocator<alias_value>>::deallocate(alloc,this,1);
		}
	};

	static anchor_value* anchor_payload(const base_t& node) {
		anchor_value* payload=node.data().value?dynamic_cast<anchor_value*>(node.data().value):nullptr;
		if (node.type()!=yaml_data_type(YDT_ANCHOR) || !payload) throw std::invalid_argument("Node does not hold a yaml anchor payload");
		return payload;
	}
	static alias_value* alias_payload(const base_t& node) {
		alias_value* payload=node.data().value?dynamic_cast<alias_value*>(node.data().value):nullptr;
		if (node.type()!=yaml_data_type(YDT_ALIAS) || !payload) throw std::invalid_argument("Node does not hold a yaml alias payload");
		return payload;
	}

	static void collect_anchors(const base_t& node,std::map<string_t,base_t>& anchors) {
		if (is_anchor(node)) {
			const anchor_value* payload=anchor_payload(node);
			anchors.emplace(payload->name,payload->target);
			collect_anchors(payload->target,anchors);
			return;
		}
		if (node.type()==structure::DDT_ARRAY || node.type()==structure::DDT_OBJECT) {
			for (auto it=node.cbegin();it!=node.cend();it++) collect_anchors(*it,anchors);
		}
	}
	static void resolve_aliases(base_t& node,std::map<string_t,base_t>& anchors,std::vector<string_t>& stack) {
		if (is_anchor(node)) {
			base_t target=std::move(anchor_target(node));
			resolve_aliases(target,anchors,stack);
			node=std::move(target);
			return;
		}
		if (is_alias(node)) {
			const string_t name=alias_name(node);
			for (const auto& it:stack) {
				if (it==name) throw std::invalid_argument("Cyclic alias '"+std::string(name.begin(),name.end())+"'");
			}
			auto found=anchors.find(name);
			if (found==anchors.end()) throw std::invalid_argument("Unknown alias '"+std::string(name.begin(),name.end())+"'");
			stack.push_back(name);
			base_t copy=found->second;
			resolve_aliases(copy,anchors,stack);
			stack.pop_back();
			node=std::move(copy);
			return;
		}
		if (node.type()==structure::DDT_ARRAY) {
			for (auto& it:*node.value().array) resolve_aliases(it,anchors,stack);
			return;
		}
		if (node.type()==structure::DDT_OBJECT) {
			for (auto& it:*node.value().object) resolve_aliases(it.second,anchors,stack);
		}
	}
	static const base_t* find_anchor(const base_t& node,const string_t& name) {
		if (is_anchor(node)) {
			const anchor_value* payload=anchor_payload(node);
			if (payload->name==name) return &payload->target;
			return find_anchor(payload->target,name);
		}
		if (node.type()==structure::DDT_ARRAY || node.type()==structure::DDT_OBJECT) {
			for (auto it=node.cbegin();it!=node.cend();it++) {
				const base_t* result=find_anchor(*it,name);
				if (result) return result;
			}
		}
		return nullptr;
	}

	//记法转换协议·源侧降级:YDT_ANCHOR降级为其目标节点;YDT_ALIAS按锚点名在源根内解析为目标副本,
	//查无锚点返回false交由转换策略处置。环状引用请先dereference(其带环检测)。
	bool degrade_unsupported(const base_t& source,base_t& replacement) const override {
		if (source.type()==yaml_data_type(YDT_ANCHOR)) {
			replacement=anchor_target(source);
			return true;
		}
		if (source.type()==yaml_data_type(YDT_ALIAS)) {
			const base_t* target=find_anchor(*this,alias_name(source));
			if (!target) return false;
			replacement=*target;
			return true;
		}
		return false;
	}

private:
	struct yaml_token {
		yaml_symbol symbol;
		std::size_t position;
		string_t text{};
		string_t aux{};
		int extra=0;
	};

	static std::string hexadecimal_code(unsigned long cp) {
		static const char digits[]="0123456789ABCDEF";
		std::string result;
		for (int shift=12;shift>=0;shift-=4) result.push_back(digits[(cp>>shift)&0xF]);
		if (cp>0xFFFF) {
			result.clear();
			for (int shift=28;shift>=0;shift-=4) result.push_back(digits[(cp>>shift)&0xF]);
			while (result.size()>4 && result[0]=='0') result.erase(result.begin());
		}
		return result;
	}
	//Diagnostics helper. A printable byte keeps the "character 'x'" wording, anything
	//else is named by its value instead of being embedded raw in the message.
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
	static bool is_printable_code(unsigned long cp) noexcept {
		if (cp==0x9 || cp==0xA || cp==0xD || cp==0x85) return true;
		if (cp>=0x20 && cp<=0x7E) return true;
		if (cp>=0xA0 && cp<=0xD7FF) return true;
		if (cp>=0xE000 && cp<=0xFFFD) return true;
		return cp>=0x10000 && cp<=0x10FFFF;
	}
	//YAML 1.2 c-printable plus UTF-8 well formedness, applied to the whole stream so
	//that whatever the parser accepts can also be serialised again.
	static bool validate_characters(std::string_view input,std::size_t& error_position,std::string& error_message) {
		const char* first=input.data();
		const char* const last=first+input.size();
		while (first<last) {
			const unsigned char lead=static_cast<unsigned char>(*first);
			unsigned long raw=0;
			std::size_t extra=0;
			if (lead<0x80) raw=lead;
			else if ((lead&0xE0)==0xC0) {
				raw=lead&0x1F;
				extra=1;
			} else if ((lead&0xF0)==0xE0) {
				raw=lead&0x0F;
				extra=2;
			} else if ((lead&0xF8)==0xF0) {
				raw=lead&0x07;
				extra=3;
			} else {
				error_position=static_cast<std::size_t>(first-input.data());
				error_message="Invalid UTF-8 lead byte";
				return false;
			}
			if (static_cast<std::size_t>(last-first)<extra+1) {
				error_position=static_cast<std::size_t>(first-input.data());
				error_message="Truncated UTF-8 sequence";
				return false;
			}
			for (std::size_t i=1;i<=extra;i++) {
				if ((static_cast<unsigned char>(first[i])&0xC0)!=0x80) {
					error_position=static_cast<std::size_t>(first+i-input.data());
					error_message="Invalid UTF-8 continuation byte";
					return false;
				}
				raw=(raw<<6)|(static_cast<unsigned char>(first[i])&0x3F);
			}
			if ((extra==1 && raw<0x80) || (extra==2 && raw<0x800) || (extra==3 && raw<0x10000)) {
				error_position=static_cast<std::size_t>(first-input.data());
				error_message="Overlong UTF-8 sequence";
				return false;
			}
			if (!is_printable_code(raw)) {
				error_position=static_cast<std::size_t>(first-input.data());
				error_message="Character U+"+hexadecimal_code(raw)+" is not allowed in YAML 1.2";
				return false;
			}
			first+=extra+1;
		}
		return true;
	}
	static const char* symbol_name(yaml_symbol symbol) noexcept {
		switch (symbol) {
			case YS_EOF: return "end of input";
			case YS_DOC_START: return "'---'";
			case YS_DOC_END: return "'...'";
			case YS_DIRECTIVE: return "a directive";
			case YS_BSEQ_START: return "the start of a block sequence";
			case YS_BSEQ_END: return "the end of a block sequence";
			case YS_BMAP_START: return "the start of a block mapping";
			case YS_BMAP_END: return "the end of a block mapping";
			case YS_ENTRY: return "'-'";
			case YS_COLON: return "':'";
			case YS_COMMA: return "','";
			case YS_FSEQ_START: return "'['";
			case YS_FSEQ_END: return "']'";
			case YS_FMAP_START: return "'{'";
			case YS_FMAP_END: return "'}'";
			case YS_SCALAR: return "a scalar";
			case YS_EMPTY: return "an empty node";
			case YS_ANCHOR: return "an anchor";
			case YS_ALIAS: return "an alias";
			case YS_TAG: return "a tag";
			default: return "a node";
		}
	}

	//10.2与10.3的判定式。10.1不调用它们:失效安全schema下没有任何标量会被解析。
	static const std::regex& null_regex(yaml_schema_type schema) {
		static const std::regex json_result(R"(null)",std::regex::optimize);
		static const std::regex core_result(R"(~|null|Null|NULL)",std::regex::optimize);
		return schema==YST_CORE?core_result:json_result;
	}
	static const std::regex& true_regex(yaml_schema_type schema) {
		static const std::regex json_result(R"(true)",std::regex::optimize);
		static const std::regex core_result(R"(true|True|TRUE)",std::regex::optimize);
		return schema==YST_CORE?core_result:json_result;
	}
	static const std::regex& false_regex(yaml_schema_type schema) {
		static const std::regex json_result(R"(false)",std::regex::optimize);
		static const std::regex core_result(R"(false|False|FALSE)",std::regex::optimize);
		return schema==YST_CORE?core_result:json_result;
	}
	static const std::regex& integer_regex(yaml_schema_type schema) {
		//10.2.1.3: -?(0|[1-9][0-9]*)
		static const std::regex json_result(R"(-?(?:0|[1-9][0-9]*))",std::regex::optimize);
		//10.3.2: [-+]?[0-9]+ 与 0o[0-7]+ 与 0x[0-9a-fA-F]+
		static const std::regex core_result(R"([-+]?[0-9]+|0x[0-9A-Fa-f]+|0o[0-7]+)",std::regex::optimize);
		return schema==YST_CORE?core_result:json_result;
	}
	static const std::regex& floating_regex(yaml_schema_type schema) {
		//10.2.1.4: -?(0|[1-9][0-9]*)(\.[0-9]*)?([eE][-+]?[0-9]+)?
		static const std::regex json_result(R"(-?(?:0|[1-9][0-9]*)(?:\.[0-9]*)?(?:[eE][-+]?[0-9]+)?)",std::regex::optimize);
		//10.3.2: 追加前导零、正号、.5形式与.inf/.nan
		static const std::regex core_result(R"([-+]?(?:\.[0-9]+|[0-9]+(?:\.[0-9]*)?)(?:[eE][-+]?[0-9]+)?|[-+]?\.(?:inf|Inf|INF)|\.(?:nan|NaN|NAN))",std::regex::optimize);
		return schema==YST_CORE?core_result:json_result;
	}

	static bool regex_match_text(const string_t& text,const std::regex& expression) {
		if (text.empty()) return false;
		const char* first=reinterpret_cast<const char*>(text.data());
		return std::regex_match(first,first+text.size(),expression);
	}
	//空标量只在10.3下解析为null,10.1与10.2下是空字符串。
	static bool plain_is_null(const string_t& text,yaml_schema_type schema) {
		if (schema==YST_FAILSAFE) return false;
		if (text.empty()) return schema==YST_CORE;
		return regex_match_text(text,null_regex(schema));
	}
	static int plain_boolean(const string_t& text,yaml_schema_type schema) {
		if (schema==YST_FAILSAFE) return -1;
		if (regex_match_text(text,true_regex(schema))) return 1;
		if (regex_match_text(text,false_regex(schema))) return 0;
		return -1;
	}
	static bool plain_integer(const string_t& text,int_t& out,yaml_schema_type schema) {
		if (schema==YST_FAILSAFE) return false;
		if (!regex_match_text(text,integer_regex(schema))) return false;
		const std::string buffer(text.begin(),text.end());
		errno=0;
		if (buffer.size()>2 && buffer[0]=='0' && (buffer[1]=='x' || buffer[1]=='o')) {
			const unsigned long long value=std::strtoull(buffer.c_str()+2,nullptr,buffer[1]=='x'?16:8);
			if (errno==ERANGE || value>static_cast<unsigned long long>((std::numeric_limits<int_t>::max)())) return false;
			out=static_cast<int_t>(value);
			return true;
		}
		const long long value=std::strtoll(buffer.c_str(),nullptr,10);
		if (errno==ERANGE || value<static_cast<long long>((std::numeric_limits<int_t>::min)()) || value>static_cast<long long>((std::numeric_limits<int_t>::max)())) return false;
		out=static_cast<int_t>(value);
		return true;
	}
	static bool plain_floating(const string_t& text,float_t& out,yaml_schema_type schema) {
		if (schema==YST_FAILSAFE) return false;
		if (!regex_match_text(text,floating_regex(schema))) return false;
		std::string buffer(text.begin(),text.end());
		std::string body=buffer;
		float_t sign=1;
		if (!body.empty() && (body[0]=='+' || body[0]=='-')) {
			if (body[0]=='-') sign=-1;
			body.erase(0,1);
		}
		if (body==".inf" || body==".Inf" || body==".INF") {
			out=sign*std::numeric_limits<float_t>::infinity();
			return true;
		}
		if (body==".nan" || body==".NaN" || body==".NAN") {
			out=std::numeric_limits<float_t>::quiet_NaN();
			return true;
		}
		out=static_cast<float_t>(std::strtod(buffer.c_str(),nullptr));
		return true;
	}

	static int tag_class(const string_t& tag) {
		const std::string text(tag.begin(),tag.end());
		if (text=="!") return 1;
		std::string suffix;
		if (text.size()>2 && text[0]=='!' && text[1]=='!') suffix=text.substr(2);
		else if (text.rfind("!<tag:yaml.org,2002:",0)==0 && text.size()>21 && text.back()=='>') suffix=text.substr(20,text.size()-21);
		else return 0;
		if (suffix=="str") return 1;
		if (suffix=="int") return 2;
		if (suffix=="float") return 3;
		if (suffix=="bool") return 4;
		if (suffix=="null") return 5;
		return 0;
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

	struct block_frame {
		std::size_t indent;
		bool sequence;
	};

	struct tokenizer {
		const char* base;
		const char* first;
		const char* last;
		const char* line_start=nullptr;
		const char* line_end=nullptr;
		const char* next_position=nullptr;
		std::vector<yaml_token>& tokens;
		std::vector<block_frame> blocks{};
		bool document_open=false;
		bool directives_pending=false;
		bool expect_node=false;
		std::size_t pending_indent=0;
		bool pending_in_map=false;
		bool colon_on_line=false;
		bool multiline=false;
		bool plain_continuing=false;
		std::ptrdiff_t plain_owner=-1;
		std::size_t pending_breaks=0;
		bool failed=false;
		std::size_t error_position=0;
		std::string error_message{};

		tokenizer(std::string_view input,std::vector<yaml_token>& tokens) : base(input.data()) , first(input.data()) , last(input.data()+input.size()) , tokens(tokens) { }

		static bool is_space(char c) noexcept {
			return c==' ' || c=='\t';
		}
		static bool is_break(char c) noexcept {
			return c=='\n' || c=='\r';
		}
		static bool is_flow_indicator(char c) noexcept {
			return c==',' || c=='[' || c==']' || c=='{' || c=='}';
		}
		std::size_t position(const char* p) const noexcept {
			return static_cast<std::size_t>(p-base);
		}
		bool fail(const char* p,std::string message) {
			failed=true;
			error_position=position(p);
			error_message=std::move(message);
			return false;
		}
		void emit(yaml_symbol symbol,const char* p) {
			yaml_token token;
			token.symbol=symbol;
			token.position=position(p);
			tokens.push_back(std::move(token));
		}
		void emit_scalar(const char* p,string_t text,int style) {
			yaml_token token;
			token.symbol=YS_SCALAR;
			token.position=position(p);
			token.text=std::move(text);
			token.extra=style;
			tokens.push_back(std::move(token));
			expect_node=false;
		}
		const char* after_line(const char* line_end) const noexcept {
			if (line_end>=last) return last;
			if (*line_end=='\r' && line_end+1<last && line_end[1]=='\n') return line_end+2;
			return line_end+1;
		}

		bool run() {
			if (last-first>=3 && static_cast<unsigned char>(first[0])==0xEF && static_cast<unsigned char>(first[1])==0xBB && static_cast<unsigned char>(first[2])==0xBF) first+=3;
			while (first<last) {
				if (!next_line()) return false;
			}
			if (expect_node) {
				emit(YS_EMPTY,last);
				expect_node=false;
			}
			while (!blocks.empty()) {
				emit(blocks.back().sequence?YS_BSEQ_END:YS_BMAP_END,last);
				blocks.pop_back();
			}
			if (directives_pending) return fail(last,"Directives must be followed by '---'");
			if (document_open) {
				emit(YS_DOC_END,last);
				document_open=false;
			}
			return true;
		}

		void close_document(const char* p) {
			if (expect_node) {
				emit(YS_EMPTY,p);
				expect_node=false;
			}
			while (!blocks.empty()) {
				emit(blocks.back().sequence?YS_BSEQ_END:YS_BMAP_END,p);
				blocks.pop_back();
			}
			if (document_open) {
				emit(YS_DOC_END,p);
				document_open=false;
			}
			pending_indent=0;
			pending_in_map=false;
			plain_continuing=false;
			pending_breaks=0;
		}

		bool next_line() {
			line_start=first;
			line_end=first;
			while (line_end<last && !is_break(*line_end)) line_end++;
			next_position=after_line(line_end);
			const char* p=line_start;
			std::size_t indent=0;
			while (p<line_end && *p==' ') {
				p++;
				indent++;
			}
			const char* probe=p;
			while (probe<line_end && is_space(*probe)) probe++;
			if (probe==line_end || *probe=='#') {
				if (plain_continuing) {
					if (probe==line_end) pending_breaks++;
					else {
						plain_continuing=false;
						pending_breaks=0;
					}
				}
				first=next_position;
				return true;
			}
			if (*p=='\t') return fail(p,"Tab characters are not allowed in indentation");
			if (*p=='%') {
				if (indent!=0) return fail(p,"Directives must start at the beginning of a line");
				if (document_open) return fail(p,"Directives are only allowed before '---'");
				if (!scan_directive(p)) return false;
				first=next_position;
				return true;
			}
			if (indent==0 && line_end-p>=3 && p[0]=='-' && p[1]=='-' && p[2]=='-' && (p+3==line_end || is_space(p[3]))) {
				close_document(p);
				emit(YS_DOC_START,p);
				document_open=true;
				directives_pending=false;
				colon_on_line=false;
				const char* q=p+3;
				while (q<line_end && is_space(*q)) q++;
				if (q<line_end && *q!='#') {
					if (!scan_line(q)) return false;
				}
				first=next_position;
				return true;
			}
			if (indent==0 && line_end-p>=3 && p[0]=='.' && p[1]=='.' && p[2]=='.' && (p+3==line_end || is_space(p[3]))) {
				if (!document_open) return fail(p,"Unexpected '...' outside a document");
				close_document(p);
				const char* q=p+3;
				while (q<line_end && is_space(*q)) q++;
				if (q<line_end && *q!='#') return fail(q,"Unexpected content after '...'");
				first=next_position;
				return true;
			}
			if (!document_open) {
				if (directives_pending) return fail(p,"Directives must be followed by '---'");
				emit(YS_DOC_START,p);
				document_open=true;
			}
			if (plain_continuing) {
				if (multiline && static_cast<std::ptrdiff_t>(indent)>plain_owner && try_fold_plain(p)) {
					first=next_position;
					return true;
				}
				plain_continuing=false;
				pending_breaks=0;
			}
			colon_on_line=false;
			const bool line_is_entry=(*p=='-' && (p+1==line_end || is_space(p[1])));
			if (expect_node) {
				if (indent>pending_indent) {
				} else if (indent==pending_indent && line_is_entry && pending_in_map) {
				} else {
					emit(YS_EMPTY,p);
					expect_node=false;
				}
			}
			if (!expect_node) {
				while (!blocks.empty() && (blocks.back().indent>indent || (blocks.back().indent==indent && blocks.back().sequence && !line_is_entry))) {
					emit(blocks.back().sequence?YS_BSEQ_END:YS_BMAP_END,p);
					blocks.pop_back();
				}
			}
			if (!scan_line(p)) return false;
			first=next_position;
			return true;
		}

		bool scan_directive(const char* p) {
			const char* q=p+1;
			const char* name_first=q;
			while (q<line_end && !is_space(*q)) q++;
			if (q==name_first) return fail(p,"Empty directive name");
			yaml_token token;
			token.symbol=YS_DIRECTIVE;
			token.position=position(p);
			token.text=string_t(name_first,q);
			while (q<line_end && is_space(*q)) q++;
			const char* value_first=q;
			const char* value_last=line_end;
			for (const char* r=value_first;r<line_end;r++) {
				if (*r=='#' && (r==value_first || is_space(r[-1]))) {
					value_last=r;
					break;
				}
			}
			while (value_last>value_first && is_space(value_last[-1])) value_last--;
			token.aux=string_t(value_first,value_last);
			tokens.push_back(std::move(token));
			directives_pending=true;
			return true;
		}

		bool begin_mapping_pair(const char* key_position,std::size_t key_column,string_t key_text,int key_style,const char* colon_position) {
			if (colon_on_line) return fail(colon_position,"Mapping values are not allowed in this context");
			if (blocks.empty() || blocks.back().sequence || blocks.back().indent!=key_column) {
				blocks.push_back(block_frame{key_column,false});
				emit(YS_BMAP_START,key_position);
			}
			yaml_token token;
			token.symbol=YS_SCALAR;
			token.position=position(key_position);
			token.text=std::move(key_text);
			token.extra=key_style;
			tokens.push_back(std::move(token));
			emit(YS_COLON,colon_position);
			colon_on_line=true;
			expect_node=true;
			pending_indent=key_column;
			pending_in_map=true;
			return true;
		}

		//多行plain续行:更深缩进、无键冒号、不以指示符开头的裸文本行折叠进上一plain标量;
		//折叠规则:单断行=空格,k个空行=k个换行;行内注释终结续行。
		bool try_fold_plain(const char* p) {
			if (tokens.empty() || tokens.back().symbol!=YS_SCALAR || tokens.back().extra!=YSS_PLAIN) return false;
			static const char indicators[]="-?:,[]{}#&*!|>'\"%@`";
			for (const char* it=indicators;*it;it++) {
				if (*p==*it) return false;
			}
			const char* span_last=line_end;
			for (const char* r=p;r<line_end;r++) {
				if (*r=='#' && r>p && is_space(r[-1])) {
					span_last=r;
					break;
				}
			}
			for (const char* r=p;r<span_last;r++) {
				if (*r==':' && (r+1>=span_last || is_space(r[1]))) return false;
			}
			const char* text_last=span_last;
			while (text_last>p && is_space(text_last[-1])) text_last--;
			if (text_last==p) return false;
			string_t& text=tokens.back().text;
			if (pending_breaks==0) text.push_back(' ');
			else {
				for (std::size_t i=0;i<pending_breaks;i++) text.push_back('\n');
			}
			pending_breaks=0;
			text.append(p,text_last);
			if (span_last<line_end) plain_continuing=false;
			return true;
		}
		void relocate_line(const char* p) {
			line_start=p;
			while (line_start>base && !is_break(line_start[-1])) line_start--;
			line_end=p;
			while (line_end<last && !is_break(*line_end)) line_end++;
			next_position=after_line(line_end);
		}

		bool read_hex_digits(const char* r,int count,unsigned long& cp) const {
			if (r+count>line_end) return false;
			cp=0;
			for (int i=0;i<count;i++) {
				const char c=r[i];
				cp<<=4;
				if (c>='0' && c<='9') cp|=static_cast<unsigned long>(c-'0');
				else if (c>='a' && c<='f') cp|=static_cast<unsigned long>(c-'a'+10);
				else if (c>='A' && c<='F') cp|=static_cast<unsigned long>(c-'A'+10);
				else return false;
			}
			return true;
		}

		bool scan_quoted(const char*& p,string_t& out) {
			const char quote=*p;
			const char* bound=multiline?last:line_end;
			const char* q=p+1;
			if (quote=='\'') {
				while (q<bound) {
					if (*q=='\'') {
						if (q+1<bound && q[1]=='\'') {
							out.push_back('\'');
							q+=2;
							continue;
						}
						p=q+1;
						return true;
					}
					if (is_break(*q)) {
						fold_quoted_break(q,bound,out);
						continue;
					}
					out.push_back(*q++);
				}
				return fail(p,multiline?"Unterminated single-quoted scalar":"Unterminated single-quoted scalar (multi-line quoted scalars require multiline_scalars)");
			}
			while (q<bound) {
				const char c=*q;
				if (c=='"') {
					p=q+1;
					return true;
				}
				if (is_break(c)) {
					fold_quoted_break(q,bound,out);
					continue;
				}
				if (c!='\\') {
					out.push_back(c);
					q++;
					continue;
				}
				q++;
				if (q==bound) return fail(q,"Unterminated escape sequence");
				if (is_break(*q)) {
					if (!multiline) return fail(q,"Unterminated escape sequence");
					if (*q=='\r' && q+1<bound && q[1]=='\n') q++;
					q++;
					while (q<bound && is_space(*q)) q++;
					continue;
				}
				unsigned long cp=0;
				switch (*q) {
					case '0': out.push_back(static_cast<typename string_t::value_type>('\0'));break;
					case 'a': out.push_back('\a');break;
					case 'b': out.push_back('\b');break;
					case 't': out.push_back('\t');break;
					case '\t': out.push_back('\t');break;
					case 'n': out.push_back('\n');break;
					case 'v': out.push_back('\v');break;
					case 'f': out.push_back('\f');break;
					case 'r': out.push_back('\r');break;
					case 'e': out.push_back(static_cast<typename string_t::value_type>(0x1B));break;
					case ' ': out.push_back(' ');break;
					case '"': out.push_back('"');break;
					case '/': out.push_back('/');break;
					case '\\': out.push_back('\\');break;
					case 'N': append_codepoint(out,0x85);break;
					case '_': append_codepoint(out,0xA0);break;
					case 'L': append_codepoint(out,0x2028);break;
					case 'P': append_codepoint(out,0x2029);break;
					case 'x': {
						if (!read_hex_digits(q+1,2,cp)) return fail(q,"Invalid hexadecimal escape");
						append_codepoint(out,cp);
						q+=2;
						break;
					}
					case 'u': {
						if (!read_hex_digits(q+1,4,cp)) return fail(q,"Invalid hexadecimal escape");
						append_codepoint(out,cp);
						q+=4;
						break;
					}
					case 'U': {
						if (!read_hex_digits(q+1,8,cp)) return fail(q,"Invalid hexadecimal escape");
						append_codepoint(out,cp);
						q+=8;
						break;
					}
					default: return fail(q,"Invalid escape sequence in double-quoted scalar");
				}
				q++;
			}
			return fail(p,multiline?"Unterminated double-quoted scalar":"Unterminated double-quoted scalar (multi-line quoted scalars require multiline_scalars)");
		}
		//引用标量断行折叠:行尾空白剥离,单断行=空格,k个空行=k个换行,续行前导空白剥离。
		void fold_quoted_break(const char*& q,const char* bound,string_t& out) {
			while (!out.empty() && (out.back()==' ' || out.back()=='\t')) out.pop_back();
			std::size_t breaks=0;
			while (q<bound) {
				if (is_break(*q)) {
					if (*q=='\r' && q+1<bound && q[1]=='\n') q++;
					q++;
					breaks++;
					continue;
				}
				if (is_space(*q)) {
					const char* probe=q;
					while (probe<bound && is_space(*probe)) probe++;
					if (probe<bound && is_break(*probe)) {
						q=probe;
						continue;
					}
					q=probe;
					break;
				}
				break;
			}
			if (breaks==1) out.push_back(' ');
			else {
				for (std::size_t i=1;i<breaks;i++) out.push_back('\n');
			}
		}

		bool scan_block_scalar(const char* p) {
			const char style_char=*p;
			const char* q=p+1;
			int chomp=0;
			int explicit_indent=0;
			while (q<line_end && (*q=='+' || *q=='-' || (*q>='1' && *q<='9'))) {
				if (*q=='+') {
					if (chomp) return fail(q,"Duplicate chomping indicator");
					chomp=2;
				} else if (*q=='-') {
					if (chomp) return fail(q,"Duplicate chomping indicator");
					chomp=1;
				} else {
					if (explicit_indent) return fail(q,"Duplicate indentation indicator");
					explicit_indent=*q-'0';
				}
				q++;
			}
			while (q<line_end && is_space(*q)) q++;
			if (q<line_end && *q!='#') return fail(q,"Unexpected characters after block scalar header");
			const std::size_t parent=expect_node?pending_indent:(blocks.empty()?0:blocks.back().indent);
			bool indent_known=(explicit_indent>0);
			std::size_t content_indent=indent_known?parent+static_cast<std::size_t>(explicit_indent):0;
			std::vector<string_t> lines;
			const char* cursor=next_position;
			while (cursor<last) {
				const char* ls=cursor;
				const char* le=ls;
				while (le<last && !is_break(*le)) le++;
				const char* r=ls;
				std::size_t line_indent=0;
				while (r<le && *r==' ') {
					r++;
					line_indent++;
				}
				const char* t=ls;
				while (t<le && is_space(*t)) t++;
				const bool blank=(t==le);
				if (!blank) {
					if (line_indent==0 && le-r>=3 && ((r[0]=='-' && r[1]=='-' && r[2]=='-') || (r[0]=='.' && r[1]=='.' && r[2]=='.')) && (r+3==le || is_space(r[3]))) break;
					if (!indent_known) {
						if (line_indent<=parent && !(blocks.empty() && !expect_node && line_indent==0)) break;
						content_indent=line_indent;
						indent_known=true;
					}
					if (line_indent<content_indent) break;
				}
				if (blank) lines.push_back(string_t());
				else {
					const char* strip=ls;
					std::size_t removed=0;
					while (strip<le && *strip==' ' && removed<content_indent) {
						strip++;
						removed++;
					}
					lines.push_back(string_t(strip,le));
				}
				cursor=after_line(le);
			}
			string_t text;
			if (style_char=='|') {
				for (std::size_t i=0;i<lines.size();i++) {
					text.append(lines[i].begin(),lines[i].end());
					text.push_back('\n');
				}
			} else {
				bool previous_empty=true;
				bool previous_more_indented=false;
				bool first_line=true;
				for (std::size_t i=0;i<lines.size();i++) {
					const string_t& line=lines[i];
					const bool empty_line=line.empty();
					const bool more_indented=!empty_line && (line[0]==' ' || line[0]=='\t');
					if (empty_line) text.push_back('\n');
					else {
						if (first_line) { }
						else if (previous_empty) { }
						else if (more_indented || previous_more_indented) text.push_back('\n');
						else text.push_back(' ');
						text.append(line.begin(),line.end());
						first_line=false;
					}
					previous_empty=empty_line;
					previous_more_indented=more_indented;
				}
				if (!lines.empty() && !lines.back().empty()) text.push_back('\n');
			}
			if (chomp==1) {
				while (!text.empty() && text.back()=='\n') text.pop_back();
			} else if (chomp==0) {
				while (!text.empty() && text.back()=='\n') text.pop_back();
				if (!text.empty()) text.push_back('\n');
			}
			emit_scalar(p,std::move(text),style_char=='|'?YSS_LITERAL:YSS_FOLDED);
			next_position=cursor;
			return true;
		}

		bool scan_flow(const char*& p) {
			int depth=0;
			while (true) {
				while (p<last && (is_space(*p) || is_break(*p))) p++;
				if (p<last && *p=='#' && (p==base || is_space(p[-1]) || is_break(p[-1]))) {
					while (p<last && !is_break(*p)) p++;
					continue;
				}
				if (p>=last) return fail(p,"Unterminated flow collection");
				const char c=*p;
				if (c=='[') {
					emit(YS_FSEQ_START,p);
					depth++;
					p++;
					continue;
				}
				if (c=='{') {
					emit(YS_FMAP_START,p);
					depth++;
					p++;
					continue;
				}
				if (c==']' || c=='}') {
					emit(c==']'?YS_FSEQ_END:YS_FMAP_END,p);
					depth--;
					p++;
					if (depth<=0) break;
					continue;
				}
				if (c==',') {
					emit(YS_COMMA,p);
					p++;
					continue;
				}
				if (c==':' && (p+1>=last || is_space(p[1]) || is_break(p[1]) || is_flow_indicator(p[1]))) {
					emit(YS_COLON,p);
					p++;
					continue;
				}
				if (c=='?' && (p+1>=last || is_space(p[1]) || is_break(p[1]))) return fail(p,"Explicit mapping keys ('? ') are not supported");
				if (c=='|' || c=='>') return fail(p,"Block scalars are not allowed in flow context");
				if (c=='&' || c=='*') {
					const char* q=p+1;
					while (q<last && !is_space(*q) && !is_break(*q) && !is_flow_indicator(*q)) q++;
					if (q==p+1) return fail(p,c=='&'?"Empty anchor name":"Empty alias name");
					yaml_token token;
					token.symbol=(c=='&')?YS_ANCHOR:YS_ALIAS;
					token.position=position(p);
					token.text=string_t(p+1,q);
					tokens.push_back(std::move(token));
					if (c=='&') expect_node=true;
					else expect_node=false;
					p=q;
					continue;
				}
				if (c=='!') {
					const char* q=p+1;
					if (q<last && *q=='<') {
						q++;
						while (q<last && *q!='>' && !is_break(*q)) q++;
						if (q>=last || *q!='>') return fail(p,"Unterminated verbatim tag");
						q++;
					} else {
						while (q<last && !is_space(*q) && !is_break(*q) && !is_flow_indicator(*q)) q++;
					}
					yaml_token token;
					token.symbol=YS_TAG;
					token.position=position(p);
					token.text=string_t(p,q);
					tokens.push_back(std::move(token));
					expect_node=true;
					p=q;
					continue;
				}
				if (c=='\'' || c=='"') {
					const char* qe=p;
					while (qe<last && !is_break(*qe)) qe++;
					line_end=qe;
					string_t text;
					const char* q=p;
					if (!scan_quoted(q,text)) return false;
					emit_scalar(p,std::move(text),c=='\''?YSS_SINGLE_QUOTED:YSS_DOUBLE_QUOTED);
					p=q;
					continue;
				}
				const char* q=p;
				while (q<last && !is_break(*q) && !is_flow_indicator(*q)) {
					if (*q==':' && (q+1>=last || is_space(q[1]) || is_break(q[1]) || is_flow_indicator(q[1]))) break;
					if (*q=='#' && q>p && is_space(q[-1])) break;
					q++;
				}
				const char* text_last=q;
				while (text_last>p && is_space(text_last[-1])) text_last--;
				if (text_last==p) return fail(p,"Unexpected character in flow context");
				string_t text(p,text_last);
				if (multiline) {
					while (true) {
						const char* peek=q;
						std::size_t breaks=0;
						while (peek<last) {
							if (is_break(*peek)) {
								if (*peek=='\r' && peek+1<last && peek[1]=='\n') peek++;
								peek++;
								breaks++;
								continue;
							}
							if (is_space(*peek)) {
								peek++;
								continue;
							}
							break;
						}
						if (breaks==0 || peek>=last) break;
						const char head=*peek;
						if (is_flow_indicator(head) || head=='#' || head==':' || head=='&' || head=='*' || head=='!' || head=='?' || head=='\'' || head=='"' || head=='|' || head=='>') break;
						const char* segment_end=peek;
						while (segment_end<last && !is_break(*segment_end) && !is_flow_indicator(*segment_end)) {
							if (*segment_end==':' && (segment_end+1>=last || is_space(segment_end[1]) || is_break(segment_end[1]) || is_flow_indicator(segment_end[1]))) break;
							if (*segment_end=='#' && segment_end>peek && is_space(segment_end[-1])) break;
							segment_end++;
						}
						const char* segment_last=segment_end;
						while (segment_last>peek && is_space(segment_last[-1])) segment_last--;
						if (segment_last==peek) break;
						if (breaks==1) text.push_back(' ');
						else {
							for (std::size_t i=1;i<breaks;i++) text.push_back('\n');
						}
						text.append(peek,segment_last);
						q=segment_end;
					}
				}
				emit_scalar(p,std::move(text),YSS_PLAIN);
				p=q;
				continue;
			}
			relocate_line(p);
			return true;
		}

		bool scan_line(const char* p) {
			bool entries_only=true;
			while (true) {
				while (p<line_end && is_space(*p)) p++;
				if (p==line_end) return true;
				if (*p=='#') return true;
				const std::size_t column=static_cast<std::size_t>(p-line_start);
				const char c=*p;
				if (c=='-' && (p+1==line_end || is_space(p[1]))) {
					if (!entries_only) return fail(p,"Block sequence entries are not allowed in this context");
					if (blocks.empty() || !blocks.back().sequence || blocks.back().indent!=column) {
						blocks.push_back(block_frame{column,true});
						emit(YS_BSEQ_START,p);
					}
					emit(YS_ENTRY,p);
					expect_node=true;
					pending_indent=column;
					pending_in_map=false;
					p++;
					continue;
				}
				if (c=='?' && (p+1==line_end || is_space(p[1]))) return fail(p,"Explicit mapping keys ('? ') are not supported");
				if (c==':' && (p+1==line_end || is_space(p[1]))) return fail(p,"A mapping value indicator requires an inline scalar key (complex keys are not supported)");
				if (c=='&' || c=='*') {
					entries_only=false;
					const char* q=p+1;
					while (q<line_end && !is_space(*q) && !is_flow_indicator(*q)) q++;
					if (q==p+1) return fail(p,c=='&'?"Empty anchor name":"Empty alias name");
					yaml_token token;
					token.symbol=(c=='&')?YS_ANCHOR:YS_ALIAS;
					token.position=position(p);
					token.text=string_t(p+1,q);
					tokens.push_back(std::move(token));
					if (c=='&') expect_node=true;
					else expect_node=false;
					p=q;
					continue;
				}
				if (c=='!') {
					entries_only=false;
					const char* q=p+1;
					if (q<line_end && *q=='<') {
						q++;
						while (q<line_end && *q!='>') q++;
						if (q==line_end) return fail(p,"Unterminated verbatim tag");
						q++;
					} else {
						while (q<line_end && !is_space(*q) && !is_flow_indicator(*q)) q++;
					}
					yaml_token token;
					token.symbol=YS_TAG;
					token.position=position(p);
					token.text=string_t(p,q);
					tokens.push_back(std::move(token));
					expect_node=true;
					p=q;
					continue;
				}
				if (c=='|' || c=='>') {
					entries_only=false;
					return scan_block_scalar(p);
				}
				if (c=='[' || c=='{') {
					entries_only=false;
					if (!scan_flow(p)) return false;
					const char* q=p;
					while (q<line_end && is_space(*q)) q++;
					if (q<line_end && *q==':' && (q+1==line_end || is_space(q[1]))) return fail(q,"Complex mapping keys are not supported");
					continue;
				}
				if (c=='\'' || c=='"') {
					entries_only=false;
					string_t text;
					const char* q=p;
					const char* previous_line_end=line_end;
					if (!scan_quoted(q,text)) return false;
					const int style=(c=='\'')?YSS_SINGLE_QUOTED:YSS_DOUBLE_QUOTED;
					if (q>previous_line_end) {
						//跨行引用标量:重定位行指针;多行引用标量不可作隐式键。
						relocate_line(q);
						emit_scalar(p,std::move(text),style);
						p=q;
						continue;
					}
					const char* r=q;
					while (r<line_end && is_space(*r)) r++;
					if (r<line_end && *r==':' && (r+1==line_end || is_space(r[1]))) {
						if (!begin_mapping_pair(p,column,std::move(text),style,r)) return false;
						p=r+1;
						continue;
					}
					emit_scalar(p,std::move(text),style);
					p=q;
					continue;
				}
				entries_only=false;
				const char* span_last=line_end;
				for (const char* r=p;r<line_end;r++) {
					if (*r=='#' && r>p && is_space(r[-1])) {
						span_last=r;
						break;
					}
				}
				const char* colon=nullptr;
				for (const char* r=p;r<span_last;r++) {
					if (*r==':' && (r+1>=span_last || is_space(r[1]))) {
						colon=r;
						break;
					}
				}
				if (colon) {
					const char* key_last=colon;
					while (key_last>p && is_space(key_last[-1])) key_last--;
					if (key_last==p) return fail(p,"Empty plain scalar mapping key");
					if (!begin_mapping_pair(p,column,string_t(p,key_last),YSS_PLAIN,colon)) return false;
					p=colon+1;
					continue;
				}
				const char* text_last=span_last;
				while (text_last>p && is_space(text_last[-1])) text_last--;
				const std::ptrdiff_t owner=expect_node?static_cast<std::ptrdiff_t>(pending_indent):(blocks.empty()?-1:static_cast<std::ptrdiff_t>(blocks.back().indent));
				emit_scalar(p,string_t(p,text_last),YSS_PLAIN);
				if (multiline) {
					plain_continuing=(span_last==line_end);
					plain_owner=owner;
					pending_breaks=0;
				}
				p=span_last;
				continue;
			}
		}
	};

	static bool tokenize(std::string_view input,std::vector<yaml_token>& tokens,std::size_t& error_position,std::string& error_message,const yaml_parse_options& options) {
		if (!validate_characters(input,error_position,error_message)) return false;
		tokenizer scanner(input,tokens);
		scanner.multiline=options.multiline_scalars;
		if (!scanner.run()) {
			error_position=scanner.error_position;
			error_message=scanner.error_message;
			return false;
		}
		yaml_token eof_token;
		eof_token.symbol=YS_EOF;
		eof_token.position=input.size();
		tokens.push_back(std::move(eof_token));
		return true;
	}

	using parser_t=syntax::parser<yaml_symbol,yaml_production>;

	//SLR(1)文法:无ε产生式,全部左递归。结构终结符由词法合成,空流由START->EOF单列。
	static bool initialize_grammar(parser_t& target) {
		auto unit=[](yaml_symbol left,std::initializer_list<yaml_symbol> rights,yaml_production id){
			return syntax::single_parser_unit<yaml_symbol,yaml_production>(left,rights,id);
		};
		target.units={
			unit(YS_START,{YS_STREAM,YS_EOF},YP_START),
			unit(YS_START,{YS_EOF},YP_START_EMPTY),
			unit(YS_STREAM,{YS_DOCUMENT},YP_STREAM_FIRST),
			unit(YS_STREAM,{YS_STREAM,YS_DOCUMENT},YP_STREAM_APPEND),
			unit(YS_DOCUMENT,{YS_DOC_START,YS_NODE,YS_DOC_END},YP_DOCUMENT_PLAIN),
			unit(YS_DOCUMENT,{YS_DOC_START,YS_DOC_END},YP_DOCUMENT_BLANK),
			unit(YS_DOCUMENT,{YS_DIRECTIVE_SEQ,YS_DOC_START,YS_NODE,YS_DOC_END},YP_DOCUMENT_DIRECTIVE),
			unit(YS_DOCUMENT,{YS_DIRECTIVE_SEQ,YS_DOC_START,YS_DOC_END},YP_DOCUMENT_DIRECTIVE_BLANK),
			unit(YS_DIRECTIVE_SEQ,{YS_DIRECTIVE},YP_DIRECTIVES_FIRST),
			unit(YS_DIRECTIVE_SEQ,{YS_DIRECTIVE_SEQ,YS_DIRECTIVE},YP_DIRECTIVES_APPEND),
			unit(YS_NODE,{YS_SCALAR},YP_NODE_SCALAR),
			unit(YS_NODE,{YS_EMPTY},YP_NODE_EMPTY),
			unit(YS_NODE,{YS_ALIAS},YP_NODE_ALIAS),
			unit(YS_NODE,{YS_ANCHOR,YS_NODE},YP_NODE_ANCHOR),
			unit(YS_NODE,{YS_TAG,YS_NODE},YP_NODE_TAG),
			unit(YS_NODE,{YS_BSEQ_START,YS_BSEQ_ITEMS,YS_BSEQ_END},YP_NODE_BSEQ),
			unit(YS_NODE,{YS_BMAP_START,YS_BMAP_ITEMS,YS_BMAP_END},YP_NODE_BMAP),
			unit(YS_NODE,{YS_FSEQ_START,YS_FSEQ_END},YP_NODE_FSEQ_EMPTY),
			unit(YS_NODE,{YS_FSEQ_START,YS_FLOW_ITEMS,YS_FSEQ_END},YP_NODE_FSEQ),
			unit(YS_NODE,{YS_FMAP_START,YS_FMAP_END},YP_NODE_FMAP_EMPTY),
			unit(YS_NODE,{YS_FMAP_START,YS_FLOW_PAIRS,YS_FMAP_END},YP_NODE_FMAP),
			unit(YS_BSEQ_ITEMS,{YS_ENTRY,YS_NODE},YP_BSEQ_FIRST),
			unit(YS_BSEQ_ITEMS,{YS_BSEQ_ITEMS,YS_ENTRY,YS_NODE},YP_BSEQ_APPEND),
			unit(YS_BMAP_ITEMS,{YS_BMAP_PAIR},YP_BMAP_FIRST),
			unit(YS_BMAP_ITEMS,{YS_BMAP_ITEMS,YS_BMAP_PAIR},YP_BMAP_APPEND),
			unit(YS_BMAP_PAIR,{YS_SCALAR,YS_COLON,YS_NODE},YP_BMAP_PAIR),
			unit(YS_FLOW_ITEMS,{YS_NODE},YP_FLOW_ITEMS_FIRST),
			unit(YS_FLOW_ITEMS,{YS_FLOW_ITEMS,YS_COMMA,YS_NODE},YP_FLOW_ITEMS_APPEND),
			unit(YS_FLOW_PAIRS,{YS_FLOW_PAIR},YP_FLOW_PAIRS_FIRST),
			unit(YS_FLOW_PAIRS,{YS_FLOW_PAIRS,YS_COMMA,YS_FLOW_PAIR},YP_FLOW_PAIRS_APPEND),
			unit(YS_FLOW_PAIR,{YS_SCALAR,YS_COLON,YS_NODE},YP_FLOW_PAIR),
			unit(YS_FLOW_PAIR,{YS_SCALAR},YP_FLOW_PAIR_KEY),
		};
		target.generate_parser();
		return true;
	}
	//The LR machine mutates its own tables while parsing, so every thread owns one.
	//This removes the global lock and makes reentrant parsing possible.
	static parser_t& grammar() {
		static thread_local parser_t instance(YS_START,YS_EPSILON,YS_EOF);
		static thread_local const bool initialized=initialize_grammar(instance);
		static_cast<void>(initialized);
		return instance;
	}

	class yaml_listener : public syntax::parser_listener<yaml_symbol,yaml_production> {
		std::vector<yaml_token>* tokens_=nullptr;
		sax_t* sax_=nullptr;
		const parser_t* parser_=nullptr;
		yaml_schema_type schema_=YST_JSON;
		bool aborted_=false;
		bool failed_=false;
		bool has_pending_tag_=false;
		string_t pending_tag_{};

		void abort_check(bool keep_going) {
			if (!keep_going) aborted_=true;
		}
		void fail(const yaml_token& token,const std::string& message) {
			failed_=true;
			const std::string text(token.text.begin(),token.text.end());
			sax_->parse_error(token.position,text,message);
		}
		void clear_pending_tag() {
			has_pending_tag_=false;
			pending_tag_.clear();
		}
		void emit_scalar_event(yaml_token& token) {
			int forced=0;
			if (has_pending_tag_) {
				forced=tag_class(pending_tag_);
				clear_pending_tag();
			}
			const yaml_scalar_style style=static_cast<yaml_scalar_style>(token.extra);
			switch (forced) {
				case 1: {
					abort_check(sax_->string(token.text,style));
					return;
				}
				case 2: {
					int_t value=0;
					if (plain_integer(token.text,value,YST_CORE)) abort_check(sax_->number_integer(value));
					else fail(token,"Scalar does not conform to the !!int tag");
					return;
				}
				case 3: {
					float_t value=0;
					if (plain_floating(token.text,value,YST_CORE)) abort_check(sax_->number_float(value,token.text));
					else fail(token,"Scalar does not conform to the !!float tag");
					return;
				}
				case 4: {
					const int value=plain_boolean(token.text,YST_CORE);
					if (value<0) fail(token,"Scalar does not conform to the !!bool tag");
					else abort_check(sax_->boolean(static_cast<boolean_t>(value==1)));
					return;
				}
				case 5: {
					if (plain_is_null(token.text,YST_CORE)) abort_check(sax_->null());
					else fail(token,"Scalar does not conform to the !!null tag");
					return;
				}
				default: break;
			}
			if (style!=YSS_PLAIN) {
				abort_check(sax_->string(token.text,style));
				return;
			}
			if (plain_is_null(token.text,schema_)) {
				abort_check(sax_->null());
				return;
			}
			const int truth=plain_boolean(token.text,schema_);
			if (truth>=0) {
				abort_check(sax_->boolean(static_cast<boolean_t>(truth==1)));
				return;
			}
			int_t integer_value=0;
			if (plain_integer(token.text,integer_value,schema_)) {
				abort_check(sax_->number_integer(integer_value));
				return;
			}
			float_t floating_value=0;
			if (plain_floating(token.text,floating_value,schema_)) {
				abort_check(sax_->number_float(floating_value,token.text));
				return;
			}
			abort_check(sax_->string(token.text,style));
		}

	public:
		std::string expected_symbols(int state) const {
			if (!parser_) return std::string();
			std::vector<std::string> names;
			for (const auto& it:parser_->lr_sheet) {
				if (it.first.second!=static_cast<uintptr_t>(state) || it.second.type==syntax::ST_ERROR) continue;
				const yaml_symbol symbol=it.first.first;
				if (symbol==YS_EPSILON) continue;
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
		void reset(std::vector<yaml_token>& tokens,sax_t& sax,const parser_t& parser,yaml_schema_type schema) {
			tokens_=&tokens;
			sax_=&sax;
			parser_=&parser;
			schema_=schema;
			aborted_=false;
			failed_=false;
			clear_pending_tag();
			this->enabled=true;
		}
		bool aborted() const noexcept {
			return aborted_;
		}
		bool failed() const noexcept {
			return failed_;
		}
		intptr_t on_shift(uintptr_t id,int state,yaml_symbol word) override {
			static_cast<void>(state);
			if (aborted_ || failed_) return 0;
			yaml_token& token=(*tokens_)[id-1];
			abort_check(sax_->location(token.position));
			if (aborted_) return 0;
			switch (word) {
				case YS_DOC_START: abort_check(sax_->start_document());break;
				case YS_BSEQ_START:
				case YS_FSEQ_START: {
					clear_pending_tag();
					abort_check(sax_->start_sequence(static_cast<std::size_t>(-1)));
					break;
				}
				case YS_BMAP_START:
				case YS_FMAP_START: {
					clear_pending_tag();
					abort_check(sax_->start_mapping(static_cast<std::size_t>(-1)));
					break;
				}
				case YS_COLON: abort_check(sax_->key((*tokens_)[id-2].text));break;
				case YS_ANCHOR: abort_check(sax_->anchor(token.text));break;
				case YS_TAG: {
					has_pending_tag_=true;
					pending_tag_=token.text;
					abort_check(sax_->tag(token.text));
					break;
				}
				default: break;
			}
			return 0;
		}
		intptr_t on_reduction(uintptr_t id,int state,int next,yaml_production sentence_id,int reduction_num) override {
			static_cast<void>(state);
			static_cast<void>(next);
			static_cast<void>(reduction_num);
			if (aborted_ || failed_) return 0;
			switch (sentence_id) {
				case YP_NODE_SCALAR:
				case YP_NODE_EMPTY: emit_scalar_event((*tokens_)[id-2]);break;
				case YP_NODE_ALIAS: {
					yaml_token& token=(*tokens_)[id-2];
					abort_check(sax_->location(token.position));
					if (aborted_) return 0;
					if (has_pending_tag_) {
						fail(token,"An alias node must not have properties");
						return 0;
					}
					abort_check(sax_->alias(token.text));
					break;
				}
				case YP_NODE_BSEQ:
				case YP_NODE_FSEQ:
				case YP_NODE_FSEQ_EMPTY: abort_check(sax_->end_sequence());break;
				case YP_NODE_BMAP:
				case YP_NODE_FMAP:
				case YP_NODE_FMAP_EMPTY: abort_check(sax_->end_mapping());break;
				case YP_FLOW_PAIR_KEY: {
					yaml_token& token=(*tokens_)[id-2];
					abort_check(sax_->key(token.text));
					if (!aborted_) abort_check(sax_->null());
					break;
				}
				case YP_DIRECTIVES_FIRST:
				case YP_DIRECTIVES_APPEND: {
					yaml_token& token=(*tokens_)[id-2];
					abort_check(sax_->directive(token.text,token.aux));
					break;
				}
				case YP_DOCUMENT_PLAIN:
				case YP_DOCUMENT_BLANK:
				case YP_DOCUMENT_DIRECTIVE:
				case YP_DOCUMENT_DIRECTIVE_BLANK: abort_check(sax_->end_document());break;
				case YP_START:
				case YP_START_EMPTY:
				case YP_STREAM_FIRST:
				case YP_STREAM_APPEND:
				case YP_NODE_ANCHOR:
				case YP_NODE_TAG:
				case YP_BSEQ_FIRST:
				case YP_BSEQ_APPEND:
				case YP_BMAP_FIRST:
				case YP_BMAP_APPEND:
				case YP_BMAP_PAIR:
				case YP_FLOW_ITEMS_FIRST:
				case YP_FLOW_ITEMS_APPEND:
				case YP_FLOW_PAIRS_FIRST:
				case YP_FLOW_PAIRS_APPEND:
				case YP_FLOW_PAIR:
				default: break;
			}
			return 0;
		}
		void on_accept() override { }
		int on_error(uintptr_t id,typename syntax::parser_listener<yaml_symbol,yaml_production>::error_type type,int state,yaml_symbol word) override {
			static_cast<void>(type);
			static_cast<void>(state);
			static_cast<void>(word);
			failed_=true;
			std::string message="Unexpected token";
			const std::string expected=expected_symbols(state);
			if (sax_ && tokens_ && id!=static_cast<uintptr_t>(-1) && id>=1 && id<=tokens_->size()) {
				const yaml_token& token=(*tokens_)[id-1];
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
	static bool sax_parse(std::string_view input,sax_t* sax,const yaml_parse_options& options=yaml_parse_options()) {
		std::vector<yaml_token> tokens;
		std::size_t error_position=0;
		std::string error_message;
		if (!tokenize(input,tokens,error_position,error_message,options)) {
			sax->parse_error(error_position,std::string(),error_message);
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
		yaml_listener listener;
		listener.reset(tokens,*sax,parser,options.scalar_schema);
		std::vector<syntax::parser_listener<yaml_symbol,yaml_production>*> outer;
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
	static yaml parse(std::string_view input,document_info_t* info=nullptr,bool allow_exceptions=true,const yaml_parse_options& options=yaml_parse_options()) {
		std::vector<yaml> documents;
		yaml_sax_dom_builder<yaml> builder(documents,info,options);
		const bool ok=sax_parse(input,&builder,options) && builder.completed();
		if (!ok) {
			if (allow_exceptions) throw std::runtime_error(std::string("Parse error at ")+describe_position(input,builder.error_position())+std::string(": ")+(builder.error_message().empty()?std::string("Incomplete document"):builder.error_message()));
			return yaml();
		}
		if (documents.size()!=1) {
			if (allow_exceptions) throw std::runtime_error(documents.empty()?std::string("Parse error: The stream contains no document"):std::string("Parse error: The stream contains multiple documents, use parse_all"));
			return yaml();
		}
		return std::move(documents.front());
	}
	static std::vector<yaml> parse_all(std::string_view input,document_info_t* info=nullptr,bool allow_exceptions=true,const yaml_parse_options& options=yaml_parse_options()) {
		std::vector<yaml> documents;
		yaml_sax_dom_builder<yaml> builder(documents,info,options);
		const bool ok=sax_parse(input,&builder,options) && builder.completed();
		if (!ok) {
			if (allow_exceptions) throw std::runtime_error(std::string("Parse error at ")+describe_position(input,builder.error_position())+std::string(": ")+(builder.error_message().empty()?std::string("Incomplete document"):builder.error_message()));
			return std::vector<yaml>();
		}
		return documents;
	}
	static bool try_parse(std::string_view input,yaml& out,document_info_t* info=nullptr,const yaml_parse_options& options=yaml_parse_options()) {
		std::vector<yaml> documents;
		yaml_sax_dom_builder<yaml> builder(documents,info,options);
		if (!sax_parse(input,&builder,options) || !builder.completed() || documents.size()!=1) return false;
		out=std::move(documents.front());
		return true;
	}
	static bool try_parse(std::string_view input,std::vector<yaml>& out,document_info_t* info=nullptr,const yaml_parse_options& options=yaml_parse_options()) {
		std::vector<yaml> documents;
		yaml_sax_dom_builder<yaml> builder(documents,info,options);
		if (!sax_parse(input,&builder,options) || !builder.completed()) return false;
		out=std::move(documents);
		return true;
	}
	static bool accept(std::string_view input,const yaml_parse_options& options=yaml_parse_options()) {
		yaml_sax_acceptor<yaml> acceptor;
		return sax_parse(input,&acceptor,options);
	}

private:
	static void dump_unicode_escape(string_t& out,unsigned long cp) {
		static const char digits[]="0123456789abcdef";
		out.push_back('\\');
		if (cp<0x10000) {
			out.push_back('u');
			for (int shift=12;shift>=0;shift-=4) out.push_back(digits[(cp>>shift)&0xF]);
		} else {
			out.push_back('U');
			for (int shift=28;shift>=0;shift-=4) out.push_back(digits[(cp>>shift)&0xF]);
		}
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
			if (c<0x20 || c==0x7F) {
				dump_unicode_escape(out,c);
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
			if (ensure_ascii) dump_unicode_escape(out,cp);
			else {
				for (std::size_t i=0;i<=extra;i++) out.push_back(static_cast<typename string_t::value_type>(first[i]));
			}
			first+=extra+1;
		}
	}
	//First characters of null_regex, true_regex, false_regex, integer_regex and
	//floating_regex. A plain scalar starting with anything else cannot match any of
	//them, so the whole resolution step can be skipped.
	static bool could_resolve_to_another_type(char c) noexcept {
		return c=='~' || c=='n' || c=='N' || c=='t' || c=='T' || c=='f' || c=='F' || c=='+' || c=='-' || c=='.' || (c>='0' && c<='9');
	}
	static bool plain_safe(const string_t& s,bool flow,bool ensure_ascii,yaml_schema_type schema) {
		if (s.empty()) return false;
		static const char indicators[]="-?:,[]{}#&*!|>'\"%@` \t";
		const char head=static_cast<char>(s[0]);
		for (const char* it=indicators;*it;it++) {
			if (head==*it) return false;
		}
		const char tail=static_cast<char>(s[s.size()-1]);
		if (tail==' ' || tail=='\t') return false;
		for (std::size_t i=0;i<s.size();i++) {
			const unsigned char c=static_cast<unsigned char>(s[i]);
			if (c<0x20 || c==0x7F) return false;
			if (ensure_ascii && c>=0x80) return false;
			if (c==':' && (i+1==s.size() || s[i+1]==' ' || s[i+1]=='\t')) return false;
			if (c=='#' && i>0 && (s[i-1]==' ' || s[i-1]=='\t')) return false;
			if (flow && (c==',' || c=='[' || c==']' || c=='{' || c=='}' || c==':')) return false;
		}
		if (schema==YST_FAILSAFE) return true;
		if (!could_resolve_to_another_type(head)) return true;
		if (plain_is_null(s,schema)) return false;
		if (plain_boolean(s,schema)>=0) return false;
		int_t integer_value=0;
		if (plain_integer(s,integer_value,schema)) return false;
		float_t floating_value=0;
		if (plain_floating(s,floating_value,schema)) return false;
		return true;
	}
	static void dump_scalar_string(string_t& out,const string_t& s,bool flow,bool ensure_ascii,yaml_schema_type schema) {
		if (plain_safe(s,flow,ensure_ascii,schema)) {
			out.append(s.begin(),s.end());
			return;
		}
		out.push_back('"');
		dump_escaped(out,s,ensure_ascii);
		out.push_back('"');
	}
	static void dump_integer(string_t& out,int_t value) {
		const std::string text=std::to_string(static_cast<long long>(value));
		out.append(text.begin(),text.end());
	}
	static void dump_floating(string_t& out,float_t value) {
		if (std::isnan(value)) {
			out.append({'.','n','a','n'});
			return;
		}
		if (std::isinf(value)) {
			if (value<0) out.push_back('-');
			out.append({'.','i','n','f'});
			return;
		}
		char buffer[64];
		int length=std::snprintf(buffer,sizeof(buffer),"%.15g",static_cast<double>(value));
		if (std::strtod(buffer,nullptr)!=static_cast<double>(value)) length=std::snprintf(buffer,sizeof(buffer),"%.16g",static_cast<double>(value));
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
	static bool is_scalar_node(const base_t& node) noexcept {
		switch (node.type()) {
			case structure::DDT_NULL:
			case structure::DDT_BOOL:
			case structure::DDT_INT:
			case structure::DDT_FLOAT:
			case structure::DDT_STRING: return true;
			default: return false;
		}
	}
	static void dump_scalar(const base_t& node,string_t& out,bool flow,bool ensure_ascii,yaml_schema_type schema) {
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
				dump_scalar_string(out,*node.template get_ptr<const string_t*>(),flow,ensure_ascii,schema);
				break;
			}
			default: break;
		}
	}
	static void unsupported_node(const base_t& node) {
		throw std::invalid_argument("Unsupported node type "+std::to_string(static_cast<long long>(static_cast<int>(node.type())))+", convert it to the base dom model first (assign_converted/convert_to)");
	}
	//剥取连续YDT_ANCHOR包装,收集"&名 "前缀并返回最内层节点。
	static const base_t* peel_anchors(const base_t& node,string_t& prefix) {
		const base_t* current=&node;
		while (is_anchor(*current)) {
			prefix.push_back('&');
			const string_t& name=anchor_name(*current);
			prefix.append(name.begin(),name.end());
			prefix.push_back(' ');
			current=&anchor_target(*current);
		}
		return current;
	}
	static void dump_flow(const base_t& node,string_t& out,bool ensure_ascii,yaml_schema_type schema) {
		if (is_alias(node)) {
			out.push_back('*');
			const string_t& name=alias_name(node);
			out.append(name.begin(),name.end());
			return;
		}
		if (is_anchor(node)) {
			out.push_back('&');
			const string_t& name=anchor_name(node);
			out.append(name.begin(),name.end());
			out.push_back(' ');
			dump_flow(anchor_target(node),out,ensure_ascii,schema);
			return;
		}
		if (is_scalar_node(node)) {
			dump_scalar(node,out,true,ensure_ascii,schema);
			return;
		}
		switch (node.type()) {
			case structure::DDT_ARRAY: {
				out.push_back('[');
				for (auto it=node.cbegin();it!=node.cend();) {
					dump_flow(*it,out,ensure_ascii,schema);
					it++;
					if (it!=node.cend()) {
						out.push_back(',');
						out.push_back(' ');
					}
				}
				out.push_back(']');
				break;
			}
			case structure::DDT_OBJECT: {
				out.push_back('{');
				for (auto it=node.cbegin();it!=node.cend();) {
					dump_scalar_string(out,it.key(),true,ensure_ascii,schema);
					out.push_back(':');
					out.push_back(' ');
					dump_flow(*it,out,ensure_ascii,schema);
					it++;
					if (it!=node.cend()) {
						out.push_back(',');
						out.push_back(' ');
					}
				}
				out.push_back('}');
				break;
			}
			default: unsupported_node(node);
		}
	}
	static void dump_indent(string_t& out,std::size_t count,typename string_t::value_type indent_char) {
		for (std::size_t i=0;i<count;i++) out.push_back(indent_char);
	}
	static void dump_block(const base_t& node,string_t& out,int indent_step,typename string_t::value_type indent_char,bool ensure_ascii,yaml_schema_type schema,std::size_t current_indent,bool skip_first_indent) {
		switch (node.type()) {
			case structure::DDT_ARRAY: {
				bool first=true;
				for (auto it=node.cbegin();it!=node.cend();it++) {
					if (!(skip_first_indent && first)) dump_indent(out,current_indent,indent_char);
					first=false;
					out.push_back('-');
					out.push_back(' ');
					string_t prefix;
					const base_t& child=*peel_anchors(*it,prefix);
					if (is_alias(child)) {
						out.append(prefix.begin(),prefix.end());
						out.push_back('*');
						const string_t& name=alias_name(child);
						out.append(name.begin(),name.end());
						out.push_back('\n');
					} else if (is_scalar_node(child)) {
						out.append(prefix.begin(),prefix.end());
						dump_scalar(child,out,false,ensure_ascii,schema);
						out.push_back('\n');
					} else if (child.type()==structure::DDT_ARRAY || child.type()==structure::DDT_OBJECT) {
						if (child.empty()) {
							out.append(prefix.begin(),prefix.end());
							out.push_back(child.type()==structure::DDT_ARRAY?'[':'{');
							out.push_back(child.type()==structure::DDT_ARRAY?']':'}');
							out.push_back('\n');
						} else if (!prefix.empty()) {
							//锚定容器:锚点独占"- &名"行,内容换行缩进(项内续行固定+2列)。
							prefix.pop_back();
							out.append(prefix.begin(),prefix.end());
							out.push_back('\n');
							dump_block(child,out,indent_step,indent_char,ensure_ascii,schema,current_indent+2,false);
						} else dump_block(child,out,indent_step,indent_char,ensure_ascii,schema,current_indent+2,true);
					} else unsupported_node(child);
				}
				break;
			}
			case structure::DDT_OBJECT: {
				bool first=true;
				for (auto it=node.cbegin();it!=node.cend();it++) {
					if (!(skip_first_indent && first)) dump_indent(out,current_indent,indent_char);
					first=false;
					dump_scalar_string(out,it.key(),false,ensure_ascii,schema);
					out.push_back(':');
					string_t prefix;
					const base_t& child=*peel_anchors(*it,prefix);
					if (is_alias(child)) {
						out.push_back(' ');
						out.append(prefix.begin(),prefix.end());
						out.push_back('*');
						const string_t& name=alias_name(child);
						out.append(name.begin(),name.end());
						out.push_back('\n');
					} else if (is_scalar_node(child)) {
						out.push_back(' ');
						out.append(prefix.begin(),prefix.end());
						dump_scalar(child,out,false,ensure_ascii,schema);
						out.push_back('\n');
					} else if (child.type()==structure::DDT_ARRAY || child.type()==structure::DDT_OBJECT) {
						if (child.empty()) {
							out.push_back(' ');
							out.append(prefix.begin(),prefix.end());
							out.push_back(child.type()==structure::DDT_ARRAY?'[':'{');
							out.push_back(child.type()==structure::DDT_ARRAY?']':'}');
							out.push_back('\n');
						} else if (!prefix.empty()) {
							out.push_back(' ');
							prefix.pop_back();
							out.append(prefix.begin(),prefix.end());
							out.push_back('\n');
							dump_block(child,out,indent_step,indent_char,ensure_ascii,schema,current_indent+static_cast<std::size_t>(indent_step),false);
						} else {
							out.push_back('\n');
							dump_block(child,out,indent_step,indent_char,ensure_ascii,schema,current_indent+static_cast<std::size_t>(indent_step),false);
						}
					} else unsupported_node(child);
				}
				break;
			}
			default: unsupported_node(node);
		}
	}

public:
	//Children of a yaml are plain dom nodes, so serialising a subtree through the
	//member dump() would require a deep copy; this entry point does not.
	static string_t dump_node(const base_t& node,int indent=-1,typename string_t::value_type indent_char=' ',bool ensure_ascii=false,yaml_schema_type schema=YST_JSON) {
		string_t result;
		if (indent<0) {
			dump_flow(node,result,ensure_ascii,schema);
			return result;
		}
		string_t prefix;
		const base_t& inner=*peel_anchors(node,prefix);
		if (is_alias(inner)) {
			result.append(prefix.begin(),prefix.end());
			result.push_back('*');
			const string_t& name=alias_name(inner);
			result.append(name.begin(),name.end());
			return result;
		}
		if (is_scalar_node(inner)) {
			result.append(prefix.begin(),prefix.end());
			dump_scalar(inner,result,false,ensure_ascii,schema);
			return result;
		}
		if (inner.type()!=structure::DDT_ARRAY && inner.type()!=structure::DDT_OBJECT) unsupported_node(inner);
		if (inner.empty()) {
			result.append(prefix.begin(),prefix.end());
			result.push_back(inner.type()==structure::DDT_ARRAY?'[':'{');
			result.push_back(inner.type()==structure::DDT_ARRAY?']':'}');
			return result;
		}
		const int indent_step=indent>0?indent:1;
		if (!prefix.empty()) {
			prefix.pop_back();
			result.append(prefix.begin(),prefix.end());
			result.push_back('\n');
		}
		dump_block(inner,result,indent_step,indent_char,ensure_ascii,schema,0,false);
		if (!result.empty() && result.back()=='\n') result.pop_back();
		return result;
	}
	virtual string_t dump(int indent=-1,typename string_t::value_type indent_char=' ',bool ensure_ascii=false,yaml_schema_type schema=YST_JSON) const {
		return dump_node(*this,indent,indent_char,ensure_ascii,schema);
	}
	static string_t dump_document(const base_t& root,const document_info_t* info=nullptr,int indent=2,typename string_t::value_type indent_char=' ',bool ensure_ascii=false,yaml_schema_type schema=YST_JSON) {
		string_t result;
		if (info && info->has_version()) {
			result.append({'%','Y','A','M','L',' '});
			result.append(info->version.begin(),info->version.end());
			result.push_back('\n');
		}
		if (info) {
			for (const auto& it:info->tag_directives) {
				result.append({'%','T','A','G',' '});
				result.append(it.first.begin(),it.first.end());
				result.push_back(' ');
				result.append(it.second.begin(),it.second.end());
				result.push_back('\n');
			}
		}
		result.append({'-','-','-'});
		if (indent<0) {
			result.push_back(' ');
			dump_flow(root,result,ensure_ascii,schema);
			result.push_back('\n');
			return result;
		}
		string_t prefix;
		const base_t& inner=*peel_anchors(root,prefix);
		if (is_alias(inner)) {
			result.push_back(' ');
			result.append(prefix.begin(),prefix.end());
			result.push_back('*');
			const string_t& name=alias_name(inner);
			result.append(name.begin(),name.end());
			result.push_back('\n');
			return result;
		}
		if (is_scalar_node(inner)) {
			result.push_back(' ');
			result.append(prefix.begin(),prefix.end());
			dump_scalar(inner,result,false,ensure_ascii,schema);
			result.push_back('\n');
			return result;
		}
		if (inner.type()!=structure::DDT_ARRAY && inner.type()!=structure::DDT_OBJECT) unsupported_node(inner);
		if (inner.empty()) {
			result.push_back(' ');
			result.append(prefix.begin(),prefix.end());
			result.push_back(inner.type()==structure::DDT_ARRAY?'[':'{');
			result.push_back(inner.type()==structure::DDT_ARRAY?']':'}');
			result.push_back('\n');
			return result;
		}
		if (!prefix.empty()) {
			result.push_back(' ');
			prefix.pop_back();
			result.append(prefix.begin(),prefix.end());
		}
		result.push_back('\n');
		dump_block(inner,result,indent>0?indent:1,indent_char,ensure_ascii,schema,0,false);
		return result;
	}
	static string_t dump_all(const std::vector<yaml>& documents,int indent=2,typename string_t::value_type indent_char=' ',bool ensure_ascii=false,yaml_schema_type schema=YST_JSON) {
		string_t result;
		for (const auto& it:documents) result+=dump_document(it,nullptr,indent,indent_char,ensure_ascii,schema);
		return result;
	}

	friend std::ostream& operator <<(std::ostream& os,const yaml& value) {
		const int indent_step=static_cast<int>(os.width());
		os.width(0);
		const string_t text=value.dump(indent_step>0?indent_step:-1,static_cast<typename string_t::value_type>(os.fill()));
		os.write(reinterpret_cast<const char*>(text.data()),static_cast<std::streamsize>(text.size()));
		return os;
	}
	friend std::istream& operator >>(std::istream& is,yaml& value) {
		std::string content((std::istreambuf_iterator<char>(is)),std::istreambuf_iterator<char>());
		value=parse(content);
		return is;
	}
};

_STDEX_DOM_TPL_DEFAULT_DECLARATION
inline typename yaml<_Int,_Float,_Boolean,_String,_Array,_Object,_Allocator>::string_t to_string(const yaml<_Int,_Float,_Boolean,_String,_Array,_Object,_Allocator>& value) {
	return value.dump();
}

}

_STDEX_DOM_TPL_DEFAULT_DECLARATION
using yaml_t=basic_yaml::yaml<_Int,_Float,_Boolean,_String,_Array,_Object,_Allocator>;
using yaml=yaml_t<>;
using basic_yaml::yaml_sax;
using basic_yaml::yaml_sax_dom_builder;
using basic_yaml::yaml_sax_acceptor;
using basic_yaml::basic_yaml_document_info;
using basic_yaml::yaml_scalar_style;
using basic_yaml::YSS_PLAIN;
using basic_yaml::YSS_SINGLE_QUOTED;
using basic_yaml::YSS_DOUBLE_QUOTED;
using basic_yaml::YSS_LITERAL;
using basic_yaml::YSS_FOLDED;
using basic_yaml::to_string;
using basic_yaml::yaml_data_type;
using basic_yaml::YDT_ANCHOR;
using basic_yaml::YDT_ALIAS;
using basic_yaml::yaml_parse_options;
using basic_yaml::yaml_schema_type;
using basic_yaml::YST_FAILSAFE;
using basic_yaml::YST_JSON;
using basic_yaml::YST_CORE;

inline namespace literals {

inline yaml_t<> operator ""_yaml(const char* s,std::size_t n) {
	return yaml_t<>::parse(std::string_view(s,n));
}

}

}

}

#endif
