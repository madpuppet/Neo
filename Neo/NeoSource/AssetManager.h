#pragma once

#include "Singleton.h"
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
	array<string> m_sourceFiles;

	virtual MemBlock AssetToMemory() = 0;
	virtual void MemoryToAsset(const MemBlock& block) = 0;
};

class TextureData : public AssetData
{
public:
	~TextureData() {}

	u16 m_width;
	u16 m_height;
	u16 m_depth;
	array<MemBlock> m_images;		// one for each mip level

	MemBlock AssetToMemory() override
	{
		Serializer_BinaryWriteGrow stream;
		stream.WriteU16(AssetType_Texture);
		stream.WriteU16(0);
		stream.WriteU16(m_width);
		stream.WriteU16(m_height);
		stream.WriteU16(m_depth);
		stream.WriteU16((u16)m_images.size());
		for (auto& block : m_images)
			stream.WriteMemory(block);

		return MemBlock::CloneMem(stream.DataStart(), stream.DataSize());
	}
	void MemoryToAsset(const MemBlock& block) override
	{
		Serializer_BinaryRead stream(block);
		m_type = (AssetType)stream.ReadU16();
		Assert(m_type == AssetType_Texture, std::format("Bad texture asset type - got {}, expected {}!", (int)m_type, AssetType_Texture));

		m_version = stream.ReadU16();
		m_width = stream.ReadU16();
		m_height = stream.ReadU16();
		m_depth = stream.ReadU16();
		int miplevels = stream.ReadU16();
		for (int i = 0; i < miplevels; i++)
		{
			m_images.push_back(stream.ReadMemory());
		}
	}
};

// callback when resource data has been finally loaded
typedef FastDelegate::FastDelegate1<AssetData*> DeliverAssetDataCB;

class AssetManager : public Singleton<AssetManager>
{
	WorkerThread<256> m_assetTasks;

public:
	AssetManager();

	// gather all data from file systems
	void DeliverAssetDataAsync(AssetType assetType, const std::string &name, const DeliverAssetDataCB& cb);
};

