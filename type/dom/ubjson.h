//Last Modified At 2026/07/04
//@Version 1.0.0.0
#ifndef _STDEX_TYPE_DOM_UBJSON_H_
#define _STDEX_TYPE_DOM_UBJSON_H_ 1

#include <cstdint>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include "../../structure/binary_dom.h"//At Least 1.0

namespace stdex {

namespace type {

namespace basic_ubjson {

_STDEX_DERIVED_KIND(ubjson_data_type,structure::binary_data_type,_STDEX_KIND_AUTO_START,
	_STDEX_KIND_VALUE_AUTO(UBDT_HIGH_PRECISION)
)

_STDEX_DOM_TPL_DECLARATION
class ubjson : public structure::binary_dom<_Int,_Float,_Boolean,_String,_Array,_Object,_Allocator> {
public:
	using base_t=structure::binary_dom<_Int,_Float,_Boolean,_String,_Array,_Object,_Allocator>;
	using dom_t=typename base_t::base_t;
	using int_t=typename base_t::int_t;
	using float_t=typename base_t::float_t;
	using boolean_t=typename base_t::boolean_t;
	using string_t=typename base_t::string_t;
	using size_type=typename base_t::size_type;
	using binary_t=typename base_t::binary_t;

protected:
	struct high_precision_value : base_t::value_t {
		string_t number{};

		high_precision_value()=default;
		explicit high_precision_value(string_t literal) : number(std::move(literal)) { }
		~high_precision_value() override=default;

		high_precision_value(const high_precision_value& other) : base_t::value_t() , number(other.number) { }

		typename base_t::value_t* clone(structure::dom_data_type t) const override {
			if (t==UBDT_HIGH_PRECISION) return create_value<high_precision_value>(*this);
			return base_t::value_t::clone(t);
		}
		void destroy(structure::dom_data_type t) override {
			if (t==UBDT_HIGH_PRECISION) return;
			base_t::value_t::destroy(t);
		}
		void destroy_self(structure::dom_data_type t) override {
			this->destroy(t);
			_Allocator<high_precision_value> alloc;
			std::allocator_traits<_Allocator<high_precision_value>>::destroy(alloc,this);
			std::allocator_traits<_Allocator<high_precision_value>>::deallocate(alloc,this,1);
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
	static high_precision_value* high_precision_payload(const base_t& node) {
		high_precision_value* payload=node.data().value?dynamic_cast<high_precision_value*>(node.data().value):nullptr;
		if (node.type()!=ubjson_data_type(UBDT_HIGH_PRECISION) || !payload) throw std::invalid_argument("Node does not hold a ubjson high-precision payload");
		return payload;
	}

public:
	using base_t::base_t;
	using base_t::operator =;

	ubjson()=default;
	~ubjson() override=default;

	ubjson(const ubjson&)=default;
	ubjson(ubjson&&) noexcept=default;

	ubjson& operator =(const ubjson&)=default;
	ubjson& operator =(ubjson&&)=default;

	bool support(structure::dom_data_type t) const noexcept override {
		return base_t::support(t) || t==UBDT_HIGH_PRECISION;
	}

	static base_t make_high_precision(string_t literal) {
		base_t node;
		node.data()=typename base_t::data_t(structure::dom_data_type(UBDT_HIGH_PRECISION),create_value<high_precision_value>(std::move(literal)));
		return node;
	}
	static ubjson binary(binary_t data) {
		ubjson result;
		result.set_binary(std::move(data));
		return result;
	}
	static ubjson high_precision(string_t literal) {
		return ubjson(make_high_precision(std::move(literal)));
	}

	static bool is_high_precision(const base_t& node) noexcept {
		return node.type()==ubjson_data_type(UBDT_HIGH_PRECISION);
	}
	bool is_high_precision() const noexcept {
		return is_high_precision(*this);
	}

	static string_t& high_precision_content(const base_t& node) {
		return high_precision_payload(node)->number;
	}
	string_t& high_precision_content() {
		return high_precision_content(*this);
	}
	const string_t& high_precision_content() const {
		return high_precision_content(*this);
	}

	static void set_high_precision(base_t& node,string_t literal) {
		node.data()=typename base_t::data_t(structure::dom_data_type(UBDT_HIGH_PRECISION),create_value<high_precision_value>(std::move(literal)));
	}
	void set_high_precision(string_t literal) {
		set_high_precision(*this,std::move(literal));
	}

	static ubjson parse(const std::uint8_t* data,std::size_t size) {
		bitwise::bit_reader reader(data,size,bitwise::BO_MSBYTE);
		ubjson result(decode_value(reader));
		skip_no_op(reader);
		if (!reader.eof()) throw std::runtime_error("Trailing bytes after ubjson value at byte "+std::to_string(base_t::byte_position(reader)));
		return result;
	}
	static ubjson parse(const binary_t& data) {
		return parse(data.data(),data.size());
	}

	binary_t dump() const {
		bitwise::bit_writer writer(bitwise::BO_MSBYTE);
		encode_value(*this,writer);
		return std::move(writer).buffer();
	}

protected:
	//记法转换协议·源侧降级:先交由binary_dom处理BDT_BINARY;UBDT_HIGH_PRECISION降级为字符串
	//(保留字面量原文,零精度损失;数值化会引入舍入故不为默认)。如需其他形态请用convert_handler_t定制。
	bool degrade_unsupported(const base_t& source,base_t& replacement) const override {
		if (base_t::degrade_unsupported(source,replacement)) return true;
		if (source.type()!=ubjson_data_type(UBDT_HIGH_PRECISION)) return false;
		replacement=base_t(high_precision_payload(source)->number);
		return true;
	}

private:
	static void skip_no_op(bitwise::bit_reader& reader) {
		while (!reader.eof() && reader.peek_bits<std::uint8_t>(8)=='N') reader.read_u8();
	}
	static bool is_integer_marker(std::uint8_t marker) noexcept {
		return marker=='i' || marker=='U' || marker=='I' || marker=='l' || marker=='L';
	}
	static bool is_value_marker(std::uint8_t marker) noexcept {
		return marker=='Z' || marker=='T' || marker=='F' || marker=='C' || marker=='S' || marker=='H' || marker=='d' || marker=='D' || is_integer_marker(marker);
	}
	static std::int64_t read_integer(std::uint8_t marker,bitwise::bit_reader& reader,std::size_t start) {
		switch (marker) {
			case 'i': return static_cast<std::int8_t>(reader.read_u8());
			case 'U': return reader.read_u8();
			case 'I': return static_cast<std::int16_t>(reader.read_u16());
			case 'l': return static_cast<std::int32_t>(reader.read_u32());
			case 'L': return static_cast<std::int64_t>(reader.read_u64());
			default: throw std::runtime_error("Expected an integer marker at byte "+std::to_string(start));
		}
	}
	static std::size_t read_length(bitwise::bit_reader& reader) {
		const std::size_t start=base_t::byte_position(reader);
		const std::int64_t value=read_integer(reader.read_u8(),reader,start);
		if (value<0) throw std::runtime_error("Negative length at byte "+std::to_string(start));
		if (static_cast<std::uint64_t>(value)>reader.remaining_bits()/CHAR_BIT) throw std::runtime_error("Declared length exceeds input size at byte "+std::to_string(start));
		return static_cast<std::size_t>(value);
	}
	static dom_t decode_integer(std::uint8_t marker,bitwise::bit_reader& reader,std::size_t start) {
		const std::int64_t value=read_integer(marker,reader,start);
		if (value<static_cast<std::int64_t>((std::numeric_limits<int_t>::min)()) || value>static_cast<std::int64_t>((std::numeric_limits<int_t>::max)())) throw std::runtime_error("Integer does not fit into int_t at byte "+std::to_string(start));
		return dom_t(static_cast<int_t>(value));
	}
	static dom_t decode_with_marker(std::uint8_t marker,bitwise::bit_reader& reader) {
		const std::size_t start=base_t::byte_position(reader);
		switch (marker) {
			case 'Z': return dom_t(nullptr);
			case 'T': return dom_t(static_cast<boolean_t>(true));
			case 'F': return dom_t(static_cast<boolean_t>(false));
			case 'i':
			case 'U':
			case 'I':
			case 'l':
			case 'L': return decode_integer(marker,reader,start);
			case 'd': return dom_t(static_cast<float_t>(base_t::bits_to_f32(reader.read_u32())));
			case 'D': return dom_t(static_cast<float_t>(base_t::bits_to_f64(reader.read_u64())));
			case 'C': {//char:规范定义等价于长度1的字符串
				string_t result;
				result.push_back(static_cast<typename string_t::value_type>(reader.read_u8()));
				return dom_t(std::move(result));
			}
			case 'S': return dom_t(base_t::template read_block<string_t>(reader,read_length(reader)));
			case 'H': {
				dom_t node;
				set_high_precision(node,base_t::template read_block<string_t>(reader,read_length(reader)));
				return node;
			}
			case '[': return decode_array(reader);
			case '{': return decode_object(reader);
			default: throw std::runtime_error("Invalid ubjson type marker at byte "+std::to_string(start));
		}
	}
	static dom_t decode_value(bitwise::bit_reader& reader) {
		skip_no_op(reader);
		return decode_with_marker(reader.read_u8(),reader);
	}
	static string_t decode_key(bitwise::bit_reader& reader) {
		skip_no_op(reader);
		return base_t::template read_block<string_t>(reader,read_length(reader));
	}
	static void decode_container_head(bitwise::bit_reader& reader,bool& typed,std::uint8_t& element_type,bool& counted,std::size_t& count) {
		typed=false;
		counted=false;
		element_type=0;
		count=0;
		if (!reader.eof() && reader.peek_bits<std::uint8_t>(8)=='$') {
			reader.read_u8();
			const std::size_t start=base_t::byte_position(reader);
			element_type=reader.read_u8();
			if (!is_value_marker(element_type)) throw std::runtime_error("Invalid strongly typed container type at byte "+std::to_string(start));
			typed=true;
			const std::size_t count_start=base_t::byte_position(reader);
			if (reader.eof() || reader.read_u8()!='#') throw std::runtime_error("Strongly typed container requires a count at byte "+std::to_string(count_start));
			counted=true;
			count=read_length(reader);
			return;
		}
		if (!reader.eof() && reader.peek_bits<std::uint8_t>(8)=='#') {
			reader.read_u8();
			counted=true;
			count=read_length(reader);
		}
	}
	static dom_t decode_array(bitwise::bit_reader& reader) {
		bool typed=false;
		bool counted=false;
		std::uint8_t element_type=0;
		std::size_t count=0;
		decode_container_head(reader,typed,element_type,counted,count);
		dom_t result(structure::DDT_ARRAY);
		typename dom_t::array_t& array=*result.value().array;
		if (typed && element_type=='U') {
			dom_t node;
			base_t::set_binary(node,base_t::template read_block<binary_t>(reader,count));
			return node;
		}
		if (counted) {
			for (std::size_t i=0;i<count;i++) {
				if (typed) array.push_back(decode_with_marker(element_type,reader));
				else array.push_back(decode_value(reader));
			}
			return result;
		}
		while (true) {
			skip_no_op(reader);
			if (reader.peek_bits<std::uint8_t>(8)==']') {
				reader.read_u8();
				break;
			}
			array.push_back(decode_value(reader));
		}
		return result;
	}
	static dom_t decode_object(bitwise::bit_reader& reader) {
		bool typed=false;
		bool counted=false;
		std::uint8_t element_type=0;
		std::size_t count=0;
		decode_container_head(reader,typed,element_type,counted,count);
		dom_t result(structure::DDT_OBJECT);
		typename dom_t::object_t& object=*result.value().object;
		if (counted) {
			for (std::size_t i=0;i<count;i++) {
				string_t key=decode_key(reader);
				if (typed) object.emplace(std::move(key),decode_with_marker(element_type,reader));
				else object.emplace(std::move(key),decode_value(reader));
			}
			return result;
		}
		while (true) {
			skip_no_op(reader);
			if (reader.peek_bits<std::uint8_t>(8)=='}') {
				reader.read_u8();
				break;
			}
			string_t key=decode_key(reader);
			object.emplace(std::move(key),decode_value(reader));
		}
		return result;
	}

	static void write_length(std::size_t value,bitwise::bit_writer& writer) {
		if (value<=127) {
			writer.write_u8('i');
			writer.write_u8(static_cast<std::uint8_t>(value));
		} else if (value<=0xFF) {
			writer.write_u8('U');
			writer.write_u8(static_cast<std::uint8_t>(value));
		} else if (value<=0x7FFF) {
			writer.write_u8('I');
			writer.write_u16(static_cast<std::uint16_t>(value));
		} else if (value<=0x7FFFFFFF) {
			writer.write_u8('l');
			writer.write_u32(static_cast<std::uint32_t>(value));
		} else {
			writer.write_u8('L');
			writer.write_u64(static_cast<std::uint64_t>(value));
		}
	}
	static void encode_integer(int_t value,bitwise::bit_writer& writer) {
		const std::int64_t svalue=static_cast<std::int64_t>(value);
		if (svalue>=-128 && svalue<=127) {
			writer.write_u8('i');
			writer.write_u8(static_cast<std::uint8_t>(static_cast<std::int8_t>(svalue)));
		} else if (svalue>=0 && svalue<=255) {
			writer.write_u8('U');
			writer.write_u8(static_cast<std::uint8_t>(svalue));
		} else if (svalue>=-32768 && svalue<=32767) {
			writer.write_u8('I');
			writer.write_u16(static_cast<std::uint16_t>(static_cast<std::int16_t>(svalue)));
		} else if (svalue>=-2147483647LL-1 && svalue<=2147483647LL) {
			writer.write_u8('l');
			writer.write_u32(static_cast<std::uint32_t>(static_cast<std::int32_t>(svalue)));
		} else {
			writer.write_u8('L');
			writer.write_u64(static_cast<std::uint64_t>(svalue));
		}
	}
	static void encode_string_body(const string_t& value,bitwise::bit_writer& writer) {
		write_length(value.size(),writer);
		writer.write_bytes(value.data(),value.size());
	}
	static void encode_value(const dom_t& node,bitwise::bit_writer& writer) {
		switch (node.type()) {
			case structure::DDT_NULL: {
				writer.write_u8('Z');
				break;
			}
			case structure::DDT_BOOL: {
				writer.write_u8(node.value().boolean?'T':'F');
				break;
			}
			case structure::DDT_INT: {
				encode_integer(node.value().integer,writer);
				break;
			}
			case structure::DDT_FLOAT: {
				writer.write_u8('D');
				writer.write_u64(base_t::f64_to_bits(static_cast<double>(node.value().floating)));
				break;
			}
			case structure::DDT_STRING: {
				writer.write_u8('S');
				encode_string_body(*node.value().string,writer);
				break;
			}
			case structure::DDT_ARRAY: {
				writer.write_u8('[');
				for (auto it=node.cbegin();it!=node.cend();it++) encode_value(*it,writer);
				writer.write_u8(']');
				break;
			}
			case structure::DDT_OBJECT: {
				writer.write_u8('{');
				for (auto it=node.cbegin();it!=node.cend();it++) {
					encode_string_body(it.key(),writer);
					encode_value(*it,writer);
				}
				writer.write_u8('}');
				break;
			}
			default: {
				if (base_t::is_binary(node)) {//惯用形:[$U#<count><bytes>
					const binary_t& bytes=base_t::get_binary(node);
					writer.write_u8('[');
					writer.write_u8('$');
					writer.write_u8('U');
					writer.write_u8('#');
					write_length(bytes.size(),writer);
					writer.write_bytes(bytes.data(),bytes.size());
					break;
				}
				if (is_high_precision(node)) {
					writer.write_u8('H');
					encode_string_body(high_precision_payload(node)->number,writer);
					break;
				}
				throw std::invalid_argument("Unsupported node type "+std::to_string(static_cast<long long>(static_cast<int>(node.type()))));
			}
		}
	}
};

}

_STDEX_DOM_TPL_DEFAULT_DECLARATION
using ubjson_t=basic_ubjson::ubjson<_Int,_Float,_Boolean,_String,_Array,_Object,_Allocator>;
using ubjson=ubjson_t<>;

}

}

#endif
