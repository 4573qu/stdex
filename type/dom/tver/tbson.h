//Last Modified At 2026/06/12
//@Version 1.1.0.0
//修改记录(1.0.0.0->1.1.0.0):补全BSON 1.1原生类型——新增八种派生kind:BSDT_OBJECT_ID(0x07,12字节)、
//BSDT_DATETIME(0x09,UTC毫秒int64)、BSDT_REGEX(0x0B,pattern+options双cstring)、BSDT_CODE(0x0D,
//JavaScript代码串)、BSDT_TIMESTAMP(0x11,内部u64:高32位秒/低32位增量,按整体读写)、BSDT_DECIMAL128
//(0x13,16字节IEEE754-2008十进制)、BSDT_MIN_KEY(0xFF)/BSDT_MAX_KEY(0x7F,无载荷);按现行标准配齐
//载荷结构/工厂/判断/访问/改写/降级与编解码闭环。弃用类型策略:undefined(0x06)落为null,
//symbol(0x0E)落为字符串(有损),DBPointer(0x0C)与code with scope(0x0F)抛错;报错措辞按规则去前缀。
#ifndef _STDEX_TYPE_DOM_BSON_H_
#define _STDEX_TYPE_DOM_BSON_H_ 1

#include <array>
#include <climits>
#include <cstdint>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include "../../structure/binary_dom.h"//At Least 1.0

namespace stdex {

namespace type {

namespace basic_bson {

//BSON专有kind:自binary_data_type(BDT_BINARY)之后续号。
_STDEX_DERIVED_KIND(bson_data_type,structure::binary_data_type,_STDEX_KIND_AUTO_START,
	_STDEX_KIND_VALUE_AUTO(BSDT_OBJECT_ID)
	_STDEX_KIND_VALUE_AUTO(BSDT_DATETIME)
	_STDEX_KIND_VALUE_AUTO(BSDT_REGEX)
	_STDEX_KIND_VALUE_AUTO(BSDT_CODE)
	_STDEX_KIND_VALUE_AUTO(BSDT_TIMESTAMP)
	_STDEX_KIND_VALUE_AUTO(BSDT_DECIMAL128)
	_STDEX_KIND_VALUE_AUTO(BSDT_MIN_KEY)
	_STDEX_KIND_VALUE_AUTO(BSDT_MAX_KEY)
)

_STDEX_DOM_TPL_DECLARATION
class bson : public structure::binary_dom<_Int,_Float,_Boolean,_String,_Array,_Object,_Allocator> {
public:
	using base_t=structure::binary_dom<_Int,_Float,_Boolean,_String,_Array,_Object,_Allocator>;
	using dom_t=typename base_t::base_t;
	using int_t=typename base_t::int_t;
	using float_t=typename base_t::float_t;
	using boolean_t=typename base_t::boolean_t;
	using string_t=typename base_t::string_t;
	using size_type=typename base_t::size_type;
	using binary_t=typename base_t::binary_t;
	using object_id_t=std::array<std::uint8_t,12>;
	using decimal128_t=std::array<std::uint8_t,16>;

protected:
	//---载荷结构:value_t子类,clone/destroy/destroy_self保证拷贝保留动态类型与按真实大小回收---
	struct object_id_value : base_t::value_t {
		object_id_t bytes{};

		object_id_value()=default;
		explicit object_id_value(const object_id_t& id) noexcept : bytes(id) { }
		~object_id_value() override=default;

		object_id_value(const object_id_value& other) : base_t::value_t() , bytes(other.bytes) { }

		typename base_t::value_t* clone(structure::dom_data_type t) const override {
			if (t==BSDT_OBJECT_ID) return create_value<object_id_value>(*this);
			return base_t::value_t::clone(t);
		}
		void destroy(structure::dom_data_type t) override {
			if (t==BSDT_OBJECT_ID) return;
			base_t::value_t::destroy(t);
		}
		void destroy_self(structure::dom_data_type t) override {
			this->destroy(t);
			_Allocator<object_id_value> alloc;
			std::allocator_traits<_Allocator<object_id_value>>::destroy(alloc,this);
			std::allocator_traits<_Allocator<object_id_value>>::deallocate(alloc,this,1);
		}
	};
	struct datetime_value : base_t::value_t {
		std::int64_t milliseconds{};

		datetime_value()=default;
		explicit datetime_value(std::int64_t utc_milliseconds) noexcept : milliseconds(utc_milliseconds) { }
		~datetime_value() override=default;

		datetime_value(const datetime_value& other) : base_t::value_t() , milliseconds(other.milliseconds) { }

		typename base_t::value_t* clone(structure::dom_data_type t) const override {
			if (t==BSDT_DATETIME) return create_value<datetime_value>(*this);
			return base_t::value_t::clone(t);
		}
		void destroy(structure::dom_data_type t) override {
			if (t==BSDT_DATETIME) return;
			base_t::value_t::destroy(t);
		}
		void destroy_self(structure::dom_data_type t) override {
			this->destroy(t);
			_Allocator<datetime_value> alloc;
			std::allocator_traits<_Allocator<datetime_value>>::destroy(alloc,this);
			std::allocator_traits<_Allocator<datetime_value>>::deallocate(alloc,this,1);
		}
	};
	struct regex_value : base_t::value_t {
		string_t pattern{};
		string_t options{};

		regex_value()=default;
		regex_value(string_t regex_pattern,string_t regex_options) : pattern(std::move(regex_pattern)) , options(std::move(regex_options)) { }
		~regex_value() override=default;

		regex_value(const regex_value& other) : base_t::value_t() , pattern(other.pattern) , options(other.options) { }

		typename base_t::value_t* clone(structure::dom_data_type t) const override {
			if (t==BSDT_REGEX) return create_value<regex_value>(*this);
			return base_t::value_t::clone(t);
		}
		void destroy(structure::dom_data_type t) override {
			if (t==BSDT_REGEX) return;
			base_t::value_t::destroy(t);
		}
		void destroy_self(structure::dom_data_type t) override {
			this->destroy(t);
			_Allocator<regex_value> alloc;
			std::allocator_traits<_Allocator<regex_value>>::destroy(alloc,this);
			std::allocator_traits<_Allocator<regex_value>>::deallocate(alloc,this,1);
		}
	};
	struct code_value : base_t::value_t {
		string_t text{};

		code_value()=default;
		explicit code_value(string_t code_text) : text(std::move(code_text)) { }
		~code_value() override=default;

		code_value(const code_value& other) : base_t::value_t() , text(other.text) { }

		typename base_t::value_t* clone(structure::dom_data_type t) const override {
			if (t==BSDT_CODE) return create_value<code_value>(*this);
			return base_t::value_t::clone(t);
		}
		void destroy(structure::dom_data_type t) override {
			if (t==BSDT_CODE) return;
			base_t::value_t::destroy(t);
		}
		void destroy_self(structure::dom_data_type t) override {
			this->destroy(t);
			_Allocator<code_value> alloc;
			std::allocator_traits<_Allocator<code_value>>::destroy(alloc,this);
			std::allocator_traits<_Allocator<code_value>>::deallocate(alloc,this,1);
		}
	};
	struct timestamp_value : base_t::value_t {
		std::uint64_t stamp{};

		timestamp_value()=default;
		explicit timestamp_value(std::uint64_t timestamp) noexcept : stamp(timestamp) { }
		~timestamp_value() override=default;

		timestamp_value(const timestamp_value& other) : base_t::value_t() , stamp(other.stamp) { }

		typename base_t::value_t* clone(structure::dom_data_type t) const override {
			if (t==BSDT_TIMESTAMP) return create_value<timestamp_value>(*this);
			return base_t::value_t::clone(t);
		}
		void destroy(structure::dom_data_type t) override {
			if (t==BSDT_TIMESTAMP) return;
			base_t::value_t::destroy(t);
		}
		void destroy_self(structure::dom_data_type t) override {
			this->destroy(t);
			_Allocator<timestamp_value> alloc;
			std::allocator_traits<_Allocator<timestamp_value>>::destroy(alloc,this);
			std::allocator_traits<_Allocator<timestamp_value>>::deallocate(alloc,this,1);
		}
	};
	struct decimal128_value : base_t::value_t {
		decimal128_t bytes{};

		decimal128_value()=default;
		explicit decimal128_value(const decimal128_t& value) noexcept : bytes(value) { }
		~decimal128_value() override=default;

		decimal128_value(const decimal128_value& other) : base_t::value_t() , bytes(other.bytes) { }

		typename base_t::value_t* clone(structure::dom_data_type t) const override {
			if (t==BSDT_DECIMAL128) return create_value<decimal128_value>(*this);
			return base_t::value_t::clone(t);
		}
		void destroy(structure::dom_data_type t) override {
			if (t==BSDT_DECIMAL128) return;
			base_t::value_t::destroy(t);
		}
		void destroy_self(structure::dom_data_type t) override {
			this->destroy(t);
			_Allocator<decimal128_value> alloc;
			std::allocator_traits<_Allocator<decimal128_value>>::destroy(alloc,this);
			std::allocator_traits<_Allocator<decimal128_value>>::deallocate(alloc,this,1);
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
	//---载荷校验:type值与动态类型双重校验(并行派生分支的kind值可能重合,以dynamic_cast为准)---
	static object_id_value* object_id_payload(const base_t& node) {
		object_id_value* payload=node.data().value?dynamic_cast<object_id_value*>(node.data().value):nullptr;
		if (node.type()!=bson_data_type(BSDT_OBJECT_ID) || !payload) throw std::invalid_argument("Node does not hold a bson ObjectId payload");
		return payload;
	}
	static datetime_value* datetime_payload(const base_t& node) {
		datetime_value* payload=node.data().value?dynamic_cast<datetime_value*>(node.data().value):nullptr;
		if (node.type()!=bson_data_type(BSDT_DATETIME) || !payload) throw std::invalid_argument("Node does not hold a bson datetime payload");
		return payload;
	}
	static regex_value* regex_payload(const base_t& node) {
		regex_value* payload=node.data().value?dynamic_cast<regex_value*>(node.data().value):nullptr;
		if (node.type()!=bson_data_type(BSDT_REGEX) || !payload) throw std::invalid_argument("Node does not hold a bson regex payload");
		return payload;
	}
	static code_value* code_payload(const base_t& node) {
		code_value* payload=node.data().value?dynamic_cast<code_value*>(node.data().value):nullptr;
		if (node.type()!=bson_data_type(BSDT_CODE) || !payload) throw std::invalid_argument("Node does not hold a bson code payload");
		return payload;
	}
	static timestamp_value* timestamp_payload(const base_t& node) {
		timestamp_value* payload=node.data().value?dynamic_cast<timestamp_value*>(node.data().value):nullptr;
		if (node.type()!=bson_data_type(BSDT_TIMESTAMP) || !payload) throw std::invalid_argument("Node does not hold a bson timestamp payload");
		return payload;
	}
	static decimal128_value* decimal128_payload(const base_t& node) {
		decimal128_value* payload=node.data().value?dynamic_cast<decimal128_value*>(node.data().value):nullptr;
		if (node.type()!=bson_data_type(BSDT_DECIMAL128) || !payload) throw std::invalid_argument("Node does not hold a bson decimal128 payload");
		return payload;
	}
	static string_t make_string(const char* text) {
		return string_t(text,text+std::char_traits<char>::length(text));
	}
	static string_t hex_string(const std::uint8_t* data,std::size_t size) {
		static const char digits[]="0123456789abcdef";
		string_t result;
		for (std::size_t i=0;i<size;i++) {
			result.push_back(digits[data[i]>>4]);
			result.push_back(digits[data[i]&0x0F]);
		}
		return result;
	}

public:
	using base_t::base_t;
	using base_t::operator =;

	bson()=default;
	~bson() override=default;

	bson(const bson&)=default;
	bson(bson&&) noexcept=default;

	bson& operator =(const bson&)=default;
	bson& operator =(bson&&)=default;

	//支持集=binary_dom支持集+BSON八种专有kind。
	bool support(structure::dom_data_type t) const noexcept override {
		return base_t::support(t) || t==BSDT_OBJECT_ID || t==BSDT_DATETIME || t==BSDT_REGEX || t==BSDT_CODE || t==BSDT_TIMESTAMP || t==BSDT_DECIMAL128 || t==BSDT_MIN_KEY || t==BSDT_MAX_KEY;
	}

	//---工厂---
	static base_t make_object_id(const object_id_t& id) {
		base_t node;
		node.data()=typename base_t::data_t(structure::dom_data_type(BSDT_OBJECT_ID),create_value<object_id_value>(id));
		return node;
	}
	static base_t make_datetime(std::int64_t milliseconds) {
		base_t node;
		node.data()=typename base_t::data_t(structure::dom_data_type(BSDT_DATETIME),create_value<datetime_value>(milliseconds));
		return node;
	}
	static base_t make_regex(string_t pattern,string_t options) {
		base_t node;
		node.data()=typename base_t::data_t(structure::dom_data_type(BSDT_REGEX),create_value<regex_value>(std::move(pattern),std::move(options)));
		return node;
	}
	static base_t make_code(string_t text) {
		base_t node;
		node.data()=typename base_t::data_t(structure::dom_data_type(BSDT_CODE),create_value<code_value>(std::move(text)));
		return node;
	}
	static base_t make_timestamp(std::uint64_t stamp) {
		base_t node;
		node.data()=typename base_t::data_t(structure::dom_data_type(BSDT_TIMESTAMP),create_value<timestamp_value>(stamp));
		return node;
	}
	static base_t make_decimal128(const decimal128_t& value) {
		base_t node;
		node.data()=typename base_t::data_t(structure::dom_data_type(BSDT_DECIMAL128),create_value<decimal128_value>(value));
		return node;
	}
	static base_t make_min_key() {
		base_t node;
		node.data()=typename base_t::data_t(structure::dom_data_type(BSDT_MIN_KEY));
		return node;
	}
	static base_t make_max_key() {
		base_t node;
		node.data()=typename base_t::data_t(structure::dom_data_type(BSDT_MAX_KEY));
		return node;
	}
	static bson binary(binary_t data) {
		bson result;
		result.set_binary(std::move(data));
		return result;
	}
	static bson object_id(const object_id_t& id) {
		return bson(make_object_id(id));
	}
	static bson datetime(std::int64_t milliseconds) {
		return bson(make_datetime(milliseconds));
	}
	static bson regex(string_t pattern,string_t options=string_t()) {
		return bson(make_regex(std::move(pattern),std::move(options)));
	}
	static bson code(string_t text) {
		return bson(make_code(std::move(text)));
	}
	static bson timestamp(std::uint64_t stamp) {
		return bson(make_timestamp(stamp));
	}
	static bson decimal128(const decimal128_t& value) {
		return bson(make_decimal128(value));
	}
	static bson min_key() {
		return bson(make_min_key());
	}
	static bson max_key() {
		return bson(make_max_key());
	}

	//---判断---
	static bool is_object_id(const base_t& node) noexcept {
		return node.type()==bson_data_type(BSDT_OBJECT_ID);
	}
	static bool is_datetime(const base_t& node) noexcept {
		return node.type()==bson_data_type(BSDT_DATETIME);
	}
	static bool is_regex(const base_t& node) noexcept {
		return node.type()==bson_data_type(BSDT_REGEX);
	}
	static bool is_code(const base_t& node) noexcept {
		return node.type()==bson_data_type(BSDT_CODE);
	}
	static bool is_timestamp(const base_t& node) noexcept {
		return node.type()==bson_data_type(BSDT_TIMESTAMP);
	}
	static bool is_decimal128(const base_t& node) noexcept {
		return node.type()==bson_data_type(BSDT_DECIMAL128);
	}
	static bool is_min_key(const base_t& node) noexcept {
		return node.type()==bson_data_type(BSDT_MIN_KEY);
	}
	static bool is_max_key(const base_t& node) noexcept {
		return node.type()==bson_data_type(BSDT_MAX_KEY);
	}
	bool is_object_id() const noexcept {
		return is_object_id(*this);
	}
	bool is_datetime() const noexcept {
		return is_datetime(*this);
	}
	bool is_regex() const noexcept {
		return is_regex(*this);
	}
	bool is_code() const noexcept {
		return is_code(*this);
	}
	bool is_timestamp() const noexcept {
		return is_timestamp(*this);
	}
	bool is_decimal128() const noexcept {
		return is_decimal128(*this);
	}
	bool is_min_key() const noexcept {
		return is_min_key(*this);
	}
	bool is_max_key() const noexcept {
		return is_max_key(*this);
	}

	//---访问(工厂与访问器同名处依实参类别消歧;code因const char*会歧义,访问器按text/text_content先例命名code_content)---
	static object_id_t& object_id(const base_t& node) {
		return object_id_payload(node)->bytes;
	}
	static std::int64_t& datetime(const base_t& node) {
		return datetime_payload(node)->milliseconds;
	}
	static string_t& regex_pattern(const base_t& node) {
		return regex_payload(node)->pattern;
	}
	static string_t& regex_options(const base_t& node) {
		return regex_payload(node)->options;
	}
	static string_t& code_content(const base_t& node) {
		return code_payload(node)->text;
	}
	static std::uint64_t& timestamp(const base_t& node) {
		return timestamp_payload(node)->stamp;
	}
	static decimal128_t& decimal128(const base_t& node) {
		return decimal128_payload(node)->bytes;
	}
	object_id_t& object_id() {
		return object_id(*this);
	}
	const object_id_t& object_id() const {
		return object_id(*this);
	}
	std::int64_t& datetime() {
		return datetime(*this);
	}
	const std::int64_t& datetime() const {
		return datetime(*this);
	}
	string_t& regex_pattern() {
		return regex_pattern(*this);
	}
	const string_t& regex_pattern() const {
		return regex_pattern(*this);
	}
	string_t& regex_options() {
		return regex_options(*this);
	}
	const string_t& regex_options() const {
		return regex_options(*this);
	}
	string_t& code_content() {
		return code_content(*this);
	}
	const string_t& code_content() const {
		return code_content(*this);
	}
	std::uint64_t& timestamp() {
		return timestamp(*this);
	}
	const std::uint64_t& timestamp() const {
		return timestamp(*this);
	}
	decimal128_t& decimal128() {
		return decimal128(*this);
	}
	const decimal128_t& decimal128() const {
		return decimal128(*this);
	}

	//---改写---
	static void set_object_id(base_t& node,const object_id_t& id) {
		node.data()=typename base_t::data_t(structure::dom_data_type(BSDT_OBJECT_ID),create_value<object_id_value>(id));
	}
	static void set_datetime(base_t& node,std::int64_t milliseconds) {
		node.data()=typename base_t::data_t(structure::dom_data_type(BSDT_DATETIME),create_value<datetime_value>(milliseconds));
	}
	static void set_regex(base_t& node,string_t pattern,string_t options=string_t()) {
		node.data()=typename base_t::data_t(structure::dom_data_type(BSDT_REGEX),create_value<regex_value>(std::move(pattern),std::move(options)));
	}
	static void set_code(base_t& node,string_t text) {
		node.data()=typename base_t::data_t(structure::dom_data_type(BSDT_CODE),create_value<code_value>(std::move(text)));
	}
	static void set_timestamp(base_t& node,std::uint64_t stamp) {
		node.data()=typename base_t::data_t(structure::dom_data_type(BSDT_TIMESTAMP),create_value<timestamp_value>(stamp));
	}
	static void set_decimal128(base_t& node,const decimal128_t& value) {
		node.data()=typename base_t::data_t(structure::dom_data_type(BSDT_DECIMAL128),create_value<decimal128_value>(value));
	}
	static void set_min_key(base_t& node) {
		node.data()=typename base_t::data_t(structure::dom_data_type(BSDT_MIN_KEY));
	}
	static void set_max_key(base_t& node) {
		node.data()=typename base_t::data_t(structure::dom_data_type(BSDT_MAX_KEY));
	}
	void set_object_id(const object_id_t& id) {
		set_object_id(*this,id);
	}
	void set_datetime(std::int64_t milliseconds) {
		set_datetime(*this,milliseconds);
	}
	void set_regex(string_t pattern,string_t options=string_t()) {
		set_regex(*this,std::move(pattern),std::move(options));
	}
	void set_code(string_t text) {
		set_code(*this,std::move(text));
	}
	void set_timestamp(std::uint64_t stamp) {
		set_timestamp(*this,stamp);
	}
	void set_decimal128(const decimal128_t& value) {
		set_decimal128(*this,value);
	}
	void set_min_key() {
		set_min_key(*this);
	}
	void set_max_key() {
		set_max_key(*this);
	}

	//---解码---
	static bson parse(const std::uint8_t* data,std::size_t size) {
		bitwise::bit_reader reader(data,size,bitwise::BO_LSBYTE);
		bson result(decode_document(reader,false));
		if (!reader.eof()) throw std::runtime_error("Trailing bytes after bson document at byte "+std::to_string(base_t::byte_position(reader)));
		return result;
	}
	static bson parse(const binary_t& data) {
		return parse(data.data(),data.size());
	}

	//---编码---
	binary_t dump() const {
		if (!this->is_object()) throw std::invalid_argument("Top-level value must be an object");
		bitwise::bit_writer writer(bitwise::BO_LSBYTE);
		encode_document(*this,writer);
		return std::move(writer).buffer();
	}

protected:
	//记法转换协议·源侧降级:先交由binary_dom处理BDT_BINARY;ObjectId/decimal128→十六进制字符串;
	//datetime/timestamp→int_t可容纳则整数、否则十进制字符串;regex→{"pattern","options"}对象;
	//code→字符串;min/max key→null(仅为排序哨兵,无数据可保)。如需其他形态请用convert_handler_t定制。
	bool degrade_unsupported(const base_t& source,base_t& replacement) const override {
		if (base_t::degrade_unsupported(source,replacement)) return true;
		if (source.type()==bson_data_type(BSDT_OBJECT_ID)) {
			const object_id_t& bytes=object_id_payload(source)->bytes;
			replacement=base_t(hex_string(bytes.data(),bytes.size()));
			return true;
		}
		if (source.type()==bson_data_type(BSDT_DATETIME)) {
			const std::int64_t value=datetime_payload(source)->milliseconds;
			if (value>=static_cast<std::int64_t>((std::numeric_limits<int_t>::min)()) && value<=static_cast<std::int64_t>((std::numeric_limits<int_t>::max)())) replacement=base_t(static_cast<int_t>(value));
			else {
				const std::string text=std::to_string(static_cast<long long>(value));
				replacement=base_t(string_t(text.begin(),text.end()));
			}
			return true;
		}
		if (source.type()==bson_data_type(BSDT_REGEX)) {
			const regex_value* payload=regex_payload(source);
			replacement=base_t(structure::DDT_OBJECT);
			replacement.value().object->emplace(make_string("pattern"),base_t(payload->pattern));
			replacement.value().object->emplace(make_string("options"),base_t(payload->options));
			return true;
		}
		if (source.type()==bson_data_type(BSDT_CODE)) {
			replacement=base_t(code_payload(source)->text);
			return true;
		}
		if (source.type()==bson_data_type(BSDT_TIMESTAMP)) {
			const std::uint64_t value=timestamp_payload(source)->stamp;
			if (value<=static_cast<std::uint64_t>((std::numeric_limits<int_t>::max)())) replacement=base_t(static_cast<int_t>(value));
			else {
				const std::string text=std::to_string(static_cast<unsigned long long>(value));
				replacement=base_t(string_t(text.begin(),text.end()));
			}
			return true;
		}
		if (source.type()==bson_data_type(BSDT_DECIMAL128)) {
			const decimal128_t& bytes=decimal128_payload(source)->bytes;
			replacement=base_t(hex_string(bytes.data(),bytes.size()));
			return true;
		}
		if (source.type()==bson_data_type(BSDT_MIN_KEY) || source.type()==bson_data_type(BSDT_MAX_KEY)) {
			replacement=base_t();
			return true;
		}
		return false;
	}

private:
	static string_t read_cstring(bitwise::bit_reader& reader) {
		string_t result;
		while (true) {
			const std::uint8_t byte=reader.read_u8();
			if (byte==0x00) break;
			result.push_back(static_cast<typename string_t::value_type>(byte));
		}
		return result;
	}
	static string_t read_string(bitwise::bit_reader& reader) {
		const std::size_t start=base_t::byte_position(reader);
		const std::uint32_t length=reader.read_u32();
		if (length<1) throw std::runtime_error("Invalid string length at byte "+std::to_string(start));
		string_t result=base_t::template read_block<string_t>(reader,length);
		if (result[length-1]!=typename string_t::value_type(0)) throw std::runtime_error("String is not null-terminated at byte "+std::to_string(start));
		result.resize(length-1);
		return result;
	}
	static dom_t decode_element(std::uint8_t element_type,bitwise::bit_reader& reader) {
		const std::size_t start=base_t::byte_position(reader);
		switch (element_type) {
			case 0x01: return dom_t(static_cast<float_t>(base_t::bits_to_f64(reader.read_u64())));
			case 0x02: return dom_t(read_string(reader));
			case 0x03: return decode_document(reader,false);
			case 0x04: return decode_document(reader,true);
			case 0x05: {
				const std::uint32_t length=reader.read_u32();
				reader.read_u8();//子类型字节:丢弃,载荷保留
				dom_t node;
				base_t::set_binary(node,base_t::template read_block<binary_t>(reader,length));
				return node;
			}
			case 0x06: return dom_t(nullptr);//undefined(弃用):落为null
			case 0x07: {
				base_t::require_bytes(reader,12);
				object_id_t id{};
				reader.read_bytes(id.data(),id.size());
				dom_t node;
				set_object_id(node,id);
				return node;
			}
			case 0x08: return dom_t(static_cast<boolean_t>(reader.read_u8()!=0));
			case 0x09: {
				dom_t node;
				set_datetime(node,static_cast<std::int64_t>(reader.read_u64()));
				return node;
			}
			case 0x0A: return dom_t(nullptr);
			case 0x0B: {
				string_t pattern=read_cstring(reader);
				string_t options=read_cstring(reader);
				dom_t node;
				set_regex(node,std::move(pattern),std::move(options));
				return node;
			}
			case 0x0D: {
				dom_t node;
				set_code(node,read_string(reader));
				return node;
			}
			case 0x0E: return dom_t(read_string(reader));//symbol(弃用):落为字符串(有损)
			case 0x10: return decode_signed(static_cast<std::int32_t>(reader.read_u32()),start);
			case 0x11: {
				dom_t node;
				set_timestamp(node,reader.read_u64());
				return node;
			}
			case 0x12: return decode_signed(static_cast<std::int64_t>(reader.read_u64()),start);
			case 0x13: {
				base_t::require_bytes(reader,16);
				decimal128_t value{};
				reader.read_bytes(value.data(),value.size());
				dom_t node;
				set_decimal128(node,value);
				return node;
			}
			case 0x7F: {
				dom_t node;
				set_max_key(node);
				return node;
			}
			case 0xFF: {
				dom_t node;
				set_min_key(node);
				return node;
			}
			default: throw std::runtime_error("Unsupported bson element type 0x"+to_hex(element_type)+" at byte "+std::to_string(start));
		}
	}
	static dom_t decode_signed(std::int64_t value,std::size_t start) {
		if (value<static_cast<std::int64_t>((std::numeric_limits<int_t>::min)()) || value>static_cast<std::int64_t>((std::numeric_limits<int_t>::max)())) throw std::runtime_error("Integer does not fit into int_t at byte "+std::to_string(start));
		return dom_t(static_cast<int_t>(value));
	}
	static dom_t decode_document(bitwise::bit_reader& reader,bool as_array) {
		const std::size_t start=base_t::byte_position(reader);
		const std::uint32_t total=reader.read_u32();
		if (total<5) throw std::runtime_error("Invalid document size at byte "+std::to_string(start));
		dom_t result(as_array?structure::dom_data_type(structure::DDT_ARRAY):structure::dom_data_type(structure::DDT_OBJECT));
		while (true) {
			const std::uint8_t element_type=reader.read_u8();
			if (element_type==0x00) break;
			string_t name=read_cstring(reader);
			dom_t value=decode_element(element_type,reader);
			if (as_array) result.value().array->push_back(std::move(value));
			else result.value().object->emplace(std::move(name),std::move(value));
		}
		if (base_t::byte_position(reader)-start!=total) throw std::runtime_error("Document size mismatch at byte "+std::to_string(base_t::byte_position(reader)));
		return result;
	}
	static std::string to_hex(std::uint8_t value) {
		static const char digits[]="0123456789ABCDEF";
		std::string result;
		result.push_back(digits[value>>4]);
		result.push_back(digits[value&0x0F]);
		return result;
	}

	static void write_cstring(const string_t& value,bitwise::bit_writer& writer) {
		for (typename string_t::value_type it:value) {
			if (it==typename string_t::value_type(0)) throw std::invalid_argument("Cstring field must not contain U+0000");
		}
		writer.write_bytes(value.data(),value.size());
		writer.write_u8(0x00);
	}
	static void write_name(const string_t& key,bitwise::bit_writer& writer) {
		for (typename string_t::value_type it:key) {
			if (it==typename string_t::value_type(0)) throw std::invalid_argument("Object key must not contain U+0000");
		}
		writer.write_bytes(key.data(),key.size());
		writer.write_u8(0x00);
	}
	static void encode_element(const string_t& key,const dom_t& node,bitwise::bit_writer& writer) {
		switch (node.type()) {
			case structure::DDT_NULL: {
				writer.write_u8(0x0A);
				write_name(key,writer);
				break;
			}
			case structure::DDT_BOOL: {
				writer.write_u8(0x08);
				write_name(key,writer);
				writer.write_u8(node.value().boolean?0x01:0x00);
				break;
			}
			case structure::DDT_INT: {
				const std::int64_t value=static_cast<std::int64_t>(node.value().integer);
				if (value>=-2147483647LL-1 && value<=2147483647LL) {
					writer.write_u8(0x10);
					write_name(key,writer);
					writer.write_u32(static_cast<std::uint32_t>(static_cast<std::int32_t>(value)));
				} else {
					writer.write_u8(0x12);
					write_name(key,writer);
					writer.write_u64(static_cast<std::uint64_t>(value));
				}
				break;
			}
			case structure::DDT_FLOAT: {
				writer.write_u8(0x01);
				write_name(key,writer);
				writer.write_u64(base_t::f64_to_bits(static_cast<double>(node.value().floating)));
				break;
			}
			case structure::DDT_STRING: {
				const string_t& value=*node.value().string;
				writer.write_u8(0x02);
				write_name(key,writer);
				writer.write_u32(static_cast<std::uint32_t>(value.size()+1));
				writer.write_bytes(value.data(),value.size());
				writer.write_u8(0x00);
				break;
			}
			case structure::DDT_ARRAY: {
				writer.write_u8(0x04);
				write_name(key,writer);
				encode_document(node,writer);
				break;
			}
			case structure::DDT_OBJECT: {
				writer.write_u8(0x03);
				write_name(key,writer);
				encode_document(node,writer);
				break;
			}
			default: {
				if (base_t::is_binary(node)) {
					const binary_t& bytes=base_t::get_binary(node);
					writer.write_u8(0x05);
					write_name(key,writer);
					writer.write_u32(static_cast<std::uint32_t>(bytes.size()));
					writer.write_u8(0x00);//子类型:generic
					writer.write_bytes(bytes.data(),bytes.size());
					break;
				}
				if (is_object_id(node)) {
					const object_id_t& bytes=object_id_payload(node)->bytes;
					writer.write_u8(0x07);
					write_name(key,writer);
					writer.write_bytes(bytes.data(),bytes.size());
					break;
				}
				if (is_datetime(node)) {
					writer.write_u8(0x09);
					write_name(key,writer);
					writer.write_u64(static_cast<std::uint64_t>(datetime_payload(node)->milliseconds));
					break;
				}
				if (is_regex(node)) {
					const regex_value* payload=regex_payload(node);
					writer.write_u8(0x0B);
					write_name(key,writer);
					write_cstring(payload->pattern,writer);
					write_cstring(payload->options,writer);
					break;
				}
				if (is_code(node)) {
					const string_t& text=code_payload(node)->text;
					writer.write_u8(0x0D);
					write_name(key,writer);
					writer.write_u32(static_cast<std::uint32_t>(text.size()+1));
					writer.write_bytes(text.data(),text.size());
					writer.write_u8(0x00);
					break;
				}
				if (is_timestamp(node)) {
					writer.write_u8(0x11);
					write_name(key,writer);
					writer.write_u64(timestamp_payload(node)->stamp);
					break;
				}
				if (is_decimal128(node)) {
					const decimal128_t& bytes=decimal128_payload(node)->bytes;
					writer.write_u8(0x13);
					write_name(key,writer);
					writer.write_bytes(bytes.data(),bytes.size());
					break;
				}
				if (is_min_key(node)) {
					writer.write_u8(0xFF);
					write_name(key,writer);
					break;
				}
				if (is_max_key(node)) {
					writer.write_u8(0x7F);
					write_name(key,writer);
					break;
				}
				throw std::invalid_argument("Unsupported node type "+std::to_string(static_cast<long long>(static_cast<int>(node.type()))));
			}
		}
	}
	static void encode_document(const dom_t& node,bitwise::bit_writer& writer) {
		const std::size_t size_position=writer.tell_bits();
		writer.write_u32(0);//总长占位:bit_writer按位或写入,先写零再回填
		if (node.type()==structure::DDT_OBJECT) {
			for (auto it=node.cbegin();it!=node.cend();it++) encode_element(it.key(),*it,writer);
		} else {
			std::size_t index=0;
			for (auto it=node.cbegin();it!=node.cend();it++) {
				const std::string text=std::to_string(index++);
				encode_element(string_t(text.begin(),text.end()),*it,writer);
			}
		}
		writer.write_u8(0x00);
		const std::size_t end_position=writer.tell_bits();
		writer.seek_bits(size_position);
		writer.write_u32(static_cast<std::uint32_t>((end_position-size_position)/CHAR_BIT));
		writer.seek_bits(end_position);
	}
};

}

_STDEX_DOM_TPL_DEFAULT_DECLARATION
using bson_t=basic_bson::bson<_Int,_Float,_Boolean,_String,_Array,_Object,_Allocator>;
using bson=bson_t<>;

}

}

#endif
