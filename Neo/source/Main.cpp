#include "neo.h"
#include "PlatformUtils.h"

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

#include "RenderThread.h"
#include "DefDynamicRenderer.h"
#include "RenderPass.h"
#include "Material.h"

u32 NeoUpdateFrameIdx = 0;
u32 NeoDrawFrameIdx = 0;

CmdLineVar<stringlist> CLV_LogFilter("log", "select log filters to show", { "" });

int main(int argc, char* argv[])
{
    Thread::RegisterThread(ThreadGUID_Main, "Main");

    gMemoryTracker.EnableTracking(true);
    NeoParseCommandLine(argc, argv);
    if (CLV_LogFilter.Exists())
        NeoSetLogFilters(CLV_LogFilter.Value());

    NeoDumpCmdLineVars();
    NeoStartupModules();
    RenderThread::Instance().DoStartupTasks();

    bool m_quit = false;
    while (!m_quit)
    {
        m_quit = PlatformUtils::Instance().PollSystemEvents();

        auto& dr = DefDynamicRenderer::Instance();
        dr.BeginFrame();
        NeoExecuteBeginUpdateTasks();
        NeoUpdateModules();
        dr.EndFrame();

        RenderThread::Instance().SignalUpdateDone();
        RenderThread::Instance().WaitDrawStarted();

        NeoUpdateFrameIdx = 1 - NeoUpdateFrameIdx;
    }
    
    gMemoryTracker.Dump();

    // kill our threads first, so they aren't using memory that will be freed up by other module shutdowns
    AssetManager::Instance().KillWorkerFarm();
    RenderThread::Instance().StopAndWait();

    NeoShutdownModules();
    return EXIT_SUCCESS;
}

#if ASSERTS_ENABLED

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
        if (PlatformUtils::Instance().ShowMessageBox(errorStr))
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

TaskList s_beginUpdateTasks;
int NeoAddBeginUpdateTask(GenericCallback callback, int priority)
{
    return s_beginUpdateTasks.Add(callback, priority);
}

void NeoRemoveBeginUpdateTask(int handle)
{
    s_beginUpdateTasks.Remove(handle);
}

void NeoExecuteBeginUpdateTasks()
{
    s_beginUpdateTasks.Execute();
}
