#include "Neo.h"
#include "Application.h"

const char* GAME_NAME = "TestGame";

//TextureRef tex;

Application::Application()
{
#if NEW_CODE
	// mount filesystems
	m_vikingRoom.Create("viking_room");
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
///	if (tex->IsLoaded())
//	{
//		Camera c;
//		c.SetProjectionOrtho(-1, 1, -1, 1);
//		c.SetLTWMatrix(glm::mat3x4);

//		Neo::PushView(v);
//
//		Material m;
//		m->Use();

//		Renderer r;
//		r.AddQuad("10,10,10,10");
//		r.Render();



//	}
}

