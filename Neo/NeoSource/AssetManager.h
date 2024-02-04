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
	AssetType_Material
};

template <>
struct std::formatter<AssetType> : std::formatter<int> {
	constexpr auto parse(std::format_parse_context& ctx) {
		return ctx.begin();
	}
	auto format(const AssetType& obj, std::format_context& ctx) const {
		const char* enumNames[] = { "Texture", "VertexShader", "PixelShader", "ComputeShader", "Model", "Animation", "Database", "Material" };
		return std::format_to(ctx.out(), "{}", enumNames[(int)obj]);
	}
};

struct AssetData
{
	AssetData() = default;
	virtual ~AssetData() = default;

	AssetType type;
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
class AssetTypeInfo
{
public:
	// function that creates an empty asset of this type
	std::function<AssetData*(void)> m_assetCreator;

	// extension for asset file
	string m_assetExt;

	// list of extensions for source files
	// some source files could be one of many extensions (ie.  png, tga, jpg)
	vector<std::pair<stringlist,bool>> m_sourceExt;
};

// callback when resource data has been finally loaded
typedef std::function<void(AssetData*)> DeliverAssetDataCB;

class AssetManager : public Module<AssetManager>
{
	// asset delivery thread can handle a maximum number of tasks at once
	WorkerFarm m_assetTasks;

	// map of asset type creators
	map<int, AssetTypeInfo*> m_assetTypeInfoMap;

public:
	AssetManager();

	// register an asset type that can be delivered
	void RegisterAssetType(int assetType, AssetTypeInfo* assetCreator) { m_assetTypeInfoMap.insert(std::pair<int, AssetTypeInfo*>(assetType, assetCreator)); }

	// gather all data from file systems
	void DeliverAssetDataAsync(AssetType type, const string &name, AssetCreateParams* params, const DeliverAssetDataCB& cb);
};

