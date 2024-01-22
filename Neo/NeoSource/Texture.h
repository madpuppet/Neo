#pragma once

/*
	Texture -> actual resource ready to use for rendering
	TextureRef -> reference a resource and use it to create/destroy it in a refcounted way
	TextureFactory -> keeps a cache of all created textures resources
	TextureData -> this serializes the texture data to/from disk
*/

#include "Resource.h"
#include "AssetManager.h"
#include "ResourceRef.h"

enum TextureType
{
	TextureType_None,
	TextureType_Image,
	TextureType_ColorBuffer,
	TextureType_DepthBuffer
};

enum TexturePixelFormat
{
	TexturePixelFormat_Automatic,
	TexturePixelFormat_RGBA8888,
	TexturePixelFormat_RGB888,
	TexturePixelFormat_RGB565,
	TexturePixelFormat_RGBA4444,
	TexturePixelFormat_A8,
	TexturePixelFormat_PVRTC2_RGB,
	TexturePixelFormat_PVRTC2_RGBA,
	TexturePixelFormat_PVRTC4_RGB,
	TexturePixelFormat_PVRTC4_RGBA,
	TexturePixelFormat_DEPTH32
};

// Asset data is the file data for this asset
// this class managed serializing to and from disk
class TextureAssetData : public AssetData
{
public:
	~TextureAssetData() {}

	static AssetData* Create(vector<MemBlock> srcFiles);

	u16 m_width;
	u16 m_height;
	u16 m_depth;
	vector<MemBlock> m_images;		// one for each mip level

	virtual MemBlock AssetToMemory() override
	{
		Serializer_BinaryWriteGrow stream;
		stream.WriteU16(AssetType_Texture);
		stream.WriteU16(0);
		stream.WriteU16(m_width);
		stream.WriteU16(m_height);
		stream.WriteU16(m_depth);
		stream.WriteU16((u16)m_images.size());
		for (auto& block : m_images)
			stream.WriteMemory(block);

		return MemBlock::CloneMem(stream.DataStart(), stream.DataSize());
	}
	virtual void MemoryToAsset(const MemBlock& block) override
	{
		Serializer_BinaryRead stream(block);
		m_type = (AssetType)stream.ReadU16();
		Assert(m_type == AssetType_Texture, std::format("Bad texture asset type - got {}, expected {}!", (int)m_type, AssetType_Texture));

		m_version = stream.ReadU16();
		m_width = stream.ReadU16();
		m_height = stream.ReadU16();
		m_depth = stream.ReadU16();
		int miplevels = stream.ReadU16();
		for (int i = 0; i < miplevels; i++)
		{
			m_images.push_back(stream.ReadMemory());
		}
	}
};

// texture is the game facing class that represents any type of texture  (zbuffer, rendertarget, image)
class Texture : public Resource
{
	void OnAssetDeliver(struct AssetData *data);
	virtual void Reload() override;

	int m_width;
	int m_height;
	int m_depth;
	TextureType m_textureType;
	TexturePixelFormat m_texturePixelFormat;
	class TextureAssetData* m_assetData;
	class TexturePlatformData* m_platformData;

public:
	Texture(const string& name);
	virtual ~Texture();
};

// texture factory keeps a map of all the currently created textures
class TextureFactory : public Module<TextureFactory>
{
	map<u64, Texture*> m_resources;

public:
	TextureFactory();

	Texture* Create(const string& name);
	void Destroy(Texture* texture);
};

// texture references is a scoped pointer to a texture.  the texture will be destroyed with the TextureRef goes out of scope or is destroyed
// you can create differnet types of textures through this using
//
// TextureRef myTex;
// myTex->Create("name");
//
using TextureRef = ResourceRef<Texture, TextureFactory>;

