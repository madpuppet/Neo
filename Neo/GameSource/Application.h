#pragma once

#include "Texture.h"
#include "Shader.h"

class Application
{
public:
	Application();
	virtual ~Application();

	void Update();
	void Draw();

protected:
	TextureRef m_testTex;
	ShaderRef m_testVertexShader;
	ShaderRef m_testPixelShader;
};

