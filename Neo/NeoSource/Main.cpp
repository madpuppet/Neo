#include "neo.h"

#include <SDL.h>
#include <SDL_vulkan.h>
#include <vulkan/vulkan.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

#include <iostream>
#include <fstream>
#include <stdexcept>
#include <algorithm>
#include <chrono>
#include <cstring>
#include <cstdlib>
#include <cstdint>
#include <limits>
#include <optional>

#include "Thread.h"
#include "FileManager.h"
#include "AssetManager.h"

#include "Application.h"  //TODO: we shouldn't know about Application - it should just itself to a generic update callback
#include "RenderThread.h"

int main(int argc, char* args[])
{
    gMemoryTracker.EnableTracking(true);
    StartupModules();

    bool m_quit = false;
    SDL_Event e;
    while (!m_quit)
    {
        while (SDL_PollEvent(&e))
        {
            switch (e.type)
            {
                case SDL_QUIT:
                    m_quit = true;
                    break;
            }
        }
        Application::Instance().Update();
        RenderThread::Instance().SignalUpdateDone();
        RenderThread::Instance().WaitDrawStarted();
    }
    
    gMemoryTracker.Dump();
    ShutdownModules();
    return EXIT_SUCCESS;
}

void Log(const string& msg)
{
    NOMEMTRACK();
    string outStr = msg + "\n";
    printf("%s", outStr.c_str());
#if defined(PLATFORM_Windows)
    OutputDebugString(outStr.c_str());
#endif

}

#if defined(_DEBUG)

void Error(const string &msg)
{
    NOMEMTRACK();
    string errorStr = string("ERROR: ") + msg + "\n";
    printf("%s", errorStr.c_str());
#if defined(PLATFORM_Windows)
    OutputDebugStringA(errorStr.c_str());
#endif
    if (GIL::Instance().ShowMessageBox(errorStr))
        __debugbreak();
}

#endif

#include <atomic>
static std::atomic<u64> s_global_unique_callback(0);
CallbackHandle AllocUniqueCallbackHandle()
{
    u64 result = s_global_unique_callback.fetch_add(1, std::memory_order_relaxed);
    return result;
}


