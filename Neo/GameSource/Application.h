#pragma once

#include "Module.h"
#include "StaticMesh.h"
#include "View.h"
#include "BitmapFont.h"
#include "Shader.h"

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

protected:
	StaticMeshRef m_vikingRoom;
	View m_view;
	vec3 m_cameraPYR = { 0,0,0 };
	vec3 m_cameraPos = { 0,0,0 };
	MaterialRef m_particleMat;
	BitmapFontRef m_font;
	MaterialRef m_beeMat;
	ShaderRef m_shader;

	mat4x4 m_cameraMatrix;
	float m_beeScale;
	array<Bee, 20000> m_bees;
	mat4x4 m_roomInstances[64 * 64];
};

