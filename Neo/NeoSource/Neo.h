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
#include <set>
#include <unordered_set>
#include <array>
#include <algorithm>
#include <deque>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#define GLM_FORCE_LEFT_HANDED
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/hash.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

//===============================================================
//========== WINDOWS ============================================
//===============================================================
#if defined(_WIN32) 
#define PLATFORM_Windows
#define GRAPHICS_Vulkan
#define WIN32_LEAN_AND_MEAN
#include "windows.h"
#include "shlobj.h"
#include <io.h>
// some macros clash with glm
#undef max
#undef min

//===============================================================
//========== IOS/MACOS ==========================================
//===============================================================
#elif defined(__APPLE__)
#import "Foundation/Foundation.h"
#include "TargetConditionals.h"
#if TARGET_OS_IPHONE
#define PLATFORM_IOS
#define GRAPHICS_Metal
#elif TARGET_OS_MAC
#define PLATFORM_OSX
#define GRAPHICS_Metal
#endif

//===============================================================
//========== ANDROID ============================================
//===============================================================
#elif defined(ANDROID)
#define PLATFORM_Android
#define GRAPHICS_Vulkan

//===============================================================
//========== LINUX ==============================================
//===============================================================
#elif __linux || __unix || __posix
#define PLATFORM_Unix
#define GRAPHICS_Vulkan

//===============================================================
//========== NSWITCH ============================================
//===============================================================
#elif defined(_NSWITCH)
#define PLATFORM_Switch
#define GRAPHICS_NVN
#endif

#define APP_TITLE "NEO"
#define VERSION "2023.01"

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
typedef glm::mat4x4     mat4x4;
typedef glm::vec4       color;

typedef std::function<void(void)> GenericCallback;		// generic callback that takes a void* and returns void

template <typename T>
struct rectangle
{
	T min;
	T size;

	bool contains(const T& pos) const { return pos.x >= min.x && pos.y >= min.y && pos.x < min.x + size.x && pos.y < min.y + size.y; }
	bool overlaps(rectangle<T> o) const
	{
		return min.x < (o.min.x + o.size.x) && (min.x + size.x) >= o.min.x && min.y < (o.min.y + o.size.y) && (min.y + size.y) >= o.min.y;
	}
	void translate(const T& offset) { min.x += offset.x; min.y += offset.y; };
};
using rect = rectangle<vec2>;
using irect = rectangle<ivec2>;

template <typename T>
struct boundingVolume
{
	T min;
	T size;

	bool contains(const T& pos) const { return pos.x >= min.x && pos.y >= min.y && pos.z >= min.z && pos.x < min.x + size.x && pos.y < min.y + size.y && pos.z < min.z + size.z; }
	bool overlaps(rectangle<T> o) const
	{
		return min.x < (o.min.x + o.size.x) && (min.x + size.x) >= o.min.x 
			&& min.y < (o.min.y + o.size.y) && (min.y + size.y) >= o.min.y
			&& min.z < (o.min.z + o.size.z) && (min.z + size.z) >= o.min.z;
	}
	void translate(const T& offset) { min.x += offset.x; min.y += offset.y; min.z += offset.z; };
};
using volume = boundingVolume<vec3>;
using ivolume = boundingVolume<ivec3>;


// neo containers... some are renamed from STL for more legibility
template<typename T> using vector = std::vector<T>;
template<typename T, size_t S> using array = std::array<T, S>;
template<typename K, typename D> using map = std::map<K, D>;
template<typename K, typename D> using hashtable = std::unordered_map<K, D>;
template<typename D> using set = std::set<D>;
template<typename D> using hashset = std::unordered_set<D>;
template<typename T> using fifo = std::deque<T>;
using string = std::string;
using stringlist = std::vector<string>;

// this used by any callback system that allows adding & removing callbacks
typedef u64             CallbackHandle;
CallbackHandle AllocUniqueCallbackHandle();

#define STR(...) std::format(__VA_ARGS__)

#if defined(_DEBUG)
void Error(const string &msg);
inline void Assert(bool cond, const string &msg) { if (!cond) Error(msg); }
#else
inline void Assert(bool, const string& msg) {};
inline void Error(const string& msg) {}
#endif

#include "Memory.h"
#include "Module.h"
#include "CmdLineVar.h"
#include "Log.h"
#include "GIL_Common.h"

extern const char* GAME_NAME;
