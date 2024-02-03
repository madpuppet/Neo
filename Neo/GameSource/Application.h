#pragma once

#include "Module.h"
#include "Model.h"
#include "View.h"

class Application : public Module<Application>
{
public:
	Application();
	virtual ~Application();

	void Update();
	void Draw();

protected:
	ModelRef m_vikingRoom;
	View m_view;
	mat4x4 m_modelMatrix;
};

