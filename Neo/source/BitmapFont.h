#pragma once

#include "Resource.h"
#include "AssetManager.h"
#include "ResourceRef.h"
#include "Material.h"

struct BitmapFontPlatformData;

//<REFLECT>
enum Alignment
{
	Alignment_TopLeft,
	Alignment_TopCenter,
	Alignment_TopRight,

	Alignment_CenterLeft,
	Alignment_Center,
	Alignment_CenterRight,

	Alignment_BottomLeft,
	Alignment_BottomCenter,
	Alignment_BottomRight
};


// Asset data is the file data for this asset
// this class managed serializing to and from disk
struct BitmapFontAssetData : public AssetData
{
public:
	~BitmapFontAssetData() {}

	struct FontInfo
	{
		string face;
		string charset;
		int size = 0;
		int stretchH = 0;
		bool bold = false;
		bool italic = false;
		bool unicode = false;
		bool smooth = false;
		bool aa = false;
		bool outline = false;
		bool packed = false;
		bool alphaChnl = false;
		ivec4 padding{};
		ivec2 spacing{};
		int lineHeight = 0;
		int base = 0;
		int scaleW = 0;
		int scaleH = 0;
		int pages = 0;
		int redChnl = 0;
		int greenChnl = 0;
		int blueChnl = 0;
	} info;

	struct PageInfo
	{
		string file;
		MaterialRef material;
	};
	vector<PageInfo> pages;

	struct CharInfo
	{
		int id = 0;
		ivec2 pos{};
		ivec2 size{};
		ivec2 offset{};
		int xadvance = 0;
		int page = 0;
		int channel = 0;
	};
	hashtable<u32, CharInfo> chars;

	virtual MemBlock AssetToMemory() override;
	virtual bool MemoryToAsset(const MemBlock& block) override;
	virtual bool SrcFilesToAsset(vector<MemBlock>& srcBlocks, struct AssetCreateParams* params) override;
};

// texture is the game facing class that represents any type of texture  (zbuffer, rendertarget, image)
class BitmapFont : public Resource
{
	virtual void Reload() override;

	BitmapFontAssetData* m_assetData = nullptr;

	MaterialRef m_white;

public:
	static const string AssetType;
	virtual const string& GetType() const { return AssetType; }

	virtual ~BitmapFont() {}
	void OnAssetDeliver(struct AssetData* data);

	BitmapFontAssetData* GetAssetData() { return m_assetData; }

	void RenderText(const string& text, const rect& area, float z, Alignment align, const vec2& scale, const color& col, float dropShadowOffset);
};

class BitmapFontFactory : public ResourceFactory<BitmapFont>, public Module<BitmapFontFactory> {};
using BitmapFontRef = ResourceRef<BitmapFont, BitmapFontFactory>;

