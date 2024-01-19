#include "Neo.h"
#include "GIL_Vulkan.h"

DECLARE_MODULE(GIL, NeoModulePri_GIL);

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

void GIL::Startup()
{
    // create WINDOW
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK | SDL_INIT_TIMER) < 0)
    {
        Log(STR("SDL could not initialize! SDL Error: {}\n", SDL_GetError()));
        exit(0);
    }

    m_window = SDL_CreateWindow(APP_TITLE "v" VERSION, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, WIDTH, HEIGHT, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_VULKAN);
}

void GIL::Shutdown()
{
}

void GIL::BeginFrame()
{
}

void GIL::EndFrame()
{
}



