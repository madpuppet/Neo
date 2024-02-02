#include "Neo.h"
#include "Application.h"
#include "RenderThread.h"

DECLARE_MODULE(Application, NeoModulePri_Application);

const char* GAME_NAME = "TestGame";

//TextureRef tex;

Application::Application()
{
	// mount filesystems
	m_vikingRoom.Create("viking_room");
	RenderThread::Instance().AddDrawTask([this]() {Draw();}, 0);
}

Application::~Application()
{
}

void Application::Update()
{
}

void Application::Draw()
{
	// wait till our resources are loaded...
	if (!m_vikingRoom->IsLoaded())
		return;

	GIL::Instance().RenderModel(m_vikingRoom);
}

