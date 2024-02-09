#pragma once

#include "Module.h"
#include "StaticMesh.h"
#include "View.h"
#include "BitmapFont.h"

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
	mat4x4 m_modelMatrix;
	vec3 m_cameraPYR = { 0,0,0 };
	vec3 m_cameraPos = { 0,0,0 };
	MaterialRef m_particleMat;
	BitmapFontRef m_font;
};

