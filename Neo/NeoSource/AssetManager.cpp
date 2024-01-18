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

static int GetTime(const string name, const vector<string> &extensions, u64& time)
{
	auto& fm = FileManager::Instance();
	for (int i = 0; i < extensions.size(); i++)
	{
		if (fm.GetTime(name + extensions[i], time))
			return i;
	}
	return -1;
}

void AssetManager::DeliverAssetDataAsync(AssetType assetType, const string& name, const DeliverAssetDataCB& cb)
{
//	Assert(m_assetTypeInfoMap.contains(assetType), std::format("Cannot create asset: {} - unregistered asset type: {}", name, assetType));

	auto assetTypeInfo = m_assetTypeInfoMap[assetType];
	m_assetTasks.AddTask
	(
		[assetType, assetTypeInfo, name, cb]()
		{
			auto& fm = FileManager::Instance();

			// get datestamp of current asset file
			u64 assetDateStamp = 0;
			string assetDataPath = string("data:") + name + assetTypeInfo->m_assetExt;
			fm.GetTime(assetDataPath, assetDateStamp);

			// get datestamp of each source file
			vector<string> srcFiles;
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
					Log(std::format("Asset {} [{}] missing src file {}", name, assetType, idx));
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

			// if no asset, or any src file is newer than the asset, we need to build the asset
			if (assetDateStamp == 0 || earliestSourceDateStamp < assetDateStamp)
			{
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
				AssetData *assetData = assetTypeInfo->m_assetCreateFromSource(srcFileMem);

				// write out the asset to then data folder
						// write the texture asset to data
				MemBlock serializedBlock = assetData->AssetToMemory();
				if (!fm.Write(assetDataPath, serializedBlock))
				{
					// non fatal error, since we have converted the asset ok, we just can't write it
					Error(std::format("Error trying to write asset to path: {}\nCheck disk space and permissions.", assetDataPath));
				}

				// finally we can deliver the asset back to the resource
				cb(assetData);
			}

			// asset is newer than its src files, so just load and serve the asset
			else
			{
				MemBlock serializedBlock;
				if (!fm.Read(assetDataPath, serializedBlock))
				{
					Error(std::format("Error reading asset data file: {}\nTry deleting that file and run again.", assetDataPath));
					cb(nullptr);
					return;
				}

				AssetData* assetData = assetTypeInfo->m_assetCreateFromData(serializedBlock);
				cb(assetData);
			}
		}
	);
}

