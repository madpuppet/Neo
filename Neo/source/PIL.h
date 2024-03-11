#pragma once

// Platform Interface Layer
//
// These are miscellaneous utility methods that require platform specific implementations
//

#if defined(PLATFORM_Windows)
#include "PIL_Windows.h"
#elif defined(PLATFORM_MacOS)
#include "PIL_MacOS.h"
#elif defined(PLATFORM_Switch)
#include "PIL_Switch.h"
#endif
