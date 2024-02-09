#include "Neo.h"
#include "Application.h"
#include "RenderThread.h"
#include "TimeManager.h"
#include "DynamicRenderer.h"

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

	m_particleMat.Create("particles");
	m_font.Create("c64");
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

	auto& dr = DynamicRenderer::Instance();
	dr.BeginFrame();
	dr.BeginRender(0);
	dr.UseMaterial(m_particleMat);
	dr.StartPrimitive(PrimType_TriangleList);
	static float time = 0.0f;
	time += dt;
	time = fmodf(time, 2 * PI);
	float width = sinf(time) * 0.01f + 0.01f;
	vec3 right = vec3(camMatrix[0] * width);
	vec3 up = vec3(camMatrix[1] * width);
	for (int i = 0; i < DYNREN_MAXTRIANGLES; i++)
	{
		vec3 pos;
		pos.x = sinf(time * 2.0f + i * 0.003f) * 0.5f + cosf(time + i * 0.01f) * 0.5f;
		pos.y = sinf(time + i * 0.01f) + cosf(time * 4.0f + i * 0.011f) * 0.1f;
		pos.z = sinf(time + i * 0.0233333f) * 1.0f;
		vec3 pos1 = pos + right;
		vec3 pos2 = pos - right;
		vec3 pos3 = pos + up;
		vec2 uv1{ 0,0 };
		vec2 uv2{ 1,0 };
		vec2 uv3{ 0.5f,1.0f };
		u32 col = vec4ToR8G8B8A8({ 1,1,1,1 });
		dr.AddVert(pos1, uv1, col);
		dr.AddVert(pos2, uv2, col);
		dr.AddVert(pos3, uv3, col);
	}
	dr.EndPrimitive();
	dr.EndRender();
	dr.EndFrame();
}

void Application::Draw()
{
	// wait till our resources are loaded...
	if (!m_vikingRoom->IsLoaded())
		return;

	m_view.Apply();
	GIL::Instance().SetModelMatrix(m_modelMatrix);
	GIL::Instance().RenderStaticMesh(m_vikingRoom);

	auto& dr = DynamicRenderer::Instance();
	dr.Render(0xffff);
}

