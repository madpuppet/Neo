#pragma once

#include "Texture.h"

class Application
{
public:
	Application();
	virtual ~Application();

	void Update();
	void Draw();

protected:
	TextureRef m_testTex;
};

