#pragma once

enum PrimType
{
    PrimType_Unknown = -1,
    PrimType_PointList = 0,
    PrimType_LineList,
    PrimType_LineStrip,
    PrimType_TriangleList,
    PrimType_TriangleStrip,
    PrimType_TriangleFan
};

inline u32 vec4ToR8G8B8A8(const vec4& col) {
    return (u32)(col.r * 255) + ((u32)(col.g * 255) << 8) + ((u32)(col.b * 255) << 16) + ((u32)(col.a * 255) << 24);
}

#if defined(GRAPHICS_Vulkan)
#include "GIL_Vulkan.h"
#elif defined(GRAPHICS_NVN)
//#include "GIL_NVN.h"
#elif defined(GRAPHICS_Metal)
//#include "GIL_Metal.h"
#endif

