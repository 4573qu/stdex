//Last Modified At 2026/06/12
//@Version 1.0.0.0
#ifndef _STDEX_TYPE_DOM_TOML_H_
#define _STDEX_TYPE_DOM_TOML_H_ 1

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
#include <set>
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

namespace basic_toml {

_STDEX_DERIVED_KIND(toml_data_type,structure::dom_data_type,_STDEX_KIND_AUTO_START,
	_STDEX_KIND_VALUE_AUTO(TDT_DATETIME)
)

enum toml_datetime_variant : int {
	TDV_OFFSET_DATETIME,
	TDV_LOCAL_DATETIME,
	TDV_LOCAL_DATE,
	TDV_LOCAL_TIME,
};

struct toml_datetime {
	toml_datetime_variant variant=TDV_LOCAL_DATE;
	int year=1970;
	int month=1;
	int day=1;
	int hour=0;
	int minute=0;
	int second=0;
	long nanosecond=0;
	int offset_minutes=0;

	bool has_date() const noexcept {
		return variant!=TDV_LOCAL_TIME;
	}
	bool has_time() const noexcept {
		return variant!=TDV_LOCAL_DATE;
	}
	bool has_offset() const noexcept {
		return variant==TDV_OFFSET_DATETIME;
	}

	friend bool operator ==(const toml_datetime& lhs,const toml_datetime& rhs) noexcept {
		return lhs.variant==rhs.variant && lhs.year==rhs.year && lhs.month==rhs.month && lhs.day==rhs.day && lhs.hour==rhs.hour && lhs.minute==rhs.minute && lhs.second==rhs.second && lhs.nanosecond==rhs.nanosecond && lhs.offset_minutes==rhs.offset_minutes;
	}
	friend bool operator !=(const toml_datetime& lhs,const toml_datetime& rhs) noexcept {
		return !(lhs==rhs);
	}
};

enum toml_symbol : int {
	TS_EPSILON,
	TS_EOF,
	TS_NEWLINE,
	TS_EQ,
	TS_DOT,
	TS_COMMA,
	TS_LBRACKET,
	TS_RBRACKET,
	TS_LBRACE,
	TS_RBRACE,
	TS_TABLE_OPEN,
	TS_TABLE_CLOSE,
	TS_ARRAY_TABLE_OPEN,
	TS_ARRAY_TABLE_CLOSE,
	TS_KEY,
	TS_STRING,
	TS_INT,
	TS_FLOAT,
	TS_BOOL,
	TS_DATETIME,
	TS_START,
	TS_TOML,
	TS_LINE_SEQ,
	TS_LINE,
	TS_TABLE,
	TS_ARRAY_TABLE,
	TS_KEYVAL,
	TS_KEYPATH,
	TS_VALUE,
	TS_ARRAY,
	TS_ELEMENTS,
	TS_INLINE_TABLE,
	TS_MEMBERS,
};

enum toml_production : int {
	TP_START,
	TP_TOML,
	TP_LINES_FIRST,
	TP_LINES_APPEND,
	TP_LINE_BLANK,
	TP_LINE_KEYVAL,
	TP_LINE_TABLE,
	TP_LINE_ARRAY_TABLE,
	TP_TABLE,
	TP_ARRAY_TABLE,
	TP_KEYVAL,
	TP_KEYPATH_FIRST,
	TP_KEYPATH_APPEND,
	TP_VALUE_STRING,
	TP_VALUE_INT,
	TP_VALUE_FLOAT,
	TP_VALUE_BOOL,
	TP_VALUE_DATETIME,
	TP_VALUE_ARRAY,
	TP_VALUE_INLINE_TABLE,
	TP_ARRAY_EMPTY,
	TP_ARRAY,
	TP_ARRAY_TRAILING,
	TP_ELEMENTS_FIRST,
	TP_ELEMENTS_APPEND,
	TP_INLINE_TABLE_EMPTY,
	TP_INLINE_TABLE,
	TP_MEMBERS_FIRST,
	TP_MEMBERS_APPEND,
};

template <typename _Toml>
struct toml_sax {
	using int_t=typename _Toml::int_t;
	using float_t=typename _Toml::float_t;
	using boolean_t=typename _Toml::boolean_t;
	using string_t=typename _Toml::string_t;
	using path_t=std::vector<string_t>;

	virtual bool table(path_t& path)=0;
	virtual bool array_table(path_t& path)=0;
	virtual bool key(path_t& path)=0;
	virtual bool string(string_t& value)=0;
	virtual bool number_integer(int_t value)=0;
	virtual bool number_float(float_t value,const string_t& raw)=0;
	virtual bool boolean(boolean_t value)=0;
	virtual bool datetime(toml_datetime& value)=0;
	virtual bool start_array(std::size_t cnt)=0;
	virtual bool end_array()=0;
	virtual bool start_inline_table(std::size_t cnt)=0;
	virtual bool end_inline_table()=0;
	virtual bool parse_error(std::size_t position,const std::string& last_token,const std::string& message)=0;
	virtual ~toml_sax()=default;
};

template <typename _Toml>
class toml_sax_dom_builder : public toml_sax<_Toml> {
public:
	using int_t=typename _Toml::int_t;
	using float_t=typename _Toml::float_t;
	using boolean_t=typename _Toml::boolean_t;
	using string_t=typename _Toml::string_t;
	using path_t=typename toml_sax<_Toml>::path_t;
	using dom_t=typename _Toml::base_t;

private:
	struct shadow_t {
		bool explicit_header=false;
		bool dotted=false;
		bool closed=false;
		bool array_table=false;
		unsigned long generation=0;
		std::vector<std::pair<string_t,shadow_t>> children{};
	};

	_Toml& root_;
	dom_t* current_table_=nullptr;
	shadow_t root_shadow_{};
	shadow_t* current_shadow_=nullptr;
	std::vector<dom_t*> ref_stack_;
	std::vector<std::set<const void*>> inline_closed_;
	path_t pending_path_;
	bool pending_=false;
	unsigned long generation_=0;
	std::size_t depth_=0;
	bool errored_=false;
	std::size_t error_position_=0;
	std::string error_message_;

	static shadow_t* shadow_child(shadow_t& parent,const string_t& key) {
		for (auto& it:parent.children) {
			if (it.first==key) return &it.second;
		}
		parent.children.emplace_back(key,shadow_t());
		return &parent.children.back().second;
	}
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

	bool navigate_header(const path_t& path,bool as_array_table) {
		dom_t* node=static_cast<dom_t*>(&root_);
		shadow_t* sh=&root_shadow_;
		for (std::size_t i=0;i+1<path.size();i++) {
			const string_t& key=path[i];
			auto& table=*node->value().object;
			shadow_t* csh=shadow_child(*sh,key);
			auto it=table.find(key);
			if (it==table.end()) {
				auto result=table.emplace(key,dom_t(structure::DDT_OBJECT));
				node=&result.first->second;
			} else if (csh->array_table) {
				node=&it->second.value().array->back();
			} else if (csh->closed) {
				return fail("key '"+narrow(key)+"' conflicts with a previously defined value");
			} else if (csh->dotted) {
				return fail("cannot extend table '"+narrow(key)+"' defined by dotted keys with a table header");
			} else {
				node=&it->second;
			}
			sh=csh;
		}
		const string_t& key=path.back();
		auto& table=*node->value().object;
		shadow_t* csh=shadow_child(*sh,key);
		auto it=table.find(key);
		if (!as_array_table) {
			if (it==table.end()) {
				auto result=table.emplace(key,dom_t(structure::DDT_OBJECT));
				node=&result.first->second;
			} else {
				if (csh->array_table) return fail("cannot redefine array of tables '"+narrow(key)+"' as a table");
				if (csh->closed) return fail("table header '"+narrow(key)+"' conflicts with a previously defined value");
				if (csh->dotted) return fail("cannot reopen table '"+narrow(key)+"' defined by dotted keys");
				if (csh->explicit_header) return fail("table '"+narrow(key)+"' is already defined");
				node=&it->second;
			}
			csh->explicit_header=true;
			current_table_=node;
		} else {
			if (it==table.end()) {
				auto result=table.emplace(key,dom_t(structure::DDT_ARRAY));
				result.first->second.value().array->push_back(dom_t(structure::DDT_OBJECT));
				csh->array_table=true;
				current_table_=&result.first->second.value().array->back();
			} else {
				if (!csh->array_table) return fail("cannot append to '"+narrow(key)+"': not an array of tables created by [[...]]");
				it->second.value().array->push_back(dom_t(structure::DDT_OBJECT));
				current_table_=&it->second.value().array->back();
			}
			csh->children.clear();
		}
		current_shadow_=csh;
		return true;
	}

	dom_t* resolve_table_slot() {
		dom_t* node=current_table_;
		shadow_t* sh=current_shadow_;
		for (std::size_t i=0;i+1<pending_path_.size();i++) {
			const string_t& key=pending_path_[i];
			auto& table=*node->value().object;
			shadow_t* csh=shadow_child(*sh,key);
			auto it=table.find(key);
			if (it==table.end()) {
				auto result=table.emplace(key,dom_t(structure::DDT_OBJECT));
				node=&result.first->second;
				csh->dotted=true;
				csh->generation=generation_;
			} else {
				if (!csh->dotted || csh->generation!=generation_) {
					fail("dotted key cannot extend '"+narrow(key)+"': it was not created by dotted keys in the current table");
					return nullptr;
				}
				node=&it->second;
			}
			sh=csh;
		}
		const string_t& key=pending_path_.back();
		auto& table=*node->value().object;
		auto it=table.find(key);
		if (it!=table.end()) {
			fail("duplicate key '"+narrow(key)+"'");
			return nullptr;
		}
		shadow_t* csh=shadow_child(*sh,key);
		csh->closed=true;
		csh->generation=generation_;
		auto result=table.emplace(key,dom_t());
		return &result.first->second;
	}
	dom_t* resolve_inline_slot() {
		dom_t* node=ref_stack_.back();
		std::set<const void*>& closed=inline_closed_.back();
		for (std::size_t i=0;i+1<pending_path_.size();i++) {
			const string_t& key=pending_path_[i];
			auto& table=*node->value().object;
			auto it=table.find(key);
			if (it==table.end()) {
				auto result=table.emplace(key,dom_t(structure::DDT_OBJECT));
				node=&result.first->second;
			} else {
				if (it->second.type()!=structure::DDT_OBJECT || closed.count(&it->second)) {
					fail("dotted key cannot extend '"+narrow(key)+"' inside an inline table");
					return nullptr;
				}
				node=&it->second;
			}
		}
		const string_t& key=pending_path_.back();
		auto& table=*node->value().object;
		if (table.find(key)!=table.end()) {
			fail("duplicate key '"+narrow(key)+"' in inline table");
			return nullptr;
		}
		auto result=table.emplace(key,dom_t());
		closed.insert(&result.first->second);
		return &result.first->second;
	}
	dom_t* handle_value(dom_t&& value) {
		if (!ref_stack_.empty() && ref_stack_.back()->type()==structure::DDT_ARRAY) {
			auto& array=*ref_stack_.back()->value().array;
			array.push_back(std::move(value));
			return &array.back();
		}
		if (!pending_) {
			fail("value arrived without a pending key");
			return nullptr;
		}
		pending_=false;
		dom_t* slot=ref_stack_.empty()?resolve_table_slot():resolve_inline_slot();
		if (!slot) return nullptr;
		*slot=std::move(value);
		return slot;
	}

public:
	explicit toml_sax_dom_builder(_Toml& root) : root_(root) {
		root_=_Toml(structure::DDT_OBJECT);
		current_table_=static_cast<dom_t*>(&root_);
		current_shadow_=&root_shadow_;
	}
	bool table(path_t& path) override {
		if (errored_) return false;
		generation_++;
		return navigate_header(path,false);
	}
	bool array_table(path_t& path) override {
		if (errored_) return false;
		generation_++;
		return navigate_header(path,true);
	}
	bool key(path_t& path) override {
		if (errored_) return false;
		pending_path_=path;
		pending_=true;
		return true;
	}
	bool string(string_t& value) override {
		if (errored_) return false;
		return handle_value(dom_t(std::move(value)))!=nullptr;
	}
	bool number_integer(int_t value) override {
		if (errored_) return false;
		return handle_value(dom_t(value))!=nullptr;
	}
	bool number_float(float_t value,const string_t& raw) override {
		static_cast<void>(raw);
		if (errored_) return false;
		return handle_value(dom_t(value))!=nullptr;
	}
	bool boolean(boolean_t value) override {
		if (errored_) return false;
		return handle_value(dom_t(value))!=nullptr;
	}
	bool datetime(toml_datetime& value) override {
		if (errored_) return false;
		return handle_value(_Toml::make_datetime(value))!=nullptr;
	}
	bool start_array(std::size_t cnt) override {
		static_cast<void>(cnt);
		if (errored_) return false;
		dom_t* node=handle_value(dom_t(structure::DDT_ARRAY));
		if (!node) return false;
		ref_stack_.push_back(node);
		depth_++;
		return true;
	}
	bool end_array() override {
		if (errored_) return false;
		ref_stack_.pop_back();
		depth_--;
		return true;
	}
	bool start_inline_table(std::size_t cnt) override {
		static_cast<void>(cnt);
		if (errored_) return false;
		dom_t* node=handle_value(dom_t(structure::DDT_OBJECT));
		if (!node) return false;
		ref_stack_.push_back(node);
		inline_closed_.emplace_back();
		depth_++;
		return true;
	}
	bool end_inline_table() override {
		if (errored_) return false;
		ref_stack_.pop_back();
		inline_closed_.pop_back();
		depth_--;
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
		return !errored_ && !depth_;
	}
	std::size_t error_position() const noexcept {
		return error_position_;
	}
	const std::string& error_message() const noexcept {
		return error_message_;
	}
};

template <typename _Toml>
class toml_sax_acceptor : public toml_sax<_Toml> {
public:
	using int_t=typename _Toml::int_t;
	using float_t=typename _Toml::float_t;
	using boolean_t=typename _Toml::boolean_t;
	using string_t=typename _Toml::string_t;
	using path_t=typename toml_sax<_Toml>::path_t;

	bool table(path_t&) override { return true; }
	bool array_table(path_t&) override { return true; }
	bool key(path_t&) override { return true; }
	bool string(string_t&) override { return true; }
	bool number_integer(int_t) override { return true; }
	bool number_float(float_t,const string_t&) override { return true; }
	bool boolean(boolean_t) override { return true; }
	bool datetime(toml_datetime&) override { return true; }
	bool start_array(std::size_t) override { return true; }
	bool end_array() override { return true; }
	bool start_inline_table(std::size_t) override { return true; }
	bool end_inline_table() override { return true; }
	bool parse_error(std::size_t,const std::string&,const std::string&) override { return false; }
};

_STDEX_DOM_TPL_DECLARATION
class toml : public structure::_STDEX_DOM_DEF {
public:
	using base_t=structure::_STDEX_DOM_DEF;
	using int_t=typename base_t::int_t;
	using float_t=typename base_t::float_t;
	using boolean_t=typename base_t::boolean_t;
	using string_t=typename base_t::string_t;
	using array_t=typename base_t::array_t;
	using object_t=typename base_t::object_t;
	using size_type=typename base_t::size_type;
	using sax_t=toml_sax<toml>;
	using datetime_t=toml_datetime;

	static_assert(sizeof(typename string_t::value_type)==1,"toml serializer assumes a byte-oriented (UTF-8) string_t.");

protected:
	struct datetime_value : base_t::value_t {
		toml_datetime moment{};

		datetime_value()=default;
		explicit datetime_value(const toml_datetime& value) noexcept : moment(value) { }
		~datetime_value() override=default;

		datetime_value(const datetime_value& other) : base_t::value_t() , moment(other.moment) { }

		typename base_t::value_t* clone(structure::dom_data_type t) const override {
			if (t==TDT_DATETIME) return create_value<datetime_value>(*this);
			return base_t::value_t::clone(t);
		}
		void destroy(structure::dom_data_type t) override {
			if (t==TDT_DATETIME) return;
			base_t::value_t::destroy(t);
		}
		void destroy_self(structure::dom_data_type t) override {
			this->destroy(t);
			_Allocator<datetime_value> alloc;
			std::allocator_traits<_Allocator<datetime_value>>::destroy(alloc,this);
			std::allocator_traits<_Allocator<datetime_value>>::deallocate(alloc,this,1);
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
	static datetime_value* datetime_payload(const base_t& node) {
		datetime_value* payload=node.data().value?dynamic_cast<datetime_value*>(node.data().value):nullptr;
		if (node.type()!=toml_data_type(TDT_DATETIME) || !payload) throw std::invalid_argument("Node does not hold a toml datetime payload");
		return payload;
	}
	static string_t make_string(const char* text) {
		return string_t(text,text+std::strlen(text));
	}

public:
	using base_t::base_t;
	using base_t::operator =;

	toml()=default;
	~toml() override=default;

	toml(const toml&)=default;
	toml(toml&&) noexcept=default;

	toml& operator =(const toml&)=default;
	toml& operator =(toml&&)=default;

	toml(const base_t& other) : base_t(other) { }
	toml(base_t&& other) noexcept : base_t(std::move(other)) { }

	bool support(structure::dom_data_type t) const noexcept override {
		return base_t::support(t) || t==TDT_DATETIME;
	}

	static toml array(typename base_t::initializer_list_t init_list={}) {
		return toml(base_t::array(init_list));
	}
	static toml object(typename base_t::initializer_list_t init_list={}) {
		return toml(base_t::object(init_list));
	}

	static base_t make_datetime(const toml_datetime& value) {
		base_t node;
		node.data()=typename base_t::data_t(structure::dom_data_type(TDT_DATETIME),create_value<datetime_value>(value));
		return node;
	}
	static toml datetime(const toml_datetime& value) {
		return toml(make_datetime(value));
	}
	static toml local_date(int year,int month,int day) {
		toml_datetime value;
		value.variant=TDV_LOCAL_DATE;
		value.year=year;
		value.month=month;
		value.day=day;
		return datetime(value);
	}
	static toml local_time(int hour,int minute,int second,long nanosecond=0) {
		toml_datetime value;
		value.variant=TDV_LOCAL_TIME;
		value.hour=hour;
		value.minute=minute;
		value.second=second;
		value.nanosecond=nanosecond;
		return datetime(value);
	}
	static toml local_datetime(int year,int month,int day,int hour,int minute,int second,long nanosecond=0) {
		toml_datetime value;
		value.variant=TDV_LOCAL_DATETIME;
		value.year=year;
		value.month=month;
		value.day=day;
		value.hour=hour;
		value.minute=minute;
		value.second=second;
		value.nanosecond=nanosecond;
		return datetime(value);
	}
	static toml offset_datetime(int year,int month,int day,int hour,int minute,int second,int offset_minutes,long nanosecond=0) {
		toml_datetime value;
		value.variant=TDV_OFFSET_DATETIME;
		value.year=year;
		value.month=month;
		value.day=day;
		value.hour=hour;
		value.minute=minute;
		value.second=second;
		value.nanosecond=nanosecond;
		value.offset_minutes=offset_minutes;
		return datetime(value);
	}

	static bool is_datetime(const base_t& node) noexcept {
		return node.type()==toml_data_type(TDT_DATETIME);
	}
	bool is_datetime() const noexcept {
		return is_datetime(*this);
	}
	static toml_datetime& get_datetime(const base_t& node) {
		return datetime_payload(node)->moment;
	}
	toml_datetime& get_datetime() {
		return get_datetime(*this);
	}
	const toml_datetime& get_datetime() const {
		return get_datetime(*this);
	}
	static void set_datetime(base_t& node,const toml_datetime& value) {
		node.data()=typename base_t::data_t(structure::dom_data_type(TDT_DATETIME),create_value<datetime_value>(value));
	}
	void set_datetime(const toml_datetime& value) {
		set_datetime(*this,value);
	}

protected:
	//记法转换协议·源侧降级:TDT_DATETIME默认降级为RFC 3339字符串(无损可读)。
	//如需其他形态(如epoch整数)请用convert_handler_t。
	bool degrade_unsupported(const base_t& source,base_t& replacement) const override {
		if (source.type()!=toml_data_type(TDT_DATETIME)) return false;
		string_t text;
		format_datetime(text,get_datetime(source));
		replacement=base_t(std::move(text));
		return true;
	}

private:
	struct toml_token {
		toml_symbol symbol;
		std::size_t position;
		string_t text{};
		int_t integer=0;
		float_t floating=0;
		boolean_t boolean=false;
		toml_datetime moment{};
	};

	static const std::regex& bare_key_regex() {
		static const std::regex result(R"([A-Za-z0-9_\-]+)",std::regex::optimize);
		return result;
	}
	static const std::regex& basic_string_regex() {
		static const std::regex result(R"#("(?:[^"\\\x00-\x08\x0A-\x1F\x7F]|\\(?:["\\btnfr]|u[0-9A-Fa-f]{4}|U[0-9A-Fa-f]{8}))*")#",std::regex::optimize);
		return result;
	}
	static const std::regex& literal_string_regex() {
		static const std::regex result(R"('[^'\x00-\x08\x0A-\x1F\x7F]*')",std::regex::optimize);
		return result;
	}
	//四种RFC 3339变体合一:组1-3日期,组4-7时间(组7小数),组8为Z,组9-11数字时差,
	//组12-15为纯本地时间。空格分隔形态仅当其后紧跟完整时间才并入,否则回退为纯日期。
	static const std::regex& datetime_regex() {
		static const std::regex result(R"((\d{4})-(\d{2})-(\d{2})(?:[Tt ](\d{2}):(\d{2}):(\d{2})(?:\.(\d+))?(?:([Zz])|([+-])(\d{2}):(\d{2}))?)?|(\d{2}):(\d{2}):(\d{2})(?:\.(\d+))?)",std::regex::optimize);
		return result;
	}
	static const std::regex& prefixed_int_regex() {
		static const std::regex result(R"(0x[0-9A-Fa-f](?:_?[0-9A-Fa-f])*|0o[0-7](?:_?[0-7])*|0b[01](?:_?[01])*)",std::regex::optimize);
		return result;
	}
	static const std::regex& float_regex() {
		static const std::regex result(R"([+-]?(?:(?:0|[1-9](?:_?[0-9])*)(?:\.[0-9](?:_?[0-9])*(?:[eE][+-]?[0-9](?:_?[0-9])*)?|[eE][+-]?[0-9](?:_?[0-9])*)|inf|nan))",std::regex::optimize);
		return result;
	}
	static const std::regex& dec_int_regex() {
		static const std::regex result(R"([+-]?(?:0|[1-9](?:_?[0-9])*))",std::regex::optimize);
		return result;
	}
	static const std::regex& bool_regex() {
		static const std::regex result(R"(true|false)",std::regex::optimize);
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
	static unsigned long hex_value(const char* p,int count) {
		unsigned long result=0;
		for (int i=0;i<count;i++) {
			const char c=p[i];
			result<<=4;
			if (c>='0' && c<='9') result|=static_cast<unsigned long>(c-'0');
			else if (c>='a' && c<='f') result|=static_cast<unsigned long>(c-'a'+10);
			else result|=static_cast<unsigned long>(c-'A'+10);
		}
		return result;
	}
	static bool valid_scalar_codepoint(unsigned long cp) noexcept {
		return cp<=0x10FFFF && !(cp>=0xD800 && cp<=0xDFFF);
	}
	static bool decode_basic(const char* first,const char* last,string_t& out,std::string& error_message) {
		first++;
		last--;
		while (first<last) {
			if (*first!='\\') {
				out.push_back(*first++);
				continue;
			}
			first++;
			switch (*first++) {
				case 'b': out.push_back('\b');break;
				case 't': out.push_back('\t');break;
				case 'n': out.push_back('\n');break;
				case 'f': out.push_back('\f');break;
				case 'r': out.push_back('\r');break;
				case '"': out.push_back('"');break;
				case '\\': out.push_back('\\');break;
				case 'u': {
					const unsigned long cp=hex_value(first,4);
					first+=4;
					if (!valid_scalar_codepoint(cp)) {
						error_message="\\u escape is not a Unicode scalar value";
						return false;
					}
					append_codepoint(out,cp);
					break;
				}
				case 'U': {
					const unsigned long cp=hex_value(first,8);
					first+=8;
					if (!valid_scalar_codepoint(cp)) {
						error_message="\\U escape is not a Unicode scalar value";
						return false;
					}
					append_codepoint(out,cp);
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
	static bool scan_ml_basic(const char* first,const char* const last,string_t& out,std::size_t& consumed,std::string& error_message) {
		const char* const begin=first;
		if (first<last && *first=='\r' && first+1<last && first[1]=='\n') first+=2;
		else if (first<last && *first=='\n') first++;
		while (first<last) {
			const char c=*first;
			if (c=='"') {
				std::size_t run=0;
				while (first+run<last && first[run]=='"') run++;
				if (run>=3) {
					if (run>5) {
						error_message="too many quotes at the end of a multi-line string";
						return false;
					}
					for (std::size_t i=0;i<run-3;i++) out.push_back('"');
					consumed=static_cast<std::size_t>(first-begin)+run;
					return true;
				}
				for (std::size_t i=0;i<run;i++) out.push_back('"');
				first+=run;
				continue;
			}
			if (c=='\\') {
				const char* p=first+1;
				while (p<last && (*p==' ' || *p=='\t')) p++;
				if (p<last && (*p=='\n' || (*p=='\r' && p+1<last && p[1]=='\n'))) {
					first=p;
					while (first<last) {
						if (*first==' ' || *first=='\t' || *first=='\n') first++;
						else if (*first=='\r') {
							if (first+1<last && first[1]=='\n') first+=2;
							else {
								error_message="stray carriage return in multi-line string";
								return false;
							}
						} else break;
					}
					continue;
				}
				if (first+1>=last) {
					error_message="unterminated escape in multi-line string";
					return false;
				}
				const char escape=first[1];
				first+=2;
				switch (escape) {
					case 'b': out.push_back('\b');break;
					case 't': out.push_back('\t');break;
					case 'n': out.push_back('\n');break;
					case 'f': out.push_back('\f');break;
					case 'r': out.push_back('\r');break;
					case '"': out.push_back('"');break;
					case '\\': out.push_back('\\');break;
					case 'u':
					case 'U': {
						const int count=escape=='u'?4:8;
						if (last-first<count) {
							error_message="truncated unicode escape";
							return false;
						}
						for (int i=0;i<count;i++) {
							const char h=first[i];
							if (!((h>='0' && h<='9') || (h>='a' && h<='f') || (h>='A' && h<='F'))) {
								error_message="invalid unicode escape";
								return false;
							}
						}
						const unsigned long cp=hex_value(first,count);
						first+=count;
						if (!valid_scalar_codepoint(cp)) {
							error_message="unicode escape is not a Unicode scalar value";
							return false;
						}
						append_codepoint(out,cp);
						break;
					}
					default: {
						error_message="invalid escape sequence";
						return false;
					}
				}
				continue;
			}
			if (c=='\r') {
				if (first+1<last && first[1]=='\n') {
					out.push_back('\r');
					out.push_back('\n');
					first+=2;
					continue;
				}
				error_message="stray carriage return in multi-line string";
				return false;
			}
			if (c=='\n' || c=='\t') {
				out.push_back(c);
				first++;
				continue;
			}
			if (static_cast<unsigned char>(c)<0x20 || c=='\x7F') {
				error_message="control character in multi-line string";
				return false;
			}
			out.push_back(c);
			first++;
		}
		error_message="unterminated multi-line string";
		return false;
	}
	static bool scan_ml_literal(const char* first,const char* const last,string_t& out,std::size_t& consumed,std::string& error_message) {
		const char* const begin=first;
		if (first<last && *first=='\r' && first+1<last && first[1]=='\n') first+=2;
		else if (first<last && *first=='\n') first++;
		while (first<last) {
			const char c=*first;
			if (c=='\'') {
				std::size_t run=0;
				while (first+run<last && first[run]=='\'') run++;
				if (run>=3) {
					if (run>5) {
						error_message="too many quotes at the end of a multi-line literal string";
						return false;
					}
					for (std::size_t i=0;i<run-3;i++) out.push_back('\'');
					consumed=static_cast<std::size_t>(first-begin)+run;
					return true;
				}
				for (std::size_t i=0;i<run;i++) out.push_back('\'');
				first+=run;
				continue;
			}
			if (c=='\r') {
				if (first+1<last && first[1]=='\n') {
					out.push_back('\r');
					out.push_back('\n');
					first+=2;
					continue;
				}
				error_message="stray carriage return in multi-line literal string";
				return false;
			}
			if (c=='\n' || c=='\t') {
				out.push_back(c);
				first++;
				continue;
			}
			if (static_cast<unsigned char>(c)<0x20 || c=='\x7F') {
				error_message="control character in multi-line literal string";
				return false;
			}
			out.push_back(c);
			first++;
		}
		error_message="unterminated multi-line literal string";
		return false;
	}

	static std::string strip_underscores(const char* first,const char* last) {
		std::string result;
		result.reserve(static_cast<std::size_t>(last-first));
		for (;first<last;first++) {
			if (*first!='_') result.push_back(*first);
		}
		return result;
	}
	static bool leap_year(int year) noexcept {
		return (year%4==0 && year%100!=0) || year%400==0;
	}
	static int days_in_month(int year,int month) noexcept {
		static const int table[12]={31,28,31,30,31,30,31,31,30,31,30,31};
		if (month==2 && leap_year(year)) return 29;
		return table[month-1];
	}
	static int decode_pair(const char* p) noexcept {
		return (p[0]-'0')*10+(p[1]-'0');
	}
	static bool validate_time(const toml_datetime& value,std::string& error_message) {
		if (value.hour>23 || value.minute>59 || value.second>60) {
			error_message="time component out of range";
			return false;
		}
		return true;
	}
	static bool validate_date(const toml_datetime& value,std::string& error_message) {
		if (value.month<1 || value.month>12 || value.day<1 || value.day>days_in_month(value.year,value.month)) {
			error_message="date component out of range";
			return false;
		}
		return true;
	}
	static long fraction_to_nanoseconds(const char* first,const char* last) noexcept {
		long result=0;
		int digits=0;
		for (;first<last && digits<9;first++,digits++) result=result*10+(*first-'0');
		for (;digits<9;digits++) result*=10;
		return result;
	}

	static bool value_boundary(const char* p,const char* const last) noexcept {
		if (p==last) return true;
		const char c=*p;
		return c==' ' || c=='\t' || c=='\r' || c=='\n' || c==',' || c==']' || c=='}' || c=='#';
	}

	static bool tokenize(std::string_view input,std::vector<toml_token>& tokens,std::size_t& error_position,std::string& error_message) {
		const char* first=input.data();
		const char* const last=input.data()+input.size();
		if (last-first>=3 && static_cast<unsigned char>(first[0])==0xEF && static_cast<unsigned char>(first[1])==0xBB && static_cast<unsigned char>(first[2])==0xBF) first+=3;
		std::cmatch match;
		const auto flags=std::regex_constants::match_continuous;
		bool mode_key=true;
		std::vector<char> ctx;
		auto post_value=[&]{
			if (ctx.empty()) mode_key=false;
			else if (ctx.back()=='A') mode_key=false;
			else mode_key=true;
		};
		while (first<last) {
			const char c=*first;
			if (c=='#') {
				first++;
				while (first<last && *first!='\n' && *first!='\r') {
					const unsigned char uc=static_cast<unsigned char>(*first);
					if ((uc<0x20 && uc!='\t') || uc==0x7F) {
						error_position=static_cast<std::size_t>(first-input.data());
						error_message="control character in comment";
						return false;
					}
					first++;
				}
				continue;
			}
			if (c==' ' || c=='\t') {
				first++;
				continue;
			}
			if (c=='\r' || c=='\n') {
				error_position=static_cast<std::size_t>(first-input.data());
				if (c=='\r') {
					if (first+1>=last || first[1]!='\n') {
						error_message="stray carriage return";
						return false;
					}
					first+=2;
				} else first++;
				if (ctx.empty()) {
					toml_token token;
					token.symbol=TS_NEWLINE;
					token.position=error_position;
					tokens.push_back(std::move(token));
					mode_key=true;
				} else if (ctx.back()=='A') {
					//数组允许跨行,换行被抑制
				} else if (ctx.back()=='T') {
					error_message="newline is not allowed inside an inline table";
					return false;
				} else {
					error_message="newline is not allowed inside a table header";
					return false;
				}
				continue;
			}
			toml_token token;
			token.position=static_cast<std::size_t>(first-input.data());
			error_position=token.position;
			if (mode_key) {
				if (c=='[') {
					if (!ctx.empty()) {
						error_message="unexpected '[' in key position";
						return false;
					}
					if (first+1<last && first[1]=='[') {
						token.symbol=TS_ARRAY_TABLE_OPEN;
						ctx.push_back('D');
						first+=2;
					} else {
						token.symbol=TS_TABLE_OPEN;
						ctx.push_back('H');
						first++;
					}
				} else if (c==']') {
					if (!ctx.empty() && ctx.back()=='H') {
						token.symbol=TS_TABLE_CLOSE;
						ctx.pop_back();
						first++;
					} else if (!ctx.empty() && ctx.back()=='D') {
						if (first+1>=last || first[1]!=']') {
							error_message="expected ']]' to close an array of tables header";
							return false;
						}
						token.symbol=TS_ARRAY_TABLE_CLOSE;
						ctx.pop_back();
						first+=2;
					} else {
						error_message="unexpected ']'";
						return false;
					}
				} else if (c=='.') {
					token.symbol=TS_DOT;
					first++;
				} else if (c=='=') {
					token.symbol=TS_EQ;
					mode_key=false;
					first++;
				} else if (c=='}') {
					if (ctx.empty() || ctx.back()!='T') {
						error_message="unexpected '}'";
						return false;
					}
					token.symbol=TS_RBRACE;
					ctx.pop_back();
					first++;
					post_value();
				} else if (c==',') {
					if (ctx.empty() || ctx.back()!='T') {
						error_message="unexpected ','";
						return false;
					}
					token.symbol=TS_COMMA;
					first++;
				} else if (c=='"') {
					if (last-first>=3 && first[1]=='"' && first[2]=='"') {
						error_message="multi-line strings cannot be used as keys";
						return false;
					}
					if (!std::regex_search(first,last,match,basic_string_regex(),flags)) {
						error_message="malformed quoted key";
						return false;
					}
					token.symbol=TS_KEY;
					if (!decode_basic(match[0].first,match[0].second,token.text,error_message)) return false;
					first=match[0].second;
				} else if (c=='\'') {
					if (last-first>=3 && first[1]=='\'' && first[2]=='\'') {
						error_message="multi-line strings cannot be used as keys";
						return false;
					}
					if (!std::regex_search(first,last,match,literal_string_regex(),flags)) {
						error_message="malformed literal key";
						return false;
					}
					token.symbol=TS_KEY;
					token.text.assign(match[0].first+1,match[0].second-1);
					first=match[0].second;
				} else if (std::regex_search(first,last,match,bare_key_regex(),flags)) {
					token.symbol=TS_KEY;
					token.text.assign(match[0].first,match[0].second);
					first=match[0].second;
				} else {
					error_message=std::string("unexpected character '")+c+"' in key position";
					return false;
				}
			} else {
				if (c=='[') {
					token.symbol=TS_LBRACKET;
					ctx.push_back('A');
					first++;
				} else if (c==']') {
					if (ctx.empty() || ctx.back()!='A') {
						error_message="unexpected ']'";
						return false;
					}
					token.symbol=TS_RBRACKET;
					ctx.pop_back();
					first++;
					post_value();
				} else if (c=='{') {
					token.symbol=TS_LBRACE;
					ctx.push_back('T');
					mode_key=true;
					first++;
				} else if (c==',') {
					if (ctx.empty() || ctx.back()!='A') {
						error_message="unexpected ','";
						return false;
					}
					token.symbol=TS_COMMA;
					first++;
				} else if (c=='"') {
					token.symbol=TS_STRING;
					if (last-first>=3 && first[1]=='"' && first[2]=='"') {
						std::size_t consumed=0;
						if (!scan_ml_basic(first+3,last,token.text,consumed,error_message)) return false;
						first+=3+consumed;
					} else {
						if (!std::regex_search(first,last,match,basic_string_regex(),flags)) {
							error_message="malformed string";
							return false;
						}
						if (!decode_basic(match[0].first,match[0].second,token.text,error_message)) return false;
						first=match[0].second;
					}
					post_value();
				} else if (c=='\'') {
					token.symbol=TS_STRING;
					if (last-first>=3 && first[1]=='\'' && first[2]=='\'') {
						std::size_t consumed=0;
						if (!scan_ml_literal(first+3,last,token.text,consumed,error_message)) return false;
						first+=3+consumed;
					} else {
						if (!std::regex_search(first,last,match,literal_string_regex(),flags)) {
							error_message="malformed literal string";
							return false;
						}
						token.text.assign(match[0].first+1,match[0].second-1);
						first=match[0].second;
					}
					post_value();
				} else if (std::regex_search(first,last,match,datetime_regex(),flags)) {
					if (!value_boundary(match[0].second,last)) {
						error_message="invalid value";
						return false;
					}
					token.symbol=TS_DATETIME;
					token.text.assign(match[0].first,match[0].second);
					toml_datetime& moment=token.moment;
					if (match[12].matched) {
						moment.variant=TDV_LOCAL_TIME;
						moment.hour=decode_pair(match[12].first);
						moment.minute=decode_pair(match[13].first);
						moment.second=decode_pair(match[14].first);
						if (match[15].matched) moment.nanosecond=fraction_to_nanoseconds(match[15].first,match[15].second);
						if (!validate_time(moment,error_message)) return false;
					} else {
						moment.year=(match[1].first[0]-'0')*1000+(match[1].first[1]-'0')*100+(match[1].first[2]-'0')*10+(match[1].first[3]-'0');
						moment.month=decode_pair(match[2].first);
						moment.day=decode_pair(match[3].first);
						if (!validate_date(moment,error_message)) return false;
						if (match[4].matched) {
							moment.hour=decode_pair(match[4].first);
							moment.minute=decode_pair(match[5].first);
							moment.second=decode_pair(match[6].first);
							if (match[7].matched) moment.nanosecond=fraction_to_nanoseconds(match[7].first,match[7].second);
							if (!validate_time(moment,error_message)) return false;
							if (match[8].matched) {
								moment.variant=TDV_OFFSET_DATETIME;
								moment.offset_minutes=0;
							} else if (match[9].matched) {
								const int hours=decode_pair(match[10].first);
								const int minutes=decode_pair(match[11].first);
								if (hours>23 || minutes>59) {
									error_message="time offset out of range";
									return false;
								}
								moment.variant=TDV_OFFSET_DATETIME;
								moment.offset_minutes=(hours*60+minutes)*(*match[9].first=='-'?-1:1);
							} else moment.variant=TDV_LOCAL_DATETIME;
						} else moment.variant=TDV_LOCAL_DATE;
					}
					first=match[0].second;
					post_value();
				} else if (std::regex_search(first,last,match,prefixed_int_regex(),flags)) {
					if (!value_boundary(match[0].second,last)) {
						error_message="invalid value";
						return false;
					}
					token.symbol=TS_INT;
					token.text.assign(match[0].first,match[0].second);
					const std::string digits=strip_underscores(match[0].first+2,match[0].second);
					const int base=match[0].first[1]=='x'?16:(match[0].first[1]=='o'?8:2);
					errno=0;
					const long long value=std::strtoll(digits.c_str(),nullptr,base);
					if (errno==ERANGE || value<static_cast<long long>((std::numeric_limits<int_t>::min)()) || value>static_cast<long long>((std::numeric_limits<int_t>::max)())) {
						error_message="integer does not fit into int_t";
						return false;
					}
					token.integer=static_cast<int_t>(value);
					first=match[0].second;
					post_value();
				} else if (std::regex_search(first,last,match,float_regex(),flags)) {
					if (!value_boundary(match[0].second,last)) {
						error_message="invalid value";
						return false;
					}
					token.symbol=TS_FLOAT;
					token.text.assign(match[0].first,match[0].second);
					const char tail=*(match[0].second-1);
					if (tail=='f') token.floating=*match[0].first=='-'?-std::numeric_limits<float_t>::infinity():std::numeric_limits<float_t>::infinity();
					else if (tail=='n') token.floating=std::numeric_limits<float_t>::quiet_NaN();
					else {
						const std::string digits=strip_underscores(match[0].first,match[0].second);
						token.floating=static_cast<float_t>(std::strtod(digits.c_str(),nullptr));
					}
					first=match[0].second;
					post_value();
				} else if (std::regex_search(first,last,match,dec_int_regex(),flags)) {
					if (!value_boundary(match[0].second,last)) {
						error_message="invalid value";
						return false;
					}
					token.symbol=TS_INT;
					token.text.assign(match[0].first,match[0].second);
					const std::string digits=strip_underscores(match[0].first,match[0].second);
					errno=0;
					const long long value=std::strtoll(digits.c_str(),nullptr,10);
					if (errno==ERANGE || value<static_cast<long long>((std::numeric_limits<int_t>::min)()) || value>static_cast<long long>((std::numeric_limits<int_t>::max)())) {
						error_message="integer does not fit into int_t";
						return false;
					}
					token.integer=static_cast<int_t>(value);
					first=match[0].second;
					post_value();
				} else if (std::regex_search(first,last,match,bool_regex(),flags)) {
					if (!value_boundary(match[0].second,last)) {
						error_message="invalid value";
						return false;
					}
					token.symbol=TS_BOOL;
					token.boolean=static_cast<boolean_t>(*first=='t');
					token.text.assign(match[0].first,match[0].second);
					first=match[0].second;
					post_value();
				} else {
					error_message=std::string("unexpected character '")+c+"' in value position";
					return false;
				}
			}
			tokens.push_back(std::move(token));
		}
		if (!ctx.empty()) {
			error_position=input.size();
			error_message=ctx.back()=='A'?"unterminated array":(ctx.back()=='T'?"unterminated inline table":"unterminated table header");
			return false;
		}
		if (!tokens.empty() && tokens.back().symbol!=TS_NEWLINE) {
			toml_token newline_token;
			newline_token.symbol=TS_NEWLINE;
			newline_token.position=input.size();
			tokens.push_back(std::move(newline_token));
		}
		toml_token eof_token;
		eof_token.symbol=TS_EOF;
		eof_token.position=input.size();
		tokens.push_back(std::move(eof_token));
		return true;
	}

	using parser_t=syntax::parser<toml_symbol,toml_production>;

	static bool initialize_grammar(parser_t& target) {
		auto unit=[](toml_symbol left,std::initializer_list<toml_symbol> rights,toml_production id){
			return syntax::single_parser_unit<toml_symbol,toml_production>(left,rights,id);
		};
		target.units={
			unit(TS_START,{TS_TOML,TS_EOF},TP_START),
			unit(TS_TOML,{TS_LINE_SEQ},TP_TOML),
			unit(TS_LINE_SEQ,{TS_LINE},TP_LINES_FIRST),
			unit(TS_LINE_SEQ,{TS_LINE_SEQ,TS_LINE},TP_LINES_APPEND),
			unit(TS_LINE,{TS_NEWLINE},TP_LINE_BLANK),
			unit(TS_LINE,{TS_KEYVAL,TS_NEWLINE},TP_LINE_KEYVAL),
			unit(TS_LINE,{TS_TABLE,TS_NEWLINE},TP_LINE_TABLE),
			unit(TS_LINE,{TS_ARRAY_TABLE,TS_NEWLINE},TP_LINE_ARRAY_TABLE),
			unit(TS_TABLE,{TS_TABLE_OPEN,TS_KEYPATH,TS_TABLE_CLOSE},TP_TABLE),
			unit(TS_ARRAY_TABLE,{TS_ARRAY_TABLE_OPEN,TS_KEYPATH,TS_ARRAY_TABLE_CLOSE},TP_ARRAY_TABLE),
			unit(TS_KEYVAL,{TS_KEYPATH,TS_EQ,TS_VALUE},TP_KEYVAL),
			unit(TS_KEYPATH,{TS_KEY},TP_KEYPATH_FIRST),
			unit(TS_KEYPATH,{TS_KEYPATH,TS_DOT,TS_KEY},TP_KEYPATH_APPEND),
			unit(TS_VALUE,{TS_STRING},TP_VALUE_STRING),
			unit(TS_VALUE,{TS_INT},TP_VALUE_INT),
			unit(TS_VALUE,{TS_FLOAT},TP_VALUE_FLOAT),
			unit(TS_VALUE,{TS_BOOL},TP_VALUE_BOOL),
			unit(TS_VALUE,{TS_DATETIME},TP_VALUE_DATETIME),
			unit(TS_VALUE,{TS_ARRAY},TP_VALUE_ARRAY),
			unit(TS_VALUE,{TS_INLINE_TABLE},TP_VALUE_INLINE_TABLE),
			unit(TS_ARRAY,{TS_LBRACKET,TS_RBRACKET},TP_ARRAY_EMPTY),
			unit(TS_ARRAY,{TS_LBRACKET,TS_ELEMENTS,TS_RBRACKET},TP_ARRAY),
			unit(TS_ARRAY,{TS_LBRACKET,TS_ELEMENTS,TS_COMMA,TS_RBRACKET},TP_ARRAY_TRAILING),
			unit(TS_ELEMENTS,{TS_VALUE},TP_ELEMENTS_FIRST),
			unit(TS_ELEMENTS,{TS_ELEMENTS,TS_COMMA,TS_VALUE},TP_ELEMENTS_APPEND),
			unit(TS_INLINE_TABLE,{TS_LBRACE,TS_RBRACE},TP_INLINE_TABLE_EMPTY),
			unit(TS_INLINE_TABLE,{TS_LBRACE,TS_MEMBERS,TS_RBRACE},TP_INLINE_TABLE),
			unit(TS_MEMBERS,{TS_KEYVAL},TP_MEMBERS_FIRST),
			unit(TS_MEMBERS,{TS_MEMBERS,TS_COMMA,TS_KEYVAL},TP_MEMBERS_APPEND),
		};
		target.generate_parser();
		return true;
	}
	static parser_t& grammar() {
		static parser_t instance(TS_START,TS_EPSILON,TS_EOF);
		static const bool initialized=initialize_grammar(instance);
		static_cast<void>(initialized);
		return instance;
	}
	static std::mutex& grammar_mutex() {
		static std::mutex instance;
		return instance;
	}

	class toml_listener : public syntax::parser_listener<toml_symbol,toml_production> {
		std::vector<toml_token>* tokens_=nullptr;
		sax_t* sax_=nullptr;
		std::vector<string_t> path_;
		bool aborted_=false;
		bool failed_=false;

		void abort_check(bool keep_going) {
			if (!keep_going) aborted_=true;
		}

	public:
		void reset(std::vector<toml_token>& tokens,sax_t& sax) {
			tokens_=&tokens;
			sax_=&sax;
			path_.clear();
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
		intptr_t on_shift(uintptr_t id,int state,toml_symbol word) override {
			static_cast<void>(state);
			if (aborted_ || failed_) return 0;
			static_cast<void>((*tokens_)[id-1]);
			switch (word) {
				case TS_EQ: abort_check(sax_->key(path_));break;
				case TS_LBRACKET: abort_check(sax_->start_array(static_cast<std::size_t>(-1)));break;
				case TS_LBRACE: abort_check(sax_->start_inline_table(static_cast<std::size_t>(-1)));break;
				default: break;
			}
			return 0;
		}
		intptr_t on_reduction(uintptr_t id,int state,int next,toml_production sentence_id,int reduction_num) override {
			static_cast<void>(state);
			static_cast<void>(next);
			static_cast<void>(reduction_num);
			if (aborted_ || failed_) return 0;
			toml_token& token=(*tokens_)[id-2];
			switch (sentence_id) {
				case TP_KEYPATH_FIRST: {
					path_.clear();
					path_.push_back(token.text);
					break;
				}
				case TP_KEYPATH_APPEND: path_.push_back(token.text);break;
				case TP_TABLE: abort_check(sax_->table(path_));break;
				case TP_ARRAY_TABLE: abort_check(sax_->array_table(path_));break;
				case TP_VALUE_STRING: abort_check(sax_->string(token.text));break;
				case TP_VALUE_INT: abort_check(sax_->number_integer(token.integer));break;
				case TP_VALUE_FLOAT: abort_check(sax_->number_float(token.floating,token.text));break;
				case TP_VALUE_BOOL: abort_check(sax_->boolean(token.boolean));break;
				case TP_VALUE_DATETIME: abort_check(sax_->datetime(token.moment));break;
				case TP_ARRAY_EMPTY:
				case TP_ARRAY:
				case TP_ARRAY_TRAILING: abort_check(sax_->end_array());break;
				case TP_INLINE_TABLE_EMPTY:
				case TP_INLINE_TABLE: abort_check(sax_->end_inline_table());break;
				case TP_START:
				case TP_TOML:
				case TP_LINES_FIRST:
				case TP_LINES_APPEND:
				case TP_LINE_BLANK:
				case TP_LINE_KEYVAL:
				case TP_LINE_TABLE:
				case TP_LINE_ARRAY_TABLE:
				case TP_KEYVAL:
				case TP_VALUE_ARRAY:
				case TP_VALUE_INLINE_TABLE:
				case TP_ELEMENTS_FIRST:
				case TP_ELEMENTS_APPEND:
				case TP_MEMBERS_FIRST:
				case TP_MEMBERS_APPEND:
				default: break;
			}
			return 0;
		}
		void on_accept() override { }
		int on_error(uintptr_t id,typename syntax::parser_listener<toml_symbol,toml_production>::error_type type,int state,toml_symbol word) override {
			static_cast<void>(type);
			static_cast<void>(state);
			static_cast<void>(word);
			failed_=true;
			if (sax_ && tokens_ && id!=static_cast<uintptr_t>(-1) && id>=1 && id<=tokens_->size()) {
				const toml_token& token=(*tokens_)[id-1];
				const std::string text(token.text.begin(),token.text.end());
				sax_->parse_error(token.position,text,"Unexpected token");
			} else if (sax_) sax_->parse_error(0,std::string(),"Unexpected end of input");
			return 0;
		}
	};

public:
	static bool sax_parse(std::string_view input,sax_t* sax) {
		std::vector<toml_token> tokens;
		std::size_t error_position=0;
		std::string error_message;
		if (!tokenize(input,tokens,error_position,error_message)) {
			sax->parse_error(error_position,std::string(),error_message);
			return false;
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
		static toml_listener listener;
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
	static toml parse(std::string_view input,bool allow_exceptions=true) {
		toml result;
		toml_sax_dom_builder<toml> builder(result);
		const bool ok=sax_parse(input,&builder) && builder.completed();
		if (!ok) {
			if (allow_exceptions) throw std::runtime_error(std::string("Parse error at byte ")+std::to_string(builder.error_position())+std::string(": ")+(builder.error_message().empty()?std::string("Incomplete document"):builder.error_message()));
			return toml();
		}
		return result;
	}
	static bool try_parse(std::string_view input,toml& out) {
		toml result;
		toml_sax_dom_builder<toml> builder(result);
		if (!sax_parse(input,&builder) || !builder.completed()) return false;
		out=std::move(result);
		return true;
	}
	static bool accept(std::string_view input) {
		toml_sax_acceptor<toml> acceptor;
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
	static void dump_hex32(string_t& out,unsigned long cp) {
		static const char digits[]="0123456789abcdef";
		out.push_back('\\');
		out.push_back('U');
		for (int shift=28;shift>=0;shift-=4) out.push_back(digits[(cp>>shift)&0xF]);
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
				case '\t': out.push_back('\t');first++;continue;
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
			} else if (cp<0x10000) {
				dump_hex16(out,cp);
			} else {
				dump_hex32(out,cp);
			}
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
	static void dump_integer(string_t& out,int_t value) {
		const std::string text=std::to_string(static_cast<long long>(value));
		out.append(text.begin(),text.end());
	}
	static void dump_floating(string_t& out,float_t value) {
		if (std::isnan(value)) {
			out.append({'n','a','n'});
			return;
		}
		if (std::isinf(value)) {
			if (value<0) out.push_back('-');
			out.append({'i','n','f'});
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
	static void format_datetime(string_t& out,const toml_datetime& value) {
		char buffer[40];
		if (value.has_date()) {
			const int length=std::snprintf(buffer,sizeof(buffer),"%04d-%02d-%02d",value.year,value.month,value.day);
			out.append(buffer,buffer+length);
			if (value.has_time()) out.push_back('T');
		}
		if (value.has_time()) {
			const int length=std::snprintf(buffer,sizeof(buffer),"%02d:%02d:%02d",value.hour,value.minute,value.second);
			out.append(buffer,buffer+length);
			if (value.nanosecond) {
				int digits=std::snprintf(buffer,sizeof(buffer),"%09ld",value.nanosecond);
				while (digits>1 && buffer[digits-1]=='0') digits--;
				out.push_back('.');
				out.append(buffer,buffer+digits);
			}
		}
		if (value.has_offset()) {
			if (!value.offset_minutes) out.push_back('Z');
			else {
				const int total=value.offset_minutes<0?-value.offset_minutes:value.offset_minutes;
				const int length=std::snprintf(buffer,sizeof(buffer),"%c%02d:%02d",value.offset_minutes<0?'-':'+',total/60,total%60);
				out.append(buffer,buffer+length);
			}
		}
	}
	static bool is_section_array(const base_t& node) {
		if (node.type()!=structure::DDT_ARRAY || node.empty()) return false;
		for (auto it=node.cbegin();it!=node.cend();it++) {
			if (it->type()!=structure::DDT_OBJECT) return false;
		}
		return true;
	}
	static void dump_value(const base_t& node,string_t& out,bool ensure_ascii) {
		switch (node.type()) {
			case structure::DDT_NULL: throw std::invalid_argument("toml has no null value; remove the node or convert it first");
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
				out.push_back('[');
				for (auto it=node.cbegin();it!=node.cend();) {
					dump_value(*it,out,ensure_ascii);
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
				if (node.empty()) {
					out.push_back('{');
					out.push_back('}');
					break;
				}
				out.push_back('{');
				out.push_back(' ');
				for (auto it=node.cbegin();it!=node.cend();) {
					render_key(out,it.key(),ensure_ascii);
					out.append({' ','=',' '});
					dump_value(*it,out,ensure_ascii);
					it++;
					if (it!=node.cend()) {
						out.push_back(',');
						out.push_back(' ');
					}
				}
				out.push_back(' ');
				out.push_back('}');
				break;
			}
			default: {
				if (is_datetime(node)) {
					format_datetime(out,get_datetime(node));
					break;
				}
				throw std::invalid_argument("toml: unsupported node type "+std::to_string(static_cast<long long>(static_cast<int>(node.type()))));
			}
		}
	}
	static void dump_indent(string_t& out,std::size_t count,typename string_t::value_type indent_char) {
		for (std::size_t i=0;i<count;i++) out.push_back(indent_char);
	}
	static void dump_table(const base_t& node,string_t& out,const string_t& header,std::size_t depth,int indent_step,typename string_t::value_type indent_char,bool ensure_ascii) {
		const std::size_t prefix=indent_step>0?depth*static_cast<std::size_t>(indent_step):0;
		for (auto it=node.cbegin();it!=node.cend();it++) {
			const base_t& child=*it;
			if (child.type()==structure::DDT_OBJECT || is_section_array(child)) continue;
			dump_indent(out,prefix,indent_char);
			render_key(out,it.key(),ensure_ascii);
			out.append({' ','=',' '});
			dump_value(child,out,ensure_ascii);
			out.push_back('\n');
		}
		for (auto it=node.cbegin();it!=node.cend();it++) {
			const base_t& child=*it;
			const bool section_array=is_section_array(child);
			if (child.type()!=structure::DDT_OBJECT && !section_array) continue;
			string_t path=header;
			if (!path.empty()) path.push_back('.');
			render_key(path,it.key(),ensure_ascii);
			if (!section_array) {
				if (!out.empty()) out.push_back('\n');
				dump_indent(out,prefix,indent_char);
				out.push_back('[');
				out.append(path);
				out.push_back(']');
				out.push_back('\n');
				dump_table(child,out,path,depth+1,indent_step,indent_char,ensure_ascii);
			} else {
				for (auto element=child.cbegin();element!=child.cend();element++) {
					if (!out.empty()) out.push_back('\n');
					dump_indent(out,prefix,indent_char);
					out.push_back('[');
					out.push_back('[');
					out.append(path);
					out.push_back(']');
					out.push_back(']');
					out.push_back('\n');
					dump_table(*element,out,path,depth+1,indent_step,indent_char,ensure_ascii);
				}
			}
		}
	}

public:
	virtual string_t dump(int indent=-1,typename string_t::value_type indent_char=' ',bool ensure_ascii=false) const {
		if (!this->is_object()) throw std::invalid_argument("toml: top-level value must be a table");
		string_t result;
		dump_table(*this,result,string_t(),0,indent,indent_char,ensure_ascii);
		return result;
	}

	friend std::ostream& operator <<(std::ostream& os,const toml& value) {
		const int indent_step=static_cast<int>(os.width());
		os.width(0);
		const string_t text=value.dump(indent_step>0?indent_step:-1,static_cast<typename string_t::value_type>(os.fill()));
		os.write(reinterpret_cast<const char*>(text.data()),static_cast<std::streamsize>(text.size()));
		return os;
	}
	friend std::istream& operator >>(std::istream& is,toml& value) {
		std::string content((std::istreambuf_iterator<char>(is)),std::istreambuf_iterator<char>());
		value=parse(content);
		return is;
	}
};

_STDEX_DOM_TPL_DEFAULT_DECLARATION
inline typename toml<_Int,_Float,_Boolean,_String,_Array,_Object,_Allocator>::string_t to_string(const toml<_Int,_Float,_Boolean,_String,_Array,_Object,_Allocator>& value) {
	return value.dump();
}

}

_STDEX_DOM_TPL_DEFAULT_DECLARATION
using toml_t=basic_toml::toml<_Int,_Float,_Boolean,_String,_Array,_Object,_Allocator>;
using toml=toml_t<>;
using basic_toml::toml_sax;
using basic_toml::toml_sax_dom_builder;
using basic_toml::toml_sax_acceptor;
using basic_toml::to_string;
using basic_toml::toml_datetime;
using basic_toml::toml_datetime_variant;
using basic_toml::TDV_OFFSET_DATETIME;
using basic_toml::TDV_LOCAL_DATETIME;
using basic_toml::TDV_LOCAL_DATE;
using basic_toml::TDV_LOCAL_TIME;

inline namespace literals {

inline toml_t<> operator ""_toml(const char* s,std::size_t n) {
	return toml_t<>::parse(std::string_view(s,n));
}

}

}

}

#endif
