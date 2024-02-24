#include "neo.h"
#include "Texture.h"
#include "StringUtils.h"
#include "RenderThread.h"
#include <stb_image.h>

#define TEXTURE_VERSION 1

DECLARE_MODULE(TextureFactory, NeoModuleInitPri_TextureFactory, NeoModulePri_None, NeoModulePri_None);

Texture::Texture(const string& name) : Resource(name)
{
	AssetManager::Instance().DeliverAssetDataAsync(AssetType_Texture, name, nullptr, [this](AssetData* data) { OnAssetDeliver(data); });
}

Texture::Texture(const string& name, int width, int height, TexturePixelFormat format) : Resource(name)
{
	m_assetData = new TextureAssetData;
	m_assetData->type = AssetType_Texture;
	m_assetData->name = name;
	m_assetData->version = TEXTURE_VERSION;
	m_assetData->width = width;
	m_assetData->height = height;
	m_assetData->format = format;
	m_assetData->isRenderTarget = true;
	RenderThread::Instance().AddPreDrawTask([this]() { m_platformData = TexturePlatformData_Create(m_assetData); OnLoadComplete(); });
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
		RenderThread::Instance().AddPreDrawTask([this]() { m_platformData = TexturePlatformData_Create(m_assetData); OnLoadComplete(); });
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

template <> ResourceFactory<Texture>::ResourceFactory()
{
	auto ati = new AssetTypeInfo();
	ati->name = "Texture";
	ati->assetExt = ".neotex";
	ati->assetCreator = []() -> AssetData* { return new TextureAssetData; };
	ati->sourceExt.push_back({ { ".png", ".tga", ".jpg" }, true });		// on of these src image files
	ati->sourceExt.push_back({ { ".tex" }, false });						// an optional text file to config how to convert the file
	AssetManager::Instance().RegisterAssetType(AssetType_Texture, ati);
}

bool TextureAssetData::SrcFilesToAsset(vector<MemBlock> &srcFiles, AssetCreateParams* params)
{
	// src image has been altered, so convert it...
	int texWidth, texHeight, texChannels;
	stbi_uc* stbi_uc = stbi_load_from_memory(srcFiles[0].Mem(), (int)srcFiles[0].Size(), &texWidth, &texHeight, &texChannels, STBI_default);

	// pack it into an asset
	width = texWidth;
	height = texHeight;
	switch (texChannels)
	{
		case 1:
			format = PixFmt_R8_UNORM;
			break;
		case 2:
			format = PixFmt_R8G8_UNORM;
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
			format = PixFmt_R8G8B8A8_SRGB;
		}
		break;
		case 4:
			format = PixFmt_R8G8B8A8_SRGB;
			break;
	}

	images.push_back(MemBlock((u8*)stbi_uc, texWidth * texHeight * texChannels, false));
	return true;
}

MemBlock TextureAssetData::AssetToMemory()
{
	Serializer_BinaryWriteGrow stream;
	stream.WriteU16(AssetType_Texture);
	stream.WriteU16(TEXTURE_VERSION);
	stream.WriteString(name);
	stream.WriteU16(width);
	stream.WriteU16(height);
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
		LOG(Texture, STR("Rebuilding {} - bad type {} - expected {}", name, (int)type, (int)AssetType_Texture));
		return false;
	}
	if (version != TEXTURE_VERSION)
	{
		LOG(Texture, STR("Rebuilding {} - old version {} - expected {}", name, version, TEXTURE_VERSION));
		return false;
	}

	width = stream.ReadU16();
	height = stream.ReadU16();
	format = (TexturePixelFormat)stream.ReadU16();
	int miplevels = stream.ReadU16();
	for (int i = 0; i < miplevels; i++)
	{
		images.push_back(stream.ReadMemory());
	}
	return true;
}
