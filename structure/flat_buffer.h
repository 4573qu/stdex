//Last Modified At 2026/05/11
//@Version 1.0.0.0
#ifndef _STDEX_STRUCTURE_FLAT_BUFFER_H_
#define _STDEX_STRUCTURE_FLAT_BUFFER_H_ 1

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iterator>
#include <limits>
#include <memory>
#include <optional>
#include <ostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

#include "../bitwise/endianness.h"//At Least 1.1
#include "../crypto/base.h"//At Least 1.0

#if __has_include("../macros/cpp_version.h")
#include "../macros/cpp_version.h"//At Least 1.0
#endif

#ifndef _STDEX_CPP20_VERSION
#define _STDEX_CPP20_VERSION 202002L
#endif
#ifndef _STDEX_CPP23_VERSION
#define _STDEX_CPP23_VERSION 202302L
#endif

#if __cplusplus>=_STDEX_CPP20_VERSION
#include <bit>
#include <span>
#endif

#if __cplusplus>=_STDEX_CPP23_VERSION
#include <stdfloat>
#endif

namespace stdex {

namespace structure {

namespace flat_buffer {

class flat_buffer;

template <typename _Tp,typename _Enable=void>
struct flat_buffer_serializer {};

enum flat_buffer_type_tag {
	FBTT_LONG_INT=1,
	FBTT_ULONG_INT,
	FBTT_LONG_DOUBLE,
	FBTT_WCHAR,
	FBTT_OTHER,
};

namespace {

template <typename _Tp>
_Tp bswap(_Tp val) noexcept {
	static_assert(std::is_integral_v<_Tp>,"bswap requires integral type.");
	//static_assert(!std::is_same_v<_Tp,bool>,"bswap does not support bool.");
	using unsigned_type=std::make_unsigned_t<_Tp>;
	return static_cast<_Tp>(bitwise::reverse_bytes(static_cast<unsigned_type>(val)));
}

template <>
inline uint8_t bswap<uint8_t>(uint8_t val) noexcept {
	return val;
}

template <>
inline int8_t bswap<int8_t>(int8_t val) noexcept {
	return val;
}

template <typename _Tp>
_Tp to_little_endian(_Tp val) noexcept {
	static_assert(std::is_integral_v<_Tp>,"to_little_endian requires integral type.");
	//static_assert(!std::is_same_v<_Tp,bool>,"to_little_endian does not support bool.");
	using unsigned_type=std::make_unsigned_t<_Tp>;
	return static_cast<_Tp>(bitwise::to_little_endian(static_cast<unsigned_type>(val)));
}

template <typename _Tp>
_Tp from_little_endian(_Tp val) noexcept {
	static_assert(std::is_integral_v<_Tp>,"from_little_endian requires integral type.");
	//static_assert(!std::is_same_v<_Tp,bool>,"from_little_endian does not support bool");
	if constexpr (bitwise::is_little_endian()) {
		return val;
	} else {
		return bswap(val);
	}
}

inline void normalize_endian(uint8_t* buf,std::size_t size,bool src_is_little) noexcept {
	bool host_is_little=bitwise::is_little_endian();
	if (src_is_little!=host_is_little) std::reverse(buf,buf+size);
}

}

template <typename _Tp,typename=void>
struct is_serializer_valid : std::false_type {};

template <typename _Tp>
struct is_serializer_valid<_Tp,std::void_t<decltype(flat_buffer_serializer<_Tp>::serialize(std::declval<flat_buffer&>(),std::declval<const _Tp&>())),decltype(flat_buffer_serializer<_Tp>::deserialize(std::declval<const flat_buffer&>(),std::declval<flat_buffer::size_type>()))>> : std::true_type {};

template <typename _Tp>
constexpr bool is_serializer_valid_v=is_serializer_valid<_Tp>::value;

template <typename _Tp,typename=void>
struct serializer_has_size : std::false_type {};

template <typename _Tp>
struct serializer_has_size<_Tp,std::void_t<decltype(flat_buffer_serializer<_Tp>::serialized_size(std::declval<const flat_buffer&>(),std::declval<flat_buffer::size_type>()))>> : std::true_type {};

template <typename _Tp>
constexpr bool serializer_has_size_v=serializer_has_size<_Tp>::value;

template <typename _Tp,typename=void>
struct has_adl_serialize : std::false_type {};

template <typename _Tp>
struct has_adl_serialize<_Tp,std::void_t<decltype(serialize_to_flat_buffer(std::declval<flat_buffer&>(),std::declval<const _Tp&>()))>> : std::true_type {};

template <typename _Tp>
constexpr bool has_adl_serialize_v=has_adl_serialize<_Tp>::value;

template <typename _Tp,typename=void>
struct has_adl_deserialize : std::false_type {};

template <typename _Tp>
struct has_adl_deserialize<_Tp,std::void_t<decltype(deserialize_from_flat_buffer(std::declval<const flat_buffer&>(),std::declval<flat_buffer::size_type>(),std::declval<_Tp&>()))>> : std::true_type {};

template <typename _Tp>
constexpr bool has_adl_deserialize_v=has_adl_deserialize<_Tp>::value;

template <typename _Tp,typename=void>
struct has_adl_serialized_size : std::false_type {};

template <typename _Tp>
struct has_adl_serialized_size<_Tp,std::void_t<decltype(serialized_size_in_flat_buffer(std::declval<const flat_buffer&>(),std::declval<flat_buffer::size_type>(),static_cast<_Tp*>(nullptr)))>> : std::true_type {};

template <typename _Tp>
constexpr bool has_adl_serialized_size_v=has_adl_serialized_size<_Tp>::value;

template <typename _Tp>
struct is_builtin_type : std::integral_constant<bool,std::is_same<_Tp,bool>::value || std::is_same<_Tp,int8_t>::value || std::is_same<_Tp,uint8_t>::value || std::is_same<_Tp,int16_t>::value || std::is_same<_Tp,uint16_t>::value || std::is_same<_Tp,int32_t>::value || std::is_same<_Tp,uint32_t>::value || std::is_same<_Tp,int64_t>::value || std::is_same<_Tp,uint64_t>::value || std::is_same<_Tp,float>::value || std::is_same<_Tp,double>::value || std::is_same<_Tp,long>::value || std::is_same<_Tp,unsigned long>::value || std::is_same<_Tp,long double>::value || std::is_same<_Tp,wchar_t>::value || std::is_same<_Tp,std::string>::value || std::is_same<_Tp,std::string_view>::value || std::is_same<_Tp,std::vector<uint8_t>>::value || std::is_same<_Tp,flat_buffer>::value> {};

template<typename _Tp>
constexpr bool is_builtin_type_v=is_builtin_type<_Tp>::value;

class flat_buffer {
public:
	using value_type=uint8_t;
	using size_type=std::size_t;
	using difference_type=std::ptrdiff_t;
	using reference=uint8_t&;
	using const_reference=const uint8_t&;
	using pointer=uint8_t*;
	using const_pointer=const uint8_t*;
	using storage_type=std::vector<uint8_t>;

	using iterator=typename storage_type::iterator;
	using const_iterator=typename storage_type::const_iterator;
	using reverse_iterator=typename storage_type::reverse_iterator;
	using const_reverse_iterator=typename storage_type::const_reverse_iterator;

	static constexpr size_type npos=static_cast<size_type>(-1);

private:
	storage_type data_;
	size_type read_pos_=0;

	template <typename _Uint>
	void write_le(_Uint val) {
		static_assert(std::is_integral<_Uint>::value && std::is_unsigned<_Uint>::value,"write_le requires unsigned integral.");
		_Uint le=to_little_endian(val);
		const uint8_t* p=reinterpret_cast<const uint8_t*>(&le);
		data_.insert(data_.end(),p,p+sizeof(_Uint));
	}

	template <typename _Uint>
	void write_le_at(size_type offset,_Uint val) {
		static_assert(std::is_integral<_Uint>::value && std::is_unsigned<_Uint>::value,"write_le_at requires unsigned integral.");
		if (offset+sizeof(_Uint)>data_.size()) throw std::out_of_range("write_le_at out of range");//"flat_buffer::write_le_at: write exceeds buffer size");
		_Uint le=to_little_endian(val);
		std::memcpy(data_.data()+offset,&le,sizeof(_Uint));
	}

	template <typename _Uint>
	_Uint read_le(size_type offset) const {
		static_assert(std::is_integral<_Uint>::value && std::is_unsigned<_Uint>::value,"read_le requires unsigned integral");
		if (offset+sizeof(_Uint)>data_.size()) throw std::out_of_range("read_le out of range");//"flat_buffer::read_le: read exceeds buffer size at offset "+std::to_string(offset));
		_Uint le=0;
		std::memcpy(&le,data_.data()+offset,sizeof(_Uint));
		return from_little_endian(le);
	}

	void write_portable_raw(flat_buffer_type_tag tag,const void* val_ptr,uint8_t byte_size) {
		data_.push_back(static_cast<uint8_t>(tag));
		data_.push_back(byte_size);
		data_.push_back(bitwise::is_little_endian()?0x00u:0x01u);
		const uint8_t* p=reinterpret_cast<const uint8_t*>(val_ptr);
		data_.insert(data_.end(),p,p+byte_size);
	}

	size_type read_portable_raw(size_type offset,flat_buffer_type_tag expected_tag,uint8_t* out_buf,size_type out_buf_capacity,uint8_t& out_byte_size) const {
		if (offset+3>data_.size()) throw std::out_of_range("read_portable_raw header truncated");//"flat_buffer::read_portable_raw: header truncated at offset "+std::to_string(offset));
		uint8_t tag_byte=data_[offset];
		uint8_t byte_size=data_[offset+1];
		uint8_t endian_flag=data_[offset+2];
		if (static_cast<flat_buffer_type_tag>(tag_byte)!=expected_tag) throw std::invalid_argument("read_portable_raw type_tag mismatch");//"flat_buffer::read_portable_raw: type tag mismatch"_;
		if (offset+3+byte_size>data_.size()) throw std::out_of_range("read_portable_raw data truncated");//"flat_buffer::read_portable_raw: data truncated at offset "+std::to_string(offset));
		size_type copy_size=std::min(static_cast<size_type>(byte_size),out_buf_capacity);
		std::memset(out_buf,0,out_buf_capacity);
		std::memcpy(out_buf,data_.data()+offset+3,copy_size);
		bool src_is_little=(endian_flag==0x00u);
		normalize_endian(out_buf,copy_size,src_is_little);
		out_byte_size=byte_size;
		return static_cast<size_type>(3)+byte_size;
	}

public:
	flat_buffer() noexcept=default;
	explicit flat_buffer(size_type initial_capacity) {
		data_.reserve(initial_capacity);
	}
	flat_buffer(const uint8_t* src,size_type len) {
		if (src && len>0) data_.assign(src,src+len);
	}
	flat_buffer(const void* src,size_type len) {
		if (src && len>0) {
			const uint8_t* p=reinterpret_cast<const uint8_t*>(src);
			data_.assign(p,p+len);
		}
	}
	explicit flat_buffer(const std::vector<uint8_t>& vec) : data_(vec) { }
	explicit flat_buffer(std::vector<uint8_t>&& vec) noexcept : data_(std::move(vec)) { }
	flat_buffer(std::initializer_list<uint8_t> init_list) : data_(init_list) { }
	~flat_buffer()=default;

	flat_buffer(const flat_buffer&)=default;
	flat_buffer(flat_buffer&&) noexcept=default;

	flat_buffer& operator =(const flat_buffer&)=default;
	flat_buffer& operator =(flat_buffer&&) noexcept=default;
	flat_buffer& operator =(std::vector<uint8_t>&& vec) noexcept {
		data_=std::move(vec);
		return *this;
	}
	flat_buffer& operator =(const std::vector<uint8_t>& vec) {
		data_=vec;
		return *this;
	}
	flat_buffer& operator =(std::initializer_list<uint8_t> init_list) {
		data_=init_list;
		return *this;
	}

#if __cplusplus>=_STDEX_CPP20_VERSION
	explicit flat_buffer(std::span<const uint8_t> sp) {
		data_.assign(sp.begin(),sp.end());
	}

	flat_buffer& operator =(std::span<const uint8_t> sp) {
		data_.assign(sp.begin(),sp.end());
		return *this;
	}
#endif

	[[nodiscard]]
	bool empty() const noexcept {
		return data_.empty();
	}
	[[nodiscard]]
	size_type size() const noexcept {
		return data_.size();
	}
	[[nodiscard]]
	size_type capacity() const noexcept {
		return data_.capacity();
	}
	[[nodiscard]]
	size_type max_size() const noexcept {
		return data_.max_size();
	}
	void reserve(size_type n) {
		data_.reserve(n);
	}
	void resize(size_type n,uint8_t fill=0x00u) {
		data_.resize(n,fill);
	}
	void shrink_to_fit() {
		data_.shrink_to_fit();
	}
	void clear() noexcept {
		data_.clear();
		read_pos_=0;
	}
	[[nodiscard]]
	pointer data() noexcept {
		return data_.data();
	}
	[[nodiscard]]
	const_pointer data() const noexcept {
		return data_.data();
	}
	[[nodiscard]]
	const storage_type& as_vector() const noexcept {
		return data_;
	}
	[[nodiscard]]
	storage_type& as_vector() noexcept {
		return data_;
	}
	[[nodiscard]]
	std::string_view as_string_view() const noexcept {
		return std::string_view(reinterpret_cast<const char*>(data_.data()),data_.size());
	}
	[[nodiscard]]
	std::string as_string() const {
		return std::string(reinterpret_cast<const char*>(data_.data()),data_.size());
	}

#if __cplusplus>=_STDEX_CPP20_VERSION
	[[nodiscard]]
	std::span<const uint8_t> as_span() const noexcept {
		return std::span<const uint8_t>(data_.data(),data_.size());
	}
	[[nodiscard]]
	std::span<uint8_t> as_span() noexcept {
		return std::span<uint8_t>(data_.data(),data_.size());
	}
#endif

	[[nodiscard]]
	iterator begin() noexcept { return data_.begin(); }
	[[nodiscard]]
	const_iterator begin() const noexcept { return data_.begin(); }
	[[nodiscard]]
	iterator end() noexcept { return data_.end(); }
	[[nodiscard]]
	const_iterator end() const noexcept { return data_.end(); }
	[[nodiscard]]
	const_iterator cbegin() const noexcept { return data_.cbegin(); }
	[[nodiscard]]
	const_iterator cend() const noexcept { return data_.cend(); }
	[[nodiscard]]
	reverse_iterator rbegin() noexcept { return data_.rbegin(); }
	[[nodiscard]]
	const_reverse_iterator rbegin() const noexcept { return data_.rbegin(); }
	[[nodiscard]]
	reverse_iterator rend() noexcept { return data_.rend(); }
	[[nodiscard]]
	const_reverse_iterator rend() const noexcept { return data_.rend(); }
	[[nodiscard]]
	const_reverse_iterator crbegin() const noexcept { return data_.crbegin(); }
	[[nodiscard]]
	const_reverse_iterator crend() const noexcept { return data_.crend(); }

	[[nodiscard]]
	reference operator [](size_type offset) noexcept {
		return data_[offset];
	}
	[[nodiscard]]
	const_reference operator [](size_type offset) const noexcept {
		return data_[offset];
	}
	[[nodiscard]]
	reference at(size_type offset) {
		if (offset>=data_.size()) throw std::out_of_range("At out of range");//"flat_buffer::at: offset "+std::to_string(offset)+" out of range [0,"+std::to_string(data_.size())+")");
		return data_[offset];
	}
	[[nodiscard]]
	const_reference at(size_type offset) const {
		if (offset>=data_.size()) throw std::out_of_range("At out of range");//"flat_buffer::at: offset "+std::to_string(offset)+" out of range [0,"+std::to_string(data_.size())+")");
		return data_[offset];
	}

	[[nodiscard]]
	reference front() noexcept { return data_.front(); }
	[[nodiscard]]
	const_reference front() const noexcept { return data_.front(); }
	[[nodiscard]]
	reference back() noexcept { return data_.back(); }
	[[nodiscard]]
	const_reference back() const noexcept { return data_.back(); }

	void push_back(uint8_t byte) { data_.push_back(byte); }
	void pop_back() { data_.pop_back(); }

	template <typename... _Args>
	reference emplace_back(_Args&&... args) {
		return data_.emplace_back(std::forward<_Args>(args)...);
	}
	iterator insert(const_iterator pos,uint8_t byte) {
		return data_.insert(pos,byte);
	}
	iterator insert(const_iterator pos,size_type count,uint8_t byte) {
		return data_.insert(pos,count,byte);
	}

	template <typename _InputIt>
	iterator insert(const_iterator pos,_InputIt first,_InputIt last) {
		return data_.insert(pos,first,last);
	}
	iterator insert(const_iterator pos,std::initializer_list<uint8_t> init_list) {
		return data_.insert(pos,init_list);
	}
	iterator erase(const_iterator pos) {
		return data_.erase(pos);
	}

	iterator erase(const_iterator first,const_iterator last) {
		return data_.erase(first,last);
	}

	flat_buffer& append(const uint8_t* src,size_type len) {
		if (src && len>0) data_.insert(data_.end(),src,src+len);
		return *this;
	}
	flat_buffer& append(const void* src,size_type len) {
		if (src && len>0) {
			const uint8_t* p=reinterpret_cast<const uint8_t*>(src);
			data_.insert(data_.end(),p,p+len);
		}
		return *this;
	}
	flat_buffer& append(const flat_buffer& other) {
		data_.insert(data_.end(),other.data_.begin(),other.data_.end());
		return *this;
	}
	flat_buffer& append(std::string_view sv) {
		data_.insert(data_.end(),reinterpret_cast<const uint8_t*>(sv.data()),reinterpret_cast<const uint8_t*>(sv.data())+sv.size());
		return *this;
	}
	flat_buffer& append(const std::vector<uint8_t>& vec) {
		data_.insert(data_.end(),vec.begin(),vec.end());
		return *this;
	}

#if __cplusplus>=_STDEX_CPP20_VERSION
	flat_buffer& append(std::span<const uint8_t> sp) {
		data_.insert(data_.end(),sp.begin(),sp.end());
		return *this;
	}
#endif

	void swap(flat_buffer& other) noexcept {
		data_.swap(other.data_);
		std::swap(read_pos_,other.read_pos_);
	}

	[[nodiscard]]
	flat_buffer sub_buffer(size_type offset,size_type len=npos) const {
		if (offset>data_.size()) throw std::out_of_range("sub_buffet out of range");//"flat_buffer::sub_buffer: offset out of range");
		size_type actual_len=std::min(len,data_.size()-offset);
		return flat_buffer(data_.data()+offset,actual_len);
	}

	flat_buffer& write(bool val) {
		data_.push_back(val?0x01u:0x00u);
		return *this;
	}
	flat_buffer& write(int8_t val) {
		data_.push_back(static_cast<uint8_t>(val));
		return *this;
	}
	flat_buffer& write(uint8_t val) {
		data_.push_back(val);
		return *this;
	}
	flat_buffer& write(int16_t val) {
		write_le(static_cast<uint16_t>(val));
		return *this;
	}
	flat_buffer& write(uint16_t val) {
		write_le(val);
		return *this;
	}
	flat_buffer& write(int32_t val) {
		write_le(static_cast<uint32_t>(val));
		return *this;
	}
	flat_buffer& write(uint32_t val) {
		write_le(val);
		return *this;
	}
	flat_buffer& write(int64_t val) {
		write_le(static_cast<uint64_t>(val));
		return *this;
	}
	flat_buffer& write(uint64_t val) {
		write_le(val);
		return *this;
	}
	flat_buffer& write(float val) {
		uint32_t bits=0;
		std::memcpy(&bits,&val,sizeof(float));
		write_le(bits);
		return *this;
	}
	flat_buffer& write(double val) {
		uint64_t bits=0;
		std::memcpy(&bits,&val,sizeof(double));
		write_le(bits);
		return *this;
	}
	flat_buffer& write(long val) {
		static_assert(sizeof(long)<=255,"long size exceeds portable protocol limit.");
		write_portable_raw(FBTT_LONG_INT,& val,static_cast<uint8_t>(sizeof(long)));
		return *this;
	}
	flat_buffer& write(unsigned long val) {
		static_assert(sizeof(unsigned long)<=255,"unsigned long size exceeds portable protocol limit.");
		write_portable_raw(FBTT_ULONG_INT,& val,static_cast<uint8_t>(sizeof(unsigned long)));
		return *this;
	}
	flat_buffer& write(long double val) {
		static_assert(sizeof(long double)<=255,"long double size exceeds portable protocol limit.");
		write_portable_raw(FBTT_LONG_DOUBLE,& val,static_cast<uint8_t>(sizeof(long double)));
		return *this;
	}
	flat_buffer& write(wchar_t val) {
		static_assert(sizeof(wchar_t)<=255,"wchar_t size exceeds portable protocol limit.");
		write_portable_raw(FBTT_WCHAR,& val,static_cast<uint8_t>(sizeof(wchar_t)));
		return *this;
	}
	flat_buffer& write(const std::string& val) {
		if (val.size()>static_cast<size_type>(std::numeric_limits<uint32_t>::max())) throw std::invalid_argument("string too long");
		write(static_cast<uint32_t>(val.size()));
		data_.insert(data_.end(),reinterpret_cast<const uint8_t*>(val.data()),reinterpret_cast<const uint8_t*>(val.data())+val.size());
		return *this;
	}
	flat_buffer& write(std::string_view val) {
		if (val.size()>static_cast<size_type>(std::numeric_limits<uint32_t>::max()))throw std::invalid_argument("string_view too long");
		write(static_cast<uint32_t>(val.size()));
		data_.insert(data_.end(),reinterpret_cast<const uint8_t*>(val.data()),reinterpret_cast<const uint8_t*>(val.data())+val.size());
		return *this;
	}

	flat_buffer& write(const std::vector<uint8_t>& val) {
		if (val.size()>static_cast<size_type>(std::numeric_limits<uint32_t>::max())) throw std::invalid_argument("vector too long");
		write(static_cast<uint32_t>(val.size()));
		data_.insert(data_.end(),val.begin(),val.end());
		return *this;
	}
	flat_buffer& write(const flat_buffer& other) {
		if (other.size()>static_cast<size_type>(std::numeric_limits<uint32_t>::max())) throw std::invalid_argument(" nested buffer too long");
		write(static_cast<uint32_t>(other.size()));
		data_.insert(data_.end(),other.data_.begin(),other.data_.end());
		return *this;
	}

#if __cplusplus>=_STDEX_CPP20_VERSION
	flat_buffer& write(std::span<const uint8_t> sp) {
		if (sp.size()>static_cast<size_type>(std::numeric_limits<uint32_t>::max())) throw std::invalid_argument("span too long");
		write(static_cast<uint32_t>(sp.size()));
		data_.insert(data_.end(),sp.begin(),sp.end());
		return *this;
	}
#endif

	template<typename _Tp,typename=std::enable_if_t<!is_builtin_type<std::decay_t<_Tp>>::value>>
	flat_buffer& write(const _Tp& val) {
		if constexpr (is_serializer_valid_v<std::decay_t<_Tp>>) {
			flat_buffer_serializer<std::decay_t<_Tp>>::serialize(*this,val);
		} else if constexpr (has_adl_serialize_v<std::decay_t<_Tp>>) {
			serialize_to_flat_buffer(*this,val);
		} else {
			static_assert(sizeof(std::decay_t<_Tp>)==0,"flat_buffer::write: no serializer found for type. Provide a flat_buffer_serializer<T> specialization or a serialize_to_flat_buffer(flat_buffer&,const std::decay_t<_Tp>&) ADL function.");
		}
		return *this;
	}

	flat_buffer& write_raw(const void* src,size_type len) {
		if (src && len>0) {
			const uint8_t* p=reinterpret_cast<const uint8_t*>(src);
			data_.insert(data_.end(),p,p+len);
		}
		return *this;
	}
	flat_buffer& write_raw(uint8_t byte) {
		data_.push_back(byte);
		return *this;
	}

	flat_buffer& write_at(size_type offset,bool val) {
		at(offset)=val?0x01u:0x00u;
		return *this;
	}
	flat_buffer& write_at(size_type offset,int8_t val) {
		at(offset)=static_cast<uint8_t>(val);
		return *this;
	}
	flat_buffer& write_at(size_type offset,uint8_t val) {
		at(offset)=val;
		return *this;
	}
	flat_buffer& write_at(size_type offset,int16_t val) {
		write_le_at(offset,static_cast<uint16_t>(val));
		return *this;
	}
	flat_buffer& write_at(size_type offset,uint16_t val) {
		write_le_at(offset,val);
		return *this;
	}
	flat_buffer& write_at(size_type offset,int32_t val) {
		write_le_at(offset,static_cast<uint32_t>(val));
		return *this;
	}
	flat_buffer& write_at(size_type offset,uint32_t val) {
		write_le_at(offset,val);
		return *this;
	}
	flat_buffer& write_at(size_type offset,int64_t val) {
		write_le_at(offset,static_cast<uint64_t>(val));
		return *this;
	}
	flat_buffer& write_at(size_type offset,uint64_t val) {
		write_le_at(offset,val);
		return *this;
	}
	flat_buffer& write_at(size_type offset,float val) {
		uint32_t bits=0;
		std::memcpy(&bits,&val,sizeof(float));
		write_le_at(offset,bits);
		return *this;
	}
	flat_buffer& write_at(size_type offset,double val) {
		uint64_t bits=0;
		std::memcpy(&bits,&val,sizeof(double));
		write_le_at(offset,bits);
		return *this;
	}

	[[nodiscard]]
	bool read_bool(size_type offset) const {
		if (offset>=data_.size()) throw std::out_of_range("read_bool out of range");
		return data_[offset]!=0x00u;
	}
	[[nodiscard]]
	int8_t read_int8(size_type offset) const {
		if (offset>=data_.size()) throw std::out_of_range("read_int8 out of range");
		return static_cast<int8_t>(data_[offset]);
	}
	[[nodiscard]]
	uint8_t read_uint8(size_type offset) const {
		if (offset>=data_.size()) throw std::out_of_range("read_uint8 out of range");
		return data_[offset];
	}
	[[nodiscard]]
	int16_t read_int16(size_type offset) const {
		return static_cast<int16_t>(read_le<uint16_t>(offset));
	}
	[[nodiscard]]
	uint16_t read_uint16(size_type offset) const {
		return read_le<uint16_t>(offset);
	}
	[[nodiscard]]
	int32_t read_int32(size_type offset) const {
		return static_cast<int32_t>(read_le<uint32_t>(offset));
	}
	[[nodiscard]]
	uint32_t read_uint32(size_type offset) const {
		return read_le<uint32_t>(offset);
	}
	[[nodiscard]]
	int64_t read_int64(size_type offset) const {
		return static_cast<int64_t>(read_le<uint64_t>(offset));
	}
	[[nodiscard]]
	uint64_t read_uint64(size_type offset) const {
		return read_le<uint64_t>(offset);
	}
	[[nodiscard]]
	float read_float(size_type offset) const {
		uint32_t bits=read_le<uint32_t>(offset);
		float val=0.0f;
		std::memcpy(&val,&bits,sizeof(float));
		return val;
	}
	[[nodiscard]]
	double read_double(size_type offset) const {
		uint64_t bits=read_le<uint64_t>(offset);
		double val=0.0;
		std::memcpy(&val,&bits,sizeof(double));
		return val;
	}
	[[nodiscard]]
	long read_long(size_type offset) const {
		uint8_t buf[sizeof(long)];
		uint8_t stored_size=0;
		read_portable_raw(offset,FBTT_LONG_INT,buf,sizeof(long),stored_size);
		long val=0;
		std::memcpy(&val,buf,std::min(static_cast<size_type>(stored_size),sizeof(long)));
		return val;
	}
	[[nodiscard]]
	unsigned long read_ulong(size_type offset) const {
		uint8_t buf[sizeof(unsigned long)];
		uint8_t stored_size=0;
		read_portable_raw(offset,FBTT_ULONG_INT,buf,sizeof(unsigned long),stored_size);
		unsigned long val=0;
		std::memcpy(&val,buf,std::min(static_cast<size_type>(stored_size),sizeof(unsigned long)));
		return val;
	}
	[[nodiscard]]
	long double read_long_double(size_type offset) const {
		uint8_t buf[sizeof(long double)];
		uint8_t stored_size=0;
		read_portable_raw(offset,FBTT_LONG_DOUBLE,buf,sizeof(long double),stored_size);
		long double val=0.0L;
		std::memcpy(&val,buf,std::min(static_cast<size_type>(stored_size),sizeof(long double)));
		return val;
	}
	[[nodiscard]]
	wchar_t read_wchar(size_type offset) const {
		uint8_t buf[sizeof(wchar_t)];
		uint8_t stored_size=0;
		read_portable_raw(offset,FBTT_WCHAR,buf,sizeof(wchar_t),stored_size);
		wchar_t val=L'\0';
		std::memcpy(&val,buf,std::min(static_cast<size_type>(stored_size),sizeof(wchar_t)));
		return val;
	}
	[[nodiscard]]
	std::string read_string(size_type offset) const {
		uint32_t len=read_le<uint32_t>(offset);
		size_type str_offset=offset+sizeof(uint32_t);
		if (str_offset+len>data_.size()) throw std::out_of_range("read_string data truncated");
		return std::string(reinterpret_cast<const char*>(data_.data())+str_offset,static_cast<size_type>(len));
	}
	[[nodiscard]]
	std::string_view read_string_view(size_type offset) const {
		uint32_t len=read_le<uint32_t>(offset);
		size_type str_offset=offset+sizeof(uint32_t);
		if (str_offset+len>data_.size()) throw std::out_of_range("read_string_view data truncated");
		return std::string_view(reinterpret_cast<const char*>(data_.data())+str_offset,static_cast<size_type>(len));
	}
	[[nodiscard]]
	std::vector<uint8_t> read_bytes_vec(size_type offset,size_type len) const {
		if (offset+len>data_.size()) throw std::out_of_range("read_bytes_vec out of range");
		return std::vector<uint8_t>(data_.begin()+offset,data_.begin()+offset+len);
	}
	[[nodiscard]]
	std::vector<uint8_t> read_length_prefixed_bytes(size_type offset) const {
		uint32_t len=read_le<uint32_t>(offset);
		size_type data_offset=offset+sizeof(uint32_t);
		if (data_offset+len>data_.size()) throw std::out_of_range("read_length_prefixed_bytes data truncated");
		return std::vector<uint8_t>(data_.begin()+data_offset,data_.begin()+data_offset+len);
	}
	[[nodiscard]]
	flat_buffer read_flat_buffer(size_type offset) const {
		uint32_t len=read_le<uint32_t>(offset);
		size_type data_offset=offset+sizeof(uint32_t);
		if (data_offset+len>data_.size()) throw std::out_of_range("read_flat_buffer data truncated");
		return flat_buffer(data_.data()+data_offset,static_cast<size_type>(len));
	}

	template <typename _Tp>
	[[nodiscard]]
	_Tp get(size_type offset=0) const {
		if constexpr (std::is_same<std::decay_t<_Tp>,bool>::value) {
			return read_bool(offset);
		} else if constexpr (std::is_same<std::decay_t<_Tp>,int8_t>::value) {
			return read_int8(offset);
		} else if constexpr (std::is_same<std::decay_t<_Tp>,uint8_t>::value) {
			return read_uint8(offset);
		} else if constexpr (std::is_same<std::decay_t<_Tp>,int16_t>::value) {
			return read_int16(offset);
		} else if constexpr (std::is_same<std::decay_t<_Tp>,uint16_t>::value) {
			return read_uint16(offset);
		} else if constexpr (std::is_same<std::decay_t<_Tp>,int32_t>::value) {
			return read_int32(offset);
		} else if constexpr (std::is_same<std::decay_t<_Tp>,uint32_t>::value) {
			return read_uint32(offset);
		} else if constexpr (std::is_same<std::decay_t<_Tp>,int64_t>::value) {
			return read_int64(offset);
		} else if constexpr (std::is_same<std::decay_t<_Tp>,uint64_t>::value) {
			return read_uint64(offset);
		} else if constexpr (std::is_same<std::decay_t<_Tp>,float>::value) {
			return read_float(offset);
		} else if constexpr (std::is_same<std::decay_t<_Tp>,double>::value) {
			return read_double(offset);
		} else if constexpr (std::is_same<std::decay_t<_Tp>,long>::value) {
			return read_long(offset);
		} else if constexpr (std::is_same<std::decay_t<_Tp>,unsigned long>::value) {
			return read_ulong(offset);
		} else if constexpr (std::is_same<std::decay_t<_Tp>,long double>::value) {
			return read_long_double(offset);
		} else if constexpr (std::is_same<std::decay_t<_Tp>,wchar_t>::value) {
			return read_wchar(offset);
		} else if constexpr (std::is_same<std::decay_t<_Tp>,std::string>::value) {
			return read_string(offset);
		} else if constexpr (std::is_same<std::decay_t<_Tp>,std::string_view>::value) {
			return read_string_view(offset);
		} else if constexpr (std::is_same<std::decay_t<_Tp>,std::vector<uint8_t>>::value) {
			return read_length_prefixed_bytes(offset);
		} else if constexpr (std::is_same<std::decay_t<_Tp>,flat_buffer>::value) {
			return read_flat_buffer(offset);
		} else if constexpr (is_serializer_valid_v<std::decay_t<_Tp>>) {
			return flat_buffer_serializer<std::decay_t<_Tp>>::deserialize(*this,offset);
		} else if constexpr (has_adl_deserialize_v<std::decay_t<_Tp>>) {
			std::decay_t<_Tp> result;
			deserialize_from_flat_buffer(*this,offset,result);
			return result;
		} else {
			static_assert(sizeof(std::decay_t<_Tp>)==0,"flat_buffer::get: no deserializer found for type. Provide a flat_buffer_serializer<T> specialization or a deserialize_from_flat_buffer(const flat_buffer&,size_type,std::decay_t<_Tp>&) ADL function.");
		}
	}

	template <typename _Tp>
	[[nodiscard]]
	const _Tp* get_ptr(size_type offset) const noexcept {
		static_assert(std::is_arithmetic<_Tp>::value,"get_ptr only supports arithmetic types.");
		if (offset+sizeof(_Tp)>data_.size()) return nullptr;
		return reinterpret_cast<const _Tp*>(data_.data()+offset);
	}
	template <typename _Tp>
	[[nodiscard]]
	_Tp* get_ptr(size_type offset) noexcept {
		static_assert(std::is_arithmetic<_Tp>::value,"get_ptr only supports arithmetic types.");
		if (offset+sizeof(_Tp)>data_.size()) return nullptr;
		return reinterpret_cast<_Tp*>(data_.data()+offset);
	}

	template <typename _Tp>
	[[nodiscard]]
	const _Tp& get_ref(size_type offset) const {
		static_assert(std::is_arithmetic<_Tp>::value,"get_ref only supports arithmetic types.");
		if (offset+sizeof(_Tp)>data_.size()) throw std::out_of_range("get_ref out of range");
		return *reinterpret_cast<const _Tp*>(data_.data()+offset);
	}
	template <typename _Tp>
	[[nodiscard]]
	_Tp& get_ref(size_type offset) {
		static_assert(std::is_arithmetic<_Tp>::value,"get_ref only supports arithmetic types.");
		if (offset+sizeof(_Tp)>data_.size()) throw std::out_of_range("get_ref out of range");
		return *reinterpret_cast<_Tp*>(data_.data()+offset);
	}

	template <typename _Tp>
	[[nodiscard]]
	_Tp value(size_type offset,const _Tp& default_val) const noexcept {
		try {
			return get<_Tp>(offset);
		} catch (...) {
			return default_val;
		}
	}
	template <typename _Tp>
	[[nodiscard]]
	_Tp value(size_type offset,_Tp&& default_val) const noexcept {
		try {
			return get<_Tp>(offset);
		} catch (...) {
			return std::forward<_Tp>(default_val);
		}
	}

	[[nodiscard]]
	iterator find(uint8_t byte) noexcept {
		return std::find(data_.begin(),data_.end(),byte);
	}
	[[nodiscard]]
	const_iterator find(uint8_t byte) const noexcept {
		return std::find(data_.begin(),data_.end(),byte);
	}
	[[nodiscard]]
	iterator find(const uint8_t* pattern,size_type pattern_len) noexcept {
		if (pattern_len==0) return data_.begin();
		if (pattern_len>data_.size()) return data_.end();
		return std::search(data_.begin(),data_.end(),pattern,pattern+pattern_len);
	}
	[[nodiscard]]
	const_iterator find(const uint8_t* pattern,size_type pattern_len) const noexcept {
		if (pattern_len==0) return data_.begin();
		if (pattern_len>data_.size()) return data_.end();
		return std::search(data_.begin(),data_.end(),pattern,pattern+pattern_len);
	}
	[[nodiscard]]
	iterator find(const flat_buffer& pattern) noexcept {
		return find(pattern.data_.data(),pattern.data_.size());
	}
	[[nodiscard]]
	const_iterator find(const flat_buffer& pattern) const noexcept {
		return find(pattern.data_.data(),pattern.data_.size());
	}
	[[nodiscard]]
	iterator find(const std::vector<uint8_t>& pattern) noexcept {
		return find(pattern.data(),pattern.size());
	}
	[[nodiscard]]
	const_iterator find(const std::vector<uint8_t>& pattern) const noexcept {
		return find(pattern.data(),pattern.size());
	}
	[[nodiscard]]
	iterator find(std::string_view pattern) noexcept {
		return find(reinterpret_cast<const uint8_t*>(pattern.data()),pattern.size());
	}
	[[nodiscard]]
	const_iterator find(std::string_view pattern) const noexcept {
		return find(reinterpret_cast<const uint8_t*>(pattern.data()),pattern.size());
	}

#if __cplusplus>=_STDEX_CPP20_VERSION
	[[nodiscard]]
	iterator find(std::span<const uint8_t> pattern) noexcept {
		return find(pattern.data(),pattern.size());
	}
	[[nodiscard]]
	const_iterator find(std::span<const uint8_t> pattern) const noexcept {
		return find(pattern.data(),pattern.size());
	}
#endif

	[[nodiscard]]
	size_type find_offset(uint8_t byte,size_type start=0) const noexcept {
		for (size_type i=start; i<data_.size();i++) {
			if (data_[i]==byte) return i;
		}
		return npos;
	}
	[[nodiscard]]
	size_type find_offset(const uint8_t* pattern,size_type pattern_len,size_type start=0) const noexcept {
		if (pattern_len==0) return start;
		if (start>=data_.size()) return npos;
		auto it=std::search(data_.begin()+start,data_.end(),pattern,pattern+pattern_len);
		if (it==data_.end()) return npos;
		return static_cast<size_type>(std::distance(data_.begin(),it));
	}
	[[nodiscard]]
	size_type find_offset(const flat_buffer& pattern,size_type start=0) const noexcept {
		return find_offset(pattern.data_.data(),pattern.data_.size(),start);
	}
	[[nodiscard]]
	size_type find_offset(std::string_view pattern,size_type start=0) const noexcept {
		return find_offset(reinterpret_cast<const uint8_t*>(pattern.data()),pattern.size(),start);
	}
	[[nodiscard]]
	size_type rfind_offset(uint8_t byte,size_type start=npos) const noexcept {
		if (data_.empty()) return npos;
		size_type from=start==npos?data_.size()-1:std::min(start,data_.size()-1);
		for (size_type i=from+1;i-->0;) {
			if (data_[i]==byte) return i;
		}
		return npos;
	}
	[[nodiscard]]
	size_type rfind_offset(const uint8_t* pattern,size_type pattern_len,size_type start=npos) const noexcept {
		if (pattern_len==0) return start==npos?data_.size():std::min(start,data_.size());
		if (pattern_len>data_.size()) return npos;
		size_type max_start=data_.size()-pattern_len;
		size_type from=(start==npos)?max_start:std::min(start,max_start);
		for (size_type i=from+1;i-->0;) {
			if (std::memcmp(data_.data()+i,pattern,pattern_len)==0) return i;
		}
		return npos;
	}
	[[nodiscard]]
	size_type rfind_offset(const flat_buffer& pattern,size_type start=npos) const noexcept {
		return rfind_offset(pattern.data_.data(),pattern.data_.size(),start);
	}
	[[nodiscard]]
	size_type rfind_offset(std::string_view pattern,size_type start=npos) const noexcept {
		return rfind_offset(reinterpret_cast<const uint8_t*>(pattern.data()),pattern.size(),start);
	}

	[[nodiscard]]
	bool contains(uint8_t byte) const noexcept {
		return find_offset(byte)!=npos;
	}
	[[nodiscard]]
	bool contains(const uint8_t* pattern,size_type pattern_len) const noexcept {
		return find_offset(pattern,pattern_len)!=npos;
	}
	[[nodiscard]]
	bool contains(const flat_buffer& pattern) const noexcept {
		return find_offset(pattern)!=npos;
	}
	[[nodiscard]]
	bool contains(std::string_view pattern) const noexcept {
		return find_offset(pattern)!=npos;
	}
	[[nodiscard]]
	size_type count(uint8_t byte) const noexcept {
		return static_cast<size_type>(std::count(data_.begin(),data_.end(),byte));
	}

	[[nodiscard]]
	bool operator ==(const flat_buffer& other) const noexcept {
		return data_==other.data_;
	}
	[[nodiscard]]
	bool operator !=(const flat_buffer& other) const noexcept {
		return !(*this==other);
	}
	[[nodiscard]]
	bool operator <(const flat_buffer& other) const noexcept {
		return data_<other.data_;
	}
	[[nodiscard]]
	bool operator <=(const flat_buffer& other) const noexcept {
		return !(other<*this);
	}
	[[nodiscard]]
	bool operator >(const flat_buffer& other) const noexcept {
		return other<*this;
	}
	[[nodiscard]]
	bool operator >=(const flat_buffer& other) const noexcept {
		return !(*this<other);
	}

#if __cplusplus>=_STDEX_CPP20_VERSION
	[[nodiscard]]
	auto operator <=>(const flat_buffer& other) const noexcept {
		return data_<=>other.data_;
	}
#endif

	[[nodiscard]]
	flat_buffer operator +(const flat_buffer& other) const {
		flat_buffer result(*this);
		result.append(other);
		return result;
	}
	flat_buffer& operator +=(const flat_buffer& other) {
		append(other);
		return *this;
	}
	flat_buffer& operator +=(uint8_t byte) {
		push_back(byte);
		return *this;
	}
	template <typename _Tp>
	flat_buffer& operator <<(const _Tp& val) {
		write(val);
		return *this;
	}

	[[nodiscard]]
	size_type read_pos() const noexcept { return read_pos_; }
	void set_read_pos(size_type pos) {
		if (pos>data_.size()) throw std::out_of_range("set_read_pos out of range");
		read_pos_=pos;
	}
	void reset_read_pos() noexcept { read_pos_=0; }

	[[nodiscard]]
	size_type remaining() const noexcept {
		return data_.size()>read_pos_?data_.size()-read_pos_:0;
	}
	[[nodiscard]]
	bool has_remaining(size_type n=1) const noexcept {
		return remaining()>=n;
	}
	void skip(size_type n) {
		if (read_pos_+n>data_.size()) throw std::out_of_range("skip exceeds buffer size");
		read_pos_+=n;
	}

	template <typename _Tp>
	[[nodiscard]]
	_Tp read_next() {
		_Tp val=get<std::decay_t<_Tp>>(read_pos_);
		read_pos_+=serialized_size_of<std::decay_t<_Tp>>(read_pos_);
		return val;
	}
	template <typename _Tp>
	[[nodiscard]]
	size_type serialized_size_of(size_type offset) const {
		if constexpr (std::is_same<std::decay_t<_Tp>,bool>::value || std::is_same<std::decay_t<_Tp>,int8_t>::value || std::is_same<std::decay_t<_Tp>,uint8_t>::value) {
			return 1;
		} else if constexpr (std::is_same<std::decay_t<_Tp>,int16_t>::value || std::is_same<std::decay_t<_Tp>,uint16_t>::value) {
			return 2;
		} else if constexpr (std::is_same<std::decay_t<_Tp>,int32_t>::value || std::is_same<std::decay_t<_Tp>,uint32_t>::value) {
			return 4;
		} else if constexpr (std::is_same<std::decay_t<_Tp>,int64_t>::value || std::is_same<std::decay_t<_Tp>,uint64_t>::value) {
			return 8;
		} else if constexpr (std::is_same<std::decay_t<_Tp>,float>::value) {
			return 4;
		} else if constexpr (std::is_same<std::decay_t<_Tp>,double>::value) {
			return 8;
		} else if constexpr (std::is_same<std::decay_t<_Tp>,long>::value || std::is_same<std::decay_t<_Tp>,unsigned long>::value || std::is_same<std::decay_t<_Tp>,long double>::value || std::is_same<std::decay_t<_Tp>,wchar_t>::value) {
			if (offset+2>data_.size()) throw std::out_of_range("serialized_size_of header truncated");
			uint8_t byte_size=data_[offset+1];
			return static_cast<size_type>(3)+byte_size;
		} else if constexpr (std::is_same<std::decay_t<_Tp>,std::string>::value || std::is_same<std::decay_t<_Tp>,std::string_view>::value || std::is_same<std::decay_t<_Tp>,std::vector<uint8_t>>::value || std::is_same<std::decay_t<_Tp>,flat_buffer>::value) {
			uint32_t len=read_le<uint32_t>(offset);
			return sizeof(uint32_t)+static_cast<size_type>(len);
		} else if constexpr (serializer_has_size_v<std::decay_t<_Tp>>) {
			return flat_buffer_serializer<std::decay_t<_Tp>>::serialized_size(*this,offset);
		} else if constexpr (has_adl_serialized_size_v<std::decay_t<_Tp>>) {
			return serialized_size_in_flat_buffer(*this,offset,static_cast<std::decay_t<_Tp>*>(nullptr));
		} else {
			static_assert(sizeof(std::decay_t<_Tp>)==0,"flat_buffer::serialized_size_of: no serialized_size found for type. Provide flat_buffer_serializer<T>::serialized_size or serialized_size_in_flat_buffer(const flat_buffer&,size_type,T*) ADL function.");
			return 0;
		}
	}

	template <typename _UnaryOp>
	flat_buffer& transform_bytes(_UnaryOp&& op) {
		std::transform(data_.begin(),data_.end(),data_.begin(),std::forward<_UnaryOp>(op));
		return *this;
	}

	template <typename _Pred>
	[[nodiscard]]
	flat_buffer filter_bytes(_Pred&& pred) const {
		flat_buffer result;
		std::copy_if(data_.begin(),data_.end(),std::back_inserter(result.data_),std::forward<_Pred>(pred));
		return result;
	}

	template <typename _UnaryFunc>
	void for_each(_UnaryFunc&& func) const {
		std::for_each(data_.begin(),data_.end(),std::forward<_UnaryFunc>(func));
	}

	template <typename _UnaryFunc>
	void for_each(_UnaryFunc&& func) {
		std::for_each(data_.begin(),data_.end(),std::forward<_UnaryFunc>(func));
	}

	[[nodiscard]]
	std::string to_hex(bool uppercase=false,std::string_view separator="") const {
		static const char lower_chars[]="0123456789abcdef";
		static const char upper_chars[]="0123456789ABCDEF";
		const char* hex_chars=uppercase?upper_chars:lower_chars;
		std::string result;
		result.reserve(data_.size()*(2+separator.size()));
		for (size_type i=0;i<data_.size();i++) {
			if (i>0 && !separator.empty()) result+=std::string(separator);
			result+=hex_chars[(data_[i]>>4)&0x0Fu];
			result+=hex_chars[data_[i]&0x0Fu];
		}
		return result;
	}
	[[nodiscard]]
	static flat_buffer from_hex(std::string_view hex) {
		flat_buffer result;
		auto hex_char_to_nibble=[](char c)->uint8_t {
			if (c>='0' && c<='9') return static_cast<uint8_t>(c-'0');
			if (c>='a' && c<='f') return static_cast<uint8_t>(c-'a'+10);
			if (c>='A' && c<='F') return static_cast<uint8_t>(c-'A'+10);
			throw std::invalid_argument("invalid hex character");
			return 0u;
		};
		std::string clean;
		clean.reserve(hex.size());
		for (char c:hex) {
			if ((c>='0' && c<='9') || (c>='a' && c<='f') || (c>='A' && c<='F')) clean+=c;
		}
		if (clean.size()%2!=0) throw std::invalid_argument("odd hex string length");
		result.data_.reserve(clean.size()/2);
		for (size_type i=0;i<clean.size();i+=2) {
			uint8_t byte=static_cast<uint8_t>((hex_char_to_nibble(clean[i])<<4)|hex_char_to_nibble(clean[i+1]));
			result.data_.push_back(byte);
		}
		return result;
	}

	[[nodiscard]]
	std::string to_base64() const {
		static const crypto::base64 codec;
		return codec.encode(data_);
	}

	[[nodiscard]]
	static flat_buffer from_base64(std::string_view b64) {
		static const crypto::base64 codec;
		crypto::base::decode_options options;
		options.padding=crypto::base::PM_ALLOW;
		options.casing=crypto::base::CM_STRICT;
		options.err=crypto::base::EM_STRICT;
		options.ignore_whitespace=false;
		options.accept_url_safe_alias=false;
		try {
			auto decoded=codec.decode(b64,options);
			flat_buffer result;
			result.data_.assign(decoded.begin(),decoded.end());
			return result;
		} catch (const std::exception& e) {
			throw;
		}
	}

	friend void swap(flat_buffer& lhs,flat_buffer& rhs) noexcept {
		lhs.swap(rhs);
	}

	friend std::ostream& operator <<(std::ostream& os,const flat_buffer& buf) {
		os<<buf.to_hex(false," ");
		return os;
	}
};

template <>
struct flat_buffer_serializer<bool> {
	static void serialize(flat_buffer& buf,bool val) {
		buf.write(val);
	}
	static bool deserialize(const flat_buffer& buf,flat_buffer::size_type offset) {
		return buf.read_bool(offset);
	}
	static flat_buffer::size_type serialized_size(const flat_buffer&,flat_buffer::size_type) noexcept {
		return 1;
	}
};

template <>
struct flat_buffer_serializer<int8_t> {
	static void serialize(flat_buffer& buf,int8_t val) { buf.write(val); }
	static int8_t deserialize(const flat_buffer& buf,flat_buffer::size_type offset) {
		return buf.read_int8(offset);
	}
	static flat_buffer::size_type serialized_size(const flat_buffer&,flat_buffer::size_type) noexcept {
		return 1;
	}
};

template<>
struct flat_buffer_serializer<uint8_t> {
	static void serialize(flat_buffer& buf,uint8_t val) { buf.write(val); }
	static uint8_t deserialize(const flat_buffer& buf,flat_buffer::size_type offset) {
		return buf.read_uint8(offset);
	}
	static flat_buffer::size_type serialized_size(const flat_buffer&,flat_buffer::size_type) noexcept {
		return 1;
	}
};

template <>
struct flat_buffer_serializer<int16_t> {
	static void serialize(flat_buffer& buf,int16_t val) { buf.write(val); }
	static int16_t deserialize(const flat_buffer& buf,flat_buffer::size_type offset) {
		return buf.read_int16(offset);
	}
	static flat_buffer::size_type serialized_size(const flat_buffer&,flat_buffer::size_type) noexcept {
		return 2;
	}
};

template <>
struct flat_buffer_serializer<uint16_t> {
	static void serialize(flat_buffer& buf,uint16_t val) { buf.write(val); }
	static uint16_t deserialize(const flat_buffer& buf,flat_buffer::size_type offset) {
		return buf.read_uint16(offset);
	}
	static flat_buffer::size_type serialized_size(const flat_buffer&,flat_buffer::size_type) noexcept {
		return 2;
	}
};

template <>
struct flat_buffer_serializer<int32_t> {
	static void serialize(flat_buffer& buf,int32_t val) { buf.write(val); }
	static int32_t deserialize(const flat_buffer& buf,flat_buffer::size_type offset) {
		return buf.read_int32(offset);
	}
	static flat_buffer::size_type serialized_size(const flat_buffer&,flat_buffer::size_type) noexcept {
		return 4;
	}
};

template <>
struct flat_buffer_serializer<uint32_t> {
	static void serialize(flat_buffer& buf,uint32_t val) { buf.write(val); }
	static uint32_t deserialize(const flat_buffer& buf,flat_buffer::size_type offset) {
		return buf.read_uint32(offset);
	}
	static flat_buffer::size_type serialized_size(const flat_buffer&,flat_buffer::size_type) noexcept {
		return 4;
	}
};

template <>
struct flat_buffer_serializer<int64_t> {
	static void serialize(flat_buffer& buf,int64_t val) { buf.write(val); }
	static int64_t deserialize(const flat_buffer& buf,flat_buffer::size_type offset) {
		return buf.read_int64(offset);
	}
	static flat_buffer::size_type serialized_size(const flat_buffer&,flat_buffer::size_type) noexcept {
		return 8;
	}
};

template <>
struct flat_buffer_serializer<uint64_t> {
	static void serialize(flat_buffer& buf,uint64_t val) { buf.write(val); }
	static uint64_t deserialize(const flat_buffer& buf,flat_buffer::size_type offset) {
		return buf.read_uint64(offset);
	}
	static flat_buffer::size_type serialized_size(const flat_buffer&,flat_buffer::size_type) noexcept {
		return 8;
	}
};

template <>
struct flat_buffer_serializer<float> {
	static void serialize(flat_buffer& buf,float val) { buf.write(val); }
	static float deserialize(const flat_buffer& buf,flat_buffer::size_type offset) {
		return buf.read_float(offset);
	}
	static flat_buffer::size_type serialized_size(const flat_buffer&,flat_buffer::size_type) noexcept {
		return 4;
	}
};

template <>
struct flat_buffer_serializer<double> {
	static void serialize(flat_buffer& buf,double val) { buf.write(val); }
	static double deserialize(const flat_buffer& buf,flat_buffer::size_type offset) {
		return buf.read_double(offset);
	}
	static flat_buffer::size_type serialized_size(const flat_buffer&,flat_buffer::size_type) noexcept {
		return 8;
	}
};

template <>
struct flat_buffer_serializer<long> {
	static void serialize(flat_buffer& buf,long val) { buf.write(val); }
	static long deserialize(const flat_buffer& buf,flat_buffer::size_type offset) {
		return buf.read_long(offset);
	}
	static flat_buffer::size_type serialized_size(const flat_buffer& buf,flat_buffer::size_type offset) {
		if (offset+2>buf.size()) throw std::out_of_range("flat_buffer_serializer<long>::serialized_size header truncated");
		uint8_t byte_size=buf[offset+1];
		return static_cast<flat_buffer::size_type>(3)+byte_size;
	}
};

template <>
struct flat_buffer_serializer<unsigned long> {
	static void serialize(flat_buffer& buf,unsigned long val) { buf.write(val); }
	static unsigned long deserialize(const flat_buffer& buf,flat_buffer::size_type offset) {
		return buf.read_ulong(offset);
	}
	static flat_buffer::size_type serialized_size(const flat_buffer& buf,flat_buffer::size_type offset) {
		if (offset+2>buf.size()) throw std::out_of_range("flat_buffer_serializer<unsigned long>::serialized_size header truncated");
		uint8_t byte_size=buf[offset+1];
		return static_cast<flat_buffer::size_type>(3)+byte_size;
	}
};

template <>
struct flat_buffer_serializer<long double> {
	static void serialize(flat_buffer& buf,long double val) { buf.write(val); }
	static long double deserialize(const flat_buffer& buf,flat_buffer::size_type offset) {
		return buf.read_long_double(offset);
	}
	static flat_buffer::size_type serialized_size(const flat_buffer& buf,flat_buffer::size_type offset) {
		if (offset+2>buf.size()) throw std::out_of_range("flat_buffer_serializer<long double>::serialized_size header truncated");
		uint8_t byte_size=buf[offset+1];
		return static_cast<flat_buffer::size_type>(3)+byte_size;
	}
};

template <>
struct flat_buffer_serializer<wchar_t> {
	static void serialize(flat_buffer& buf,wchar_t val) { buf.write(val); }
	static wchar_t deserialize(const flat_buffer& buf,flat_buffer::size_type offset) {
		return buf.read_wchar(offset);
	}
	static flat_buffer::size_type serialized_size(const flat_buffer& buf,flat_buffer::size_type offset) {
		if (offset+2>buf.size()) throw std::out_of_range("flat_buffer_serializer<wchar_t>::serialized_size header truncated");
		uint8_t byte_size=buf[offset+1];
		return static_cast<flat_buffer::size_type>(3)+byte_size;
	}
};

template <>
struct flat_buffer_serializer<std::string> {
	static void serialize(flat_buffer& buf,const std::string& val) { buf.write(val); }
	static std::string deserialize(const flat_buffer& buf,flat_buffer::size_type offset) {
		return buf.read_string(offset);
	}
	static flat_buffer::size_type serialized_size(const flat_buffer& buf,flat_buffer::size_type offset) {
		uint32_t len=buf.read_uint32(offset);
		return sizeof(uint32_t)+static_cast<flat_buffer::size_type>(len);
	}
};

template <>
struct flat_buffer_serializer<std::string_view> {
	static void serialize(flat_buffer& buf,std::string_view val) { buf.write(val); }
	static std::string_view deserialize(const flat_buffer& buf,flat_buffer::size_type offset) {
		return buf.read_string_view(offset);
	}
	static flat_buffer::size_type serialized_size(const flat_buffer& buf,flat_buffer::size_type offset) {
		uint32_t len=buf.read_uint32(offset);
		return sizeof(uint32_t)+static_cast<flat_buffer::size_type>(len);
	}
};

template <>
struct flat_buffer_serializer<std::vector<uint8_t>> {
	static void serialize(flat_buffer& buf,const std::vector<uint8_t>& val) { buf.write(val); }
	static std::vector<uint8_t> deserialize(const flat_buffer& buf,flat_buffer::size_type offset) {
		return buf.read_length_prefixed_bytes(offset);
	}
	static flat_buffer::size_type serialized_size(const flat_buffer& buf,flat_buffer::size_type offset) {
		uint32_t len=buf.read_uint32(offset);
		return sizeof(uint32_t)+static_cast<flat_buffer::size_type>(len);
	}
};

template <>
struct flat_buffer_serializer<flat_buffer> {
	static void serialize(flat_buffer& buf,const flat_buffer& val) { buf.write(val); }
	static flat_buffer deserialize(const flat_buffer& buf,flat_buffer::size_type offset) {
		return buf.read_flat_buffer(offset);
	}
	static flat_buffer::size_type serialized_size(const flat_buffer& buf,flat_buffer::size_type offset) {
		uint32_t len=buf.read_uint32(offset);
		return sizeof(uint32_t)+static_cast<flat_buffer::size_type>(len);
	}
};


template <typename _Tp>
struct flat_buffer_serializer<_Tp,std::enable_if_t<std::is_enum<_Tp>::value>> {
	static void serialize(flat_buffer& buf,_Tp val) {
		flat_buffer_serializer<std::underlying_type_t<_Tp>>::serialize(buf,static_cast<std::underlying_type_t<_Tp>>(val));
	}
	static _Tp deserialize(const flat_buffer& buf,flat_buffer::size_type offset) {
		return static_cast<_Tp>(flat_buffer_serializer<std::underlying_type_t<_Tp>>::deserialize(buf,offset));
	}
	static flat_buffer::size_type serialized_size(const flat_buffer& buf,flat_buffer::size_type offset) {
		return flat_buffer_serializer<std::underlying_type_t<_Tp>>::serialized_size(buf,offset);
	}
};

#if __cplusplus>=_STDEX_CPP23_VERSION
template <>
struct flat_buffer_serializer<std::float16_t> {
	static void serialize(flat_buffer& buf,std::float16_t val) {
		uint16_t bits=0;
		std::memcpy(&bits,&val,sizeof(uint16_t));
		flat_buffer_serializer<uint16_t>::serialize(buf,bits);
	}
	static std::float16_t deserialize(const flat_buffer& buf,flat_buffer::size_type offset) {
		uint16_t bits=flat_buffer_serializer<uint16_t>::deserialize(buf,offset);
		std::float16_t val;
		std::memcpy(&val,&bits,sizeof(uint16_t));
		return val;
	}
	static flat_buffer::size_type serialized_size(const flat_buffer&,flat_buffer::size_type) noexcept {
		return 2;
	}
};

template <>
struct flat_buffer_serializer<std::float32_t> {
	static void serialize(flat_buffer& buf,std::float32_t val) {
		uint32_t bits=0;
		std::memcpy(&bits,&val,sizeof(uint32_t));
		flat_buffer_serializer<uint32_t>::serialize(buf,bits);
	}
	static std::float32_t deserialize(const flat_buffer& buf,flat_buffer::size_type offset) {
		uint32_t bits=flat_buffer_serializer<uint32_t>::deserialize(buf,offset);
		std::float32_t val;
		std::memcpy(&val,&bits,sizeof(uint32_t));
		return val;
	}
	static flat_buffer::size_type serialized_size(const flat_buffer&,flat_buffer::size_type) noexcept {
		return 4;
	}
};

template <>
struct flat_buffer_serializer<std::float64_t> {
	static void serialize(flat_buffer& buf,std::float64_t val) {
		uint64_t bits=0;
		std::memcpy(&bits,&val,sizeof(uint64_t));
		flat_buffer_serializer<uint64_t>::serialize(buf,bits);
	}
	static std::float64_t deserialize(const flat_buffer& buf,flat_buffer::size_type offset) {
		uint64_t bits=flat_buffer_serializer<uint64_t>::deserialize(buf,offset);
		std::float64_t val;
		std::memcpy(&val,&bits,sizeof(uint64_t));
		return val;
	}
	static flat_buffer::size_type serialized_size(const flat_buffer&,flat_buffer::size_type) noexcept {
		return 8;
	}
};

template <>
struct flat_buffer_serializer<std::float128_t> {
	static void serialize(flat_buffer& buf,std::float128_t val) {
		// float128_t = 16字节，按元数据协议存储（平台相关大小）
		static_assert(sizeof(std::float128_t)<=255,"float128_t size exceeds portable protocol limit");
		// 复用 portable_raw 协议思路：直接写16字节 + 字节序标记
		// 此处借道 write_raw + 手动构造头部，因为 write_portable_raw 是 private
		// 通过 flat_buffer_serializer<uint8_t> 逐字节写头
		flat_buffer_serializer<uint8_t>::serialize(buf,static_cast<uint8_t>(FBTT_LONG_DOUBLE));
		flat_buffer_serializer<uint8_t>::serialize(buf,static_cast<uint8_t>(sizeof(std::float128_t)));
		flat_buffer_serializer<uint8_t>::serialize(buf,is_little_endian_host()?0x00u:0x01u);
		buf.write_raw(&val,sizeof(std::float128_t));
	}
	static std::float128_t deserialize(const flat_buffer& buf,flat_buffer::size_type offset) {
		// 手动读取元数据头
		if (offset+3>buf.size()) throw std::out_of_range("flat_buffer_serializer<float128_t>::deserialize header truncated");
		uint8_t byte_size=buf[offset+1];
		uint8_t endian_flag=buf[offset+2];
		flat_buffer::size_type data_offset=offset+3;
		if (data_offset+byte_size>buf.size()) throw std::out_of_range("flat_buffer_serializer<float128_t>::deserialize data truncated");
		uint8_t raw[sizeof(std::float128_t)]={};
		flat_buffer::size_type copy_size=std::min(static_cast<flat_buffer::size_type>(byte_size),sizeof(std::float128_t));
		std::memcpy(raw,buf.data()+data_offset,copy_size);
		bool src_is_little=endian_flag==0x00u;
		normalize_endian(raw,copy_size,src_is_little);
		std::float128_t val;
		std::memcpy(&val,raw,sizeof(std::float128_t));
		return val;
	}
	static flat_buffer::size_type serialized_size(const flat_buffer& buf,flat_buffer::size_type offset) {
		if (offset+2>buf.size()) throw std::out_of_range("flat_buffer_serializer<float128_t>::serialized_size header truncated");
		uint8_t byte_size=buf[offset+1];
		return static_cast<flat_buffer::size_type>(3)+byte_size;
	}
};
#endif

template <typename... _Bufs>
[[nodiscard]]
flat_buffer concat_buffers(const flat_buffer& first,const _Bufs& ...rest) {
	flat_buffer result(first);
	(result.append(rest),...);
	return result;
}

[[nodiscard]]
inline bool starts_with(const flat_buffer& buf,const flat_buffer& prefix) {
	if (prefix.size()>buf.size()) return false;
	return std::equal(prefix.begin(),prefix.end(),buf.begin());
}

[[nodiscard]]
inline bool starts_with(const flat_buffer& buf,std::string_view prefix) {
	if (prefix.size()>buf.size()) return false;
	return std::equal(prefix.begin(),prefix.end(),reinterpret_cast<const char*>(buf.data()));
}

[[nodiscard]]
inline bool ends_with(const flat_buffer& buf,const flat_buffer& suffix) {
	if (suffix.size()>buf.size()) return false;
	return std::equal(suffix.begin(),suffix.end(),buf.begin()+static_cast<std::ptrdiff_t>(buf.size()-suffix.size()));
}

[[nodiscard]]
inline bool ends_with(const flat_buffer& buf,std::string_view suffix) {
	if (suffix.size()>buf.size()) return false;
	return std::equal(suffix.begin(),suffix.end(),reinterpret_cast<const char*>(buf.data())+buf.size()-suffix.size());
}

[[nodiscard]]
inline flat_buffer xor_buffers(const flat_buffer& lhs,const flat_buffer& rhs) {
	flat_buffer result;
	flat_buffer::size_type len=std::min(lhs.size(),rhs.size());
	result.reserve(len);
	for (flat_buffer::size_type i=0;i<len;i++) result.push_back(lhs[i]^rhs[i]);
	return result;
}

[[nodiscard]]
inline flat_buffer and_buffers(const flat_buffer& lhs,const flat_buffer& rhs) {
	flat_buffer result;
	flat_buffer::size_type len=std::min(lhs.size(),rhs.size());
	result.reserve(len);
	for (flat_buffer::size_type i=0;i<len;i++) result.push_back(lhs[i]&rhs[i]);
	return result;
}

[[nodiscard]]
inline flat_buffer or_buffers(const flat_buffer& lhs,const flat_buffer& rhs) {
	flat_buffer result;
	flat_buffer::size_type len=std::min(lhs.size(),rhs.size());
	result.reserve(len);
	for (flat_buffer::size_type i=0;i<len;i++) result.push_back(lhs[i]|rhs[i]);
	return result;
}

[[nodiscard]]
inline flat_buffer not_buffer(const flat_buffer& buf) {
	flat_buffer result(buf);
	for (flat_buffer::size_type i=0;i<result.size();i++) result[i]=~result[i];
	return result;
}

}

}

}

#endif