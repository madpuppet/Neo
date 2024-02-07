#include "neo.h"
#include "BitmapFont.h"
#include "StringUtils.h"
#include "RenderThread.h"
#include <stb_image.h>

#define BITMAPFONT_VERSION 1

DECLARE_MODULE(BitmapFontFactory, NeoModuleInitPri_BitmapFontFactory, NeoModulePri_None, NeoModulePri_None);

BitmapFont::BitmapFont(const string& name) : Resource(name)
{
	AssetManager::Instance().DeliverAssetDataAsync(AssetType_BitmapFont, name, nullptr, [this](AssetData* data) { OnAssetDeliver(data); });
}

BitmapFont::~BitmapFont()
{
}

void BitmapFont::OnAssetDeliver(AssetData* data)
{
	if (data)
	{
		Assert(data->type == AssetType_BitmapFont, "Bad Asset Type");
		m_assetData = dynamic_cast<BitmapFontAssetData*>(data);
		RenderThread::Instance().AddPreDrawTask([this]() { m_platformData = BitmapFontPlatformData_Create(m_assetData); OnLoadComplete(); });
	}
	else
	{
		m_failedToLoad = true;
		OnLoadComplete();
	}
}

void BitmapFont::Reload()
{
}

BitmapFontFactory::BitmapFontFactory()
{
	auto ati = new AssetTypeInfo();
	ati->name = "BitmapFont";
	ati->assetExt = ".neobmf";
	ati->assetCreator = []() -> AssetData* { return new BitmapFontAssetData; };
	ati->sourceExt.push_back({ { ".fnt", }, true });		// on of these src image files
	AssetManager::Instance().RegisterAssetType(AssetType_BitmapFont, ati);
}

BitmapFont* BitmapFontFactory::Create(const string& name)
{
	u64 hash = StringHash64(name);
	auto it = m_resources.find(hash);
	if (it == m_resources.end())
	{
		BitmapFont* resource = new BitmapFont(name);
		m_resources.insert(std::pair<u64, BitmapFont*>(hash, resource));

		return resource;
	}
	it->second->IncRef();
	return it->second;
}

void BitmapFontFactory::Destroy(BitmapFont* BitmapFont)
{
	if (BitmapFont && BitmapFont->DecRef() == 0)
	{
		u64 hash = StringHash64(BitmapFont->GetName());
		m_resources.erase(hash);
		delete BitmapFont;
	}
}

class LineParser
{
	char m_token[256];
	char m_value[256];
	char* m_mem;
	char* m_end;
	char* m_next;

public:
	LineParser(MemBlock &block)
	{
		m_mem = (char*)block.Mem();
		m_end = m_mem + block.Size();
	}

	// grab token & value --  return TRUE if we have not hit eol yet
	bool GrabTokenValue(string &token, vector<string> &values)
	{
		values.clear();

		// skip white space
		while (m_mem < m_end && (*m_mem == ' ' || *m_mem == '\t'))
			m_mem++;

		// token
		int tokenIdx = 0;
		bool inQuotes = false;
		while (m_mem < m_end && tokenIdx < 255 && *m_mem != 13 && *m_mem != 10 && (inQuotes || (*m_mem != ',' && *m_mem != ' ' && *m_mem != '=')))
		{
			if (*m_mem == '"')
				inQuotes = !inQuotes;
			else
				m_token[tokenIdx++] = *m_mem++;
			m_mem++;
		}
		m_token[tokenIdx] = 0;
		token = string(m_token);

		if (*m_mem == '=')
		{
			*m_mem++;
			bool moreValues = false;
			do
			{
				moreValues = false;

				// skip white space but not EOLs
				while (m_mem < m_end && (*m_mem == ' ' || *m_mem == '\t'))
					m_mem++;

				// value
				int valueIdx = 0;
				while (m_mem < m_end && valueIdx < 255 && *m_mem != 13 && *m_mem != 10 && (inQuotes || (*m_mem != ' ' && *m_mem != '\t' && *m_mem != ',')))
				{
					if (*m_mem == '"')
						inQuotes = !inQuotes;
					else
						m_value[valueIdx++] = *m_mem++;
					m_mem++;
				}
				m_value[valueIdx] = 0;
				values.emplace_back(string(m_value));

				// skip white space but not EOLs
				while (m_mem < m_end && (*m_mem == ' ' || *m_mem == '\t'))
					m_mem++;

				if (m_mem < m_end && *m_mem == ',')
				{
					moreValues = true;
					m_mem++;
				}
			} while (moreValues);
		}

		// skip white space but not EOLs
		while (m_mem < m_end && (*m_mem == ' ' || *m_mem == '\t'))
			m_mem++;

		return (*m_mem != 10 && *m_mem != 13);
	}

	// skip EOL  (will skip multiple including any white space)
	void NextLine()
	{
		while (m_mem < m_end && (*m_mem == 10 || *m_mem == 13  || *m_mem == ' ' || *m_mem == '\t'))
			*m_mem++;
	}

	bool IsFinished()
	{
		return m_mem == m_end;
	}
};



bool BitmapFontAssetData::SrcFilesToAsset(vector<MemBlock>& srcFiles, AssetCreateParams* params)
{
	string token;
	vector<string> values;
	LineParser parser(srcFiles[0]);
	while (!parser.IsFinished())
	{
		parser.GrabTokenValue(token, values);
		if (token == "info")
		{
			// parse info line
			while (parser.GrabTokenValue(token, values))
			{
				if (token == "face")
					info.face = token;
				else if (token == "size" && values.size() == 1)
					info.size = StringToI32(values[0]);
				else if (token == "bold" && values.size() == 1)
					info.bold = StringToBool(values[0]);
				else if (token == "italic" && values.size() == 1)
					info.italic = StringToBool(values[0]);
				else if (token == "charset" && values.size() == 1)
					info.charset = values[0];
				else if (token == "unicode" && values.size() == 1)
					info.unicode = StringToBool(values[0]);
				else if (token == "stretchH" && values.size() == 1)
					info.stretchH = StringToI32(values[0]);
				else if (token == "smooth" && values.size() == 1)
					info.smooth = StringToBool(values[0]);
				else if (token == "aa" && values.size() == 1)
					info.aa = StringToBool(values[0]);
				else if (token == "padding" && values.size() == 4)
				{
					info.padding.x = StringToI32(values[0]);
					info.padding.y = StringToI32(values[1]);
					info.padding.z = StringToI32(values[2]);
					info.padding.w = StringToI32(values[3]);
				}
				else if (token == "spacing" && values.size() == 2)
				{
					info.padding.x = StringToI32(values[0]);
					info.padding.y = StringToI32(values[1]);
				}
				else if (token == "outline" && values.size() == 1)
					info.outline = StringToBool(values[0]);
			}
		}
		else if (token == "common")
		{	
			// parse common line
			while (parser.GrabTokenValue(token, values))
			{
				if (token == "lineHeight" && values.size() == 1)
					info.lineHeight = StringToI32(values[0]);
				else if (token == "base" && values.size() == 1)
					info.base = StringToI32(values[0]);
				else if (token == "scaleW" && values.size() == 1)
					info.scaleW = StringToI32(values[0]);
				else if (token == "scaleH" && values.size() == 1)
					info.scaleH = StringToI32(values[0]);
				else if (token == "pages" && values.size() == 1)
					info.pages = StringToI32(values[0]);
				else if (token == "packed" && values.size() == 1)
					info.packed = StringToBool(values[0]);
				else if (token == "alphaChnl" && values.size() == 1)
					info.alphaChnl = StringToBool(values[0]);
				else if (token == "redChnl" && values.size() == 1)
					info.redChnl = StringToI32(values[0]);
				else if (token == "greenChnl" && values.size() == 1)
					info.greenChnl = StringToI32(values[0]);
				else if (token == "blueChnl" && values.size() == 1)
					info.blueChnl = StringToI32(values[0]);
			}
		}
		else if (token == "page")
		{
			// parse page line
			PageInfo page;
			while (parser.GrabTokenValue(token, values))
			{
				if (token == "id" && values.size() == 1)
					page.id = StringToI32(values[0]);
				else if (token == "file" && values.size() == 1)
					page.file = values[0];
			}
			pages.emplace_back(page);
		}
		else if (token == "char")
		{
			CharInfo charInfo;
			while (parser.GrabTokenValue(token, values))
			{
				if (token == "id" && values.size() == 1)
					charInfo.id = StringToI32(values[0]);
				else if (token == "x" && values.size() == 1)
					charInfo.pos.x = StringToI32(values[0]);
				else if (token == "y" && values.size() == 1)
					charInfo.pos.y = StringToI32(values[0]);
				else if (token == "width" && values.size() == 1)
					charInfo.size.x = StringToI32(values[0]);
				else if (token == "height" && values.size() == 1)
					charInfo.size.x = StringToI32(values[0]);
				else if (token == "xoffset" && values.size() == 1)
					charInfo.offset.x = StringToI32(values[0]);
				else if (token == "yoffset" && values.size() == 1)
					charInfo.offset.y = StringToI32(values[0]);
				else if (token == "xadvance" && values.size() == 1)
					charInfo.xadvance = StringToI32(values[0]);
				else if (token == "page" && values.size() == 1)
					charInfo.page = StringToI32(values[0]);
				else if (token == "chnl" && values.size() == 1)
					charInfo.channel = StringToI32(values[0]);
			}
			chars.emplace_back(charInfo);
		}
		else
		{
			while (parser.GrabTokenValue(token, values));
		}
		parser.NextLine();
	}
	return true;
}

void BitmapFontAssetData::Serialize(Serializer& stream)
{
	type = AssetType_BitmapFont;
	u16 assetType = (u16)type;
	stream& assetType& version& name;

	stream& info.face& info.charset& info.stretchH& info.bold& info.italic& info.unicode& info.smooth
		& info.aa& info.outline& info.packed& info.alphaChnl& info.padding& info.spacing& info.lineHeight
		& info.base& info.scaleW& info.scaleH& info.pages& info.redChnl& info.greenChnl& info.blueChnl;

	u32 size = (u32)pages.size();
	stream& size;
	pages.resize(size);
	for (auto& page : pages)
	{
		stream& page.id& page.file;
	}

	size = (u32)chars.size();
	stream& size;
	chars.resize(size);
	for (auto& ch : chars)
	{
		stream& ch.id& ch.pos& ch.size& ch.offset& ch.xadvance& ch.page& ch.channel;
	}
}

MemBlock BitmapFontAssetData::AssetToMemory()
{
	Serializer_BinaryWriteGrow stream;
	Serialize(stream);
	return MemBlock::CloneMem(stream.DataStart(), stream.DataSize());
}

bool BitmapFontAssetData::MemoryToAsset(const MemBlock& block)
{
	Serializer_BinaryRead stream(block);
	Serialize(stream);

	if (type != AssetType_BitmapFont)
	{
		LOG(BitmapFont, STR("Rebuilding {} - bad type {} - expected {}", name, (int)type, (int)AssetType_BitmapFont));
		return false;
	}
	if (version != BITMAPFONT_VERSION)
	{
		LOG(BitmapFont, STR("Rebuilding {} - old version {} - expected {}", name, version, BITMAPFONT_VERSION));
		return false;
	}

	return true;
}
