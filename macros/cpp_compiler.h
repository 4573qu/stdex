//Last Modified At 2025/10/31
//@Version 1.0.0.0
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

#ifndef _STDEX_RETURNS_NON_NULL
#if _STDEX_GNU_COMPILER || _STDEX_CLANG_COMPILER
#define _STDEX_RETURNS_NON_NULL __attribute__((__returns_nonnull__))
#else
#define _STDEX_RETURNS_NON_NULL
#endif
#endif