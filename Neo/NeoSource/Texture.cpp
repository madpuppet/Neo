#include "neo.h"
#include "Texture.h"
#include "StringUtils.h"
#include "RenderThread.h"
#include <stb_image.h>

#define TEXTURE_VERSION 1

DECLARE_MODULE(TextureFactory, NeoModulePri_TextureFactory);

Texture::Texture(const string& name) : Resource(name)
{
	AssetManager::Instance().DeliverAssetDataAsync(AssetType_Texture, name, nullptr, [this](AssetData* data) { OnAssetDeliver(data); });
}

Texture::~Texture()
{
}

void Texture::OnAssetDeliver(AssetData* data)
{
	if (data)
	{
		Assert(data->type == AssetType_Texture, "Bad Asset Type");
		m_assetData = dynamic_cast<TextureAssetData*>(data);
		RenderThread::Instance().AddGILTask([this]() { m_platformData = TexturePlatformData_Create(m_assetData); OnLoadComplete(); });
	}
	else
	{
		m_failedToLoad = true;
		OnLoadComplete();
	}
}

void Texture::Reload()
{
}

TextureFactory::TextureFactory()
{
	auto ati = new AssetTypeInfo();
	ati->m_assetCreateFromData = [](MemBlock memBlock) -> AssetData* { auto assetData = new TextureAssetData; assetData->MemoryToAsset(memBlock); return assetData; };
	ati->m_assetCreateFromSource = [](const vector<MemBlock>& srcBlocks, AssetCreateParams* params) -> AssetData* { return TextureAssetData::Create(srcBlocks, params);  };
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

AssetData* TextureAssetData::Create(vector<MemBlock> srcFiles, AssetCreateParams* params)
{
	// src image has been altered, so convert it...
	int texWidth, texHeight, texChannels;
	stbi_uc* stbi_uc = stbi_load_from_memory(srcFiles[0].Mem(), (int)srcFiles[0].Size(), &texWidth, &texHeight, &texChannels, STBI_default);

	// pack it into an asset
	auto texAsset = new TextureAssetData;
	texAsset->width = texWidth;
	texAsset->height = texHeight;
	texAsset->depth = texChannels;
	switch (texAsset->depth)
	{
		case 1:
			texAsset->format = PixFmt_R8_UNORM;
			break;
		case 2:
			texAsset->format = PixFmt_R8G8_UNORM;
			break;
		case 3:
			// gpu's don't support 3 channel... need to put in a fake ALPHA
		{
			u8* mem = new u8[texWidth * texHeight * 4];
			u8* in = stbi_uc;
			u8* out = mem;
			for (int i = 0; i < texWidth * texHeight; i++)
			{
				*out++ = *in++;
				*out++ = *in++;
				*out++ = *in++;
				*out++ = 0xff;
			}
			delete[] stbi_uc;
			stbi_uc = mem;
			texChannels = 4;
			texAsset->format = PixFmt_R8G8B8A8_UNORM;
		}
		break;
		case 4:
			texAsset->format = PixFmt_R8G8B8A8_UNORM;
			break;
	}

	texAsset->images.push_back(MemBlock((u8*)stbi_uc, texWidth * texHeight * texChannels, false));
	return texAsset;
}

MemBlock TextureAssetData::AssetToMemory()
{
	Serializer_BinaryWriteGrow stream;
	stream.WriteU16(AssetType_Texture);
	stream.WriteU16(TEXTURE_VERSION);
	stream.WriteString(name);
	stream.WriteU16(width);
	stream.WriteU16(height);
	stream.WriteU16(depth);
	stream.WriteU16((u16)format);
	stream.WriteU16((u16)images.size());
	for (auto& block : images)
		stream.WriteMemory(block);

	return MemBlock::CloneMem(stream.DataStart(), stream.DataSize());
}

bool TextureAssetData::MemoryToAsset(const MemBlock& block)
{
	Serializer_BinaryRead stream(block);
	type = (AssetType)stream.ReadU16();
	version = stream.ReadU16();
	name = stream.ReadString();

	if (type != AssetType_Texture)
	{
		Log(STR("Rebuilding {} - bad type {} - expected {}", name, (int)type, (int)AssetType_Texture));
		return false;
	}
	if (version != TEXTURE_VERSION)
	{
		Log(STR("Rebuilding {} - old version {} - expected {}", name, version, TEXTURE_VERSION));
		return false;
	}

	width = stream.ReadU16();
	height = stream.ReadU16();
	depth = stream.ReadU16();
	format = (TexturePixelFormat)stream.ReadU16();
	int miplevels = stream.ReadU16();
	for (int i = 0; i < miplevels; i++)
	{
		images.push_back(stream.ReadMemory());
	}
	return true;
}
