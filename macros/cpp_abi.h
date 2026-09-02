//Last Modified At 2026/08/29
//@Version 2.0.0.0

#include "cpp_version.h"//At Least 1.0
#include "cpp_compiler.h"//At Least 1.0

#if __cplusplus>=_STDEX_CPP20_VERSION
#include <version>
#else
#include <ciso646>
#endif

#ifndef _STDEX_ABI_ARM64
#if defined(__aarch64__) || defined(_M_ARM64)
#define _STDEX_ABI_ARM64 1
#else
#define _STDEX_ABI_ARM64 0
#endif
#endif
#ifndef _STDEX_ABI_ARMEABI
#if defined(__arm__) || defined(_M_ARM)
#define _STDEX_ABI_ARMEABI 1
#else
#define _STDEX_ABI_ARMEABI 0
#endif
#endif
#ifndef _STDEX_ABI_X86_64
#if defined(__x86_64__) || defined(_M_X64)
#define _STDEX_ABI_X86_64 1
#else
#define _STDEX_ABI_X86_64 0
#endif
#endif
#ifndef _STDEX_ABI_X86
#if defined(__i386__) || defined(_M_IX86)
#define _STDEX_ABI_X86 1
#else
#define _STDEX_ABI_X86 0
#endif
#endif

#ifndef _STDEX_ABI_STD_GNU
#if defined(__GLIBCXX__) || defined(_GLIBCXX_RELEASE)
#define _STDEX_ABI_STD_GNU 1
#else
#define _STDEX_ABI_STD_GNU 0
#endif
#endif
#ifndef _STDEX_ABI_STD_LLVM
#if defined(_LIBCPP_VERSION)
#define _STDEX_ABI_STD_LLVM 1
#else
#define _STDEX_ABI_STD_LLVM 0
#endif
#endif
#ifndef _STDEX_ABI_STD_MSVC
#if defined(_MSVC_STL_VERSION) || defined(_CPPLIB_VER)
#define _STDEX_ABI_STD_MSVC 1
#else
#define _STDEX_ABI_STD_MSVC 0
#endif
#endif

#ifndef _STDEX_ABI_STD_GNU_CXX11
#if _STDEX_ABI_STD_GNU && defined(_GLIBCXX_USE_CXX11_ABI) && _GLIBCXX_USE_CXX11_ABI
#define _STDEX_ABI_STD_GNU_CXX11 1
#else
#define _STDEX_ABI_STD_GNU_CXX11 0
#endif
#endif
#ifndef _STDEX_ABI_STD_GNU_DEBUG
#if _STDEX_ABI_STD_GNU && defined(_GLIBCXX_DEBUG)
#define _STDEX_ABI_STD_GNU_DEBUG 1
#else
#define _STDEX_ABI_STD_GNU_DEBUG 0
#endif
#endif
#ifndef _STDEX_ABI_STD_GNU_ASSERTIONS
#if _STDEX_ABI_STD_GNU && defined(_GLIBCXX_ASSERTIONS)
#define _STDEX_ABI_STD_GNU_ASSERTIONS 1
#else
#define _STDEX_ABI_STD_GNU_ASSERTIONS 0
#endif
#endif
#ifndef _STDEX_ABI_STD_LLVM_HARDENING
#if _STDEX_ABI_STD_LLVM && defined(_LIBCPP_HARDENING_MODE) && defined(_LIBCPP_HARDENING_MODE_NONE)
#if _LIBCPP_HARDENING_MODE!=_LIBCPP_HARDENING_MODE_NONE
#define _STDEX_ABI_STD_LLVM_HARDENING 1
#else
#define _STDEX_ABI_STD_LLVM_HARDENING 0
#endif
#elif _STDEX_ABI_STD_LLVM && defined(_LIBCPP_ENABLE_ASSERTIONS) && _LIBCPP_ENABLE_ASSERTIONS
#define _STDEX_ABI_STD_LLVM_HARDENING 1
#else
#define _STDEX_ABI_STD_LLVM_HARDENING 0
#endif
#endif

#ifndef _STDEX_ABI_STD_MSVC_ITERATOR_DEBUG
#if _STDEX_ABI_STD_MSVC
#if defined(_ITERATOR_DEBUG_LEVEL)
#define _STDEX_ABI_STD_MSVC_ITERATOR_DEBUG _ITERATOR_DEBUG_LEVEL
#elif defined(_DEBUG)
#define _STDEX_ABI_STD_MSVC_ITERATOR_DEBUG 2
#else
#define _STDEX_ABI_STD_MSVC_ITERATOR_DEBUG 0
#endif
#else
#define _STDEX_ABI_STD_MSVC_ITERATOR_DEBUG 0
#endif
#endif

#ifndef _STDEX_ABI_DEBUG
#if defined(_DEBUG) || _STDEX_ABI_STD_GNU_DEBUG || _STDEX_ABI_STD_GNU_ASSERTIONS || _STDEX_ABI_STD_LLVM_HARDENING || _STDEX_ABI_STD_MSVC_ITERATOR_DEBUG>0
#define _STDEX_ABI_DEBUG 1
#else
#define _STDEX_ABI_DEBUG 0
#endif
#endif