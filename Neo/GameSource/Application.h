#pragma once

#include "Module.h"
#include "StaticMesh.h"
#include "View.h"
#include "BitmapFont.h"
#include "Shader.h"
#include "Thread.h"
#include "RenderPass.h"

enum ApplicationThreads
{
	GameThreadGUID_UpdateWorkerThread = ThreadGUID_MAX
};

struct Bee
{
	vec3 pos;
	vec3 vel;
};

class Application : public Module<Application>
{
public:
	Application();
	virtual ~Application();

	void Update();
	void Draw();
	void Shutdown();

	void RenderParticles();
	void RenderRooms();
	void RenderBees();
	void RenderUI();

protected:
	WorkerFarm m_workerFarm;

	StaticMeshRef m_vikingRoom;
	MaterialRef m_vikingRoomMat;
	View m_view;
	vec3 m_cameraPYR = { 0,0,0 };
	vec3 m_cameraPos = { 0,0,0 };
	MaterialRef m_particleMat;
	BitmapFontRef m_font;
	MaterialRef m_beeMat;
	ShaderRef m_shader;
	
	RenderPassRef m_rpMain;
	RenderPassRef m_rpBee;
	RenderPassRef m_rpUI;

	mat4x4 m_cameraMatrix;
	float m_beeScale;
	array<Bee, 20000> m_bees;

	static const int roomGridSize = 5;
	mat4x4 m_roomInstances[roomGridSize * roomGridSize];
};

