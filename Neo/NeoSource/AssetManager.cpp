#include "Neo.h"
#include "AssetManager.h"
#include "FileManager.h"
#include "Thread.h"
#include "Serializer.h"

DECLARE_MODULE(AssetManager, NeoModulePri_AssetManager);

AssetManager::AssetManager() : m_assetTasks(ThreadGUID_AssetManager, string("AssetManager"))
{
	m_assetTasks.Start();
}

static int GetTime(const string name, const stringlist &extensions, u64& time)
{
	auto& fm = FileManager::Instance();
	for (int i = 0; i < extensions.size(); i++)
	{
		if (fm.GetTime(name + extensions[i], time))
			return i;
	}
	return -1;
}

void AssetManager::DeliverAssetDataAsync(AssetType assetType, const string& name, AssetCreateParams* params, const DeliverAssetDataCB& cb)
{
	Assert(m_assetTypeInfoMap.contains(assetType), std::format("Cannot create asset: {} - unregistered asset type: {}", name, (int)assetType));

	auto assetTypeInfo = m_assetTypeInfoMap[assetType];
	m_assetTasks.AddTask
	(
		[assetType, assetTypeInfo, name, cb, params]()
		{
			auto& fm = FileManager::Instance();

			// get datestamp of current asset file
			u64 assetDateStamp = 0;
			string assetDataPath = string("data:") + name + assetTypeInfo->m_assetExt;
			fm.GetTime(assetDataPath, assetDateStamp);

			// get datestamp of each source file
			stringlist srcFiles;
			bool missingSrcFile = false;
			int idx = 0;
			u64 earliestSourceDateStamp = 0;

			for (auto& srcExt : assetTypeInfo->m_sourceExt)
			{
				u64 srcDateStamp;
				int ext = GetTime(string("src:") + name, srcExt.first, srcDateStamp);
				srcFiles.push_back((ext == -1) ? "" : string("src:") + name + srcExt.first[ext]);
				if (srcDateStamp > 0 && (idx == 0 || srcDateStamp < earliestSourceDateStamp))
					earliestSourceDateStamp = srcDateStamp;

				// if this is a non-optional source file and it wasn't found, then we can't convert the asset
				if (srcExt.second && (ext == -1 || srcDateStamp == 0))
				{
					Log(std::format("Asset {} [{}] missing src file {}", name, (int)assetType, idx));
					missingSrcFile = true;
				}
				idx++;
			}

			// missing at least one non-optional source file
			if (missingSrcFile)
			{
				Error("Asset conversion aborted due to missing source files!");
				cb(nullptr);
				return;
			}

			// if asset is newer than source files, just load and createFromData
			if (assetDateStamp > earliestSourceDateStamp)
			{
				MemBlock serializedBlock;
				if (!fm.Read(assetDataPath, serializedBlock))
				{
					Error(std::format("Error reading asset data file: {}\nTry deleting that file and run again.", assetDataPath));
					cb(nullptr);
					return;
				}

				// create from data
				// this can return nullptr if version is old
				Log(STR("Create {} From Data: {}", name, (int)assetType));
				AssetData* assetData = assetTypeInfo->m_assetCreateFromData(serializedBlock);
				if (assetData)
				{
					Log(STR("Deliver Asset: {}", name));
					cb(assetData);
					return;
				}
				Log("Asset was not delivered.. try creating from source...");
			}

			// didn't return assetData, so try building an asset from source
			// first we load each src file into an array of memblocks
			vector<MemBlock> srcFileMem;
			for (auto &src : srcFiles)
			{
				MemBlock memblock;
				if (src.empty())
				{
					// just push an empty memblock - the asset creator should be prepared for these if it had optional src files
					srcFileMem.emplace_back(memblock);
				}
				else
				{
					if (!fm.Read(src, memblock))
					{
						Error(std::format("Asset building error trying to load src: {}", src));
						srcFiles.clear();
						cb(nullptr);
						return;
					}
					srcFileMem.emplace_back(memblock);
				}
			}

			// now create the AssetData from the src files
			Log(STR("Create {} From Source: {}", name, (int)assetType));
			AssetData *assetData = assetTypeInfo->m_assetCreateFromSource(srcFileMem, params);
			assetData->name = name;
			assetData->type = assetType;

			// write out the asset to then data folder
			// write the texture asset to data
			MemBlock serializedBlock = assetData->AssetToMemory();
			Log(STR("Write Asset Data: {} -> {}", name, assetDataPath));
			if (!fm.Write(assetDataPath, serializedBlock))
			{
				// non fatal error, since we have converted the asset ok, we just can't write it
				Error(std::format("Error trying to write asset to path: {}\nCheck disk space and permissions.", assetDataPath));
			}

			// finally we can deliver the asset back to the resource
			Log(STR("Deliver Asset: {}", name));
			cb(assetData);
		}
	);
}

