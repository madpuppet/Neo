#include "Neo.h"
#include "Application.h"
#include "RenderThread.h"
#include "TimeManager.h"
#include "DefDynamicRenderer.h"
#include "ImmDynamicRenderer.h"
#include "Shader.h"

DECLARE_MODULE(Application, NeoModuleInitPri_Application, NeoModulePri_Early, NeoModulePri_Early);

const char* GAME_NAME = "TestGame";

//TextureRef tex;

Application::Application()
{
	// mount filesystems
	m_vikingRoom.Create("viking_room");
	RenderThread::Instance().AddDrawTask([this]() {Draw(); }, DrawTaskPri_BasePixel);

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
	m_beeMat.Create("bee");
	m_shader.Create("standard");

	for (auto& bee : m_bees)
	{
		bee.pos.x = (rand() & 0xffff) / 32768.0f - 1.0f;
		bee.pos.y = (rand() & 0xffff) / 32768.0f - 1.0f;
		bee.pos.z = (rand() & 0xffff) / 32768.0f - 1.0f;
		bee.vel.x = (rand() & 0xffff) / 32768.0f - 1.0f;
		bee.vel.y = (rand() & 0xffff) / 32768.0f - 1.0f;
		bee.vel.z = (rand() & 0xffff) / 32768.0f - 1.0f;
	}
}

Application::~Application()
{
}

void Application::Update()
{
	float dt = (float)NeoTimeDelta;
	dt = Min(dt, 0.1f);

	float x = GIL::Instance().GetJoystickAxis(0) * dt * 4.0f;
	float y = -GIL::Instance().GetJoystickAxis(1) * dt * 4.0f;
	float yaw = GIL::Instance().GetJoystickAxis(2) * dt;
	float pitch = GIL::Instance().GetJoystickAxis(3) * dt;

	for (auto& bee : m_bees)
	{
		bee.pos += bee.vel * dt * 0.01f;
		float range = glm::length(bee.pos);
		if (range > 4.0f)
			bee.vel = -bee.pos + vec3(((rand() & 0xff) / 255.0f - 0.5f), ((rand() & 0xff) / 255.0f - 0.5f), ((rand() & 0xff) / 255.0f - 0.5f));
	}

	m_cameraPYR.x += pitch;
	m_cameraPYR.y += yaw;

	mat4x4 pitchMatrix = glm::rotate(glm::mat4(1.0f), m_cameraPYR.x, glm::vec3(1.0f, 0.0f, 0.0f));
	mat4x4 yawMatrix = glm::rotate(glm::mat4(1.0f), m_cameraPYR.y, glm::vec3(0.0f, 1.0f, 0.0f));
	mat4x4 camMatrix = yawMatrix * pitchMatrix;
	
	m_cameraPos += x * vec3(camMatrix[0]) + y * vec3(camMatrix[2]);
	camMatrix[3] = vec4(m_cameraPos, 1.0);
	m_view.SetCameraMatrix(camMatrix);

	// TODO: need this threadsafe
	m_cameraMatrix = camMatrix;
	auto& ddr = DefDynamicRenderer::Instance();
	if (m_particleMat->IsLoaded())
	{
		ddr.BeginRender(0);
		ddr.StartPrimitive(PrimType_TriangleList);
		ddr.UseMaterial(m_particleMat);
		static float time = 0.0f;
		time += dt;
		float width = sinf(time) * 0.05f + 0.05f;
		vec3 right = vec3(camMatrix[0] * width);
		vec3 up = vec3(camMatrix[1] * width);
		for (int i = 0; i < 20000; i++)
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
			ddr.AddVert(pos1, uv1, col);
			ddr.AddVert(pos2, uv2, col);
			ddr.AddVert(pos3, uv3, col);
		}
		ddr.EndPrimitive();
		ddr.EndRender();
	}
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

	auto& ddr = DefDynamicRenderer::Instance();
	ddr.Render(0xffff);

	auto& idr = ImmDynamicRenderer::Instance();
	idr.BeginRender();
	idr.StartPrimitive(PrimType_TriangleList);
	idr.UseMaterial(m_beeMat);
	vec3 right = m_cameraMatrix[0] * 0.1f;
	vec3 up = m_cameraMatrix[1] * 0.1f;
	u32 beeCol = 0xffffffff;
	for (auto& bee : m_bees)
	{
		vec3 bl = bee.pos - right - up;
		vec3 br = bee.pos + right - up;
		vec3 tl = bee.pos - right + up;
		vec3 tr = bee.pos + right + up;

		idr.AddVert(br, { 1,1 }, beeCol);
		idr.AddVert(bl, { 0,1 }, beeCol);
		idr.AddVert(tl, { 0,0 }, beeCol);

		idr.AddVert(br, { 1,1 }, beeCol);
		idr.AddVert(tl, { 0,0 }, beeCol);
		idr.AddVert(tr, { 1,0 }, beeCol);
	}
	idr.EndPrimitive();
	idr.EndRender();

	if (m_font->IsLoaded())
		m_font->RenderText("Test", rect({ 0,0 }, { 1,0.2 }), 0, Alignment_Center, { 0.01f,0.01f }, { 0,0,0,1 });
}

