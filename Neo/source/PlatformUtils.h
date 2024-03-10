#pragma once

#if defined(PLATFORM_Windows)
#include "PlatformUtils_Windows.h"
#elif defined(PLATFORM_MacOS)
#include "PlatformUtils_MacOS.h"
#elif defined(PLATFORM_Switch)
#include "PlatformUtils_Switch.h"
#endif
