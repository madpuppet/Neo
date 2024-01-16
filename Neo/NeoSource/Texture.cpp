#include "neo.h"
#include "Texture.h"

Texture::Texture(const std::string& name) : Resource(name)
{
	AssetManager::Instance().DeliverAssetDataAsync(AssetType_Texture, name, DELEGATE(Texture::OnAssetDeliver));
}

Texture::~Texture()
{
}

void Texture::OnAssetDeliver(AssetData* data)
{
	TextureData* texData = dynamic_cast<TextureData*>(data);

}

void Texture::Reload()
{
}
