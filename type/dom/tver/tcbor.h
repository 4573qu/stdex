//Last Modified At 2026/06/12
//@Version 1.1.0.0
//修改记录(1.0.0.0->1.1.0.0):补全CBOR原生类型——①新增派生kind CDT_TAG(uint64标号+被标注的dom内容):
//解码major6不再剥取而是建CDT_TAG节点(标号与内容完整往返),编码发head(6,标号)+内容;②新增CDT_SIMPLE
//(simple值,std::uint8_t):承载0-19与32-255;20/21/22仍映射bool/null,undefined(23)由落null改为
//simple(23)以保往返;set_simple拒收20-31(20-23走bool/null规范型,24-31为编码保留段);③按现行标准
//配齐载荷/工厂(tag/simple/make_*)/判断(is_tag/is_simple)/访问(tag_number/tag_content/simple)/
//改写(set_tag/set_simple)/降级(tag取内容链式再转换,simple降整数);④报错措辞按规则去前缀。
#ifndef _STDEX_TYPE_DOM_CBOR_H_
#define _STDEX_TYPE_DOM_CBOR_H_ 1

#include <cmath>
#include <cstdint>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include "../../structure/binary_dom.h"//At Least 1.0

namespace stdex {

namespace type {

namespace basic_cbor {

//CBOR专有kind:自binary_data_type(BDT_BINARY)之后续号。
_STDEX_DERIVED_KIND(cbor_data_type,structure::binary_data_type,_STDEX_KIND_AUTO_START,
	_STDEX_KIND_VALUE_AUTO(CDT_TAG)
	_STDEX_KIND_VALUE_AUTO(CDT_SIMPLE)
)

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

protected:
	//tag载荷:语义标号+被标注的内容(内容为完整dom子树,可再嵌tag)。
	struct tag_value : base_t::value_t {
		std::uint64_t number{};
		dom_t content{};

		tag_value()=default;
		tag_value(std::uint64_t tag_number,dom_t tagged) : number(tag_number) , content(std::move(tagged)) { }
		~tag_value() override=default;

		tag_value(const tag_value& other) : base_t::value_t() , number(other.number) , content(other.content) { }

		typename base_t::value_t* clone(structure::dom_data_type t) const override {
			if (t==CDT_TAG) return create_value<tag_value>(*this);
			return base_t::value_t::clone(t);
		}
		void destroy(structure::dom_data_type t) override {
			if (t==CDT_TAG) return;
			base_t::value_t::destroy(t);
		}
		void destroy_self(structure::dom_data_type t) override {
			this->destroy(t);
			_Allocator<tag_value> alloc;
			std::allocator_traits<_Allocator<tag_value>>::destroy(alloc,this);
			std::allocator_traits<_Allocator<tag_value>>::deallocate(alloc,this,1);
		}
	};
	//simple载荷:未指派的简单值(0-19,32-255)。
	struct simple_value : base_t::value_t {
		std::uint8_t code{};

		simple_value()=default;
		explicit simple_value(std::uint8_t simple_code) noexcept : code(simple_code) { }
		~simple_value() override=default;

		simple_value(const simple_value& other) : base_t::value_t() , code(other.code) { }

		typename base_t::value_t* clone(structure::dom_data_type t) const override {
			if (t==CDT_SIMPLE) return create_value<simple_value>(*this);
			return base_t::value_t::clone(t);
		}
		void destroy(structure::dom_data_type t) override {
			if (t==CDT_SIMPLE) return;
			base_t::value_t::destroy(t);
		}
		void destroy_self(structure::dom_data_type t) override {
			this->destroy(t);
			_Allocator<simple_value> alloc;
			std::allocator_traits<_Allocator<simple_value>>::destroy(alloc,this);
			std::allocator_traits<_Allocator<simple_value>>::deallocate(alloc,this,1);
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
	static tag_value* tag_payload(const base_t& node) {
		tag_value* payload=node.data().value?dynamic_cast<tag_value*>(node.data().value):nullptr;
		if (node.type()!=cbor_data_type(CDT_TAG) || !payload) throw std::invalid_argument("Node does not hold a cbor tag payload");
		return payload;
	}
	static simple_value* simple_payload(const base_t& node) {
		simple_value* payload=node.data().value?dynamic_cast<simple_value*>(node.data().value):nullptr;
		if (node.type()!=cbor_data_type(CDT_SIMPLE) || !payload) throw std::invalid_argument("Node does not hold a cbor simple value payload");
		return payload;
	}

public:
	using base_t::base_t;
	using base_t::operator =;

	cbor()=default;
	~cbor() override=default;

	cbor(const cbor&)=default;
	cbor(cbor&&) noexcept=default;

	cbor& operator =(const cbor&)=default;
	cbor& operator =(cbor&&)=default;

	//支持集=binary_dom支持集+CDT_TAG+CDT_SIMPLE。
	bool support(structure::dom_data_type t) const noexcept override {
		return base_t::support(t) || t==CDT_TAG || t==CDT_SIMPLE;
	}

	//---工厂---
	static base_t make_tag(std::uint64_t number,base_t content) {
		base_t node;
		node.data()=typename base_t::data_t(structure::dom_data_type(CDT_TAG),create_value<tag_value>(number,std::move(content)));
		return node;
	}
	static base_t make_simple(std::uint8_t code) {
		check_simple(code);
		base_t node;
		node.data()=typename base_t::data_t(structure::dom_data_type(CDT_SIMPLE),create_value<simple_value>(code));
		return node;
	}
	static cbor binary(binary_t data) {
		cbor result;
		result.set_binary(std::move(data));
		return result;
	}
	static cbor tag(std::uint64_t number,base_t content) {
		return cbor(make_tag(number,std::move(content)));
	}
	static cbor simple(std::uint8_t code) {
		return cbor(make_simple(code));
	}

	//---判断---
	static bool is_tag(const base_t& node) noexcept {
		return node.type()==cbor_data_type(CDT_TAG);
	}
	static bool is_simple(const base_t& node) noexcept {
		return node.type()==cbor_data_type(CDT_SIMPLE);
	}
	bool is_tag() const noexcept {
		return is_tag(*this);
	}
	bool is_simple() const noexcept {
		return is_simple(*this);
	}

	//---访问---
	static std::uint64_t& tag_number(const base_t& node) {
		return tag_payload(node)->number;
	}
	static dom_t& tag_content(const base_t& node) {
		return tag_payload(node)->content;
	}
	static std::uint8_t& simple(const base_t& node) {
		return simple_payload(node)->code;
	}
	std::uint64_t& tag_number() {
		return tag_number(*this);
	}
	const std::uint64_t& tag_number() const {
		return tag_number(*this);
	}
	dom_t& tag_content() {
		return tag_content(*this);
	}
	const dom_t& tag_content() const {
		return tag_content(*this);
	}
	std::uint8_t& simple() {
		return simple(*this);
	}
	const std::uint8_t& simple() const {
		return simple(*this);
	}

	//---改写---
	static void set_tag(base_t& node,std::uint64_t number,base_t content) {
		node.data()=typename base_t::data_t(structure::dom_data_type(CDT_TAG),create_value<tag_value>(number,std::move(content)));
	}
	static void set_simple(base_t& node,std::uint8_t code) {
		check_simple(code);
		node.data()=typename base_t::data_t(structure::dom_data_type(CDT_SIMPLE),create_value<simple_value>(code));
	}
	void set_tag(std::uint64_t number,base_t content) {
		set_tag(*this,number,std::move(content));
	}
	void set_simple(std::uint8_t code) {
		set_simple(*this,code);
	}

	//---解码---
	static cbor parse(const std::uint8_t* data,std::size_t size) {
		bitwise::bit_reader reader(data,size,bitwise::BO_MSBYTE);
		cbor result(decode_value(reader));
		if (!reader.eof()) throw std::runtime_error("Trailing bytes after cbor value at byte "+std::to_string(base_t::byte_position(reader)));
		return result;
	}
	static cbor parse(const binary_t& data) {
		return parse(data.data(),data.size());
	}

	//---编码---
	binary_t dump() const {
		bitwise::bit_writer writer(bitwise::BO_MSBYTE);
		encode_value(*this,writer);
		return std::move(writer).buffer();
	}

protected:
	//记法转换协议·源侧降级:先交由binary_dom处理BDT_BINARY;CDT_TAG降级为其内容(去标号,协议对
	//replacement递归,内容内嵌的特殊节点会继续降级);CDT_SIMPLE降级为整数。内容整搬经data()做
	//data_t级拷贝,绕开operator=的根support检查(内容自身可能是tag/binary等派生kind)。
	//如需其他形态(如保留标号为对象)请用convert_handler_t定制。
	bool degrade_unsupported(const base_t& source,base_t& replacement) const override {
		if (base_t::degrade_unsupported(source,replacement)) return true;
		if (source.type()==cbor_data_type(CDT_TAG)) {
			replacement.data()=typename base_t::data_t(tag_payload(source)->content.data());
			return true;
		}
		if (source.type()==cbor_data_type(CDT_SIMPLE)) {
			replacement=base_t(static_cast<int_t>(simple_payload(source)->code));
			return true;
		}
		return false;
	}

private:
	static constexpr std::uint8_t indefinite_info=0x1F;
	static constexpr std::uint8_t break_byte=0xFF;

	//simple值域校验:20-23走boolean/null规范型,24-31为编码保留段。
	static void check_simple(std::uint8_t code) {
		if (code>=20 && code<=31) throw std::invalid_argument("Simple values 20 to 31 are reserved");
	}
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
			case 6: {//tag:保留标号与内容,建CDT_TAG节点
				const std::uint64_t number=read_count(reader,info,start);
				dom_t node;
				set_tag(node,number,decode_value(reader));
				return node;
			}
			default: {//major 7:simple/浮点
				switch (info) {
					case 20: return dom_t(static_cast<boolean_t>(false));
					case 21: return dom_t(static_cast<boolean_t>(true));
					case 22: return dom_t(nullptr);
					case 23: return make_simple(23);//undefined:以simple(23)保往返
					case 24: {
						const std::uint8_t code=reader.read_u8();
						if (code<32) throw std::runtime_error("Invalid simple value encoding at byte "+std::to_string(start));
						return make_simple(code);
					}
					case 25: return dom_t(static_cast<float_t>(decode_half(reader.read_u16())));
					case 26: return dom_t(static_cast<float_t>(base_t::bits_to_f32(reader.read_u32())));
					case 27: return dom_t(static_cast<float_t>(base_t::bits_to_f64(reader.read_u64())));
					case 31: throw std::runtime_error("Unexpected break outside indefinite-length item at byte "+std::to_string(start));
					default: return make_simple(info);//0-19:未指派simple
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
				if (is_tag(node)) {
					const tag_value* payload=tag_payload(node);
					write_head(writer,6,payload->number);
					encode_value(payload->content,writer);
					break;
				}
				if (is_simple(node)) {
					const std::uint8_t code=simple_payload(node)->code;
					if (code<24) writer.write_u8(static_cast<std::uint8_t>(0xE0|code));
					else {
						writer.write_u8(0xF8);
						writer.write_u8(code);
					}
					break;
				}
				throw std::invalid_argument("Unsupported node type "+std::to_string(static_cast<long long>(static_cast<int>(node.type()))));
			}
		}
	}
};

}

_STDEX_DOM_TPL_DEFAULT_DECLARATION
using cbor_t=basic_cbor::cbor<_Int,_Float,_Boolean,_String,_Array,_Object,_Allocator>;
using cbor=cbor_t<>;

}

}

#endif
