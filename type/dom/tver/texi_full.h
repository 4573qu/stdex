//Last Modified At 2026/07/11
//@Version 1.0.0.0
#ifndef _STDEX_TYPE_DOM_EXI_H_
#define _STDEX_TYPE_DOM_EXI_H_ 1

#include <climits>
#include <cstdint>
#include <cstring>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "../../bitwise/bit_reader.h"//At Least 2.2
#include "../../bitwise/bit_writer.h"//At Least 2.2
#include "../../crypto/deflate.h"//At Least 1.0
#include "xml.h"//At Least 1.0

//EXI(W3C Efficient XML Interchange)·schema-less模式实现。
//范围与理由(不实现的部分是与本架构的明确不匹配,而非省略):
//1.实现:内建文法(built-in grammars,含文法学习)、位打包/字节对齐两种对齐、EXI头与选项文档
//  (选项文档按规范以EXI Options schema的strict文法编码,此处为该固定schema手工特化自动机)、
//  字符串表全套(URI/前缀/局部名/全局与局部值分区,valueMaxLength/valuePartitionCapacity)、
//  Preserve.{comments,pis,prefixes}保真。解码产出SAX事件流并复用xml_sax_dom_builder建树,
//  编码为树遍历发事件——EXI的事件模型与xml.h的SAX层一一对应,bitwise的BO_MSBYTE+n-bit读写
//  与EXI位打包模型一一对应,这是实现schema-less半边的架构依据。
//2.不实现·schema-informed文法:需要完整XSD处理器(类型/粒子/出现次数/替代组),stdex无该
//  基础设施且XSD本身比EXI更大;流头声明了schema(schemaId非nil)时解码明确抛出。
//3.实现·compression/pre-compression对齐(§9):块/结构信道/按限定名值信道/小信道合流分组,
//  DEFLATE经crypto::deflate(DSF_RAW即RFC1951裸流)。deflate的公开解压API不报告输入消耗量,
//  而压缩模式是多条裸流背靠背;deflate_decompressor::done()对喂入前缀长度单调,故以
//  指数探进+二分定界(O(n·log n)),不向crypto层要求接口变更。预压缩模式信道由内容自定界。
//4.DT/ER拆分对待:DT(DOCTYPE)经document_info往返——与xml文本侧"DOCTYPE入info不进树"对称,
//  DT四元组(name/public/system/内部子集)与原文之间做词法桥接;EXI的DT本身只承载四元组,
//  声明内部空白不逐字保留(格式属性,非本实现额外损失)。ER(实体引用)不实现:kind虽可扩展,
//  但xml的parser永远产不出实体引用节点(实体词法层展开,自定义实体无DTD即无从声明),私加
//  ER节点种类会造出只有本编解码器能产能吃、xml自身parse/dump均无法往返的孤岛kind,破坏
//  "派生kind必须经本记法自身往返"的家族不变量(XDT_CDATA/COMMENT/PROCINST均满足),也违反
//  通用性约束;且ER语义依赖DTD实体声明,处理声明=半个DTD处理器,与schema-informed需要半个
//  XSD处理器是同一层次错误。带ER事件的流在遇到该事件码时明确拒绝;Preserve.dtd本身接受,
//  其在元素文法二级码中的ER槽位按规范参与计宽以保证互操作。
//5.不实现·fragment文法(与xml.h根唯一的文档模型冲突)、selfContained(需编解码器状态快照)、
//  datatypeRepresentationMap(依赖schema);解码遇到即抛出。
//6.CDATA无EXI对应,按XPath数据模型同口径编码为文本(内容无损,CDATA身份有损);
//  Preserve.lexicalValues仅对类型化值有意义,schema-less下接受该标志并自然满足。
//7.命名空间:EXI为Infoset(URI,local)模型而xml.h为词法保真模型。编码经xmlns祖先栈解析前缀
//  (不可解析抛invalid_argument),Preserve.prefixes下前缀与xmlns经NS事件无损往返,
//  关闭时解码合成ns1,ns2...前缀并补xmlns声明(Infoset等价,词法形式有损)。

namespace stdex {

namespace type {

namespace basic_exi {

enum exi_alignment : int {
	EA_BIT_PACKED,
	EA_BYTE_ALIGNED,
	EA_PRE_COMPRESSION,
	EA_COMPRESSION,
};

//编码选项。默认值取"往返友好"配置(保注释/PI/前缀,位打包,写出选项文档);
//规范默认(全不保真)可用spec_defaults()。
struct exi_options {
	exi_alignment alignment=EA_BIT_PACKED;
	bool preserve_comments=true;
	bool preserve_pis=true;
	bool preserve_prefixes=true;
	bool preserve_lexical_values=false;
	bool preserve_dtd=false;//DT经document_info往返;ER事件仍在遇到时拒绝
	bool include_cookie=false;
	bool include_options=true;//头部选项文档存在位;false时解码端按规范默认解释
	std::size_t value_max_length=static_cast<std::size_t>(-1);
	std::size_t value_partition_capacity=static_cast<std::size_t>(-1);
	std::size_t block_size=1000000;//§9:每块值数上限,仅对(pre-)compression有效

	static exi_options spec_defaults() noexcept {
		exi_options result;
		result.preserve_comments=false;
		result.preserve_pis=false;
		result.preserve_prefixes=false;
		result.include_options=false;
		return result;
	}
};

_STDEX_DOM_TPL_DECLARATION
class exi : public basic_xml::xml<_Int,_Float,_Boolean,_String,_Array,_Object,_Allocator> {
public:
	using base_t=basic_xml::xml<_Int,_Float,_Boolean,_String,_Array,_Object,_Allocator>;
	using dom_t=typename base_t::base_t;
	using int_t=typename base_t::int_t;
	using float_t=typename base_t::float_t;
	using boolean_t=typename base_t::boolean_t;
	using string_t=typename base_t::string_t;
	using size_type=typename base_t::size_type;
	using binary_t=std::vector<std::uint8_t>;
	using sax_t=basic_xml::xml_sax<exi>;
	using document_info_t=typename base_t::document_info_t;

	using base_t::base_t;
	using base_t::operator =;

	exi()=default;
	~exi() override=default;

	exi(const exi&)=default;
	exi(exi&&) noexcept=default;

	exi& operator =(const exi&)=default;
	exi& operator =(exi&&)=default;

	exi(const base_t& other) : base_t(other) { }
	exi(base_t&& other) noexcept : base_t(std::move(other)) { }
	exi(const dom_t& other) : base_t(other) { }
	exi(dom_t&& other) noexcept : base_t(std::move(other)) { }

private:
	static std::size_t byte_position(const bitwise::bit_reader& reader) noexcept {
		return reader.tell_bits()/CHAR_BIT;
	}
	[[noreturn]]
	static void decode_fail(const bitwise::bit_reader& reader,const std::string& message) {
		throw std::runtime_error(message+" at byte "+std::to_string(byte_position(reader)));
	}
	static std::size_t bits_for(std::size_t count) noexcept {//ceil(log2(count)),count<2→0
		std::size_t result=0;
		std::size_t limit=1;
		while (limit<count) {
			limit<<=1;
			result++;
		}
		return result;
	}

	//---基元编码(§10):n-bit无符号(位打包=MSB-first原始位;字节对齐=最少八位组,低位组在前),
	//无界无符号整数=7-bit续位组(两种对齐相同),布尔=1-bit,字符串=码点数+逐码点无符号整数---
	static void write_nbit(bitwise::bit_writer& writer,exi_alignment alignment,std::size_t bits,std::uint64_t value) {
		if (bits==0) return;
		if (alignment==EA_BIT_PACKED) {
			writer.write_bits(bits,value);
			return;
		}
		const std::size_t octets=(bits+CHAR_BIT-1)/CHAR_BIT;
		for (std::size_t i=0;i<octets;i++) writer.write_u8(static_cast<std::uint8_t>((value>>(i*CHAR_BIT))&0xFF));
	}
	static std::uint64_t read_nbit(bitwise::bit_reader& reader,exi_alignment alignment,std::size_t bits) {
		if (bits==0) return 0;
		if (alignment==EA_BIT_PACKED) return reader.read_bits<std::uint64_t>(bits);
		const std::size_t octets=(bits+CHAR_BIT-1)/CHAR_BIT;
		std::uint64_t result=0;
		for (std::size_t i=0;i<octets;i++) result|=static_cast<std::uint64_t>(reader.read_u8())<<(i*CHAR_BIT);
		return result;
	}
	static void write_unsigned(bitwise::bit_writer& writer,std::uint64_t value) {
		while (true) {
			const std::uint8_t group=static_cast<std::uint8_t>(value&0x7F);
			value>>=7;
			if (value) writer.write_u8(group|0x80);
			else {
				writer.write_u8(group);
				return;
			}
		}
	}
	static std::uint64_t read_unsigned(bitwise::bit_reader& reader) {
		std::uint64_t result=0;
		std::size_t shift=0;
		while (true) {
			const std::uint8_t group=reader.read_u8();
			result|=static_cast<std::uint64_t>(group&0x7F)<<shift;
			if (!(group&0x80)) return result;
			shift+=7;
			if (shift>63) decode_fail(reader,"Unsigned integer is too long");
		}
	}
	static void write_boolean(bitwise::bit_writer& writer,exi_alignment alignment,bool value) {
		write_nbit(writer,alignment,1,value?1:0);
	}
	static bool read_boolean(bitwise::bit_reader& reader,exi_alignment alignment) {
		return read_nbit(reader,alignment,1)!=0;
	}
	//UTF-8字节串↔码点序列(EXI字符串按码点计长、逐码点编码)。
	static void decode_codepoints(const string_t& text,std::vector<std::uint32_t>& out) {
		const unsigned char* first=reinterpret_cast<const unsigned char*>(text.data());
		const unsigned char* const last=first+text.size();
		while (first<last) {
			const unsigned char c=*first;
			std::uint32_t cp=0;
			std::size_t extra=0;
			if (c<0x80) cp=c;
			else if ((c&0xE0)==0xC0) {
				cp=c&0x1F;
				extra=1;
			} else if ((c&0xF0)==0xE0) {
				cp=c&0x0F;
				extra=2;
			} else if ((c&0xF8)==0xF0) {
				cp=c&0x07;
				extra=3;
			} else throw std::invalid_argument("Invalid UTF-8 byte in string content");
			if (static_cast<std::size_t>(last-first)<extra+1) throw std::invalid_argument("Truncated UTF-8 sequence in string content");
			for (std::size_t i=1;i<=extra;i++) {
				if ((first[i]&0xC0)!=0x80) throw std::invalid_argument("Invalid UTF-8 continuation byte in string content");
				cp=(cp<<6)|(first[i]&0x3F);
			}
			out.push_back(cp);
			first+=extra+1;
		}
	}
	static void append_codepoint(string_t& out,std::uint32_t cp) {
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
	static void write_codepoints(bitwise::bit_writer& writer,const string_t& text) {
		std::vector<std::uint32_t> codepoints;
		decode_codepoints(text,codepoints);
		for (const auto it:codepoints) write_unsigned(writer,it);
	}
	static void write_string(bitwise::bit_writer& writer,const string_t& text) {//长度+码点
		std::vector<std::uint32_t> codepoints;
		decode_codepoints(text,codepoints);
		write_unsigned(writer,codepoints.size());
		for (const auto it:codepoints) write_unsigned(writer,it);
	}
	static string_t read_codepoints(bitwise::bit_reader& reader,std::uint64_t count) {
		string_t result;
		for (std::uint64_t i=0;i<count;i++) append_codepoint(result,static_cast<std::uint32_t>(read_unsigned(reader)));
		return result;
	}
	static string_t read_string(bitwise::bit_reader& reader) {
		return read_codepoints(reader,read_unsigned(reader));
	}
	static string_t make_string(const char* text) {
		return string_t(text,text+std::strlen(text));
	}

	//---字符串表(§D):URI/前缀/局部名分区(编号自增),值分区(全局+按限定名局部,
	//valueMaxLength限制入表长度,valuePartitionCapacity按规范以id=总数mod容量循环覆盖全局槽)---
	struct string_table {
		std::vector<string_t> uris;
		std::vector<std::vector<string_t>> prefixes;//按URI分区
		std::vector<std::vector<string_t>> locals;//按URI分区
		std::vector<string_t> global_values;
		std::map<string_t,std::size_t> global_index;
		std::map<std::pair<std::size_t,std::size_t>,std::vector<string_t>> local_values;//键=(uri,local)
		std::size_t total_values=0;
		std::size_t value_max_length=static_cast<std::size_t>(-1);
		std::size_t value_partition_capacity=static_cast<std::size_t>(-1);

		string_table() {//schema-less预置项(§D.1-D.3)
			uris.push_back(string_t());
			uris.push_back(make_string("http://www.w3.org/XML/1998/namespace"));
			uris.push_back(make_string("http://www.w3.org/2001/XMLSchema-instance"));
			prefixes.resize(3);
			prefixes[0].push_back(string_t());
			prefixes[1].push_back(make_string("xml"));
			prefixes[2].push_back(make_string("xsi"));
			locals.resize(3);
			locals[1].push_back(make_string("base"));
			locals[1].push_back(make_string("id"));
			locals[1].push_back(make_string("lang"));
			locals[1].push_back(make_string("space"));
			locals[2].push_back(make_string("nil"));
			locals[2].push_back(make_string("type"));
		}
		bool find_uri(const string_t& uri,std::size_t& index) const {
			for (std::size_t i=0;i<uris.size();i++) {
				if (uris[i]==uri) {
					index=i;
					return true;
				}
			}
			return false;
		}
		std::size_t add_uri(string_t uri) {
			uris.push_back(std::move(uri));
			prefixes.emplace_back();
			locals.emplace_back();
			return uris.size()-1;
		}
		static bool find_in(const std::vector<string_t>& table,const string_t& value,std::size_t& index) {
			for (std::size_t i=0;i<table.size();i++) {
				if (table[i]==value) {
					index=i;
					return true;
				}
			}
			return false;
		}
		//值入表(命中失败后调用):同时进局部与全局分区,容量循环覆盖全局槽。
		void add_value(std::size_t uri,std::size_t local,const string_t& value) {
			std::vector<std::uint32_t> probe;
			std::size_t codepoints=0;
			for (const auto it:value) {
				if ((static_cast<unsigned char>(it)&0xC0)!=0x80) codepoints++;
			}
			if (codepoints>value_max_length) return;
			if (value_partition_capacity==0) return;
			local_values[std::make_pair(uri,local)].push_back(value);
			if (value_partition_capacity==static_cast<std::size_t>(-1)) {
				global_index[value]=global_values.size();
				global_values.push_back(value);
			} else {
				const std::size_t slot=total_values%value_partition_capacity;
				if (slot<global_values.size()) {
					global_index.erase(global_values[slot]);
					global_index[value]=slot;
					global_values[slot]=value;
				} else {
					global_index[value]=global_values.size();
					global_values.push_back(value);
				}
			}
			total_values++;
		}
	};

	//---内建文法(§8.4.3):状态=已学习产生式表+固定二级项;学习即在码0处插入---
	enum event_kind : int {
		EV_SE,
		EV_AT,
		EV_CH,
		EV_EE,
	};
	struct learned_production {
		event_kind kind=EV_SE;
		std::size_t uri=0;
		std::size_t local=0;
	};
	struct grammar_state {
		std::vector<learned_production> learned;

		bool find(event_kind kind,std::size_t uri,std::size_t local,std::size_t& code) const {
			for (std::size_t i=0;i<learned.size();i++) {
				if (learned[i].kind==kind && (kind==EV_CH || kind==EV_EE || (learned[i].uri==uri && learned[i].local==local))) {
					code=i;
					return true;
				}
			}
			return false;
		}
		void learn(event_kind kind,std::size_t uri,std::size_t local) {//新产生式获得码0,余者顺延
			learned_production production;
			production.kind=kind;
			production.uri=uri;
			production.local=local;
			learned.insert(learned.begin(),std::move(production));
		}
	};
	struct element_grammar {
		grammar_state start_tag;
		grammar_state content;
	};

	//---EXI头(§5):可选"$EXI"甜饼,判别位"10",选项存在位,版本(1bit最终版标志+4bit组)---
	static void write_header(bitwise::bit_writer& writer,const exi_options& options) {
		if (options.include_cookie) {
			writer.write_u8('$');
			writer.write_u8('E');
			writer.write_u8('X');
			writer.write_u8('I');
		}
		writer.write_bits(2,static_cast<std::uint64_t>(2));//"10"
		writer.write_bits(1,options.include_options?1:0);
		writer.write_bits(1,0);//最终版
		writer.write_bits(4,0);//版本1
		if (options.include_options) write_options_document(writer,options);
	}
	static exi_options read_header(bitwise::bit_reader& reader) {
		if (reader.remaining_bits()>=32) {
			const std::uint32_t cookie=reader.peek_bits<std::uint32_t>(32);
			if (cookie==0x24455849UL) reader.read_bits<std::uint32_t>(32);//"$EXI"
		}
		if (reader.read_bits<std::uint8_t>(2)!=2) decode_fail(reader,"Missing EXI distinguishing bits");
		const bool has_options=reader.read_bits<std::uint8_t>(1)!=0;
		if (reader.read_bits<std::uint8_t>(1)!=0) decode_fail(reader,"EXI preview versions are not supported");
		std::uint64_t version=0;
		while (true) {
			const std::uint64_t group=reader.read_bits<std::uint8_t>(4);
			version+=group;
			if (group!=15) break;
		}
		if (version!=0) decode_fail(reader,"Unsupported EXI format version");
		exi_options result=exi_options::spec_defaults();
		if (has_options) read_options_document(reader,result);
		return result;
	}

	//---选项文档(§5.4):以EXI Options schema的strict文法编码,位打包+规范默认选项。
	//此处为该固定schema的手工特化自动机(exip同型做法),不引入通用schema引擎。
	//结构:header{lesscommon{uncommon{alignment{byte|pre-compress},selfContained,valueMaxLength,
	//valuePartitionCapacity,datatypeRepresentationMap*},preserve{dtd,prefixes,lexicalValues,
	//comments,pis},blockSize},common{compression,fragment,schemaId},strict}。
	//strict顺序文法的事件码=当前位置起剩余候选(含EE)的序号,位宽=ceil(log2(候选数)):
	//读写共用同一游标推进逻辑,避免手写固定位宽出错。
	struct option_sequence {
		std::size_t total=0;//顺序中的元素个数(不含EE)
		std::size_t position=0;

		std::size_t remaining() const noexcept {
			return total-position+1;//+1为EE
		}
		//写出"选择下标为index的元素"(index为顺序中的绝对序号),并推进游标越过它。
		void emit(bitwise::bit_writer& writer,std::size_t index) {
			write_nbit(writer,EA_BIT_PACKED,bits_for(remaining()),index-position);
			position=index+1;
		}
		void emit_end(bitwise::bit_writer& writer) {
			write_nbit(writer,EA_BIT_PACKED,bits_for(remaining()),total-position);
		}
		//读侧:返回绝对序号,或total表示EE。
		std::size_t next(bitwise::bit_reader& reader) {
			const std::uint64_t code=read_nbit(reader,EA_BIT_PACKED,bits_for(remaining()));
			const std::size_t absolute=position+static_cast<std::size_t>(code);
			if (absolute<total) position=absolute+1;
			return absolute;
		}
	};

	static void write_options_document(bitwise::bit_writer& writer,const exi_options& options) {
		const exi_options defaults=exi_options::spec_defaults();
		const bool byte_or_pre=(options.alignment==EA_BYTE_ALIGNED||options.alignment==EA_PRE_COMPRESSION);
		const bool need_vml=options.value_max_length!=defaults.value_max_length;
		const bool need_vpc=options.value_partition_capacity!=defaults.value_partition_capacity;
		const bool need_uncommon=byte_or_pre||need_vml||need_vpc;
		const bool need_preserve=options.preserve_comments||options.preserve_pis||options.preserve_prefixes||options.preserve_lexical_values||options.preserve_dtd;
		const bool compressed=(options.alignment==EA_COMPRESSION||options.alignment==EA_PRE_COMPRESSION);
		const bool need_block=compressed&&options.block_size!=defaults.block_size;
		const bool need_lesscommon=need_uncommon||need_preserve||need_block;
		const bool need_common=(options.alignment==EA_COMPRESSION);
		option_sequence header_seq;
		header_seq.total=3;//lesscommon(0),common(1),strict(2)
		if (need_lesscommon) {
			header_seq.emit(writer,0);
			option_sequence lesscommon_seq;
			lesscommon_seq.total=3;//uncommon(0),preserve(1),blockSize(2)
			if (need_uncommon) {
				lesscommon_seq.emit(writer,0);
				option_sequence uncommon_seq;
				uncommon_seq.total=5;//alignment(0),selfContained(1),valueMaxLength(2),valuePartitionCapacity(3),dtrm(4)
				if (byte_or_pre) {
					uncommon_seq.emit(writer,0);
					write_nbit(writer,EA_BIT_PACKED,1,options.alignment==EA_PRE_COMPRESSION?1:0);//choice[byte,pre-compress]
					//被选中子元素为空元素(EE 0bit),alignment自身EE 0bit
				}
				if (need_vml) {
					uncommon_seq.emit(writer,2);
					write_unsigned(writer,options.value_max_length);//unsignedInt类型化内容:CH(0bit)+值+EE(0bit)
				}
				if (need_vpc) {
					uncommon_seq.emit(writer,3);
					write_unsigned(writer,options.value_partition_capacity);
				}
				uncommon_seq.emit_end(writer);
			}
			if (need_preserve) {
				lesscommon_seq.emit(writer,1);
				option_sequence preserve_seq;
				preserve_seq.total=5;//dtd(0),prefixes(1),lexicalValues(2),comments(3),pis(4)
				if (options.preserve_dtd) preserve_seq.emit(writer,0);
				if (options.preserve_prefixes) preserve_seq.emit(writer,1);
				if (options.preserve_lexical_values) preserve_seq.emit(writer,2);
				if (options.preserve_comments) preserve_seq.emit(writer,3);
				if (options.preserve_pis) preserve_seq.emit(writer,4);
				preserve_seq.emit_end(writer);
			}
			if (need_block) {
				lesscommon_seq.emit(writer,2);
				write_unsigned(writer,options.block_size);
			}
			lesscommon_seq.emit_end(writer);
		}
		if (need_common) {
			header_seq.emit(writer,1);
			option_sequence common_seq;
			common_seq.total=3;//compression(0),fragment(1),schemaId(2)
			common_seq.emit(writer,0);//compression为空元素
			common_seq.emit_end(writer);
		}
		header_seq.emit_end(writer);
	}
	static void read_options_document(bitwise::bit_reader& reader,exi_options& options) {
		options=exi_options::spec_defaults();
		bool has_alignment=false;
		bool has_compression=false;
		option_sequence header_seq;
		header_seq.total=3;
		while (true) {
			const std::size_t header_item=header_seq.next(reader);
			if (header_item==3) break;//EE(header)
			if (header_item==0) {//lesscommon
				option_sequence lesscommon_seq;
				lesscommon_seq.total=3;
				while (true) {
					const std::size_t lesscommon_item=lesscommon_seq.next(reader);
					if (lesscommon_item==3) break;
					if (lesscommon_item==0) {//uncommon
						option_sequence uncommon_seq;
						uncommon_seq.total=5;
						while (true) {
							const std::size_t uncommon_item=uncommon_seq.next(reader);
							if (uncommon_item==5) break;
							if (uncommon_item==0) {
								has_alignment=true;
								options.alignment=read_nbit(reader,EA_BIT_PACKED,1)?EA_PRE_COMPRESSION:EA_BYTE_ALIGNED;
							} else if (uncommon_item==1) decode_fail(reader,"EXI selfContained elements are not supported (codec state snapshots are out of scope)");
							else if (uncommon_item==2) options.value_max_length=static_cast<std::size_t>(read_unsigned(reader));
							else if (uncommon_item==3) options.value_partition_capacity=static_cast<std::size_t>(read_unsigned(reader));
							else decode_fail(reader,"EXI datatypeRepresentationMap requires schema-informed grammars which are out of scope");
						}
					} else if (lesscommon_item==1) {//preserve
						option_sequence preserve_seq;
						preserve_seq.total=5;
						while (true) {
							const std::size_t preserve_item=preserve_seq.next(reader);
							if (preserve_item==5) break;
							if (preserve_item==0) options.preserve_dtd=true;//DT经document_info;ER事件遇到时拒绝
							else if (preserve_item==1) options.preserve_prefixes=true;
							else if (preserve_item==2) options.preserve_lexical_values=true;
							else if (preserve_item==3) options.preserve_comments=true;
							else options.preserve_pis=true;
						}
					} else options.block_size=static_cast<std::size_t>(read_unsigned(reader));
				}
			} else if (header_item==1) {//common
				option_sequence common_seq;
				common_seq.total=3;
				while (true) {
					const std::size_t common_item=common_seq.next(reader);
					if (common_item==3) break;
					if (common_item==0) has_compression=true;
					else if (common_item==1) decode_fail(reader,"EXI fragment grammars conflict with the single-root document model of xml.h");
					else {//schemaId:nillable string:[AT(xsi:nil)(0),CH(1)]→1bit
						if (read_nbit(reader,EA_BIT_PACKED,1)==0) {
							const bool nil=read_nbit(reader,EA_BIT_PACKED,1)!=0;//xsd:boolean=1bit
							if (!nil) decode_fail(reader,"schemaId with xsi:nil='false' and empty content is not a valid schema-less declaration");
							//nil=true:显式schema-less,元素空:EE(0bit)
						} else {
							const string_t schema_id=read_value_plain(reader);
							static_cast<void>(schema_id);
							decode_fail(reader,"Schema-informed EXI streams are not supported (stdex has no XSD infrastructure; schemaId declared a schema)");
						}
					}
				}
			} else decode_fail(reader,"EXI strict mode requires schema-informed grammars which are out of scope");
		}
		if (has_compression) {
			if (has_alignment) decode_fail(reader,"EXI options declare both compression and an alignment");
			options.alignment=EA_COMPRESSION;
		}
	}
	//选项文档内schemaId的字符串值:新鲜值表上必为miss(len+2)编码。
	static string_t read_value_plain(bitwise::bit_reader& reader) {
		const std::uint64_t head=read_unsigned(reader);
		if (head<2) decode_fail(reader,"Unexpected value table hit in the options document");
		return read_codepoints(reader,head-2);
	}

	//---限定名/前缀/值编码(§7.1,§7.3):URI=n-bit(0=miss+String字面),局部名=无符号头
	//(0=hit+n-bit id,L>0=miss长L-1),值=无符号头(0=局部hit,1=全局hit,h≥2=miss长h-2)---
	static void write_uri(bitwise::bit_writer& writer,exi_alignment alignment,string_table& tables,const string_t& uri,std::size_t& uri_id) {
		std::size_t found=0;
		if (tables.find_uri(uri,found)) {
			write_nbit(writer,alignment,bits_for(tables.uris.size()+1),found+1);
			uri_id=found;
			return;
		}
		write_nbit(writer,alignment,bits_for(tables.uris.size()+1),0);
		write_string(writer,uri);
		uri_id=tables.add_uri(uri);
	}
	static std::size_t read_uri(bitwise::bit_reader& reader,exi_alignment alignment,string_table& tables) {
		const std::uint64_t code=read_nbit(reader,alignment,bits_for(tables.uris.size()+1));
		if (code==0) return tables.add_uri(read_string(reader));
		const std::size_t index=static_cast<std::size_t>(code-1);
		if (index>=tables.uris.size()) decode_fail(reader,"URI compact identifier out of range");
		return index;
	}
	static void write_local(bitwise::bit_writer& writer,exi_alignment alignment,string_table& tables,std::size_t uri_id,const string_t& local,std::size_t& local_id) {
		std::vector<string_t>& partition=tables.locals[uri_id];
		std::size_t found=0;
		if (string_table::find_in(partition,local,found)) {
			write_unsigned(writer,0);
			write_nbit(writer,alignment,bits_for(partition.size()),found);
			local_id=found;
			return;
		}
		std::vector<std::uint32_t> codepoints;
		decode_codepoints(local,codepoints);
		write_unsigned(writer,codepoints.size()+1);
		for (const auto it:codepoints) write_unsigned(writer,it);
		partition.push_back(local);
		local_id=partition.size()-1;
	}
	static std::size_t read_local(bitwise::bit_reader& reader,exi_alignment alignment,string_table& tables,std::size_t uri_id) {
		std::vector<string_t>& partition=tables.locals[uri_id];
		const std::uint64_t head=read_unsigned(reader);
		if (head==0) {
			const std::uint64_t index=read_nbit(reader,alignment,bits_for(partition.size()));
			if (index>=partition.size()) decode_fail(reader,"Local name compact identifier out of range");
			return static_cast<std::size_t>(index);
		}
		partition.push_back(read_codepoints(reader,head-1));
		return partition.size()-1;
	}
	static void write_prefix_entry(bitwise::bit_writer& writer,exi_alignment alignment,string_table& tables,std::size_t uri_id,const string_t& prefix) {//NS事件内的前缀:局部名式miss/hit
		std::vector<string_t>& partition=tables.prefixes[uri_id];
		std::size_t found=0;
		if (string_table::find_in(partition,prefix,found)) {
			write_unsigned(writer,0);
			write_nbit(writer,alignment,bits_for(partition.size()),found);
			return;
		}
		std::vector<std::uint32_t> codepoints;
		decode_codepoints(prefix,codepoints);
		write_unsigned(writer,codepoints.size()+1);
		for (const auto it:codepoints) write_unsigned(writer,it);
		partition.push_back(prefix);
	}
	static string_t read_prefix_entry(bitwise::bit_reader& reader,exi_alignment alignment,string_table& tables,std::size_t uri_id) {
		std::vector<string_t>& partition=tables.prefixes[uri_id];
		const std::uint64_t head=read_unsigned(reader);
		if (head==0) {
			const std::uint64_t index=read_nbit(reader,alignment,bits_for(partition.size()));
			if (index>=partition.size()) decode_fail(reader,"Prefix compact identifier out of range");
			return partition[index];
		}
		string_t prefix=read_codepoints(reader,head-1);
		partition.push_back(prefix);
		return prefix;
	}
	//SE/AT的前缀分量(Preserve.prefixes):n-bit指向该URI前缀分区;未入分区时写0,
	//由NS事件的local-element-ns旗标在解码侧覆盖(与exificient互操作口径一致)。
	static void write_prefix_component(bitwise::bit_writer& writer,exi_alignment alignment,string_table& tables,std::size_t uri_id,const string_t& prefix) {
		const std::vector<string_t>& partition=tables.prefixes[uri_id];
		std::size_t found=0;
		if (!string_table::find_in(partition,prefix,found)) found=0;
		write_nbit(writer,alignment,bits_for(partition.size()),partition.empty()?0:found);
	}
	static string_t read_prefix_component(bitwise::bit_reader& reader,exi_alignment alignment,string_table& tables,std::size_t uri_id) {
		const std::vector<string_t>& partition=tables.prefixes[uri_id];
		const std::uint64_t index=read_nbit(reader,alignment,bits_for(partition.size()));
		if (partition.empty()) return string_t();
		if (index>=partition.size()) decode_fail(reader,"Prefix compact identifier out of range");
		return partition[index];
	}
	static void write_value_string(bitwise::bit_writer& writer,exi_alignment alignment,string_table& tables,std::size_t uri_id,std::size_t local_id,const string_t& text) {
		auto local_it=tables.local_values.find(std::make_pair(uri_id,local_id));
		if (local_it!=tables.local_values.end()) {
			std::size_t found=0;
			if (string_table::find_in(local_it->second,text,found)) {
				write_unsigned(writer,0);
				write_nbit(writer,alignment,bits_for(local_it->second.size()),found);
				return;
			}
		}
		auto global_it=tables.global_index.find(text);
		if (global_it!=tables.global_index.end()) {
			write_unsigned(writer,1);
			write_nbit(writer,alignment,bits_for(tables.global_values.size()),global_it->second);
			return;
		}
		std::vector<std::uint32_t> codepoints;
		decode_codepoints(text,codepoints);
		write_unsigned(writer,codepoints.size()+2);
		for (const auto it:codepoints) write_unsigned(writer,it);
		tables.add_value(uri_id,local_id,text);
	}
	static string_t read_value_string(bitwise::bit_reader& reader,exi_alignment alignment,string_table& tables,std::size_t uri_id,std::size_t local_id) {
		const std::uint64_t head=read_unsigned(reader);
		if (head==0) {
			std::vector<string_t>& partition=tables.local_values[std::make_pair(uri_id,local_id)];
			const std::uint64_t index=read_nbit(reader,alignment,bits_for(partition.size()));
			if (index>=partition.size()) decode_fail(reader,"Local value compact identifier out of range");
			return partition[index];
		}
		if (head==1) {
			const std::uint64_t index=read_nbit(reader,alignment,bits_for(tables.global_values.size()));
			if (index>=tables.global_values.size()) decode_fail(reader,"Global value compact identifier out of range");
			return tables.global_values[index];
		}
		string_t text=read_codepoints(reader,head-2);
		tables.add_value(uri_id,local_id,text);
		return text;
	}

	//---事件码布局(内建文法§8.4.3)。STC二级项:EE(0),AT*(1),[NS],SE*,CH,[CM/PI组];
	//EC二级项:SE*(0),CH(1),[组];DocContent二级仅[组](DT不支持故计数1→0bit);DocEnd二级为CM/PI---
	static bool fidelity_group(const exi_options& options) noexcept {
		return options.preserve_comments||options.preserve_pis;
	}
	static bool doc_escape(const exi_options& options) noexcept {//DocContent转义存在性:DT或CM/PI组
		return options.preserve_dtd||fidelity_group(options);
	}
	static std::size_t third_count(const exi_options& options) noexcept {
		return (options.preserve_comments&&options.preserve_pis)?2:1;
	}
	static std::size_t stc_second_count(const exi_options& options) noexcept {
		return 4+(options.preserve_prefixes?1:0)+(options.preserve_dtd?1:0)+(fidelity_group(options)?1:0);
	}
	static std::size_t stc_ns_index(const exi_options&) noexcept {
		return 2;
	}
	static std::size_t stc_se_index(const exi_options& options) noexcept {
		return 2+(options.preserve_prefixes?1:0);
	}
	static std::size_t stc_ch_index(const exi_options& options) noexcept {
		return 3+(options.preserve_prefixes?1:0);
	}
	static std::size_t stc_er_index(const exi_options& options) noexcept {
		return 4+(options.preserve_prefixes?1:0);
	}
	static std::size_t stc_group_index(const exi_options& options) noexcept {
		return 4+(options.preserve_prefixes?1:0)+(options.preserve_dtd?1:0);
	}
	static std::size_t ec_second_count(const exi_options& options) noexcept {
		return 2+(options.preserve_dtd?1:0)+(fidelity_group(options)?1:0);
	}
	static std::size_t ec_group_index(const exi_options& options) noexcept {
		return 2+(options.preserve_dtd?1:0);
	}
	static void write_third(bitwise::bit_writer& writer,exi_alignment alignment,const exi_options& options,bool is_pi) {
		write_nbit(writer,alignment,bits_for(third_count(options)),(options.preserve_comments&&options.preserve_pis)?(is_pi?1:0):0);
	}
	static bool read_third(bitwise::bit_reader& reader,exi_alignment alignment,const exi_options& options) {//返回是否PI
		const std::uint64_t code=read_nbit(reader,alignment,bits_for(third_count(options)));
		if (options.preserve_comments&&options.preserve_pis) return code!=0;
		return options.preserve_pis;
	}

	//---解码事件缓冲:统一"先全量解结构、再补值、后重放SAX"三段式(信道模式必需,
	//内联模式复用同一路径,值即时解入text)---
	enum decoded_kind : int {
		DK_SE,
		DK_EE,
		DK_AT,
		DK_CH,
		DK_NS,
		DK_CM,
		DK_PI,
		DK_DT,
	};
	struct decoded_event {
		decoded_kind kind=DK_SE;
		string_t uri{};
		string_t local{};
		string_t prefix{};
		bool ns_flag=false;
		string_t text{};
		string_t aux{};//DK_DT:系统标识
		std::ptrdiff_t value_slot=-1;//信道模式:块内值下标
	};
	struct pending_value {
		std::size_t uri=0;
		std::size_t local=0;
		string_t text{};
	};
	struct channel_layout {//块内信道:首现序
		std::vector<std::pair<std::size_t,std::size_t>> order;
		std::map<std::pair<std::size_t,std::size_t>,std::vector<std::size_t>> members;//值下标按信道

		void add(std::size_t uri,std::size_t local,std::size_t slot) {
			const std::pair<std::size_t,std::size_t> key(uri,local);
			auto it=members.find(key);
			if (it==members.end()) {
				order.push_back(key);
				members[key].push_back(slot);
			} else it->second.push_back(slot);
		}
	};

	//---编解码共享文法状态---
	struct codec_state {
		string_table tables;
		std::map<std::pair<std::size_t,std::size_t>,std::size_t> element_index;
		std::vector<element_grammar> grammar_pool;
		grammar_state document_content;

		std::size_t grammar_of(std::size_t uri,std::size_t local) {
			const std::pair<std::size_t,std::size_t> key(uri,local);
			auto it=element_index.find(key);
			if (it!=element_index.end()) return it->second;
			grammar_pool.emplace_back();
			element_index[key]=grammar_pool.size()-1;
			return grammar_pool.size()-1;
		}
	};

	//---编码器---
	struct encode_context {
		exi_options options;
		exi_alignment content_alignment=EA_BIT_PACKED;
		bool channelized=false;
		bitwise::bit_writer* writer=nullptr;//内联:整流;信道:当前块结构缓冲
		bitwise::bit_writer* output=nullptr;//信道:最终输出
		codec_state state;
		std::vector<pending_value> block_values;
		channel_layout layout;
	};
	static void emit_value(encode_context& ctx,std::size_t uri,std::size_t local,const string_t& text) {
		if (!ctx.channelized) {
			write_value_string(*ctx.writer,ctx.content_alignment,ctx.state.tables,uri,local,text);
			return;
		}
		pending_value value;
		value.uri=uri;
		value.local=local;
		value.text=text;
		ctx.layout.add(uri,local,ctx.block_values.size());
		ctx.block_values.push_back(std::move(value));
		if (ctx.block_values.size()>=ctx.options.block_size) flush_block(ctx);
	}
	//块装配(§9):值总数≤100→结构+全部信道合为一段;否则结构一段、小信道(≤100)合段、
	//大信道各自一段;compression对每段做RFC1951裸DEFLATE,pre-compression原样。
	static void flush_block(encode_context& ctx) {
		binary_t structure_bytes=std::move(*ctx.writer).buffer();
		*ctx.writer=bitwise::bit_writer(bitwise::BO_MSBYTE);
		std::vector<binary_t> streams;
		if (ctx.block_values.size()<=100) {
			bitwise::bit_writer combined(bitwise::BO_MSBYTE);
			combined.write_bytes(structure_bytes.data(),structure_bytes.size());
			for (const auto& key:ctx.layout.order) {
				for (const std::size_t slot:ctx.layout.members[key]) write_value_string(combined,EA_BYTE_ALIGNED,ctx.state.tables,ctx.block_values[slot].uri,ctx.block_values[slot].local,ctx.block_values[slot].text);
			}
			streams.push_back(std::move(combined).buffer());
		} else {
			streams.push_back(std::move(structure_bytes));
			bitwise::bit_writer small(bitwise::BO_MSBYTE);
			for (const auto& key:ctx.layout.order) {
				if (ctx.layout.members[key].size()>100) continue;
				for (const std::size_t slot:ctx.layout.members[key]) write_value_string(small,EA_BYTE_ALIGNED,ctx.state.tables,ctx.block_values[slot].uri,ctx.block_values[slot].local,ctx.block_values[slot].text);
			}
			streams.push_back(std::move(small).buffer());
			for (const auto& key:ctx.layout.order) {
				if (ctx.layout.members[key].size()<=100) continue;
				bitwise::bit_writer large(bitwise::BO_MSBYTE);
				for (const std::size_t slot:ctx.layout.members[key]) write_value_string(large,EA_BYTE_ALIGNED,ctx.state.tables,ctx.block_values[slot].uri,ctx.block_values[slot].local,ctx.block_values[slot].text);
				streams.push_back(std::move(large).buffer());
			}
		}
		for (const auto& it:streams) {
			if (ctx.options.alignment==EA_COMPRESSION) {
				const binary_t packed=crypto::deflate::compress(it.data(),it.size(),crypto::DL_NORMAL,crypto::DSF_RAW);
				ctx.output->write_bytes(packed.data(),packed.size());
			} else ctx.output->write_bytes(it.data(),it.size());
		}
		ctx.block_values.clear();
		ctx.layout=channel_layout();
	}
	//标量子节点文本化(与xml::dump同策略:容器请先走转换协议)。
	static string_t scalar_to_text(const typename base_t::base_t& node) {
		switch (static_cast<int>(node.type().get())) {
			case static_cast<int>(structure::DDT_STRING): return *node.template get_ptr<const string_t*>();
			case static_cast<int>(structure::DDT_INT): {
				const std::string text=std::to_string(static_cast<long long>(*node.template get_ptr<const int_t*>()));
				return string_t(text.begin(),text.end());
			}
			case static_cast<int>(structure::DDT_FLOAT): {
				char buffer[64];
				const double value=static_cast<double>(*node.template get_ptr<const float_t*>());
				int length=std::snprintf(buffer,sizeof(buffer),"%.15g",value);
				if (std::strtod(buffer,nullptr)!=value) length=std::snprintf(buffer,sizeof(buffer),"%.17g",value);
				return string_t(buffer,buffer+length);
			}
			case static_cast<int>(structure::DDT_BOOL): return (*node.template get_ptr<const boolean_t*>())?make_string("true"):make_string("false");
			default: throw std::invalid_argument("Unsupported node type "+std::to_string(static_cast<long long>(static_cast<int>(node.type())))+" in exi content (use the dom conversion protocol first)");
		}
	}
	struct ns_binding {
		string_t prefix{};
		string_t uri{};
	};
	static bool resolve_prefix(const std::vector<std::vector<ns_binding>>& scopes,const string_t& prefix,string_t& uri) {
		if (prefix==make_string("xml")) {
			uri=make_string("http://www.w3.org/XML/1998/namespace");
			return true;
		}
		for (std::size_t i=scopes.size();i>0;i--) {
			for (const auto& it:scopes[i-1]) {
				if (it.prefix==prefix) {
					uri=it.uri;
					return uri.empty()?prefix.empty():true;//xmlns=""撤销默认
				}
			}
		}
		uri.clear();
		return prefix.empty();
	}
	static bool split_qname(const string_t& qualified,string_t& prefix,string_t& local) {
		const std::size_t colon=qualified.find(':');
		if (colon==string_t::npos) {
			prefix.clear();
			local=qualified;
			return false;
		}
		prefix=qualified.substr(0,colon);
		local=qualified.substr(colon+1);
		return true;
	}
	//---DOCTYPE原文↔DT四元组(name/public/system/内部子集)的词法桥(引号感知,与xml.h的
	//scan_doctype同口径)。EXI的DT只承载四元组,声明内部空白不逐字保留(格式属性,任何EXI
	//实现皆如此,非本实现额外损失)---
	static void skip_doctype_blank(const string_t& text,std::size_t& pos) noexcept {
		while (pos<text.size()&&(text[pos]==' '||text[pos]=='\t'||text[pos]=='\r'||text[pos]=='\n')) pos++;
	}
	static string_t read_doctype_literal(const string_t& text,std::size_t& pos) {
		skip_doctype_blank(text,pos);
		if (pos>=text.size()||(text[pos]!='"'&&text[pos]!='\'')) throw std::invalid_argument("DOCTYPE external identifier requires a quoted literal");
		const typename string_t::value_type quote=text[pos];
		pos++;
		const std::size_t start=pos;
		while (pos<text.size()&&text[pos]!=quote) pos++;
		if (pos>=text.size()) throw std::invalid_argument("Unterminated literal in DOCTYPE declaration");
		const string_t result=text.substr(start,pos-start);
		pos++;
		return result;
	}
	static void split_doctype(const string_t& raw,string_t& name,string_t& public_id,string_t& system_id,string_t& subset) {
		std::size_t pos=0;
		skip_doctype_blank(raw,pos);
		const string_t keyword=make_string("<!DOCTYPE");
		if (raw.compare(pos,keyword.size(),keyword)!=0) throw std::invalid_argument("DOCTYPE text must begin with '<!DOCTYPE'");
		pos+=keyword.size();
		skip_doctype_blank(raw,pos);
		const std::size_t name_start=pos;
		while (pos<raw.size()&&raw[pos]!=' '&&raw[pos]!='\t'&&raw[pos]!='\r'&&raw[pos]!='\n'&&raw[pos]!='['&&raw[pos]!='>') pos++;
		name=raw.substr(name_start,pos-name_start);
		if (name.empty()) throw std::invalid_argument("DOCTYPE declaration requires a name");
		skip_doctype_blank(raw,pos);
		if (raw.compare(pos,6,make_string("PUBLIC"))==0) {
			pos+=6;
			public_id=read_doctype_literal(raw,pos);
			system_id=read_doctype_literal(raw,pos);
		} else if (raw.compare(pos,6,make_string("SYSTEM"))==0) {
			pos+=6;
			system_id=read_doctype_literal(raw,pos);
		}
		skip_doctype_blank(raw,pos);
		if (pos<raw.size()&&raw[pos]=='[') {
			pos++;
			const std::size_t subset_start=pos;
			int depth=1;
			typename string_t::value_type quote=0;
			while (pos<raw.size()) {
				const typename string_t::value_type c=raw[pos];
				if (quote) {
					if (c==quote) quote=0;
				} else if (c=='"'||c=='\'') quote=c;
				else if (c=='[') depth++;
				else if (c==']') {
					depth--;
					if (depth==0) break;
				}
				pos++;
			}
			if (depth!=0) throw std::invalid_argument("Unterminated internal subset in DOCTYPE declaration");
			subset=raw.substr(subset_start,pos-subset_start);
		}
	}
	static string_t quote_doctype_literal(const string_t& value) {
		string_t result;
		if (value.find('"')==string_t::npos) {
			result.push_back('"');
			result+=value;
			result.push_back('"');
			return result;
		}
		if (value.find('\'')!=string_t::npos) throw std::runtime_error("DOCTYPE literal contains both quote characters");
		result.push_back('\'');
		result+=value;
		result.push_back('\'');
		return result;
	}
	static string_t build_doctype(const string_t& name,const string_t& public_id,const string_t& system_id,const string_t& subset) {
		string_t result=make_string("<!DOCTYPE ");
		result+=name;
		if (!public_id.empty()) {
			result+=make_string(" PUBLIC ");
			result+=quote_doctype_literal(public_id);
			result.push_back(' ');
			result+=quote_doctype_literal(system_id);
		} else if (!system_id.empty()) {
			result+=make_string(" SYSTEM ");
			result+=quote_doctype_literal(system_id);
		}
		if (!subset.empty()) {
			result+=make_string(" [");
			result+=subset;
			result.push_back(']');
		}
		result.push_back('>');
		return result;
	}
	//元素编码:SE(父态)→NS(Preserve.prefixes)→AT→内容(CH/SE/CM/PI)→EE。
	static void encode_element(encode_context& ctx,const typename base_t::base_t& node,std::vector<std::vector<ns_binding>>& scopes,grammar_state& parent_state,bool parent_is_document,bool parent_in_content) {
		static_cast<void>(parent_in_content);
		bitwise::bit_writer& writer=*ctx.writer;
		const exi_alignment alignment=ctx.content_alignment;
		const exi_options& options=ctx.options;
		//收集本层xmlns与常规属性
		std::vector<ns_binding> bindings;
		std::vector<std::pair<string_t,string_t>> plain_attributes;//键原文,值文本
		for (const auto& it:base_t::attributes(node)) {
			const string_t xmlns=make_string("xmlns");
			if (it.first==xmlns) {
				ns_binding binding;
				binding.uri=scalar_to_text(it.second);
				bindings.push_back(std::move(binding));
			} else if (it.first.size()>6&&it.first.compare(0,6,make_string("xmlns:"))==0) {
				ns_binding binding;
				binding.prefix=it.first.substr(6);
				binding.uri=scalar_to_text(it.second);
				bindings.push_back(std::move(binding));
			} else plain_attributes.emplace_back(it.first,scalar_to_text(it.second));
		}
		scopes.push_back(bindings);
		string_t element_prefix;
		string_t element_local;
		split_qname(base_t::name(node),element_prefix,element_local);
		string_t element_uri;
		if (!resolve_prefix(scopes,element_prefix,element_uri)) throw std::invalid_argument("Undeclared namespace prefix '"+std::string(element_prefix.begin(),element_prefix.end())+"' on element '"+std::string(base_t::name(node).begin(),base_t::name(node).end())+"'");
		//SE事件:父态学习命中走一级码,否则SE(*)转义+限定名字面+学习
		std::size_t uri_id=0;
		std::size_t local_id=0;
		{
			std::size_t learned_code=0;
			bool hit=false;
			std::size_t probe_uri=0;
			if (ctx.state.tables.find_uri(element_uri,probe_uri)) {
				std::size_t probe_local=0;
				if (string_table::find_in(ctx.state.tables.locals[probe_uri],element_local,probe_local)) hit=parent_state.find(EV_SE,probe_uri,probe_local,learned_code);
			}
			if (parent_is_document) {
				const std::size_t part1_count=parent_state.learned.size()+1+(doc_escape(options)?1:0);
				if (hit) write_nbit(writer,alignment,bits_for(part1_count),learned_code);
				else {
					write_nbit(writer,alignment,bits_for(part1_count),parent_state.learned.size());//SE(*)
					write_uri(writer,alignment,ctx.state.tables,element_uri,uri_id);
					local_id=0;
					write_local(writer,alignment,ctx.state.tables,uri_id,element_local,local_id);
				}
			} else {
				//父元素态:STC part1=learned+转义;EC part1=learned+EE+转义
				const bool parent_content=parent_in_content;
				const std::size_t part1_count=parent_state.learned.size()+(parent_content?2:1);
				if (hit) write_nbit(writer,alignment,bits_for(part1_count),learned_code);
				else {
					write_nbit(writer,alignment,bits_for(part1_count),parent_state.learned.size()+(parent_content?1:0));//转义
					const std::size_t second_count=parent_content?ec_second_count(options):stc_second_count(options);
					const std::size_t second_index=parent_content?0:stc_se_index(options);
					write_nbit(writer,alignment,bits_for(second_count),second_index);
					write_uri(writer,alignment,ctx.state.tables,element_uri,uri_id);
					write_local(writer,alignment,ctx.state.tables,uri_id,element_local,local_id);
				}
			}
			if (hit) {
				uri_id=parent_state.learned[learned_code].uri;
				local_id=parent_state.learned[learned_code].local;
			} else parent_state.learn(EV_SE,uri_id,local_id);
			if (options.preserve_prefixes) write_prefix_component(writer,alignment,ctx.state.tables,uri_id,element_prefix);
		}
		const std::size_t grammar_id=ctx.state.grammar_of(uri_id,local_id);
		bool in_content=false;
		//NS事件(不学习,恒为二级码)
		if (options.preserve_prefixes) {
			for (const auto& it:bindings) {
				grammar_state& st=ctx.state.grammar_pool[grammar_id].start_tag;
				write_nbit(writer,alignment,bits_for(st.learned.size()+1),st.learned.size());
				write_nbit(writer,alignment,bits_for(stc_second_count(options)),stc_ns_index(options));
				std::size_t ns_uri_id=0;
				write_uri(writer,alignment,ctx.state.tables,it.uri,ns_uri_id);
				write_prefix_entry(writer,alignment,ctx.state.tables,ns_uri_id,it.prefix);
				write_boolean(writer,alignment,it.prefix==element_prefix&&it.uri==element_uri);
			}
		}
		//AT事件
		for (const auto& it:plain_attributes) {
			string_t attribute_prefix;
			string_t attribute_local;
			split_qname(it.first,attribute_prefix,attribute_local);
			string_t attribute_uri;
			if (!attribute_prefix.empty()) {
				if (!resolve_prefix(scopes,attribute_prefix,attribute_uri)) throw std::invalid_argument("Undeclared namespace prefix '"+std::string(attribute_prefix.begin(),attribute_prefix.end())+"' on attribute '"+std::string(it.first.begin(),it.first.end())+"'");
			}
			grammar_state& st=ctx.state.grammar_pool[grammar_id].start_tag;
			std::size_t attribute_uri_id=0;
			std::size_t attribute_local_id=0;
			std::size_t learned_code=0;
			bool hit=false;
			std::size_t probe_uri=0;
			if (ctx.state.tables.find_uri(attribute_uri,probe_uri)) {
				std::size_t probe_local=0;
				if (string_table::find_in(ctx.state.tables.locals[probe_uri],attribute_local,probe_local)) hit=st.find(EV_AT,probe_uri,probe_local,learned_code);
			}
			if (hit) {
				write_nbit(writer,alignment,bits_for(st.learned.size()+1),learned_code);
				attribute_uri_id=st.learned[learned_code].uri;
				attribute_local_id=st.learned[learned_code].local;
			} else {
				write_nbit(writer,alignment,bits_for(st.learned.size()+1),st.learned.size());
				write_nbit(writer,alignment,bits_for(stc_second_count(options)),1);//AT(*)
				write_uri(writer,alignment,ctx.state.tables,attribute_uri,attribute_uri_id);
				write_local(writer,alignment,ctx.state.tables,attribute_uri_id,attribute_local,attribute_local_id);
				st.learn(EV_AT,attribute_uri_id,attribute_local_id);
			}
			if (options.preserve_prefixes) write_prefix_component(writer,alignment,ctx.state.tables,attribute_uri_id,attribute_prefix);
			emit_value(ctx,attribute_uri_id,attribute_local_id,it.second);
		}
		//内容
		for (const auto& child:base_t::children(node)) {
			grammar_state& stc=ctx.state.grammar_pool[grammar_id].start_tag;
			grammar_state& ec=ctx.state.grammar_pool[grammar_id].content;
			grammar_state& st=in_content?ec:stc;
			if (base_t::is_element(child)) {
				encode_element(ctx,child,scopes,st,false,in_content);
				in_content=true;
			} else if (child.type()==structure::DDT_STRING||base_t::is_cdata(child)||child.type()==structure::DDT_INT||child.type()==structure::DDT_FLOAT||child.type()==structure::DDT_BOOL) {
				const string_t text=base_t::is_cdata(child)?base_t::text_content(child):scalar_to_text(child);
				std::size_t learned_code=0;
				const bool hit=st.find(EV_CH,0,0,learned_code);
				const std::size_t part1_count=st.learned.size()+(in_content?2:1);
				if (hit) write_nbit(writer,alignment,bits_for(part1_count),learned_code);
				else {
					write_nbit(writer,alignment,bits_for(part1_count),st.learned.size()+(in_content?1:0));
					write_nbit(writer,alignment,bits_for(in_content?ec_second_count(options):stc_second_count(options)),in_content?1:stc_ch_index(options));
					st.learn(EV_CH,0,0);
				}
				emit_value(ctx,uri_id,local_id,text);
				in_content=true;
			} else if (base_t::is_comment(child)) {
				if (!options.preserve_comments) continue;
				const std::size_t part1_count=st.learned.size()+(in_content?2:1);
				write_nbit(writer,alignment,bits_for(part1_count),st.learned.size()+(in_content?1:0));
				write_nbit(writer,alignment,bits_for(in_content?ec_second_count(options):stc_second_count(options)),in_content?ec_group_index(options):stc_group_index(options));
				write_third(writer,alignment,options,false);
				write_string(writer,base_t::text_content(child));
				in_content=true;
			} else if (base_t::is_processing_instruction(child)) {
				if (!options.preserve_pis) continue;
				const std::size_t part1_count=st.learned.size()+(in_content?2:1);
				write_nbit(writer,alignment,bits_for(part1_count),st.learned.size()+(in_content?1:0));
				write_nbit(writer,alignment,bits_for(in_content?ec_second_count(options):stc_second_count(options)),in_content?ec_group_index(options):stc_group_index(options));
				write_third(writer,alignment,options,true);
				write_string(writer,base_t::pi_target(child));
				write_string(writer,base_t::pi_data(child));
				in_content=true;
			} else if (child.type()==structure::DDT_NULL) continue;//空内容,与dump口径一致
			else scalar_to_text(child);//容器/异族类型:抛出
		}
		//EE
		{
			grammar_state& st=in_content?ctx.state.grammar_pool[grammar_id].content:ctx.state.grammar_pool[grammar_id].start_tag;
			if (in_content) write_nbit(writer,alignment,bits_for(st.learned.size()+2),st.learned.size());//EC一级固定EE
			else {
				std::size_t learned_code=0;
				if (st.find(EV_EE,0,0,learned_code)) write_nbit(writer,alignment,bits_for(st.learned.size()+1),learned_code);
				else {
					write_nbit(writer,alignment,bits_for(st.learned.size()+1),st.learned.size());
					write_nbit(writer,alignment,bits_for(stc_second_count(options)),0);//EE
					st.learn(EV_EE,0,0);
				}
			}
		}
		scopes.pop_back();
	}

	//---文档编码入口---
	static binary_t encode_document(const typename base_t::base_t& root,const exi_options& options,const typename base_t::document_info_t* info) {
		if (!base_t::is_element(root)) throw std::invalid_argument("An exi document requires an element root");
		bitwise::bit_writer output(bitwise::BO_MSBYTE);
		write_header(output,options);
		encode_context ctx;
		ctx.options=options;
		ctx.channelized=(options.alignment==EA_COMPRESSION||options.alignment==EA_PRE_COMPRESSION);
		ctx.content_alignment=(options.alignment==EA_BIT_PACKED)?EA_BIT_PACKED:EA_BYTE_ALIGNED;
		bitwise::bit_writer structure(bitwise::BO_MSBYTE);
		if (ctx.channelized) {
			output.byte_align();
			ctx.writer=&structure;
			ctx.output=&output;
		} else {
			if (options.alignment==EA_BYTE_ALIGNED) output.byte_align();
			ctx.writer=&output;
		}
		//Document文法:SD为唯一产生式,0bit隐式
		std::vector<std::vector<ns_binding>> scopes;
		//保真选项一律显式:preserve_dtd未开启时info中的DOCTYPE不写出(与注释/PI同口径)
		if (options.preserve_dtd&&info&&info->has_doctype()) {
			string_t doctype_name;
			string_t doctype_public;
			string_t doctype_system;
			string_t doctype_subset;
			split_doctype(info->doctype,doctype_name,doctype_public,doctype_system,doctype_subset);
			//DocContent转义:此刻无学习产生式,part1=[SE(*)(0),转义(1)]→1bit;二级DT=0
			write_nbit(*ctx.writer,ctx.content_alignment,bits_for(2),1);
			write_nbit(*ctx.writer,ctx.content_alignment,bits_for((options.preserve_dtd?1:0)+(fidelity_group(options)?1:0)),0);
			write_string(*ctx.writer,doctype_name);
			write_string(*ctx.writer,doctype_public);
			write_string(*ctx.writer,doctype_system);
			write_string(*ctx.writer,doctype_subset);
		}
		encode_element(ctx,root,scopes,ctx.state.document_content,true,false);
		//DocEnd:[ED(0),转义(CM/PI)]:树中无文档级杂项,恒写ED
		write_nbit(*ctx.writer,ctx.content_alignment,bits_for(1+(fidelity_group(options)?1:0)),0);
		if (ctx.channelized) flush_block(ctx);//含零值块:仅结构一段
		return std::move(output).buffer();
	}
	//---DEFLATE流定界:deflate的公开解压API不报告输入消耗量,而done()对喂入前缀长度单调
	//(不完整回滚为false,完整翻true),以指数探进+二分取得最小完成前缀=流的字节边界---
	static binary_t inflate_one(const std::uint8_t* data,std::size_t size,std::size_t offset,std::size_t& consumed) {
		const std::size_t remain=size-offset;
		if (remain==0) throw std::runtime_error("Truncated DEFLATE stream at byte "+std::to_string(offset));
		auto probe=[&](std::size_t length,binary_t* out)->bool{
			crypto::deflate_decompressor engine(crypto::DSF_RAW);
			engine.feed(data+offset,length);
			if (!engine.done()) return false;
			if (out) *out=engine.output();
			return true;
		};
		std::size_t low=0;
		std::size_t high=0;
		bool found=false;
		for (std::size_t length=64;;length*=2) {
			if (length>=remain) {
				if (probe(remain,nullptr)) {
					high=remain;
					found=true;
				}
				break;
			}
			if (probe(length,nullptr)) {
				high=length;
				found=true;
				break;
			}
			low=length;
		}
		if (!found) throw std::runtime_error("Truncated or corrupt DEFLATE stream at byte "+std::to_string(offset));
		while (high-low>1) {
			const std::size_t middle=low+(high-low)/2;
			if (probe(middle,nullptr)) high=middle;
			else low=middle;
		}
		binary_t result;
		probe(high,&result);
		consumed=high;
		return result;
	}

	//---解码器:三段式(结构解析→信道补值→SAX重放)。结构解析持久化文法栈以支持块边界
	//落在元素中间;值配额=blockSize;文法学习只依赖事件码与限定名,与值无关---
	struct decode_frame {
		std::size_t grammar=0;
		bool in_content=false;
		std::size_t uri=0;
		std::size_t local=0;
	};
	struct decode_context {
		exi_options options;
		exi_alignment content_alignment=EA_BIT_PACKED;
		bool channelized=false;
		codec_state state;
		std::vector<decoded_event> events;
		std::vector<std::size_t> slot_events;//块内值槽→事件下标
		channel_layout layout;
		std::vector<decode_frame> frames;
		bool root_seen=false;
		bool document_done=false;
	};
	//结构解析:消费事件码至值配额耗尽或文档完结。值槽在信道模式下登记为占位,内联模式即时解出。
	static void parse_structure(bitwise::bit_reader& reader,decode_context& ctx,std::size_t value_quota) {
		const exi_options& options=ctx.options;
		const exi_alignment alignment=ctx.content_alignment;
		std::size_t values_taken=0;
		auto take_value=[&](std::size_t uri,std::size_t local)->string_t{
			if (!ctx.channelized) return read_value_string(reader,alignment,ctx.state.tables,uri,local);
			ctx.layout.add(uri,local,ctx.slot_events.size());
			ctx.slot_events.push_back(ctx.events.size());
			values_taken++;
			return string_t();
		};
		auto read_qname=[&](std::size_t& uri,std::size_t& local){
			uri=read_uri(reader,alignment,ctx.state.tables);
			local=read_local(reader,alignment,ctx.state.tables,uri);
		};
		auto push_se=[&](std::size_t uri,std::size_t local){
			decoded_event event;
			event.kind=DK_SE;
			event.uri=ctx.state.tables.uris[uri];
			event.local=ctx.state.tables.locals[uri][local];
			if (options.preserve_prefixes) event.prefix=read_prefix_component(reader,alignment,ctx.state.tables,uri);
			ctx.events.push_back(std::move(event));
			decode_frame frame;
			frame.grammar=ctx.state.grammar_of(uri,local);
			frame.uri=uri;
			frame.local=local;
			ctx.frames.push_back(frame);
		};
		while (true) {
			if (values_taken>=value_quota) return;
			if (ctx.frames.empty()) {
				if (!ctx.root_seen) {//DocContent
					grammar_state& st=ctx.state.document_content;
					const std::size_t part1_count=st.learned.size()+1+(doc_escape(options)?1:0);
					const std::uint64_t code=read_nbit(reader,alignment,bits_for(part1_count));
					if (code<st.learned.size()) {
						ctx.root_seen=true;
						push_se(st.learned[code].uri,st.learned[code].local);
					} else if (code==st.learned.size()) {
						std::size_t uri=0;
						std::size_t local=0;
						read_qname(uri,local);
						st.learn(EV_SE,uri,local);
						ctx.root_seen=true;
						push_se(uri,local);
					} else {//二级:[DT(若preserve.dtd)]+[CM/PI组]
						const std::size_t doc_items=(options.preserve_dtd?1:0)+(fidelity_group(options)?1:0);
						const std::uint64_t second=read_nbit(reader,alignment,bits_for(doc_items));
						decoded_event event;
						if (options.preserve_dtd&&second==0) {
							event.kind=DK_DT;
							event.local=read_string(reader);//名称
							event.prefix=read_string(reader);//公共标识
							event.aux=read_string(reader);//系统标识
							event.text=read_string(reader);//内部子集
						} else if (read_third(reader,alignment,options)) {
							event.kind=DK_PI;
							event.local=read_string(reader);
							event.text=read_string(reader);
						} else {
							event.kind=DK_CM;
							event.text=read_string(reader);
						}
						ctx.events.push_back(std::move(event));
					}
				} else {//DocEnd
					const std::size_t part1_count=1+(fidelity_group(options)?1:0);
					const std::uint64_t code=read_nbit(reader,alignment,bits_for(part1_count));
					if (code==0) {
						ctx.document_done=true;
						return;
					}
					const std::size_t enabled=(options.preserve_comments?1:0)+(options.preserve_pis?1:0);
					const std::uint64_t second=read_nbit(reader,alignment,bits_for(enabled));
					const bool is_pi=options.preserve_comments&&options.preserve_pis?second!=0:options.preserve_pis;
					decoded_event event;
					if (is_pi) {
						event.kind=DK_PI;
						event.local=read_string(reader);
						event.text=read_string(reader);
					} else {
						event.kind=DK_CM;
						event.text=read_string(reader);
					}
					ctx.events.push_back(std::move(event));
				}
				continue;
			}
			decode_frame& frame=ctx.frames.back();
			element_grammar& grammar=ctx.state.grammar_pool[frame.grammar];
			grammar_state& st=frame.in_content?grammar.content:grammar.start_tag;
			const std::size_t part1_count=st.learned.size()+(frame.in_content?2:1);
			const std::uint64_t code=read_nbit(reader,alignment,bits_for(part1_count));
			if (code<st.learned.size()) {
				const learned_production production=st.learned[code];
				if (production.kind==EV_SE) {
					frame.in_content=true;
					push_se(production.uri,production.local);
				} else if (production.kind==EV_AT) {
					decoded_event event;
					event.kind=DK_AT;
					event.uri=ctx.state.tables.uris[production.uri];
					event.local=ctx.state.tables.locals[production.uri][production.local];
					if (options.preserve_prefixes) event.prefix=read_prefix_component(reader,alignment,ctx.state.tables,production.uri);
					event.value_slot=ctx.channelized?static_cast<std::ptrdiff_t>(ctx.slot_events.size()):-1;
					event.text=take_value(production.uri,production.local);
					ctx.events.push_back(std::move(event));
				} else if (production.kind==EV_CH) {
					decoded_event event;
					event.kind=DK_CH;
					event.value_slot=ctx.channelized?static_cast<std::ptrdiff_t>(ctx.slot_events.size()):-1;
					event.text=take_value(frame.uri,frame.local);
					frame.in_content=true;
					ctx.events.push_back(std::move(event));
				} else {//EV_EE(STC学习命中)
					decoded_event event;
					event.kind=DK_EE;
					ctx.events.push_back(std::move(event));
					ctx.frames.pop_back();
				}
			} else if (frame.in_content&&code==st.learned.size()) {//EC一级固定EE
				decoded_event event;
				event.kind=DK_EE;
				ctx.events.push_back(std::move(event));
				ctx.frames.pop_back();
			} else {//转义→二级
				const std::size_t second_count=frame.in_content?ec_second_count(options):stc_second_count(options);
				const std::uint64_t second=read_nbit(reader,alignment,bits_for(second_count));
				if (!frame.in_content&&second==0) {//EE
					st.learn(EV_EE,0,0);
					decoded_event event;
					event.kind=DK_EE;
					ctx.events.push_back(std::move(event));
					ctx.frames.pop_back();
				} else if (!frame.in_content&&second==1) {//AT(*)
					std::size_t uri=0;
					std::size_t local=0;
					read_qname(uri,local);
					st.learn(EV_AT,uri,local);
					decoded_event event;
					event.kind=DK_AT;
					event.uri=ctx.state.tables.uris[uri];
					event.local=ctx.state.tables.locals[uri][local];
					if (options.preserve_prefixes) event.prefix=read_prefix_component(reader,alignment,ctx.state.tables,uri);
					event.value_slot=ctx.channelized?static_cast<std::ptrdiff_t>(ctx.slot_events.size()):-1;
					event.text=take_value(uri,local);
					ctx.events.push_back(std::move(event));
				} else if (!frame.in_content&&options.preserve_prefixes&&second==stc_ns_index(options)) {//NS
					const std::size_t uri=read_uri(reader,alignment,ctx.state.tables);
					decoded_event event;
					event.kind=DK_NS;
					event.uri=ctx.state.tables.uris[uri];
					event.prefix=read_prefix_entry(reader,alignment,ctx.state.tables,uri);
					event.ns_flag=read_boolean(reader,alignment);
					ctx.events.push_back(std::move(event));
				} else if ((frame.in_content&&second==0)||(!frame.in_content&&second==stc_se_index(options))) {//SE(*)
					std::size_t uri=0;
					std::size_t local=0;
					read_qname(uri,local);
					st.learn(EV_SE,uri,local);
					frame.in_content=true;
					push_se(uri,local);
				} else if ((frame.in_content&&second==1)||(!frame.in_content&&second==stc_ch_index(options))) {//CH
					st.learn(EV_CH,0,0);
					decoded_event event;
					event.kind=DK_CH;
					event.value_slot=ctx.channelized?static_cast<std::ptrdiff_t>(ctx.slot_events.size()):-1;
					event.text=take_value(frame.uri,frame.local);
					frame.in_content=true;
					ctx.events.push_back(std::move(event));
				} else if (options.preserve_dtd&&((frame.in_content&&second==2)||(!frame.in_content&&second==stc_er_index(options)))) {
					decode_fail(reader,"ER events require DTD entity processing which the xml.h lexical-expansion model excludes");
				} else {//CM/PI组(不迁移学习,但迁移内容态)
					decoded_event event;
					if (read_third(reader,alignment,options)) {
						event.kind=DK_PI;
						event.local=read_string(reader);
						event.text=read_string(reader);
					} else {
						event.kind=DK_CM;
						event.text=read_string(reader);
					}
					frame.in_content=true;
					ctx.events.push_back(std::move(event));
				}
			}
		}
	}
	//信道补值:按块装配序(≤100合流序或小-大分组序)读取值并回填事件文本。
	static void fill_block_values(decode_context& ctx,bitwise::bit_reader& combined_or_input,const std::uint8_t* data,std::size_t size,std::size_t& offset,bool compressed) {
		const std::size_t total=ctx.slot_events.size();
		auto fill_channel=[&](bitwise::bit_reader& reader,const std::pair<std::size_t,std::size_t>& key){
			for (const std::size_t slot:ctx.layout.members[key]) ctx.events[ctx.slot_events[slot]].text=read_value_string(reader,EA_BYTE_ALIGNED,ctx.state.tables,key.first,key.second);
		};
		if (total<=100) {
			for (const auto& key:ctx.layout.order) fill_channel(combined_or_input,key);
		} else if (!compressed) {
			for (const auto& key:ctx.layout.order) {
				if (ctx.layout.members[key].size()>100) continue;
				fill_channel(combined_or_input,key);
			}
			for (const auto& key:ctx.layout.order) {
				if (ctx.layout.members[key].size()<=100) continue;
				fill_channel(combined_or_input,key);
			}
		} else {
			std::size_t consumed=0;
			binary_t small=inflate_one(data,size,offset,consumed);
			offset+=consumed;
			bitwise::bit_reader small_reader(small.data(),small.size(),bitwise::BO_MSBYTE);
			for (const auto& key:ctx.layout.order) {
				if (ctx.layout.members[key].size()>100) continue;
				fill_channel(small_reader,key);
			}
			for (const auto& key:ctx.layout.order) {
				if (ctx.layout.members[key].size()<=100) continue;
				binary_t large=inflate_one(data,size,offset,consumed);
				offset+=consumed;
				bitwise::bit_reader large_reader(large.data(),large.size(),bitwise::BO_MSBYTE);
				fill_channel(large_reader,key);
			}
		}
		ctx.slot_events.clear();
		ctx.layout=channel_layout();
	}

	//---SAX重放:Preserve.prefixes下由NS事件忠实重建xmlns与前缀(local-element-ns旗标覆盖
	//SE前缀分量);关闭时按URI合成ns1,ns2...并补xmlns声明(Infoset等价,词法形式有损)---
	static bool replay_events(const std::vector<decoded_event>& events,const exi_options& options,sax_t& sax) {
		std::vector<std::vector<ns_binding>> scopes;
		std::size_t synth_counter=0;
		auto lexical=[](const string_t& prefix,const string_t& local)->string_t{
			if (prefix.empty()) return local;
			string_t result=prefix;
			result.push_back(':');
			result+=local;
			return result;
		};
		auto find_prefix=[&](const string_t& uri,bool for_attribute,bool& fresh)->string_t{
			for (std::size_t i=scopes.size();i>0;i--) {
				for (const auto& it:scopes[i-1]) {
					if (it.uri==uri&&(!for_attribute||!it.prefix.empty())) {
						fresh=false;
						return it.prefix;
					}
				}
			}
			fresh=true;
			synth_counter++;
			const std::string generated="ns"+std::to_string(synth_counter);
			return string_t(generated.begin(),generated.end());
		};
		std::size_t index=0;
		while (index<events.size()) {
			const decoded_event& current=events[index];
			switch (current.kind) {
				case DK_SE: {
					std::size_t next=index+1;
					std::vector<const decoded_event*> ns_events;
					std::vector<const decoded_event*> at_events;
					while (next<events.size()&&(events[next].kind==DK_NS||events[next].kind==DK_AT)) {
						if (events[next].kind==DK_NS) ns_events.push_back(&events[next]);
						else at_events.push_back(&events[next]);
						next++;
					}
					scopes.emplace_back();
					string_t element_prefix=current.prefix;
					std::vector<std::pair<string_t,string_t>> xmlns_attributes;
					if (options.preserve_prefixes) {
						for (const auto* it:ns_events) {
							ns_binding binding;
							binding.prefix=it->prefix;
							binding.uri=it->uri;
							scopes.back().push_back(binding);
							xmlns_attributes.emplace_back(it->prefix.empty()?make_string("xmlns"):make_string("xmlns:")+it->prefix,it->uri);
							if (it->ns_flag) element_prefix=it->prefix;
						}
					} else if (!current.uri.empty()) {
						bool fresh=false;
						element_prefix=find_prefix(current.uri,false,fresh);
						if (fresh) {
							ns_binding binding;
							binding.prefix=element_prefix;
							binding.uri=current.uri;
							scopes.back().push_back(binding);
							xmlns_attributes.emplace_back(element_prefix.empty()?make_string("xmlns"):make_string("xmlns:")+element_prefix,current.uri);
						}
					} else element_prefix.clear();
					string_t element_name=lexical(element_prefix,current.local);
					if (!sax.start_element(element_name)) return false;
					for (auto& it:xmlns_attributes) {
						if (!sax.attribute(it.first,it.second)) return false;
					}
					for (const auto* it:at_events) {
						string_t attribute_prefix=it->prefix;
						if (!options.preserve_prefixes&&!it->uri.empty()) {
							bool fresh=false;
							attribute_prefix=find_prefix(it->uri,true,fresh);
							if (fresh) {
								ns_binding binding;
								binding.prefix=attribute_prefix;
								binding.uri=it->uri;
								scopes.back().push_back(binding);
								string_t key=make_string("xmlns:")+attribute_prefix;
								string_t value=it->uri;
								if (!sax.attribute(key,value)) return false;
							}
						}
						string_t attribute_name=lexical(it->uri.empty()?string_t():attribute_prefix,it->local);
						string_t attribute_value=it->text;
						if (!sax.attribute(attribute_name,attribute_value)) return false;
					}
					index=next;
					continue;
				}
				case DK_EE: {
					if (!sax.end_element()) return false;
					if (!scopes.empty()) scopes.pop_back();
					break;
				}
				case DK_CH: {
					string_t text=current.text;
					if (!sax.characters(text)) return false;
					break;
				}
				case DK_CM: {
					string_t text=current.text;
					if (!sax.comment(text)) return false;
					break;
				}
				case DK_PI: {
					string_t target=current.local;
					string_t data=current.text;
					if (!sax.processing_instruction(target,data)) return false;
					break;
				}
				case DK_DT: {
					string_t text=build_doctype(current.local,current.prefix,current.aux,current.text);
					if (!sax.doctype(text)) return false;
					break;
				}
				default: break;
			}
			index++;
		}
		return true;
	}
	static bool run_decode(const std::uint8_t* data,std::size_t size,sax_t& sax) {
		bitwise::bit_reader reader(data,size,bitwise::BO_MSBYTE);
		decode_context ctx;
		ctx.options=read_header(reader);
		ctx.channelized=(ctx.options.alignment==EA_COMPRESSION||ctx.options.alignment==EA_PRE_COMPRESSION);
		ctx.content_alignment=(ctx.options.alignment==EA_BIT_PACKED)?EA_BIT_PACKED:EA_BYTE_ALIGNED;
		if (!ctx.channelized) {
			if (ctx.options.alignment==EA_BYTE_ALIGNED) reader.byte_align();
			parse_structure(reader,ctx,static_cast<std::size_t>(-1));
			if (!ctx.document_done) decode_fail(reader,"Unexpected end of exi body");
			if (reader.remaining_bits()>=CHAR_BIT) decode_fail(reader,"Trailing bytes after exi document");
		} else {
			reader.byte_align();
			std::size_t offset=byte_position(reader);
			while (!ctx.document_done) {
				if (ctx.options.alignment==EA_COMPRESSION) {
					std::size_t consumed=0;
					binary_t structure=inflate_one(data,size,offset,consumed);
					offset+=consumed;
					bitwise::bit_reader structure_reader(structure.data(),structure.size(),bitwise::BO_MSBYTE);
					parse_structure(structure_reader,ctx,ctx.options.block_size);
					fill_block_values(ctx,structure_reader,data,size,offset,true);
				} else {
					parse_structure(reader,ctx,ctx.options.block_size);
					std::size_t unused=0;
					fill_block_values(ctx,reader,data,size,unused,false);
				}
			}
			if (ctx.options.alignment==EA_COMPRESSION) {
				if (offset!=size) throw std::runtime_error("Trailing bytes after exi document at byte "+std::to_string(offset));
			} else if (reader.remaining_bits()>=CHAR_BIT) decode_fail(reader,"Trailing bytes after exi document");
		}
		return replay_events(ctx.events,ctx.options,sax);
	}

public:
	//二进制编解码入口(文本侧的parse/dump继承自xml,故此处命名encode/decode避免同名混淆)。
	binary_t encode(const exi_options& options=exi_options(),const document_info_t* info=nullptr) const {
		return encode_document(*this,options,info);
	}
	static binary_t encode(const typename base_t::base_t& root,const exi_options& options=exi_options(),const document_info_t* info=nullptr) {
		return encode_document(root,options,info);
	}
	static exi decode(const std::uint8_t* data,std::size_t size,document_info_t* info=nullptr) {
		exi result;
		basic_xml::xml_sax_dom_builder<exi> builder(result,info,true);
		if (!run_decode(data,size,builder)||!builder.completed()) throw std::runtime_error("Incomplete exi document");
		return result;
	}
	static exi decode(const binary_t& data,document_info_t* info=nullptr) {
		return decode(data.data(),data.size(),info);
	}
	//二进制SAX:解码事件直接驱动xml_sax监听器(返回false表示监听器中止)。
	static bool sax_decode(const std::uint8_t* data,std::size_t size,sax_t* sax) {
		return run_decode(data,size,*sax);
	}
	static bool sax_decode(const binary_t& data,sax_t* sax) {
		return sax_decode(data.data(),data.size(),sax);
	}
};

}

_STDEX_DOM_TPL_DEFAULT_DECLARATION
using exi_t=basic_exi::exi<_Int,_Float,_Boolean,_String,_Array,_Object,_Allocator>;
using exi=exi_t<>;
using basic_exi::exi_options;
using basic_exi::exi_alignment;
using basic_exi::EA_BIT_PACKED;
using basic_exi::EA_BYTE_ALIGNED;
using basic_exi::EA_PRE_COMPRESSION;
using basic_exi::EA_COMPRESSION;

}

}

#endif
