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
	RenderThread::Instance().AddDrawTask([this]() {Draw(); }, 0);

	View::PerspectiveInfo persp;
	persp.fov = DegToRad(70.0f);
	persp.nearPlane = 0.25f;
	persp.farPlane = 50.0f;
	m_view.SetPerspective(persp);

	m_view.SetViewport({ { 0,0 }, { 800, 600 } });
	m_view.SetDepthRange(0.1f, 1.0f);
	m_view.SetScissorRect({ { 0,0 }, { 800,600 } });
}

Application::~Application()
{
}

void Application::Update()
{
	static float time = 0.0f;
	time += 0.001f;
	vec3 eye{ sinf(time)*1.0f, 1.5f, cosf(time) * 1.0f };
	vec3 target{ 0,0,0 };
	vec3 up{ 0,1,0 };
	m_view.SetLookAt(eye, target, up);

	m_modelMatrix = glm::rotate(glm::mat4(1.0f), time*0.1f, glm::vec3(0.0f, 1.0f, 0.0f));
}

void Application::Draw()
{
	// wait till our resources are loaded...
	if (!m_vikingRoom->IsLoaded())
		return;

	m_view.Apply();
	GIL::Instance().SetModelMatrix(m_modelMatrix);
	GIL::Instance().RenderModel(m_vikingRoom);
}

