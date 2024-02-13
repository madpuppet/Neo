#pragma once

#include "Resource.h"
#include "AssetManager.h"
#include "ResourceRef.h"
#include "Material.h"

struct BitmapFontPlatformData;

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
	void OnAssetDeliver(struct AssetData* data);

	virtual void Reload() override;
	virtual AssetType GetAssetType() const override { return AssetType_BitmapFont; }

	BitmapFontAssetData* m_assetData;

	MaterialRef m_white;

public:
	BitmapFont(const string& name);
	virtual ~BitmapFont();

	BitmapFontAssetData* GetAssetData() { return m_assetData; }

	void RenderText(const string& text, const rect& area, float z, Alignment align, const vec2& scale, const color& col, float dropShadowOffset);
};

// texture factory keeps a map of all the currently created textures
class BitmapFontFactory : public Module<BitmapFontFactory>
{
	map<u64, BitmapFont*> m_resources;

public:
	BitmapFontFactory();

	BitmapFont* Create(const string& name);
	void Destroy(BitmapFont* texture);
};

using BitmapFontRef = ResourceRef<BitmapFont, BitmapFontFactory>;

