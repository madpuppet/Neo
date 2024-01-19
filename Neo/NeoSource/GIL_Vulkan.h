#pragma once

#if defined(GRAPHICS_Vulkan)

// Graphics Interface Layer (GIL) for Windows
//
// This is a vulkan layer to run on windows platform - abstracts all vulkan interfaces from the rest of the engine
// generally all these functions should only be running on the GraphicsInterface thread
//
// Note: I may make this a vulkan layer if it turns out vulkan compatibilty between multiple platforms is nearly 100%
//       in which case I'll add a GIL_Vulkan header/class and have multiple platforms just include that

#include <SDL.h>
#include <SDL_vulkan.h>
#include <vulkan/vulkan.h>

class GIL : public Module<GIL>
{
public:
	void Startup();
	void Shutdown();

	void BeginFrame();
	void EndFrame();

protected:
	SDL_Window* m_window;
};

#endif // GRAPHICS_Vulkan



