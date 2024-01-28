#include "Neo.h"
#include "Application.h"
#include "RenderThread.h"

DECLARE_MODULE(Application, NeoModulePri_Application);

const char* GAME_NAME = "TestGame";

//TextureRef tex;

Application::Application()
{
#if NEW_CODE
	// mount filesystems
	m_vikingRoom.Create("viking_room");
	RenderThread::Instance().AddDrawTask([this]() {Draw();}, 0);
#endif
}

Application::~Application()
{
}

void Application::Update()
{
}

void Application::Draw()
{
#if NEW_CODE
	// wait till our resources are loaded...
	if (!m_vikingRoom->IsLoaded())
		return;

	GIL::Instance().RenderModel(m_vikingRoom);
#endif
}

