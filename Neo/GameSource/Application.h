#pragma once

#include "Model.h"

class Application
{
public:
	Application();
	virtual ~Application();

	void Update();
	void Draw();

protected:
	ModelRef m_vikingRoom;
};

