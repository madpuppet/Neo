#pragma once

#include "Module.h"
#include "Model.h"

class Application : public Module<Application>
{
public:
	Application();
	virtual ~Application();

	void Update();
	void Draw();

protected:
	ModelRef m_vikingRoom;
};

