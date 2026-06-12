//Last Modified At 2026/06/11
//@Version 1.0.0.0
#ifndef _STDEX_TYPE_DOM_MSGPACK_H_
#define _STDEX_TYPE_DOM_MSGPACK_H_ 1

#include <cstdint>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

#include "../../strcuture/binary_dom.h"//At Least 1.0

namespace stdex {

namespace type {

namespace basic_msgpack {

_STDEX_DOM_TPL_DECLARATION
class msgpack : public structure::binary_dom<_Int,_Float,_Boolean,_String,_Array,_Object,_Allocator> {
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

	msgpack()=default;
	~msgpack() override=default;

	msgpack(const msgpack&)=default;
	msgpack(msgpack&&) noexcept=default;

	msgpack& operator =(const msgpack&)=default;
	msgpack& operator =(msgpack&&)=default;

	static msgpack binary(binary_t data) {
		msgpack result;
		result.set_binary(std::move(data));
		return result;
	}

	static msgpack parse(const std::uint8_t* data,std::size_t size) {
		bitwise::bit_reader reader(data,size,bitwise::BO_MSBYTE);
		msgpack result(decode_value(reader));
		if (!reader.eof()) throw std::runtime_error("Trailing bytes after msgpack value at byte "+std::to_string(base_t::byte_position(reader)));
		return result;
	}
	static msgpack parse(const binary_t& data) {
		return parse(data.data(),data.size());
	}

	binary_t dump() const {
		bitwise::bit_writer writer(bitwise::BO_MSBYTE);
		encode_value(*this,writer);
		return std::move(writer).buffer();
	}

private:
	static dom_t decode_binary(bitwise::bit_reader& reader,std::size_t length) {
		dom_t node;
		base_t::set_binary(node,base_t::template read_block<binary_t>(reader,length));
		return node;
	}
	static string_t decode_key(bitwise::bit_reader& reader) {
		const std::size_t start=base_t::byte_position(reader);
		const std::uint8_t head=reader.read_u8();
		if (head>=0xA0 && head<=0xBF) return base_t::template read_block<string_t>(reader,head&0x1F);
		switch (head) {
			case 0xD9: return base_t::template read_block<string_t>(reader,reader.read_u8());
			case 0xDA: return base_t::template read_block<string_t>(reader,reader.read_u16());
			case 0xDB: return base_t::template read_block<string_t>(reader,reader.read_u32());
			default: throw std::runtime_error("msgpack object key must be a string at byte "+std::to_string(start));
		}
	}
	static dom_t decode_array(bitwise::bit_reader& reader,std::size_t count) {
		dom_t result(structure::DDT_ARRAY);
		typename dom_t::array_t& array=*result.value().array;
		for (std::size_t i=0;i<count;i++) array.push_back(decode_value(reader));
		return result;
	}
	static dom_t decode_map(bitwise::bit_reader& reader,std::size_t count) {
		dom_t result(structure::DDT_OBJECT);
		typename dom_t::object_t& object=*result.value().object;
		for (std::size_t i=0;i<count;i++) {
			string_t key=decode_key(reader);
			object.emplace(std::move(key),decode_value(reader));
		}
		return result;
	}
	static dom_t decode_unsigned(std::uint64_t value,std::size_t start) {
		if (value>static_cast<std::uint64_t>((std::numeric_limits<int_t>::max)())) throw std::runtime_error("Unsigned integer does not fit into int_t at byte "+std::to_string(start));
		return dom_t(static_cast<int_t>(value));
	}
	static dom_t decode_signed(std::int64_t value,std::size_t start) {
		if (value<static_cast<std::int64_t>((std::numeric_limits<int_t>::min)())||value>static_cast<std::int64_t>((std::numeric_limits<int_t>::max)())) throw std::runtime_error("Signed integer does not fit into int_t at byte "+std::to_string(start));
		return dom_t(static_cast<int_t>(value));
	}
	static dom_t decode_value(bitwise::bit_reader& reader) {
		const std::size_t start=base_t::byte_position(reader);
		const std::uint8_t head=reader.read_u8();
		if (head<=0x7F) return dom_t(static_cast<int_t>(head));//positive fixint
		if (head>=0xE0) return dom_t(static_cast<int_t>(static_cast<std::int8_t>(head)));//negative fixint
		if (head>=0x80 && head<=0x8F) return decode_map(reader,head&0x0F);//fixmap
		if (head>=0x90 && head<=0x9F) return decode_array(reader,head&0x0F);//fixarray
		if (head>=0xA0 && head<=0xBF) return dom_t(base_t::template read_block<string_t>(reader,head&0x1F));//fixstr
		switch (head) {
			case 0xC0: return dom_t(nullptr);
			case 0xC2: return dom_t(static_cast<boolean_t>(false));
			case 0xC3: return dom_t(static_cast<boolean_t>(true));
			case 0xC4: return decode_binary(reader,reader.read_u8());
			case 0xC5: return decode_binary(reader,reader.read_u16());
			case 0xC6: return decode_binary(reader,reader.read_u32());
			case 0xC7:
			case 0xC8:
			case 0xC9:
			case 0xD4:
			case 0xD5:
			case 0xD6:
			case 0xD7:
			case 0xD8: throw std::runtime_error("msgpack ext family is not supported by this data model at byte "+std::to_string(start));
			case 0xCA: return dom_t(static_cast<float_t>(base_t::bits_to_f32(reader.read_u32())));
			case 0xCB: return dom_t(static_cast<float_t>(base_t::bits_to_f64(reader.read_u64())));
			case 0xCC: return decode_unsigned(reader.read_u8(),start);
			case 0xCD: return decode_unsigned(reader.read_u16(),start);
			case 0xCE: return decode_unsigned(reader.read_u32(),start);
			case 0xCF: return decode_unsigned(reader.read_u64(),start);
			case 0xD0: return decode_signed(static_cast<std::int8_t>(reader.read_u8()),start);
			case 0xD1: return decode_signed(static_cast<std::int16_t>(reader.read_u16()),start);
			case 0xD2: return decode_signed(static_cast<std::int32_t>(reader.read_u32()),start);
			case 0xD3: return decode_signed(static_cast<std::int64_t>(reader.read_u64()),start);
			case 0xD9: return dom_t(base_t::template read_block<string_t>(reader,reader.read_u8()));
			case 0xDA: return dom_t(base_t::template read_block<string_t>(reader,reader.read_u16()));
			case 0xDB: return dom_t(base_t::template read_block<string_t>(reader,reader.read_u32()));
			case 0xDC: return decode_array(reader,reader.read_u16());
			case 0xDD: return decode_array(reader,reader.read_u32());
			case 0xDE: return decode_map(reader,reader.read_u16());
			case 0xDF: return decode_map(reader,reader.read_u32());
			default: throw std::runtime_error("Invalid msgpack type byte at byte "+std::to_string(start));
		}
	}

	static void encode_integer(int_t value,bitwise::bit_writer& writer) {
		if (value>=0) {
			const std::uint64_t uvalue=static_cast<std::uint64_t>(value);
			if (uvalue<=0x7F) writer.write_u8(static_cast<std::uint8_t>(uvalue));
			else if (uvalue<=0xFF) {
				writer.write_u8(0xCC);
				writer.write_u8(static_cast<std::uint8_t>(uvalue));
			} else if (uvalue<=0xFFFF) {
				writer.write_u8(0xCD);
				writer.write_u16(static_cast<std::uint16_t>(uvalue));
			} else if (uvalue<=0xFFFFFFFF) {
				writer.write_u8(0xCE);
				writer.write_u32(static_cast<std::uint32_t>(uvalue));
			} else {
				writer.write_u8(0xCF);
				writer.write_u64(uvalue);
			}
		} else {
			const std::int64_t svalue=static_cast<std::int64_t>(value);
			if (svalue>=-32) writer.write_u8(static_cast<std::uint8_t>(static_cast<std::int8_t>(svalue)));
			else if (svalue>=-128) {
				writer.write_u8(0xD0);
				writer.write_u8(static_cast<std::uint8_t>(static_cast<std::int8_t>(svalue)));
			} else if (svalue>=-32768) {
				writer.write_u8(0xD1);
				writer.write_u16(static_cast<std::uint16_t>(static_cast<std::int16_t>(svalue)));
			} else if (svalue>=-2147483647LL-1) {
				writer.write_u8(0xD2);
				writer.write_u32(static_cast<std::uint32_t>(static_cast<std::int32_t>(svalue)));
			} else {
				writer.write_u8(0xD3);
				writer.write_u64(static_cast<std::uint64_t>(svalue));
			}
		}
	}
	static void encode_string(const string_t& value,bitwise::bit_writer& writer) {
		const std::size_t length=value.size();
		if (length<=31) writer.write_u8(static_cast<std::uint8_t>(0xA0|length));
		else if (length<=0xFF) {
			writer.write_u8(0xD9);
			writer.write_u8(static_cast<std::uint8_t>(length));
		} else if (length<=0xFFFF) {
			writer.write_u8(0xDA);
			writer.write_u16(static_cast<std::uint16_t>(length));
		} else {
			writer.write_u8(0xDB);
			writer.write_u32(static_cast<std::uint32_t>(length));
		}
		writer.write_bytes(value.data(),length);
	}
	static void encode_value(const dom_t& node,bitwise::bit_writer& writer) {
		switch (node.type()) {
			case structure::DDT_NULL: {
				writer.write_u8(0xC0);
				break;
			}
			case structure::DDT_BOOL: {
				writer.write_u8(node.value().boolean?0xC3:0xC2);
				break;
			}
			case structure::DDT_INT: {
				encode_integer(node.value().integer,writer);
				break;
			}
			case structure::DDT_FLOAT: {
				writer.write_u8(0xCB);
				writer.write_u64(base_t::f64_to_bits(static_cast<double>(node.value().floating)));
				break;
			}
			case structure::DDT_STRING: {
				encode_string(*node.value().string,writer);
				break;
			}
			case structure::DDT_ARRAY: {
				const std::size_t count=node.size();
				if (count<=15) writer.write_u8(static_cast<std::uint8_t>(0x90|count));
				else if (count<=0xFFFF) {
					writer.write_u8(0xDC);
					writer.write_u16(static_cast<std::uint16_t>(count));
				} else {
					writer.write_u8(0xDD);
					writer.write_u32(static_cast<std::uint32_t>(count));
				}
				for (auto it=node.cbegin();it!=node.cend();it++) encode_value(*it,writer);
				break;
			}
			case structure::DDT_OBJECT: {
				const std::size_t count=node.size();
				if (count<=15) writer.write_u8(static_cast<std::uint8_t>(0x80|count));
				else if (count<=0xFFFF) {
					writer.write_u8(0xDE);
					writer.write_u16(static_cast<std::uint16_t>(count));
				} else {
					writer.write_u8(0xDF);
					writer.write_u32(static_cast<std::uint32_t>(count));
				}
				for (auto it=node.cbegin();it!=node.cend();it++) {
					encode_string(it.key(),writer);
					encode_value(*it,writer);
				}
				break;
			}
			default: {
				if (base_t::is_binary(node)) {
					const binary_t& bytes=base_t::get_binary(node);
					const std::size_t length=bytes.size();
					if (length<=0xFF) {
						writer.write_u8(0xC4);
						writer.write_u8(static_cast<std::uint8_t>(length));
					} else if (length<=0xFFFF) {
						writer.write_u8(0xC5);
						writer.write_u16(static_cast<std::uint16_t>(length));
					} else {
						writer.write_u8(0xC6);
						writer.write_u32(static_cast<std::uint32_t>(length));
					}
					writer.write_bytes(bytes.data(),length);
					break;
				}
				throw std::invalid_argument("msgpack: unsupported node type "+std::to_string(static_cast<long long>(static_cast<int>(node.type()))));
			}
		}
	}
};

}

_STDEX_DOM_TPL_DEFAULT_DECLARATION
using msgpack_t=basic_msgpack::msgpack<_Int,_Float,_Boolean,_String,_Array,_Object,_Allocator>;
using msgpack=msgpack_t<>;

inline namespace literals {

inline msgpack_t<> operator ""_msgpack(const uint8_t* v,std::size_t n) {
	return msgpack_t<>::parse(v,n);
}

}

}

}

#endif
