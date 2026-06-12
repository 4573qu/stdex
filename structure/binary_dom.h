//Last Modified At 2026/06/11
//@Version 1.0.0.0
#ifndef _STDEX_STRUCTURE_BINARY_DOM_H_
#define _STDEX_STRUCTURE_BINARY_DOM_H_ 1

#include <climits>
#include <cstdint>
#include <cstring>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include "../bitwise/bit_reader.h"//At Least 2.2
#include "../bitwise/bit_writer.h"//At Least 2.2
#include "../utility/kind.h"//At Least 1.1
#include "dom.h"//At Least 1.0

namespace stdex {

namespace structure {

_STDEX_DERIVED_KIND(binary_data_type,dom_data_type,_STDEX_KIND_AUTO_START,
	_STDEX_KIND_VALUE_AUTO(BDT_BINARY)
)

_STDEX_DOM_TPL_DECLARATION
class binary_dom : public _STDEX_DOM_DEF {
public:
	using base_t=STDEX_DOM_DEF;
	using int_t=typename base_t::int_t;
	using float_t=typename base_t::float_t;
	using boolean_t=typename base_t::boolean_t;
	using string_t=typename base_t::string_t;
	using size_type=typename base_t::size_type;
	using binary_t=std::vector<std::uint8_t>;

	static_assert(sizeof(typename string_t::value_type)==1,"binary notations assume a byte-oriented string_t.");

protected:
	struct binary_value : base_t::value_t {
		binary_t bytes{};

		binary_value()=default;
		explicit binary_value(binary_t data) noexcept : bytes(std::move(data)) { }
		~binary_value() override=default;

		typename base_t::value_t* clone(dom_data_type t) const override {
			if (t==BDT_BINARY) return create_binary_value(bytes);
			return base_t::value_t::clone(t);
		}
		void destroy(dom_data_type t) override {
			if (t==BDT_BINARY) return;
			base_t::value_t::destroy(t);
		}
		void destroy_self(dom_data_type t) override {
			this->destroy(t);
			_Allocator<binary_value> alloc;
			std::allocator_traits<_Allocator<binary_value>>::destroy(alloc,this);
			std::allocator_traits<_Allocator<binary_value>>::deallocate(alloc,this,1);
		}
	};

	static binary_value* create_binary_value(binary_t data) {
		_Allocator<binary_value> alloc;
		binary_value* result=std::allocator_traits<_Allocator<binary_value>>::allocate(alloc,1);
		try {
			std::allocator_traits<_Allocator<binary_value>>::construct(alloc,result,std::move(data));
		} catch (...) {
			std::allocator_traits<_Allocator<binary_value>>::deallocate(alloc,result,1);
			throw;
		}
		return result;
	}

	static std::size_t byte_position(const bitwise::bit_reader& reader) noexcept {
		return reader.tell_bits()/CHAR_BIT;
	}
	static void require_bytes(const bitwise::bit_reader& reader,std::size_t count) {
		if (reader.remaining_bits()/CHAR_BIT<count) throw std::runtime_error("Unexpected end of input at byte "+std::to_string(byte_position(reader)));
	}
	template <typename _Container>
	static _Container read_block(bitwise::bit_reader& reader,std::size_t count) {
		require_bytes(reader,count);
		_Container result;
		result.resize(count);
		if (count) reader.read_bytes(&result[0],count);
		return result;
	}
	static std::uint64_t f64_to_bits(double value) noexcept {
		std::uint64_t bits;
		std::memcpy(&bits,&value,sizeof(bits));
		return bits;
	}
	static double bits_to_f64(std::uint64_t bits) noexcept {
		double result;
		std::memcpy(&result,&bits,sizeof(result));
		return result;
	}
	static float bits_to_f32(std::uint32_t bits) noexcept {
		float result;
		std::memcpy(&result,&bits,sizeof(result));
		return result;
	}

public:
	using base_t::base_t;
	using base_t::operator =;

	binary_dom()=default;
	~binary_dom() override=default;

	binary_dom(const binary_dom&)=default;
	binary_dom(binary_dom&&) noexcept=default;

	binary_dom& operator =(const binary_dom&)=default;
	binary_dom& operator =(binary_dom&&)=default;

	binary_dom(const base_t& other) : base_t(other) { }
	binary_dom(base_t&& other) noexcept : base_t(std::move(other)) { }

	bool support(dom_data_type t) const noexcept override {
		return base_t::support(t) || t==BDT_BINARY;
	}

	static bool is_binary(const base_t& node) noexcept {
		return node.type()==BDT_BINARY;
	}
	bool is_binary() const noexcept {
		return is_binary(*this);
	}
	static binary_t& get_binary(const base_t& node) {
		binary_value* payload=node.data().value?dynamic_cast<binary_value*>(node.data().value):nullptr;
		if (!is_binary(node) || !payload) throw std::invalid_argument("Node does not hold a binary payload");
		return payload->bytes;
	}
	binary_t& get_binary() {
		return get_binary(*this);
	}
	const binary_t& get_binary() const {
		return get_binary(*this);
	}
	static void set_binary(base_t& node,binary_t data) {
		node.data()=typename base_t::data_t(dom_data_type(BDT_BINARY),create_binary_value(std::move(data)));
	}
	void set_binary(binary_t data) {
		set_binary(*this,std::move(data));
	}

protected:
	//记法转换协议·源侧降级:BDT_BINARY默认降级为整数数组(无损但每字节膨胀为一个节点)。
	//如需其他形态请用convert_handler_t,例如base64字符串:
	//handler=[](const dom& s,dom& r){ if (s.type()!=BDT_BINARY) return false; r=base64(get_binary(s)); return true; }
	bool degrade_unsupported(const base_t& source,base_t& replacement) const override {
		if (source.type()!=BDT_BINARY) return false;
		replacement=base_t(DDT_ARRAY);
		for (std::uint8_t it:get_binary(source)) replacement.push_back(static_cast<int_t>(it));
		return true;
	}
};

}

}

#endif
