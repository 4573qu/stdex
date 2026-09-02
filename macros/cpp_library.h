//Last Modified At 2026/08/29
//@Version 1.0.0.0
#include "cpp_compiler.h"//At Least 1.0
#include "cpp_platform.h"//At Least 1.0

//Marks a symbol as exported from the enclosing binary: dllexport on Windows, default visibility
//where the ELF/Mach-O visibility attribute is available, nothing otherwise.
#ifndef _STDEX_LIBRARY_EXPORT
#if _STDEX_WINDOWS_PLATFORM
#define _STDEX_LIBRARY_EXPORT __declspec(dllexport)
#elif _STDEX_GNU_COMPILER || _STDEX_CLANG_COMPILER
#define _STDEX_LIBRARY_EXPORT __attribute__((visibility("default")))
#else
#define _STDEX_LIBRARY_EXPORT
#endif
#endif
//Marks a symbol as imported from another binary: dllimport on Windows, nothing otherwise (ELF and
//Mach-O do not distinguish the import direction).
#ifndef _STDEX_LIBRARY_IMPORT
#if _STDEX_WINDOWS_PLATFORM
#define _STDEX_LIBRARY_IMPORT __declspec(dllimport)
#else
#define _STDEX_LIBRARY_IMPORT
#endif
#endif
//Hides a symbol from the dynamic export table where supported. On Windows symbols are hidden by
//default, so the macro expands to nothing there.
#ifndef _STDEX_LIBRARY_LOCAL
#if _STDEX_WINDOWS_PLATFORM
#define _STDEX_LIBRARY_LOCAL
#elif _STDEX_GNU_COMPILER || _STDEX_CLANG_COMPILER
#define _STDEX_LIBRARY_LOCAL __attribute__((visibility("hidden")))
#else
#define _STDEX_LIBRARY_LOCAL
#endif
#endif
