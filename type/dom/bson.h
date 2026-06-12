//Last Modified At 2026/06/11
//@Version 1.0.0.0
#ifndef _STDEX_TYPE_DOM_BSON_H_
#define _STDEX_TYPE_DOM_BSON_H_ 1

#include <climits>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

#include "../../structure/binary_dom.h"//At Least 1.0

namespace stdex {

namespace type {

namespace basic_bson {

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

	using base_t::base_t;
	using base_t::operator =;

	bson()=default;
	~bson() override=default;

	bson(const bson&)=default;
	bson(bson&&) noexcept=default;

	bson& operator =(const bson&)=default;
	bson& operator =(bson&&)=default;

	static bson binary(binary_t data) {
		bson result;
		result.set_binary(std::move(data));
		return result;
	}

	static bson parse(const std::uint8_t* data,std::size_t size) {
		bitwise::bit_reader reader(data,size,bitwise::BO_LSBYTE);
		bson result(decode_document(reader,false));
		if (!reader.eof()) throw std::runtime_error("Trailing bytes after bson document at byte "+std::to_string(base_t::byte_position(reader)));
		return result;
	}
	static bson parse(const binary_t& data) {
		return parse(data.data(),data.size());
	}

	binary_t dump() const {
		if (!this->is_object()) throw std::invalid_argument("bson: top-level value must be an object");
		bitwise::bit_writer writer(bitwise::BO_LSBYTE);
		encode_document(*this,writer);
		return std::move(writer).buffer();
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
				reader.read_u8();
				dom_t node;
				base_t::set_binary(node,base_t::template read_block<binary_t>(reader,length));
				return node;
			}
			case 0x08: return dom_t(static_cast<boolean_t>(reader.read_u8()!=0));
			case 0x0A: return dom_t(nullptr);
			case 0x10: return decode_signed(static_cast<std::int32_t>(reader.read_u32()),start);
			case 0x12: return decode_signed(static_cast<std::int64_t>(reader.read_u64()),start);
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

	static void write_name(const string_t& key,bitwise::bit_writer& writer) {
		for (typename string_t::value_type it:key) {
			if (it==typename string_t::value_type(0)) throw std::invalid_argument("bson: object key must not contain U+0000");
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
					writer.write_u8(0x00);
					writer.write_bytes(bytes.data(),bytes.size());
					break;
				}
				throw std::invalid_argument("Unsupported node type "+std::to_string(static_cast<long long>(static_cast<int>(node.type()))));
			}
		}
	}
	static void encode_document(const dom_t& node,bitwise::bit_writer& writer) {
		const std::size_t size_position=writer.tell_bits();
		writer.write_u32(0);
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

inline namespace literals {

inline bson_t<> operator ""_bson(const uint8_t* v,std::size_t n) {
	return bson_t<>::parse(v,n);
}

}

}

}

#endif
