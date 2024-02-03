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
	persp.farPlane = 10.0f;
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
	time += 0.0001f;
	vec3 pos = { 0.0f, -2.0f + sinf(time)*2.0f, 2.0f};
	m_view.SetLookAt(pos, {0,0,0}, {0,1,0});
//	Log(STR("Pos {}", pos.z));
}

void Application::Draw()
{
	// wait till our resources are loaded...
	if (!m_vikingRoom->IsLoaded())
		return;

	m_view.Apply();

	static auto startTime = std::chrono::high_resolution_clock::now();
	auto currentTime = std::chrono::high_resolution_clock::now();
	float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();
	mat4x4 modelMatrix = glm::rotate(glm::mat4(1.0f), time*0.1f, glm::vec3(0.0f, 1.0f, 0.0f));
	modelMatrix[3][2] = 2.0f;
	modelMatrix[3][1] = -0.5f;

	GIL::Instance().SetModelMatrix(modelMatrix);
	GIL::Instance().RenderModel(m_vikingRoom);
}

