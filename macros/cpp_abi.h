//Last Modified At 2026/06/01
//@Version 1.0.0.0
#ifndef _STDEX_ABI_ARM64
#if defined(__aarch64__)
#define _STDEX_ABI_ARM64 1
#else
#define _STDEX_ABI_ARM64 0
#endif
#endif
#ifndef _STDEX_ABI_ARMEABI
#if defined(__arm__)
#define _STDEX_ABI_ARMEABI 1
#else
#define _STDEX_ABI_ARMEABI 0
#endif
#endif
#ifndef _STDEX_ABI_X86_64
#if defined(__x86_64__)
#define _STDEX_ABI_X86_64 1
#else
#define _STDEX_ABI_X86_64 0
#endif
#endif
#ifndef _STDEX_ABI_X86
#if defined(__i386__)
#define _STDEX_ABI_X86 1
#else
#define _STDEX_ABI_X86 0
#endif
#endif
