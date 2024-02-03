#pragma once

/*
	Texture -> actual resource ready to use for rendering
	TextureRef -> reference a resource and use it to create/destroy it in a refcounted way
	TextureFactory -> keeps a cache of all created textures resources
	TextureAssetData -> this serializes the texture data to/from disk
	TexturePlatformData -> platform specific (ie. vulkan/metal) data for a texture
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
	PixFmt_Undefined,

	PixFmt_R4G4_UNORM,
	PixFmt_R4G4B4A4_UNORM,
	PixFmt_R5G6B5_UNORM,
	PixFmt_R5G6B5A1_UNORM,

	PixFmt_R8_UNORM,
	PixFmt_R8_SNORM,
	PixFmt_R8_UINT,
	PixFmt_R8_SINT,
	PixFmt_R8_SRGB,

	PixFmt_R8G8_UNORM,
	PixFmt_R8G8_SNORM,
	PixFmt_R8G8_UINT,
	PixFmt_R8G8_SINT,

	PixFmt_R8G8B8A8_UNORM,
	PixFmt_R8G8B8A8_SNORM,
	PixFmt_R8G8B8A8_UINT,
	PixFmt_R8G8B8A8_SINT,
	PixFmt_R8G8B8A8_SRGB,

	PixFmt_R16_UNORM,
	PixFmt_R16_SNORM,
	PixFmt_R16_UINT,
	PixFmt_R16_SINT,
	PixFmt_R16_SFLOAT,

	PixFmt_B10G11R11_UFLOAT,
	
	PixFmt_BC1_RGB_UNORM,
	PixFmt_BC1_RGB_SRGB,
	PixFmt_BC3_RGBA_UNORM,
	PixFmt_BC3_RGBA_SRGB,

	PixFmt_D32_SFLOAT,
	PixFmt_D24_UNORM_S8_UINT
};

// Asset data is the file data for this asset
// this class managed serializing to and from disk
struct TextureAssetData : public AssetData
{
public:
	~TextureAssetData() {}

	virtual MemBlock AssetToMemory() override;
	virtual bool MemoryToAsset(const MemBlock& block) override;
	virtual bool SrcFilesToAsset(const vector<MemBlock>& srcBlocks, struct AssetCreateParams* params) override;

	u16 width;
	u16 height;
	u16 depth;
	TexturePixelFormat format;
	vector<MemBlock> images;		// one for each mip level
};

// texture is the game facing class that represents any type of texture  (zbuffer, rendertarget, image)
class Texture : public Resource
{
	void OnAssetDeliver(struct AssetData *data);

	virtual void Reload() override;
	virtual AssetType GetAssetType() const override { return AssetType_Texture; }

	int m_width;
	int m_height;
	int m_depth;
	TextureType m_textureType;
	TexturePixelFormat m_texturePixelFormat;
	TextureAssetData* m_assetData;
	struct TexturePlatformData* m_platformData;

public:
	Texture(const string& name);
	virtual ~Texture();

	TextureAssetData* GetAssetData() { return m_assetData; }
	TexturePlatformData* GetPlatformData() { return m_platformData; }
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

