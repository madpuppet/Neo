#pragma once

#if defined(GRAPHICS_Vulkan)
#include "GIL_Vulkan.h"
#elif defined(GRAPHICS_NVN)
//#include "GIL_NVN.h"
#elif defined(GRAPHICS_Metal)
//#include "GIL_Metal.h"
#endif

// platform data creation methods - these implemented differently per platform
extern class TexturePlatformData* TexturePlatformData_Create(class TextureAssetData* assetData);
extern void TexturePlatformData_Destroy(class TexturePlatformData* platformData);
