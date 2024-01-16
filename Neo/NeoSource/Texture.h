#pragma once

#include "Resource.h"
#include "AssetManager.h"

enum TextureType
{
	TextureType_None,
	TextureType_Image,
	TextureType_ColorBuffer,
	TextureType_DepthBuffer
};

class Texture : public Resource
{
	void OnAssetDeliver(struct AssetData *data);
	virtual void Reload() override;

public:
	Texture(const std::string& name);
	virtual ~Texture();
};

