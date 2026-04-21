//Last Modified At 2025/11/26
//@Version 1.0.0.0
#ifndef _STDEX_WINDOWS_PLATFORM
#if defined(_WIN32)
#define _STDEX_WINDOWS_PLATFORM 1
#else
#define _STDEX_WINDOWS_PLATFORM 0
#endif
#endif
#ifndef _STDEX_WINDOWS64_PLATFORM
#if defined(_WIN64)
#define _STDEX_WINDOWS64_PLATFORM 1
#else
#define _STDEX_WINDOWS64_PLATFORM 0
#endif
#endif
#ifndef _STDEX_WINDOWS32_PLATFORM
#if _STDEX_WINDOWS_PLATFORM && _STDEX_WINDOWS64_PLATFORM
#define _STDEX_WINDOWS32_PLATFORM 0
#else
#define _STDEX_WINDOWS32_PLATFORM 1
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
