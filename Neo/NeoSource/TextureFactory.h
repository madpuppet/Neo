#pragma once

#include "Singleton.h"
#include "Texture.h"
#include "ResourceRef.h"

class TextureFactory : public Singleton<TextureFactory>
{
	std::map<u64, Texture*> m_resources;

public:
	Texture* Create(const std::string& name);
	void Destroy(Texture* texture);
};

using TextureRef = ResourceRef<Texture, TextureFactory>;


