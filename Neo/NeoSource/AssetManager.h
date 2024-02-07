#pragma once

#include "Module.h"
#include "Thread.h"
#include "MemBlock.h"
#include "Serializer.h"

enum AssetType
{
	AssetType_Texture,
	AssetType_VertexShader,
	AssetType_PixelShader,
	AssetType_ComputeShader,
	AssetType_StaticMesh,
	AssetType_Animation,
	AssetType_Database,
	AssetType_Material,
	AssetType_BitmapFont,

	AssetType_Extended = 0x100		// game can use asset type values after this point to create their own asset types
};

struct AssetData
{
	AssetData() = default;
	virtual ~AssetData() = default;

	u32 type;		// AssetType (or custom asset type)
	string name;	// for debug purposes
	u16 version;	// increment the version to force a rebuild of assets

	virtual MemBlock AssetToMemory() = 0;
	virtual bool MemoryToAsset(const MemBlock& block) = 0;
	virtual bool SrcFilesToAsset(vector<MemBlock>& srcBlocks, struct AssetCreateParams* params) = 0;
};

// derive options asset creation params off this
// these params will only be used when creating an asset from source files
// if a resource already exists, or is created from data:asset, this will be ignored.
struct AssetCreateParams
{
public:
	~AssetCreateParams() {}
};

// asset type info creates all the functions and data needed to create this asset type
struct AssetTypeInfo
{
	// name of this type - for Log output
	string name;

	// extension for asset file
	string assetExt;

	// function that creates an empty asset of this type
	std::function<AssetData*(void)> assetCreator;

	// list of extensions for source files
	// some source files could be one of many extensions (ie.  png, tga, jpg)
	vector<std::pair<stringlist,bool>> sourceExt;
};

// callback when resource data has been finally loaded
typedef std::function<void(AssetData*)> DeliverAssetDataCB;

class AssetManager : public Module<AssetManager>
{
	// asset delivery thread can handle a maximum number of tasks at once
	WorkerFarm m_assetTasks;

	// map of asset type creators
	hashtable<int, AssetTypeInfo*> m_assetTypeInfoMap;

public:
	AssetManager();

	// kill the worker farm
	void KillWorkerFarm();

	// register an asset type that can be delivered
	void RegisterAssetType(int assetType, AssetTypeInfo* assetCreator) { m_assetTypeInfoMap.insert(std::pair<int, AssetTypeInfo*>(assetType, assetCreator)); }

	// gather all data from file systems
	void DeliverAssetDataAsync(AssetType type, const string &name, AssetCreateParams* params, const DeliverAssetDataCB& cb);

	// get registered asset type info for a specified type
	AssetTypeInfo *FindAssetTypeInfo(int type);
};

// C++20 formatter lets us convert AssetType to string for Log(STR("{}", (AssetType)type));
template <>
struct std::formatter<AssetType> : std::formatter<int> {
	constexpr auto parse(std::format_parse_context& ctx) {
		return ctx.begin();
	}
	auto format(const AssetType& obj, std::format_context& ctx) const {
		auto typeInfo = AssetManager::Instance().FindAssetTypeInfo((int)obj);
		if (typeInfo)
			return std::format_to(ctx.out(), "{}", typeInfo->name);
		else
			return std::format_to(ctx.out(), "??assetType_{}", (int)obj);
	}
};