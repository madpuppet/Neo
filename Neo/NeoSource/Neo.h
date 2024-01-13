#pragma once

#include <string>
#include <format>
#include <cstdint>
#include <functional>
#include <sys/stat.h>
#include <sys/types.h>
#include <vector>
#include <map>
#include <algorithm>

#if defined(_WIN32)
#define PLATFORM_Windows
#define WIN32_LEAN_AND_MEAN
#include "windows.h"
#include "shlobj.h"
#include <io.h>
#elif defined(__APPLE__)
#import "Foundation/Foundation.h"
#include "TargetConditionals.h"
#if TARGET_OS_IPHONE
#define PLATFORM_IOS
#elif TARGET_OS_MAC
#define PLATFORM_OSX
#endif

#elif defined(ANDROID)
#define PLATFORM_Android

#elif __linux || __unix || __posix
#define PLATFORM_Unix

#elif defined(_NSWITCH)
#define PLATFORM_Switch
#endif

typedef uint64_t            u64;
typedef int64_t             i64;
typedef uint32_t            u32;
typedef int32_t             i32;
typedef uint16_t            u16;
typedef int16_t             i16;
typedef uint8_t              u8;
typedef int8_t               i8;
typedef float               f32;
typedef double              f64;

#define STR(...) std::format(__VA_ARGS__)
#define LOG(...) Log(STR(__VA_ARGS__))

void Log(const std::string &msg);

#if defined(_DEBUG)
void Error(const std::string &msg);
inline void Assert(bool cond, const std::string &msg) { if (cond) Error(msg); }
#else
inline void Assert(bool, const std::string& msg) {};
inline void Error(const std::string& msg) {}
#endif

#include "FastDelegate.h"
#include "Memory.h"
