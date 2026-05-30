//Last Modified At 2026/05/29
//@Version 2.1.0.1
#ifndef _STDEX_MACHINE_SEHCATCHER_H_
#define _STDEX_MACHINE_SEHCATCHER_H_ 1

#include <chrono>
#include <csetjmp>
#include <csignal>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "../bitwise/flags.h"//At Least 1.1

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
#if defined(__linux__) && !defined(__ANDROID__)
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
#ifndef _STDEX_IOS_PLATFORM
#if _STDEX_APPLE_PLATFORM
#include <TargetConditionals.h>
#if TARGET_OS_IPHONE
#define _STDEX_IOS_PLATFORM 1
#else
#define _STDEX_IOS_PLATFORM 0
#endif
#else
#define _STDEX_IOS_PLATFORM 0
#endif
#endif
#ifndef _STDEX_MACOS_PLATFORM
#if _STDEX_APPLE_PLATFORM
#include <TargetConditionals.h>
#if TARGET_OS_OSX
#define _STDEX_MACOS_PLATFORM 1
#else
#define _STDEX_MACOS_PLATFORM 0
#endif
#else
#define _STDEX_MACOS_PLATFORM 0
#endif
#endif

#if _STDEX_WINDOWS_PLATFORM
	extern "C" {
		#include <dbghelp.h>
	}
	#include <TlHelp32.h>
	#include <windows.h>
	#pragma comment(lib,"dbghelp.lib")
	#pragma comment(lib,"version.lib")
#elif _STDEX_APPLE_PLATFORM
	#include <TargetConditionals.h>
	#include <dlfcn.h>
	#include <execinfo.h>
	#include <mach/mach.h>
	#include <mach-o/dyld.h>
	#include <mach-o/loader.h>
	#include <setjmp.h>
	#include <signal.h>
	#include <sys/types.h>
	#include <sys/utsname.h>
	#include <ucontext.h>
	#include <unistd.h>
#elif _STDEX_ANDROID_PLATFORM
	#include <android/log.h>
	#include <dlfcn.h>
	#include <elf.h>
	#if __has_include(<execinfo.h>)
		#include <execinfo.h>
	#endif
	#include <fcntl.h>
	#include <link.h>
	#include <setjmp.h>
	#include <signal.h>
	#include <sys/prctl.h>
	#include <sys/resource.h>
	#include <sys/types.h>
	#include <sys/utsname.h>
	#include <ucontext.h>
	#include <unistd.h>
	#include <unwind.h>
#elif _STDEX_LINUX_PLATFORM
	#include <dlfcn.h>
	#include <execinfo.h>
	#include <fcntl.h>
	#include <link.h>
	#include <setjmp.h>
	#include <signal.h>
	#include <sys/prctl.h>
	#include <sys/resource.h>
	#include <sys/types.h>
	#include <sys/utsname.h>
	#include <ucontext.h>
	#include <unistd.h>
#endif

namespace stdex {

namespace machine {

enum seh_feature {
	SF_NONE=0,
	SF_OUTPUT=1<<0,
	SF_LOGGING=1<<1,
	SF_DUMP=1<<2,
	SF_TRACE=1<<3,
	SF_RECOVERY=1<<4,
	SF_SYMBOLS=1<<5,
	SF_MODULES=1<<6,
	SF_SKIP=1<<7,
	SF_CLEANUP=1<<8,
	SF_ASYNC_SAFE=1<<9,
	SF_ALL=~0u,
};

struct module_info {
	std::string name;
	uintptr_t base_address;
	uintptr_t size;
	std::string version;
	std::string to_string() const {
		std::ostringstream oss;
		oss<<"  "<<name<<" @ 0x"<<std::hex<<std::uppercase<<std::setfill('0')<<std::setw(sizeof(uintptr_t)*2)<<base_address<<" size=0x"<<std::hex<<std::uppercase<<std::setfill('0')<<size;
		if (!version.empty()) oss<<" ver="<<version;
		return oss.str();
	}
};

struct stack_frame {
	void* address;
	std::string symbol;
	std::string module;
	uintptr_t offset;
	std::string source_file;
	uint32_t source_line;
	std::string to_string() const {
		std::ostringstream oss;
		oss<<"  [0x"<<std::hex<<std::uppercase<<std::setfill('0')<<std::setw(sizeof(void*)*2)<<reinterpret_cast<uintptr_t>(address)<<"]";
		if (!module.empty()) oss<<" "<<module;
		if (!symbol.empty()) oss<<"!"<<symbol;
		if (offset) oss<<"+0x"<<std::hex<<offset;
		if (!source_file.empty()) oss<<" ("<<source_file<<":"<<std::dec<<source_line<<")";
		return oss.str();
	}
};

struct exception_infos {
	uintptr_t code;
	void* fault_address;
	std::vector<std::pair<std::string,uintptr_t>> registers;
	std::string system_info;
	std::string error_message;
	std::string error_detail;
	std::vector<stack_frame> stack_trace;
	std::vector<module_info> modules;
	uint32_t process_id;
	uint32_t thread_id;
	std::string timestamp;
	bool is_supported_platform;

	exception_infos() : code(0) , fault_address(nullptr) , process_id(0) , thread_id(0) , is_supported_platform(false){ }

	std::string to_string(bool show_modules=true,bool show_stack=true,bool show_registers=true) const {
		std::ostringstream oss;
		if (!is_supported_platform) {
			oss<<"[seh_catcher] Unsupported platform - no exception information available.\n";
			return oss.str();
		}
		oss<<"Timestamp:"<<timestamp<<"\n";
		oss<<"System Info:"<<system_info<<"\n";
		oss<<"PID:"<<std::dec<<process_id<<"\n";
		oss<<"TID:"<<std::dec<<thread_id<<"\n";
		oss<<"Error ["<<std::hex<<std::uppercase<<code<<"]:"<<error_message<<"\n";
		if (!error_detail.empty()) oss<<"Detail:"<<error_detail<<"\n";
		oss<<"Fault Address:"<<std::hex<<std::uppercase<<"0x"<<std::setfill('0')<<std::setw(sizeof(void*)*2)<<reinterpret_cast<uintptr_t>(fault_address)<<"\n";
		if (show_registers && !registers.empty()) {
			oss<<"Registers:\n";
			std::size_t col=0;
			for (const auto& [name,value]:registers) {
				oss<<"  "<<std::left<<std::setfill(' ')<<std::setw(8)<<name<<": 0x"<<std::right<<std::setfill('0')<<std::hex<<std::uppercase<<std::setw(sizeof(uintptr_t)*2)<<value<<std::setfill(' ')<<std::dec<<std::right;
				if (++col%3==0) oss<<"\n";
				else oss<<"  ";
			}
			if (col%3!=0) oss<<"\n";
		}
		if (show_stack && !stack_trace.empty()) {
			oss<<"Stack Trace ("<<std::dec<<stack_trace.size()<<" frames):\n";
			for (std::size_t i=0;i<stack_trace.size();i++) oss<<"  #"<<std::dec<<std::setw(3)<<std::setfill('0')<<i<<" "<<stack_trace[i].to_string().substr(2)<<"\n";
		}
		if (show_modules && !modules.empty()) {
			oss<<"Loaded Modules ("<<std::dec<<modules.size()<<"):\n";
			for (const auto& m:modules) oss<<m.to_string()<<"\n";
		}
		return oss.str();
	}
};



class exception_handler {
public:
	using handler_t=std::function<bool(exception_infos&)>;
	using recovery_t=std::function<bool(exception_infos&)>;
	using cleanup_t=std::function<bool(exception_infos&)>;

private:
	stdex::bitwise::flags<seh_feature> features_;
	std::string log_path_;
	std::string dump_path_;
	handler_t user_handler_;
	recovery_t recovery_handler_;
	cleanup_t cleanup_handler_;

	exception_infos last_exception_;
	volatile sig_atomic_t last_exception_valid_=0;

	#if _STDEX_WINDOWS_PLATFORM
		static EXCEPTION_POINTERS* current_exception_;
	#elif _STDEX_APPLE_PLATFORM || _STDEX_ANDROID_PLATFORM || _STDEX_LINUX_PLATFORM
		struct sigaction prev_actions_[6];
		void* alt_stack_mem_=nullptr;
		static void* current_ucontext_;
		static volatile sig_atomic_t last_signal_;
		static siginfo_t last_siginfo_;
		static ucontext_t last_ucontext_storage_;
	#endif

	static std::string make_timestamp() {
		const auto now=std::chrono::system_clock::now();
		const std::time_t t=std::chrono::system_clock::to_time_t(now);
				struct tm tmbuf;
		#if _STDEX_WINDOWS_PLATFORM
				localtime_s(&tmbuf,&t);
		#else
				localtime_r(&t,&tmbuf);
		#endif
				char buf[32];
				std::size_t n=std::strftime(buf,sizeof(buf),"%Y-%m-%d %H:%M:%S",&tmbuf);
				return std::string(buf,n);
	}
	bool default_handler(exception_infos& info,bool* windows_recovery=nullptr) {
		bool should_recover=false;
		if (info.timestamp.empty()) info.timestamp=make_timestamp();
		if (info.process_id==0) {
			#if _STDEX_WINDOWS_PLATFORM
				info.process_id=static_cast<uint32_t>(GetCurrentProcessId());
				info.thread_id=static_cast<uint32_t>(GetCurrentThreadId());
			#elif _STDEX_APPLE_PLATFORM || _STDEX_ANDROID_PLATFORM || _STDEX_LINUX_PLATFORM
				info.process_id=static_cast<uint32_t>(getpid());
				#if _STDEX_APPLE_PLATFORM
					uint64_t tid=0;
					pthread_threadid_np(nullptr,&tid);
					info.thread_id=static_cast<uint32_t>(tid);
				#elif _STDEX_ANDROID_PLATFORM
					info.thread_id=static_cast<uint32_t>(gettid());
				#elif _STDEX_LINUX_PLATFORM
					info.thread_id=static_cast<uint32_t>(syscall(SYS_gettid));
				#endif
			#endif
		}
		if (info.stack_trace.empty() && features_.contains(SF_TRACE)) collect_stack_trace(info);
		if (info.modules.empty() && features_.contains(SF_MODULES)) collect_modules(info);
		if (features_.contains(SF_OUTPUT)) {
			std::cerr<<"[seh_catcher] Crash detected:\n"<<info.to_string()<<"\n";
			#if _STDEX_ANDROID_PLATFORM
				#ifdef _STDEX_SEH_USE_DYNAMIC_LOG
					using android_log_print_fn=int(*)(int,const char*,const char*,...);
					static android_log_print_fn s_log_print=[]()->android_log_print_fn {
						void* h=dlopen("liblog.so",RTLD_NOW|RTLD_LOCAL);
						if (!h) return nullptr;
						return reinterpret_cast<android_log_print_fn>(dlsym(h,"__android_log_print"));
					}();
					if (s_log_print) s_log_print(ANDROID_LOG_FATAL,"seh_catcher","%s",info.to_string().c_str());
				#else
					__android_log_print(ANDROID_LOG_FATAL,"seh_catcher","%s",info.to_string().c_str());
				#endif
			#endif
		}
		if (features_.contains(SF_LOGGING)) {
			std::ofstream log(log_path_,std::ios::app);
			if (log) log<<"Crash at "<<info.timestamp<<"\n"<<info.to_string()<<"\n"<<"========================================\n";
		}
		if (features_.contains(SF_DUMP)) {
			bool dumped=write_dump(info);
			if (!dumped && features_.contains(SF_OUTPUT)) std::cerr<<"[seh_catcher] dump file not written; see error_detail for reason\n";
		}
		if (features_.contains(SF_CLEANUP)) {
			if (cleanup_handler_) cleanup_handler_(info);
		} else if (features_.contains(SF_RECOVERY)) {
			if (recovery_point_valid) {
				bool do_recover=true;
				if (recovery_handler_) do_recover=recovery_handler_(info);
				if (do_recover) {
					if (windows_recovery) *windows_recovery=true;
					should_recover=true;
					#if _STDEX_APPLE_PLATFORM || _STDEX_ANDROID_PLATFORM || _STDEX_LINUX_PLATFORM
						siglongjmp(recovery_point,1);
					#endif
				}
			}
		} else if (features_.contains(SF_SKIP)) {
			if (windows_recovery) *windows_recovery=false;
			should_recover=basic_recovery(info);
		}
		return should_recover;
	}

	void collect_stack_trace(exception_infos& info) {
		info.stack_trace.clear();
		#if _STDEX_WINDOWS_PLATFORM
			collect_stack_trace_windows(info);
		#elif _STDEX_APPLE_PLATFORM || _STDEX_LINUX_PLATFORM
			collect_stack_trace_posix(info);
		#elif _STDEX_ANDROID_PLATFORM
			collect_stack_trace_android(info);
		#endif
	}

	#if _STDEX_WINDOWS_PLATFORM
		void collect_stack_trace_windows(exception_infos& info) {
			const HANDLE proc=GetCurrentProcess();
			SymInitialize(proc,nullptr,TRUE);
			SymSetOptions(SYMOPT_LOAD_LINES|SYMOPT_UNDNAME|SYMOPT_DEFERRED_LOADS);
			std::vector<void*> raw(stack_length);
			const USHORT frames=CaptureStackBackTrace(0,static_cast<DWORD>(stack_length),raw.data(),nullptr);
			const std::size_t sym_buf_size=sizeof(SYMBOL_INFO)+MAX_SYM_NAME*sizeof(TCHAR);
			std::vector<uint8_t> sym_buf(sym_buf_size,0);
			SYMBOL_INFO* sym=reinterpret_cast<SYMBOL_INFO*>(sym_buf.data());
			sym->SizeOfStruct=sizeof(SYMBOL_INFO);
			sym->MaxNameLen=MAX_SYM_NAME;
			IMAGEHLP_LINE64 line={};
			line.SizeOfStruct=sizeof(IMAGEHLP_LINE64);
			DWORD line_disp=0;
			IMAGEHLP_MODULE64 mod={};
			mod.SizeOfStruct=sizeof(IMAGEHLP_MODULE64);
			for (USHORT i=0;i<frames;i++) {
				stack_frame sf={};
				sf.address=raw[i];
				const DWORD64 addr=reinterpret_cast<DWORD64>(raw[i]);
				DWORD64 sym_disp=0;
				if (features_.contains(SF_SYMBOLS)) {
					if (SymFromAddr(proc,addr,&sym_disp,sym)) {
						sf.symbol=sym->Name;
						sf.offset=static_cast<uintptr_t>(sym_disp);
					}
					if (SymGetLineFromAddr64(proc,addr,&line_disp,&line)) {
						sf.source_file=line.FileName?line.FileName:"";
						sf.source_line=line.LineNumber;
					}
					if (SymGetModuleInfo64(proc,addr,&mod)) sf.module=mod.ModuleName;
				}
				info.stack_trace.push_back(std::move(sf));
			}
			SymCleanup(proc);
		}
	#endif

	#if _STDEX_APPLE_PLATFORM || _STDEX_LINUX_PLATFORM
		void collect_stack_trace_posix(exception_infos& info) {
			std::vector<void*> raw(stack_length);
			const int frames=backtrace(raw.data(),static_cast<int>(stack_length));
			char** syms=nullptr;
			if (features_.contains(SF_SYMBOLS)) syms=backtrace_symbols(raw.data(),frames);
			for (int i=0;i<frames;i++) {
				stack_frame sf={};
				sf.address=raw[i];
				if (syms && syms[i]) {
					sf.symbol=syms[i];
					#if _STDEX_APPLE_PLATFORM || _STDEX_LINUX_PLATFORM
						Dl_info dli={};
						if (dladdr(raw[i],&dli)) {
							if (dli.dli_sname) sf.symbol=dli.dli_sname;
							if (dli.dli_fname) sf.module=dli.dli_fname;
							sf.offset=static_cast<uintptr_t>(reinterpret_cast<uintptr_t>(raw[i])-reinterpret_cast<uintptr_t>(dli.dli_saddr));
						}
					#endif
				}
				info.stack_trace.push_back(std::move(sf));
			}
			free(syms);
		}
	#endif

	#if _STDEX_ANDROID_PLATFORM
		struct android_unwind_state {
			std::vector<void*>* frames;
			std::size_t max;
		};
		static _Unwind_Reason_Code android_unwind_cb(_Unwind_Context* ctx,void* arg) {
			auto* st=reinterpret_cast<android_unwind_state*>(arg);
			uintptr_t pc=_Unwind_GetIP(ctx);
			if (pc && st->frames->size()<st->max) {
				st->frames->push_back(reinterpret_cast<void*>(pc));
				return _URC_NO_REASON;
			}
			return _URC_END_OF_STACK;
		}
		void collect_stack_trace_android(exception_infos& info) {
			std::vector<void*> raw;
			raw.reserve(stack_length);
			android_unwind_state st{&raw,stack_length};
			_Unwind_Backtrace(android_unwind_cb,&st);
			for (auto* addr:raw) {
				stack_frame sf={};
				sf.address=addr;
				Dl_info dli={};
				if (features_.contains(SF_SYMBOLS) && dladdr(addr,&dli)) {
					if (dli.dli_sname) sf.symbol=dli.dli_sname;
					if (dli.dli_fname) sf.module=dli.dli_fname;
					sf.offset=static_cast<uintptr_t>(reinterpret_cast<uintptr_t>(addr)-reinterpret_cast<uintptr_t>(dli.dli_saddr));
				}
				info.stack_trace.push_back(std::move(sf));
			}
		}
	#endif

	void collect_modules(exception_infos& info) {
		info.modules.clear();
		#if _STDEX_WINDOWS_PLATFORM
			collect_modules_windows(info);
		#elif _STDEX_APPLE_PLATFORM
			collect_modules_apple(info);
		#elif _STDEX_ANDROID_PLATFORM || _STDEX_LINUX_PLATFORM
			collect_modules_linux(info);
		#endif
	}

	#if _STDEX_WINDOWS_PLATFORM
		void collect_modules_windows(exception_infos& info) {
			HANDLE snap=CreateToolhelp32Snapshot(TH32CS_SNAPMODULE|TH32CS_SNAPMODULE32,0);
			if (snap==INVALID_HANDLE_VALUE) return;
			MODULEENTRY32 me={};
			me.dwSize=sizeof(me);
			if (Module32First(snap,&me)) {
				do {
					module_info mi={};
					mi.name=me.szModule;
					mi.base_address=reinterpret_cast<uintptr_t>(me.modBaseAddr);
					mi.size=me.modBaseSize;
					char path[MAX_PATH];
					if (GetModuleFileNameA(me.hModule,path,MAX_PATH)) {
						DWORD dummy=0;
						DWORD vs=GetFileVersionInfoSizeA(path,&dummy);
						if (vs) {
							std::vector<uint8_t> vbuf(vs);
							if (GetFileVersionInfoA(path,0,vs,vbuf.data())) {
								VS_FIXEDFILEINFO* ffi=nullptr;
								UINT ffi_size=0;
								if (VerQueryValueA(vbuf.data(),"\\",reinterpret_cast<void**>(&ffi),&ffi_size) && ffi) {
									std::ostringstream vs2;
									vs2<<HIWORD(ffi->dwFileVersionMS)<<"."<<LOWORD(ffi->dwFileVersionMS)<<"."<<HIWORD(ffi->dwFileVersionLS)<<"."<<LOWORD(ffi->dwFileVersionLS);
									mi.version=vs2.str();
								}
							}
						}
					}
					info.modules.push_back(std::move(mi));
				} while (Module32Next(snap,&me));
			}
			CloseHandle(snap);
		}
	#endif

	#if _STDEX_APPLE_PLATFORM
		void collect_modules_apple(exception_infos& info) {
			const uint32_t count=_dyld_image_count();
			for (uint32_t i=0;i<count;i++) {
				module_info mi={};
				const char* name=_dyld_get_image_name(i);
				mi.name=name?name:"";
				const struct mach_header* hdr=_dyld_get_image_header(i);
				intptr_t slide=_dyld_get_image_vmaddr_slide(i);
				mi.base_address=reinterpret_cast<uintptr_t>(hdr);
				if (hdr) {
					uintptr_t total=0;
					const bool is64=(hdr->magic==MH_MAGIC_64 || hdr->magic==MH_CIGAM_64);
					const uint8_t* p=reinterpret_cast<const uint8_t*>(hdr)+(is64?sizeof(mach_header_64):sizeof(mach_header));
					for (uint32_t j=0;j<hdr->ncmds;j++) {
						const struct load_command* lc=reinterpret_cast<const load_command*>(p);
						if (lc->cmd==LC_SEGMENT_64) total+=reinterpret_cast<const segment_command_64*>(lc)->vmsize;
						else if (lc->cmd==LC_SEGMENT) total+=reinterpret_cast<const segment_command*>(lc)->vmsize;
						p+=lc->cmdsize;
					}
					mi.size=total;
				}
				(void)slide;
				info.modules.push_back(std::move(mi));
			}
		}
	#endif

	#if _STDEX_ANDROID_PLATFORM || _STDEX_LINUX_PLATFORM
		struct dl_iterate_arg {
			std::vector<module_info>* modules;
		};
		static int dl_iterate_cb(::dl_phdr_info* info,std::size_t /*size*/,void* data) {
			auto* arg=reinterpret_cast<dl_iterate_arg*>(data);
			module_info mi={};
			mi.name=info->dlpi_name?info->dlpi_name:"";
			mi.base_address=static_cast<uintptr_t>(info->dlpi_addr);
			uintptr_t max_va=0;
			uintptr_t min_va=UINTPTR_MAX;
			for (int i=0;i<info->dlpi_phnum;i++) {
				const auto& ph=info->dlpi_phdr[i];
				if (ph.p_type!=PT_LOAD) continue;
				uintptr_t s=static_cast<uintptr_t>(ph.p_vaddr);
				uintptr_t e=s+static_cast<uintptr_t>(ph.p_memsz);
				if (s<min_va) min_va=s;
				if (e>max_va) max_va=e;
			}
			mi.size=(max_va>min_va)?(max_va-min_va):0;
			arg->modules->push_back(std::move(mi));
			return 0;
		}
		void collect_modules_linux(exception_infos& info) {
			dl_iterate_arg arg{&info.modules};
			dl_iterate_phdr(dl_iterate_cb,&arg);
		}
	#endif

	bool write_dump(exception_infos& info) {
		#if _STDEX_WINDOWS_PLATFORM
			if (!current_exception_) {
				info.error_detail+="\n[write_dump] no active exception";
				return false;
			}
			HANDLE file=CreateFileA(dump_path_.c_str(),GENERIC_WRITE,0,nullptr,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,nullptr);
			if (file==INVALID_HANDLE_VALUE) {
				info.error_detail+="\n[write_dump] CreateFileA failed, GLE=";
				info.error_detail+=std::to_string(GetLastError());
				return false;
			}
			MINIDUMP_EXCEPTION_INFORMATION mei={};
			mei.ThreadId=GetCurrentThreadId();
			mei.ExceptionPointers=current_exception_;
			mei.ClientPointers=FALSE;
			BOOL ok=MiniDumpWriteDump(GetCurrentProcess(),GetCurrentProcessId(),file,static_cast<MINIDUMP_TYPE>(MiniDumpWithFullMemoryInfo|MiniDumpWithHandleData|MiniDumpWithUnloadedModules|MiniDumpWithThreadInfo),&mei,nullptr,nullptr);
			CloseHandle(file);
			if (!ok) {
				info.error_detail+="\n[write_dump] MiniDumpWriteDump failed, GLE=";
				info.error_detail+=std::to_string(GetLastError());
				return false;
			}
			return true;
		#elif _STDEX_ANDROID_PLATFORM || _STDEX_LINUX_PLATFORM
			struct rlimit rl={RLIM_INFINITY,RLIM_INFINITY};
			if (setrlimit(RLIMIT_CORE, &rl)!=0) {
				info.error_detail+="\n[write_dump] setrlimit(RLIMIT_CORE) failed: errno=";
				info.error_detail+=std::to_string(errno);
				return false;
			}
			if (prctl(PR_SET_DUMPABLE,1,0,0,0)!=0) {
				info.error_detail+="\n[write_dump] prctl(PR_SET_DUMPABLE) failed: errno=";
				info.error_detail+=std::to_string(errno);
			}
			if (!dump_path_.empty()) {
				int fd=::open("/proc/sys/kernel/core_pattern",O_WRONLY);
				if (fd>=0) {
					ssize_t w=::write(fd, dump_path_.c_str(),dump_path_.size());
					::close(fd);
					if (w<0 || static_cast<std::size_t>(w)!=dump_path_.size()) {
						info.error_detail+="\n[write_dump] writing core_pattern failed: errno=";
						info.error_detail+=std::to_string(errno);
						info.error_detail+=" (core will be written per existing core_pattern)";
						return false;
					}
					return true;
				} else {
					info.error_detail+="\n[write_dump] cannot write /proc/sys/kernel/core_pattern (errno=";
					info.error_detail+=std::to_string(errno);
					info.error_detail+="); CAP_SYS_ADMIN required. ";
					info.error_detail+="Core dump will be written to system default location.";
					return false;
				}
			}
			return true;
		#elif _STDEX_APPLE_PLATFORM
			info.error_detail+="\n[write_dump] custom dump path not supported on Apple platforms. ";
			info.error_detail+="Crash reports are written by the OS to ";
			#if _STDEX_IOS_PLATFORM
				info.error_detail+="the device's crash log (retrieve via Xcode Organizer or sysdiagnose).";
			#else
				info.error_detail+="~/Library/Logs/DiagnosticReports/ .";
			#endif
			(void)dump_path_;
			return false;
		#else
			info.error_detail+="\n[write_dump] not supported on this platform";
			(void)dump_path_;
			(void)info;
			return false;
		#endif
	}

	bool basic_recovery(exception_infos& info) {
		#if _STDEX_WINDOWS_PLATFORM
			(void)info;
			return false;
		#elif _STDEX_APPLE_PLATFORM || _STDEX_ANDROID_PLATFORM || _STDEX_LINUX_PLATFORM
			if (!current_ucontext_) return false;
			ucontext_t* uc=static_cast<ucontext_t*>(current_ucontext_);
			#if defined(__x86_64__)
				#if _STDEX_APPLE_PLATFORM
					uc->uc_mcontext->__ss.__rip+=2;
				#else
					uc->uc_mcontext.gregs[REG_RIP]+=2;
				#endif
				return true;
			#elif defined(__i386__)
				#if _STDEX_APPLE_PLATFORM
					uc->uc_mcontext->__ss.__eip+=2;
				#else
					uc->uc_mcontext.gregs[REG_EIP]+=2;
				#endif
				return true;
			#elif defined(__aarch64__)
				#if _STDEX_APPLE_PLATFORM
					uc->uc_mcontext->__ss.__pc+=4;
				#else
					uc->uc_mcontext.pc+=4;
				#endif
				return true;
			#elif defined(__arm__)
				#if _STDEX_APPLE_PLATFORM
					uc->uc_mcontext->__ss.__pc+=4;
				#else
					uc->uc_mcontext.arm_pc+=4;
				#endif
				return true;
			#else
				(void)uc;
				return false;
			#endif
		#else
			(void)info;
			return false;
		#endif
	}

	#if _STDEX_WINDOWS_PLATFORM
		LPTOP_LEVEL_EXCEPTION_FILTER prev_filter_=nullptr;

		static LONG WINAPI seh_filter(EXCEPTION_POINTERS* exptrs) {
			auto& self=instance();
			current_exception_=exptrs;
			exception_infos ex_info=collect_windows_info(exptrs);
			ex_info.timestamp=make_timestamp();
			ex_info.process_id=static_cast<uint32_t>(GetCurrentProcessId());
			ex_info.thread_id=static_cast<uint32_t>(GetCurrentThreadId());
			if (self.features_.contains(SF_TRACE)) self.collect_stack_trace(ex_info);
			if (self.features_.contains(SF_MODULES)) self.collect_modules(ex_info);
			bool should_default=true;
			bool windows_recovery=false;
			if (self.user_handler_) should_default=self.user_handler_(ex_info);
			if (should_default) self.default_handler(ex_info,&windows_recovery);
			current_exception_=nullptr;
			return windows_recovery?EXCEPTION_CONTINUE_EXECUTION:EXCEPTION_EXECUTE_HANDLER;
		}

		static exception_infos collect_windows_info(EXCEPTION_POINTERS* exptrs) {
			exception_infos info;
			info.is_supported_platform=true;
			info.code=exptrs->ExceptionRecord->ExceptionCode;
			info.fault_address=exptrs->ExceptionRecord->ExceptionAddress;
			#if defined(_M_IX86)
				info.system_info="Windows x86 32-bit";
				auto* ctx=exptrs->ContextRecord;
				info.registers={
					{"EAX",ctx->Eax},{"EBX",ctx->Ebx},{"ECX",ctx->Ecx},{"EDX",ctx->Edx},
					{"ESI",ctx->Esi},{"EDI",ctx->Edi},{"EBP",ctx->Ebp},{"ESP",ctx->Esp},
					{"EIP",ctx->Eip},{"EFLAGS",ctx->EFlags},
					{"CS",ctx->SegCs},{"DS",ctx->SegDs},{"ES",ctx->SegEs},
					{"FS",ctx->SegFs},{"GS",ctx->SegGs},{"SS",ctx->SegSs}
				};
			#elif defined(_M_X64)
				info.system_info="Windows x86-64 64-bit";
				auto* ctx=exptrs->ContextRecord;
				info.registers={
					{"RAX",ctx->Rax},{"RBX",ctx->Rbx},{"RCX",ctx->Rcx},{"RDX",ctx->Rdx},
					{"RSI",ctx->Rsi},{"RDI",ctx->Rdi},{"RBP",ctx->Rbp},{"RSP",ctx->Rsp},
					{"RIP",ctx->Rip},
					{"R8", ctx->R8}, {"R9", ctx->R9}, {"R10",ctx->R10},{"R11",ctx->R11},
					{"R12",ctx->R12},{"R13",ctx->R13},{"R14",ctx->R14},{"R15",ctx->R15},
					{"EFLAGS",ctx->EFlags},
					{"CS",ctx->SegCs},{"DS",ctx->SegDs},{"ES",ctx->SegEs},
					{"FS",ctx->SegFs},{"GS",ctx->SegGs},{"SS",ctx->SegSs}
				};
			#elif defined(_M_ARM64)
				info.system_info="Windows ARM64";
				auto* ctx=exptrs->ContextRecord;
				info.registers={
					{"X0", ctx->X0}, {"X1", ctx->X1}, {"X2", ctx->X2}, {"X3", ctx->X3},
					{"X4", ctx->X4}, {"X5", ctx->X5}, {"X6", ctx->X6}, {"X7", ctx->X7},
					{"X8", ctx->X8}, {"X9", ctx->X9}, {"X10",ctx->X10},{"X11",ctx->X11},
					{"X12",ctx->X12},{"X13",ctx->X13},{"X14",ctx->X14},{"X15",ctx->X15},
					{"X16",ctx->X16},{"X17",ctx->X17},{"X18",ctx->X18},{"X19",ctx->X19},
					{"X20",ctx->X20},{"X21",ctx->X21},{"X22",ctx->X22},{"X23",ctx->X23},
					{"X24",ctx->X24},{"X25",ctx->X25},{"X26",ctx->X26},{"X27",ctx->X27},
					{"X28",ctx->X28},{"FP", ctx->Fp}, {"LR", ctx->Lr}, {"SP", ctx->Sp},
					{"PC", ctx->Pc}, {"CPSR",ctx->Cpsr}
				};
			#else
				info.system_info="Windows (unknown arch)";
			#endif
			switch (info.code) {
				case EXCEPTION_ACCESS_VIOLATION: {
					info.error_message="Access Violation";
					if (exptrs->ExceptionRecord->NumberParameters>=2) {
						std::ostringstream det;
						det<<(exptrs->ExceptionRecord->ExceptionInformation[0]?"Write":"Read")<<" at 0x"<<std::hex<<std::uppercase<<std::setfill('0')<<std::setw(sizeof(ULONG_PTR)*2)<<exptrs->ExceptionRecord->ExceptionInformation[1];
						info.error_detail=det.str();
					}
					break;
				}
				case EXCEPTION_STACK_OVERFLOW: {
					info.error_message="Stack Overflow";
					break;
				}
				case EXCEPTION_INT_DIVIDE_BY_ZERO: {
					info.error_message="Integer Divide by Zero";
					break;
				}
				case EXCEPTION_INT_OVERFLOW: {
					info.error_message="Integer Overflow";
					break;
				}
				case EXCEPTION_FLT_DIVIDE_BY_ZERO: {
					info.error_message="FP Divide by Zero";
					break;
				}
				case EXCEPTION_FLT_OVERFLOW: {
					info.error_message="FP Overflow";
					break;
				}
				case EXCEPTION_FLT_UNDERFLOW: {
					info.error_message="FP Underflow";
					break;
				}
				case EXCEPTION_FLT_INVALID_OPERATION: {
					info.error_message="FP Invalid Operation";
					break;
				}
				case EXCEPTION_FLT_DENORMAL_OPERAND: {
					info.error_message="FP Denormal Operand";
					break;
				}
				case EXCEPTION_FLT_INEXACT_RESULT: {
					info.error_message="FP Inexact Result";
					break;
				}
				case EXCEPTION_FLT_STACK_CHECK: {
					info.error_message="FP Stack Check";
					break;
				}
				case EXCEPTION_ILLEGAL_INSTRUCTION: {
					info.error_message="Illegal Instruction";
					break;
				}
				case EXCEPTION_PRIV_INSTRUCTION: {
					info.error_message="Privileged Instruction";
					break;
				}
				case EXCEPTION_IN_PAGE_ERROR: {
					info.error_message="In-Page Error";
					break;
				}
				case EXCEPTION_DATATYPE_MISALIGNMENT: {
					info.error_message="Datatype Misalignment";
					break;
				}
				case EXCEPTION_ARRAY_BOUNDS_EXCEEDED: {
					info.error_message="Array Bounds Exceeded";
					break;
				}
				case EXCEPTION_NONCONTINUABLE_EXCEPTION: {
					info.error_message="Noncontinuable Exception";
					break;
				}
				case EXCEPTION_INVALID_DISPOSITION: {
					info.error_message="Invalid Disposition";
					break;
				}
				case EXCEPTION_GUARD_PAGE: {
					info.error_message="Guard Page Violation";
					break;
				}
				case EXCEPTION_INVALID_HANDLE: {
					info.error_message="Invalid Handle";
					break;
				}
				case 0xE06D7363: {
					info.error_message="C++ Exception (0xE06D7363)";
					break;
				}
				default: {
					std::ostringstream oss;
					oss<<"Unknown Exception (0x"<<std::hex<<std::uppercase<<info.code<<")";
					info.error_message=oss.str();
					break;
				}
			}
			return info;
		}
	#endif

	#if _STDEX_APPLE_PLATFORM || _STDEX_ANDROID_PLATFORM || _STDEX_LINUX_PLATFORM
		static void signal_handler(int sig,siginfo_t* si,void* ucontext) {
			auto& self=instance();
			if (self.features_.contains(SF_ASYNC_SAFE)) {
				last_signal_=sig;
				if (si) std::memcpy(&last_siginfo_,si,sizeof(siginfo_t));
				if (ucontext) {
					std::memcpy(&last_ucontext_storage_,ucontext,sizeof(ucontext_t));
					current_ucontext_=&last_ucontext_storage_;
				} else current_ucontext_=nullptr;
				if (self.features_.contains(SF_RECOVERY) && self.recovery_point_valid) {
					self.recovery_point_valid=0;
					siglongjmp(self.recovery_point,1);
				}
				/*{
					static const char prefix[]="[seh_catcher] fatal signal ";
					(void)!write(STDERR_FILENO,prefix,sizeof(prefix)-1);
					char buf[16];
					int n=0;
					int s=sig;
					if (s==0) buf[n++]='0';
					else {
						char tmp[16];
						int k=0;
						while (s>0) {
							tmp[k++]=char('0'+(s%10));
							s/=10;
						}
						while (k>0) buf[n++]=tmp[--k];
					}
					buf[n++]='\n';
					(void)!write(STDERR_FILENO,buf,n);
				}*/
				int idx=-1;
				switch (sig) {
					case SIGSEGV: {
						idx=0;
						break;
					}
					case SIGBUS: {
						idx=1;
						break;
					}
					case SIGFPE: {
						idx=2;
						break;
					}
					case SIGILL: {
						idx=3;
						break;
					}
					case SIGABRT: {
						idx=4;
						break;
					}
					case SIGTRAP: {
						idx=5;
						break;
					}
				}
				if (idx>=0) sigaction(sig,&self.prev_actions_[idx],nullptr);
				/*else {
					struct sigaction dfl={};
					dfl.sa_handler=SIG_DFL;
					sigemptyset(&dfl.sa_mask);
					sigaction(sig,&dfl,nullptr);
				}
				sigset_t unblock;
				sigemptyset(&unblock);
				sigaddset(&unblock,sig);
				sigprocmask(SIG_UNBLOCK,&unblock,nullptr);*/
				raise(sig);
				_Exit(EXIT_FAILURE);
			}
			exception_infos ex_info=collect_posix_info(sig,si,ucontext);
			ex_info.timestamp=make_timestamp();
			ex_info.process_id=static_cast<uint32_t>(getpid());
			#if _STDEX_APPLE_PLATFORM
				uint64_t tid=0;
				pthread_threadid_np(nullptr,&tid);
				ex_info.thread_id=static_cast<uint32_t>(tid);
			#elif _STDEX_ANDROID_PLATFORM
				ex_info.thread_id=static_cast<uint32_t>(gettid());
			#elif _STDEX_LINUX_PLATFORM
				ex_info.thread_id=static_cast<uint32_t>(syscall(SYS_gettid));
			#endif
			if (self.features_.contains(SF_TRACE)) self.collect_stack_trace(ex_info);
			if (self.features_.contains(SF_MODULES)) self.collect_modules(ex_info);
			bool should_default=true;
			if (self.user_handler_) should_default=self.user_handler_(ex_info);
			if (should_default) self.default_handler(ex_info);
			current_ucontext_=nullptr;
			_Exit(EXIT_FAILURE);
		}

		exception_infos build_info_from_last_signal() {
			exception_infos info;
			if (last_signal_==0) return info;
			info=collect_posix_info(last_signal_,&last_siginfo_,&last_ucontext_storage_);
			info.timestamp=make_timestamp();
			info.process_id=static_cast<uint32_t>(getpid());
			#if _STDEX_APPLE_PLATFORM
				uint64_t tid=0;
				pthread_threadid_np(nullptr,&tid);
				info.thread_id=static_cast<uint32_t>(tid);
			#elif _STDEX_ANDROID_PLATFORM
				info.thread_id=static_cast<uint32_t>(gettid());
			#elif _STDEX_LINUX_PLATFORM
				info.thread_id=static_cast<uint32_t>(syscall(SYS_gettid));
			#endif
			if (features_.contains(SF_TRACE)) collect_stack_trace(info);
			if (features_.contains(SF_MODULES)) collect_modules(info);
			return info;
		}

		static exception_infos collect_posix_info(int sig,siginfo_t* si,void* ucontext) {
			exception_infos info;
			info.is_supported_platform=true;
			info.code=static_cast<uintptr_t>(sig);
			info.fault_address=si->si_addr;
			#if _STDEX_APPLE_PLATFORM
				#if _STDEX_IOS_PLATFORM
					info.system_info="iOS / iPadOS";
			 	#elif _STDEX_MACOS_PLATFORM
					info.system_info="macOS";
				#else
					info.system_info="Apple platform";
			 	#endif
			#elif _STDEX_ANDROID_PLATFORM
				info.system_info="Android";
			#else
				{
					struct utsname u{};
					uname(&u);
					std::ostringstream oss;
					oss<<u.sysname<<" "<<u.release<<" "<<u.machine;
					info.system_info=oss.str();
				}
			#endif
			#if _STDEX_APPLE_PLATFORM
				ucontext_t* uc=static_cast<ucontext_t*>(ucontext);
				#if defined(__x86_64__)
					auto& r=uc->uc_mcontext->__ss;
					info.registers={
						{"RAX",r.__rax},{"RBX",r.__rbx},{"RCX",r.__rcx},{"RDX",r.__rdx},
						{"RSI",r.__rsi},{"RDI",r.__rdi},{"RBP",r.__rbp},{"RSP",r.__rsp},
						{"RIP",r.__rip},
						{"R8", r.__r8}, {"R9", r.__r9}, {"R10",r.__r10},{"R11",r.__r11},
						{"R12",r.__r12},{"R13",r.__r13},{"R14",r.__r14},{"R15",r.__r15},
						{"RFLAGS",r.__rflags},{"CS",r.__cs},{"FS",r.__fs},{"GS",r.__gs}
					};
				#elif defined(__i386__)
					auto& r=uc->uc_mcontext->__ss;
					info.registers={
						{"EAX",r.__eax},{"EBX",r.__ebx},{"ECX",r.__ecx},{"EDX",r.__edx},
						{"ESI",r.__esi},{"EDI",r.__edi},{"EBP",r.__ebp},{"ESP",r.__esp},
						{"EIP",r.__eip},{"EFLAGS",r.__eflags},
						{"CS",r.__cs},{"DS",r.__ds},{"ES",r.__es},
						{"FS",r.__fs},{"GS",r.__gs},{"SS",r.__ss}
					};
				#elif defined(__aarch64__)
					auto& r=uc->uc_mcontext->__ss;
					info.registers={
						{"X0", r.__x[0]}, {"X1", r.__x[1]}, {"X2", r.__x[2]}, {"X3", r.__x[3]},
						{"X4", r.__x[4]}, {"X5", r.__x[5]}, {"X6", r.__x[6]}, {"X7", r.__x[7]},
						{"X8", r.__x[8]}, {"X9", r.__x[9]}, {"X10",r.__x[10]},{"X11",r.__x[11]},
						{"X12",r.__x[12]},{"X13",r.__x[13]},{"X14",r.__x[14]},{"X15",r.__x[15]},
						{"X16",r.__x[16]},{"X17",r.__x[17]},{"X18",r.__x[18]},{"X19",r.__x[19]},
						{"X20",r.__x[20]},{"X21",r.__x[21]},{"X22",r.__x[22]},{"X23",r.__x[23]},
						{"X24",r.__x[24]},{"X25",r.__x[25]},{"X26",r.__x[26]},{"X27",r.__x[27]},
						{"X28",r.__x[28]},{"FP", r.__fp},   {"LR", r.__lr},   {"SP", r.__sp},
						{"PC", r.__pc},   {"CPSR",r.__cpsr}
					};
				#elif defined(__arm__)
					auto& r=uc->uc_mcontext->__ss;
					info.registers={
						{"R0",r.__r[0]},{"R1",r.__r[1]},{"R2",r.__r[2]},{"R3",r.__r[3]},
						{"R4",r.__r[4]},{"R5",r.__r[5]},{"R6",r.__r[6]},{"R7",r.__r[7]},
						{"R8",r.__r[8]},{"R9",r.__r[9]},{"R10",r.__r[10]},{"R11",r.__r[11]},
						{"R12",r.__r[12]},{"SP",r.__sp},{"LR",r.__lr},{"PC",r.__pc},
						{"CPSR",r.__cpsr}
					};
				#endif
			#elif _STDEX_ANDROID_PLATFORM || _STDEX_LINUX_PLATFORM
				ucontext_t* uc=static_cast<ucontext_t*>(ucontext);
				mcontext_t& mctx=uc->uc_mcontext;
				#if defined(__x86_64__)
					info.registers={
						{"RAX",static_cast<uintptr_t>(mctx.gregs[REG_RAX])},
						{"RBX",static_cast<uintptr_t>(mctx.gregs[REG_RBX])},
						{"RCX",static_cast<uintptr_t>(mctx.gregs[REG_RCX])},
						{"RDX",static_cast<uintptr_t>(mctx.gregs[REG_RDX])},
						{"RSI",static_cast<uintptr_t>(mctx.gregs[REG_RSI])},
						{"RDI",static_cast<uintptr_t>(mctx.gregs[REG_RDI])},
						{"RBP",static_cast<uintptr_t>(mctx.gregs[REG_RBP])},
						{"RSP",static_cast<uintptr_t>(mctx.gregs[REG_RSP])},
						{"RIP",static_cast<uintptr_t>(mctx.gregs[REG_RIP])},
						{"R8", static_cast<uintptr_t>(mctx.gregs[REG_R8])},
						{"R9", static_cast<uintptr_t>(mctx.gregs[REG_R9])},
						{"R10",static_cast<uintptr_t>(mctx.gregs[REG_R10])},
						{"R11",static_cast<uintptr_t>(mctx.gregs[REG_R11])},
						{"R12",static_cast<uintptr_t>(mctx.gregs[REG_R12])},
						{"R13",static_cast<uintptr_t>(mctx.gregs[REG_R13])},
						{"R14",static_cast<uintptr_t>(mctx.gregs[REG_R14])},
						{"R15",static_cast<uintptr_t>(mctx.gregs[REG_R15])},
						{"EFLAGS",static_cast<uintptr_t>(mctx.gregs[REG_EFL])},
						{"CSGSFS",static_cast<uintptr_t>(mctx.gregs[REG_CSGSFS])},
						{"CS",static_cast<uintptr_t>(mctx.gregs[REG_CSGSFS]&0xFFFF)},
						{"GS",static_cast<uintptr_t>((mctx.gregs[REG_CSGSFS]>>16)&0xFFFF)},
						{"FS",static_cast<uintptr_t>((mctx.gregs[REG_CSGSFS]>>32)&0xFFFF)},
						{"TRAPNO",static_cast<uintptr_t>(mctx.gregs[REG_TRAPNO])},
						{"ERR",static_cast<uintptr_t>(mctx.gregs[REG_ERR])},
						#ifdef REG_CR2
							{"CR2",static_cast<uintptr_t>(mctx.gregs[REG_CR2])},
						#endif
						#ifdef REG_OLDMASK
							{"OLDMASK",static_cast<uintptr_t>(mctx.gregs[REG_OLDMASK])}
						#endif
					};
				#elif defined(__i386__)
					info.registers={
						{"EAX",static_cast<uintptr_t>(mctx.gregs[REG_EAX])},
						{"EBX",static_cast<uintptr_t>(mctx.gregs[REG_EBX])},
						{"ECX",static_cast<uintptr_t>(mctx.gregs[REG_ECX])},
						{"EDX",static_cast<uintptr_t>(mctx.gregs[REG_EDX])},
						{"ESI",static_cast<uintptr_t>(mctx.gregs[REG_ESI])},
						{"EDI",static_cast<uintptr_t>(mctx.gregs[REG_EDI])},
						{"EBP",static_cast<uintptr_t>(mctx.gregs[REG_EBP])},
						{"ESP",static_cast<uintptr_t>(mctx.gregs[REG_ESP])},
						{"EIP",static_cast<uintptr_t>(mctx.gregs[REG_EIP])},
						{"EFLAGS",static_cast<uintptr_t>(mctx.gregs[REG_EFL])},
						{"TRAPNO",static_cast<uintptr_t>(mctx.gregs[REG_TRAPNO])},
						{"ERR",static_cast<uintptr_t>(mctx.gregs[REG_ERR])},
						{"CS",static_cast<uintptr_t>(mctx.gregs[REG_CS])},
						{"SS",static_cast<uintptr_t>(mctx.gregs[REG_SS])},
						{"DS",static_cast<uintptr_t>(mctx.gregs[REG_DS])},
						{"ES",static_cast<uintptr_t>(mctx.gregs[REG_ES])},
						{"FS",static_cast<uintptr_t>(mctx.gregs[REG_FS])},
						{"GS",static_cast<uintptr_t>(mctx.gregs[REG_GS])},
						{"UESP",static_cast<uintptr_t>(mctx.gregs[REG_UESP])}
					};
				#elif defined(__aarch64__)
					info.registers={
						{"X0", static_cast<uintptr_t>(mctx.regs[0])},
						{"X1", static_cast<uintptr_t>(mctx.regs[1])},
						{"X2", static_cast<uintptr_t>(mctx.regs[2])},
						{"X3", static_cast<uintptr_t>(mctx.regs[3])},
						{"X4", static_cast<uintptr_t>(mctx.regs[4])},
						{"X5", static_cast<uintptr_t>(mctx.regs[5])},
						{"X6", static_cast<uintptr_t>(mctx.regs[6])},
						{"X7", static_cast<uintptr_t>(mctx.regs[7])},
						{"X8", static_cast<uintptr_t>(mctx.regs[8])},
						{"X9", static_cast<uintptr_t>(mctx.regs[9])},
						{"X10",static_cast<uintptr_t>(mctx.regs[10])},
						{"X11",static_cast<uintptr_t>(mctx.regs[11])},
						{"X12",static_cast<uintptr_t>(mctx.regs[12])},
						{"X13",static_cast<uintptr_t>(mctx.regs[13])},
						{"X14",static_cast<uintptr_t>(mctx.regs[14])},
						{"X15",static_cast<uintptr_t>(mctx.regs[15])},
						{"X16",static_cast<uintptr_t>(mctx.regs[16])},
						{"X17",static_cast<uintptr_t>(mctx.regs[17])},
						{"X18",static_cast<uintptr_t>(mctx.regs[18])},
						{"X19",static_cast<uintptr_t>(mctx.regs[19])},
						{"X20",static_cast<uintptr_t>(mctx.regs[20])},
						{"X21",static_cast<uintptr_t>(mctx.regs[21])},
						{"X22",static_cast<uintptr_t>(mctx.regs[22])},
						{"X23",static_cast<uintptr_t>(mctx.regs[23])},
						{"X24",static_cast<uintptr_t>(mctx.regs[24])},
						{"X25",static_cast<uintptr_t>(mctx.regs[25])},
						{"X26",static_cast<uintptr_t>(mctx.regs[26])},
						{"X27",static_cast<uintptr_t>(mctx.regs[27])},
						{"X28",static_cast<uintptr_t>(mctx.regs[28])},
						{"FP", static_cast<uintptr_t>(mctx.regs[29])},
						{"LR", static_cast<uintptr_t>(mctx.regs[30])},
						{"SP", static_cast<uintptr_t>(mctx.sp)},
						{"PC", static_cast<uintptr_t>(mctx.pc)},
						{"PSTATE",static_cast<uintptr_t>(mctx.pstate)}
					};
				#elif defined(__arm__)
					info.registers={
						{"R0", static_cast<uintptr_t>(mctx.arm_r0)},
						{"R1", static_cast<uintptr_t>(mctx.arm_r1)},
						{"R2", static_cast<uintptr_t>(mctx.arm_r2)},
						{"R3", static_cast<uintptr_t>(mctx.arm_r3)},
						{"R4", static_cast<uintptr_t>(mctx.arm_r4)},
						{"R5", static_cast<uintptr_t>(mctx.arm_r5)},
						{"R6", static_cast<uintptr_t>(mctx.arm_r6)},
						{"R7", static_cast<uintptr_t>(mctx.arm_r7)},
						{"R8", static_cast<uintptr_t>(mctx.arm_r8)},
						{"R9", static_cast<uintptr_t>(mctx.arm_r9)},
						{"R10",static_cast<uintptr_t>(mctx.arm_r10)},
						{"R11",static_cast<uintptr_t>(mctx.arm_fp)},
						{"R12",static_cast<uintptr_t>(mctx.arm_ip)},
						{"SP", static_cast<uintptr_t>(mctx.arm_sp)},
						{"LR", static_cast<uintptr_t>(mctx.arm_lr)},
						{"PC", static_cast<uintptr_t>(mctx.arm_pc)},
						{"CPSR",static_cast<uintptr_t>(mctx.arm_cpsr)}
					};
				#endif
			#endif
			switch (sig) {
				case SIGSEGV: {
					info.error_message="Segmentation Fault (SIGSEGV)";
					std::ostringstream det;
					switch (si->si_code) {
						case SEGV_MAPERR: {
							det<<"Address not mapped to object";
							break;
						}
						case SEGV_ACCERR: {
							det<<"Invalid permissions for mapped object";
							break;
						}
						default: {
							det<<"si_code="<<si->si_code;
							break;
						}
					}
					info.error_detail=det.str();
					break;
				}
				case SIGBUS: {
					info.error_message="Bus Error (SIGBUS)";
					std::ostringstream det;
					switch (si->si_code) {
						case BUS_ADRALN: {
							det<<"Invalid address alignment";
							break;
						}
						case BUS_ADRERR: {
							det<<"Nonexistent physical address";
							break;
						}
						case BUS_OBJERR: {
							det<<"Object-specific hardware error";
							break;
						}
						default: {
							det<<"si_code="<<si->si_code;
							break;
						}
					}
					info.error_detail=det.str();
					break;
				}
				case SIGFPE: {
					info.error_message="Floating Point Exception (SIGFPE)";
					std::ostringstream det;
					switch (si->si_code) {
						case FPE_INTDIV: {
							det<<"Integer divide by zero";
							break;
						}
						case FPE_INTOVF: {
							det<<"Integer overflow";
							break;
						}
						case FPE_FLTDIV: {
							det<<"FP divide by zero";
							break;
						}
						case FPE_FLTOVF: {
							det<<"FP overflow";
							break;
						}
						case FPE_FLTUND: {
							det<<"FP underflow";
							break;
						}
						case FPE_FLTRES: {
							det<<"FP inexact result";
							break;
						}
						case FPE_FLTINV: {
							det<<"FP invalid operation";
							break;
						}
						case FPE_FLTSUB: {
							det<<"Subscript out of range";
							break;
						}
						default: {
							det<<"si_code="<<si->si_code;
							break;
						}
					}
					info.error_detail=det.str();
					break;
				}
				case SIGILL: {
					info.error_message="Illegal Instruction (SIGILL)";
					std::ostringstream det;
					switch (si->si_code) {
						case ILL_ILLOPC: {
							det<<"Illegal opcode";
							break;
						}
						case ILL_ILLOPN: {
							det<<"Illegal operand";
							break;
						}
						case ILL_ILLADR: {
							det<<"Illegal addressing mode";
							break;
						}
						case ILL_ILLTRP: {
							det<<"Illegal trap";
							break;
						}
						case ILL_PRVOPC: {
							det<<"Privileged opcode";
							break;
						}
						case ILL_PRVREG: {
							det<<"Privileged register";
							break;
						}
						case ILL_COPROC: {
							det<<"Coprocessor error";
							break;
						}
						case ILL_BADSTK: {
							det<<"Internal stack error";
							break;
						}
						default: {
							det<<"si_code="<<si->si_code;
							break;
						}
					}
					info.error_detail=det.str();
					break;
				}
				case SIGABRT: {
					info.error_message="Abort (SIGABRT)";
					break;
				}
				case SIGTRAP: {
					info.error_message="Breakpoint / Trace Trap (SIGTRAP)";
					break;
				}
				default: {
					std::ostringstream oss;
					oss<<"Unknown Signal ("<<sig<<")";
					info.error_message=oss.str();
					break;
				}
			}
			return info;
		}
	#endif


public:
	std::size_t stack_length=64;
	#if _STDEX_WINDOWS_PLATFORM
		jmp_buf recovery_point;
	#elif _STDEX_APPLE_PLATFORM || _STDEX_ANDROID_PLATFORM || _STDEX_LINUX_PLATFORM
		sigjmp_buf recovery_point;
	#endif
	volatile sig_atomic_t recovery_point_valid=0;

	void configure(stdex::bitwise::flags<seh_feature> features,const std::string& log_path="crash.log",const std::string& dump_path="crash.dmp") {
		features_=features;
		log_path_=log_path;
		dump_path_=dump_path;
	}

	static exception_handler& instance() {
		static exception_handler inst;
		return inst;
	}

	void set_handler(handler_t handler) {
		user_handler_=std::move(handler);
	}
	void set_recovery_handler(recovery_t handler) {
		recovery_handler_=std::move(handler);
	}
	void set_cleanup_handler(cleanup_t handler) {
		cleanup_handler_=std::move(handler);
	}

	[[deprecated("Calling set_recovery_point() with no argument is undefined behavior under the C standard (the setjmp frame is destroyed on function return). Use set_recovery_point(callable) instead.")]]
	bool set_recovery_point() {
		recovery_point_valid=0;
				return false;
	}
	template <typename _Func>
	bool set_recovery_point(_Func&& func) {
		recovery_point_valid=0;
		#if _STDEX_WINDOWS_PLATFORM
			if (setjmp(recovery_point)==0) {
				recovery_point_valid=1;
				std::forward<_Func>(func)();
				recovery_point_valid=0;
				return false;
			}
			recovery_point_valid=0;
			return true;
		#elif _STDEX_APPLE_PLATFORM || _STDEX_ANDROID_PLATFORM || _STDEX_LINUX_PLATFORM
			if (sigsetjmp(recovery_point,1)==0) {
				recovery_point_valid=1;
				std::forward<_Func>(func)();
				recovery_point_valid=0;
				return false;
			}
			recovery_point_valid=0;
			return true;
		#else
			std::forward<_Func>(func)();
			return false;
		#endif
	}
	void clear_recovery_point() {
		recovery_point_valid=0;
	}

	bool init() {
		#if _STDEX_WINDOWS_PLATFORM
			prev_filter_=SetUnhandledExceptionFilter(seh_filter);
			return true;
		#elif _STDEX_APPLE_PLATFORM || _STDEX_ANDROID_PLATFORM || _STDEX_LINUX_PLATFORM
			struct sigaction sa={};
			sa.sa_sigaction=signal_handler;
			sa.sa_flags=SA_SIGINFO|SA_ONSTACK;
			sigemptyset(&sa.sa_mask);
			alt_stack_mem_=malloc(SIGSTKSZ*4);
			if (!alt_stack_mem_) return false;
			stack_t ss={};
			ss.ss_sp=alt_stack_mem_;
			ss.ss_size=SIGSTKSZ*4;
			ss.ss_flags=0;
			if (sigaltstack(&ss,nullptr)<0) {
				free(alt_stack_mem_);
				alt_stack_mem_=nullptr;
				return false;
			}
			bool ok=true;
			ok&=!sigaction(SIGSEGV,&sa,&prev_actions_[0]);
			ok&=!sigaction(SIGBUS,&sa,&prev_actions_[1]);
			ok&=!sigaction(SIGFPE,&sa,&prev_actions_[2]);
			ok&=!sigaction(SIGILL,&sa,&prev_actions_[3]);
			ok&=!sigaction(SIGABRT,&sa,&prev_actions_[4]);
			ok&=!sigaction(SIGTRAP,&sa,&prev_actions_[5]);
			return ok;
		#else
			return false;
		#endif
	}

	~exception_handler() {
		#if _STDEX_APPLE_PLATFORM || _STDEX_ANDROID_PLATFORM || _STDEX_LINUX_PLATFORM
			static const int sigs[6]={SIGSEGV,SIGBUS,SIGFPE,SIGILL,SIGABRT,SIGTRAP };
			for (int i=0;i<6;i++) sigaction(sigs[i],&prev_actions_[i],nullptr);
			if (alt_stack_mem_) {
				stack_t ss={};
				ss.ss_flags=SS_DISABLE;
				sigaltstack(&ss,nullptr);
				free(alt_stack_mem_);
				alt_stack_mem_=nullptr;
				}
		#elif _STDEX_WINDOWS_PLATFORM
			if (prev_filter_) {
				SetUnhandledExceptionFilter(prev_filter_);
				prev_filter_=nullptr;
			}
		#endif
	}

	exception_infos get_last_exception() {
		#if _STDEX_WINDOWS_PLATFORM
			if (!last_exception_valid_) return exception_infos{};
			return last_exception_;
		#elif _STDEX_APPLE_PLATFORM || _STDEX_ANDROID_PLATFORM || _STDEX_LINUX_PLATFORM
			if (!last_exception_valid_ || last_signal_==0) return exception_infos{};
			exception_infos info=collect_posix_info(last_signal_,&last_siginfo_,&last_ucontext_storage_);
			info.timestamp=make_timestamp();
			info.process_id=static_cast<uint32_t>(getpid());
			#if _STDEX_APPLE_PLATFORM
				uint64_t tid=0;
				pthread_threadid_np(nullptr,&tid);
				info.thread_id=static_cast<uint32_t>(tid);
			#elif _STDEX_ANDROID_PLATFORM
				info.thread_id=static_cast<uint32_t>(gettid());
			#elif _STDEX_LINUX_PLATFORM
				info.thread_id=static_cast<uint32_t>(syscall(SYS_gettid));
			#endif
			if (features_.contains(SF_TRACE)) collect_stack_trace(info);
			if (features_.contains(SF_MODULES)) collect_modules(info);
			return info;
		#else
			return exception_infos{};
		#endif
	}
	void clear_last_exception() noexcept {
		last_exception_valid_=0;
		#if _STDEX_APPLE_PLATFORM || _STDEX_ANDROID_PLATFORM || _STDEX_LINUX_PLATFORM
			last_signal_=0;
		#endif
	}
};

#if _STDEX_WINDOWS_PLATFORM
	inline EXCEPTION_POINTERS* exception_handler::current_exception_=nullptr;
#elif _STDEX_APPLE_PLATFORM || _STDEX_ANDROID_PLATFORM || _STDEX_LINUX_PLATFORM
	inline void* exception_handler::current_ucontext_=nullptr;
	inline volatile sig_atomic_t exception_handler::last_signal_=0;
	inline siginfo_t exception_handler::last_siginfo_={};
	inline ucontext_t exception_handler::last_ucontext_storage_={};
#endif

}

}

#if _STDEX_WINDOWS_PLATFORM
	#define STDEX_SEH_SET_RECOVERY_POINT() (stdex::machine::exception_handler::instance().recovery_point_valid=0,setjmp(stdex::machine::exception_handler::instance().recovery_point)==0?(stdex::machine::exception_handler::instance().recovery_point_valid=1,false):(stdex::machine::exception_handler::instance().recovery_point_valid=0,true))
#elif _STDEX_APPLE_PLATFORM || _STDEX_ANDROID_PLATFORM || _STDEX_LINUX_PLATFORM
	#define STDEX_SEH_SET_RECOVERY_POINT() (stdex::machine::exception_handler::instance().recovery_point_valid=0,sigsetjmp(stdex::machine::exception_handler::instance().recovery_point,1)==0?(stdex::machine::exception_handler::instance().recovery_point_valid=1,false):(stdex::machine::exception_handler::instance().recovery_point_valid=0,true))
#else
	#define STDEX_SEH_SET_RECOVERY_POINT() (false)
#endif

#endif