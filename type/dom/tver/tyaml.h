//Last Modified At 2026/06/12
//@Version 1.0.0.0
#ifndef _STDEX_TYPE_DOM_YAML_H_
#define _STDEX_TYPE_DOM_YAML_H_ 1

#include <cerrno>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <istream>
#include <limits>
#include <map>
#include <mutex>
#include <ostream>
#include <regex>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "../structure/dom.h"//At Least 1.0
#include "../syntax/parser.h"//At Least 3.4

namespace stdex {

namespace type {

namespace basic_yaml {

//yaml的数据模型(null/bool/int/float/string/sequence/mapping)与dom基础七型一一对应,
//因此不派生kind、不携带载荷子类。具体情况具体分析:json/xml是文本记法故有SAX,
//bson/cbor/msgpack是二进制记法故有binary_t;yaml属前者,提供SAX,不提供binary_t。
//yaml独有内容:多文档流、%YAML/%TAG指令、锚点/别名、标签、块/流双风格、块标量。

//块结构上下文(缩进)在词法层由缩进栈消解并合成结构终结符(BSEQ/BMAP的START与END、
//DOC_START/DOC_END、缺省值EMPTY),从而文法保持上下文无关,可直接交给SLR(1)分析器。
enum yaml_symbol : int {
	YS_EPSILON,
	YS_EOF,
	YS_DOC_START,
	YS_DOC_END,
	YS_DIRECTIVE,
	YS_BSEQ_START,
	YS_BSEQ_END,
	YS_BMAP_START,
	YS_BMAP_END,
	YS_ENTRY,
	YS_COLON,
	YS_COMMA,
	YS_FSEQ_START,
	YS_FSEQ_END,
	YS_FMAP_START,
	YS_FMAP_END,
	YS_SCALAR,
	YS_EMPTY,
	YS_ANCHOR,
	YS_ALIAS,
	YS_TAG,
	YS_START,
	YS_STREAM,
	YS_DOCUMENT,
	YS_DIRECTIVE_SEQ,
	YS_NODE,
	YS_BSEQ_ITEMS,
	YS_BMAP_ITEMS,
	YS_BMAP_PAIR,
	YS_FLOW_ITEMS,
	YS_FLOW_PAIRS,
	YS_FLOW_PAIR,
};

enum yaml_production : int {
	YP_START,
	YP_START_EMPTY,
	YP_STREAM_FIRST,
	YP_STREAM_APPEND,
	YP_DOCUMENT_PLAIN,
	YP_DOCUMENT_BLANK,
	YP_DOCUMENT_DIRECTIVE,
	YP_DOCUMENT_DIRECTIVE_BLANK,
	YP_DIRECTIVES_FIRST,
	YP_DIRECTIVES_APPEND,
	YP_NODE_SCALAR,
	YP_NODE_EMPTY,
	YP_NODE_ALIAS,
	YP_NODE_ANCHOR,
	YP_NODE_TAG,
	YP_NODE_BSEQ,
	YP_NODE_BMAP,
	YP_NODE_FSEQ_EMPTY,
	YP_NODE_FSEQ,
	YP_NODE_FMAP_EMPTY,
	YP_NODE_FMAP,
	YP_BSEQ_FIRST,
	YP_BSEQ_APPEND,
	YP_BMAP_FIRST,
	YP_BMAP_APPEND,
	YP_BMAP_PAIR,
	YP_FLOW_ITEMS_FIRST,
	YP_FLOW_ITEMS_APPEND,
	YP_FLOW_PAIRS_FIRST,
	YP_FLOW_PAIRS_APPEND,
	YP_FLOW_PAIR,
	YP_FLOW_PAIR_KEY,
};

enum yaml_scalar_style : int {
	YSS_PLAIN,
	YSS_SINGLE_QUOTED,
	YSS_DOUBLE_QUOTED,
	YSS_LITERAL,
	YSS_FOLDED,
};

template <typename _String>
struct basic_yaml_document_info {
	_String version{};
	std::vector<std::pair<_String,_String>> tag_directives{};

	bool has_version() const noexcept {
		return !version.empty();
	}
	bool has_tag_directives() const noexcept {
		return !tag_directives.empty();
	}
};

template <typename _Yaml>
struct yaml_sax {
	using int_t=typename _Yaml::int_t;
	using float_t=typename _Yaml::float_t;
	using boolean_t=typename _Yaml::boolean_t;
	using string_t=typename _Yaml::string_t;

	virtual bool start_document()=0;
	virtual bool end_document()=0;
	virtual bool directive(string_t& name,string_t& value)=0;
	virtual bool null()=0;
	virtual bool boolean(boolean_t value)=0;
	virtual bool number_integer(int_t value)=0;
	virtual bool number_float(float_t value,const string_t& raw)=0;
	virtual bool string(string_t& value,yaml_scalar_style style)=0;
	virtual bool start_mapping(std::size_t cnt)=0;
	virtual bool key(string_t& value)=0;
	virtual bool end_mapping()=0;
	virtual bool start_sequence(std::size_t cnt)=0;
	virtual bool end_sequence()=0;
	virtual bool anchor(string_t& name)=0;
	virtual bool alias(string_t& name)=0;
	virtual bool tag(string_t& text)=0;
	virtual bool parse_error(std::size_t position,const std::string& last_token,const std::string& message)=0;
	virtual ~yaml_sax()=default;
};

//锚点/别名在建树侧消解:别名展开为锚定子树的拷贝(dom是树而非图,环状引用不可表示,
//指向未完成祖先的别名将因查无锚点而报错);锚点作用域为单个文档。
//标签的类型强制已在listener侧完成,建树侧忽略tag事件。
template <typename _Yaml>
class yaml_sax_dom_builder : public yaml_sax<_Yaml> {
public:
	using int_t=typename _Yaml::int_t;
	using float_t=typename _Yaml::float_t;
	using boolean_t=typename _Yaml::boolean_t;
	using string_t=typename _Yaml::string_t;
	using document_info_t=typename _Yaml::document_info_t;

private:
	std::vector<_Yaml>& documents_;
	document_info_t* info_=nullptr;
	std::vector<_Yaml*> ref_stack_;
	string_t key_;
	std::map<string_t,_Yaml> anchors_;
	std::vector<std::pair<string_t,std::size_t>> pending_anchors_;
	bool in_document_=false;
	bool document_value_seen_=false;
	bool errored_=false;
	std::size_t error_position_=0;
	std::string error_message_;

	static bool text_equals(const string_t& text,const char* literal) noexcept {
		std::size_t i=0;
		for (;literal[i];i++) {
			if (i>=text.size() || text[i]!=static_cast<typename string_t::value_type>(literal[i])) return false;
		}
		return i==text.size();
	}

	template <typename _Vp>
	_Yaml* handle_value(_Vp&& value) {
		if (ref_stack_.empty()) {
			documents_.push_back(_Yaml(std::forward<_Vp>(value)));
			document_value_seen_=true;
			return &documents_.back();
		}
		_Yaml* parent=ref_stack_.back();
		if (parent->is_array()) {
			parent->push_back(std::forward<_Vp>(value));
			return static_cast<_Yaml*>(&parent->back());
		}
		auto& slot=(*parent)[std::move(key_)];
		slot=std::forward<_Vp>(value);
		return static_cast<_Yaml*>(&slot);
	}
	void bind_anchors(std::size_t depth,_Yaml* node) {
		while (!pending_anchors_.empty() && pending_anchors_.back().second==depth) {
			anchors_[std::move(pending_anchors_.back().first)]=*node;
			pending_anchors_.pop_back();
		}
	}

public:
	explicit yaml_sax_dom_builder(std::vector<_Yaml>& documents,document_info_t* info=nullptr) : documents_(documents) , info_(info) { }
	bool start_document() override {
		in_document_=true;
		document_value_seen_=false;
		return true;
	}
	bool end_document() override {
		if (!document_value_seen_) documents_.push_back(_Yaml(nullptr));
		in_document_=false;
		document_value_seen_=false;
		anchors_.clear();
		pending_anchors_.clear();
		return true;
	}
	bool directive(string_t& name,string_t& value) override {
		if (!info_) return true;
		if (text_equals(name,"YAML")) info_->version=std::move(value);
		else if (text_equals(name,"TAG")) {
			std::size_t split=0;
			while (split<value.size() && value[split]!=' ' && value[split]!='\t') split++;
			string_t handle(value.begin(),value.begin()+split);
			while (split<value.size() && (value[split]==' ' || value[split]=='\t')) split++;
			string_t prefix(value.begin()+split,value.end());
			info_->tag_directives.emplace_back(std::move(handle),std::move(prefix));
		}
		return true;
	}
	bool null() override {
		_Yaml* node=handle_value(nullptr);
		bind_anchors(ref_stack_.size(),node);
		return true;
	}
	bool boolean(boolean_t value) override {
		_Yaml* node=handle_value(value);
		bind_anchors(ref_stack_.size(),node);
		return true;
	}
	bool number_integer(int_t value) override {
		_Yaml* node=handle_value(value);
		bind_anchors(ref_stack_.size(),node);
		return true;
	}
	bool number_float(float_t value,const string_t& raw) override {
		static_cast<void>(raw);
		_Yaml* node=handle_value(value);
		bind_anchors(ref_stack_.size(),node);
		return true;
	}
	bool string(string_t& value,yaml_scalar_style style) override {
		static_cast<void>(style);
		_Yaml* node=handle_value(std::move(value));
		bind_anchors(ref_stack_.size(),node);
		return true;
	}
	bool start_mapping(std::size_t cnt) override {
		static_cast<void>(cnt);
		ref_stack_.push_back(handle_value(_Yaml(structure::DDT_OBJECT)));
		return true;
	}
	bool key(string_t& value) override {
		key_=std::move(value);
		return true;
	}
	bool end_mapping() override {
		if (ref_stack_.empty()) return true;
		_Yaml* node=ref_stack_.back();
		ref_stack_.pop_back();
		bind_anchors(ref_stack_.size(),node);
		return true;
	}
	bool start_sequence(std::size_t cnt) override {
		static_cast<void>(cnt);
		ref_stack_.push_back(handle_value(_Yaml(structure::DDT_ARRAY)));
		return true;
	}
	bool end_sequence() override {
		if (ref_stack_.empty()) return true;
		_Yaml* node=ref_stack_.back();
		ref_stack_.pop_back();
		bind_anchors(ref_stack_.size(),node);
		return true;
	}
	bool anchor(string_t& name) override {
		pending_anchors_.emplace_back(std::move(name),ref_stack_.size());
		return true;
	}
	bool alias(string_t& name) override {
		auto it=anchors_.find(name);
		if (it==anchors_.end()) {
			errored_=true;
			error_message_="unknown alias '"+std::string(name.begin(),name.end())+"'";
			return false;
		}
		handle_value(_Yaml(it->second));
		return true;
	}
	bool tag(string_t& text) override {
		static_cast<void>(text);
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
		return !errored_ && !in_document_ && ref_stack_.empty();
	}
	std::size_t error_position() const noexcept {
		return error_position_;
	}
	const std::string& error_message() const noexcept {
		return error_message_;
	}
};

template <typename _Yaml>
class yaml_sax_acceptor : public yaml_sax<_Yaml> {
public:
	using int_t=typename _Yaml::int_t;
	using float_t=typename _Yaml::float_t;
	using boolean_t=typename _Yaml::boolean_t;
	using string_t=typename _Yaml::string_t;

	bool start_document() override { return true; }
	bool end_document() override { return true; }
	bool directive(string_t&,string_t&) override { return true; }
	bool null() override { return true; }
	bool boolean(boolean_t) override { return true; }
	bool number_integer(int_t) override { return true; }
	bool number_float(float_t,const string_t&) override { return true; }
	bool string(string_t&,yaml_scalar_style) override { return true; }
	bool start_mapping(std::size_t) override { return true; }
	bool key(string_t&) override { return true; }
	bool end_mapping() override { return true; }
	bool start_sequence(std::size_t) override { return true; }
	bool end_sequence() override { return true; }
	bool anchor(string_t&) override { return true; }
	bool alias(string_t&) override { return true; }
	bool tag(string_t&) override { return true; }
	bool parse_error(std::size_t,const std::string&,const std::string&) override { return false; }
};

_STDEX_DOM_TPL_DECLARATION
class yaml : public structure::_STDEX_DOM_DEF {
public:
	using base_t=structure::_STDEX_DOM_DEF;
	using int_t=typename base_t::int_t;
	using float_t=typename base_t::float_t;
	using boolean_t=typename base_t::boolean_t;
	using string_t=typename base_t::string_t;
	using array_t=typename base_t::array_t;
	using object_t=typename base_t::object_t;
	using size_type=typename base_t::size_type;
	using sax_t=yaml_sax<yaml>;
	using document_info_t=basic_yaml_document_info<string_t>;

	static_assert(sizeof(typename string_t::value_type)==1,"yaml serializer assumes a byte-oriented (UTF-8) string_t.");

	using base_t::base_t;
	using base_t::operator =;

	yaml()=default;
	~yaml() override=default;

	yaml(const yaml&)=default;
	yaml(yaml&&) noexcept=default;

	yaml& operator =(const yaml&)=default;
	yaml& operator =(yaml&&)=default;

	yaml(const base_t& other) : base_t(other) { }
	yaml(base_t&& other) noexcept : base_t(std::move(other)) { }

	static yaml sequence(typename base_t::initializer_list_t init_list={}) {
		return yaml(base_t::array(init_list));
	}
	static yaml mapping(typename base_t::initializer_list_t init_list={}) {
		return yaml(base_t::object(init_list));
	}
	static yaml array(typename base_t::initializer_list_t init_list={}) {
		return yaml(base_t::array(init_list));
	}
	static yaml object(typename base_t::initializer_list_t init_list={}) {
		return yaml(base_t::object(init_list));
	}

private:
	struct yaml_token {
		yaml_symbol symbol;
		std::size_t position;
		string_t text{};
		string_t aux{};
		int extra=0;
	};

	//YAML 1.2 Core Schema的标量判定(plain风格才参与类型解析;引用/块标量恒为字符串)。
	static const std::regex& null_regex() {
		static const std::regex result(R"(~|null|Null|NULL)",std::regex::optimize);
		return result;
	}
	static const std::regex& true_regex() {
		static const std::regex result(R"(true|True|TRUE)",std::regex::optimize);
		return result;
	}
	static const std::regex& false_regex() {
		static const std::regex result(R"(false|False|FALSE)",std::regex::optimize);
		return result;
	}
	static const std::regex& integer_regex() {
		static const std::regex result(R"([-+]?[0-9]+|0x[0-9A-Fa-f]+|0o[0-7]+)",std::regex::optimize);
		return result;
	}
	static const std::regex& floating_regex() {
		static const std::regex result(R"([-+]?(?:\.[0-9]+|[0-9]+(?:\.[0-9]*)?)(?:[eE][-+]?[0-9]+)?|[-+]?\.(?:inf|Inf|INF)|\.(?:nan|NaN|NAN))",std::regex::optimize);
		return result;
	}

	static bool regex_match_text(const string_t& text,const std::regex& expression) {
		if (text.empty()) return false;
		const char* first=reinterpret_cast<const char*>(text.data());
		return std::regex_match(first,first+text.size(),expression);
	}
	static bool plain_is_null(const string_t& text) {
		return text.empty() || regex_match_text(text,null_regex());
	}
	static int plain_boolean(const string_t& text) {
		if (regex_match_text(text,true_regex())) return 1;
		if (regex_match_text(text,false_regex())) return 0;
		return -1;
	}
	static bool plain_integer(const string_t& text,int_t& out) {
		if (!regex_match_text(text,integer_regex())) return false;
		const std::string buffer(text.begin(),text.end());
		errno=0;
		if (buffer.size()>2 && buffer[0]=='0' && (buffer[1]=='x' || buffer[1]=='o')) {
			const unsigned long long value=std::strtoull(buffer.c_str()+2,nullptr,buffer[1]=='x'?16:8);
			if (errno==ERANGE || value>static_cast<unsigned long long>((std::numeric_limits<int_t>::max)())) return false;
			out=static_cast<int_t>(value);
			return true;
		}
		const long long value=std::strtoll(buffer.c_str(),nullptr,10);
		if (errno==ERANGE || value<static_cast<long long>((std::numeric_limits<int_t>::min)()) || value>static_cast<long long>((std::numeric_limits<int_t>::max)())) return false;
		out=static_cast<int_t>(value);
		return true;
	}
	static bool plain_floating(const string_t& text,float_t& out) {
		if (!regex_match_text(text,floating_regex())) return false;
		std::string buffer(text.begin(),text.end());
		std::string body=buffer;
		float_t sign=1;
		if (!body.empty() && (body[0]=='+' || body[0]=='-')) {
			if (body[0]=='-') sign=-1;
			body.erase(0,1);
		}
		if (body==".inf" || body==".Inf" || body==".INF") {
			out=sign*std::numeric_limits<float_t>::infinity();
			return true;
		}
		if (body==".nan" || body==".NaN" || body==".NAN") {
			out=std::numeric_limits<float_t>::quiet_NaN();
			return true;
		}
		out=static_cast<float_t>(std::strtod(buffer.c_str(),nullptr));
		return true;
	}
	//识别标准标签的类型强制:0无/未知,1 str,2 int,3 float,4 bool,5 null。
	//"!"非特定标签按规范对标量解析为字符串;%TAG前缀展开不在识别范围(原文存info)。
	static int tag_class(const string_t& tag) {
		const std::string text(tag.begin(),tag.end());
		if (text=="!") return 1;
		std::string suffix;
		if (text.size()>2 && text[0]=='!' && text[1]=='!') suffix=text.substr(2);
		else if (text.rfind("!<tag:yaml.org,2002:",0)==0 && text.size()>21 && text.back()=='>') suffix=text.substr(20,text.size()-21);
		else return 0;
		if (suffix=="str") return 1;
		if (suffix=="int") return 2;
		if (suffix=="float") return 3;
		if (suffix=="bool") return 4;
		if (suffix=="null") return 5;
		return 0;
	}

	static void append_codepoint(string_t& out,unsigned long cp) {
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

	struct block_frame {
		std::size_t indent;
		bool sequence;
	};

	//双层词法:行驱动的块上下文(缩进栈合成START/END/ENTRY/EMPTY/DOC_*)+字符驱动的流上下文。
	//挂起值协议:COLON/ENTRY/ANCHOR/TAG之后置expect_node_;换行后更深缩进=值内容,
	//否则先补发EMPTY(空值)再做退栈;"序列与所属映射同缩进"的零缩进序列由pending_in_map_放行。
	struct tokenizer {
		const char* base_;
		const char* first_;
		const char* last_;
		const char* line_start_=nullptr;
		const char* line_end_=nullptr;
		const char* next_position_=nullptr;
		std::vector<yaml_token>& tokens_;
		std::vector<block_frame> blocks_{};
		bool document_open_=false;
		bool directives_pending_=false;
		bool expect_node_=false;
		std::size_t pending_indent_=0;
		bool pending_in_map_=false;
		bool colon_on_line_=false;
		bool failed_=false;
		std::size_t error_position_=0;
		std::string error_message_{};

		tokenizer(std::string_view input,std::vector<yaml_token>& tokens) : base_(input.data()) , first_(input.data()) , last_(input.data()+input.size()) , tokens_(tokens) { }

		static bool is_space(char c) noexcept {
			return c==' ' || c=='\t';
		}
		static bool is_break(char c) noexcept {
			return c=='\n' || c=='\r';
		}
		static bool is_flow_indicator(char c) noexcept {
			return c==',' || c=='[' || c==']' || c=='{' || c=='}';
		}
		std::size_t position(const char* p) const noexcept {
			return static_cast<std::size_t>(p-base_);
		}
		bool fail(const char* p,std::string message) {
			failed_=true;
			error_position_=position(p);
			error_message_=std::move(message);
			return false;
		}
		void emit(yaml_symbol symbol,const char* p) {
			yaml_token token;
			token.symbol=symbol;
			token.position=position(p);
			tokens_.push_back(std::move(token));
		}
		void emit_scalar(const char* p,string_t text,int style) {
			yaml_token token;
			token.symbol=YS_SCALAR;
			token.position=position(p);
			token.text=std::move(text);
			token.extra=style;
			tokens_.push_back(std::move(token));
			expect_node_=false;
		}
		const char* after_line(const char* line_end) const noexcept {
			if (line_end>=last_) return last_;
			if (*line_end=='\r' && line_end+1<last_ && line_end[1]=='\n') return line_end+2;
			return line_end+1;
		}

		bool run() {
			if (last_-first_>=3 && static_cast<unsigned char>(first_[0])==0xEF && static_cast<unsigned char>(first_[1])==0xBB && static_cast<unsigned char>(first_[2])==0xBF) first_+=3;
			while (first_<last_) {
				if (!next_line()) return false;
			}
			if (expect_node_) {
				emit(YS_EMPTY,last_);
				expect_node_=false;
			}
			while (!blocks_.empty()) {
				emit(blocks_.back().sequence?YS_BSEQ_END:YS_BMAP_END,last_);
				blocks_.pop_back();
			}
			if (directives_pending_) return fail(last_,"directives must be followed by '---'");
			if (document_open_) {
				emit(YS_DOC_END,last_);
				document_open_=false;
			}
			return true;
		}

		void close_document(const char* p) {
			if (expect_node_) {
				emit(YS_EMPTY,p);
				expect_node_=false;
			}
			while (!blocks_.empty()) {
				emit(blocks_.back().sequence?YS_BSEQ_END:YS_BMAP_END,p);
				blocks_.pop_back();
			}
			if (document_open_) {
				emit(YS_DOC_END,p);
				document_open_=false;
			}
			pending_indent_=0;
			pending_in_map_=false;
		}

		bool next_line() {
			line_start_=first_;
			line_end_=first_;
			while (line_end_<last_ && !is_break(*line_end_)) line_end_++;
			next_position_=after_line(line_end_);
			const char* p=line_start_;
			std::size_t indent=0;
			while (p<line_end_ && *p==' ') {
				p++;
				indent++;
			}
			const char* probe=p;
			while (probe<line_end_ && is_space(*probe)) probe++;
			if (probe==line_end_ || *probe=='#') {
				first_=next_position_;
				return true;
			}
			if (*p=='\t') return fail(p,"tab characters are not allowed in indentation");
			if (*p=='%') {
				if (indent!=0) return fail(p,"directives must start at the beginning of a line");
				if (document_open_) return fail(p,"directives are only allowed before '---'");
				if (!scan_directive(p)) return false;
				first_=next_position_;
				return true;
			}
			if (indent==0 && line_end_-p>=3 && p[0]=='-' && p[1]=='-' && p[2]=='-' && (p+3==line_end_ || is_space(p[3]))) {
				close_document(p);
				emit(YS_DOC_START,p);
				document_open_=true;
				directives_pending_=false;
				colon_on_line_=false;
				const char* q=p+3;
				while (q<line_end_ && is_space(*q)) q++;
				if (q<line_end_ && *q!='#') {
					if (!scan_line(q)) return false;
				}
				first_=next_position_;
				return true;
			}
			if (indent==0 && line_end_-p>=3 && p[0]=='.' && p[1]=='.' && p[2]=='.' && (p+3==line_end_ || is_space(p[3]))) {
				if (!document_open_) return fail(p,"unexpected '...' outside a document");
				close_document(p);
				const char* q=p+3;
				while (q<line_end_ && is_space(*q)) q++;
				if (q<line_end_ && *q!='#') return fail(q,"unexpected content after '...'");
				first_=next_position_;
				return true;
			}
			if (!document_open_) {
				if (directives_pending_) return fail(p,"directives must be followed by '---'");
				emit(YS_DOC_START,p);
				document_open_=true;
			}
			colon_on_line_=false;
			const bool line_is_entry=(*p=='-' && (p+1==line_end_ || is_space(p[1])));
			if (expect_node_) {
				if (indent>pending_indent_) {
					//更深缩进:挂起值的内容,保持期望,不退栈。
				} else if (indent==pending_indent_ && line_is_entry && pending_in_map_) {
					//零缩进序列:块序列可与所属映射键同列充当其值。
				} else {
					emit(YS_EMPTY,p);
					expect_node_=false;
				}
			}
			if (!expect_node_) {
				while (!blocks_.empty() && (blocks_.back().indent>indent || (blocks_.back().indent==indent && blocks_.back().sequence && !line_is_entry))) {
					emit(blocks_.back().sequence?YS_BSEQ_END:YS_BMAP_END,p);
					blocks_.pop_back();
				}
			}
			if (!scan_line(p)) return false;
			first_=next_position_;
			return true;
		}

		bool scan_directive(const char* p) {
			const char* q=p+1;
			const char* name_first=q;
			while (q<line_end_ && !is_space(*q)) q++;
			if (q==name_first) return fail(p,"empty directive name");
			yaml_token token;
			token.symbol=YS_DIRECTIVE;
			token.position=position(p);
			token.text=string_t(name_first,q);
			while (q<line_end_ && is_space(*q)) q++;
			const char* value_first=q;
			const char* value_last=line_end_;
			for (const char* r=value_first;r<line_end_;r++) {
				if (*r=='#' && (r==value_first || is_space(r[-1]))) {
					value_last=r;
					break;
				}
			}
			while (value_last>value_first && is_space(value_last[-1])) value_last--;
			token.aux=string_t(value_first,value_last);
			tokens_.push_back(std::move(token));
			directives_pending_=true;
			return true;
		}

		bool begin_mapping_pair(const char* key_position,std::size_t key_column,string_t key_text,int key_style,const char* colon_position) {
			if (colon_on_line_) return fail(colon_position,"mapping values are not allowed in this context");
			if (blocks_.empty() || blocks_.back().sequence || blocks_.back().indent!=key_column) {
				blocks_.push_back(block_frame{key_column,false});
				emit(YS_BMAP_START,key_position);
			}
			yaml_token token;
			token.symbol=YS_SCALAR;
			token.position=position(key_position);
			token.text=std::move(key_text);
			token.extra=key_style;
			tokens_.push_back(std::move(token));
			emit(YS_COLON,colon_position);
			colon_on_line_=true;
			expect_node_=true;
			pending_indent_=key_column;
			pending_in_map_=true;
			return true;
		}

		bool read_hex_digits(const char* r,int count,unsigned long& cp) const {
			if (r+count>line_end_) return false;
			cp=0;
			for (int i=0;i<count;i++) {
				const char c=r[i];
				cp<<=4;
				if (c>='0' && c<='9') cp|=static_cast<unsigned long>(c-'0');
				else if (c>='a' && c<='f') cp|=static_cast<unsigned long>(c-'a'+10);
				else if (c>='A' && c<='F') cp|=static_cast<unsigned long>(c-'A'+10);
				else return false;
			}
			return true;
		}

		bool scan_quoted(const char*& p,string_t& out) {
			const char quote=*p;
			const char* q=p+1;
			if (quote=='\'') {
				while (q<line_end_) {
					if (*q=='\'') {
						if (q+1<line_end_ && q[1]=='\'') {
							out.push_back('\'');
							q+=2;
							continue;
						}
						p=q+1;
						return true;
					}
					out.push_back(*q++);
				}
				return fail(p,"unterminated single-quoted scalar (multi-line quoted scalars are not supported)");
			}
			while (q<line_end_) {
				const char c=*q;
				if (c=='"') {
					p=q+1;
					return true;
				}
				if (c!='\\') {
					out.push_back(c);
					q++;
					continue;
				}
				q++;
				if (q==line_end_) return fail(q,"unterminated escape sequence");
				unsigned long cp=0;
				switch (*q) {
					case '0': out.push_back(static_cast<typename string_t::value_type>('\0'));break;
					case 'a': out.push_back('\a');break;
					case 'b': out.push_back('\b');break;
					case 't': out.push_back('\t');break;
					case '\t': out.push_back('\t');break;
					case 'n': out.push_back('\n');break;
					case 'v': out.push_back('\v');break;
					case 'f': out.push_back('\f');break;
					case 'r': out.push_back('\r');break;
					case 'e': out.push_back(static_cast<typename string_t::value_type>(0x1B));break;
					case ' ': out.push_back(' ');break;
					case '"': out.push_back('"');break;
					case '/': out.push_back('/');break;
					case '\\': out.push_back('\\');break;
					case 'N': append_codepoint(out,0x85);break;
					case '_': append_codepoint(out,0xA0);break;
					case 'L': append_codepoint(out,0x2028);break;
					case 'P': append_codepoint(out,0x2029);break;
					case 'x': {
						if (!read_hex_digits(q+1,2,cp)) return fail(q,"invalid hexadecimal escape");
						append_codepoint(out,cp);
						q+=2;
						break;
					}
					case 'u': {
						if (!read_hex_digits(q+1,4,cp)) return fail(q,"invalid hexadecimal escape");
						append_codepoint(out,cp);
						q+=4;
						break;
					}
					case 'U': {
						if (!read_hex_digits(q+1,8,cp)) return fail(q,"invalid hexadecimal escape");
						append_codepoint(out,cp);
						q+=8;
						break;
					}
					default: return fail(q,"invalid escape sequence in double-quoted scalar");
				}
				q++;
			}
			return fail(p,"unterminated double-quoted scalar (multi-line quoted scalars are not supported)");
		}

		//块标量:|字面/＞折叠+chomping(-strip/缺省clip/+keep)+可选显式缩进指示符。
		//内容缩进自动探测取首个非空行;按规范折叠规则处理更深缩进行与空行。
		bool scan_block_scalar(const char* p) {
			const char style_char=*p;
			const char* q=p+1;
			int chomp=0;
			int explicit_indent=0;
			while (q<line_end_ && (*q=='+' || *q=='-' || (*q>='1' && *q<='9'))) {
				if (*q=='+') {
					if (chomp) return fail(q,"duplicate chomping indicator");
					chomp=2;
				} else if (*q=='-') {
					if (chomp) return fail(q,"duplicate chomping indicator");
					chomp=1;
				} else {
					if (explicit_indent) return fail(q,"duplicate indentation indicator");
					explicit_indent=*q-'0';
				}
				q++;
			}
			while (q<line_end_ && is_space(*q)) q++;
			if (q<line_end_ && *q!='#') return fail(q,"unexpected characters after block scalar header");
			const std::size_t parent=expect_node_?pending_indent_:(blocks_.empty()?0:blocks_.back().indent);
			bool indent_known=(explicit_indent>0);
			std::size_t content_indent=indent_known?parent+static_cast<std::size_t>(explicit_indent):0;
			std::vector<string_t> lines;
			const char* cursor=next_position_;
			while (cursor<last_) {
				const char* ls=cursor;
				const char* le=ls;
				while (le<last_ && !is_break(*le)) le++;
				const char* r=ls;
				std::size_t line_indent=0;
				while (r<le && *r==' ') {
					r++;
					line_indent++;
				}
				const char* t=ls;
				while (t<le && is_space(*t)) t++;
				const bool blank=(t==le);
				if (!blank) {
					if (line_indent==0 && le-r>=3 && ((r[0]=='-' && r[1]=='-' && r[2]=='-') || (r[0]=='.' && r[1]=='.' && r[2]=='.')) && (r+3==le || is_space(r[3]))) break;
					if (!indent_known) {
						if (line_indent<=parent && !(blocks_.empty() && !expect_node_ && line_indent==0)) break;
						content_indent=line_indent;
						indent_known=true;
					}
					if (line_indent<content_indent) break;
				}
				if (blank) lines.push_back(string_t());
				else {
					const char* strip=ls;
					std::size_t removed=0;
					while (strip<le && *strip==' ' && removed<content_indent) {
						strip++;
						removed++;
					}
					lines.push_back(string_t(strip,le));
				}
				cursor=after_line(le);
			}
			string_t text;
			if (style_char=='|') {
				for (std::size_t i=0;i<lines.size();i++) {
					text.append(lines[i].begin(),lines[i].end());
					text.push_back('\n');
				}
			} else {
				bool previous_empty=true;
				bool previous_more_indented=false;
				bool first_line=true;
				for (std::size_t i=0;i<lines.size();i++) {
					const string_t& line=lines[i];
					const bool empty_line=line.empty();
					const bool more_indented=!empty_line && (line[0]==' ' || line[0]=='\t');
					if (empty_line) text.push_back('\n');
					else {
						if (first_line) { }
						else if (previous_empty) { }
						else if (more_indented || previous_more_indented) text.push_back('\n');
						else text.push_back(' ');
						text.append(line.begin(),line.end());
						first_line=false;
					}
					previous_empty=empty_line;
					previous_more_indented=more_indented;
				}
				if (!lines.empty() && !lines.back().empty()) text.push_back('\n');
			}
			if (chomp==1) {
				while (!text.empty() && text.back()=='\n') text.pop_back();
			} else if (chomp==0) {
				while (!text.empty() && text.back()=='\n') text.pop_back();
				if (!text.empty()) text.push_back('\n');
			}
			emit_scalar(p,std::move(text),style_char=='|'?YSS_LITERAL:YSS_FOLDED);
			next_position_=cursor;
			return true;
		}

		//流上下文:换行视作空白,可跨行;深度归零返回;块标量与显式键在流中非法。
		bool scan_flow(const char*& p) {
			int depth=0;
			while (true) {
				while (p<last_ && (is_space(*p) || is_break(*p))) p++;
				if (p<last_ && *p=='#' && (p==base_ || is_space(p[-1]) || is_break(p[-1]))) {
					while (p<last_ && !is_break(*p)) p++;
					continue;
				}
				if (p>=last_) return fail(p,"unterminated flow collection");
				const char c=*p;
				if (c=='[') {
					emit(YS_FSEQ_START,p);
					depth++;
					p++;
					continue;
				}
				if (c=='{') {
					emit(YS_FMAP_START,p);
					depth++;
					p++;
					continue;
				}
				if (c==']' || c=='}') {
					emit(c==']'?YS_FSEQ_END:YS_FMAP_END,p);
					depth--;
					p++;
					if (depth<=0) break;
					continue;
				}
				if (c==',') {
					emit(YS_COMMA,p);
					p++;
					continue;
				}
				if (c==':' && (p+1>=last_ || is_space(p[1]) || is_break(p[1]) || is_flow_indicator(p[1]))) {
					emit(YS_COLON,p);
					p++;
					continue;
				}
				if (c=='?' && (p+1>=last_ || is_space(p[1]) || is_break(p[1]))) return fail(p,"explicit mapping keys ('? ') are not supported");
				if (c=='|' || c=='>') return fail(p,"block scalars are not allowed in flow context");
				if (c=='&' || c=='*') {
					const char* q=p+1;
					while (q<last_ && !is_space(*q) && !is_break(*q) && !is_flow_indicator(*q)) q++;
					if (q==p+1) return fail(p,c=='&'?"empty anchor name":"empty alias name");
					yaml_token token;
					token.symbol=(c=='&')?YS_ANCHOR:YS_ALIAS;
					token.position=position(p);
					token.text=string_t(p+1,q);
					tokens_.push_back(std::move(token));
					if (c=='&') expect_node_=true;
					else expect_node_=false;
					p=q;
					continue;
				}
				if (c=='!') {
					const char* q=p+1;
					if (q<last_ && *q=='<') {
						q++;
						while (q<last_ && *q!='>' && !is_break(*q)) q++;
						if (q>=last_ || *q!='>') return fail(p,"unterminated verbatim tag");
						q++;
					} else {
						while (q<last_ && !is_space(*q) && !is_break(*q) && !is_flow_indicator(*q)) q++;
					}
					yaml_token token;
					token.symbol=YS_TAG;
					token.position=position(p);
					token.text=string_t(p,q);
					tokens_.push_back(std::move(token));
					expect_node_=true;
					p=q;
					continue;
				}
				if (c=='\'' || c=='"') {
					const char* qe=p;
					while (qe<last_ && !is_break(*qe)) qe++;
					line_end_=qe;
					string_t text;
					const char* q=p;
					if (!scan_quoted(q,text)) return false;
					emit_scalar(p,std::move(text),c=='\''?YSS_SINGLE_QUOTED:YSS_DOUBLE_QUOTED);
					p=q;
					continue;
				}
				const char* q=p;
				while (q<last_ && !is_break(*q) && !is_flow_indicator(*q)) {
					if (*q==':' && (q+1>=last_ || is_space(q[1]) || is_break(q[1]) || is_flow_indicator(q[1]))) break;
					if (*q=='#' && q>p && is_space(q[-1])) break;
					q++;
				}
				const char* text_last=q;
				while (text_last>p && is_space(text_last[-1])) text_last--;
				if (text_last==p) return fail(p,"unexpected character in flow context");
				emit_scalar(p,string_t(p,text_last),YSS_PLAIN);
				p=q;
				continue;
			}
			line_start_=p;
			while (line_start_>base_ && !is_break(line_start_[-1])) line_start_--;
			line_end_=p;
			while (line_end_<last_ && !is_break(*line_end_)) line_end_++;
			next_position_=after_line(line_end_);
			return true;
		}

		//行内扫描:依次识别条目"- "/锚点/别名/标签/块标量/流集合/引用标量/plain标量与"键: "。
		//同一行检测到第二个块映射冒号即"mapping values are not allowed in this context"。
		bool scan_line(const char* p) {
			bool entries_only=true;
			while (true) {
				while (p<line_end_ && is_space(*p)) p++;
				if (p==line_end_) return true;
				if (*p=='#') return true;
				const std::size_t column=static_cast<std::size_t>(p-line_start_);
				const char c=*p;
				if (c=='-' && (p+1==line_end_ || is_space(p[1]))) {
					if (!entries_only) return fail(p,"block sequence entries are not allowed in this context");
					if (blocks_.empty() || !blocks_.back().sequence || blocks_.back().indent!=column) {
						blocks_.push_back(block_frame{column,true});
						emit(YS_BSEQ_START,p);
					}
					emit(YS_ENTRY,p);
					expect_node_=true;
					pending_indent_=column;
					pending_in_map_=false;
					p++;
					continue;
				}
				if (c=='?' && (p+1==line_end_ || is_space(p[1]))) return fail(p,"explicit mapping keys ('? ') are not supported");
				if (c==':' && (p+1==line_end_ || is_space(p[1]))) return fail(p,"a mapping value indicator requires an inline scalar key (complex keys are not supported)");
				if (c=='&' || c=='*') {
					entries_only=false;
					const char* q=p+1;
					while (q<line_end_ && !is_space(*q) && !is_flow_indicator(*q)) q++;
					if (q==p+1) return fail(p,c=='&'?"empty anchor name":"empty alias name");
					yaml_token token;
					token.symbol=(c=='&')?YS_ANCHOR:YS_ALIAS;
					token.position=position(p);
					token.text=string_t(p+1,q);
					tokens_.push_back(std::move(token));
					if (c=='&') expect_node_=true;
					else expect_node_=false;
					p=q;
					continue;
				}
				if (c=='!') {
					entries_only=false;
					const char* q=p+1;
					if (q<line_end_ && *q=='<') {
						q++;
						while (q<line_end_ && *q!='>') q++;
						if (q==line_end_) return fail(p,"unterminated verbatim tag");
						q++;
					} else {
						while (q<line_end_ && !is_space(*q) && !is_flow_indicator(*q)) q++;
					}
					yaml_token token;
					token.symbol=YS_TAG;
					token.position=position(p);
					token.text=string_t(p,q);
					tokens_.push_back(std::move(token));
					expect_node_=true;
					p=q;
					continue;
				}
				if (c=='|' || c=='>') {
					entries_only=false;
					return scan_block_scalar(p);
				}
				if (c=='[' || c=='{') {
					entries_only=false;
					if (!scan_flow(p)) return false;
					const char* q=p;
					while (q<line_end_ && is_space(*q)) q++;
					if (q<line_end_ && *q==':' && (q+1==line_end_ || is_space(q[1]))) return fail(q,"complex mapping keys are not supported");
					continue;
				}
				if (c=='\'' || c=='"') {
					entries_only=false;
					string_t text;
					const char* q=p;
					if (!scan_quoted(q,text)) return false;
					const int style=(c=='\'')?YSS_SINGLE_QUOTED:YSS_DOUBLE_QUOTED;
					const char* r=q;
					while (r<line_end_ && is_space(*r)) r++;
					if (r<line_end_ && *r==':' && (r+1==line_end_ || is_space(r[1]))) {
						if (!begin_mapping_pair(p,column,std::move(text),style,r)) return false;
						p=r+1;
						continue;
					}
					emit_scalar(p,std::move(text),style);
					p=q;
					continue;
				}
				entries_only=false;
				const char* span_last=line_end_;
				for (const char* r=p;r<line_end_;r++) {
					if (*r=='#' && r>p && is_space(r[-1])) {
						span_last=r;
						break;
					}
				}
				const char* colon=nullptr;
				for (const char* r=p;r<span_last;r++) {
					if (*r==':' && (r+1>=span_last || is_space(r[1]))) {
						colon=r;
						break;
					}
				}
				if (colon) {
					const char* key_last=colon;
					while (key_last>p && is_space(key_last[-1])) key_last--;
					if (key_last==p) return fail(p,"empty plain scalar mapping key");
					if (!begin_mapping_pair(p,column,string_t(p,key_last),YSS_PLAIN,colon)) return false;
					p=colon+1;
					continue;
				}
				const char* text_last=span_last;
				while (text_last>p && is_space(text_last[-1])) text_last--;
				emit_scalar(p,string_t(p,text_last),YSS_PLAIN);
				p=span_last;
				continue;
			}
		}
	};

	static bool tokenize(std::string_view input,std::vector<yaml_token>& tokens,std::size_t& error_position,std::string& error_message) {
		tokenizer scanner(input,tokens);
		if (!scanner.run()) {
			error_position=scanner.error_position_;
			error_message=scanner.error_message_;
			return false;
		}
		yaml_token eof_token;
		eof_token.symbol=YS_EOF;
		eof_token.position=input.size();
		tokens.push_back(std::move(eof_token));
		return true;
	}

	using parser_t=syntax::parser<yaml_symbol,yaml_production>;

	//SLR(1)文法:无ε产生式,全部左递归。结构终结符由词法合成,空流由START->EOF单列。
	static bool initialize_grammar(parser_t& target) {
		auto unit=[](yaml_symbol left,std::initializer_list<yaml_symbol> rights,yaml_production id){
			return syntax::single_parser_unit<yaml_symbol,yaml_production>(left,rights,id);
		};
		target.units={
			unit(YS_START,{YS_STREAM,YS_EOF},YP_START),
			unit(YS_START,{YS_EOF},YP_START_EMPTY),
			unit(YS_STREAM,{YS_DOCUMENT},YP_STREAM_FIRST),
			unit(YS_STREAM,{YS_STREAM,YS_DOCUMENT},YP_STREAM_APPEND),
			unit(YS_DOCUMENT,{YS_DOC_START,YS_NODE,YS_DOC_END},YP_DOCUMENT_PLAIN),
			unit(YS_DOCUMENT,{YS_DOC_START,YS_DOC_END},YP_DOCUMENT_BLANK),
			unit(YS_DOCUMENT,{YS_DIRECTIVE_SEQ,YS_DOC_START,YS_NODE,YS_DOC_END},YP_DOCUMENT_DIRECTIVE),
			unit(YS_DOCUMENT,{YS_DIRECTIVE_SEQ,YS_DOC_START,YS_DOC_END},YP_DOCUMENT_DIRECTIVE_BLANK),
			unit(YS_DIRECTIVE_SEQ,{YS_DIRECTIVE},YP_DIRECTIVES_FIRST),
			unit(YS_DIRECTIVE_SEQ,{YS_DIRECTIVE_SEQ,YS_DIRECTIVE},YP_DIRECTIVES_APPEND),
			unit(YS_NODE,{YS_SCALAR},YP_NODE_SCALAR),
			unit(YS_NODE,{YS_EMPTY},YP_NODE_EMPTY),
			unit(YS_NODE,{YS_ALIAS},YP_NODE_ALIAS),
			unit(YS_NODE,{YS_ANCHOR,YS_NODE},YP_NODE_ANCHOR),
			unit(YS_NODE,{YS_TAG,YS_NODE},YP_NODE_TAG),
			unit(YS_NODE,{YS_BSEQ_START,YS_BSEQ_ITEMS,YS_BSEQ_END},YP_NODE_BSEQ),
			unit(YS_NODE,{YS_BMAP_START,YS_BMAP_ITEMS,YS_BMAP_END},YP_NODE_BMAP),
			unit(YS_NODE,{YS_FSEQ_START,YS_FSEQ_END},YP_NODE_FSEQ_EMPTY),
			unit(YS_NODE,{YS_FSEQ_START,YS_FLOW_ITEMS,YS_FSEQ_END},YP_NODE_FSEQ),
			unit(YS_NODE,{YS_FMAP_START,YS_FMAP_END},YP_NODE_FMAP_EMPTY),
			unit(YS_NODE,{YS_FMAP_START,YS_FLOW_PAIRS,YS_FMAP_END},YP_NODE_FMAP),
			unit(YS_BSEQ_ITEMS,{YS_ENTRY,YS_NODE},YP_BSEQ_FIRST),
			unit(YS_BSEQ_ITEMS,{YS_BSEQ_ITEMS,YS_ENTRY,YS_NODE},YP_BSEQ_APPEND),
			unit(YS_BMAP_ITEMS,{YS_BMAP_PAIR},YP_BMAP_FIRST),
			unit(YS_BMAP_ITEMS,{YS_BMAP_ITEMS,YS_BMAP_PAIR},YP_BMAP_APPEND),
			unit(YS_BMAP_PAIR,{YS_SCALAR,YS_COLON,YS_NODE},YP_BMAP_PAIR),
			unit(YS_FLOW_ITEMS,{YS_NODE},YP_FLOW_ITEMS_FIRST),
			unit(YS_FLOW_ITEMS,{YS_FLOW_ITEMS,YS_COMMA,YS_NODE},YP_FLOW_ITEMS_APPEND),
			unit(YS_FLOW_PAIRS,{YS_FLOW_PAIR},YP_FLOW_PAIRS_FIRST),
			unit(YS_FLOW_PAIRS,{YS_FLOW_PAIRS,YS_COMMA,YS_FLOW_PAIR},YP_FLOW_PAIRS_APPEND),
			unit(YS_FLOW_PAIR,{YS_SCALAR,YS_COLON,YS_NODE},YP_FLOW_PAIR),
			unit(YS_FLOW_PAIR,{YS_SCALAR},YP_FLOW_PAIR_KEY),
		};
		target.generate_parser();
		return true;
	}
	static parser_t& grammar() {
		static parser_t instance(YS_START,YS_EPSILON,YS_EOF);
		static const bool initialized=initialize_grammar(instance);
		static_cast<void>(initialized);
		return instance;
	}
	static std::mutex& grammar_mutex() {
		static std::mutex instance;
		return instance;
	}

	//上下文相关检查与SAX事件次序全部落在listener(文法保持上下文无关):
	//①键事件在COLON移进时发出(id-2即键SCALAR),保证键先于值;②值标量在NODE->SCALAR归约发出;
	//③容器end_*在NODE归约发出(而非END移进),保证"{a}"等无冒号键的key/null先于end_mapping;
	//④标签强制与plain标量类型解析;⑤别名不得携带属性。
	class yaml_listener : public syntax::parser_listener<yaml_symbol,yaml_production> {
		std::vector<yaml_token>* tokens_=nullptr;
		sax_t* sax_=nullptr;
		bool aborted_=false;
		bool failed_=false;
		bool has_pending_tag_=false;
		string_t pending_tag_{};

		void abort_check(bool keep_going) {
			if (!keep_going) aborted_=true;
		}
		void fail(const yaml_token& token,const std::string& message) {
			failed_=true;
			const std::string text(token.text.begin(),token.text.end());
			sax_->parse_error(token.position,text,message);
		}
		void clear_pending_tag() {
			has_pending_tag_=false;
			pending_tag_.clear();
		}
		void emit_scalar_event(yaml_token& token) {
			int forced=0;
			if (has_pending_tag_) {
				forced=tag_class(pending_tag_);
				clear_pending_tag();
			}
			const yaml_scalar_style style=static_cast<yaml_scalar_style>(token.extra);
			switch (forced) {
				case 1: {
					abort_check(sax_->string(token.text,style));
					return;
				}
				case 2: {
					int_t value=0;
					if (plain_integer(token.text,value)) abort_check(sax_->number_integer(value));
					else fail(token,"scalar does not conform to the !!int tag");
					return;
				}
				case 3: {
					float_t value=0;
					if (plain_floating(token.text,value)) abort_check(sax_->number_float(value,token.text));
					else fail(token,"scalar does not conform to the !!float tag");
					return;
				}
				case 4: {
					const int value=plain_boolean(token.text);
					if (value<0) fail(token,"scalar does not conform to the !!bool tag");
					else abort_check(sax_->boolean(static_cast<boolean_t>(value==1)));
					return;
				}
				case 5: {
					if (plain_is_null(token.text)) abort_check(sax_->null());
					else fail(token,"scalar does not conform to the !!null tag");
					return;
				}
				default: break;
			}
			if (style!=YSS_PLAIN) {
				abort_check(sax_->string(token.text,style));
				return;
			}
			if (plain_is_null(token.text)) {
				abort_check(sax_->null());
				return;
			}
			const int truth=plain_boolean(token.text);
			if (truth>=0) {
				abort_check(sax_->boolean(static_cast<boolean_t>(truth==1)));
				return;
			}
			int_t integer_value=0;
			if (plain_integer(token.text,integer_value)) {
				abort_check(sax_->number_integer(integer_value));
				return;
			}
			float_t floating_value=0;
			if (plain_floating(token.text,floating_value)) {
				abort_check(sax_->number_float(floating_value,token.text));
				return;
			}
			abort_check(sax_->string(token.text,style));
		}

	public:
		void reset(std::vector<yaml_token>& tokens,sax_t& sax) {
			tokens_=&tokens;
			sax_=&sax;
			aborted_=false;
			failed_=false;
			clear_pending_tag();
			this->enabled=true;
		}
		bool aborted() const noexcept {
			return aborted_;
		}
		bool failed() const noexcept {
			return failed_;
		}
		intptr_t on_shift(uintptr_t id,int state,yaml_symbol word) override {
			static_cast<void>(state);
			if (aborted_ || failed_) return 0;
			yaml_token& token=(*tokens_)[id-1];
			switch (word) {
				case YS_DOC_START: abort_check(sax_->start_document());break;
				case YS_BSEQ_START:
				case YS_FSEQ_START: {
					clear_pending_tag();
					abort_check(sax_->start_sequence(static_cast<std::size_t>(-1)));
					break;
				}
				case YS_BMAP_START:
				case YS_FMAP_START: {
					clear_pending_tag();
					abort_check(sax_->start_mapping(static_cast<std::size_t>(-1)));
					break;
				}
				case YS_COLON: abort_check(sax_->key((*tokens_)[id-2].text));break;
				case YS_ANCHOR: abort_check(sax_->anchor(token.text));break;
				case YS_TAG: {
					has_pending_tag_=true;
					pending_tag_=token.text;
					abort_check(sax_->tag(token.text));
					break;
				}
				default: break;
			}
			return 0;
		}
		intptr_t on_reduction(uintptr_t id,int state,int next,yaml_production sentence_id,int reduction_num) override {
			static_cast<void>(state);
			static_cast<void>(next);
			static_cast<void>(reduction_num);
			if (aborted_ || failed_) return 0;
			switch (sentence_id) {
				case YP_NODE_SCALAR:
				case YP_NODE_EMPTY: emit_scalar_event((*tokens_)[id-2]);break;
				case YP_NODE_ALIAS: {
					yaml_token& token=(*tokens_)[id-2];
					if (has_pending_tag_) {
						fail(token,"an alias node must not have properties");
						return 0;
					}
					abort_check(sax_->alias(token.text));
					break;
				}
				case YP_NODE_BSEQ:
				case YP_NODE_FSEQ:
				case YP_NODE_FSEQ_EMPTY: abort_check(sax_->end_sequence());break;
				case YP_NODE_BMAP:
				case YP_NODE_FMAP:
				case YP_NODE_FMAP_EMPTY: abort_check(sax_->end_mapping());break;
				case YP_FLOW_PAIR_KEY: {
					yaml_token& token=(*tokens_)[id-2];
					abort_check(sax_->key(token.text));
					if (!aborted_) abort_check(sax_->null());
					break;
				}
				case YP_DIRECTIVES_FIRST:
				case YP_DIRECTIVES_APPEND: {
					yaml_token& token=(*tokens_)[id-2];
					abort_check(sax_->directive(token.text,token.aux));
					break;
				}
				case YP_DOCUMENT_PLAIN:
				case YP_DOCUMENT_BLANK:
				case YP_DOCUMENT_DIRECTIVE:
				case YP_DOCUMENT_DIRECTIVE_BLANK: abort_check(sax_->end_document());break;
				case YP_START:
				case YP_START_EMPTY:
				case YP_STREAM_FIRST:
				case YP_STREAM_APPEND:
				case YP_NODE_ANCHOR:
				case YP_NODE_TAG:
				case YP_BSEQ_FIRST:
				case YP_BSEQ_APPEND:
				case YP_BMAP_FIRST:
				case YP_BMAP_APPEND:
				case YP_BMAP_PAIR:
				case YP_FLOW_ITEMS_FIRST:
				case YP_FLOW_ITEMS_APPEND:
				case YP_FLOW_PAIRS_FIRST:
				case YP_FLOW_PAIRS_APPEND:
				case YP_FLOW_PAIR:
				default: break;
			}
			return 0;
		}
		void on_accept() override { }
		int on_error(uintptr_t id,typename syntax::parser_listener<yaml_symbol,yaml_production>::error_type type,int state,yaml_symbol word) override {
			static_cast<void>(type);
			static_cast<void>(state);
			static_cast<void>(word);
			failed_=true;
			if (sax_ && tokens_ && id!=static_cast<uintptr_t>(-1) && id>=1 && id<=tokens_->size()) {
				const yaml_token& token=(*tokens_)[id-1];
				const std::string text(token.text.begin(),token.text.end());
				sax_->parse_error(token.position,text,"Unexpected token");
			} else if (sax_) sax_->parse_error(0,std::string(),"Unexpected end of input");
			return 0;
		}
	};

public:
	static bool sax_parse(std::string_view input,sax_t* sax) {
		std::vector<yaml_token> tokens;
		std::size_t error_position=0;
		std::string error_message;
		if (!tokenize(input,tokens,error_position,error_message)) {
			sax->parse_error(error_position,std::string(),error_message);
			return false;
		}
		//空流(零文档)是合法yaml,由START->EOF接受,不在此拦截。
		std::vector<typename parser_t::parse_node> nodes;
		nodes.reserve(tokens.size());
		for (const auto& it:tokens) {
			typename parser_t::parse_node node;
			node.op=it.symbol;
			nodes.push_back(std::move(node));
		}
		std::lock_guard<std::mutex> lock(grammar_mutex());
		parser_t& parser=grammar();
		static yaml_listener listener;
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
	static yaml parse(std::string_view input,document_info_t* info=nullptr,bool allow_exceptions=true) {
		std::vector<yaml> documents;
		yaml_sax_dom_builder<yaml> builder(documents,info);
		const bool ok=sax_parse(input,&builder) && builder.completed();
		if (!ok) {
			if (allow_exceptions) throw std::runtime_error(std::string("Parse error at byte ")+std::to_string(builder.error_position())+std::string(": ")+(builder.error_message().empty()?std::string("Incomplete document"):builder.error_message()));
			return yaml();
		}
		if (documents.size()!=1) {
			if (allow_exceptions) throw std::runtime_error(documents.empty()?std::string("Parse error: the stream contains no document"):std::string("Parse error: the stream contains multiple documents, use parse_all"));
			return yaml();
		}
		return std::move(documents.front());
	}
	static std::vector<yaml> parse_all(std::string_view input,document_info_t* info=nullptr,bool allow_exceptions=true) {
		std::vector<yaml> documents;
		yaml_sax_dom_builder<yaml> builder(documents,info);
		const bool ok=sax_parse(input,&builder) && builder.completed();
		if (!ok) {
			if (allow_exceptions) throw std::runtime_error(std::string("Parse error at byte ")+std::to_string(builder.error_position())+std::string(": ")+(builder.error_message().empty()?std::string("Incomplete document"):builder.error_message()));
			return std::vector<yaml>();
		}
		return documents;
	}
	static bool try_parse(std::string_view input,yaml& out,document_info_t* info=nullptr) {
		std::vector<yaml> documents;
		yaml_sax_dom_builder<yaml> builder(documents,info);
		if (!sax_parse(input,&builder) || !builder.completed() || documents.size()!=1) return false;
		out=std::move(documents.front());
		return true;
	}
	static bool try_parse(std::string_view input,std::vector<yaml>& out,document_info_t* info=nullptr) {
		std::vector<yaml> documents;
		yaml_sax_dom_builder<yaml> builder(documents,info);
		if (!sax_parse(input,&builder) || !builder.completed()) return false;
		out=std::move(documents);
		return true;
	}
	static bool accept(std::string_view input) {
		yaml_sax_acceptor<yaml> acceptor;
		return sax_parse(input,&acceptor);
	}

private:
	static void dump_unicode_escape(string_t& out,unsigned long cp) {
		static const char digits[]="0123456789abcdef";
		out.push_back('\\');
		if (cp<0x10000) {
			out.push_back('u');
			for (int shift=12;shift>=0;shift-=4) out.push_back(digits[(cp>>shift)&0xF]);
		} else {
			out.push_back('U');
			for (int shift=28;shift>=0;shift-=4) out.push_back(digits[(cp>>shift)&0xF]);
		}
	}
	//双引号风格转义;ensure_ascii按UTF-8解码非ASCII序列并写\u/\U转义,非法UTF-8抛出。
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
			if (c<0x20 || c==0x7F) {
				dump_unicode_escape(out,c);
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
			if (ensure_ascii) dump_unicode_escape(out,cp);
			else {
				for (std::size_t i=0;i<=extra;i++) out.push_back(static_cast<typename string_t::value_type>(first[i]));
			}
			first+=extra+1;
		}
	}
	//plain风格安全性:为空、首字符为指示符、含"键冒号"/" #"/控制字符/换行、
	//或会被解析为null/bool/int/float的文本必须加引号;流上下文额外排斥流指示符与':'。
	static bool plain_safe(const string_t& s,bool flow,bool ensure_ascii) {
		if (s.empty()) return false;
		static const char indicators[]="-?:,[]{}#&*!|>'\"%@` \t";
		const char head=static_cast<char>(s[0]);
		for (const char* it=indicators;*it;it++) {
			if (head==*it) return false;
		}
		const char tail=static_cast<char>(s[s.size()-1]);
		if (tail==' ' || tail=='\t') return false;
		for (std::size_t i=0;i<s.size();i++) {
			const unsigned char c=static_cast<unsigned char>(s[i]);
			if (c<0x20 || c==0x7F) return false;
			if (ensure_ascii && c>=0x80) return false;
			if (c==':' && (i+1==s.size() || s[i+1]==' ' || s[i+1]=='\t')) return false;
			if (c=='#' && i>0 && (s[i-1]==' ' || s[i-1]=='\t')) return false;
			if (flow && (c==',' || c=='[' || c==']' || c=='{' || c=='}' || c==':')) return false;
		}
		if (plain_is_null(s)) return false;
		if (plain_boolean(s)>=0) return false;
		int_t integer_value=0;
		if (plain_integer(s,integer_value)) return false;
		float_t floating_value=0;
		if (plain_floating(s,floating_value)) return false;
		return true;
	}
	static void dump_scalar_string(string_t& out,const string_t& s,bool flow,bool ensure_ascii) {
		if (plain_safe(s,flow,ensure_ascii)) {
			out.append(s.begin(),s.end());
			return;
		}
		out.push_back('"');
		dump_escaped(out,s,ensure_ascii);
		out.push_back('"');
	}
	static void dump_integer(string_t& out,int_t value) {
		const std::string text=std::to_string(static_cast<long long>(value));
		out.append(text.begin(),text.end());
	}
	//非有限值用yaml原生.nan/.inf/-.inf;有限值%.15g起步、必要时%.17g保往返,
	//无小数点与指数时补".0"以保证重读仍为float。
	static void dump_floating(string_t& out,float_t value) {
		if (std::isnan(value)) {
			out.append({'.','n','a','n'});
			return;
		}
		if (std::isinf(value)) {
			if (value<0) out.push_back('-');
			out.append({'.','i','n','f'});
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
	static bool is_scalar_node(const base_t& node) noexcept {
		switch (node.type()) {
			case structure::DDT_NULL:
			case structure::DDT_BOOL:
			case structure::DDT_INT:
			case structure::DDT_FLOAT:
			case structure::DDT_STRING: return true;
			default: return false;
		}
	}
	static void dump_scalar(const base_t& node,string_t& out,bool flow,bool ensure_ascii) {
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
				dump_scalar_string(out,*node.template get_ptr<const string_t*>(),flow,ensure_ascii);
				break;
			}
			default: break;
		}
	}
	//外来派生节点(如BDT_BINARY/XDT_ELEMENT)不属于yaml数据模型:依记法转换协议,
	//请先经assign_converted/convert_to(源侧degrade_unsupported或convert_handler_t)落回基础七型再dump。
	static void unsupported_node(const base_t& node) {
		throw std::invalid_argument("yaml: unsupported node type "+std::to_string(static_cast<long long>(static_cast<int>(node.type())))+", convert it to the base dom model first (assign_converted/convert_to)");
	}
	static void dump_flow(const base_t& node,string_t& out,bool ensure_ascii) {
		if (is_scalar_node(node)) {
			dump_scalar(node,out,true,ensure_ascii);
			return;
		}
		switch (node.type()) {
			case structure::DDT_ARRAY: {
				out.push_back('[');
				for (auto it=node.cbegin();it!=node.cend();) {
					dump_flow(*it,out,ensure_ascii);
					it++;
					if (it!=node.cend()) {
						out.push_back(',');
						out.push_back(' ');
					}
				}
				out.push_back(']');
				break;
			}
			case structure::DDT_OBJECT: {
				out.push_back('{');
				for (auto it=node.cbegin();it!=node.cend();) {
					dump_scalar_string(out,it.key(),true,ensure_ascii);
					out.push_back(':');
					out.push_back(' ');
					dump_flow(*it,out,ensure_ascii);
					it++;
					if (it!=node.cend()) {
						out.push_back(',');
						out.push_back(' ');
					}
				}
				out.push_back('}');
				break;
			}
			default: unsupported_node(node);
		}
	}
	static void dump_indent(string_t& out,std::size_t count,typename string_t::value_type indent_char) {
		for (std::size_t i=0;i<count;i++) out.push_back(indent_char);
	}
	//块风格:序列项为容器时采用"- "续行内联(项内续行固定+2列,yaml允许各块自选缩进宽度),
	//映射值为容器时换行加indent_step缩进;空容器以流式[]/{}内联。
	static void dump_block(const base_t& node,string_t& out,int indent_step,typename string_t::value_type indent_char,bool ensure_ascii,std::size_t current_indent,bool skip_first_indent) {
		switch (node.type()) {
			case structure::DDT_ARRAY: {
				bool first=true;
				for (auto it=node.cbegin();it!=node.cend();it++) {
					if (!(skip_first_indent && first)) dump_indent(out,current_indent,indent_char);
					first=false;
					const base_t& child=*it;
					if (is_scalar_node(child)) {
						out.push_back('-');
						out.push_back(' ');
						dump_scalar(child,out,false,ensure_ascii);
						out.push_back('\n');
					} else if (child.empty()) {
						out.push_back('-');
						out.push_back(' ');
						out.push_back(child.type()==structure::DDT_ARRAY?'[':'{');
						out.push_back(child.type()==structure::DDT_ARRAY?']':'}');
						out.push_back('\n');
					} else if (child.type()==structure::DDT_ARRAY || child.type()==structure::DDT_OBJECT) {
						out.push_back('-');
						out.push_back(' ');
						dump_block(child,out,indent_step,indent_char,ensure_ascii,current_indent+2,true);
					} else unsupported_node(child);
				}
				break;
			}
			case structure::DDT_OBJECT: {
				bool first=true;
				for (auto it=node.cbegin();it!=node.cend();it++) {
					if (!(skip_first_indent && first)) dump_indent(out,current_indent,indent_char);
					first=false;
					dump_scalar_string(out,it.key(),false,ensure_ascii);
					out.push_back(':');
					const base_t& child=*it;
					if (is_scalar_node(child)) {
						out.push_back(' ');
						dump_scalar(child,out,false,ensure_ascii);
						out.push_back('\n');
					} else if (child.empty()) {
						out.push_back(' ');
						out.push_back(child.type()==structure::DDT_ARRAY?'[':'{');
						out.push_back(child.type()==structure::DDT_ARRAY?']':'}');
						out.push_back('\n');
					} else if (child.type()==structure::DDT_ARRAY || child.type()==structure::DDT_OBJECT) {
						out.push_back('\n');
						dump_block(child,out,indent_step,indent_char,ensure_ascii,current_indent+static_cast<std::size_t>(indent_step),false);
					} else unsupported_node(child);
				}
				break;
			}
			default: unsupported_node(node);
		}
	}

public:
	//indent<0:流风格单行(与json的compact对应);indent>=0:块风格,indent为层缩进宽度(0按1处理)。
	virtual string_t dump(int indent=-1,typename string_t::value_type indent_char=' ',bool ensure_ascii=false) const {
		string_t result;
		if (indent<0) {
			dump_flow(*this,result,ensure_ascii);
			return result;
		}
		if (is_scalar_node(*this)) {
			dump_scalar(*this,result,false,ensure_ascii);
			return result;
		}
		if (this->empty()) {
			result.push_back(this->type()==structure::DDT_ARRAY?'[':'{');
			result.push_back(this->type()==structure::DDT_ARRAY?']':'}');
			return result;
		}
		const int indent_step=indent>0?indent:1;
		dump_block(*this,result,indent_step,indent_char,ensure_ascii,0,false);
		if (!result.empty() && result.back()=='\n') result.pop_back();
		return result;
	}
	//单文档序列化:%YAML/%TAG指令(如info给出)+"---"+内容,恒以换行收尾。
	static string_t dump_document(const base_t& root,const document_info_t* info=nullptr,int indent=2,typename string_t::value_type indent_char=' ',bool ensure_ascii=false) {
		string_t result;
		if (info && info->has_version()) {
			result.append({'%','Y','A','M','L',' '});
			result.append(info->version.begin(),info->version.end());
			result.push_back('\n');
		}
		if (info) {
			for (const auto& it:info->tag_directives) {
				result.append({'%','T','A','G',' '});
				result.append(it.first.begin(),it.first.end());
				result.push_back(' ');
				result.append(it.second.begin(),it.second.end());
				result.push_back('\n');
			}
		}
		result.append({'-','-','-'});
		if (indent<0) {
			result.push_back(' ');
			dump_flow(root,result,ensure_ascii);
			result.push_back('\n');
			return result;
		}
		if (is_scalar_node(root)) {
			result.push_back(' ');
			dump_scalar(root,result,false,ensure_ascii);
			result.push_back('\n');
			return result;
		}
		if (root.empty()) {
			result.push_back(' ');
			result.push_back(root.type()==structure::DDT_ARRAY?'[':'{');
			result.push_back(root.type()==structure::DDT_ARRAY?']':'}');
			result.push_back('\n');
			return result;
		}
		result.push_back('\n');
		dump_block(root,result,indent>0?indent:1,indent_char,ensure_ascii,0,false);
		return result;
	}
	//多文档流序列化,与parse_all对偶。
	static string_t dump_all(const std::vector<yaml>& documents,int indent=2,typename string_t::value_type indent_char=' ',bool ensure_ascii=false) {
		string_t result;
		for (const auto& it:documents) result+=dump_document(it,nullptr,indent,indent_char,ensure_ascii);
		return result;
	}

	friend std::ostream& operator <<(std::ostream& os,const yaml& value) {
		const int indent_step=static_cast<int>(os.width());
		os.width(0);
		const string_t text=value.dump(indent_step>0?indent_step:-1,static_cast<typename string_t::value_type>(os.fill()));
		os.write(reinterpret_cast<const char*>(text.data()),static_cast<std::streamsize>(text.size()));
		return os;
	}
	friend std::istream& operator >>(std::istream& is,yaml& value) {
		std::string content;
		char buffer[4096];
		while (is.read(buffer,sizeof(buffer))) content.append(buffer,sizeof(buffer));
		content.append(buffer,static_cast<std::size_t>(is.gcount()));
		value=parse(content);
		return is;
	}
};

_STDEX_DOM_TPL_DECLARATION
inline typename yaml<_Int,_Float,_Boolean,_String,_Array,_Object,_Allocator>::string_t to_string(const yaml<_Int,_Float,_Boolean,_String,_Array,_Object,_Allocator>& value) {
	return value.dump();
}

}

_STDEX_DOM_TPL_DECLARATION
using yaml_t=basic_yaml::yaml<_Int,_Float,_Boolean,_String,_Array,_Object,_Allocator>;
using yaml=yaml_t<>;
using basic_yaml::yaml_sax;
using basic_yaml::yaml_sax_dom_builder;
using basic_yaml::yaml_sax_acceptor;
using basic_yaml::basic_yaml_document_info;
using basic_yaml::yaml_scalar_style;
using basic_yaml::YSS_PLAIN;
using basic_yaml::YSS_SINGLE_QUOTED;
using basic_yaml::YSS_DOUBLE_QUOTED;
using basic_yaml::YSS_LITERAL;
using basic_yaml::YSS_FOLDED;
using basic_yaml::to_string;

inline namespace literals {

inline yaml_t<> operator ""_yaml(const char* s,std::size_t n) {
	return yaml_t<>::parse(std::string_view(s,n));
}

}

}

}

#endif
