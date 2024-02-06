#include "Neo.h"
#include "Application.h"
#include "RenderThread.h"
#include "TimeManager.h"

DECLARE_MODULE(Application, NeoModuleInitPri_Application, NeoModulePri_Early, NeoModulePri_Early);

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

	m_view.SetViewport({ { 0,0 }, { 1, 1 } });
	m_view.SetDepthRange(0.1f, 1.0f);
	m_view.SetScissorRect({ { 0,0 }, { 1,1 } });

	m_modelMatrix = mat4x4(1);

	m_cameraPYR = { DegToRad(45.0f), 0, 0};
	m_cameraPos = { 0, 1.5f, -1.0f };
}

Application::~Application()
{
}

void Application::Update()
{
	float dt = (float)NeoTimeDelta;
	float x = GIL::Instance().GetJoystickAxis(0) * dt;
	float y = -GIL::Instance().GetJoystickAxis(1) * dt;
	float yaw = GIL::Instance().GetJoystickAxis(2) * dt;
	float pitch = GIL::Instance().GetJoystickAxis(3) * dt;

	m_cameraPYR.x += pitch;
	m_cameraPYR.y += yaw;

	mat4x4 pitchMatrix = glm::rotate(glm::mat4(1.0f), m_cameraPYR.x, glm::vec3(1.0f, 0.0f, 0.0f));
	mat4x4 yawMatrix = glm::rotate(glm::mat4(1.0f), m_cameraPYR.y, glm::vec3(0.0f, 1.0f, 0.0f));
	mat4x4 camMatrix = yawMatrix * pitchMatrix;
	
	m_cameraPos += x * vec3(camMatrix[0]) + y * vec3(camMatrix[2]);
	camMatrix[3] = vec4(m_cameraPos, 1.0);
	m_view.SetCameraMatrix(camMatrix);
}

void Application::Draw()
{
	// wait till our resources are loaded...
	if (!m_vikingRoom->IsLoaded())
		return;

	m_view.Apply();
	GIL::Instance().SetModelMatrix(m_modelMatrix);
	GIL::Instance().RenderStaticMesh(m_vikingRoom);
}

