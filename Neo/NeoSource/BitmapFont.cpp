#include "neo.h"
#include "BitmapFont.h"
#include "StringUtils.h"
#include "RenderThread.h"
#include "ResourceLoadedManager.h"
#include "ImmDynamicRenderer.h"
#include <stb_image.h>

#define BITMAPFONT_VERSION 1

CmdLineVar<bool> CLV_ShowFontBorders("showFontBorders", "draw a white border around font draw areas", true);

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

		// create dependant resources
		vector<Resource*> dependantResources;
		for (auto& page : m_assetData->pages)
		{
			page.material.Create(page.file);
			dependantResources.push_back(*page.material);
		}

		m_white.Create("white_ortho");
		dependantResources.push_back(m_white);
		
		LOG(Font, STR("BitmapFont {} Dependancy List: {}", data->name, dependantResources.size()));

		// we need to wait for our dependant resources, like Shaders and Textures,  to load first before creating our platform data (which are pipeline states)
		// note that if they are already loaded, this will just trigger off the callback immediately
		ResourceLoadedManager::Instance().AddDependancyList(this, dependantResources, [this]() { OnLoadComplete(); });
	}
	else
	{
		// failed to load but we still need to make it as loaded so dependant resources can continue
		m_failedToLoad = true;
		OnLoadComplete();
	}
}

void BitmapFont::Reload()
{
}

template <> ResourceFactory<BitmapFont>::ResourceFactory()
{
	auto ati = new AssetTypeInfo();
	ati->name = "BitmapFont";
	ati->assetExt = ".neobmf";
	ati->assetCreator = []() -> AssetData* { return new BitmapFontAssetData; };
	ati->sourceExt.push_back({ { ".fnt", }, true });		// on of these src image files
	AssetManager::Instance().RegisterAssetType(AssetType_BitmapFont, ati);
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
				m_token[tokenIdx++] = *m_mem;
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
						m_value[valueIdx++] = *m_mem;
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

		return !token.empty();
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
				{
					int id = StringToI32(values[0]);
					Assert(id == pages.size(), "Unexpected Page ID - expected them to be in order 0..N");
				}
				else if (token == "file" && values.size() == 1)
					page.file = StringGetFilenameNoExt(values[0]);
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
					charInfo.size.y = StringToI32(values[0]);
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
			chars[charInfo.id] = charInfo;
		}
		else
		{
			while (parser.GrabTokenValue(token, values));
		}
		parser.NextLine();
	}
	return true;
}

MemBlock BitmapFontAssetData::AssetToMemory()
{
	Serializer_BinaryWriteGrow stream;

	stream << type << version << name;

	stream << info.face << info.charset << info.stretchH << info.bold << info.italic << info.unicode << info.smooth
		   << info.aa << info.outline << info.packed << info.alphaChnl << info.padding << info.spacing << info.lineHeight
		   << info.base << info.scaleW << info.scaleH << info.pages << info.redChnl << info.greenChnl << info.blueChnl;

	stream << (u32)pages.size();
	for (auto& page : pages)
	{
		stream << page.file;
	}

	stream << (u32)chars.size();
	for (auto &it : chars)
	{
		auto& ch = it.second;
		stream << ch.id << ch.pos << ch.size << ch.offset << ch.xadvance << ch.page << ch.channel;
	}

	return MemBlock::CloneMem(stream.DataStart(), stream.DataSize());
}

bool BitmapFontAssetData::MemoryToAsset(const MemBlock& block)
{
	Serializer_BinaryRead stream(block);

	stream >> type >> version >> name;

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

	stream >> info.face >> info.charset >> info.stretchH >> info.bold >> info.italic >> info.unicode >> info.smooth
		>> info.aa >> info.outline >> info.packed >> info.alphaChnl >> info.padding >> info.spacing >> info.lineHeight
		>> info.base >> info.scaleW >> info.scaleH >> info.pages >> info.redChnl >> info.greenChnl >> info.blueChnl;

	u32 size = stream.ReadU32();
	pages.clear();
	for (u32 i = 0; i < size; i++)
	{
		PageInfo page;
		stream >> page.file;
		pages.push_back(page);
	}

	size = stream.ReadU32();
	for (auto& it : chars)
	{
		auto& ch = it.second;
		stream << ch.id << ch.pos << ch.size << ch.offset << ch.xadvance << ch.page << ch.channel;
	}
	return true;
}

void BitmapFont::RenderText(const string& text, const rect& area, float z, Alignment align, const vec2& scale, const color& col, float dropShadowOffset)
{
	Assert(Thread::IsOnThread(ThreadGUID_Render), "Must be called on Render Thread");

	auto& dr = ImmDynamicRenderer::Instance();
	if (CLV_ShowFontBorders.Value())
	{
		dr.BeginRender();
		dr.StartPrimitive(PrimType_LineStrip);
		dr.UseMaterial(m_white);
		dr.AddVert({ area.x,area.y,z }, vec2(0, 0), 0xffffffff);
		dr.AddVert({ area.x2(), area.y, z }, vec2(1, 0), 0xffffffff);
		dr.AddVert({ area.x2(), area.y2(),z }, vec2(0, 1), 0xffffffff);
		dr.AddVert({ area.x, area.y2(), z }, vec2(1, 1), 0xffffffff);
		dr.AddVert({ area.x,area.y,z }, vec2(0, 0), 0xffffffff);
		dr.EndPrimitive();
		dr.EndRender();
	}

	// first decode to font char indexes
	vector<u32> codes = StringToUnicode(text);

	// generate the render boxes for characters
	struct CharBox
	{
		float x1, y1, x2, y2;
		float u1, v1, u2, v2;
	};

	vector<CharBox> boxes;
	float x = 0.0f;
	float y = 0.0f;
	float miny = 10000.0f;
	float maxy = -10000.0f;

	float scaleU = 1.0f / m_assetData->info.scaleW;
	float scaleV = 1.0f / m_assetData->info.scaleH;
	for (auto ch : codes)
	{
		auto it = m_assetData->chars.find(ch);
		if (it != m_assetData->chars.end())
		{
			auto& chinfo = it->second;
			float x1 = x + chinfo.offset.x;
			float x2 = x1 + chinfo.size.x;
			float y1 = y;
			float y2 = y1 + chinfo.size.y;
			float u1 = chinfo.pos.x* scaleU;
			float u2 = (chinfo.pos.x + chinfo.size.x)* scaleU;
			float v2 = (chinfo.pos.y* scaleV);
			float v1 = ((chinfo.pos.y + chinfo.size.y)* scaleV);
			x = x + chinfo.xadvance;
			x1 *= scale.x;
			x2 *= scale.x;
			y1 *= scale.y;
			y2 *= scale.y;
			boxes.emplace_back(x1,y1,x2,y2,u1,v1,u2,v2);
			miny = Min(miny, y1);
			maxy = Max(maxy, y2);
		}
	}
	if (boxes.empty())
		return;

	float minx = boxes.front().x1;
	float maxx = boxes.back().x2;

	// calculate the appropriate offset to align the box to the draw area
	float xoffset = 0.0f;
	float yoffset = 0.0f;
	switch (align)
	{
		case Alignment_TopLeft:
			xoffset = area.x - minx;
			yoffset = area.y2() - maxy;
			break;
		case Alignment_TopCenter:
			xoffset = area.x + area.w / 2 - (maxx - minx) / 2 - minx;
			yoffset = area.y2() - maxy;
			break;
		case Alignment_TopRight:
			xoffset = area.x2() - maxx;
			yoffset = area.y2() - maxy;
			break;
		case Alignment_CenterLeft:
			xoffset = area.x - minx;
			yoffset = area.y + area.h/2 - (maxy - miny) / 2 - miny;
			break;
		case Alignment_Center:
			xoffset = area.x + area.w / 2 - (maxx - minx) / 2 - minx;
			yoffset = area.y + area.h / 2 - (maxy - miny) / 2 - miny;
			break;
		case Alignment_CenterRight:
			xoffset = area.x2() - maxx;
			yoffset = area.y + area.h / 2 - (maxy - miny) / 2 - miny;
			break;
		case Alignment_BottomLeft:
			xoffset = area.x - minx;
			yoffset = area.y - miny;
			break;
		case Alignment_BottomCenter:
			xoffset = area.x + area.w / 2 - (maxx - minx) / 2 - minx;
			yoffset = area.y - miny;
			break;
		case Alignment_BottomRight:
			xoffset = area.x2() - maxx;
			yoffset = area.y - miny;
			break;
	};

	dr.BeginRender();
	dr.StartPrimitive(PrimType_TriangleList);
	dr.UseMaterial(m_assetData->pages[0].material);

	if (dropShadowOffset)
	{
		int toggleZ = 0;
		for (auto& ch : boxes)
		{
			float _z = z + toggleZ * 0.001f;
			toggleZ = 1 - toggleZ;
			dr.AddVert({ ch.x1 + xoffset + dropShadowOffset, ch.y1 + yoffset + dropShadowOffset, _z }, { ch.u1, ch.v1 }, 0xff000000);
			dr.AddVert({ ch.x1 + xoffset + dropShadowOffset, ch.y2 + yoffset + dropShadowOffset, _z }, { ch.u1, ch.v2 }, 0xff000000);
			dr.AddVert({ ch.x2 + xoffset + dropShadowOffset, ch.y1 + yoffset + dropShadowOffset, _z }, { ch.u2, ch.v1 }, 0xff000000);

			dr.AddVert({ ch.x2 + xoffset + dropShadowOffset, ch.y2 + yoffset + dropShadowOffset, _z }, { ch.u2, ch.v2 }, 0xff000000);
			dr.AddVert({ ch.x2 + xoffset + dropShadowOffset, ch.y1 + yoffset + dropShadowOffset, _z }, { ch.u2, ch.v1 }, 0xff000000);
			dr.AddVert({ ch.x1 + xoffset + dropShadowOffset, ch.y2 + yoffset + dropShadowOffset, _z }, { ch.u1, ch.v2 }, 0xff000000);
		}
	}

	u32 col32 = vec4ToR8G8B8A8(col);
	int toggleZ = 0;
	for (auto &ch : boxes)
	{
		float _z = z + toggleZ * 0.001f;
		toggleZ = 1 - toggleZ;

		dr.AddVert({ ch.x1+ xoffset, ch.y1 + yoffset, _z }, { ch.u1, ch.v1 }, col32);
		dr.AddVert({ ch.x1 + xoffset, ch.y2 + yoffset, _z }, { ch.u1, ch.v2 }, col32);
		dr.AddVert({ ch.x2 + xoffset, ch.y1 + yoffset, _z }, { ch.u2, ch.v1 }, col32);

		dr.AddVert({ ch.x2 + xoffset, ch.y2 + yoffset, _z }, { ch.u2, ch.v2 }, col32);
		dr.AddVert({ ch.x2 + xoffset, ch.y1 + yoffset, _z }, { ch.u2, ch.v1 }, col32);
		dr.AddVert({ ch.x1 + xoffset, ch.y2 + yoffset, _z }, { ch.u1, ch.v2 }, col32);
	}

	dr.EndPrimitive();
	dr.EndRender();
}




