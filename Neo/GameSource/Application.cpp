#include "Neo.h"
#include "Application.h"
#include "RenderThread.h"
#include "TimeManager.h"
#include "DefDynamicRenderer.h"
#include "ImmDynamicRenderer.h"
#include "Shader.h"
#include "ResourceLoadedManager.h"
#include "Profiler.h"

DECLARE_MODULE(Application, NeoModuleInitPri_Application, NeoModulePri_Early);

const char* GAME_NAME = "TestGame";

//TextureRef tex;

Application::Application() : m_workerFarm(GameThreadGUID_UpdateWorkerThread, "Update Worker", 4, true)
{
	View::PerspectiveInfo persp;
	persp.fov = DegToRad(70.0f);
	persp.nearPlane = 0.25f;
	persp.farPlane = 100.0f;
	m_view.SetPerspective(persp);
	m_cameraPYR = { DegToRad(45.0f), 0, 0 };
	m_cameraPos = { 0, 1.5f, -1.0f };

	m_rpBee.Create("bee");
	m_rpBee->SetView(&m_view);
	m_rpBee->AddTask([this]() {RenderRooms(); });

	m_rpMain.Create("main");
	m_rpMain->AddTask([this]() {RenderBees(); });
	m_rpMain->AddTask([this]() {RenderParticles(); });
	m_rpMain->AddTask([this]() {RenderRooms(); });

	m_rpUI.Create("ui");
	m_rpUI->AddTask([this]() {RenderUI(); });

	m_renderScene.Create("standard");
	PROFILE_ADD_RENDER(m_rpUI);

	// ensure the render scene creation (and renderpasses) complete before any materials are created that might bind to render pass targets
	AssetManager::Instance().AddBarrier();

	m_vikingRoom.Create("viking_room");
	m_vikingRoomMat.Create("viking_room");

	m_renderScene->Apply();

	for (int x = 0; x < roomGridSize; x++)
	{
		for (int y = 0; y < roomGridSize; y++)
		{
			u32 idx = x + y * roomGridSize;
			m_roomInstances[idx] = mat4x4(1);
			m_roomInstances[idx][3][0] = x*1.44f;
			m_roomInstances[idx][3][2] = y*1.44f;
		}
	}

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

	m_workerFarm.StartWork();
}

Application::~Application()
{
}

void Application::Shutdown()
{
	m_workerFarm.KillWorkers();
}

void Application::Update()
{
	PROFILE_CPU("App::Update");

	float dt = (float)NeoTimeDelta;
	dt = Min(dt, 0.1f);

	float x = GIL::Instance().GetJoystickAxis(0) * dt * 4.0f;
	float y = -GIL::Instance().GetJoystickAxis(1) * dt * 4.0f;
	float yaw = GIL::Instance().GetJoystickAxis(2) * dt;
	float pitch = GIL::Instance().GetJoystickAxis(3) * dt;

	m_workerFarm.AddTask([this, dt]()
		{
			PROFILE_CPU("BEES");
			for (auto& bee : m_bees)
			{
				bee.pos += bee.vel * dt;
				float range = glm::length(bee.pos);
				if (range > 20.0f)
					bee.vel = -bee.pos * 0.1f + vec3(((rand() & 0xff) / 255.0f - 0.5f), ((rand() & 0xff) / 255.0f - 0.5f), ((rand() & 0xff) / 255.0f - 0.5f));
			}
		}
	);

	m_cameraPYR.x += pitch;
	m_cameraPYR.y += yaw;

	mat4x4 pitchMatrix = glm::rotate(glm::mat4(1.0f), m_cameraPYR.x, glm::vec3(1.0f, 0.0f, 0.0f));
	mat4x4 yawMatrix = glm::rotate(glm::mat4(1.0f), m_cameraPYR.y, glm::vec3(0.0f, 1.0f, 0.0f));
	mat4x4 camMatrix = yawMatrix * pitchMatrix;
	
	m_cameraPos += x * vec3(camMatrix[0]) + y * vec3(camMatrix[2]);
	camMatrix[3] = vec4(m_cameraPos, 1.0);
	m_view.SetCameraMatrix(camMatrix);

	View::OrthographicInfo orthoInfo;
	orthoInfo.orthoRect = { 0.0f,0.0f,1280.0f,720.0f };
	orthoInfo.nearPlane = 0.0f;
	orthoInfo.farPlane = 1.0f;
	m_view.SetOrthographic(orthoInfo);

	// TODO: need this threadsafe
	m_cameraMatrix = camMatrix;

	static float time = 0.0f;
	time += dt;
	m_beeScale = sinf(time) * 0.1f + 0.11f;

	m_workerFarm.AddTask([this, dt, camMatrix]()
		{
			PROFILE_CPU("PARTICLES");
			auto& ddr = DefDynamicRenderer::Instance();
			if (m_particleMat->IsLoaded())
			{
				ddr.BeginRender(0);
				ddr.StartPrimitive(PrimType_TriangleList);
				ddr.UseMaterial(m_particleMat);
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
		);

	while (!m_workerFarm.AllTasksComplete());
}

void Application::RenderParticles()
{
	if (!m_particleMat->IsLoaded())
		return;

	auto& gil = GIL::Instance();
	UBO_Model modelData;
	auto modelUBOInstance = ShaderManager::Instance().FindUBO("UBO_Model")->dynamicInstance;
	modelData.model = mat4x4(1);
	gil.UpdateUBOInstance(modelUBOInstance, &modelData, sizeof(modelData), true);

	{
		PROFILE_GPU("PARTICLES");
		auto& ddr = DefDynamicRenderer::Instance();
		ddr.Render(0xffff);
	}
}

void Application::RenderRooms()
{
	if (!m_vikingRoom->IsLoaded())
		return;

	auto& gil = GIL::Instance();
	UBO_Model modelData;
	auto modelUBOInstance = ShaderManager::Instance().FindUBO("UBO_Model")->dynamicInstance;
	modelData.model = mat4x4(1);
	gil.UpdateUBOInstance(modelUBOInstance, &modelData, sizeof(modelData), false);

	{
		PROFILE_GPU("ROOMS");
		vec4 col1{ 1.0f, 0.0f, 0.0f, 1.0f };
		m_vikingRoomMat->SetUniform_vec4("blendColor", col1, true);
		gil.RenderStaticMeshInstances(m_vikingRoom, m_roomInstances, roomGridSize * roomGridSize / 2);

		vec4 col2{ 1.0f, 1.0f, 1.0f, 1.0f };
		m_vikingRoomMat->SetUniform_vec4("blendColor", col2, true);

		gil.RenderStaticMeshInstances(m_vikingRoom, &m_roomInstances[roomGridSize * roomGridSize / 2], roomGridSize * roomGridSize / 2);
	}

	modelData.model = mat4x4(1);
	gil.UpdateUBOInstance(modelUBOInstance, &modelData, sizeof(modelData), false);
}

void Application::RenderBees()
{
	if (!m_beeMat->IsLoaded())
		return;

	PROFILE_GPU("BEES");
	auto& idr = ImmDynamicRenderer::Instance();
	idr.BeginRender();
	idr.StartPrimitive(PrimType_TriangleList);
	idr.UseMaterial(m_beeMat);
	vec3 right = m_cameraMatrix[0] * m_beeScale;
	vec3 up = m_cameraMatrix[1] * m_beeScale;
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
}

void Application::RenderUI()
{
	if (!m_font->IsLoaded())
		return;

	int fps = (int)(1.0f / NeoTimeDelta);
	int ms = (int)(1000.0f * NeoTimeDelta);
	m_font->RenderText(STR("{}ms {}fps", ms, fps), rect(20.0f, 680.0f, 500.0f, 20.0f), 0.0f, Alignment_CenterLeft, { 2.0f,2.0f }, { 1,1,1,1 }, -3.0f);
}


