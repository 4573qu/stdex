//Last Modified At 2026/06/12
//@Version 1.1.0.0
//修改记录(1.0.0.0->1.1.0.0):补全MessagePack原生类型——新增派生kind MPDT_EXT(ext族:int8类型码+字节载荷),
//解码0xC7-0xC9/0xD4-0xD8不再拒绝而是建MPDT_EXT节点(timestamp即ext(-1),原样往返);编码按载荷长
//1/2/4/8/16择fixext否则ext8/16/32;按现行标准配齐ext_value载荷/工厂(ext/make_ext)/判断(is_ext)/
//访问(ext_type/ext_data)/改写(set_ext)/降级(degrade_unsupported链式);报错措辞按规则去前缀。
#ifndef _STDEX_TYPE_DOM_MSGPACK_H_
#define _STDEX_TYPE_DOM_MSGPACK_H_ 1

#include <cstdint>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include "../../structure/binary_dom.h"//At Least 1.0

namespace stdex {

namespace type {

namespace basic_msgpack {

//MessagePack专有kind:自binary_data_type(BDT_BINARY)之后续号。
_STDEX_DERIVED_KIND(msgpack_data_type,structure::binary_data_type,_STDEX_KIND_AUTO_START,
	_STDEX_KIND_VALUE_AUTO(MPDT_EXT)
)

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

protected:
	//ext载荷:类型码(int8)+字节序列;union成员闲置。
	struct ext_value : base_t::value_t {
		std::int8_t code{};
		binary_t data{};

		ext_value()=default;
		ext_value(std::int8_t type_code,binary_t payload) noexcept : code(type_code) , data(std::move(payload)) { }
		~ext_value() override=default;

		ext_value(const ext_value& other) : base_t::value_t() , code(other.code) , data(other.data) { }

		typename base_t::value_t* clone(structure::dom_data_type t) const override {
			if (t==MPDT_EXT) return create_value<ext_value>(*this);
			return base_t::value_t::clone(t);
		}
		void destroy(structure::dom_data_type t) override {
			if (t==MPDT_EXT) return;
			base_t::value_t::destroy(t);
		}
		void destroy_self(structure::dom_data_type t) override {
			this->destroy(t);
			_Allocator<ext_value> alloc;
			std::allocator_traits<_Allocator<ext_value>>::destroy(alloc,this);
			std::allocator_traits<_Allocator<ext_value>>::deallocate(alloc,this,1);
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
	//载荷校验:type值与动态类型双重校验(并行派生分支的kind值可能重合,以dynamic_cast为准)。
	static ext_value* ext_payload(const base_t& node) {
		ext_value* payload=node.data().value?dynamic_cast<ext_value*>(node.data().value):nullptr;
		if (node.type()!=msgpack_data_type(MPDT_EXT) || !payload) throw std::invalid_argument("Node does not hold a msgpack ext payload");
		return payload;
	}
	static string_t make_string(const char* text) {
		return string_t(text,text+std::char_traits<char>::length(text));
	}

public:
	using base_t::base_t;
	using base_t::operator =;

	msgpack()=default;
	~msgpack() override=default;

	msgpack(const msgpack&)=default;
	msgpack(msgpack&&) noexcept=default;

	msgpack& operator =(const msgpack&)=default;
	msgpack& operator =(msgpack&&)=default;

	//支持集=binary_dom支持集+MPDT_EXT。
	bool support(structure::dom_data_type t) const noexcept override {
		return base_t::support(t) || t==MPDT_EXT;
	}

	//---工厂---
	static base_t make_ext(std::int8_t code,binary_t data) {
		base_t node;
		node.data()=typename base_t::data_t(structure::dom_data_type(MPDT_EXT),create_value<ext_value>(code,std::move(data)));
		return node;
	}
	static msgpack binary(binary_t data) {
		msgpack result;
		result.set_binary(std::move(data));
		return result;
	}
	static msgpack ext(std::int8_t code,binary_t data) {
		return msgpack(make_ext(code,std::move(data)));
	}

	//---判断---
	static bool is_ext(const base_t& node) noexcept {
		return node.type()==msgpack_data_type(MPDT_EXT);
	}
	bool is_ext() const noexcept {
		return is_ext(*this);
	}

	//---访问---
	static std::int8_t& ext_type(const base_t& node) {
		return ext_payload(node)->code;
	}
	static binary_t& ext_data(const base_t& node) {
		return ext_payload(node)->data;
	}
	std::int8_t& ext_type() {
		return ext_type(*this);
	}
	const std::int8_t& ext_type() const {
		return ext_type(*this);
	}
	binary_t& ext_data() {
		return ext_data(*this);
	}
	const binary_t& ext_data() const {
		return ext_data(*this);
	}

	//---改写---
	static void set_ext(base_t& node,std::int8_t code,binary_t data) {
		node.data()=typename base_t::data_t(structure::dom_data_type(MPDT_EXT),create_value<ext_value>(code,std::move(data)));
	}
	void set_ext(std::int8_t code,binary_t data) {
		set_ext(*this,code,std::move(data));
	}

	//---解码---
	static msgpack parse(const std::uint8_t* data,std::size_t size) {
		bitwise::bit_reader reader(data,size,bitwise::BO_MSBYTE);
		msgpack result(decode_value(reader));
		if (!reader.eof()) throw std::runtime_error("Trailing bytes after msgpack value at byte "+std::to_string(base_t::byte_position(reader)));
		return result;
	}
	static msgpack parse(const binary_t& data) {
		return parse(data.data(),data.size());
	}

	//---编码---
	binary_t dump() const {
		bitwise::bit_writer writer(bitwise::BO_MSBYTE);
		encode_value(*this,writer);
		return std::move(writer).buffer();
	}

protected:
	//记法转换协议·源侧降级:先交由binary_dom处理BDT_BINARY;MPDT_EXT降级为{"type","data"}对象,
	//其中data为二进制节点——协议对replacement递归,目标若亦不支持二进制会继续链式降级为整数数组。
	//如需其他形态请用convert_handler_t定制。
	bool degrade_unsupported(const base_t& source,base_t& replacement) const override {
		if (base_t::degrade_unsupported(source,replacement)) return true;
		if (source.type()!=msgpack_data_type(MPDT_EXT)) return false;
		const ext_value* payload=ext_payload(source);
		replacement=base_t(structure::DDT_OBJECT);
		replacement.value().object->emplace(make_string("type"),base_t(static_cast<int_t>(payload->code)));
		base_t data_node;
		base_t::set_binary(data_node,payload->data);
		replacement.value().object->emplace(make_string("data"),std::move(data_node));
		return true;
	}

private:
	static dom_t decode_binary(bitwise::bit_reader& reader,std::size_t length) {
		dom_t node;
		base_t::set_binary(node,base_t::template read_block<binary_t>(reader,length));
		return node;
	}
	static dom_t decode_ext(bitwise::bit_reader& reader,std::size_t length) {
		const std::int8_t code=static_cast<std::int8_t>(reader.read_u8());
		dom_t node;
		set_ext(node,code,base_t::template read_block<binary_t>(reader,length));
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
		if (value<static_cast<std::int64_t>((std::numeric_limits<int_t>::min)()) || value>static_cast<std::int64_t>((std::numeric_limits<int_t>::max)())) throw std::runtime_error("Signed integer does not fit into int_t at byte "+std::to_string(start));
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
			case 0xC7: return decode_ext(reader,reader.read_u8());
			case 0xC8: return decode_ext(reader,reader.read_u16());
			case 0xC9: return decode_ext(reader,reader.read_u32());
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
			case 0xD4: return decode_ext(reader,1);//fixext族
			case 0xD5: return decode_ext(reader,2);
			case 0xD6: return decode_ext(reader,4);
			case 0xD7: return decode_ext(reader,8);
			case 0xD8: return decode_ext(reader,16);
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
	static void encode_ext(const ext_value& payload,bitwise::bit_writer& writer) {
		const std::size_t length=payload.data.size();
		switch (length) {
			case 1: writer.write_u8(0xD4);break;
			case 2: writer.write_u8(0xD5);break;
			case 4: writer.write_u8(0xD6);break;
			case 8: writer.write_u8(0xD7);break;
			case 16: writer.write_u8(0xD8);break;
			default: {
				if (length<=0xFF) {
					writer.write_u8(0xC7);
					writer.write_u8(static_cast<std::uint8_t>(length));
				} else if (length<=0xFFFF) {
					writer.write_u8(0xC8);
					writer.write_u16(static_cast<std::uint16_t>(length));
				} else {
					writer.write_u8(0xC9);
					writer.write_u32(static_cast<std::uint32_t>(length));
				}
				break;
			}
		}
		writer.write_u8(static_cast<std::uint8_t>(payload.code));
		writer.write_bytes(payload.data.data(),length);
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
				if (is_ext(node)) {
					encode_ext(*ext_payload(node),writer);
					break;
				}
				throw std::invalid_argument("Unsupported node type "+std::to_string(static_cast<long long>(static_cast<int>(node.type()))));
			}
		}
	}
};

}

_STDEX_DOM_TPL_DEFAULT_DECLARATION
using msgpack_t=basic_msgpack::msgpack<_Int,_Float,_Boolean,_String,_Array,_Object,_Allocator>;
using msgpack=msgpack_t<>;

}

}

#endif
