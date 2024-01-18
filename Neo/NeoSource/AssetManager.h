#pragma once

#include "Module.h"
#include "Thread.h"
#include "MemBlock.h"
#include "Serializer.h"

enum AssetType
{
	AssetType_Texture,
	AssetType_Mesh,
	AssetType_Animation,
	AssetType_Database
};

template <>
struct std::formatter<AssetType> : std::formatter<int> {
	constexpr auto parse(std::format_parse_context& ctx) {
		return ctx.begin();
	}
	auto format(const AssetType& obj, std::format_context& ctx) const {
		const char *enumNames[] = { "AssetType:Texture", "AssetType:Mesh", "AssetType:Animation", "AssetType:Database"};
		return std::format_to(ctx.out(), "{}", enumNames[(int)obj] );
	}
};

struct AssetData
{
	virtual ~AssetData() {}

	AssetType m_type;
	u16 m_version;
	string m_name;
	vector<string> m_sourceFiles;

	virtual MemBlock AssetToMemory() = 0;
	virtual void MemoryToAsset(const MemBlock& block) = 0;
};

// asset type info creates all the functions and data needed to create this asset type
class AssetTypeInfo
{
public:
	// function to turn the source files into a single asset file
	std::function<AssetData* (const vector<MemBlock>& srcBlocks)> m_assetCreator;

	// extension for asset file
	string m_assetExt;

	// list of extensions for source files
	// some source files could be one of many extensions (ie.  png, tga, jpg)
	vector<stringlist> m_sourceExt;
};



// callback when resource data has been finally loaded
typedef FastDelegate::FastDelegate1<AssetData*> DeliverAssetDataCB;

class AssetManager : public Module<AssetManager>
{
	// asset delivery thread can handle a maximum number of tasks at once
	WorkerThread<256> m_assetTasks;

	// map of asset type creators
	map<int, AssetTypeInfo*> m_assetTypeInfoMap;

public:
	AssetManager();

	// register an asset type that can be delivered
	void RegisterAssetType(int assetType, AssetTypeInfo* assetCreator) { m_assetTypeInfoMap.insert(std::pair<int, AssetTypeInfo*>(assetType, assetCreator)); }

	// gather all data from file systems
	void DeliverAssetDataAsync(AssetType assetType, const std::string &name, const DeliverAssetDataCB& cb);
};

