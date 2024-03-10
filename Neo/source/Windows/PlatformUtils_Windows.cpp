#include "Neo.h"
#include "PlatformUtils_Windows.h"
#include <SDL.h>
#include "RenderThread.h"
#include "GIL.h"

DECLARE_MODULE(PlatformUtils, NeoModuleInitPri_PlatformUtils, NeoModulePri_None)

bool PlatformUtils::PollSystemEvents()
{
    SDL_Event e;
    while (SDL_PollEvent(&e))
    {
        switch (e.type)
        {
        case SDL_QUIT:
            return true;

        case SDL_WINDOWEVENT:
            if (e.window.event == SDL_WINDOWEVENT_RESIZED)
            {
                ivec2 newSize{ e.window.data1,e.window.data2 };
                if (RenderThread::Exists())
                {
                    // at the start fo the next render frame, we will recreate everything
                    RenderThread::Instance().AddPreDrawTask([newSize]()
                    {
                        // need to wait for gpu to be idle so its not using any resources we are about to recreate
                        GIL::Instance().WaitForGPU();
                        // destroy the render pass first since it references the swap chain in its framebuffers
                        RenderPassFactory::Instance().DestroyPlatformData();
                        // destroy and recreate the swapchain
                        GIL::Instance().ResizeSwapChain(newSize);
                        // create the render passes
                        RenderPassFactory::Instance().CreatePlatformData();
                        // finally recreate all the graphics pipelines since they reference viewport sizes of the render passes
                        MaterialFactory::Instance().OnSwapChainResize();
                    });
                }
            }
            break;

        }
    }
    return false;
}

bool PlatformUtils::ShowMessageBox(const string& string)
{
    SDL_MessageBoxButtonData buttons[2];
    buttons[0].flags = SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT;
    buttons[0].buttonid = 1;
    buttons[0].text = "Break";
    buttons[1].flags = SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT;
    buttons[1].buttonid = 0;
    buttons[1].text = "Ignore";

    SDL_MessageBoxData data;
    data.flags = SDL_MESSAGEBOX_ERROR;
    data.window = m_window;
    data.title = "Neo Assert Hit...";
    data.message = string.c_str();
    data.numbuttons = 2;
    data.buttons = buttons;
    data.colorScheme = nullptr;
    int result = 0;
    SDL_ShowMessageBox(&data, &result);
    return result == 1;
}

float PlatformUtils::GetJoystickAxis(int idx)
{
    float joyval = Clamp(SDL_JoystickGetAxis(m_joystick, idx) / 32767.0f, -1.0f, 1.0f);
    if (abs(joyval) < 0.1f)
        joyval = 0.0f;
    return joyval;
}

void PlatformUtils::Startup()
{
    m_window = GIL::Instance().Window();
    m_joystick = SDL_JoystickOpen(0);
}
