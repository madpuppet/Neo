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
#include "DefDynamicRenderer.h"

CmdLineVar<stringlist> CLV_LogFilter("log", "select log filters to show", { "" });

int main(int argc, char* argv[])
{
    Thread::RegisterThread(ThreadGUID_Main, "MainThread");

    gMemoryTracker.EnableTracking(true);
    NeoParseCommandLine(argc, argv);
    if (CLV_LogFilter.Exists())
        NeoSetLogFilters(CLV_LogFilter.Value());

    NeoDumpCmdLineVars();
    NeoStartupModules();
    RenderThread::Instance().DoStartupTasks();

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

                case SDL_WINDOWEVENT:
                    if (e.window.event == SDL_WINDOWEVENT_RESIZED) 
                    {
                        int newWidth = e.window.data1;
                        int newHeight = e.window.data2;
                        if (GIL::Exists())
                        {
                            GIL::Instance().ResizeFrameBuffers(newWidth, newHeight);
                        }
                    }
                    break;

            }
        }

        auto& dr = DefDynamicRenderer::Instance();
        dr.BeginFrame();
        NeoUpdateModules();
        dr.EndFrame();

        RenderThread::Instance().SignalUpdateDone();
        RenderThread::Instance().WaitDrawStarted();
    }
    
    gMemoryTracker.Dump();

    // kill our threads first, so they aren't using memory that will be freed up by other module shutdowns
    AssetManager::Instance().KillWorkerFarm();
    RenderThread::Instance().StopAndWait();

    NeoShutdownModules();
    return EXIT_SUCCESS;
}

#if defined(_DEBUG)

void Error(const string& msg)
{
    NOMEMTRACK();
    string errorStr = string("ERROR: ") + msg + "\n";
    printf("%s", errorStr.c_str());
#if defined(PLATFORM_Windows)
    OutputDebugStringA(errorStr.c_str());
#endif

    if (Thread::IsOnThread(ThreadGUID_Main) && GIL::Exists())
    {
        if (GIL::Instance().ShowMessageBox(errorStr))
            __debugbreak();
    }
    else
    {
        __debugbreak();
    }
}

#endif

#include <atomic>
static std::atomic<u64> s_global_unique_callback(0);
CallbackHandle AllocUniqueCallbackHandle()
{
    u64 result = s_global_unique_callback.fetch_add(1, std::memory_order_relaxed);
    return result;
}


