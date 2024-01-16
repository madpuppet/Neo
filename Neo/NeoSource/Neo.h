#pragma once

#include <string>
#include <format>
#include <cstdint>
#include <functional>
#include <sys/stat.h>
#include <sys/types.h>
#include <vector>
#include <unordered_map>
#include <map>
#include <algorithm>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/hash.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

#if defined(_WIN32)
#define PLATFORM_Windows
#define WIN32_LEAN_AND_MEAN
#include "windows.h"
#include "shlobj.h"
#include <io.h>
// some macros clash with glm
#undef max
#undef min
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

typedef uint64_t        u64;
typedef int64_t         i64;
typedef uint32_t        u32;
typedef int32_t         i32;
typedef uint16_t        u16;
typedef int16_t         i16;
typedef uint8_t         u8;
typedef int8_t          i8;
typedef float           f32;
typedef double          f64;

typedef glm::vec2       vec2;
typedef glm::vec3		vec3;
typedef glm::vec4       vec4;
typedef glm::quat       quat;
typedef glm::ivec2      ivec2;
typedef glm::ivec3      ivec3;
typedef glm::ivec4      ivec4;
typedef glm::mat3x4     mat3x4;
typedef glm::mat4x4     mat4x4;

using string = std::string;
template<typename T> using array = std::vector<T>;
template<typename K, typename D> using treemap = std::map<K, D>;
template<typename K, typename D> using hashtable = std::unordered_map<K, D>;

#define STR(...) std::format(__VA_ARGS__)
#define LOG(...) Log(STR(__VA_ARGS__))

void Log(const string &msg);

#if defined(_DEBUG)
void Error(const string &msg);
inline void Assert(bool cond, const string &msg) { if (!cond) Error(msg); }
#else
inline void Assert(bool, const string& msg) {};
inline void Error(const string& msg) {}
#endif

#include "FastDelegate.h"
#include "Memory.h"

extern const char* GAME_NAME;
