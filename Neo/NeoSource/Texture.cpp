#include "neo.h"
#include "Texture.h"
#include "StringUtils.h"

DECLARE_MODULE(TextureFactory, NeoModulePri_TextureFactory);

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
	m_assetData = dynamic_cast<TextureAssetData*>(data);
	m_platformData = TexturePlatformData_Create(m_assetData);
}

void Texture::Reload()
{
}

TextureFactory::TextureFactory()
{
	auto ati = new AssetTypeInfo();
	ati->m_assetCreator = [](const vector<MemBlock>& srcBlocks) -> AssetData* { return TextureAssetData::Create(srcBlocks);  };
	ati->m_assetExt = ".neotex";
	ati->m_sourceExt.push_back({ ".png", ".tga", ".jpg" });
	AssetManager::Instance().RegisterAssetType(AssetType_Texture, ati);
}

Texture* TextureFactory::Create(const std::string& name)
{
	u64 hash = StringHash64(name);
	auto it = m_resources.find(hash);
	if (it == m_resources.end())
	{
		Texture* resource = new Texture(name);
		m_resources.insert(std::pair<u64, Texture*>(hash, resource));

		return resource;
	}
	it->second->IncRef();
	return it->second;
}

void TextureFactory::Destroy(Texture* texture)
{
	if (texture && texture->DecRef() == 0)
	{
		u64 hash = StringHash64(texture->GetName());
		m_resources.erase(hash);
		delete texture;
	}
}

AssetData* TextureAssetData::Create(vector<MemBlock> srcFiles)
{
	return nullptr;
}

