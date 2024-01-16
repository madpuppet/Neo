#pragma once

#include "Singleton.h"
#include "Thread.h"

// callback when resource data has been finally loaded
typedef FastDelegate::FastDelegate1<class AssetData*> DeliverAssetDataCB;

enum AssetType
{
	AssetType_Texture,
	AssetType_Mesh,
	AssetType_Animation,
	AssetType_Database
};

class AssetData
{
protected:
	std::string m_name;
	std::vector<std::string> m_sourceFiles;
public:
	virtual ~AssetData() {}
};

class TextureData : public AssetData
{
public:
	~TextureData() {}
};


class AssetManager : public Singleton<AssetManager>
{
	WorkerThread<256> m_assetTasks;

public:
	AssetManager();

	// gather all data from file systems
	void DeliverAssetDataAsync(AssetType assetType, const std::string &name, const DeliverAssetDataCB& cb);
};

