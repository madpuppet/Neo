#pragma once

#include "Module.h"
#include "Thread.h"
#include "MemBlock.h"
#include "Serializer.h"

struct AssetData
{
	AssetData() = default;
	virtual ~AssetData() = default;

	string type;	// asset type name (ie. Material, Texture)
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
	hashtable<string, AssetTypeInfo*> m_assetTypeInfoMap;

public:
	AssetManager();

	// start worker farm
	void StartWork();

	// add barrier to ensure all previous assets complete before others start
	// this is important for renderPasses to complete first since they must create all the render targets before materials try to access them
	// if a material runs first, it will try to load the texture since it didn't find the render target
	void AddBarrier();

	// kill the worker farm
	void KillWorkerFarm();

	// register an asset type that can be delivered
	void RegisterAssetType(AssetTypeInfo* assetCreator) { m_assetTypeInfoMap[assetCreator->name] = assetCreator; }

	// gather all data from file systems
	void DeliverAssetDataAsync(const string &type, const string &name, AssetCreateParams* params, const DeliverAssetDataCB& cb);

	// get registered asset type info for a specified type
	AssetTypeInfo *FindAssetTypeInfo(const string& type);
};
