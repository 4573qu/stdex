//Last Modified At 2026/07/02
//@Version 1.0.0.0
#ifndef _STDEX_TYPE_DOM_HOCON_H_
#define _STDEX_TYPE_DOM_HOCON_H_ 1

#include <cerrno>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <functional>
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

namespace basic_hocon {

_STDEX_DERIVED_KIND(hocon_data_type,structure::dom_data_type,_STDEX_KIND_AUTO_START,
	_STDEX_KIND_VALUE_AUTO(HDT_SUBSTITUTION)
	_STDEX_KIND_VALUE_AUTO(HDT_CONCATENATION)
)

enum hocon_include_kind : int {
	HIK_HEURISTIC,
	HIK_FILE,
	HIK_URL,
	HIK_CLASSPATH,
};

enum hocon_symbol : int {
	HS_EPSILON,
	HS_EOF,
	HS_NEWLINE,
	HS_COMMA,
	HS_LBRACE,
	HS_RBRACE,
	HS_LBRACKET,
	HS_RBRACKET,
	HS_COLON,
	HS_EQ,
	HS_PLUSEQ,
	HS_DOT,
	HS_SUB_OPEN,
	HS_SUB_OPT_OPEN,
	HS_WS,
	HS_KEY,
	HS_SUBKEY,
	HS_QUOTED,
	HS_MULTILINE,
	HS_UNQUOTED,
	HS_INCLUDE,
	HS_START,
	HS_ROOT,
	HS_OBJECT,
	HS_OBJECT_BODY,
	HS_MEMBERS,
	HS_MEMBER,
	HS_SEP,
	HS_FIELD,
	HS_KEYPATH,
	HS_VALUE,
	HS_CONCAT,
	HS_PART,
	HS_SUBST,
	HS_SUBPATH,
	HS_ARRAY,
	HS_ELEMENTS,
};

enum hocon_production : int {
	HP_START,
	HP_ROOT_OBJECT,
	HP_ROOT_ARRAY,
	HP_ROOT_BODY,
	HP_OBJECT_EMPTY,
	HP_OBJECT,
	HP_OBJECT_BODY_PLAIN,
	HP_OBJECT_BODY_TRAILING,
	HP_MEMBERS_FIRST,
	HP_MEMBERS_APPEND,
	HP_SEP_COMMA,
	HP_SEP_NEWLINE,
	HP_MEMBER_FIELD,
	HP_MEMBER_INCLUDE,
	HP_FIELD_COLON,
	HP_FIELD_EQ,
	HP_FIELD_PLUSEQ,
	HP_KEYPATH_FIRST,
	HP_KEYPATH_APPEND,
	HP_VALUE,
	HP_CONCAT_FIRST,
	HP_CONCAT_APPEND,
	HP_CONCAT_APPEND_WS,
	HP_PART_QUOTED,
	HP_PART_MULTILINE,
	HP_PART_UNQUOTED,
	HP_PART_OBJECT,
	HP_PART_ARRAY,
	HP_PART_SUBST,
	HP_SUBST_PLAIN,
	HP_SUBST_OPTIONAL,
	HP_SUBPATH_FIRST,
	HP_SUBPATH_APPEND,
	HP_ARRAY_EMPTY,
	HP_ARRAY,
	HP_ARRAY_TRAILING,
	HP_ELEMENTS_FIRST,
	HP_ELEMENTS_APPEND,
};

template <typename _Hocon>
struct hocon_sax {
	using int_t=typename _Hocon::int_t;
	using float_t=typename _Hocon::float_t;
	using boolean_t=typename _Hocon::boolean_t;
	using string_t=typename _Hocon::string_t;
	using path_t=std::vector<string_t>;

	virtual bool start_object(std::size_t cnt)=0;
	virtual bool end_object()=0;
	virtual bool start_array(std::size_t cnt)=0;
	virtual bool end_array()=0;
	virtual bool key(path_t& path,int op)=0;
	virtual bool end_value()=0;
	virtual bool scalar(string_t& text,int style)=0;//style:0 quoted,1 multiline,2 unquoted。
	virtual bool substitution(path_t& path,bool optional)=0;
	virtual bool gap(string_t& whitespace)=0;
	virtual bool include(string_t& spec,int kind,bool required)=0;
	virtual bool parse_error(std::size_t position,const std::string& last_token,const std::string& message)=0;
	virtual ~hocon_sax()=default;
};

template <typename _String>
struct basic_hocon_location_seg {
	bool is_index=false;
	_String key{};
	std::size_t index=0;
};

template <typename _String>
struct basic_hocon_include_record {
	std::vector<basic_hocon_location_seg<_String>> location{};
	_String spec{};
	hocon_include_kind kind=HIK_HEURISTIC;
	bool required=false;
};

template <typename _Hocon>
class hocon_sax_dom_builder : public hocon_sax<_Hocon> {
public:
	using int_t=typename _Hocon::int_t;
	using float_t=typename _Hocon::float_t;
	using boolean_t=typename _Hocon::boolean_t;
	using string_t=typename _Hocon::string_t;
	using path_t=typename hocon_sax<_Hocon>::path_t;
	using dom_t=typename _Hocon::base_t;
	using part_t=typename _Hocon::concatenation_part;
	using location_t=std::vector<basic_hocon_location_seg<string_t>>;
	using include_record_t=basic_hocon_include_record<string_t>;

private:
	enum frame_kind : int {
		FK_OBJECT,
		FK_ARRAY,
		FK_VALUE,
	};
	struct frame_t {
		int kind=FK_OBJECT;
		bool root=false;
		bool has_pending=false;
		int pending_op=0;
		dom_t node{};
		std::vector<part_t> parts{};
		string_t pending_lead{};
		path_t pending_path{};
		location_t location{};
	};

	std::vector<frame_t> stack_;
	std::vector<include_record_t> includes_;
	bool errored_=false;
	std::size_t error_position_=0;
	std::string error_message_;

	static std::string narrow(const string_t& text) {
		return std::string(text.begin(),text.end());
	}
	bool fail(const std::string& message) {
		if (!errored_) {
			errored_=true;
			error_message_=message;
		}
		return false;
	}
	frame_t& push_value_frame() {
		frame_t& owner=stack_.back();
		frame_t frame;
		frame.kind=FK_VALUE;
		frame.location=owner.location;
		if (owner.kind==FK_OBJECT && owner.has_pending) {
			for (const auto& it:owner.pending_path) {
				basic_hocon_location_seg<string_t> seg;
				seg.key=it;
				frame.location.push_back(std::move(seg));
			}
		} else if (owner.kind==FK_ARRAY) {
			basic_hocon_location_seg<string_t> seg;
			seg.is_index=true;
			seg.index=owner.node.value().array->size();
			frame.location.push_back(std::move(seg));
		}
		stack_.push_back(std::move(frame));
		return stack_.back();
	}
	frame_t& value_frame() {
		if (stack_.back().kind!=FK_VALUE) return push_value_frame();
		return stack_.back();
	}
	void push_container(int kind) {
		frame_t& owner=value_frame();
		frame_t frame;
		frame.kind=kind;
		frame.node=dom_t(kind==FK_OBJECT?structure::DDT_OBJECT:structure::DDT_ARRAY);
		frame.location=owner.location;
		stack_.push_back(std::move(frame));
	}
	bool merge_field(frame_t& frame,dom_t&& value) {
		dom_t* node=&frame.node;
		const path_t& path=frame.pending_path;
		for (std::size_t i=0;i+1<path.size();i++) {
			auto& table=*node->value().object;
			auto it=table.find(path[i]);
			if (it==table.end()) {
				auto result=table.emplace(path[i],dom_t(structure::DDT_OBJECT));
				node=&result.first->second;
			} else {
				if (it->second.type()!=structure::DDT_OBJECT) it->second=dom_t(structure::DDT_OBJECT);
				node=&it->second;
			}
		}
		auto& table=*node->value().object;
		const string_t& key=path.back();
		auto it=table.find(key);
		if (frame.pending_op==2) {
			if (it!=table.end() && it->second.type()==structure::DDT_ARRAY) it->second.value().array->push_back(std::move(value));
			else {
				dom_t appended(structure::DDT_ARRAY);
				appended.value().array->push_back(std::move(value));
				if (it!=table.end()) it->second=std::move(appended);
				else table.emplace(key,std::move(appended));
			}
		} else if (it==table.end()) table.emplace(key,std::move(value));
		else if (it->second.type()==structure::DDT_OBJECT && value.type()==structure::DDT_OBJECT) _Hocon::deep_merge(it->second,value);
		else it->second=std::move(value);
		frame.has_pending=false;
		frame.pending_path.clear();
		frame.pending_op=0;
		return true;
	}

public:
	hocon_sax_dom_builder() {
		frame_t frame;
		frame.kind=FK_OBJECT;
		frame.root=true;
		frame.node=dom_t(structure::DDT_OBJECT);
		stack_.push_back(std::move(frame));
	}

	bool start_object(std::size_t cnt) override {
		static_cast<void>(cnt);
		if (errored_) return false;
		push_container(FK_OBJECT);
		return true;
	}
	bool end_object() override {
		if (errored_) return false;
		if (stack_.size()<2 || stack_.back().kind!=FK_OBJECT) return fail("Unbalanced object close");
		frame_t completed=std::move(stack_.back());
		stack_.pop_back();
		if (completed.has_pending) return fail("Field '"+narrow(completed.pending_path.back())+"' has no value");
		frame_t& owner=stack_.back();
		if (owner.kind!=FK_VALUE) return fail("Object completed outside of a value");
		part_t part;
		part.node=std::move(completed.node);
		part.lead=std::move(owner.pending_lead);
		owner.pending_lead.clear();
		owner.parts.push_back(std::move(part));
		return true;
	}
	bool start_array(std::size_t cnt) override {
		static_cast<void>(cnt);
		if (errored_) return false;
		push_container(FK_ARRAY);
		return true;
	}
	bool end_array() override {
		if (errored_) return false;
		if (stack_.size()<2 || stack_.back().kind!=FK_ARRAY) return fail("Unbalanced array close");
		frame_t completed=std::move(stack_.back());
		stack_.pop_back();
		frame_t& owner=stack_.back();
		if (owner.kind!=FK_VALUE) return fail("Array completed outside of a value");
		part_t part;
		part.node=std::move(completed.node);
		part.lead=std::move(owner.pending_lead);
		owner.pending_lead.clear();
		owner.parts.push_back(std::move(part));
		return true;
	}
	bool key(path_t& path,int op) override {
		if (errored_) return false;
		if (stack_.back().kind!=FK_OBJECT) return fail("Key outside of an object");
		frame_t& frame=stack_.back();
		if (frame.has_pending) return fail("Field '"+narrow(frame.pending_path.back())+"' has no value");
		frame.pending_path=path;
		frame.pending_op=op;
		frame.has_pending=true;
		return true;
	}
	bool end_value() override {
		if (errored_) return false;
		if (stack_.back().kind!=FK_VALUE) return fail("Unbalanced value close");
		frame_t completed=std::move(stack_.back());
		stack_.pop_back();
		if (completed.parts.empty()) return fail("Empty value");
		dom_t value;
		if (completed.parts.size()==1 && !completed.parts.front().unquoted) value=std::move(completed.parts.front().node);
		else value=_Hocon::make_concatenation(std::move(completed.parts));
		frame_t& owner=stack_.back();
		if (owner.kind==FK_ARRAY) {
			owner.node.value().array->push_back(std::move(value));
			return true;
		}
		if (owner.kind==FK_OBJECT) {
			if (owner.has_pending) return merge_field(owner,std::move(value));
			if (owner.root) {
				owner.node=std::move(value);
				return true;
			}
		}
		return fail("Value without a key");
	}
	bool scalar(string_t& text,int style) override {
		if (errored_) return false;
		frame_t& frame=value_frame();
		part_t part;
		part.node=dom_t(string_t(text));
		part.lead=std::move(frame.pending_lead);
		frame.pending_lead.clear();
		part.unquoted=(style==2);
		frame.parts.push_back(std::move(part));
		return true;
	}
	bool substitution(path_t& path,bool optional) override {
		if (errored_) return false;
		frame_t& frame=value_frame();
		part_t part;
		part.node=_Hocon::make_substitution(path,optional);
		part.lead=std::move(frame.pending_lead);
		frame.pending_lead.clear();
		frame.parts.push_back(std::move(part));
		return true;
	}
	bool gap(string_t& whitespace) override {
		if (errored_) return false;
		if (stack_.back().kind!=FK_VALUE) return fail("Whitespace joint outside of a value");
		stack_.back().pending_lead=whitespace;
		return true;
	}
	bool include(string_t& spec,int kind,bool required) override {
		if (errored_) return false;
		if (stack_.back().kind!=FK_OBJECT) return fail("Include outside of an object");
		include_record_t record;
		record.location=stack_.back().location;
		record.spec=spec;
		record.kind=static_cast<hocon_include_kind>(kind);
		record.required=required;
		includes_.push_back(std::move(record));
		return true;
	}
	bool parse_error(std::size_t position,const std::string& last_token,const std::string& message) override {
		if (!errored_) {
			errored_=true;
			error_message_=message+(last_token.empty()?std::string():(" near '"+last_token+"'"));
		}
		if (!error_position_) error_position_=position;
		return false;
	}

	bool errored() const noexcept {
		return errored_;
	}
	bool completed() const noexcept {
		return !errored_ && stack_.size()==1 && !stack_.front().has_pending;
	}
	dom_t& root() noexcept {
		return stack_.front().node;
	}
	const dom_t& root() const noexcept {
		return stack_.front().node;
	}
	std::vector<include_record_t>& includes() noexcept {
		return includes_;
	}
	const std::vector<include_record_t>& includes() const noexcept {
		return includes_;
	}
	std::size_t error_position() const noexcept {
		return error_position_;
	}
	const std::string& error_message() const noexcept {
		return error_message_;
	}
};

template <typename _Hocon>
class hocon_sax_acceptor : public hocon_sax<_Hocon> {
public:
	using int_t=typename _Hocon::int_t;
	using float_t=typename _Hocon::float_t;
	using boolean_t=typename _Hocon::boolean_t;
	using string_t=typename _Hocon::string_t;
	using path_t=typename hocon_sax<_Hocon>::path_t;

	bool start_object(std::size_t cnt) override {
		static_cast<void>(cnt);
		return true;
	}
	bool end_object() override {
		return true;
	}
	bool start_array(std::size_t cnt) override {
		static_cast<void>(cnt);
		return true;
	}
	bool end_array() override {
		return true;
	}
	bool key(path_t& path,int op) override {
		static_cast<void>(path);
		static_cast<void>(op);
		return true;
	}
	bool end_value() override {
		return true;
	}
	bool scalar(string_t& text,int style) override {
		static_cast<void>(text);
		static_cast<void>(style);
		return true;
	}
	bool substitution(path_t& path,bool optional) override {
		static_cast<void>(path);
		static_cast<void>(optional);
		return true;
	}
	bool gap(string_t& whitespace) override {
		static_cast<void>(whitespace);
		return true;
	}
	bool include(string_t& spec,int kind,bool required) override {
		static_cast<void>(spec);
		static_cast<void>(kind);
		static_cast<void>(required);
		return true;
	}
	bool parse_error(std::size_t position,const std::string& last_token,const std::string& message) override {
		static_cast<void>(position);
		static_cast<void>(last_token);
		static_cast<void>(message);
		return false;
	}
};

_STDEX_DOM_TPL_DECLARATION
class hocon : public structure::_STDEX_DOM_DEF {
public:
	using base_t=structure::_STDEX_DOM_DEF;
	using int_t=typename base_t::int_t;
	using float_t=typename base_t::float_t;
	using boolean_t=typename base_t::boolean_t;
	using string_t=typename base_t::string_t;
	using array_t=typename base_t::array_t;
	using object_t=typename base_t::object_t;
	using size_type=typename base_t::size_type;
	using sax_t=hocon_sax<hocon>;
	using path_t=typename sax_t::path_t;
	using location_t=std::vector<basic_hocon_location_seg<string_t>>;
	using include_record_t=basic_hocon_include_record<string_t>;

	struct concatenation_part {
		base_t node{};
		string_t lead{};
		bool unquoted=false;
	};

	using include_handler_t=std::function<bool(const string_t&,hocon_include_kind,bool,hocon&)>;

	static_assert(sizeof(typename string_t::value_type)==1,"hocon serializer assumes a byte-oriented (UTF-8) string_t.");

protected:
	struct substitution_value : base_t::value_t {
		path_t path{};
		bool optional=false;

		substitution_value()=default;
		substitution_value(path_t&& target,bool optional_reference) : path(std::move(target)) , optional(optional_reference) { }
		substitution_value(const path_t& target,bool optional_reference) : path(target) , optional(optional_reference) { }
		~substitution_value() override=default;

		substitution_value(const substitution_value& other) : base_t::value_t() , path(other.path) , optional(other.optional) { }

		typename base_t::value_t* clone(structure::dom_data_type t) const override {
			if (t==HDT_SUBSTITUTION) return create_value<substitution_value>(*this);
			return base_t::value_t::clone(t);
		}
		void destroy(structure::dom_data_type t) override {
			if (t==HDT_SUBSTITUTION) return;
			base_t::value_t::destroy(t);
		}
		void destroy_self(structure::dom_data_type t) override {
			this->destroy(t);
			_Allocator<substitution_value> alloc;
			std::allocator_traits<_Allocator<substitution_value>>::destroy(alloc,this);
			std::allocator_traits<_Allocator<substitution_value>>::deallocate(alloc,this,1);
		}
	};
	struct concatenation_value : base_t::value_t {
		std::vector<concatenation_part> parts{};

		concatenation_value()=default;
		explicit concatenation_value(std::vector<concatenation_part>&& sequence) : parts(std::move(sequence)) { }
		explicit concatenation_value(const std::vector<concatenation_part>& sequence) : parts(sequence) { }
		~concatenation_value() override=default;

		concatenation_value(const concatenation_value& other) : base_t::value_t() , parts(other.parts) { }

		typename base_t::value_t* clone(structure::dom_data_type t) const override {
			if (t==HDT_CONCATENATION) return create_value<concatenation_value>(*this);
			return base_t::value_t::clone(t);
		}
		void destroy(structure::dom_data_type t) override {
			if (t==HDT_CONCATENATION) return;
			base_t::value_t::destroy(t);
		}
		void destroy_self(structure::dom_data_type t) override {
			this->destroy(t);
			_Allocator<concatenation_value> alloc;
			std::allocator_traits<_Allocator<concatenation_value>>::destroy(alloc,this);
			std::allocator_traits<_Allocator<concatenation_value>>::deallocate(alloc,this,1);
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
	static substitution_value* substitution_payload(const base_t& node) {
		substitution_value* payload=node.data().value?dynamic_cast<substitution_value*>(node.data().value):nullptr;
		if (node.type()!=hocon_data_type(HDT_SUBSTITUTION) || !payload) throw std::invalid_argument("Node does not hold a hocon substitution payload");
		return payload;
	}
	static concatenation_value* concatenation_payload(const base_t& node) {
		concatenation_value* payload=node.data().value?dynamic_cast<concatenation_value*>(node.data().value):nullptr;
		if (node.type()!=hocon_data_type(HDT_CONCATENATION) || !payload) throw std::invalid_argument("Node does not hold a hocon concatenation payload");
		return payload;
	}
	static string_t make_string(const char* text) {
		return string_t(text,text+std::strlen(text));
	}

public:
	using base_t::base_t;
	using base_t::operator =;

	hocon()=default;
	hocon(const hocon&)=default;
	hocon(hocon&&)=default;
	hocon& operator =(const hocon&)=default;
	hocon& operator =(hocon&&)=default;
	hocon(const base_t& other) : base_t(other) { }
	hocon(base_t&& other) : base_t(std::move(other)) { }

	bool support(structure::dom_data_type t) const noexcept override {
		return base_t::support(t) || t==hocon_data_type(HDT_SUBSTITUTION) || t==hocon_data_type(HDT_CONCATENATION);
	}

	static base_t make_substitution(path_t path,bool optional=false) {
		base_t node;
		node.data()=typename base_t::data_t(hocon_data_type(HDT_SUBSTITUTION),create_value<substitution_value>(std::move(path),optional));
		return node;
	}
	static base_t make_concatenation(std::vector<concatenation_part> parts) {
		base_t node;
		node.data()=typename base_t::data_t(hocon_data_type(HDT_CONCATENATION),create_value<concatenation_value>(std::move(parts)));
		return node;
	}
	static bool is_substitution(const base_t& node) noexcept {
		return node.type()==hocon_data_type(HDT_SUBSTITUTION);
	}
	bool is_substitution() const noexcept {
		return hocon::is_substitution(*this);
	}
	static bool is_concatenation(const base_t& node) noexcept {
		return node.type()==hocon_data_type(HDT_CONCATENATION);
	}
	bool is_concatenation() const noexcept {
		return hocon::is_concatenation(*this);
	}
	static bool is_resolved(const base_t& node) {
		switch (node.type()) {
			case structure::DDT_OBJECT:
			case structure::DDT_ARRAY: {
				for (auto it=node.cbegin();it!=node.cend();it++) {
					if (!is_resolved(*it)) return false;
				}
				return true;
			}
			default: return !is_substitution(node) && !is_concatenation(node);
		}
	}
	bool is_resolved() const {
		return hocon::is_resolved(*this);
	}
	static path_t& substitution_path(const base_t& node) {
		return substitution_payload(node)->path;
	}
	static bool substitution_optional(const base_t& node) {
		return substitution_payload(node)->optional;
	}
	static std::vector<concatenation_part>& concatenation_parts(const base_t& node) {
		return concatenation_payload(node)->parts;
	}

	static void deep_merge(base_t& target,base_t& source) {
		auto& table=*target.value().object;
		for (auto&& it:*source.value().object) {
			auto slot=table.find(it.first);
			if (slot==table.end()) table.emplace(it.first,std::move(it.second));
			else if (slot->second.type()==structure::DDT_OBJECT && it.second.type()==structure::DDT_OBJECT) deep_merge(slot->second,it.second);
			else slot->second=std::move(it.second);
		}
	}

protected:
	//记法转换协议·源侧降级:HDT_SUBSTITUTION降级为其原文形式"${path}"/"${?path}"字符串;
	//HDT_CONCATENATION若全为标量片段则降级为拼接串,否则降级为片段数组(片段内的悬置节点交由转换协议继续处理)。
	bool degrade_unsupported(const base_t& source,base_t& replacement) const override {
		if (source.type()==hocon_data_type(HDT_SUBSTITUTION)) {
			substitution_value* payload=substitution_payload(source);
			string_t text=make_string(payload->optional?"${?":"${");
			for (std::size_t i=0;i<payload->path.size();i++) {
				if (i) text.push_back('.');
				text.append(payload->path[i]);
			}
			text.push_back('}');
			replacement=base_t(std::move(text));
			return true;
		}
		if (source.type()==hocon_data_type(HDT_CONCATENATION)) {
			std::vector<concatenation_part>& parts=concatenation_parts(source);
			bool scalar_only=true;
			for (const auto& it:parts) {
				if (it.node.type()==structure::DDT_OBJECT || it.node.type()==structure::DDT_ARRAY || is_substitution(it.node) || is_concatenation(it.node)) {
					scalar_only=false;
					break;
				}
			}
			if (scalar_only) {
				string_t text;
				for (std::size_t i=0;i<parts.size();i++) {
					if (i) text.append(parts[i].lead);
					stringify_scalar(text,parts[i].node);
				}
				replacement=base_t(std::move(text));
			} else {
				base_t sequence(structure::DDT_ARRAY);
				for (const auto& it:parts) sequence.value().array->push_back(it.node);
				replacement=std::move(sequence);
			}
			return true;
		}
		return false;
	}

private:
	struct hocon_token {
		hocon_symbol symbol=HS_EPSILON;
		std::size_t position=0;
		string_t text{};
		int detail=0;
		bool required=false;
	};

	static const std::regex& integer_regex() {
		static const std::regex result(R"(-?[0-9]+)",std::regex::optimize);
		return result;
	}
	static const std::regex& float_regex() {
		static const std::regex result(R"(-?(?:[0-9]+(?:\.[0-9]*)?|\.[0-9]+)(?:[eE][+-]?[0-9]+)?)",std::regex::optimize);
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
	static bool hex4(const char* p,const char* last,unsigned long& out) {
		if (last-p<4) return false;
		out=0;
		for (int i=0;i<4;i++) {
			const char c=p[i];
			out<<=4;
			if (c>='0' && c<='9') out|=static_cast<unsigned long>(c-'0');
			else if (c>='a' && c<='f') out|=static_cast<unsigned long>(c-'a'+10);
			else if (c>='A' && c<='F') out|=static_cast<unsigned long>(c-'A'+10);
			else return false;
		}
		return true;
	}

	struct tokenizer {
		const char* base;
		const char* first;
		const char* last;
		std::vector<hocon_token>& tokens;
		struct context_t {
			char kind='R';
			bool concat_open=false;
		};
		std::vector<context_t> ctx_{};
		bool value_mode=false;
		std::size_t error_position=0;
		std::string error_message;

		tokenizer(std::string_view input,std::vector<hocon_token>& target) : base(input.data()) , first(input.data()) , last(input.data()+input.size()) , tokens(target) {
			ctx_.push_back(context_t{});
		}

		bool fail(const char* p,const std::string& message) {
			error_position=static_cast<std::size_t>(p-base);
			error_message=message;
			return false;
		}
		void emit(hocon_symbol symbol,const char* p) {
			hocon_token token;
			token.symbol=symbol;
			token.position=static_cast<std::size_t>(p-base);
			tokens.push_back(std::move(token));
		}
		void emit_text(hocon_symbol symbol,const char* p,string_t text) {
			hocon_token token;
			token.symbol=symbol;
			token.position=static_cast<std::size_t>(p-base);
			token.text=std::move(text);
			tokens.push_back(std::move(token));
		}
		void emit_separator(hocon_symbol symbol,const char* p) {
			if (symbol==HS_NEWLINE) {
				if (tokens.empty()) return;
				const hocon_symbol previous=tokens.back().symbol;
				if (previous==HS_NEWLINE || previous==HS_COMMA || previous==HS_LBRACE || previous==HS_LBRACKET) return;
				emit(HS_NEWLINE,p);
				return;
			}
			if (!tokens.empty() && tokens.back().symbol==HS_NEWLINE) {
				tokens.back().symbol=HS_COMMA;
				tokens.back().position=static_cast<std::size_t>(p-base);
				return;
			}
			emit(HS_COMMA,p);
		}
		static bool is_inline_space(char c) noexcept {
			return c==' ' || c=='\t' || c=='\r' || c=='\f' || c=='\v';
		}
		static bool unquoted_forbidden(char c) noexcept {
			switch (c) {
				case '$':
				case '"':
				case '{':
				case '}':
				case '[':
				case ']':
				case ':':
				case '=':
				case ',':
				case '+':
				case '#':
				case '`':
				case '^':
				case '?':
				case '!':
				case '@':
				case '*':
				case '&':
				case '\\': return true;
				default: return false;
			}
		}
		bool comment_start(const char* p) const noexcept {
			return p<last && (*p=='#' || (*p=='/' && p+1<last && p[1]=='/'));
		}
		void skip_comment() {
			while (first<last && *first!='\n') first++;
		}
		void skip_inline() {
			while (first<last && is_inline_space(*first)) first++;
		}
		void close_container() {
			if (ctx_.back().kind=='R') value_mode=false;
			else value_mode=true;
		}

		bool tokenize() {
			while (first<last) {
				if (value_mode) {
					if (!scan_value()) return false;
				} else if (!scan_member()) return false;
			}
			emit(HS_EOF,last);
			return true;
		}

		bool scan_member() {
			skip_inline();
			if (first>=last) return true;
			const char c=*first;
			if (comment_start(first)) {
				skip_comment();
				return true;
			}
			if (c=='\n') {
				emit_separator(HS_NEWLINE,first);
				first++;
				return true;
			}
			if (c=='}') {
				if (ctx_.back().kind!='O') return fail(first,"Unexpected '}'");
				emit(HS_RBRACE,first);
				first++;
				ctx_.pop_back();
				close_container();
				return true;
			}
			if (c==',') {
				emit_separator(HS_COMMA,first);
				first++;
				return true;
			}
			if (c=='{') {
				if (ctx_.back().kind!='R') return fail(first,"Expected a key");
				emit(HS_LBRACE,first);
				first++;
				ctx_.push_back(context_t{'O',false});
				return true;
			}
			if (c=='[') {
				if (ctx_.back().kind!='R') return fail(first,"Expected a key");
				emit(HS_LBRACKET,first);
				first++;
				ctx_.push_back(context_t{'A',false});
				value_mode=true;
				return true;
			}
			if (c==']') return fail(first,"Unexpected ']'");
			if (c==':' || c=='=' || c=='+' || c=='.') return fail(first,"Expected a key");
			if (looks_like_include()) return scan_include();
			return scan_keypath();
		}

		bool looks_like_include() const {
			if (last-first<8 || std::memcmp(first,"include",7)!=0) return false;
			const char* p=first+7;
			if (!is_inline_space(*p)) return false;
			while (p<last && is_inline_space(*p)) p++;
			if (p>=last) return false;
			if (*p=='"') return true;
			const std::size_t remain=static_cast<std::size_t>(last-p);
			auto opener=[&](const char* keyword,std::size_t length) {
				return remain>length && std::memcmp(p,keyword,length)==0 && p[length]=='(';
			};
			return opener("file",4) || opener("url",3) || opener("classpath",9) || opener("required",8);
		}
		bool scan_include() {
			const char* start=first;
			first+=7;
			skip_inline();
			bool required=false;
			int kind=HIK_HEURISTIC;
			if (last-first>8 && std::memcmp(first,"required",8)==0 && first[8]=='(') {
				required=true;
				first+=9;
				skip_inline();
			}
			if (last-first>4 && std::memcmp(first,"file",4)==0 && first[4]=='(') {
				kind=HIK_FILE;
				first+=5;
			} else if (last-first>3 && std::memcmp(first,"url",3)==0 && first[3]=='(') {
				kind=HIK_URL;
				first+=4;
			} else if (last-first>9 && std::memcmp(first,"classpath",9)==0 && first[9]=='(') {
				kind=HIK_CLASSPATH;
				first+=10;
			}
			skip_inline();
			if (first>=last || *first!='"') return fail(first,"Expected a quoted resource name in the include directive");
			if (first+2<last && first[1]=='"' && first[2]=='"') return fail(first,"Include resource names must use a basic quoted string");
			string_t spec;
			if (!scan_quoted(spec)) return false;
			if (kind!=HIK_HEURISTIC) {
				skip_inline();
				if (first>=last || *first!=')') return fail(first,"Expected ')' to close the include resource wrapper");
				first++;
			}
			if (required) {
				skip_inline();
				if (first>=last || *first!=')') return fail(first,"Expected ')' to close required(...)");
				first++;
			}
			hocon_token token;
			token.symbol=HS_INCLUDE;
			token.position=static_cast<std::size_t>(start-base);
			token.text=std::move(spec);
			token.detail=kind;
			token.required=required;
			tokens.push_back(std::move(token));
			return true;
		}

		bool scan_keypath() {
			for (;;) {
				skip_inline();
				if (first>=last) return fail(first,"Expected ':', '=', '+=' or '{' after the key");
				if (*first=='"') {
					const char* start=first;
					string_t keypart;
					if (!scan_quoted(keypart)) return false;
					emit_text(HS_KEY,start,std::move(keypart));
				} else {
					const char* start=first;
					const char* stop=first;
					while (first<last) {
						const char c=*first;
						if (c=='\n' || c=='.') break;
						if (c==':' || c=='=' || c=='{' || c=='}' || c==',' || c=='+' || c=='#') break;
						if (c=='/' && first+1<last && first[1]=='/') break;
						if (unquoted_forbidden(c)) return fail(first,"Character '"+std::string(1,c)+"' is not allowed in an unquoted key");
						first++;
						if (!is_inline_space(c)) stop=first;
					}
					if (stop==start) return fail(start,"Expected a key");
					emit_text(HS_KEY,start,string_t(start,stop));
				}
				skip_inline();
				if (first>=last) return fail(first,"Expected ':', '=', '+=' or '{' after the key");
				const char c=*first;
				if (c=='.') {
					emit(HS_DOT,first);
					first++;
					continue;
				}
				if (c==':') {
					emit(HS_COLON,first);
					first++;
					value_mode=true;
					return true;
				}
				if (c=='=') {
					emit(HS_EQ,first);
					first++;
					value_mode=true;
					return true;
				}
				if (c=='+') {
					if (first+1<last && first[1]=='=') {
						emit(HS_PLUSEQ,first);
						first+=2;
						value_mode=true;
						return true;
					}
					return fail(first,"Expected '+=' after the key");
				}
				if (c=='{') {
					emit(HS_COLON,first);
					value_mode=true;
					return true;
				}
				return fail(first,"Expected ':', '=', '+=' or '{' after the key");
			}
		}

		bool scan_value() {
			context_t& ctx=ctx_.back();
			const char* ws_start=first;
			skip_inline();
			string_t lead(ws_start,first);
			if (first>=last) {
				ctx.concat_open=false;
				return true;
			}
			const char c=*first;
			if (comment_start(first)) {
				ctx.concat_open=false;
				skip_comment();
				return true;
			}
			if (c=='\n') {
				ctx.concat_open=false;
				emit_separator(HS_NEWLINE,first);
				first++;
				if (ctx.kind!='A') value_mode=false;
				return true;
			}
			if (c==',') {
				ctx.concat_open=false;
				emit_separator(HS_COMMA,first);
				first++;
				if (ctx.kind!='A') value_mode=false;
				return true;
			}
			if (c=='}') {
				if (ctx.kind!='O') return fail(first,"Unexpected '}'");
				emit(HS_RBRACE,first);
				first++;
				ctx_.pop_back();
				close_container();
				return true;
			}
			if (c==']') {
				if (ctx.kind!='A') return fail(first,"Unexpected ']'");
				emit(HS_RBRACKET,first);
				first++;
				ctx_.pop_back();
				close_container();
				return true;
			}
			if (ctx.concat_open && !lead.empty()) emit_text(HS_WS,ws_start,std::move(lead));
			if (c=='"') {
				if (first+2<last && first[1]=='"' && first[2]=='"') {
					const char* start=first;
					string_t text;
					if (!scan_multiline(text)) return false;
					emit_text(HS_MULTILINE,start,std::move(text));
				} else {
					const char* start=first;
					string_t text;
					if (!scan_quoted(text)) return false;
					emit_text(HS_QUOTED,start,std::move(text));
				}
				ctx.concat_open=true;
				return true;
			}
			if (c=='$' && first+1<last && first[1]=='{') {
				if (!scan_substitution()) return false;
				ctx.concat_open=true;
				return true;
			}
			if (c=='{') {
				emit(HS_LBRACE,first);
				first++;
				ctx.concat_open=true;
				ctx_.push_back(context_t{'O',false});
				value_mode=false;
				return true;
			}
			if (c=='[') {
				emit(HS_LBRACKET,first);
				first++;
				ctx.concat_open=true;
				ctx_.push_back(context_t{'A',false});
				return true;
			}
			if (unquoted_forbidden(c)) return fail(first,"Character '"+std::string(1,c)+"' is not allowed in an unquoted value");
			const char* start=first;
			const char* stop=first;
			while (first<last) {
				const char ch=*first;
				if (ch=='\n' || is_inline_space(ch)) break;
				if (ch=='#' || (ch=='/' && first+1<last && first[1]=='/')) break;
				if (ch=='$' || ch=='"' || ch=='{' || ch=='[' || ch=='}' || ch==']' || ch==',') break;
				if (unquoted_forbidden(ch)) return fail(first,"Character '"+std::string(1,ch)+"' is not allowed in an unquoted value");
				first++;
				stop=first;
			}
			emit_text(HS_UNQUOTED,start,string_t(start,stop));
			ctx.concat_open=true;
			return true;
		}

		bool scan_substitution() {
			const char* start=first;
			if (first+2<last && first[2]=='?') {
				emit(HS_SUB_OPT_OPEN,start);
				first+=3;
			} else {
				emit(HS_SUB_OPEN,start);
				first+=2;
			}
			for (;;) {
				skip_inline();
				if (first>=last) return fail(first,"Unterminated substitution");
				if (*first=='"') {
					const char* key_start=first;
					string_t keypart;
					if (!scan_quoted(keypart)) return false;
					emit_text(HS_SUBKEY,key_start,std::move(keypart));
				} else {
					const char* key_start=first;
					const char* stop=first;
					while (first<last) {
						const char c=*first;
						if (c=='\n') return fail(first,"Unterminated substitution");
						if (c=='.' || c=='}') break;
						if (c=='/' && first+1<last && first[1]=='/') return fail(first,"Comments are not allowed inside a substitution");
						if (unquoted_forbidden(c)) return fail(first,"Character '"+std::string(1,c)+"' is not allowed in a substitution path");
						first++;
						if (!is_inline_space(c)) stop=first;
					}
					if (stop==key_start) return fail(key_start,"Expected a path inside the substitution");
					emit_text(HS_SUBKEY,key_start,string_t(key_start,stop));
				}
				skip_inline();
				if (first>=last) return fail(first,"Unterminated substitution");
				if (*first=='.') {
					emit(HS_DOT,first);
					first++;
					continue;
				}
				if (*first=='}') {
					emit(HS_RBRACE,first);
					first++;
					return true;
				}
				return fail(first,"Expected '.' or '}' in the substitution");
			}
		}

		bool scan_quoted(string_t& out) {
			const char* start=first;
			first++;
			while (first<last) {
				const char c=*first;
				if (c=='"') {
					first++;
					return true;
				}
				if (c=='\n') return fail(first,"Unterminated string");
				if (static_cast<unsigned char>(c)<0x20) return fail(first,"Control characters must be escaped inside strings");
				if (c!='\\') {
					out.push_back(static_cast<typename string_t::value_type>(c));
					first++;
					continue;
				}
				first++;
				if (first>=last) return fail(first,"Unterminated escape sequence");
				const char escape=*first;
				first++;
				switch (escape) {
					case '"': out.push_back('"');break;
					case '\\': out.push_back('\\');break;
					case '/': out.push_back('/');break;
					case 'b': out.push_back('\b');break;
					case 'f': out.push_back('\f');break;
					case 'n': out.push_back('\n');break;
					case 'r': out.push_back('\r');break;
					case 't': out.push_back('\t');break;
					case 'u': {
						unsigned long cp=0;
						if (!hex4(first,last,cp)) return fail(first,"Expected four hexadecimal digits after \\u");
						first+=4;
						if (cp>=0xD800 && cp<=0xDBFF) {
							if (last-first<6 || first[0]!='\\' || first[1]!='u') return fail(first,"Expected a low surrogate after a high surrogate");
							unsigned long low=0;
							if (!hex4(first+2,last,low)) return fail(first,"Expected four hexadecimal digits after \\u");
							if (low<0xDC00 || low>0xDFFF) return fail(first,"Invalid low surrogate");
							first+=6;
							cp=0x10000+((cp-0xD800)<<10)+(low-0xDC00);
						} else if (cp>=0xDC00 && cp<=0xDFFF) return fail(first,"Unexpected low surrogate");
						append_codepoint(out,cp);
						break;
					}
					default: return fail(first-1,"Invalid escape character");
				}
			}
			return fail(start,"Unterminated string");
		}

		bool scan_multiline(string_t& out) {
			const char* start=first;
			first+=3;
			const char* p=first;
			while (p<last) {
				if (*p!='"') {
					p++;
					continue;
				}
				const char* run=p;
				while (run<last && *run=='"') run++;
				if (run-p>=3) {
					out=string_t(first,run-3);
					first=run;
					return true;
				}
				p=run;
			}
			return fail(start,"Unterminated multiline string");
		}
	};

	using parser_t=syntax::parser<hocon_symbol,hocon_production>;

	static bool initialize_grammar(parser_t& target) {
		auto unit=[](hocon_symbol left,std::initializer_list<hocon_symbol> rights,hocon_production id){
			return syntax::single_parser_unit<hocon_symbol,hocon_production>(left,rights,id);
		};
		target.units={
			unit(HS_START,{HS_ROOT,HS_EOF},HP_START),
			unit(HS_ROOT,{HS_OBJECT},HP_ROOT_OBJECT),
			unit(HS_ROOT,{HS_ARRAY},HP_ROOT_ARRAY),
			unit(HS_ROOT,{HS_OBJECT_BODY},HP_ROOT_BODY),
			unit(HS_OBJECT,{HS_LBRACE,HS_RBRACE},HP_OBJECT_EMPTY),
			unit(HS_OBJECT,{HS_LBRACE,HS_OBJECT_BODY,HS_RBRACE},HP_OBJECT),
			unit(HS_OBJECT_BODY,{HS_MEMBERS},HP_OBJECT_BODY_PLAIN),
			unit(HS_OBJECT_BODY,{HS_MEMBERS,HS_SEP},HP_OBJECT_BODY_TRAILING),
			unit(HS_MEMBERS,{HS_MEMBER},HP_MEMBERS_FIRST),
			unit(HS_MEMBERS,{HS_MEMBERS,HS_SEP,HS_MEMBER},HP_MEMBERS_APPEND),
			unit(HS_SEP,{HS_COMMA},HP_SEP_COMMA),
			unit(HS_SEP,{HS_NEWLINE},HP_SEP_NEWLINE),
			unit(HS_MEMBER,{HS_FIELD},HP_MEMBER_FIELD),
			unit(HS_MEMBER,{HS_INCLUDE},HP_MEMBER_INCLUDE),
			unit(HS_FIELD,{HS_KEYPATH,HS_COLON,HS_VALUE},HP_FIELD_COLON),
			unit(HS_FIELD,{HS_KEYPATH,HS_EQ,HS_VALUE},HP_FIELD_EQ),
			unit(HS_FIELD,{HS_KEYPATH,HS_PLUSEQ,HS_VALUE},HP_FIELD_PLUSEQ),
			unit(HS_KEYPATH,{HS_KEY},HP_KEYPATH_FIRST),
			unit(HS_KEYPATH,{HS_KEYPATH,HS_DOT,HS_KEY},HP_KEYPATH_APPEND),
			unit(HS_VALUE,{HS_CONCAT},HP_VALUE),
			unit(HS_CONCAT,{HS_PART},HP_CONCAT_FIRST),
			unit(HS_CONCAT,{HS_CONCAT,HS_PART},HP_CONCAT_APPEND),
			unit(HS_CONCAT,{HS_CONCAT,HS_WS,HS_PART},HP_CONCAT_APPEND_WS),
			unit(HS_PART,{HS_QUOTED},HP_PART_QUOTED),
			unit(HS_PART,{HS_MULTILINE},HP_PART_MULTILINE),
			unit(HS_PART,{HS_UNQUOTED},HP_PART_UNQUOTED),
			unit(HS_PART,{HS_OBJECT},HP_PART_OBJECT),
			unit(HS_PART,{HS_ARRAY},HP_PART_ARRAY),
			unit(HS_PART,{HS_SUBST},HP_PART_SUBST),
			unit(HS_SUBST,{HS_SUB_OPEN,HS_SUBPATH,HS_RBRACE},HP_SUBST_PLAIN),
			unit(HS_SUBST,{HS_SUB_OPT_OPEN,HS_SUBPATH,HS_RBRACE},HP_SUBST_OPTIONAL),
			unit(HS_SUBPATH,{HS_SUBKEY},HP_SUBPATH_FIRST),
			unit(HS_SUBPATH,{HS_SUBPATH,HS_DOT,HS_SUBKEY},HP_SUBPATH_APPEND),
			unit(HS_ARRAY,{HS_LBRACKET,HS_RBRACKET},HP_ARRAY_EMPTY),
			unit(HS_ARRAY,{HS_LBRACKET,HS_ELEMENTS,HS_RBRACKET},HP_ARRAY),
			unit(HS_ARRAY,{HS_LBRACKET,HS_ELEMENTS,HS_SEP,HS_RBRACKET},HP_ARRAY_TRAILING),
			unit(HS_ELEMENTS,{HS_VALUE},HP_ELEMENTS_FIRST),
			unit(HS_ELEMENTS,{HS_ELEMENTS,HS_SEP,HS_VALUE},HP_ELEMENTS_APPEND),
		};
		target.generate_parser();
		return true;
	}
	static parser_t& grammar() {
		static parser_t instance(HS_START,HS_EPSILON,HS_EOF);
		static const bool initialized=initialize_grammar(instance);
		static_cast<void>(initialized);
		return instance;
	}
	static std::mutex& grammar_mutex() {
		static std::mutex instance;
		return instance;
	}

	class hocon_listener : public syntax::parser_listener<hocon_symbol,hocon_production> {
		std::vector<hocon_token>* tokens_=nullptr;
		sax_t* sax_=nullptr;
		typename sax_t::path_t key_path_;
		typename sax_t::path_t subst_path_;
		bool aborted_=false;
		bool failed_=false;

		void abort_check(bool keep_going) {
			if (!keep_going) aborted_=true;
		}

	public:
		void reset(std::vector<hocon_token>& tokens,sax_t& sax) {
			tokens_=&tokens;
			sax_=&sax;
			key_path_.clear();
			subst_path_.clear();
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
		intptr_t on_shift(uintptr_t id,int state,hocon_symbol word) override {
			static_cast<void>(state);
			if (aborted_ || failed_) return 0;
			hocon_token& token=(*tokens_)[id-1];
			switch (word) {
				case HS_LBRACE: abort_check(sax_->start_object(static_cast<std::size_t>(-1)));break;
				case HS_LBRACKET: abort_check(sax_->start_array(static_cast<std::size_t>(-1)));break;
				case HS_KEY: key_path_.push_back(token.text);break;
				case HS_SUBKEY: subst_path_.push_back(token.text);break;
				case HS_COLON: {
					abort_check(sax_->key(key_path_,0));
					key_path_.clear();
					break;
				}
				case HS_EQ: {
					abort_check(sax_->key(key_path_,1));
					key_path_.clear();
					break;
				}
				case HS_PLUSEQ: {
					abort_check(sax_->key(key_path_,2));
					key_path_.clear();
					break;
				}
				case HS_WS: abort_check(sax_->gap(token.text));break;
				case HS_INCLUDE: abort_check(sax_->include(token.text,token.detail,token.required));break;
				default: break;
			}
			return 0;
		}
		intptr_t on_reduction(uintptr_t id,int state,int next,hocon_production sentence_id,int reduction_num) override {
			static_cast<void>(state);
			static_cast<void>(next);
			static_cast<void>(reduction_num);
			if (aborted_ || failed_) return 0;
			hocon_token& token=(*tokens_)[id-2];
			switch (sentence_id) {
				case HP_PART_QUOTED: abort_check(sax_->scalar(token.text,0));break;
				case HP_PART_MULTILINE: abort_check(sax_->scalar(token.text,1));break;
				case HP_PART_UNQUOTED: abort_check(sax_->scalar(token.text,2));break;
				case HP_SUBST_PLAIN: {
					abort_check(sax_->substitution(subst_path_,false));
					subst_path_.clear();
					break;
				}
				case HP_SUBST_OPTIONAL: {
					abort_check(sax_->substitution(subst_path_,true));
					subst_path_.clear();
					break;
				}
				case HP_OBJECT_EMPTY:
				case HP_OBJECT: abort_check(sax_->end_object());break;
				case HP_ARRAY_EMPTY:
				case HP_ARRAY:
				case HP_ARRAY_TRAILING: abort_check(sax_->end_array());break;
				case HP_VALUE: abort_check(sax_->end_value());break;
				case HP_ROOT_OBJECT:
				case HP_ROOT_ARRAY: abort_check(sax_->end_value());break;
				default: break;
			}
			return 0;
		}
		void on_accept() override { }
		int on_error(uintptr_t id,typename syntax::parser_listener<hocon_symbol,hocon_production>::error_type type,int state,hocon_symbol word) override {
			static_cast<void>(type);
			static_cast<void>(state);
			static_cast<void>(word);
			failed_=true;
			if (sax_ && tokens_ && id!=static_cast<uintptr_t>(-1) && id>=1 && id<=tokens_->size()) {
				const hocon_token& token=(*tokens_)[id-1];
				const std::string text(token.text.begin(),token.text.end());
				sax_->parse_error(token.position,text,"Unexpected token");
			} else if (sax_) sax_->parse_error(0,std::string(),"Unexpected end of input");
			return 0;
		}
	};

public:
	static bool sax_parse(std::string_view input,sax_t* sax) {
		std::vector<hocon_token> tokens;
		{
			tokenizer scanner(input,tokens);
			if (!scanner.tokenize()) {
				sax->parse_error(scanner.error_position,std::string(),scanner.error_message);
				return false;
			}
		}
		if (tokens.size()==1) return true;
		std::vector<typename parser_t::parse_node> nodes;
		nodes.reserve(tokens.size());
		for (const auto& it:tokens) {
			typename parser_t::parse_node node;
			node.op=it.symbol;
			nodes.push_back(std::move(node));
		}
		std::lock_guard<std::mutex> lock(grammar_mutex());
		parser_t& parser=grammar();
		static hocon_listener listener;
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

protected:
	static std::string narrow(const string_t& text) {
		return std::string(text.begin(),text.end());
	}
	static std::string narrow_path(const path_t& path) {
		std::string result;
		for (std::size_t i=0;i<path.size();i++) {
			if (i) result.push_back('.');
			result.append(path[i].begin(),path[i].end());
		}
		return result;
	}
	static void stringify_scalar(string_t& out,const base_t& node) {
		switch (node.type()) {
			case structure::DDT_NULL: out.append(make_string("null"));break;
			case structure::DDT_BOOL: out.append(make_string(*node.template get_ptr<const boolean_t*>()?"true":"false"));break;
			case structure::DDT_INT: dump_integer(out,*node.template get_ptr<const int_t*>());break;
			case structure::DDT_FLOAT: dump_floating(out,*node.template get_ptr<const float_t*>());break;
			case structure::DDT_STRING: out.append(*node.template get_ptr<const string_t*>());break;
			default: throw std::runtime_error("Cannot join a non-scalar value into a string concatenation");
		}
	}
	static base_t classify_unquoted(const string_t& text) {
		const std::string raw(text.begin(),text.end());
		if (raw=="null") return base_t(nullptr);
		if (raw=="true") return base_t(static_cast<boolean_t>(true));
		if (raw=="false") return base_t(static_cast<boolean_t>(false));
		if (std::regex_match(raw,integer_regex())) {
			errno=0;
			const long long parsed=std::strtoll(raw.c_str(),nullptr,10);
			if (errno!=ERANGE) return base_t(static_cast<int_t>(parsed));
			return base_t(static_cast<float_t>(std::strtod(raw.c_str(),nullptr)));
		}
		if (std::regex_match(raw,float_regex())) return base_t(static_cast<float_t>(std::strtod(raw.c_str(),nullptr)));
		return base_t(string_t(text));
	}
	static const base_t* lookup_path(const base_t& root,const path_t& path) {
		const base_t* node=&root;
		for (const auto& key:path) {
			if (node->type()!=structure::DDT_OBJECT) return nullptr;
			const auto& table=*node->value().object;
			const auto it=table.find(key);
			if (it==table.end()) return nullptr;
			node=&it->second;
		}
		return node;
	}
	static base_t collapse_concatenation(std::vector<concatenation_part>& parts) {
		std::size_t objects=0;
		std::size_t arrays=0;
		for (const auto& it:parts) {
			if (it.node.type()==structure::DDT_OBJECT) objects++;
			else if (it.node.type()==structure::DDT_ARRAY) arrays++;
		}
		if (objects==parts.size()) {
			base_t result=std::move(parts.front().node);
			for (std::size_t i=1;i<parts.size();i++) deep_merge(result,parts[i].node);
			return result;
		}
		if (arrays==parts.size()) {
			base_t result=std::move(parts.front().node);
			for (std::size_t i=1;i<parts.size();i++) {
				auto& elements=*parts[i].node.value().array;
				for (auto& element:elements) result.value().array->push_back(std::move(element));
			}
			return result;
		}
		if (objects || arrays) throw std::runtime_error("Cannot concatenate objects or arrays with other value kinds");
		if (parts.size()==1) {
			if (parts.front().unquoted) return classify_unquoted(*parts.front().node.template get_ptr<const string_t*>());
			return std::move(parts.front().node);
		}
		string_t text;
		for (std::size_t i=0;i<parts.size();i++) {
			if (i) text.append(parts[i].lead);
			stringify_scalar(text,parts[i].node);
		}
		return base_t(std::move(text));
	}
	static bool resolve_pass(base_t& node,const base_t& root) {
		bool progress=false;
		switch (node.type()) {
			case structure::DDT_OBJECT:
			case structure::DDT_ARRAY: {
				for (auto it=node.begin();it!=node.end();it++) {
					if (resolve_pass(*it,root)) progress=true;
				}
				return progress;
			}
			default: break;
		}
		if (is_substitution(node)) {
			const substitution_value* payload=substitution_payload(node);
			const base_t* found=lookup_path(root,payload->path);
			if (found && found!=&node && is_resolved(*found)) {
				base_t copy(*found);
				node=std::move(copy);
				return true;
			}
			return false;
		}
		if (is_concatenation(node)) {
			auto& parts=concatenation_parts(node);
			bool ready=true;
			for (auto& part:parts) {
				if (resolve_pass(part.node,root)) progress=true;
				if (!is_resolved(part.node)) ready=false;
			}
			if (ready) {
				base_t collapsed=collapse_concatenation(parts);
				node=std::move(collapsed);
				return true;
			}
			return progress;
		}
		return false;
	}
	static bool finalize_node(base_t& node,const base_t& root,bool& changed) {
		switch (node.type()) {
			case structure::DDT_OBJECT: {
				auto& table=*node.value().object;
				for (auto it=table.begin();it!=table.end();) {
					if (finalize_node(it->second,root,changed)) it++;
					else {
						it=table.erase(it);
						changed=true;
					}
				}
				return true;
			}
			case structure::DDT_ARRAY: {
				auto& elements=*node.value().array;
				for (std::size_t i=0;i<elements.size();) {
					if (finalize_node(elements[i],root,changed)) i++;
					else {
						elements.erase(elements.begin()+static_cast<typename array_t::difference_type>(i));
						changed=true;
					}
				}
				return true;
			}
			default: break;
		}
		if (is_substitution(node)) {
			const substitution_value* payload=substitution_payload(node);
			if (lookup_path(root,payload->path)) return true;
			const std::string name=narrow_path(payload->path);
			const char* fallback=std::getenv(name.c_str());
			if (fallback) {
				node=base_t(make_string(fallback));
				changed=true;
				return true;
			}
			if (payload->optional) return false;
			throw std::runtime_error("Could not resolve substitution '${"+name+"}'");
		}
		if (is_concatenation(node)) {
			auto& parts=concatenation_parts(node);
			for (std::size_t i=0;i<parts.size();) {
				if (finalize_node(parts[i].node,root,changed)) i++;
				else {
					parts.erase(parts.begin()+static_cast<std::ptrdiff_t>(i));
					changed=true;
				}
			}
			if (parts.empty()) return false;
			return true;
		}
		return true;
	}
	static void resolve_tree(base_t& root) {
		bool changed=true;
		while (changed) {
			while (resolve_pass(root,root)) { }
			changed=false;
			finalize_node(root,root,changed);
		}
		if (!is_resolved(root)) throw std::runtime_error("Substitution cycle detected");
	}
	static base_t* navigate_location(base_t& root,const location_t& location) {
		base_t* node=&root;
		for (const auto& seg:location) {
			if (seg.is_index) {
				if (node->type()!=structure::DDT_ARRAY) return nullptr;
				auto& elements=*node->value().array;
				if (seg.index>=elements.size()) return nullptr;
				node=&elements[seg.index];
			} else {
				if (node->type()!=structure::DDT_OBJECT) return nullptr;
				auto& table=*node->value().object;
				const auto it=table.find(seg.key);
				if (it==table.end()) return nullptr;
				node=&it->second;
			}
		}
		return node;
	}
	static void merge_underneath(base_t& target,base_t& source) {
		auto& table=*target.value().object;
		for (auto&& it:*source.value().object) {
			const auto slot=table.find(it.first);
			if (slot==table.end()) table.emplace(it.first,std::move(it.second));
			else if (slot->second.type()==structure::DDT_OBJECT && it.second.type()==structure::DDT_OBJECT) merge_underneath(slot->second,it.second);
		}
	}
	static void apply_include(base_t& root,const include_record_t& record,const include_handler_t& handler) {
		hocon fetched;
		bool found=false;
		if (handler) found=handler(record.spec,record.kind,record.required,fetched);
		if (!found) {
			if (record.required) throw std::runtime_error("Required include '"+narrow(record.spec)+"' could not be resolved"+(handler?std::string():std::string(" (no include handler was provided)")));
			return;
		}
		if (!fetched.is_object()) throw std::runtime_error("Included configuration '"+narrow(record.spec)+"' must be an object");
		base_t* target=navigate_location(root,record.location);
		if (!target || target->type()!=structure::DDT_OBJECT) return;
		merge_underneath(*target,fetched);
	}

public:
	static void resolve_document(base_t& root,const std::vector<include_record_t>& includes,const include_handler_t& handler) {
		for (const auto& record:includes) apply_include(root,record,handler);
		resolve_tree(root);
	}
	void resolve(const include_handler_t& handler={}) {
		resolve_document(*this,std::vector<include_record_t>(),handler);
	}

protected:
	static void dump_hex16(string_t& out,unsigned long cp) {
		char buffer[8];
		const int length=std::snprintf(buffer,sizeof(buffer),"\\u%04lX",cp&0xFFFFul);
		out.append(buffer,buffer+length);
	}
	static void dump_hex32(string_t& out,unsigned long cp) {
		const unsigned long value=cp-0x10000ul;
		dump_hex16(out,0xD800ul|(value>>10));
		dump_hex16(out,0xDC00ul|(value&0x3FFul));
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
			} else if (cp<0x10000) dump_hex16(out,cp);
			else dump_hex32(out,cp);
			first+=extra+1;
		}
	}
	static bool bare_key_allowed(const string_t& key) noexcept {
		if (key.empty()) return false;
		for (auto it:key) {
			const char c=static_cast<char>(it);
			if (!((c>='A' && c<='Z') || (c>='a' && c<='z') || (c>='0' && c<='9') || c=='_' || c=='-')) return false;
		}
		return true;
	}
	static void render_key(string_t& out,const string_t& key,bool ensure_ascii) {
		if (bare_key_allowed(key)) {
			out.append(key);
			return;
		}
		out.push_back('"');
		dump_escaped(out,key,ensure_ascii);
		out.push_back('"');
	}
	static void render_path(string_t& out,const path_t& path,bool ensure_ascii) {
		for (std::size_t i=0;i<path.size();i++) {
			if (i) out.push_back('.');
			render_key(out,path[i],ensure_ascii);
		}
	}
	static void dump_integer(string_t& out,int_t value) {
		const std::string text=std::to_string(static_cast<long long>(value));
		out.append(text.begin(),text.end());
	}
	static void dump_floating(string_t& out,float_t value) {
		if (!std::isfinite(static_cast<double>(value))) {
			out.append(make_string("null"));
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
	static void dump_members(const base_t& node,string_t& out,int indent_step,std::size_t depth,typename string_t::value_type indent_char,bool ensure_ascii) {
		const bool pretty=indent_step>0;
		const std::size_t prefix=pretty?depth*static_cast<std::size_t>(indent_step):0;
		for (auto it=node.cbegin();it!=node.cend();) {
			if (pretty) dump_indent(out,prefix,indent_char);
			render_key(out,it.key(),ensure_ascii);
			if (pretty) out.append(make_string(" = "));
			else out.push_back('=');
			dump_value(*it,out,indent_step,depth,indent_char,ensure_ascii);
			it++;
			if (it!=node.cend()) {
				if (pretty) out.push_back('\n');
				else {
					out.push_back(',');
					out.push_back(' ');
				}
			}
		}
	}
	static void dump_value(const base_t& node,string_t& out,int indent_step,std::size_t depth,typename string_t::value_type indent_char,bool ensure_ascii) {
		const bool pretty=indent_step>0;
		switch (node.type()) {
			case structure::DDT_NULL: out.append(make_string("null"));break;
			case structure::DDT_BOOL: out.append(make_string(*node.template get_ptr<const boolean_t*>()?"true":"false"));break;
			case structure::DDT_INT: dump_integer(out,*node.template get_ptr<const int_t*>());break;
			case structure::DDT_FLOAT: dump_floating(out,*node.template get_ptr<const float_t*>());break;
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
				if (pretty) {
					out.push_back('\n');
					for (auto it=node.cbegin();it!=node.cend();) {
						dump_indent(out,(depth+1)*static_cast<std::size_t>(indent_step),indent_char);
						dump_value(*it,out,indent_step,depth+1,indent_char,ensure_ascii);
						it++;
						if (it!=node.cend()) out.push_back(',');
						out.push_back('\n');
					}
					dump_indent(out,depth*static_cast<std::size_t>(indent_step),indent_char);
				} else {
					for (auto it=node.cbegin();it!=node.cend();) {
						dump_value(*it,out,indent_step,depth,indent_char,ensure_ascii);
						it++;
						if (it!=node.cend()) {
							out.push_back(',');
							out.push_back(' ');
						}
					}
				}
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
				if (pretty) {
					out.push_back('\n');
					dump_members(node,out,indent_step,depth+1,indent_char,ensure_ascii);
					out.push_back('\n');
					dump_indent(out,depth*static_cast<std::size_t>(indent_step),indent_char);
				} else {
					dump_members(node,out,indent_step,depth,indent_char,ensure_ascii);
				}
				out.push_back('}');
				break;
			}
			default: {
				if (is_substitution(node)) {
					const substitution_value* payload=substitution_payload(node);
					out.append(make_string(payload->optional?"${?":"${"));
					render_path(out,payload->path,ensure_ascii);
					out.push_back('}');
					break;
				}
				if (is_concatenation(node)) {
					const auto& parts=concatenation_parts(node);
					for (std::size_t i=0;i<parts.size();i++) {
						if (i) out.append(parts[i].lead);
						if (parts[i].unquoted && parts[i].node.type()==structure::DDT_STRING) out.append(*parts[i].node.template get_ptr<const string_t*>());
						else dump_value(parts[i].node,out,indent_step,depth,indent_char,ensure_ascii);
					}
					break;
				}
				throw std::invalid_argument("Unsupported node type "+std::to_string(static_cast<long long>(node.type()))+", convert it to the base dom model first");
			}
		}
	}

public:
	virtual string_t dump(int indent=-1,typename string_t::value_type indent_char=' ',bool ensure_ascii=false) const {
		string_t result;
		if (this->is_object()) {
			dump_members(*this,result,indent,0,indent_char,ensure_ascii);
			if (indent>0 && !this->empty()) result.push_back('\n');
			return result;
		}
		if (this->is_array()) {
			dump_value(*this,result,indent,0,indent_char,ensure_ascii);
			return result;
		}
		throw std::invalid_argument("Top-level value must be an object or an array");
	}

	//---解析族---
	static hocon parse(std::string_view input,const include_handler_t& handler={},bool allow_exceptions=true) {
		hocon_sax_dom_builder<hocon> builder;
		const bool ok=sax_parse(input,&builder) && builder.completed();
		if (!ok) {
			if (allow_exceptions) throw std::runtime_error(std::string("Parse error at byte ")+std::to_string(builder.error_position())+std::string(": ")+(builder.error_message().empty()?std::string("Incomplete document"):builder.error_message()));
			return hocon(nullptr);
		}
		hocon result;
		static_cast<base_t&>(result)=std::move(builder.root());
		try {
			resolve_document(result,builder.includes(),handler);
		} catch (...) {
			if (allow_exceptions) throw;
			return hocon(nullptr);
		}
		return result;
	}
	static hocon parse_unresolved(std::string_view input,bool allow_exceptions=true) {
		hocon_sax_dom_builder<hocon> builder;
		const bool ok=sax_parse(input,&builder) && builder.completed();
		if (!ok) {
			if (allow_exceptions) throw std::runtime_error(std::string("Parse error at byte ")+std::to_string(builder.error_position())+std::string(": ")+(builder.error_message().empty()?std::string("Incomplete document"):builder.error_message()));
			return hocon(nullptr);
		}
		hocon result;
		static_cast<base_t&>(result)=std::move(builder.root());
		return result;
	}
	static bool try_parse(std::string_view input,hocon& out,const include_handler_t& handler={}) {
		hocon_sax_dom_builder<hocon> builder;
		if (!sax_parse(input,&builder) || !builder.completed()) return false;
		hocon result;
		static_cast<base_t&>(result)=std::move(builder.root());
		try {
			resolve_document(result,builder.includes(),handler);
		} catch (...) {
			return false;
		}
		out=std::move(result);
		return true;
	}
	static bool accept(std::string_view input) {
		hocon_sax_acceptor<hocon> acceptor;
		return sax_parse(input,&acceptor);
	}

	friend std::ostream& operator <<(std::ostream& os,const hocon& value) {
		const int indent_step=static_cast<int>(os.width());
		os.width(0);
		const string_t text=value.dump(indent_step>0?indent_step:-1,static_cast<typename string_t::value_type>(os.fill()));
		os.write(reinterpret_cast<const char*>(text.data()),static_cast<std::streamsize>(text.size()));
		return os;
	}
	friend std::istream& operator >>(std::istream& is,hocon& value) {
		std::string content((std::istreambuf_iterator<char>(is)),std::istreambuf_iterator<char>());
		value=parse(content);
		return is;
	}
};

_STDEX_DOM_TPL_DEFAULT_DECLARATION
inline typename hocon<_Int,_Float,_Boolean,_String,_Array,_Object,_Allocator>::string_t to_string(const hocon<_Int,_Float,_Boolean,_String,_Array,_Object,_Allocator>& value) {
	return value.dump();
}

}

_STDEX_DOM_TPL_DEFAULT_DECLARATION
using hocon_t=basic_hocon::hocon<_Int,_Float,_Boolean,_String,_Array,_Object,_Allocator>;
using hocon=hocon_t<>;
using basic_hocon::hocon_sax;
using basic_hocon::hocon_sax_dom_builder;
using basic_hocon::hocon_sax_acceptor;
using basic_hocon::to_string;
using basic_hocon::hocon_include_kind;
using basic_hocon::HIK_HEURISTIC;
using basic_hocon::HIK_FILE;
using basic_hocon::HIK_URL;
using basic_hocon::HIK_CLASSPATH;

inline namespace literals {

inline hocon_t<> operator ""_hocon(const char* s,std::size_t n) {
	return hocon_t<>::parse(std::string_view(s,n));
}

}

}

}

#endif
