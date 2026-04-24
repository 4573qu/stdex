//Last Modified At 2026/04/25
//@Version 1.0.0.0
#ifndef _STDEX_MACHINE_DEBUG_H_
#define _STDEX_MACHINE_DEBUG_H_ 1

#include <atomic>
#include <chrono>
#include <cstdarg>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <functional>
#include <mutex>
#include <string>
#include <string_view>

#include "../bitwise/flags.h"//At Least 1.1

#if __has_include("../macros/cpp_compiler.h")
#include "../macros/cpp_compiler.h"//At Least 1.0
#endif

#ifndef _STDEX_GNU_COMPILER
#if defined(__GNUC__)
#define _STDEX_GNU_COMPILER 1
#else
#define _STDEX_GNU_COMPILER 0
#endif
#endif
#ifndef _STDEX_CLANG_COMPILER
#if defined(__clang__)
#define _STDEX_CLANG_COMPILER 1
#else
#define _STDEX_CLANG_COMPILER 0
#endif
#endif
#ifndef _STDEX_MSVC_COMPILER
#if defined(_MSC_VER)
#define _STDEX_MSVC_COMPILER 1
#else
#define _STDEX_MSVC_COMPILER 0
#endif
#endif

#if __has_include("../macros/cpp_platform.h")
#include "../macros/cpp_platform.h"//At Least 1.0
#endif

#ifndef _STDEX_WINDOWS_PLATFORM
#if defined(_WIN32)
#define _STDEX_WINDOWS_PLATFORM 1
#else
#define _STDEX_WINDOWS_PLATFORM 0
#endif
#endif
#ifndef _STDEX_LINUX_PLATFORM
#if defined(__linux__)
#define _STDEX_LINUX_PLATFORM 1
#else
#define _STDEX_LINUX_PLATFORM 0
#endif
#endif
#ifndef _STDEX_ANDROID_PLATFORM
#if defined(__ANDROID__)
#define _STDEX_ANDROID_PLATFORM 1
#else
#define _STDEX_ANDROID_PLATFORM 0
#endif
#endif
#ifndef _STDEX_APPLE_PLATFORM
#if defined(__APPLE__)
#define _STDEX_APPLE_PLATFORM 1
#else
#define _STDEX_APPLE_PLATFORM 0
#endif
#endif

#if _STDEX_WINDOWS_PLATFORM
	#include <windows.h>
	#include <direct.h>
#elif _STDEX_ANDROID_PLATFORM
	#include <android/log.h>
	#include <fcntl.h>
	#include <unistd.h>
	#include <sys/stat.h>
#elif _STDEX_APPLE_PLATFORM
	#include <mach-o/dyld.h>
	#include <fcntl.h>
	#include <unistd.h>
	#include <sys/stat.h>
#elif _STDEX_LINUX_PLATFORM
	#include <fcntl.h>
	#include <unistd.h>
	#include <sys/stat.h>
#endif

namespace stdex {

namespace machine {

namespace debug {

static constexpr std::size_t debug_buf_size=2048u;
static constexpr std::size_t max_alloc_size=10u*1024u*1024u;
static constexpr long long throttle_seconds=1LL;
static constexpr std::size_t hesitation_buf_size=262144u;
static constexpr std::size_t hesitation_msg_size=256u;

}

enum debug_feature {
	DF_NONE=0,
	DF_OUTPUT=1<<0,
	DF_STDERR=1<<1,
	DF_LOGGING=1<<2,
	DF_TIMESTAMP=1<<3,
	DF_LOCATION=1<<4,
	DF_THROTTLE=1<<5,
	DF_MEMORY=1<<6,
	DF_HESITATE=1<<7,
	DF_ALL=~0u,
};

enum debug_level {
	DL_VERBOSE=0,
	DL_INFO=1,
	DL_WARNING=2,
	DL_ERROR=3,
	DL_FATAL=4,
};

enum assert_action {
	AA_IGNORE=0,
	AA_BREAK=1,
	AA_ABORT=2,
};

struct log_record {
	debug_level level;
	std::string message;
	std::string file;
	int line;
	std::string function;
	std::string timestamp;

	const char* to_string(debug_level lv) {
		switch (lv) {
			case DL_VERBOSE: return "VERBOSE";
			case DL_INFO: return "INFO";
			case DL_WARNING: return "WARNING";
			case DL_ERROR: return "ERROR";
			case DL_FATAL: return "FATAL";
			default: return "UNKNOWN";
		}
	}

	std::string to_string() const {
		std::string s;
		s.reserve(256);
		if (!timestamp.empty()) {
			s+='[';
			s+=timestamp;
			s+="] ";
		}
		s+='[';
		s+=to_string(level);
		s+=']';
		if (!file.empty()) {
			s+=' ';
			s+=file;
			s+=':';
			s+=std::to_string(line);
		}
		if (!function.empty()) {
			s+=" (";
			s+=function;
			s+=')';
		}
		s+=' ';
		s+=message;
		return s;
	}
};

struct assert_record {
	std::string condition;
	std::string message;
	std::string file;
	int line;
	std::string function;
	std::string timestamp;

	std::string to_string() const {
		std::string s;
		s.reserve(512);
		if (!timestamp.empty()) {
			s+='[';
			s+=timestamp;
			s+="] ";
		}
		s+="ASSERT FAILED";
		if (!file.empty()) {
			s+="@";
			s+=file;
			s+=':';
			s+=std::to_string(line);
		}
		if (!function.empty()) {
			s+=" in ";
			s+=function;
		}
		if (!condition.empty()) {
			s+="\nCondition:";
			s+=condition;
		}
		if (!message.empty()) {
			s+="\nMessage:";
			s+=message;
		}
		return s;
	}
};

struct memory_trace {
	memory_trace* next;
	memory_trace* prev;
	std::size_t bytes;
	const char* file;
	int line;
	const char* function;

	std::string to_string() const {
		std::string s;
		s.reserve(128);
		s+="Alloc ";
		s+=std::to_string(bytes);
		s+=" bytes";
		if (file) {
			s+=" @ ";
			s+=file;
			s+=':';
			s+=std::to_string(line);
		}
		if (function) {
			s+=" (";
			s+=function;
			s+=')';
		}
		return s;
	}
};

struct memory_tracker {
	bool initialized;
	std::mutex mtx;
	memory_trace* head;
	std::atomic<std::size_t> total_bytes {0};

	memory_tracker() : initialized(true) , head(nullptr) { }
	memory_tracker(const memory_tracker&)=delete;
	memory_tracker& operator=(const memory_tracker&)=delete;
};

struct hesitation_buffer {
	using clock_t=std::chrono::steady_clock;
	using timepoint_t=clock_t::time_point;

	std::size_t current_pos;
	timepoint_t start_time;
	int last_trace_ms;
	int indent;
	bool recording;
	char text[debug::hesitation_buf_size];

	hesitation_buffer() : current_pos(0) , start_time(clock_t::now()) , last_trace_ms(0) , indent(0) , recording(true) {
		text[0]='\0';
	}

	hesitation_buffer(const hesitation_buffer&)=delete;
	hesitation_buffer& operator =(const hesitation_buffer&)=delete;

	void reset() {
		current_pos=0;
		start_time=clock_t::now();
		last_trace_ms=0;
		indent=0;
		recording=true;
		text[0]='\0';
	}

	int elapsed_ms() const {
		return static_cast<int>(std::chrono::duration_cast<std::chrono::milliseconds>(clock_t::now()-start_time).count());
	}
};

struct hesitation_bracket {
	hesitation_buffer* buffer;
	char message[debug::hesitation_msg_size];
	hesitation_buffer::timepoint_t enter_time;
	int threshold_ms;
	bool active;

	hesitation_bracket(hesitation_buffer* buf,int warn_threshold_ms,const char* fmt,...) : buffer(buf) , threshold_ms(warn_threshold_ms) , active(buf && buf->recording) {
		message[0]='\0';
		if (!active) return;
		va_list args;
		va_start(args, fmt);
		std::vsnprintf(message,debug::hesitation_msg_size,fmt,args);
		va_end(args);
		enter_time=hesitation_buffer::clock_t::now();
		++buf->indent;
	}

	~hesitation_bracket() {
		end_bracket();
	}

	void end_bracket() {
		if (!active) return;
		active=false;
		if (!buffer) return;
		--buffer->indent;
		const int elapsed=static_cast<int>(std::chrono::duration_cast<std::chrono::milliseconds>(hesitation_buffer::clock_t::now()-enter_time).count());
		if (elapsed>=threshold_ms && buffer->current_pos<debug::hesitation_buf_size-128) {
			int written=std::snprintf(buffer->text+buffer->current_pos,debug::hesitation_buf_size-buffer->current_pos,"%*s[HESITATE] %s : %d ms\n",buffer->indent*2,"",message,elapsed);
			if (written>0) buffer->current_pos+=static_cast<std::size_t>(written);
		}
	}

	hesitation_bracket(const hesitation_bracket&)=delete;
	hesitation_bracket& operator =(const hesitation_bracket&)=delete;
};

struct debug_config {
public:
	using log_handler_t=std::function<void(const log_record&)>;
	using assert_handler_t=std::function<assert_action(const assert_record&)>;
	using submit_handler_t=std::function<void()>;

private:

	std::FILE* log_file_ =nullptr;
	std::string log_path_;
	std::atomic<bool> in_assert_{false};

	static std::string make_timestamp() {
		const auto now=std::chrono::system_clock::now();
		const std::time_t t=std::chrono::system_clock::to_time_t(now);
		char buf[32]={};
		#if _STDEX_WINDOWS_PLATFORM
			struct tm tm_buf={};
			localtime_s(&tm_buf,&t);
			std::strftime(buf,sizeof(buf),"%Y-%m-%d %H:%M:%S",&tm_buf);
		#else
			struct tm tm_buf={};
			localtime_r(&t,&tm_buf);
			std::strftime(buf,sizeof(buf),"%Y-%m-%d %H:%M:%S",&tm_buf);
		#endif
		return buf;
	}

	static void platform_debug_output(const char* msg) {
		#if _STDEX_WINDOWS_PLATFORM
			OutputDebugStringA(msg);
		#elif _STDEX_ANDROID_PLATFORM
			__android_log_print(ANDROID_LOG_DEBUG,"stdex_debug","%s",msg);
		#else
			(void)msg;
		#endif
	}

	friend void log_string(debug_config&,std::string_view);
	friend void logv(debug_config&,debug_level,const char*,int,const char*,const char*,va_list);
	friend void assert_failed(debug_config&,const char*,const char*,int,const char*,const char*,...);

public:
	bitwise::flags<debug_feature> features=(debug_feature)(DF_OUTPUT|DF_STDERR|DF_TIMESTAMP);
	debug_level min_level=DL_VERBOSE;
	std::string debug_data_folder;
	log_handler_t log_handler;
	assert_handler_t assert_handler;
	submit_handler_t submit_handler;
	memory_tracker* mem_tracker=nullptr;
	hesitation_buffer* hes_buffer=nullptr;

	debug_config()=default;
	debug_config(const debug_config&)=delete;
	debug_config(debug_config&&)=delete;
	debug_config& operator =(const debug_config&)=delete;
	debug_config& operator =(debug_config&&)=delete;

	~debug_config() {
		close_log_file();
	}

	bool open_log_file(const std::string& path) {
		close_log_file();
		log_file_=std::fopen(path.c_str(), "a");
		if (!log_file_) return false;
		log_path_=path;
		return true;
	}

	void close_log_file() {
		if (log_file_) {
			std::fflush(log_file_);
			std::fclose(log_file_);
			log_file_=nullptr;
			log_path_.clear();
		}
	}
};

inline debug_config& default_config() {
	static debug_config cfg;
	return cfg;
}

namespace debug {

inline int vsnprintf_safe(char* buf,std::size_t size,const char* fmt,va_list args) {
	if (!buf || size==0) return 0;
	int n=std::vsnprintf(buf,size,fmt,args);
	if (n < 0) {
		buf[0]='\0';
		return 0;
	}
	if (static_cast<std::size_t>(n)>=size) n=static_cast<int>(size)-1;
	buf[n]='\0';
	return n;
}

inline void ensure_newline(char* buf,std::size_t capacity,int& length) {
	if (length<=0) return;
	if (buf[length-1]=='\n') return;
	if (static_cast<std::size_t>(length+1)<capacity) {
		buf[length]='\n';
		buf[length+1]='\0';
		length++;
	} else buf[length-1]='\n';
}

inline std::string format_string(const char* fmt,...) {
	char buf[debug::debug_buf_size];
	va_list args;
	va_start(args,fmt);
	vsnprintf_safe(buf,sizeof(buf),fmt,args);
	va_end(args);
	return buf;
}

inline bool is_debugger_present() {
	#if _STDEX_WINDOWS_PLATFORM
		return IsDebuggerPresent()!=FALSE;
	#elif _STDEX_LINUX_PLATFORM || _STDEX_ANDROID_PLATFORM
		int fd=open("/proc/self/status",O_RDONLY);
		if (fd<0) return false;
		char buf[1024]={};
		ssize_t n=read(fd,buf,sizeof(buf)-1);
		close(fd);
		if (n<=0) return false;
		const char* p=std::strstr(buf,"TracerPid:");
		if (!p) return false;
		p+=10;
		while (*p==' ' || *p=='\t') p++;
		return *p!='0';
	#else
		return false;
	#endif
}

inline std::string get_exe_directory() {
	#if _STDEX_WINDOWS_PLATFORM
		char path[MAX_PATH]={};
		GetModuleFileNameA(nullptr,path,MAX_PATH);
		char* last=std::strrchr(path,'\\');
		if (last) *(last+1)='\0';
		return path;
	#elif _STDEX_APPLE_PLATFORM
		char path[4096]={};
		uint32_t size=sizeof(path);
		if (_NSGetExecutablePath(path,&size)!=0) return "";
		char* last=std::strrchr(path,'/');
		if (last) *(last+1)='\0';
		return path;
	#elif _STDEX_LINUX_PLATFORM || _STDEX_ANDROID_PLATFORM
		char path[4096]={};
		ssize_t len=readlink("/proc/self/exe",path,sizeof(path)-1);
		if (len<0) return "";
		path[len]='\0';
		char* last=std::strrchr(path,'/');
		if (last) *(last+1)='\0';
		return path;
	#else
		return "";
	#endif
}

inline bool ensure_directory(const std::string& path) {
	if (path.empty()) return false;
	#if _STDEX_WINDOWS_PLATFORM
		return CreateDirectoryA(path.c_str(),nullptr)!=FALSE || GetLastError()==ERROR_ALREADY_EXISTS;
	#else
		return mkdir(path.c_str(),0755)==0 || errno==EEXIST;
	#endif
}

inline std::string make_log_path(const std::string& dir,const std::string& name) {
	if (dir.empty()) return name;
	#if _STDEX_WINDOWS_PLATFORM
		const char sep='\\';
	#else
		const char sep='/';
	#endif
	std::string result=dir;
	if (result.back()!=sep) result+=sep;
	result+=name;
	return result;
}

inline void log_string(debug_config& cfg,std::string_view msg) {
	if (msg.empty()) return;
	std::string s(msg);
	if (s.back()!='\n') s+='\n';
	if (cfg.features.contains(DF_OUTPUT)) debug_config::platform_debug_output(s.c_str());
	if (cfg.features.contains(DF_STDERR)) std::fputs(s.c_str(),stderr);
	if ((cfg.features.contains(DF_LOGGING)) && cfg.log_file_) {
		std::fputs(s.c_str(),cfg.log_file_);
		std::fflush(cfg.log_file_);
	}
}

inline void log_string(std::string_view msg) {
	log_string(default_config(),msg);
}

inline void logv(debug_config& cfg,debug_level level,const char* file,int line,const char* func,const char* fmt,va_list args) {
	if (level<cfg.min_level) return;
	char buf[debug::debug_buf_size];
	int len=vsnprintf_safe(buf,sizeof(buf),fmt,args);
	ensure_newline(buf,sizeof(buf),len);
	log_record rec;
	rec.level=level;
	rec.message=buf;
	rec.file=(cfg.features.contains(DF_LOCATION)) && file?file:"";
	rec.line=(cfg.features.contains(DF_LOCATION))?line:0;
	rec.function=(cfg.features.contains(DF_LOCATION)) && func?func:"";
	if (cfg.features.contains(DF_TIMESTAMP)) rec.timestamp=debug_config::make_timestamp();
	if (cfg.log_handler) {
		cfg.log_handler(rec);
		return;
	}
	std::string s=rec.to_string();
	if (s.empty() || s.back()!='\n') s+='\n';
	if (cfg.features.contains(DF_OUTPUT)) debug_config::platform_debug_output(s.c_str());
	if (cfg.features.contains(DF_STDERR)) std::fputs(s.c_str(), stderr);
	if ((cfg.features.contains(DF_LOGGING)) && cfg.log_file_) {
		std::fputs(s.c_str(),cfg.log_file_);
		std::fflush(cfg.log_file_);
	}
}

inline void log(debug_config& cfg,debug_level level,const char* file,int line,const char* func,const char* fmt,...) {
	va_list args;
	va_start(args,fmt);
	logv(cfg,level,file,line,func,fmt,args);
	va_end(args);
}

inline void log(debug_config& cfg,debug_level level,const char* fmt,...) {
	va_list args;
	va_start(args,fmt);
	logv(cfg,level,nullptr,0,nullptr,fmt,args);
	va_end(args);
}

inline void log(debug_level level,const char* fmt,...) {
	va_list args;
	va_start(args,fmt);
	logv(default_config(),level,nullptr,0,nullptr,fmt,args);
	va_end(args);
}

inline void trace(debug_config& cfg,const char* fmt,...) {
	va_list args;
	va_start(args,fmt);
	logv(cfg, DL_VERBOSE,nullptr,0,nullptr,fmt,args);
	va_end(args);
}

inline void trace(const char* fmt,...) {
	va_list args;
	va_start(args,fmt);
	logv(default_config(),DL_VERBOSE,nullptr,0,nullptr,fmt,args);
	va_end(args);
}

inline void trace_and_log(debug_config& cfg,debug_level level,const char* fmt,...) {
	va_list args;
	va_start(args,fmt);
	logv(cfg, level,nullptr,0,nullptr,fmt,args);
	va_end(args);
}

inline void trace_and_log(debug_level level,const char* fmt,...) {
	va_list args;
	va_start(args,fmt);
	logv(default_config(),level,nullptr,0,nullptr,fmt,args);
	va_end(args);
}

inline void trace_throttled(debug_config& cfg,const char* fmt,...) {
	using clock_t=std::chrono::steady_clock;
	static clock_t::time_point last_tp=clock_t::time_point::min();
	if (cfg.features.contains(DF_THROTTLE)) {
		const auto now=clock_t::now();
		const auto elapsed=std::chrono::duration_cast<std::chrono::seconds>(now-last_tp).count();
		if (elapsed<debug::throttle_seconds) return;
		last_tp=now;
	}
	va_list args;
	va_start(args,fmt);
	logv(cfg,DL_VERBOSE,nullptr,0,nullptr,fmt,args);
	va_end(args);
}

inline void trace_throttled(const char* fmt,...) {
	using clock_t=std::chrono::steady_clock;
	static clock_t::time_point last_tp=clock_t::time_point::min();
	auto& cfg=default_config();
	if (cfg.features.contains(DF_THROTTLE)) {
		const auto now=clock_t::now();
		const auto elapsed=std::chrono::duration_cast<std::chrono::seconds>(now-last_tp).count();
		if (elapsed<debug::throttle_seconds) return;
		last_tp=now;
	}
	va_list args;
	va_start(args,fmt);
	logv(cfg,DL_VERBOSE,nullptr,0,nullptr,fmt,args);
	va_end(args);
}

inline void trace_throttled_n(debug_config& cfg,int every_n,const char* fmt,...) {
	static std::atomic<int> counter{0};
	const int c=counter.fetch_add(1,std::memory_order_relaxed);
	if (every_n<=0 || (c%every_n)!=0) return;
	va_list args;
	va_start(args,fmt);
	logv(cfg,DL_VERBOSE,nullptr,0,nullptr,fmt,args);
	va_end(args);
}

inline void trace_throttled_n(int every_n,const char* fmt,...) {
	static std::atomic<int> counter{0};
	const int c=counter.fetch_add(1,std::memory_order_relaxed);
	if (every_n<=0 || (c%every_n)!=0) return;
	va_list args;
	va_start(args,fmt);
	logv(default_config(),DL_VERBOSE,nullptr,0,nullptr,fmt,args);
	va_end(args);
}

inline void assert_failed(debug_config& cfg,const char* condition,const char* file,int line,const char* func,const char* fmt,...) {
	char msg_buf[debug::debug_buf_size]={};
	if (fmt && fmt[0]!='\0') {
		va_list args;
		va_start(args,fmt);
		vsnprintf_safe(msg_buf,sizeof(msg_buf),fmt,args);
		va_end(args);
	}
	assert_record rec;
	rec.condition=condition?condition:"";
	rec.message=msg_buf;
	rec.file=file?file:"";
	rec.line=line;
	rec.function=func?func:"";
	if (cfg.features.contains(DF_TIMESTAMP)) rec.timestamp=debug_config::make_timestamp();
	log_string(cfg,rec.to_string());
	assert_action action=AA_ABORT;
	bool expected=false;
	if (cfg.in_assert_.compare_exchange_strong(expected, true)) {
		if (cfg.assert_handler) action=cfg.assert_handler(rec);
		cfg.in_assert_.store(false);
	} else {
		log_string(cfg,"Recursive assert detected - aborting.\n");
		std::abort();
	}
	if (cfg.submit_handler) cfg.submit_handler();
	switch (action) {
		case AA_IGNORE: {
			return;
		}
		case AA_BREAK: {
			if (is_debugger_present()) {
				#if _STDEX_WINDOWS_PLATFORM
					__debugbreak();
				#elif _STDEX_GNU_COMPILER || _STDEX_CLANG_COMPILER
					__builtin_trap();
				#endif
			}
			return;
		}
		case AA_ABORT:
		default: {
			std::abort();
		}
	}
}

inline void assert_failed(const char* condition,const char* file,int line,const char* func,const char* fmt,...) {
	char msg_buf[debug::debug_buf_size]={};
	if (fmt && fmt[0]!='\0') {
		va_list args;
		va_start(args,fmt);
		vsnprintf_safe(msg_buf,sizeof(msg_buf),fmt,args);
		va_end(args);
	}
	assert_failed(default_config(),condition,file,line,func,"%s",msg_buf);
}

inline void* safe_malloc(std::size_t size) {
	if (size==0 || size>debug::max_alloc_size) return nullptr;
	return std::malloc(size);
}

inline void safe_free(void* ptr) {
	if (ptr) std::free(ptr);
}

inline void* tracked_malloc(debug_config& cfg,std::size_t size,const char* file,int line,const char* func) {
	if (size==0 || size>debug::max_alloc_size) return nullptr;
	void* raw=std::malloc(sizeof(memory_trace)+size);
	if (!raw) return nullptr;
	auto* node=static_cast<memory_trace*>(raw);
	node->next=nullptr;
	node->prev=nullptr;
	node->bytes=size;
	node->file=file;
	node->line=line;
	node->function=func;
	if ((cfg.features.contains(DF_MEMORY)) && cfg.mem_tracker && cfg.mem_tracker->initialized) {
		std::lock_guard<std::mutex> lock(cfg.mem_tracker->mtx);
		node->next=cfg.mem_tracker->head;
		if (cfg.mem_tracker->head) cfg.mem_tracker->head->prev=node;
		cfg.mem_tracker->head=node;
		cfg.mem_tracker->total_bytes.fetch_add(size,std::memory_order_relaxed);
	}
	return static_cast<char*>(raw)+sizeof(memory_trace);
}

inline void tracked_free(debug_config& cfg,void* ptr) {
	if (!ptr) return;
	void* raw=static_cast<char*>(ptr)-sizeof(memory_trace);
	auto* node=static_cast<memory_trace*>(raw);
	if ((cfg.features.contains(DF_MEMORY)) && cfg.mem_tracker && cfg.mem_tracker->initialized) {
		std::lock_guard<std::mutex> lock(cfg.mem_tracker->mtx);
		if (node->prev) node->prev->next=node->next;
		else cfg.mem_tracker->head=node->next;
		if (node->next) node->next->prev=node->prev;
		cfg.mem_tracker->total_bytes.fetch_sub(node->bytes,std::memory_order_relaxed);
	}
	std::free(raw);
}

inline void trace_memory(debug_config& cfg) {
	if (!cfg.mem_tracker || !cfg.mem_tracker->initialized) {
		log_string(cfg,"[trace_memory] Memory tracker not initialized.\n");
		return;
	}
	std::lock_guard<std::mutex> lock(cfg.mem_tracker->mtx);
	if (!cfg.mem_tracker->head) {
		log_string(cfg,"[trace_memory] No live allocations.\n");
		return;
	}
	char header[128];
	std::snprintf(header,sizeof(header),"[trace_memory] Live allocations (total %zu bytes):\n",cfg.mem_tracker->total_bytes.load(std::memory_order_relaxed));
	log_string(cfg,header);
	for (memory_trace* n=cfg.mem_tracker->head;;n=n->next) log_string(cfg,n->to_string());
}

inline void trace_memory() {
	trace_memory(default_config());
}

}

#if _STDEX_GNU_COMPILER || _STDEX_CLANG_COMPILER || _STDEX_MSVC_COMPILER
#define _STDEX_MACHINE_DEBUG_FUNC __FUNCTION__
#else
#define _STDEX_MACHINE_DEBUG_FUNC ""
#endif

#if _STDEX_WINDOWS_PLATFORM
#define _STDEX_MACHINE_DEBUG_BREAK() __debugbreak()
#elif _STDEX_GNU_COMPILER || _STDEX_CLANG_COMPILER
#define _STDEX_MACHINE_DEBUG_BREAK() __builtin_trap()
#else
#define _STDEX_MACHINE_DEBUG_BREAK() do { } while (false)
#endif

#if defined(_DEBUG) || defined(_STDEX_MACHINE_DEBUG_DEBUGON)
#define _STDEX_MACHINE_DEBUG_ASSERT(cfg,cond,...) \
	do { \
		if (!bool(cond)) { \
			stdex::machine::debug::assert_failed( \
				cfg,"" #cond, \
				__FILE__,__LINE__,_STDEX_MACHINE_DEBUG_FUNC, \
				"" __VA_ARGS__); \
			if (stdex::machine::debug::is_debugger_present()) { \
				_STDEX_MACHINE_DEBUG_BREAK(); \
			} \
			stdex::machine::debug::trace_memory(cfg); \
		} \
	} while (false)

#define _STDEX_MACHINE_DEBUG_ASSERT_DEFAULT(cond,...) \
	do { \
		if (!bool(cond)) { \
			stdex::machine::debug::assert_failed( \
				stdex::machine::debug::default_config(),"" #cond, \
				__FILE__,__LINE__,_STDEX_MACHINE_DEBUG_FUNC, \
				"" __VA_ARGS__); \
			if (stdex::machine::debug::is_debugger_present()) { \
				_STDEX_MACHINE_DEBUG_BREAK(); \
			} \
			stdex::machine::debug::trace_memory(); \
		} \
	} while (false)

#define _STDEX_MACHINE_DEBUG_FAIL(cfg,fmt,...) \
	stdex::machine::debug::assert_failed( \
		cfg,"", \
		__FILE__,__LINE__,_STDEX_MACHINE_DEBUG_FUNC, \
		fmt,##__VA_ARGS__)

#define _STDEX_MACHINE_DEBUG_FAIL_DEFAULT(fmt,...) \
	stdex::machine::debug::assert_failed( \
		stdex::machine::default_config(),"", \
		__FILE__,__LINE__,_STDEX_MACHINE_DEBUG_FUNC, \
		fmt,##__VA_ARGS__)

#define _STDEX_MACHINE_DEBUG_LOG(cfg,level,fmt,...) \
	stdex::machine::debug::log( \
		cfg,stdex::machine::level, \
		__FILE__,__LINE__,_STDEX_MACHINE_DEBUG_FUNC, \
		fmt,##__VA_ARGS__)

#define _STDEX_MACHINE_DEBUG_LOG_DEFAULT(level,fmt,...) \
	stdex::machine::debug::log( \
		stdex::machine::default_config(), \
		stdex::machine::level, \
		__FILE__,__LINE__,_STDEX_MACHINE_DEBUG_FUNC, \
		fmt,##__VA_ARGS__)

#define _STDEX_MACHINE_DEBUG_HESITATION_BRACKET(buf,threshold_ms,fmt, ...) \
	stdex::machine::hesitation_bracket \
		_stdex_hes_bracket_##__LINE__(buf,threshold_ms,fmt,##__VA_ARGS__)

#define _STDEX_MACHINE_DEBUG_TRACE_MEMORY(cfg) \
	stdex::machine::debug::trace_memory(cfg)

#define _STDEX_MACHINE_DEBUG_TRACE_MEMORY_DEFAULT() \
	stdex::machine::debug::trace_memory()

#define _STDEX_MACHINE_DEBUG_TRACKED_MALLOC(cfg,size) \
	stdex::machine::debug::tracked_malloc( \
		cfg,static_cast<std::size_t>(size), \
		__FILE__,__LINE__,_STDEX_MACHINE_DEBUG_FUNC)

#define _STDEX_MACHINE_DEBUG_TRACKED_MALLOC_DEFAULT(size) \
	stdex::machine::debug::tracked_malloc( \
		stdex::machine::default_config(), \
		static_cast<std::size_t>(size), \
		__FILE__,__LINE__,_STDEX_MACHINE_DEBUG_FUNC)

#define _STDEX_MACHINE_DEBUG_TRACKED_FREE(cfg,ptr) \
	stdex::machine::debug::tracked_free(cfg,ptr)

#define _STDEX_MACHINE_DEBUG_TRACKED_FREE_DEFAULT(ptr) \
	stdex::machine::debug::tracked_free(stdex::machine::default_config(),ptr)
#else 
#define _STDEX_MACHINE_DEBUG_ASSERT(cfg,cond,...) \
	do { (void)sizeof(bool(cond)); } while (false)
#define _STDEX_MACHINE_DEBUG_ASSERT_DEFAULT(cond,...) \
	do { (void)sizeof(bool(cond)); } while (false)
#define _STDEX_MACHINE_DEBUG_FAIL(cfg,fmt,...) do { } while (false)
#define _STDEX_MACHINE_DEBUG_FAIL_DEFAULT(fmt,...) do { } while (false)
#define _STDEX_MACHINE_DEBUG_LOG(cfg,level,fmt,...) do { } while (false)
#define _STDEX_MACHINE_DEBUG_LOG_DEFAULT(level,fmt,...) do { } while (false)
#define _STDEX_MACHINE_DEBUG_HESITATION_BRACKET(buf,ms,fmt,...) do { } while (false)
#define _STDEX_MACHINE_DEBUG_TRACE_MEMORY(cfg) do { } while (false)
#define _STDEX_MACHINE_DEBUG_TRACE_MEMORY_DEFAULT() do { } while (false)
#define _STDEX_MACHINE_DEBUG_TRACKED_MALLOC(cfg,size) \
	stdex::machine::debug::safe_malloc(static_cast<std::size_t>(size))
#define _STDEX_MACHINE_DEBUG_TRACKED_MALLOC_DEFAULT(size) \
	stdex::machine::debug::safe_malloc(static_cast<std::size_t>(size))
#define _STDEX_MACHINE_DEBUG_TRACKED_FREE(cfg, ptr) \
	stdex::machine::debug::safe_free(ptr)
#define _STDEX_MACHINE_DEBUG_TRACKED_FREE_DEFAULT(ptr) \
	stdex::machine::debug::safe_free(ptr)
#endif

}

}

#endif