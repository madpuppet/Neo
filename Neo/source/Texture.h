#pragma once

/*
	Texture -> actual resource ready to use for rendering
	TextureRef -> reference a resource and use it to create/destroy it in a refcounted way
	TextureFactory -> keeps a cache of all created textures resources
	TextureAssetData -> this serializes the texture data to/from disk
	TexturePlatformData -> platform specific (ie. vulkan/metal) data for a texture
*/

#include "Resource.h"
#include "ResourceFactory.h"
#include "AssetManager.h"
#include "ResourceRef.h"

enum TextureLayout
{
	TextureLayout_Undefined,		// initial undefined layout
	TextureLayout_TransferDest,		// destination of a copy command
	TextureLayout_ColorAttachment,	// render pass color attachment
	TextureLayout_DepthAttachment,	// render pass depth attachment
	TextureLayout_ShaderRead,		// shader sampling resource
	TextureLayout_Present			// presentation (for swapchain images)
};

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

	PixFmt_B8G8R8A8_UNORM,
	PixFmt_B8G8R8A8_SNORM,
	PixFmt_B8G8R8A8_UINT,
	PixFmt_B8G8R8A8_SINT,
	PixFmt_B8G8R8A8_SRGB,

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
TexturePixelFormat StringToTexturePixelFormat(const string& str);


// Asset data is the file data for this asset
// this class managed serializing to and from disk
struct TextureAssetData : public AssetData
{
public:
	~TextureAssetData() {}

	virtual MemBlock AssetToMemory() override;
	virtual bool MemoryToAsset(const MemBlock& block) override;
	virtual bool SrcFilesToAsset(vector<MemBlock>& srcBlocks, struct AssetCreateParams* params) override;

	bool isRenderTarget = false;
	u16 width = 0;
	u16 height = 0;
	TexturePixelFormat format = PixFmt_Undefined;
	vector<MemBlock> images;		// one for each mip level
};

// texture is the game facing class that represents any type of texture  (zbuffer, rendertarget, image)
class Texture : public Resource
{
	virtual void Reload() override;

	TextureAssetData* m_assetData;
	struct TexturePlatformData* m_platformData;

	// current layout determines how this texture is currently being used
	// we need to change the layout to use 
	TextureLayout m_currentLayout;

public:
	void OnAssetDeliver(struct AssetData* data);
	void InitRenderTarget(const string& name, int width, int height, TexturePixelFormat format);
	void SetLayout(TextureLayout newLayout);

	static const string AssetType;
	virtual const string& GetType() const { return AssetType; }
	virtual ~Texture();

	TextureAssetData* GetAssetData() { return m_assetData; }
	TexturePlatformData* GetPlatformData() { return m_platformData; }
};

// texture factory keeps a map of all the currently created textures
class TextureFactory : public ResourceFactory<Texture>, public Module<TextureFactory>
{
public:
	Texture* CreateRenderTarget(const string& name, int width, int height, TexturePixelFormat format);
};

class TextureRef : public ResourceRef<Texture, TextureFactory>
{
public:
	void CreateRenderTarget(const string& name, int width, int height, TexturePixelFormat format)
	{
		Destroy();
		m_ptr = TextureFactory::Instance().CreateRenderTarget(name, width, height, format);
	}
};
