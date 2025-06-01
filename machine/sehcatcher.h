//Last Modified At 2025/05/10
//@Version 1.01
#ifndef _STD4573_MACHINE_SEHCATCHER_H_
#define _STD4573_MACHINE_SEHCATCHER_H_ 1

#if !defined(_WIN32) && !defined(__linux__)
	#if defined(__GNUC__) || defined(__clang__)
		#warning "sehcatcher: This library is only fully supported on Windows and Linux platforms"
	#elif defined(_MSC_VER)
		#pragma message("sehcatcher: This library is only fully supported on Windows and Linux platforms")
	#else
		#pragma message "sehcatcher: This library is only fully supported on Windows and Linux platforms"
	#endif
#endif

#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <functional>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include "../bitmask/flags.h"//At Least 1.1

#if defined(_WIN32)
	#include <windows.h>
	#include <dbghelp.h>
	#pragma comment(lib,"dbghelp.lib")
#elif defined(__linux__)
	#include <execinfo.h>
	#include <fcntl.h>
	#include <signal.h>
	#include <ucontext.h>
	#include <unistd.h>
#endif

namespace std {

namespace machine {

enum SEH_FEATURE {
	SF_NONE=0,
	SF_OUTPUT=1<<0,
	SF_LOGGING=1<<1,
	SF_DUMP=1<<2,
	SF_TRACE=1<<3,
	SF_RECOVERY=1<<4,
	SF_ALL=~0u
};

struct exception_infos {
	uintptr_t code_;
	void* fault_address_;
	std::vector<std::pair<std::string,uintptr_t>> registers_;
	std::string system_info_;
	std::string error_message_;
	std::vector<void*> stack_trace_;
	std::string to_string() const {
		std::ostringstream oss;
		oss<<"System Info: "<<system_info_<<"\n";
		oss<<"Error ["<<std::hex<<std::uppercase<<std::setfill('0')<<code_<<"]: "<<error_message_<<"\n";
		oss<<"Fault address: "<<fault_address_<<"\n";
		oss<<"Registers:\n";
		for (const auto& [name,value]:registers_) oss<<"  "<<name<<": 0x"<<std::hex<<std::uppercase<<std::setfill('0')<<value<<"\n";
		oss<<"Stack trace:\n";
		for (auto&& addr:stack_trace_) oss<<"  ["<<std::hex<<std::uppercase<<std::setfill('0')<<addr<<"]\n";
		return oss.str();
	}
};

using handler_t=std::function<bool(exception_infos&)>;

class exception_handler {
public:
	void configure(std::bitmask::flags<SEH_FEATURE> features,const std::string& log_path="crash.log",const std::string& dump_path="crash.dmp") {
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
	bool init() {
		#if defined(_WIN32)
			prev_filter_=SetUnhandledExceptionFilter(seh_filter);
			return true;
		#elif defined(__linux__)
			struct sigaction sa;
			stack_t ss;
			ss.ss_sp=malloc(SIGSTKSZ);
			if (!ss.ss_sp) return false;
			ss.ss_size=SIGSTKSZ;
			ss.ss_flags=0;
			if (sigaltstack(&ss,nullptr)<0) {
				free(ss.ss_sp);
				return false;
			}
			sa.sa_sigaction=signal_handler;
			sa.sa_flags=SA_SIGINFO|SA_ONSTACK|SA_NODEFER;
			sigemptyset(&sa.sa_mask);
			bool success=true;
			success&=!sigaction(SIGSEGV,&sa,&prev_actions_[0]);
			success&=!sigaction(SIGBUS,&sa,&prev_actions_[1]);
			success&=!sigaction(SIGFPE,&sa,&prev_actions_[2]);
			success&=!sigaction(SIGILL,&sa,&prev_actions_[3]);
			return success;
		#else
			static_assert(false,"Unsupported platform");
			return false;
		#endif
    }

public:
	size_t stack_length_=50;

private:
	exception_handler()=default;
	std::bitmask::flags<SEH_FEATURE> features_;
	std::string log_path_;
	std::string dump_path_;
	handler_t user_handler_;
	bool default_handler(exception_infos& info,bool* windows_recovery=nullptr) {
		bool should_recover=false;
		info.stack_trace_.clear();
		if (features_.contains(SF_TRACE)) {
			#if defined(_WIN32)
				void* stack[stack_length_];
				auto frames=CaptureStackBackTrace(0,stack_length_,stack,nullptr);
				info.stack_trace_.assign(stack,stack+frames);
			#elif defined(__linux__)
				void* stack[stack_length_];
				auto frames=backtrace(stack,stack_length_);
				info.stack_trace_.assign(stack,stack+frames);
			#endif
		}
		if (features_.contains(SF_OUTPUT)) std::cerr<<"[SEHCatcher] Crash detected:\n"<<info.to_string()<<"\n";
		if (features_.contains(SF_LOGGING)) {
			std::ofstream log(log_path_,std::ios::app);
			const auto crash_time=std::chrono::system_clock::now();
			const std::time_t t=std::chrono::system_clock::to_time_t(crash_time);
			std::string timestamp=std::ctime(&t);
			timestamp.pop_back();
			if (log) log<<"Crash at "<<timestamp<<"\n"<<info.to_string()<<"\n";
		}
		/*if (features_.contains(SF_DUMP)) {
			#if defined(_WIN32)
				MINIDUMP_EXCEPTION_INFORMATION mei;
				mei.ThreadId=GetCurrentThreadId();
				mei.ExceptionPointers=current_exception_;
				mei.ClientPointers=FALSE;
				MiniDumpWriteDump(GetCurrentProcess(),GetCurrentProcessId(),CreateFileA(dump_path_.c_str(),GENERIC_WRITE,0,nullptr,CREATE_ALWAYS,0,nullptr),MiniDumpNormal,&mei,nullptr,nullptr);
			#elif defined(__linux__)
				struct rlimit core_limit={ RLIM_INFINITY,RLIM_INFINITY };
				setrlimit(RLIMIT_CORE, &core_limit);
				kill(getpid(), SIGABRT);
			#endif
		}*/
		if (features_.contains(SF_RECOVERY)) {
			if (windows_recovery) *windows_recovery=false;//true;
			should_recover=basic_recovery(info);
		}
		return should_recover;
	}
	bool basic_recovery(exception_infos& info) {
		#if defined(_WIN32)
			if (info.code_==EXCEPTION_ACCESS_VIOLATION) {
				//current_exception_->ExceptionRecord->ExceptionInformation[1]=reinterpret_cast<ULONG_PTR>(info.fault_address_);
				//return true;
			}
		#elif defined(__linux__)
			ucontext_t* uc=static_cast<ucontext_t*>(current_ucontext_);
			#if defined(__x86_64__)
				uc->uc_mcontext.gregs[REG_RIP]+=2;
			#elif defined(__i386__)
				uc->uc_mcontext.gregs[REG_EIP]+=2;
			#endif
			return true;
		#endif
		return false;
	}
	#if defined(_WIN32)
		LPTOP_LEVEL_EXCEPTION_FILTER prev_filter_=nullptr;
		static LONG WINAPI seh_filter(EXCEPTION_POINTERS* exptrs) {
			auto& self=instance();
			exception_infos ex_info=collect_windows_info(exptrs);
			bool should_default=true;
			bool windows_recovery=false;
			if (self.features_.contains(SF_TRACE)) {
				auto temp_features=self.features_;
				self.features_=self.features_>>SF_ALL<<SF_TRACE;
				self.default_handler(ex_info,&windows_recovery);
				self.features_=temp_features;
			}
			if (self.user_handler_) should_default=self.user_handler_(ex_info);
			if (should_default) self.default_handler(ex_info,&windows_recovery);
			return windows_recovery?EXCEPTION_CONTINUE_EXECUTION:EXCEPTION_EXECUTE_HANDLER;
		}
		static exception_infos collect_windows_info(EXCEPTION_POINTERS* exptrs) {
			exception_infos info;
			info.code_=exptrs->ExceptionRecord->ExceptionCode;
			info.fault_address_=exptrs->ExceptionRecord->ExceptionAddress;
			current_exception_=exptrs;
			#if defined(_M_IX86)
				info.system_info_="Windows 32-bit";
				auto ctx=exptrs->ContextRecord;
				info.registers_={
					{"EAX",ctx->Eax},
					{"EBX",ctx->Ebx},
					{"ECX",ctx->Ecx},
					{"EDX",ctx->Edx},
					{"ESI",ctx->Esi},
					{"EDI",ctx->Edi},
					{"EBP",ctx->Ebp},
					{"ESP",ctx->Esp},
					{"EIP",ctx->Eip},
					{"EFLAGS",ctx->EFlags}
				};
			#elif defined(_M_X64)
				info.system_info_="Windows 64-bit";
				auto ctx=exptrs->ContextRecord;
				info.registers_={
					{"RAX",ctx->Rax},
					{"RBX",ctx->Rbx},
					{"RCX",ctx->Rcx},
					{"RDX",ctx->Rdx},
					{"RSI",ctx->Rsi},
					{"RDI",ctx->Rdi},
					{"RBP",ctx->Rbp},
					{"RSP",ctx->Rsp},
					{"RIP",ctx->Rip},
					{"R8",ctx->R8},
					{"R9",ctx->R9},
					{"R10",ctx->R10},
					{"R11",ctx->R11},
					{"R12",ctx->R12},
					{"R13",ctx->R13},
					{"R14",ctx->R14},
					{"R15",ctx->R15},
					{"EFLAGS",ctx->EFlags}
				};
			#endif
			switch (info.code_) {
				case EXCEPTION_ACCESS_VIOLATION: { info.error_message_="Access violation"; break; } 
				case EXCEPTION_STACK_OVERFLOW: { info.error_message_="Stack overflow"; break; }
				case EXCEPTION_INT_DIVIDE_BY_ZERO: { info.error_message_="Integer divide by zero"; break; }  
				case EXCEPTION_FLT_DIVIDE_BY_ZERO: { info.error_message_="Floating point divide by zero"; break; }
				default: { info.error_message_="Unknown exception"; break; }
			}
			return info;
		}
	#elif defined(__linux__)
		struct sigaction prev_actions_[4];
		static void signal_handler(int sig,siginfo_t* info,void* ucontext) {
			auto& self=instance();
			exception_infos ex_info=collect_linux_info(ucontext,info);
			ex_info.code_=sig;
			ex_info.fault_address_=info->si_addr;
			bool should_default=true;
			if (self.features_.contains(SF_TRACE)) {
				auto temp_features=self.features_;
				self.features_=self.features_>>SF_ALL<<SF_TRACE;
				self.default_handler(ex_info,&windows_recovery);
				self.features_=temp_features;
			}
			if (self.user_handler_) should_default=self.user_handler_(ex_info);
			if (should_default) default_handler(ex_info);
			_Exit(EXIT_FAILURE);
		}
		static exception_infos collect_linux_info(void* ucontext,siginfo_t* info) {
			exception_infos info;
			ucontext_t* uc=static_cast<ucontext_t*>(ucontext);
			mcontext_t& mctx=uc->uc_mcontext;
			current_ucontext_=ucontext;
			#if defined(__i386__)
				info.system_info_="Linux 32-bit";
				info.registers_={
					{"EAX",mctx.gregs[REG_EAX]},
					{"EBX",mctx.gregs[REG_EBX]},
					{"ECX",mctx.gregs[REG_ECX]},
					{"EDX",mctx.gregs[REG_EDX]},
					{"ESI",mctx.gregs[REG_ESI]},
					{"EDI",mctx.gregs[REG_EDI]},
					{"EBP",mctx.gregs[REG_EBP]},
					{"ESP",mctx.gregs[REG_ESP]},
					{"EIP",mctx.gregs[REG_EIP]},
					{"EFLAGS",mctx.gregs[REG_EFL]}
				};
			#elif defined(__x86_64__)
				info.system_info_="Linux 64-bit";
				info.registers_={
					{"RAX",mctx.gregs[REG_RAX]},
					{"RBX",mctx.gregs[REG_RBX]},
					{"RCX",mctx.gregs[REG_RCX]},
					{"RDX",mctx.gregs[REG_RDX]},
					{"RSI",mctx.gregs[REG_RSI]},
					{"RDI",mctx.gregs[REG_RDI]},
					{"RBP",mctx.gregs[REG_RBP]},
					{"RSP",mctx.gregs[REG_RSP]},
					{"RIP",mctx.gregs[REG_RIP]},
					{"R8",mctx.gregs[REG_R8]},
					{"R9",mctx.gregs[REG_R9]},
					{"R10",mctx.gregs[REG_R10]},
					{"R11",mctx.gregs[REG_R11]},
					{"R12",mctx.gregs[REG_R12]},
					{"R13",mctx.gregs[REG_R13]},
					{"R14",mctx.gregs[REG_R14]},
					{"R15",mctx.gregs[REG_R15]},
					{"EFLAGS",mctx.gregs[REG_EFL]}
				};
			#endif
			switch (info.code_) {
				case SIGSEGV: { info.error_message_="Segmentation fault"; break; }
				case SIGBUS: { info.error_message_="Bus error"; break; }
				case SIGFPE: { info.error_message_="Floating point exception"; break; }
				case SIGILL: { info.error_message_="Illegal instruction"; break; }
				default: { info.error_message_="Unknown signal"; break; }
			}
			return info;
		}
	#endif
	#if defined(_WIN32)
		static EXCEPTION_POINTERS* current_exception_;
	#elif defined(__linux__)
		static void* current_ucontext_;
	#endif
};

#if defined(_WIN32)
	EXCEPTION_POINTERS* exception_handler::current_exception_=nullptr;
#elif defined(__linux__)
	void* exception_handler::current_ucontext_=nullptr;
#endif
	
}

}

#endif