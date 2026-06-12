//Last Modified At 2026/06/11
//@Version 1.0.0.0
#ifndef _STDEX_TYPE_DOM_JSON_H_
#define _STDEX_TYPE_DOM_JSON_H_ 1

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <istream>
#include <mutex>
#include <ostream>
#include <regex>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include "../../structure/dom.h"//At Least 1.0
#include "../../syntax/parser.h"//At Least 3.4

namespace stdex {

namespace type {

namespace basic_json {

enum json_symbol : int {
	JS_EPSILON,
	JS_EOF,
	JS_LBRACE,
	JS_RBRACE,
	JS_LBRACKET,
	JS_RBRACKET,
	JS_COLON,
	JS_COMMA,
	JS_STRING,
	JS_INT,
	JS_FLOAT,
	JS_TRUE,
	JS_FALSE,
	JS_NULL,
	JS_START,
	JS_VALUE,
	JS_OBJECT,
	JS_MEMBERS,
	JS_MEMBER,
	JS_ARRAY,
	JS_ELEMENTS,
};

enum json_production : int {
	JP_START,
	JP_VALUE_OBJECT,
	JP_VALUE_ARRAY,
	JP_VALUE_STRING,
	JP_VALUE_INT,
	JP_VALUE_FLOAT,
	JP_VALUE_TRUE,
	JP_VALUE_FALSE,
	JP_VALUE_NULL,
	JP_OBJECT_EMPTY,
	JP_OBJECT,
	JP_MEMBERS_FIRST,
	JP_MEMBERS_APPEND,
	JP_MEMBER,
	JP_ARRAY_EMPTY,
	JP_ARRAY,
	JP_ELEMENTS_FIRST,
	JP_ELEMENTS_APPEND,
};

template <typename _Json>
struct json_sax {
	using int_t=typename _Json::int_t;
	using float_t=typename _Json::float_t;
	using boolean_t=typename _Json::boolean_t;
	using string_t=typename _Json::string_t;

	virtual bool null()=0;
	virtual bool boolean(boolean_t value)=0;
	virtual bool number_integer(int_t value)=0;
	virtual bool number_float(float_t value,const string_t& raw)=0;
	virtual bool string(string_t& value)=0;
	virtual bool start_object(std::size_t cnt)=0;
	virtual bool key(string_t& value)=0;
	virtual bool end_object()=0;
	virtual bool start_array(std::size_t cnt)=0;
	virtual bool end_array()=0;
	virtual bool parse_error(std::size_t position,const std::string& last_token,const std::string& message)=0;
	virtual ~json_sax()=default;
};

template <typename _Json>
class json_sax_dom_builder : public json_sax<_Json> {
public:
	using int_t=typename _Json::int_t;
	using float_t=typename _Json::float_t;
	using boolean_t=typename _Json::boolean_t;
	using string_t=typename _Json::string_t;

private:
	_Json& root_;
	std::vector<_Json*> ref_stack_;
	string_t key_;
	bool root_set_=false;
	bool errored_=false;
	std::size_t error_position_=0;
	std::string error_message_;

	template <typename _Vp>
	_Json* handle_value(_Vp&& value) {
		if (ref_stack_.empty()) {
			root_=_Json(std::forward<_Vp>(value));
			root_set_=true;
			return &root_;
		}
		_Json* parent=ref_stack_.back();
		if (parent->is_array()) {
			parent->push_back(std::forward<_Vp>(value));
			return static_cast<_Json*>(&parent->back());
		}
		auto& slot=(*parent)[std::move(key_)];
		slot=std::forward<_Vp>(value);
		return static_cast<_Json*>(&slot);
	}

public:
	explicit json_sax_dom_builder(_Json& root) : root_(root) { }
	bool null() override {
		handle_value(nullptr);
		return true;
	}
	bool boolean(boolean_t value) override {
		handle_value(value);
		return true;
	}
	bool number_integer(int_t value) override {
		handle_value(value);
		return true;
	}
	bool number_float(float_t value,const string_t& raw) override {
		static_cast<void>(raw);
		handle_value(value);
		return true;
	}
	bool string(string_t& value) override {
		handle_value(std::move(value));
		return true;
	}
	bool start_object(std::size_t cnt) override {
		static_cast<void>(cnt);
		ref_stack_.push_back(handle_value(_Json(structure::DDT_OBJECT)));
		return true;
	}
	bool key(string_t& value) override {
		key_=std::move(value);
		return true;
	}
	bool end_object() override {
		ref_stack_.pop_back();
		return true;
	}
	bool start_array(std::size_t cnt) override {
		static_cast<void>(cnt);
		ref_stack_.push_back(handle_value(_Json(structure::DDT_ARRAY)));
		return true;
	}
	bool end_array() override {
		ref_stack_.pop_back();
		return true;
	}
	bool parse_error(std::size_t position,const std::string& last_token,const std::string& message) override {
		errored_=true;
		error_position_=position;
		error_message_=message+(last_token.empty()?std::string():(" near '"+last_token+"'"));
		return false;
	}
	bool errored() const noexcept {
		return errored_;
	}
	bool completed() const noexcept {
		return root_set_ && ref_stack_.empty();
	}
	std::size_t error_position() const noexcept {
		return error_position_;
	}
	const std::string& error_message() const noexcept {
		return error_message_;
	}
};

template <typename _Json>
class json_sax_acceptor : public json_sax<_Json> {
public:
	using int_t=typename _Json::int_t;
	using float_t=typename _Json::float_t;
	using boolean_t=typename _Json::boolean_t;
	using string_t=typename _Json::string_t;

	bool null() override { return true; }
	bool boolean(boolean_t) override { return true; }
	bool number_integer(int_t) override { return true; }
	bool number_float(float_t,const string_t&) override { return true; }
	bool string(string_t&) override { return true; }
	bool start_object(std::size_t) override { return true; }
	bool key(string_t&) override { return true; }
	bool end_object() override { return true; }
	bool start_array(std::size_t) override { return true; }
	bool end_array() override { return true; }
	bool parse_error(std::size_t,const std::string&,const std::string&) override { return false; }
};

_STDEX_DOM_TPL_DECLARATION
class json : public structure::_STDEX_DOM_DEF {
public:
	using base_t=structure::_STDEX_DOM_DEF;
	using int_t=typename base_t::int_t;
	using float_t=typename base_t::float_t;
	using boolean_t=typename base_t::boolean_t;
	using string_t=typename base_t::string_t;
	using size_type=typename base_t::size_type;
	using sax_t=json_sax<json>;

	static_assert(sizeof(typename string_t::value_type)==1,"json serializer assumes a byte-oriented (UTF-8) string_t.");

	using base_t::base_t;
	using base_t::operator =;

	json()=default;
	~json() override=default;

	json(const json&)=default;
	json(json&&) noexcept=default;

	json& operator =(const json&)=default;
	json& operator =(json&&)=default;

	json(const base_t& other) : base_t(other) { }
	json(base_t&& other) noexcept : base_t(std::move(other)) { }

	static json array(typename base_t::initializer_list_t init_list={}) {
		return json(base_t::array(init_list));
	}
	static json object(typename base_t::initializer_list_t init_list={}) {
		return json(base_t::object(init_list));
	}

private:
	struct json_token {
		json_symbol symbol;
		std::size_t position;
		string_t string{};
		int_t integer=0;
		float_t floating=0;
		bool key=false;
	};

	static const std::regex& whitespace_regex() {
		static const std::regex result(R"([ \t\r\n]+)",std::regex::optimize);
		return result;
	}
	static const std::regex& string_regex() {
		static const std::regex result(R"("(?:[^"\\\x00-\x1F]|\\(?:["\\/bfnrt]|u[0-9A-Fa-f]{4}))*")",std::regex::optimize);
		return result;
	}
	static const std::regex& number_regex() {
		static const std::regex result(R"(-?(?:0|[1-9][0-9]*)(\.[0-9]+)?([eE][+-]?[0-9]+)?)",std::regex::optimize);
		return result;
	}
	static const std::regex& literal_regex() {
		static const std::regex result(R"(true|false|null)",std::regex::optimize);
		return result;
	}
	static const std::regex& punctuation_regex() {
		static const std::regex result(R"([{}\[\]:,])",std::regex::optimize);
		return result;
	}

	static bool decode_string(const char* first,const char* last,string_t& out,std::string& error_message) {
		first++;
		last--;
		auto append_codepoint=[&out](unsigned long cp){
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
		};
		auto hex4=[](const char* p)->unsigned long{
			unsigned long result=0;
			for (int i=0;i<4;i++) {
				const char c=p[i];
				result<<=4;
				if (c>='0' && c<='9') result|=static_cast<unsigned long>(c-'0');
				else if (c>='a' && c<='f') result|=static_cast<unsigned long>(c-'a'+10);
				else result|=static_cast<unsigned long>(c-'A'+10);
			}
			return result;
		};
		while (first<last) {
			if (*first!='\\') {
				out.push_back(*first++);
				continue;
			}
			first++;
			switch (*first++) {
				case '"': out.push_back('"');break;
				case '\\': out.push_back('\\');break;
				case '/': out.push_back('/');break;
				case 'b': out.push_back('\b');break;
				case 'f': out.push_back('\f');break;
				case 'n': out.push_back('\n');break;
				case 'r': out.push_back('\r');break;
				case 't': out.push_back('\t');break;
				case 'u': {
					unsigned long cp=hex4(first);
					first+=4;
					if (cp>=0xD800 && cp<=0xDBFF) {
						if (last-first>=6 && first[0]=='\\' && first[1]=='u') {
							const unsigned long low=hex4(first+2);
							if (low>=0xDC00 && low<=0xDFFF) {
								cp=0x10000+((cp-0xD800)<<10)+(low-0xDC00);
								first+=6;
							} else {
								error_message="invalid low surrogate in \\u escape";
								return false;
							}
						} else {
							error_message="lone high surrogate in \\u escape";
							return false;
						}
					} else if (cp>=0xDC00 && cp<=0xDFFF) {
						error_message="lone low surrogate in \\u escape";
						return false;
					}
					append_codepoint(cp);
					break;
				}
				default: {
					error_message="invalid escape sequence";
					return false;
				}
			}
		}
		return true;
	}

	static bool tokenize(std::string_view input,std::vector<json_token>& tokens,std::size_t& error_position,std::string& error_message) {
		const char* first=input.data();
		const char* const last=input.data()+input.size();
		std::cmatch match;
		const auto flags=std::regex_constants::match_continuous;
		while (first<last) {
			if (std::regex_search(first,last,match,whitespace_regex(),flags)) {
				first=match[0].second;
				continue;
			}
			json_token token;
			token.position=static_cast<std::size_t>(first-input.data());
			if (std::regex_search(first,last,match,punctuation_regex(),flags)) {
				switch (*first) {
					case '{': token.symbol=JS_LBRACE;break;
					case '}': token.symbol=JS_RBRACE;break;
					case '[': token.symbol=JS_LBRACKET;break;
					case ']': token.symbol=JS_RBRACKET;break;
					case ':': token.symbol=JS_COLON;break;
					default: token.symbol=JS_COMMA;break;
				}
				first=match[0].second;
			} else if (std::regex_search(first,last,match,string_regex(),flags)) {
				token.symbol=JS_STRING;
				if (!decode_string(match[0].first,match[0].second,token.string,error_message)) {
					error_position=token.position;
					return false;
				}
				first=match[0].second;
			} else if (std::regex_search(first,last,match,number_regex(),flags)) {
				const std::string text(match[0].first,match[0].second);
				if (match[1].matched || match[2].matched) {
					token.symbol=JS_FLOAT;
					token.floating=static_cast<float_t>(std::strtod(text.c_str(),nullptr));
				} else {
					errno=0;
					const long long value=std::strtoll(text.c_str(),nullptr,10);
					if (errno==ERANGE || value<static_cast<long long>((std::numeric_limits<int_t>::min)()) || value>static_cast<long long>((std::numeric_limits<int_t>::max)())) {
						token.symbol=JS_FLOAT;
						token.floating=static_cast<float_t>(std::strtod(text.c_str(),nullptr));
					} else {
						token.symbol=JS_INT;
						token.integer=static_cast<int_t>(value);
					}
				}
				token.string=string_t(text.begin(),text.end());
				first=match[0].second;
			} else if (std::regex_search(first,last,match,literal_regex(),flags)) {
				token.symbol=(*first=='t')?JS_TRUE:((*first=='f')?JS_FALSE:JS_NULL);
				first=match[0].second;
			} else {
				error_position=token.position;
				error_message=std::string("unexpected character '")+*first+"'";
				return false;
			}
			tokens.push_back(std::move(token));
		}
		for (std::size_t i=0;i+1<tokens.size();i++) {
			if (tokens[i].symbol==JS_STRING && tokens[i+1].symbol==JS_COLON) tokens[i].key=true;
		}
		json_token eof_token;
		eof_token.symbol=JS_EOF;
		eof_token.position=input.size();
		tokens.push_back(std::move(eof_token));
		return true;
	}

	using parser_t=syntax::parser<json_symbol,json_production>;

	static bool initialize_grammar(parser_t& target) {
		auto unit=[](json_symbol left,std::initializer_list<json_symbol> rights,json_production id){
			return syntax::single_parser_unit<json_symbol,json_production>(left,rights,id);
		};
		target.units={
			unit(JS_START,{JS_VALUE,JS_EOF},JP_START),
			unit(JS_VALUE,{JS_OBJECT},JP_VALUE_OBJECT),
			unit(JS_VALUE,{JS_ARRAY},JP_VALUE_ARRAY),
			unit(JS_VALUE,{JS_STRING},JP_VALUE_STRING),
			unit(JS_VALUE,{JS_INT},JP_VALUE_INT),
			unit(JS_VALUE,{JS_FLOAT},JP_VALUE_FLOAT),
			unit(JS_VALUE,{JS_TRUE},JP_VALUE_TRUE),
			unit(JS_VALUE,{JS_FALSE},JP_VALUE_FALSE),
			unit(JS_VALUE,{JS_NULL},JP_VALUE_NULL),
			unit(JS_OBJECT,{JS_LBRACE,JS_RBRACE},JP_OBJECT_EMPTY),
			unit(JS_OBJECT,{JS_LBRACE,JS_MEMBERS,JS_RBRACE},JP_OBJECT),
			unit(JS_MEMBERS,{JS_MEMBER},JP_MEMBERS_FIRST),
			unit(JS_MEMBERS,{JS_MEMBERS,JS_COMMA,JS_MEMBER},JP_MEMBERS_APPEND),
			unit(JS_MEMBER,{JS_STRING,JS_COLON,JS_VALUE},JP_MEMBER),
			unit(JS_ARRAY,{JS_LBRACKET,JS_RBRACKET},JP_ARRAY_EMPTY),
			unit(JS_ARRAY,{JS_LBRACKET,JS_ELEMENTS,JS_RBRACKET},JP_ARRAY),
			unit(JS_ELEMENTS,{JS_VALUE},JP_ELEMENTS_FIRST),
			unit(JS_ELEMENTS,{JS_ELEMENTS,JS_COMMA,JS_VALUE},JP_ELEMENTS_APPEND),
		};
		target.generate_parser();
		return true;
	}
	static parser_t& grammar() {
		static parser_t instance(JS_START,JS_EPSILON,JS_EOF);
		static const bool initialized=initialize_grammar(instance);
		static_cast<void>(initialized);
		return instance;
	}
	static std::mutex& grammar_mutex() {
		static std::mutex instance;
		return instance;
	}

	class json_listener : public syntax::parser_listener<json_symbol,json_production> {
		std::vector<json_token>* tokens_=nullptr;
		sax_t* sax_=nullptr;
		bool aborted_=false;
		bool failed_=false;

		void abort_check(bool keep_going) {
			if (!keep_going) aborted_=true;
		}

	public:
		void reset(std::vector<json_token>& tokens,sax_t& sax) {
			tokens_=&tokens;
			sax_=&sax;
			aborted_=false;
			failed_=false;
			this->enabled=true;
		}
		bool aborted() const noexcept {
			return aborted_;
		}
		bool failed() const noexcept {
			return failed_;
		}
		intptr_t on_shift(uintptr_t id,int state,json_symbol word) override {
			static_cast<void>(state);
			if (aborted_ || failed_) return 0;
			json_token& token=(*tokens_)[id-1];
			switch (word) {
				case JS_LBRACE: abort_check(sax_->start_object(static_cast<std::size_t>(-1)));break;
				case JS_LBRACKET: abort_check(sax_->start_array(static_cast<std::size_t>(-1)));break;
				case JS_STRING: {
					if (token.key) abort_check(sax_->key(token.string));
					break;
				}
				default: break;
			}
			return 0;
		}
		intptr_t on_reduction(uintptr_t id,int state,int next,json_production sentence_id,int reduction_num) override {
			static_cast<void>(state);
			static_cast<void>(next);
			static_cast<void>(reduction_num);
			if (aborted_ || failed_) return 0;
			json_token& token=(*tokens_)[id-2];
			switch (sentence_id) {
				case JP_VALUE_STRING: abort_check(sax_->string(token.string));break;
				case JP_VALUE_INT: abort_check(sax_->number_integer(token.integer));break;
				case JP_VALUE_FLOAT: abort_check(sax_->number_float(token.floating,token.string));break;
				case JP_VALUE_TRUE: abort_check(sax_->boolean(static_cast<boolean_t>(true)));break;
				case JP_VALUE_FALSE: abort_check(sax_->boolean(static_cast<boolean_t>(false)));break;
				case JP_VALUE_NULL: abort_check(sax_->null());break;
				case JP_OBJECT_EMPTY:
				case JP_OBJECT: abort_check(sax_->end_object());break;
				case JP_ARRAY_EMPTY:
				case JP_ARRAY: abort_check(sax_->end_array());break;
				case JP_START:
				case JP_VALUE_OBJECT:
				case JP_VALUE_ARRAY:
				case JP_MEMBERS_FIRST:
				case JP_MEMBERS_APPEND:
				case JP_MEMBER:
				case JP_ELEMENTS_FIRST:
				case JP_ELEMENTS_APPEND:
				default: break;
			}
			return 0;
		}
		void on_accept() override { }
		int on_error(uintptr_t id,typename syntax::parser_listener<json_symbol,json_production>::error_type type,int state,json_symbol word) override {
			static_cast<void>(type);
			static_cast<void>(state);
			static_cast<void>(word);
			failed_=true;
			if (sax_ && tokens_ && id!=static_cast<uintptr_t>(-1) && id>=1 && id<=tokens_->size()) {
				const json_token& token=(*tokens_)[id-1];
				const std::string text(token.string.begin(),token.string.end());
				sax_->parse_error(token.position,text,"Unexpected token");
			} else if (sax_) sax_->parse_error(0,std::string(),"Unexpected end of input");
			return 0;
		}
	};

public:
	static bool sax_parse(std::string_view input,sax_t* sax) {
		std::vector<json_token> tokens;
		std::size_t error_position=0;
		std::string error_message;
		if (!tokenize(input,tokens,error_position,error_message)) {
			sax->parse_error(error_position,std::string(),error_message);
			return false;
		}
		if (tokens.size()==1) {
			sax->parse_error(0,std::string(),"Attempting to parse an empty input");
			return false;
		}
		std::vector<typename parser_t::parse_node> nodes;
		nodes.reserve(tokens.size());
		for (const auto& it:tokens) {
			typename parser_t::parse_node node;
			node.op=it.symbol;
			nodes.push_back(std::move(node));
		}
		std::lock_guard<std::mutex> lock(grammar_mutex());
		parser_t& parser=grammar();
		static json_listener listener;
		listener.reset(tokens,*sax);
		parser.listeners.push_back(&listener);
		bool result=false;
		try {
			result=parser.parse_with_listener(nodes);
		} catch (...) {
			parser.listeners.pop_back();
			throw;
		}
		parser.listeners.pop_back();
		return result && !listener.aborted() && !listener.failed();
	}
	static json parse(std::string_view input,bool allow_exceptions=true) {
		json result;
		json_sax_dom_builder<json> builder(result);
		const bool ok=sax_parse(input,&builder) && builder.completed();
		if (!ok) {
			if (allow_exceptions) throw std::runtime_error(std::string("Parse error at byte ")+std::to_string(builder.error_position())+std::string(": ")+(builder.error_message().empty()?std::string("Incomplete document"):builder.error_message()));
			return json();
		}
		return result;
	}
	static bool try_parse(std::string_view input,json& out) {
		json result;
		json_sax_dom_builder<json> builder(result);
		if (!sax_parse(input,&builder) || !builder.completed()) return false;
		out=std::move(result);
		return true;
	}
	static bool accept(std::string_view input) {
		json_sax_acceptor<json> acceptor;
		return sax_parse(input,&acceptor);
	}

private:
	static void dump_hex16(string_t& out,unsigned long cp) {
		static const char digits[]="0123456789abcdef";
		out.push_back('\\');
		out.push_back('u');
		out.push_back(digits[(cp>>12)&0xF]);
		out.push_back(digits[(cp>>8)&0xF]);
		out.push_back(digits[(cp>>4)&0xF]);
		out.push_back(digits[cp&0xF]);
	}
	static void dump_escaped(string_t& out,const string_t& s,bool ensure_ascii) {
		const unsigned char* first=reinterpret_cast<const unsigned char*>(s.data());
		const unsigned char* const last=first+s.size();
		while (first<last) {
			const unsigned char c=*first;
			switch (c) {
				case '"': out.push_back('\\');out.push_back('"');first++;continue;
				case '\\': out.push_back('\\');out.push_back('\\');first++;continue;
				case '\b': out.push_back('\\');out.push_back('b');first++;continue;
				case '\f': out.push_back('\\');out.push_back('f');first++;continue;
				case '\n': out.push_back('\\');out.push_back('n');first++;continue;
				case '\r': out.push_back('\\');out.push_back('r');first++;continue;
				case '\t': out.push_back('\\');out.push_back('t');first++;continue;
				default: break;
			}
			if (c<0x20) {
				dump_hex16(out,c);
				first++;
				continue;
			}
			if (c<0x80) {
				out.push_back(static_cast<typename string_t::value_type>(c));
				first++;
				continue;
			}
			unsigned long cp=0;
			std::size_t extra=0;
			if ((c&0xE0)==0xC0) {
				cp=c&0x1F;
				extra=1;
			} else if ((c&0xF0)==0xE0) {
				cp=c&0x0F;
				extra=2;
			} else if ((c&0xF8)==0xF0) {
				cp=c&0x07;
				extra=3;
			} else throw std::invalid_argument("Invalid UTF-8 byte at index "+std::to_string(first-reinterpret_cast<const unsigned char*>(s.data())));
			if (static_cast<std::size_t>(last-first)<extra+1) throw std::invalid_argument("Truncated UTF-8 sequence");
			for (std::size_t i=1;i<=extra;i++) {
				if ((first[i]&0xC0)!=0x80) throw std::invalid_argument("Invalid UTF-8 continuation byte");
				cp=(cp<<6)|(first[i]&0x3F);
			}
			if (!ensure_ascii) {
				for (std::size_t i=0;i<=extra;i++) out.push_back(static_cast<typename string_t::value_type>(first[i]));
			} else if (cp<0x10000) {
				dump_hex16(out,cp);
			} else {
				cp-=0x10000;
				dump_hex16(out,0xD800+(cp>>10));
				dump_hex16(out,0xDC00+(cp&0x3FF));
			}
			first+=extra+1;
		}
	}
	static void dump_integer(string_t& out,int_t value) {
		const std::string text=std::to_string(static_cast<long long>(value));
		out.append(text.begin(),text.end());
	}
	static void dump_floating(string_t& out,float_t value) {
		if (std::isnan(value) || std::isinf(value)) {
			out.append({'n','u','l','l'});
			return;
		}
		char buffer[64];
		int length=std::snprintf(buffer,sizeof(buffer),"%.15g",static_cast<double>(value));
		if (std::strtod(buffer,nullptr)!=static_cast<double>(value)) length=std::snprintf(buffer,sizeof(buffer),"%.17g",static_cast<double>(value));
		bool needs_dot=true;
		for (int i=0;i<length;i++) {
			if (buffer[i]=='.' || buffer[i]=='e' || buffer[i]=='E') {
				needs_dot=false;
				break;
			}
		}
		out.append(buffer,buffer+length);
		if (needs_dot) {
			out.push_back('.');
			out.push_back('0');
		}
	}
	static void dump_indent(string_t& out,std::size_t count,typename string_t::value_type indent_char) {
		for (std::size_t i=0;i<count;i++) out.push_back(indent_char);
	}
	static void dump_internal(const base_t& node,string_t& out,int indent_step,typename string_t::value_type indent_char,bool ensure_ascii,std::size_t current_indent) {
		const bool pretty=indent_step>0;
		const std::size_t child_indent=current_indent+(pretty?static_cast<std::size_t>(indent_step):0);
		switch (node.type()) {
			case structure::DDT_NULL: {
				out.append({'n','u','l','l'});
				break;
			}
			case structure::DDT_BOOL: {
				if (*node.template get_ptr<const boolean_t*>()) out.append({'t','r','u','e'});
				else out.append({'f','a','l','s','e'});
				break;
			}
			case structure::DDT_INT: {
				dump_integer(out,*node.template get_ptr<const int_t*>());
				break;
			}
			case structure::DDT_FLOAT: {
				dump_floating(out,*node.template get_ptr<const float_t*>());
				break;
			}
			case structure::DDT_STRING: {
				out.push_back('"');
				dump_escaped(out,*node.template get_ptr<const string_t*>(),ensure_ascii);
				out.push_back('"');
				break;
			}
			case structure::DDT_ARRAY: {
				if (node.empty()) {
					out.push_back('[');
					out.push_back(']');
					break;
				}
				out.push_back('[');
				if (pretty) out.push_back('\n');
				for (auto it=node.cbegin();it!=node.cend();) {
					if (pretty) dump_indent(out,child_indent,indent_char);
					dump_internal(*it,out,indent_step,indent_char,ensure_ascii,child_indent);
					it++;
					if (it!=node.cend()) out.push_back(',');
					if (pretty) out.push_back('\n');
				}
				if (pretty) dump_indent(out,current_indent,indent_char);
				out.push_back(']');
				break;
			}
			case structure::DDT_OBJECT: {
				if (node.empty()) {
					out.push_back('{');
					out.push_back('}');
					break;
				}
				out.push_back('{');
				if (pretty) out.push_back('\n');
				for (auto it=node.cbegin();it!=node.cend();) {
					if (pretty) dump_indent(out,child_indent,indent_char);
					out.push_back('"');
					dump_escaped(out,it.key(),ensure_ascii);
					out.push_back('"');
					out.push_back(':');
					if (pretty) out.push_back(' ');
					dump_internal(*it,out,indent_step,indent_char,ensure_ascii,child_indent);
					it++;
					if (it!=node.cend()) out.push_back(',');
					if (pretty) out.push_back('\n');
				}
				if (pretty) dump_indent(out,current_indent,indent_char);
				out.push_back('}');
				break;
			}
			default: break;
		}
	}
 
public:
	virtual string_t dump(int indent=-1,typename string_t::value_type indent_char=' ',bool ensure_ascii=false) const {
		string_t result;
		dump_internal(*this,result,indent,indent_char,ensure_ascii,0);
		return result;
	}

	friend std::ostream& operator <<(std::ostream& os,const json& value) {
		const int indent_step=static_cast<int>(os.width());
		os.width(0);
		const string_t text=value.dump(indent_step>0?indent_step:-1,static_cast<typename string_t::value_type>(os.fill()));
		os.write(reinterpret_cast<const char*>(text.data()),static_cast<std::streamsize>(text.size()));
		return os;
	}
	friend std::istream& operator >>(std::istream& is,json& value) {
		std::string content((std::istreambuf_iterator<char>(is)),std::istreambuf_iterator<char>());
		value=parse(content);
		return is;
	}
};
 
_STDEX_DOM_TPL_DEFAULT_DECLARATION
inline typename json<_Int,_Float,_Boolean,_String,_Array,_Object,_Allocator>::string_t to_string(const json<_Int,_Float,_Boolean,_String,_Array,_Object,_Allocator>& value) {
	return value.dump();
}
 
}
 
_STDEX_DOM_TPL_DEFAULT_DECLARATION
using json_t=basic_json::json<_Int,_Float,_Boolean,_String,_Array,_Object,_Allocator>;
using json=json_t<>;
using basic_json::json_sax;
using basic_json::json_sax_dom_builder;
using basic_json::json_sax_acceptor;
using basic_json::to_string;
 
inline namespace literals {
 
inline json_t<> operator ""_json(const char* s,std::size_t n) {
	return json_t<>::parse(std::string_view(s,n));
}

inline structure::dom_pointer<std::string> operator ""_json_pointer(const char* s,std::size_t n) {
	return structure::dom_pointer<std::string>(std::string(s,n));
}
 
}
 
}
 
}
 
#endif