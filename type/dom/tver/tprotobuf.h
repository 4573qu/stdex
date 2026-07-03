//Last Modified At 2026/07/04
//@Version 1.0.0.0
#ifndef _STDEX_TYPE_DOM_PROTOBUF_H_
#define _STDEX_TYPE_DOM_PROTOBUF_H_ 1

#include <cctype>
#include <cstdlib>
#include <cstring>
#include <istream>
#include <limits>
#include <memory>
#include <ostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "../../structure/dom.h"//At Least 1.0

namespace stdex {

namespace type {

namespace basic_protobuf {

//Protocol Buffers为携带外置schema的二进制记法:wire流内只有(字段编号,wire类型,载荷),
//字段名/语义类型/嵌套结构全部由schema提供,故序列化与反序列化入口均要求(schema,消息名)。
//dom映射:message→DDT_OBJECT(字段名为键),repeated→DDT_ARRAY,整型族→DDT_INT,float/double→DDT_FLOAT,
//bool→DDT_BOOL,string/bytes→DDT_STRING(字节串),enum→已知值取其名字符串/未知值退化整数,
//map<k,v>→DDT_OBJECT(键字符串化);未知编号字段按wire类型跳过。
enum protobuf_field_type : int {
	PFT_DOUBLE,
	PFT_FLOAT,
	PFT_INT32,
	PFT_INT64,
	PFT_UINT32,
	PFT_UINT64,
	PFT_SINT32,
	PFT_SINT64,
	PFT_FIXED32,
	PFT_FIXED64,
	PFT_SFIXED32,
	PFT_SFIXED64,
	PFT_BOOL,
	PFT_STRING,
	PFT_BYTES,
	PFT_MESSAGE,
	PFT_ENUM,
};

enum protobuf_field_label : int {
	PFL_SINGULAR,
	PFL_OPTIONAL,
	PFL_REPEATED,
	PFL_REQUIRED,
};

enum protobuf_wire_type : int {
	PWT_VARINT=0,
	PWT_FIXED64=1,
	PWT_LENGTH=2,
	PWT_GROUP_START=3,
	PWT_GROUP_END=4,
	PWT_FIXED32=5,
};

template <typename _String>
struct basic_protobuf_enum_value {
	_String name{};
	long long number=0;
};

template <typename _String>
struct basic_protobuf_enum {
	_String name{};
	std::vector<basic_protobuf_enum_value<_String>> values{};

	const _String* find_name(long long number) const {
		for (const auto& it:values) {
			if (it.number==number) return &it.name;
		}
		return nullptr;
	}
	bool find_number(const _String& target,long long& out) const {
		for (const auto& it:values) {
			if (it.name==target) {
				out=it.number;
				return true;
			}
		}
		return false;
	}
};

template <typename _String>
struct basic_protobuf_field {
	_String name{};
	int number=0;
	protobuf_field_type type=PFT_INT32;
	protobuf_field_label label=PFL_SINGULAR;
	_String type_name{};
	bool is_map=false;
	protobuf_field_type key_type=PFT_STRING;
	bool packed=true;
	int oneof_index=-1;
	_String default_text{};
};

template <typename _String>
struct basic_protobuf_message {
	_String name{};
	std::vector<basic_protobuf_field<_String>> fields{};
	std::vector<_String> oneofs{};

	const basic_protobuf_field<_String>* find_field(int number) const {
		for (const auto& it:fields) {
			if (it.number==number) return &it;
		}
		return nullptr;
	}
	const basic_protobuf_field<_String>* find_field(const _String& target) const {
		for (const auto& it:fields) {
			if (it.name==target) return &it;
		}
		return nullptr;
	}
};

//schema容器:message/enum按完整限定名(点分,不含package前缀)扁平登记,嵌套关系由名字前缀表达;
//既可编程拼装(add_message/add_enum),也可由.proto(proto3子集)文本解析(parse)与回写(to_string)。
template <typename _String=std::string>
class protobuf_schema {
public:
	using string_t=_String;
	using field_t=basic_protobuf_field<_String>;
	using message_t=basic_protobuf_message<_String>;
	using enum_t=basic_protobuf_enum<_String>;

private:
	std::vector<message_t> messages_;
	std::vector<enum_t> enums_;
	string_t package_{};

	static string_t make_string(const char* text) {
		return string_t(text,text+std::strlen(text));
	}
	static string_t make_string(const std::string& text) {
		return string_t(text.begin(),text.end());
	}

	//.proto文本扫描器:跳过空白与//、/* */注释,提供标识符(容许点分)/整数/字符串字面量/单字符消费。
	struct text_scanner {
		const char* first;
		const char* last;

		text_scanner(std::string_view text) : first(text.data()) , last(text.data()+text.size()) { }

		void skip() {
			for (;;) {
				while (first<last && (*first==' ' || *first=='\t' || *first=='\r' || *first=='\n' || *first=='\f' || *first=='\v')) first++;
				if (first+1<last && first[0]=='/' && first[1]=='/') {
					while (first<last && *first!='\n') first++;
					continue;
				}
				if (first+1<last && first[0]=='/' && first[1]=='*') {
					first+=2;
					while (first+1<last && !(first[0]=='*' && first[1]=='/')) first++;
					if (first+1<last) first+=2;
					else first=last;
					continue;
				}
				return;
			}
		}
		bool eof() {
			skip();
			return first>=last;
		}
		char peek() {
			skip();
			return first<last?*first:'\0';
		}
		bool consume(char c) {
			skip();
			if (first<last && *first==c) {
				first++;
				return true;
			}
			return false;
		}
		void expect(char c) {
			if (!consume(c)) throw std::invalid_argument("Expected '"+std::string(1,c)+"' in the schema text");
		}
		static bool ident_start(char c) noexcept {
			return (c>='A' && c<='Z') || (c>='a' && c<='z') || c=='_';
		}
		static bool ident_body(char c) noexcept {
			return ident_start(c) || (c>='0' && c<='9');
		}
		std::string ident(bool dotted=true) {
			skip();
			const char* start=first;
			if (first<last && *first=='.') first++;//容许前导'.'的全限定引用
			if (first>=last || !ident_start(*first)) throw std::invalid_argument("Expected an identifier in the schema text");
			while (first<last && (ident_body(*first) || (dotted && *first=='.'))) first++;
			return std::string(start,first);
		}
		long long integer() {
			skip();
			const char* start=first;
			if (first<last && (*first=='-' || *first=='+')) first++;
			if (first>=last || !(*first>='0' && *first<='9')) throw std::invalid_argument("Expected an integer in the schema text");
			while (first<last && ((*first>='0' && *first<='9') || (*first>='a' && *first<='f') || (*first>='A' && *first<='F') || *first=='x' || *first=='X')) first++;
			return std::strtoll(std::string(start,first).c_str(),nullptr,0);
		}
		std::string literal() {
			skip();
			if (first>=last || (*first!='"' && *first!='\'')) throw std::invalid_argument("Expected a string literal in the schema text");
			const char quote=*first;
			first++;
			std::string result;
			while (first<last && *first!=quote) {
				if (*first=='\\' && first+1<last) first++;
				result.push_back(*first);
				first++;
			}
			if (first>=last) throw std::invalid_argument("Unterminated string literal in the schema text");
			first++;
			return result;
		}
		//值(标识符/整数/字面量)一枚,用于跳过option等无关内容。
		void skip_value() {
			skip();
			if (first<last && (*first=='"' || *first=='\'')) {
				literal();
				return;
			}
			if (first<last && (*first=='-' || *first=='+' || (*first>='0' && *first<='9'))) {
				integer();
				return;
			}
			if (first<last && *first=='{') {
				std::size_t depth=0;
				do {
					if (*first=='{') depth++;
					else if (*first=='}') depth--;
					first++;
				} while (first<last && depth);
				return;
			}
			ident();
		}
		//原文值捕获:字面量原文/裸token(标识符、数字、inf等),供default选项无损保留。
		std::string value_text() {
			skip();
			if (first<last && (*first=='"' || *first=='\'')) return literal();
			const char* start=first;
			while (first<last && *first!=';' && *first!=',' && *first!=']' && *first!=' ' && *first!='\t' && *first!='\r' && *first!='\n') first++;
			if (start==first) throw std::invalid_argument("Expected an option value in the schema text");
			return std::string(start,first);
		}
		//平衡跳过一个'{...}'块(保护字符串字面量与注释)。
		void skip_block() {
			expect('{');
			std::size_t depth=1;
			while (first<last && depth) {
				skip();
				if (first>=last) break;
				if (*first=='"' || *first=='\'') {
					literal();
					continue;
				}
				if (*first=='{') depth++;
				else if (*first=='}') depth--;
				first++;
			}
			if (depth) throw std::invalid_argument("Unterminated block in the schema text");
		}
		void skip_statement() {
			while (first<last && !consume(';')) {
				skip();
				if (first>=last) return;
				if (*first=='"' || *first=='\'') {
					literal();
					continue;
				}
				if (*first=='{') {
					skip_value();
					consume(';');//aggregate体后的';'可省
					return;
				}
				first++;
			}
		}
	};

	static bool keyword_type(const std::string& word,protobuf_field_type& out) {
		if (word=="double") out=PFT_DOUBLE;
		else if (word=="float") out=PFT_FLOAT;
		else if (word=="int32") out=PFT_INT32;
		else if (word=="int64") out=PFT_INT64;
		else if (word=="uint32") out=PFT_UINT32;
		else if (word=="uint64") out=PFT_UINT64;
		else if (word=="sint32") out=PFT_SINT32;
		else if (word=="sint64") out=PFT_SINT64;
		else if (word=="fixed32") out=PFT_FIXED32;
		else if (word=="fixed64") out=PFT_FIXED64;
		else if (word=="sfixed32") out=PFT_SFIXED32;
		else if (word=="sfixed64") out=PFT_SFIXED64;
		else if (word=="bool") out=PFT_BOOL;
		else if (word=="string") out=PFT_STRING;
		else if (word=="bytes") out=PFT_BYTES;
		else return false;
		return true;
	}
	static const char* type_keyword(protobuf_field_type type) {
		switch (type) {
			case PFT_DOUBLE: return "double";
			case PFT_FLOAT: return "float";
			case PFT_INT32: return "int32";
			case PFT_INT64: return "int64";
			case PFT_UINT32: return "uint32";
			case PFT_UINT64: return "uint64";
			case PFT_SINT32: return "sint32";
			case PFT_SINT64: return "sint64";
			case PFT_FIXED32: return "fixed32";
			case PFT_FIXED64: return "fixed64";
			case PFT_SFIXED32: return "sfixed32";
			case PFT_SFIXED64: return "sfixed64";
			case PFT_BOOL: return "bool";
			case PFT_STRING: return "string";
			case PFT_BYTES: return "bytes";
			default: return nullptr;
		}
	}

public:
	message_t& add_message(message_t item) {
		messages_.push_back(std::move(item));
		return messages_.back();
	}
	enum_t& add_enum(enum_t item) {
		enums_.push_back(std::move(item));
		return enums_.back();
	}
	message_t* find_message(const string_t& name) {
		for (auto& it:messages_) {
			if (it.name==name) return &it;
		}
		return nullptr;
	}
	const message_t* find_message(const string_t& name) const {
		for (const auto& it:messages_) {
			if (it.name==name) return &it;
		}
		return nullptr;
	}
	enum_t* find_enum(const string_t& name) {
		for (auto& it:enums_) {
			if (it.name==name) return &it;
		}
		return nullptr;
	}
	const enum_t* find_enum(const string_t& name) const {
		for (const auto& it:enums_) {
			if (it.name==name) return &it;
		}
		return nullptr;
	}
	bool contains_message(const string_t& name) const {
		return find_message(name)!=nullptr;
	}
	bool contains_enum(const string_t& name) const {
		return find_enum(name)!=nullptr;
	}
	const std::vector<message_t>& messages() const noexcept {
		return messages_;
	}
	const std::vector<enum_t>& enums() const noexcept {
		return enums_;
	}
	const string_t& package() const noexcept {
		return package_;
	}
	void package(string_t name) {
		package_=std::move(name);
	}

private:
	struct pending_reference {
		std::size_t message_index=0;
		std::size_t field_index=0;
		std::string raw{};
		std::string scope{};
	};

	//字段声明子例程:label/类型/名字=编号/[options];未知类型名连同声明处作用域挂起,待全量登记后解析。
	static void parse_field(text_scanner& scanner,protobuf_schema& result,std::size_t message_index,const std::string& scope,std::vector<pending_reference>& pendings,protobuf_field_label label,const std::string& type_word) {
		const std::size_t pending_mark=pendings.size();
		field_t field;
		field.label=label;
		if (!keyword_type(type_word,field.type)) {
			field.type=PFT_MESSAGE;//暂定,解析期修正为ENUM或保持
			pending_reference pending;
			pending.message_index=message_index;
			pending.raw=type_word;
			pending.scope=scope;
			pendings.push_back(std::move(pending));
		}
		field.name=make_string(scanner.ident(false));
		scanner.expect('=');
		field.number=static_cast<int>(scanner.integer());
		parse_field_options(scanner,field);
		scanner.expect(';');
		result.messages_[message_index].fields.push_back(std::move(field));
		if (pendings.size()>pending_mark) pendings.back().field_index=result.messages_[message_index].fields.size()-1;
	}
	static void parse_field_options(text_scanner& scanner,field_t& field) {
		if (!scanner.consume('[')) return;
		for (;;) {
			const std::string key=scanner.ident();
			scanner.expect('=');
			if (key=="packed") {
				const std::string flag=scanner.ident();
				field.packed=(flag=="true");
			} else if (key=="default") field.default_text=make_string(scanner.value_text());
			else scanner.skip_value();
			if (scanner.consume(',')) continue;
			scanner.expect(']');
			return;
		}
	}
	static void parse_message(text_scanner& scanner,protobuf_schema& result,const std::string& scope,std::vector<pending_reference>& pendings) {
		const std::string leaf=scanner.ident(false);
		const std::string full=scope.empty()?leaf:scope+"."+leaf;
		message_t item;
		item.name=make_string(full);
		result.messages_.push_back(std::move(item));
		const std::size_t index=result.messages_.size()-1;
		scanner.expect('{');
		while (!scanner.consume('}')) {
			if (scanner.eof()) throw std::invalid_argument("Unterminated message body in the schema text");
			if (scanner.consume(';')) continue;
			const std::string word=scanner.ident();
			if (word=="message") {
				parse_message(scanner,result,full,pendings);
				continue;
			}
			if (word=="enum") {
				parse_enum(scanner,result,full);
				continue;
			}
			if (word=="option" || word=="reserved" || word=="extensions") {
				scanner.skip_statement();
				continue;
			}
			if (word=="extend") {
				scanner.ident();
				scanner.skip_block();
				continue;
			}
			if (word=="oneof") {
				const std::string group=scanner.ident(false);
				result.messages_[index].oneofs.push_back(make_string(group));
				const int oneof_index=static_cast<int>(result.messages_[index].oneofs.size())-1;
				scanner.expect('{');
				while (!scanner.consume('}')) {
					if (scanner.eof()) throw std::invalid_argument("Unterminated oneof body in the schema text");
					if (scanner.consume(';')) continue;
					const std::string inner=scanner.ident();
					if (inner=="option") {
						scanner.skip_statement();
						continue;
					}
					parse_field(scanner,result,index,full,pendings,PFL_SINGULAR,inner);
					result.messages_[index].fields.back().oneof_index=oneof_index;
				}
				continue;
			}
			if (word=="map") {
				scanner.expect('<');
				const std::string key_word=scanner.ident();
				field_t field;
				field.is_map=true;
				field.label=PFL_REPEATED;
				if (!keyword_type(key_word,field.key_type)) throw std::invalid_argument("Map keys must be a scalar type");
				scanner.expect(',');
				const std::string value_word=scanner.ident();
				if (!keyword_type(value_word,field.type)) {
					field.type=PFT_MESSAGE;
					pending_reference pending;
					pending.message_index=index;
					pending.raw=value_word;
					pending.scope=full;
					pending.field_index=result.messages_[index].fields.size();
					pendings.push_back(std::move(pending));
				}
				scanner.expect('>');
				field.name=make_string(scanner.ident(false));
				scanner.expect('=');
				field.number=static_cast<int>(scanner.integer());
				parse_field_options(scanner,field);
				scanner.expect(';');
				result.messages_[index].fields.push_back(std::move(field));
				continue;
			}
			protobuf_field_label label=PFL_SINGULAR;
			std::string type_word=word;
			if (word=="repeated" || word=="optional" || word=="required") {
				label=(word=="repeated")?PFL_REPEATED:((word=="optional")?PFL_OPTIONAL:PFL_REQUIRED);
				type_word=scanner.ident();
			}
			parse_field(scanner,result,index,full,pendings,label,type_word);
		}
	}
	static void parse_enum(text_scanner& scanner,protobuf_schema& result,const std::string& scope) {
		const std::string leaf=scanner.ident(false);
		const std::string full=scope.empty()?leaf:scope+"."+leaf;
		enum_t item;
		item.name=make_string(full);
		scanner.expect('{');
		while (!scanner.consume('}')) {
			if (scanner.eof()) throw std::invalid_argument("Unterminated enum body in the schema text");
			if (scanner.consume(';')) continue;
			const std::string word=scanner.ident();
			if (word=="option" || word=="reserved") {
				scanner.skip_statement();
				continue;
			}
			basic_protobuf_enum_value<_String> value;
			value.name=make_string(word);
			scanner.expect('=');
			value.number=scanner.integer();
			if (scanner.consume('[')) {
				while (!scanner.consume(']')) {
					if (scanner.eof()) throw std::invalid_argument("Unterminated option list in the schema text");
					scanner.first++;
				}
			}
			scanner.expect(';');
			item.values.push_back(std::move(value));
		}
		result.enums_.push_back(std::move(item));
	}
	//类型名解析:自声明作用域由内向外尝试"scope前缀+.raw",最后裸raw;命中enum定型ENUM,命中message定型MESSAGE。
	void resolve_pending(const pending_reference& pending) {
		std::string raw=pending.raw;
		if (!raw.empty() && raw.front()=='.') raw.erase(raw.begin());
		std::string scope=pending.scope;
		for (;;) {
			const std::string candidate=scope.empty()?raw:scope+"."+raw;
			const string_t name=make_string(candidate);
			field_t& field=messages_[pending.message_index].fields[pending.field_index];
			if (find_enum(name)) {
				field.type=PFT_ENUM;
				field.type_name=name;
				return;
			}
			if (find_message(name)) {
				field.type=PFT_MESSAGE;
				field.type_name=name;
				return;
			}
			if (scope.empty()) break;
			const std::size_t cut=scope.rfind('.');
			scope=(cut==std::string::npos)?std::string():scope.substr(0,cut);
		}
		throw std::invalid_argument("Unresolved type '"+pending.raw+"' in the schema text");
	}

public:
	static protobuf_schema parse(std::string_view text,bool allow_exceptions=true) {
		protobuf_schema result;
		try {
			text_scanner scanner(text);
			std::vector<pending_reference> pendings;
			while (!scanner.eof()) {
				if (scanner.consume(';')) continue;
				const std::string word=scanner.ident();
				if (word=="syntax" || word=="edition") {
					scanner.expect('=');
					scanner.literal();
					scanner.expect(';');
					continue;
				}
				if (word=="package") {
					result.package_=make_string(scanner.ident());
					scanner.expect(';');
					continue;
				}
				if (word=="import" || word=="option") {
					scanner.skip_statement();
					continue;
				}
				if (word=="service" || word=="extend") {
					scanner.ident();
					scanner.skip_block();
					continue;
				}
				if (word=="message") {
					parse_message(scanner,result,std::string(),pendings);
					continue;
				}
				if (word=="enum") {
					parse_enum(scanner,result,std::string());
					continue;
				}
				throw std::invalid_argument("Unexpected token '"+word+"' in the schema text");
			}
			for (const auto& it:pendings) result.resolve_pending(it);
		} catch (...) {
			if (allow_exceptions) throw;
			return protobuf_schema();
		}
		return result;
	}

private:
	void emit_indent(string_t& out,std::size_t depth) const {
		for (std::size_t i=0;i<depth;i++) out.push_back('\t');
	}
	static std::string leaf_of(const string_t& full) {
		const std::string name(full.begin(),full.end());
		const std::size_t cut=name.rfind('.');
		return cut==std::string::npos?name:name.substr(cut+1);
	}
	void emit_enum(const enum_t& item,string_t& out,std::size_t depth) const {
		emit_indent(out,depth);
		out.append(make_string("enum "+leaf_of(item.name)+" {\n"));
		for (const auto& it:item.values) {
			emit_indent(out,depth+1);
			out.append(it.name);
			out.append(make_string(" = "+std::to_string(it.number)+";\n"));
		}
		emit_indent(out,depth);
		out.append(make_string("}\n"));
	}
	void emit_message(const message_t& item,string_t& out,std::size_t depth) const {
		emit_indent(out,depth);
		out.append(make_string("message "+leaf_of(item.name)+" {\n"));
		const std::string prefix=std::string(item.name.begin(),item.name.end())+".";
		for (const auto& it:enums_) {
			const std::string name(it.name.begin(),it.name.end());
			if (name.size()>prefix.size() && name.compare(0,prefix.size(),prefix)==0 && name.find('.',prefix.size())==std::string::npos) emit_enum(it,out,depth+1);
		}
		for (const auto& it:messages_) {
			const std::string name(it.name.begin(),it.name.end());
			if (name.size()>prefix.size() && name.compare(0,prefix.size(),prefix)==0 && name.find('.',prefix.size())==std::string::npos) emit_message(it,out,depth+1);
		}
		for (const auto& it:item.fields) {
			if (it.oneof_index>=0) continue;
			emit_indent(out,depth+1);
			if (it.is_map) {
				out.append(make_string(std::string("map<")+type_keyword(it.key_type)+", "));
				if (it.type==PFT_MESSAGE || it.type==PFT_ENUM) out.append(it.type_name);
				else out.append(make_string(type_keyword(it.type)));
				out.append(make_string("> "));
			} else {
				if (it.label==PFL_REPEATED) out.append(make_string("repeated "));
				else if (it.label==PFL_OPTIONAL) out.append(make_string("optional "));
				else if (it.label==PFL_REQUIRED) out.append(make_string("required "));
				if (it.type==PFT_MESSAGE || it.type==PFT_ENUM) out.append(it.type_name);
				else out.append(make_string(type_keyword(it.type)));
				out.push_back(' ');
			}
			out.append(it.name);
			out.append(make_string(" = "+std::to_string(it.number)));
			emit_field_options(it,out);
			out.append(make_string(";\n"));
		}
		for (std::size_t group=0;group<item.oneofs.size();group++) {
			emit_indent(out,depth+1);
			out.append(make_string("oneof "));
			out.append(item.oneofs[group]);
			out.append(make_string(" {\n"));
			for (const auto& it:item.fields) {
				if (it.oneof_index!=static_cast<int>(group)) continue;
				emit_indent(out,depth+2);
				if (it.type==PFT_MESSAGE || it.type==PFT_ENUM) out.append(it.type_name);
				else out.append(make_string(type_keyword(it.type)));
				out.push_back(' ');
				out.append(it.name);
				out.append(make_string(" = "+std::to_string(it.number)));
				emit_field_options(it,out);
				out.append(make_string(";\n"));
			}
			emit_indent(out,depth+1);
			out.append(make_string("}\n"));
		}
		emit_indent(out,depth);
		out.append(make_string("}\n"));
	}
	//字段选项回写:[default=...,packed=false];字符串型default补引号。
	void emit_field_options(const field_t& item,string_t& out) const {
		std::string options;
		if (!item.default_text.empty()) {
			std::string text(item.default_text.begin(),item.default_text.end());
			if (item.type==PFT_STRING || item.type==PFT_BYTES) text="\""+text+"\"";
			options+="default = "+text;
		}
		if (item.label==PFL_REPEATED && !item.is_map && !item.packed) {
			if (!options.empty()) options+=", ";
			options+="packed = false";
		}
		if (!options.empty()) out.append(make_string(" ["+options+"]"));
	}

public:
	//schema一致性校验:字段编号区间(1..536870911,避开19000..19999保留段)、同消息内编号/名唯一、
	//message/enum引用可解析、map键为整数/布尔/字符串标量、oneof索引有效;失败时经reason给出首个原因。
	bool validate(std::string* reason=nullptr) const {
		auto complain=[&](const std::string& text) {
			if (reason) *reason=text;
			return false;
		};
		for (const auto& message:messages_) {
			const std::string owner(message.name.begin(),message.name.end());
			for (std::size_t i=0;i<message.fields.size();i++) {
				const field_t& field=message.fields[i];
				const std::string label="Field '"+std::string(field.name.begin(),field.name.end())+"' in message '"+owner+"'";
				if (field.number<1 || field.number>536870911 || (field.number>=19000 && field.number<=19999)) return complain(label+" has an invalid number");
				for (std::size_t j=0;j<i;j++) {
					if (message.fields[j].number==field.number) return complain(label+" duplicates a field number");
					if (message.fields[j].name==field.name) return complain(label+" duplicates a field name");
				}
				if ((field.type==PFT_MESSAGE && !contains_message(field.type_name)) || (field.type==PFT_ENUM && !contains_enum(field.type_name))) return complain(label+" references an unresolved type");
				if (field.is_map) {
					switch (field.key_type) {
						case PFT_DOUBLE:
						case PFT_FLOAT:
						case PFT_BYTES:
						case PFT_MESSAGE:
						case PFT_ENUM: return complain(label+" uses an invalid map key type");
						default: break;
					}
				}
				if (field.oneof_index>=0 && static_cast<std::size_t>(field.oneof_index)>=message.oneofs.size()) return complain(label+" references an invalid oneof group");
			}
		}
		for (const auto& item:enums_) {
			for (std::size_t i=0;i<item.values.size();i++) {
				for (std::size_t j=0;j<i;j++) {
					if (item.values[j].name==item.values[i].name) return complain("Enum '"+std::string(item.name.begin(),item.name.end())+"' duplicates a value name");
				}
			}
		}
		return true;
	}
	string_t to_string() const {
		string_t out=make_string("syntax = \"proto3\";\n");
		if (!package_.empty()) {
			out.append(make_string("package "));
			out.append(package_);
			out.append(make_string(";\n"));
		}
		for (const auto& it:enums_) {
			if (std::string(it.name.begin(),it.name.end()).find('.')==std::string::npos) emit_enum(it,out,0);
		}
		for (const auto& it:messages_) {
			if (std::string(it.name.begin(),it.name.end()).find('.')==std::string::npos) emit_message(it,out,0);
		}
		return out;
	}
};

//protobuf记法主体:承载dom树并提供带schema的wire序列化/反序列化。
_STDEX_DOM_TPL_DECLARATION
class protobuf : public structure::_STDEX_DOM_DEF {
public:
	using base_t=structure::_STDEX_DOM_DEF;
	using int_t=typename base_t::int_t;
	using float_t=typename base_t::float_t;
	using boolean_t=typename base_t::boolean_t;
	using string_t=typename base_t::string_t;
	using array_t=typename base_t::array_t;
	using object_t=typename base_t::object_t;
	using size_type=typename base_t::size_type;
	using schema_t=protobuf_schema<string_t>;
	using field_t=typename schema_t::field_t;
	using message_t=typename schema_t::message_t;
	using enum_t=typename schema_t::enum_t;

	static_assert(sizeof(typename string_t::value_type)==1,"protobuf serializer assumes a byte-oriented string_t.");
	static_assert(sizeof(int_t)>=8,"protobuf requires an int_t of at least 64 bits for lossless integers.");

	using base_t::base_t;
	using base_t::operator =;

	protobuf()=default;
	protobuf(const protobuf&)=default;
	protobuf(protobuf&&)=default;
	protobuf& operator =(const protobuf&)=default;
	protobuf& operator =(protobuf&&)=default;
	protobuf(const base_t& other) : base_t(other) { }
	protobuf(base_t&& other) : base_t(std::move(other)) { }

protected:
	static string_t make_string(const char* text) {
		return string_t(text,text+std::strlen(text));
	}
	static std::string narrow(const string_t& text) {
		return std::string(text.begin(),text.end());
	}

	//---wire原语---
	struct writer {
		string_t& out;

		void byte(unsigned char value) {
			out.push_back(static_cast<typename string_t::value_type>(value));
		}
		void varint(unsigned long long value) {
			while (value>=0x80) {
				byte(static_cast<unsigned char>(value)|0x80);
				value>>=7;
			}
			byte(static_cast<unsigned char>(value));
		}
		void fixed32(unsigned long value) {
			for (int i=0;i<4;i++) byte(static_cast<unsigned char>((value>>(8*i))&0xFF));
		}
		void fixed64(unsigned long long value) {
			for (int i=0;i<8;i++) byte(static_cast<unsigned char>((value>>(8*i))&0xFF));
		}
		void tag(int number,int wire) {
			varint((static_cast<unsigned long long>(number)<<3)|static_cast<unsigned long long>(wire));
		}
		void length_delimited(const string_t& payload) {
			varint(payload.size());
			out.append(payload);
		}
	};
	struct reader {
		const unsigned char* first;
		const unsigned char* last;

		bool empty() const noexcept {
			return first>=last;
		}
		unsigned long long varint() {
			unsigned long long result=0;
			int shift=0;
			while (first<last) {
				const unsigned char c=*first;
				first++;
				result|=static_cast<unsigned long long>(c&0x7F)<<shift;
				if (!(c&0x80)) return result;
				shift+=7;
				if (shift>=64) throw std::runtime_error("Malformed varint");
			}
			throw std::runtime_error("Truncated varint");
		}
		unsigned long fixed32() {
			if (last-first<4) throw std::runtime_error("Truncated fixed32 value");
			unsigned long result=0;
			for (int i=0;i<4;i++) result|=static_cast<unsigned long>(first[i])<<(8*i);
			first+=4;
			return result;
		}
		unsigned long long fixed64() {
			if (last-first<8) throw std::runtime_error("Truncated fixed64 value");
			unsigned long long result=0;
			for (int i=0;i<8;i++) result|=static_cast<unsigned long long>(first[i])<<(8*i);
			first+=8;
			return result;
		}
		string_t bytes() {
			const unsigned long long length=varint();
			if (static_cast<unsigned long long>(last-first)<length) throw std::runtime_error("Truncated length-delimited payload");
			string_t result(reinterpret_cast<const typename string_t::value_type*>(first),reinterpret_cast<const typename string_t::value_type*>(first)+length);
			first+=length;
			return result;
		}
		reader sub() {
			const unsigned long long length=varint();
			if (static_cast<unsigned long long>(last-first)<length) throw std::runtime_error("Truncated length-delimited payload");
			reader result{first,first+length};
			first+=length;
			return result;
		}
		//捕获start-group(编号number)至配对end-group之间的原文字节(不含结束tag),嵌套组整体归入内容。
		string_t capture_group(int number) {
			const unsigned char* start=first;
			for (;;) {
				const unsigned char* mark=first;
				const unsigned long long head=varint();
				const int wire=static_cast<int>(head&7);
				const int inner=static_cast<int>(head>>3);
				if (wire==PWT_GROUP_END) {
					if (inner!=number) throw std::runtime_error("Mismatched end-group tag");
					return string_t(reinterpret_cast<const typename string_t::value_type*>(start),reinterpret_cast<const typename string_t::value_type*>(mark));
				}
				skip(wire);
			}
		}
		void skip(int wire) {
			switch (wire) {
				case PWT_VARINT: varint();return;
				case PWT_FIXED64: {
					if (last-first<8) throw std::runtime_error("Truncated fixed64 value");
					first+=8;
					return;
				}
				case PWT_LENGTH: {
					const unsigned long long length=varint();
					if (static_cast<unsigned long long>(last-first)<length) throw std::runtime_error("Truncated length-delimited payload");
					first+=length;
					return;
				}
				case PWT_FIXED32: {
					if (last-first<4) throw std::runtime_error("Truncated fixed32 value");
					first+=4;
					return;
				}
				case PWT_GROUP_START: {
					for (;;) {
						const unsigned long long head=varint();
						const int inner=static_cast<int>(head&7);
						if (inner==PWT_GROUP_END) return;
						skip(inner);
					}
				}
				case PWT_GROUP_END: throw std::runtime_error("Unexpected end-group tag");
				default: throw std::runtime_error("Unsupported wire type "+std::to_string(wire));
			}
		}
	};
	static unsigned long long zigzag_encode(long long value) {
		return (static_cast<unsigned long long>(value)<<1)^static_cast<unsigned long long>(value>>63);
	}
	static long long zigzag_decode(unsigned long long value) {
		return static_cast<long long>((value>>1)^(~(value&1)+1));
	}
	static int wire_of(protobuf_field_type type) {
		switch (type) {
			case PFT_DOUBLE:
			case PFT_FIXED64:
			case PFT_SFIXED64: return PWT_FIXED64;
			case PFT_FLOAT:
			case PFT_FIXED32:
			case PFT_SFIXED32: return PWT_FIXED32;
			case PFT_STRING:
			case PFT_BYTES:
			case PFT_MESSAGE: return PWT_LENGTH;
			default: return PWT_VARINT;
		}
	}
	static bool scalar_numeric(protobuf_field_type type) noexcept {
		switch (type) {
			case PFT_STRING:
			case PFT_BYTES:
			case PFT_MESSAGE: return false;
			default: return true;
		}
	}
	static long long node_to_int(const base_t& node) {
		switch (node.type()) {
			case structure::DDT_INT: return static_cast<long long>(*node.template get_ptr<const int_t*>());
			case structure::DDT_FLOAT: return static_cast<long long>(*node.template get_ptr<const float_t*>());
			case structure::DDT_BOOL: return *node.template get_ptr<const boolean_t*>()?1:0;
			default: throw std::invalid_argument("Field value is not numeric");
		}
	}
	static double node_to_float(const base_t& node) {
		switch (node.type()) {
			case structure::DDT_INT: return static_cast<double>(*node.template get_ptr<const int_t*>());
			case structure::DDT_FLOAT: return static_cast<double>(*node.template get_ptr<const float_t*>());
			case structure::DDT_BOOL: return *node.template get_ptr<const boolean_t*>()?1.0:0.0;
			default: throw std::invalid_argument("Field value is not numeric");
		}
	}

	//未知字段保留键:'$'不属proto合法标识符字符,不会与真实字段冲突。
	static string_t unknown_key() {
		return make_string("$unknown");
	}
	//未知字段落树:{"number":编号,"wire":wire类型,"data":载荷},载荷按wire取位保真形态
	//(varint/fixed→整数位值,length→字节串,start-group→组内原文字节)。
	static void store_unknown(base_t& target,int number,int wire,reader& in) {
		base_t entry(structure::DDT_OBJECT);
		auto& record=*entry.value().object;
		record.emplace(make_string("number"),base_t(static_cast<int_t>(number)));
		record.emplace(make_string("wire"),base_t(static_cast<int_t>(wire)));
		base_t data;
		switch (wire) {
			case PWT_VARINT: data=base_t(static_cast<int_t>(in.varint()));break;
			case PWT_FIXED64: data=base_t(static_cast<int_t>(in.fixed64()));break;
			case PWT_FIXED32: data=base_t(static_cast<int_t>(in.fixed32()));break;
			case PWT_LENGTH: data=base_t(in.bytes());break;
			case PWT_GROUP_START: data=base_t(in.capture_group(number));break;
			default: throw std::runtime_error("Unsupported wire type "+std::to_string(wire));
		}
		record.emplace(make_string("data"),std::move(data));
		auto& table=*target.value().object;
		auto it=table.find(unknown_key());
		if (it==table.end()) it=table.emplace(unknown_key(),base_t(structure::DDT_ARRAY)).first;
		else if (it->second.type()!=structure::DDT_ARRAY) it->second=base_t(structure::DDT_ARRAY);
		it->second.value().array->push_back(std::move(entry));
	}
	//未知字段重发:按保留形态逐条回写wire流,组以start/end tag包裹原文。
	static void write_unknown(writer& out,const base_t& list) {
		for (auto it=list.cbegin();it!=list.cend();it++) {
			const auto& record=*(*it).value().object;
			const auto number_slot=record.find(make_string("number"));
			const auto wire_slot=record.find(make_string("wire"));
			const auto data_slot=record.find(make_string("data"));
			if (number_slot==record.end() || wire_slot==record.end() || data_slot==record.end()) throw std::invalid_argument("Unknown-field entries require number, wire and data members");
			const int number=static_cast<int>(node_to_int(number_slot->second));
			const int wire=static_cast<int>(node_to_int(wire_slot->second));
			switch (wire) {
				case PWT_VARINT: {
					out.tag(number,PWT_VARINT);
					out.varint(static_cast<unsigned long long>(node_to_int(data_slot->second)));
					break;
				}
				case PWT_FIXED64: {
					out.tag(number,PWT_FIXED64);
					out.fixed64(static_cast<unsigned long long>(node_to_int(data_slot->second)));
					break;
				}
				case PWT_FIXED32: {
					out.tag(number,PWT_FIXED32);
					out.fixed32(static_cast<unsigned long>(static_cast<unsigned long long>(node_to_int(data_slot->second))&0xFFFFFFFFull));
					break;
				}
				case PWT_LENGTH: {
					if (data_slot->second.type()!=structure::DDT_STRING) throw std::invalid_argument("Length-delimited unknown fields require string data");
					out.tag(number,PWT_LENGTH);
					out.length_delimited(*data_slot->second.template get_ptr<const string_t*>());
					break;
				}
				case PWT_GROUP_START: {
					if (data_slot->second.type()!=structure::DDT_STRING) throw std::invalid_argument("Group unknown fields require string data");
					out.tag(number,PWT_GROUP_START);
					out.out.append(*data_slot->second.template get_ptr<const string_t*>());
					out.tag(number,PWT_GROUP_END);
					break;
				}
				default: throw std::invalid_argument("Unsupported wire type "+std::to_string(wire));
			}
		}
	}

	//---序列化---
	//标量载荷(不含tag):数值按wire编码,string/bytes带长度前缀,enum接受名字符串或整数。
	static void write_scalar(writer& out,protobuf_field_type type,const string_t& type_name,const base_t& node,const schema_t& schema) {
		switch (type) {
			case PFT_DOUBLE: {
				const double value=node_to_float(node);
				unsigned long long bits=0;
				std::memcpy(&bits,&value,sizeof(bits));
				out.fixed64(bits);
				return;
			}
			case PFT_FLOAT: {
				const float value=static_cast<float>(node_to_float(node));
				unsigned long bits=0;
				std::memcpy(&bits,&value,sizeof(float));
				out.fixed32(bits);
				return;
			}
			case PFT_INT32:
			case PFT_INT64:
			case PFT_UINT32:
			case PFT_UINT64: out.varint(static_cast<unsigned long long>(node_to_int(node)));return;
			case PFT_SINT32:
			case PFT_SINT64: out.varint(zigzag_encode(node_to_int(node)));return;
			case PFT_FIXED32:
			case PFT_SFIXED32: out.fixed32(static_cast<unsigned long>(static_cast<unsigned long long>(node_to_int(node))&0xFFFFFFFFull));return;
			case PFT_FIXED64:
			case PFT_SFIXED64: out.fixed64(static_cast<unsigned long long>(node_to_int(node)));return;
			case PFT_BOOL: out.varint(node_to_int(node)?1:0);return;
			case PFT_STRING:
			case PFT_BYTES: {
				if (node.type()!=structure::DDT_STRING) throw std::invalid_argument("Field value is not a string");
				out.length_delimited(*node.template get_ptr<const string_t*>());
				return;
			}
			case PFT_ENUM: {
				if (node.type()==structure::DDT_STRING) {
					const enum_t* definition=schema.find_enum(type_name);
					if (!definition) throw std::invalid_argument("Unknown enum type '"+std::string(type_name.begin(),type_name.end())+"'");
					long long value=0;
					if (!definition->find_number(*node.template get_ptr<const string_t*>(),value)) throw std::invalid_argument("Unknown enum value name");
					out.varint(static_cast<unsigned long long>(value));
					return;
				}
				out.varint(static_cast<unsigned long long>(node_to_int(node)));
				return;
			}
			default: throw std::invalid_argument("Message values must be written through write_single");
		}
	}
	//单字段=tag+载荷:message递归编码到临时缓冲后带长度前缀落盘。
	static void write_single(writer& out,const field_t& field,const base_t& node,const schema_t& schema) {
		if (field.type==PFT_MESSAGE) {
			const message_t* definition=schema.find_message(field.type_name);
			if (!definition) throw std::invalid_argument("Unknown message type '"+std::string(field.type_name.begin(),field.type_name.end())+"'");
			string_t buffer;
			writer nested{buffer};
			write_message_body(nested,*definition,node,schema);
			out.tag(field.number,PWT_LENGTH);
			out.length_delimited(buffer);
			return;
		}
		out.tag(field.number,wire_of(field.type));
		write_scalar(out,field.type,field.type_name,node,schema);
	}
	static void write_packed(writer& out,const field_t& field,const base_t& node,const schema_t& schema) {
		string_t buffer;
		writer nested{buffer};
		for (auto it=node.cbegin();it!=node.cend();it++) write_scalar(nested,field.type,field.type_name,*it,schema);
		out.tag(field.number,PWT_LENGTH);
		out.length_delimited(buffer);
	}
	//map:每个键值对编码为entry子消息(1=key,2=value)。
	static void write_map(writer& out,const field_t& field,const base_t& node,const schema_t& schema) {
		if (node.type()!=structure::DDT_OBJECT) throw std::invalid_argument("Map field value must be an object");
		field_t key_field;
		key_field.number=1;
		key_field.type=field.key_type;
		field_t value_field;
		value_field.number=2;
		value_field.type=field.type;
		value_field.type_name=field.type_name;
		for (auto it=node.cbegin();it!=node.cend();it++) {
			string_t buffer;
			writer entry{buffer};
			const base_t key=map_key_from_string(field.key_type,it.key());
			write_single(entry,key_field,key,schema);
			write_single(entry,value_field,*it,schema);
			out.tag(field.number,PWT_LENGTH);
			out.length_delimited(buffer);
		}
	}
	//消息体:按schema字段序遍历,对象中缺失或为null的字段跳过(隐式缺省);repeated要求数组。
	static void write_message_body(writer& out,const message_t& message,const base_t& node,const schema_t& schema) {
		if (node.type()!=structure::DDT_OBJECT) throw std::invalid_argument("Message value must be an object");
		const auto& table=*node.value().object;
		std::vector<int> oneof_counts(message.oneofs.size(),0);
		for (const auto& field:message.fields) {
			const auto it=table.find(field.name);
			if (it==table.end() || it->second.type()==structure::DDT_NULL) continue;
			if (field.oneof_index>=0 && static_cast<std::size_t>(field.oneof_index)<oneof_counts.size()) {
				oneof_counts[field.oneof_index]++;
				if (oneof_counts[field.oneof_index]>1) throw std::invalid_argument("Multiple members of oneof '"+std::string(message.oneofs[field.oneof_index].begin(),message.oneofs[field.oneof_index].end())+"' are set");
			}
			if (field.is_map) {
				write_map(out,field,it->second,schema);
				continue;
			}
			if (field.label==PFL_REPEATED) {
				if (it->second.type()!=structure::DDT_ARRAY) throw std::invalid_argument("Repeated field value must be an array");
				if (field.packed && scalar_numeric(field.type) && !it->second.empty()) {
					write_packed(out,field,it->second,schema);
					continue;
				}
				for (auto element=it->second.cbegin();element!=it->second.cend();element++) write_single(out,field,*element,schema);
				continue;
			}
			write_single(out,field,it->second,schema);
		}
		const auto unknown=table.find(unknown_key());
		if (unknown!=table.end() && unknown->second.type()==structure::DDT_ARRAY) write_unknown(out,unknown->second);
	}

	//---反序列化---
	static base_t read_scalar(reader& in,protobuf_field_type type,const string_t& type_name,const schema_t& schema) {
		switch (type) {
			case PFT_DOUBLE: {
				const unsigned long long bits=in.fixed64();
				double value=0;
				std::memcpy(&value,&bits,sizeof(value));
				return base_t(static_cast<float_t>(value));
			}
			case PFT_FLOAT: {
				const unsigned long bits=in.fixed32();
				float value=0;
				const unsigned char raw[4]={static_cast<unsigned char>(bits&0xFF),static_cast<unsigned char>((bits>>8)&0xFF),static_cast<unsigned char>((bits>>16)&0xFF),static_cast<unsigned char>((bits>>24)&0xFF)};
				std::memcpy(&value,raw,sizeof(value));
				return base_t(static_cast<float_t>(value));
			}
			case PFT_INT32:
			case PFT_INT64: return base_t(static_cast<int_t>(static_cast<long long>(in.varint())));
			case PFT_UINT32: return base_t(static_cast<int_t>(in.varint()&0xFFFFFFFFull));
			case PFT_UINT64: return base_t(static_cast<int_t>(in.varint()));
			case PFT_SINT32:
			case PFT_SINT64: return base_t(static_cast<int_t>(zigzag_decode(in.varint())));
			case PFT_FIXED32: return base_t(static_cast<int_t>(in.fixed32()));
			case PFT_SFIXED32: {
				const unsigned long bits=in.fixed32()&0xFFFFFFFFul;
				long long value=static_cast<long long>(bits);
				if (bits&0x80000000ul) value-=0x100000000ll;
				return base_t(static_cast<int_t>(value));
			}
			case PFT_FIXED64: return base_t(static_cast<int_t>(in.fixed64()));
			case PFT_SFIXED64: return base_t(static_cast<int_t>(static_cast<long long>(in.fixed64())));
			case PFT_BOOL: return base_t(static_cast<boolean_t>(in.varint()!=0));
			case PFT_STRING:
			case PFT_BYTES: return base_t(in.bytes());
			case PFT_ENUM: {
				const long long value=static_cast<long long>(in.varint());
				const enum_t* definition=schema.find_enum(type_name);
				if (definition) {
					const string_t* name=definition->find_name(value);
					if (name) return base_t(string_t(*name));
				}
				return base_t(static_cast<int_t>(value));
			}
			default: throw std::invalid_argument("Message values must be read through read_field");
		}
	}
	static void place_value(const field_t& field,base_t& target,base_t&& value) {
		auto& table=*target.value().object;
		if (field.label==PFL_REPEATED && !field.is_map) {
			auto it=table.find(field.name);
			if (it==table.end()) it=table.emplace(field.name,base_t(structure::DDT_ARRAY)).first;
			else if (it->second.type()!=structure::DDT_ARRAY) it->second=base_t(structure::DDT_ARRAY);
			it->second.value().array->push_back(std::move(value));
			return;
		}
		auto it=table.find(field.name);
		if (it==table.end()) table.emplace(field.name,std::move(value));
		else it->second=std::move(value);
	}
	static void read_field(reader& in,const field_t& field,int wire,base_t& target,const schema_t& schema,bool preserve) {
		if (field.is_map) {
			if (wire!=PWT_LENGTH) throw std::runtime_error("Map entries must be length-delimited");
			reader entry=in.sub();
			base_t key;
			base_t value;
			bool has_value=false;
			while (!entry.empty()) {
				const unsigned long long head=entry.varint();
				const int entry_wire=static_cast<int>(head&7);
				const int number=static_cast<int>(head>>3);
				if (number==1) key=read_scalar(entry,field.key_type,string_t(),schema);
				else if (number==2) {
					if (field.type==PFT_MESSAGE) {
						const message_t* definition=schema.find_message(field.type_name);
						if (!definition) throw std::invalid_argument("Unknown message type '"+std::string(field.type_name.begin(),field.type_name.end())+"'");
						reader nested=entry.sub();
						value=base_t(structure::DDT_OBJECT);
						read_message_body(nested,*definition,value,schema,preserve);
					} else value=read_scalar(entry,field.type,field.type_name,schema);
					has_value=true;
				} else entry.skip(entry_wire);
			}
			auto& table=*target.value().object;
			auto it=table.find(field.name);
			if (it==table.end()) it=table.emplace(field.name,base_t(structure::DDT_OBJECT)).first;
			else if (it->second.type()!=structure::DDT_OBJECT) it->second=base_t(structure::DDT_OBJECT);
			const string_t key_text=map_key_to_string(key);
			auto& entries=*it->second.value().object;
			base_t stored=has_value?std::move(value):default_value(field,schema);
			const auto slot=entries.find(key_text);
			if (slot==entries.end()) entries.emplace(key_text,std::move(stored));
			else slot->second=std::move(stored);
			return;
		}
		if (field.type==PFT_MESSAGE) {
			if (wire!=PWT_LENGTH) throw std::runtime_error("Message fields must be length-delimited");
			const message_t* definition=schema.find_message(field.type_name);
			if (!definition) throw std::invalid_argument("Unknown message type '"+std::string(field.type_name.begin(),field.type_name.end())+"'");
			reader nested=in.sub();
			base_t child(structure::DDT_OBJECT);
			read_message_body(nested,*definition,child,schema,preserve);
			place_value(field,target,std::move(child));
			return;
		}
		if (field.label==PFL_REPEATED && scalar_numeric(field.type) && wire==PWT_LENGTH) {
			reader packed=in.sub();
			while (!packed.empty()) place_value(field,target,read_scalar(packed,field.type,field.type_name,schema));
			return;
		}
		place_value(field,target,read_scalar(in,field.type,field.type_name,schema));
	}
	//map条目缺value时的缺省值(proto3零值)。
	static base_t default_value(const field_t& field,const schema_t& schema) {
		switch (field.type) {
			case PFT_DOUBLE:
			case PFT_FLOAT: return base_t(static_cast<float_t>(0));
			case PFT_BOOL: return base_t(static_cast<boolean_t>(false));
			case PFT_STRING:
			case PFT_BYTES: return base_t(string_t());
			case PFT_MESSAGE: return base_t(structure::DDT_OBJECT);
			case PFT_ENUM: {
				const enum_t* definition=schema.find_enum(field.type_name);
				if (definition) {
					const string_t* name=definition->find_name(0);
					if (name) return base_t(string_t(*name));
				}
				return base_t(static_cast<int_t>(0));
			}
			default: return base_t(static_cast<int_t>(0));
		}
	}
	static void read_message_body(reader& in,const message_t& message,base_t& target,const schema_t& schema,bool preserve) {
		while (!in.empty()) {
			const unsigned long long head=in.varint();
			const int wire=static_cast<int>(head&7);
			const int number=static_cast<int>(head>>3);
			const field_t* field=message.find_field(number);
			if (!field) {
				if (preserve) store_unknown(target,number,wire,in);
				else in.skip(wire);
				continue;
			}
			read_field(in,*field,wire,target,schema,preserve);
		}
	}
	static string_t map_key_to_string(const base_t& key) {
		switch (key.type()) {
			case structure::DDT_STRING: return *key.template get_ptr<const string_t*>();
			case structure::DDT_INT: {
				const std::string text=std::to_string(static_cast<long long>(*key.template get_ptr<const int_t*>()));
				return string_t(text.begin(),text.end());
			}
			case structure::DDT_BOOL: return make_string(*key.template get_ptr<const boolean_t*>()?"true":"false");
			default: throw std::runtime_error("Unsupported map key kind");
		}
	}
	static base_t map_key_from_string(protobuf_field_type type,const string_t& key) {
		switch (type) {
			case PFT_STRING: return base_t(string_t(key));
			case PFT_BOOL: return base_t(static_cast<boolean_t>(key==make_string("true")));
			default: {
				const std::string text(key.begin(),key.end());
				return base_t(static_cast<int_t>(std::strtoll(text.c_str(),nullptr,10)));
			}
		}
	}

public:
	//---序列化/反序列化入口---
	static string_t serialize(const base_t& node,const schema_t& schema,const string_t& message_name) {
		const message_t* definition=schema.find_message(message_name);
		if (!definition) throw std::invalid_argument("Unknown message type '"+std::string(message_name.begin(),message_name.end())+"'");
		string_t out;
		writer sink{out};
		write_message_body(sink,*definition,node,schema);
		return out;
	}
	string_t dump(const schema_t& schema,const string_t& message_name) const {
		return protobuf::serialize(*this,schema,message_name);
	}
	static protobuf parse(std::string_view input,const schema_t& schema,const string_t& message_name,bool preserve_unknown=true,bool allow_exceptions=true) {
		try {
			const message_t* definition=schema.find_message(message_name);
			if (!definition) throw std::invalid_argument("Unknown message type '"+std::string(message_name.begin(),message_name.end())+"'");
			protobuf result;
			static_cast<base_t&>(result)=base_t(structure::DDT_OBJECT);
			reader source{reinterpret_cast<const unsigned char*>(input.data()),reinterpret_cast<const unsigned char*>(input.data())+input.size()};
			read_message_body(source,*definition,result,schema,preserve_unknown);
			return result;
		} catch (...) {
			if (allow_exceptions) throw;
			return protobuf(nullptr);
		}
	}
	static bool try_parse(std::string_view input,const schema_t& schema,const string_t& message_name,protobuf& out,bool preserve_unknown=true) {
		try {
			protobuf result=parse(input,schema,message_name,preserve_unknown,true);
			out=std::move(result);
			return true;
		} catch (...) {
			return false;
		}
	}
	static bool accept(std::string_view input,const schema_t& schema,const string_t& message_name) {
		protobuf discarded;
		return try_parse(input,schema,message_name,discarded,false);
	}
	//流入口:吞读整个流后解析(wire为定长二进制,无增量语义)。
	static protobuf parse(std::istream& input,const schema_t& schema,const string_t& message_name,bool preserve_unknown=true,bool allow_exceptions=true) {
		std::string content;
		char buffer[4096];
		while (input.read(buffer,sizeof(buffer)) || input.gcount()) content.append(buffer,static_cast<std::size_t>(input.gcount()));
		return parse(std::string_view(content),schema,message_name,preserve_unknown,allow_exceptions);
	}
};

_STDEX_DOM_TPL_DEFAULT_DECLARATION
inline typename protobuf<_Int,_Float,_Boolean,_String,_Array,_Object,_Allocator>::string_t to_string(const protobuf<_Int,_Float,_Boolean,_String,_Array,_Object,_Allocator>& value,const typename protobuf<_Int,_Float,_Boolean,_String,_Array,_Object,_Allocator>::schema_t& schema,const typename protobuf<_Int,_Float,_Boolean,_String,_Array,_Object,_Allocator>::string_t& message_name) {
	return value.dump(schema,message_name);
}

}

_STDEX_DOM_TPL_DEFAULT_DECLARATION
using protobuf_t=basic_protobuf::protobuf<_Int,_Float,_Boolean,_String,_Array,_Object,_Allocator>;
using protobuf=protobuf_t<>;
template <typename _String=std::string>
using protobuf_schema=basic_protobuf::protobuf_schema<_String>;
using basic_protobuf::to_string;
using basic_protobuf::protobuf_field_type;
using basic_protobuf::protobuf_field_label;
using basic_protobuf::protobuf_wire_type;
using basic_protobuf::PFT_DOUBLE;
using basic_protobuf::PFT_FLOAT;
using basic_protobuf::PFT_INT32;
using basic_protobuf::PFT_INT64;
using basic_protobuf::PFT_UINT32;
using basic_protobuf::PFT_UINT64;
using basic_protobuf::PFT_SINT32;
using basic_protobuf::PFT_SINT64;
using basic_protobuf::PFT_FIXED32;
using basic_protobuf::PFT_FIXED64;
using basic_protobuf::PFT_SFIXED32;
using basic_protobuf::PFT_SFIXED64;
using basic_protobuf::PFT_BOOL;
using basic_protobuf::PFT_STRING;
using basic_protobuf::PFT_BYTES;
using basic_protobuf::PFT_MESSAGE;
using basic_protobuf::PFT_ENUM;
using basic_protobuf::PFL_SINGULAR;
using basic_protobuf::PFL_OPTIONAL;
using basic_protobuf::PFL_REPEATED;
using basic_protobuf::PWT_VARINT;
using basic_protobuf::PWT_FIXED64;
using basic_protobuf::PWT_LENGTH;
using basic_protobuf::PWT_FIXED32;
using basic_protobuf::basic_protobuf_field;
using basic_protobuf::basic_protobuf_message;
using basic_protobuf::basic_protobuf_enum;
using basic_protobuf::basic_protobuf_enum_value;

}

}

#endif
