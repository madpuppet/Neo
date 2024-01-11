#pragma once

#include <string>
#include <format>
#include <cstdint>
#include <functional>
#include <vector>
#include <map>

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

#include "String.h"

void Log(const String&);

#if defined(_DEBUG)

#define Assert(expr, msg) { if (expr) Error(msg); }
void Error(const String&);

#else

inline void Assert(bool, const String&) {}
inline void Error(const String&) {}

#endif

template<typename T, typename... U>
size_t getAddress(std::function<T(U...)> f) {
    typedef T(fnType)(U...);
    fnType** fnPointer = f.template target<fnType*>();
    return (size_t)*fnPointer;
}

#include "Array.h"
#include "FastDelegate.h"

