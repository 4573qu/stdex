//Last Modified At 2026/06/11
//@Version 1.0.0.0
#ifndef _STDEX_TYPE_DOM_CBOR_H_
#define _STDEX_TYPE_DOM_CBOR_H_ 1

#include <cmath>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

#include "../../structure/binary_dom.h"//At Least 1.0

namespace stdex {

namespace type {

namespace basic_cbor {

_STDEX_DOM_TPL_DECLARATION
class cbor : public structure::binary_dom<_Int,_Float,_Boolean,_String,_Array,_Object,_Allocator> {
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

	cbor()=default;
	~cbor() override=default;

	cbor(const cbor&)=default;
	cbor(cbor&&) noexcept=default;

	cbor& operator =(const cbor&)=default;
	cbor& operator =(cbor&&)=default;

	static cbor binary(binary_t data) {
		cbor result;
		result.set_binary(std::move(data));
		return result;
	}

	static cbor parse(const std::uint8_t* data,std::size_t size) {
		bitwise::bit_reader reader(data,size,bitwise::BO_MSBYTE);
		cbor result(decode_value(reader));
		if (!reader.eof()) throw std::runtime_error("Trailing bytes after cbor value at byte "+std::to_string(base_t::byte_position(reader)));
		return result;
	}
	static cbor parse(const binary_t& data) {
		return parse(data.data(),data.size());
	}

	binary_t dump() const {
		bitwise::bit_writer writer(bitwise::BO_MSBYTE);
		encode_value(*this,writer);
		return std::move(writer).buffer();
	}

private:
	static constexpr std::uint8_t indefinite_info=0x1F;
	static constexpr std::uint8_t break_byte=0xFF;

	static std::uint64_t read_count(bitwise::bit_reader& reader,std::uint8_t info,std::size_t start) {
		if (info<24) return info;
		switch (info) {
			case 24: return reader.read_u8();
			case 25: return reader.read_u16();
			case 26: return reader.read_u32();
			case 27: return reader.read_u64();
			default: throw std::runtime_error("Invalid additional information at byte "+std::to_string(start));
		}
	}
	static std::size_t checked_length(std::uint64_t count,const bitwise::bit_reader& reader,std::size_t start) {
		if (count>reader.remaining_bits()/CHAR_BIT) throw std::runtime_error("Declared length exceeds input size at byte "+std::to_string(start));
		return static_cast<std::size_t>(count);
	}
	static double decode_half(std::uint16_t half) {
		const int exponent=(half>>10)&0x1F;
		const int mantissa=half&0x3FF;
		double value;
		if (exponent==0) value=std::ldexp(mantissa,-24);
		else if (exponent!=31) value=std::ldexp(mantissa+1024,exponent-25);
		else value=mantissa==0?std::numeric_limits<double>::infinity():std::numeric_limits<double>::quiet_NaN();
		return (half&0x8000)?-value:value;
	}

	template <typename _Container>
	static _Container decode_chunks(bitwise::bit_reader& reader,std::uint8_t major,std::uint8_t info,std::size_t start) {
		if (info!=indefinite_info) return base_t::template read_block<_Container>(reader,checked_length(read_count(reader,info,start),reader,start));
		_Container result;
		while (true) {
			const std::size_t chunk_start=base_t::byte_position(reader);
			const std::uint8_t head=reader.read_u8();
			if (head==break_byte) break;
			if ((head>>5)!=major || (head&0x1F)==indefinite_info) throw std::runtime_error("Invalid chunk inside indefinite-length string at byte "+std::to_string(chunk_start));
			_Container chunk=base_t::template read_block<_Container>(reader,checked_length(read_count(reader,head&0x1F,chunk_start),reader,chunk_start));
			result.insert(result.end(),chunk.begin(),chunk.end());
		}
		return result;
	}
	static string_t decode_key(bitwise::bit_reader& reader) {
		const std::size_t start=base_t::byte_position(reader);
		const std::uint8_t head=reader.read_u8();
		if ((head>>5)!=3) throw std::runtime_error("cbor object key must be a text string at byte "+std::to_string(start));
		return decode_chunks<string_t>(reader,3,head&0x1F,start);
	}
	static dom_t decode_value(bitwise::bit_reader& reader) {
		const std::size_t start=base_t::byte_position(reader);
		const std::uint8_t head=reader.read_u8();
		const std::uint8_t major=head>>5;
		const std::uint8_t info=head&0x1F;
		switch (major) {
			case 0: {
				const std::uint64_t count=read_count(reader,info,start);
				if (count>static_cast<std::uint64_t>((std::numeric_limits<int_t>::max)())) throw std::runtime_error("Unsigned integer does not fit into int_t at byte "+std::to_string(start));
				return dom_t(static_cast<int_t>(count));
			}
			case 1: {
				const std::uint64_t count=read_count(reader,info,start);
				if (count>static_cast<std::uint64_t>((std::numeric_limits<int_t>::max)())) throw std::runtime_error("Negative integer does not fit into int_t at byte "+std::to_string(start));
				return dom_t(static_cast<int_t>(-1-static_cast<int_t>(count)));
			}
			case 2: {
				dom_t node;
				base_t::set_binary(node,decode_chunks<binary_t>(reader,2,info,start));
				return node;
			}
			case 3: return dom_t(decode_chunks<string_t>(reader,3,info,start));
			case 4: {
				dom_t result(structure::DDT_ARRAY);
				typename dom_t::array_t& array=*result.value().array;
				if (info==indefinite_info) {
					while (reader.peek_bits<std::uint8_t>(8)!=break_byte) array.push_back(decode_value(reader));
					reader.read_u8();
				} else {
					const std::uint64_t count=read_count(reader,info,start);
					for (std::uint64_t i=0;i<count;i++) array.push_back(decode_value(reader));
				}
				return result;
			}
			case 5: {
				dom_t result(structure::DDT_OBJECT);
				typename dom_t::object_t& object=*result.value().object;
				if (info==indefinite_info) {
					while (reader.peek_bits<std::uint8_t>(8)!=break_byte) {
						string_t key=decode_key(reader);
						object.emplace(std::move(key),decode_value(reader));
					}
					reader.read_u8();
				} else {
					const std::uint64_t count=read_count(reader,info,start);
					for (std::uint64_t i=0;i<count;i++) {
						string_t key=decode_key(reader);
						object.emplace(std::move(key),decode_value(reader));
					}
				}
				return result;
			}
			case 6: {
				read_count(reader,info,start);
				return decode_value(reader);
			}
			default: {
				switch (info) {
					case 20: return dom_t(static_cast<boolean_t>(false));
					case 21: return dom_t(static_cast<boolean_t>(true));
					case 22: return dom_t(nullptr);
					case 23: return dom_t(nullptr);//undefined落为null
					case 25: return dom_t(static_cast<float_t>(decode_half(reader.read_u16())));
					case 26: return dom_t(static_cast<float_t>(base_t::bits_to_f32(reader.read_u32())));
					case 27: return dom_t(static_cast<float_t>(base_t::bits_to_f64(reader.read_u64())));
					case 31: throw std::runtime_error("Unexpected break outside indefinite-length item at byte "+std::to_string(start));
					default: throw std::runtime_error("Unsupported simple value at byte "+std::to_string(start));
				}
			}
		}
	}

	static void write_head(bitwise::bit_writer& writer,std::uint8_t major,std::uint64_t count) {
		const std::uint8_t base=static_cast<std::uint8_t>(major<<5);
		if (count<24) writer.write_u8(static_cast<std::uint8_t>(base|count));
		else if (count<=0xFF) {
			writer.write_u8(base|24);
			writer.write_u8(static_cast<std::uint8_t>(count));
		} else if (count<=0xFFFF) {
			writer.write_u8(base|25);
			writer.write_u16(static_cast<std::uint16_t>(count));
		} else if (count<=0xFFFFFFFF) {
			writer.write_u8(base|26);
			writer.write_u32(static_cast<std::uint32_t>(count));
		} else {
			writer.write_u8(base|27);
			writer.write_u64(count);
		}
	}
	static void encode_value(const dom_t& node,bitwise::bit_writer& writer) {
		switch (node.type()) {
			case structure::DDT_NULL: {
				writer.write_u8(0xF6);
				break;
			}
			case structure::DDT_BOOL: {
				writer.write_u8(node.value().boolean?0xF5:0xF4);
				break;
			}
			case structure::DDT_INT: {
				const int_t value=node.value().integer;
				if (value>=0) write_head(writer,0,static_cast<std::uint64_t>(value));
				else write_head(writer,1,static_cast<std::uint64_t>(-(value+1)));
				break;
			}
			case structure::DDT_FLOAT: {
				writer.write_u8(0xFB);
				writer.write_u64(base_t::f64_to_bits(static_cast<double>(node.value().floating)));
				break;
			}
			case structure::DDT_STRING: {
				const string_t& value=*node.value().string;
				write_head(writer,3,value.size());
				writer.write_bytes(value.data(),value.size());
				break;
			}
			case structure::DDT_ARRAY: {
				write_head(writer,4,node.size());
				for (auto it=node.cbegin();it!=node.cend();it++) encode_value(*it,writer);
				break;
			}
			case structure::DDT_OBJECT: {
				write_head(writer,5,node.size());
				for (auto it=node.cbegin();it!=node.cend();it++) {
					write_head(writer,3,it.key().size());
					writer.write_bytes(it.key().data(),it.key().size());
					encode_value(*it,writer);
				}
				break;
			}
			default: {
				if (base_t::is_binary(node)) {
					const binary_t& bytes=base_t::get_binary(node);
					write_head(writer,2,bytes.size());
					writer.write_bytes(bytes.data(),bytes.size());
					break;
				}
				throw std::invalid_argument("cbor: unsupported node type "+std::to_string(static_cast<long long>(static_cast<int>(node.type()))));
			}
		}
	}
};

}

_STDEX_DOM_TPL_DEFAULT_DECLARATION
using cbor_t=basic_cbor::cbor<_Int,_Float,_Boolean,_String,_Array,_Object,_Allocator>;
using cbor=cbor_t<>;

inline namespace literals {

inline cbor_t<> operator ""_cbor(const uint8_t* v,std::size_t n) {
	return cbor_t<>::parse(v,n);
}

}

}

}

#endif
