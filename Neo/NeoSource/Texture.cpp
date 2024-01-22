#include "neo.h"
#include "Texture.h"
#include "StringUtils.h"
#include "GraphicsThread.h"
#include <stb_image.h>

DECLARE_MODULE(TextureFactory, NeoModulePri_TextureFactory);

Texture::Texture(const string& name) : Resource(name)
{
	AssetManager::Instance().DeliverAssetDataAsync(AssetType_Texture, name, [this](AssetData* data) { OnAssetDeliver(data); } );
}

Texture::~Texture()
{
}

void Texture::OnAssetDeliver(AssetData* data)
{
	Assert(data->m_type == AssetType_Texture, "Bad Asset Type");
	m_assetData = dynamic_cast<TextureAssetData*>(data);

	GraphicsThread::Instance().AddNonRenderTask([this]() { m_platformData = TexturePlatformData_Create(m_assetData); });
}

void Texture::Reload()
{
}

TextureFactory::TextureFactory()
{
	auto ati = new AssetTypeInfo();
	ati->m_assetCreateFromData = [](MemBlock memBlock) -> AssetData* { auto assetData = new TextureAssetData; assetData->MemoryToAsset(memBlock); return assetData; };
	ati->m_assetCreateFromSource = [](const vector<MemBlock>& srcBlocks) -> AssetData* { return TextureAssetData::Create(srcBlocks);  };
	ati->m_assetExt = ".neotex";
	ati->m_sourceExt.push_back({ { ".png", ".tga", ".jpg" }, true });		// on of these src image files
	ati->m_sourceExt.push_back({ { ".tex" }, false });						// an optional text file to config how to convert the file
	AssetManager::Instance().RegisterAssetType(AssetType_Texture, ati);
}

Texture* TextureFactory::Create(const string& name)
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
	// src image has been altered, so convert it...
	int texWidth, texHeight, texChannels;
	stbi_uc* stbi_uc = stbi_load_from_memory(srcFiles[0].Mem(), (int)srcFiles[0].Size(), &texWidth, &texHeight, &texChannels, STBI_default);

	// pack it into an asset
	auto texAsset = new TextureAssetData;
	texAsset->m_width = texWidth;
	texAsset->m_height = texHeight;
	texAsset->m_depth = texChannels;
	texAsset->m_images.push_back(MemBlock((u8*)stbi_uc, texWidth * texHeight * texChannels, false));
	return texAsset;
}

