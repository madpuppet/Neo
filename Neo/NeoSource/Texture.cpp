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
	Assert(data->m_type == AssetType_Texture, "Bad Asset Type");
	TextureData* texData = dynamic_cast<TextureData*>(data);

}

void Texture::Reload()
{
}
