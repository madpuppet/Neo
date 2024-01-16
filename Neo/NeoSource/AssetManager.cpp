#include "Neo.h"
#include "AssetManager.h"
#include "FileManager.h"
#include "Thread.h"
#include <stb_image.h>

AssetManager::AssetManager() : m_assetTasks(ThreadGUID_AssetManager, string("AssetManager"))
{
	m_assetTasks.Start();
}

int GetTime(const string name, const array<string> extensions, u64& time)
{
	auto& fm = FileManager::Instance();
	for (int i = 0; i < extensions.size(); i++)
	{
		if (fm.GetTime(name + extensions[i], time))
			return i;
	}
	return -1;
}

static array<string> s_textureExtensions = { ".png", ".tga", ".jpg" };
void AssetManager::DeliverAssetDataAsync(AssetType assetType, const string& name, const DeliverAssetDataCB& cb)
{
	m_assetTasks.AddTask
	(
		[assetType, name, cb]()
		{
			auto& fm = FileManager::Instance();
			switch (assetType)
			{
				case AssetType_Texture:
				{
					// get time of current data asset
					u64 assetTime = 0;
					fm.GetTime(string("data:") + name + ".neotex", assetTime);

					// get time of child assets
					u64 srcImageTime = 0;
					int ext = GetTime(string("src:") + name, s_textureExtensions, srcImageTime);

					// no valid src asset - deliver null asset
					if (ext == -1 || srcImageTime == 0)
					{
						Error(std::format("No valid source asset found for texture: {}", name));
						cb(nullptr);
						return;
					}

					// no asset, or src image is newer than asset
					else if (assetTime == 0 || srcImageTime < assetTime)
					{
						MemBlock memblock;
						string srcFile = string("src:") + name + s_textureExtensions[ext];
						if (!fm.Read(srcFile, memblock))
						{
							Error(std::format("Error trying to load image: {}", srcFile));
							cb(nullptr);
							return;
						}

						// src image has been altered, so convert it...
						int texWidth, texHeight, texChannels;
						stbi_uc* image = stbi_load_from_memory(memblock.Mem(), memblock.Size(), &texWidth, &texHeight, &texChannels, STBI_default);


						//					stbi_uc* pixels = stbi_load(TEXTURE_PATH.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
					}

				}
				break;

				default:
					break;
					// todo more asset types
			}
		}
	);
}

