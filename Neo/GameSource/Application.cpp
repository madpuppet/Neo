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

#if 0
	auto& dr = DynamicRenderer::Instance();
	if (m_particleMat->IsLoaded())
	{
		dr.BeginRender(0);
		dr.StartPrimitive(PrimType_LineList);
		dr.UseMaterial(m_particleMat);
		static float time = 0.0f;
		time += dt;
		float width = sinf(time) * 0.05f + 0.05f;
		vec3 right = vec3(camMatrix[0] * width);
		vec3 up = vec3(camMatrix[1] * width);
		for (int i = 0; i < 10000; i++)
		{
			float rnd = (rand() & 0xff) / 2550.0f;
			vec3 pos;
			pos.x = sinf(time * 0.1f + i * 0.111f) + cosf(time * 0.1f + i * 0.111f) + rnd;
			pos.y = sinf(time * 0.1f + i * 0.112f) + cosf(time * 0.1f + i * 0.112f) + rnd;
			pos.z = sinf(time * 0.1f + i * 0.113f) + cosf(time * 0.1f + i * 0.113f) + rnd;
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
	}
#endif

	if (m_font->IsLoaded())
		m_font->RenderText("Test", rect({ 0,0 }, { 1,0.2 }), 0, Alignment_Center, { 0.01f,0.01f }, { 0,0,0,1 });
}

void Application::Draw()
{
	// wait till our resources are loaded...
	if (!m_vikingRoom->IsLoaded())
		return;

	m_view.Apply();
	GIL::Instance().SetMaterialBlendColor({ 1,1,1,1 });

	for (int x = 0; x < 10; x++)
	{
		for (int y = 0; y < 10; y++)
		{
			m_modelMatrix[3][0] = x*1.4f;
			m_modelMatrix[3][2] = y*1.4f;
			GIL::Instance().SetAndBindModelMatrix(m_modelMatrix);
			GIL::Instance().RenderStaticMesh(m_vikingRoom);
		}
	}

	auto& dr = DynamicRenderer::Instance();
	dr.Render(0xffff);
}

