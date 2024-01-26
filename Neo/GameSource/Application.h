#pragma once

#include "Material.h"

class Application
{
public:
	Application();
	virtual ~Application();

	void Update();
	void Draw();

protected:
	MaterialRef m_vikingRoom;
};

