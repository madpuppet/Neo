#pragma once

#include "Singleton.h"

// callback when resource data has been finally loaded
typedef FastDelegate::FastDelegate1<ResourceData*> LoadResourceDataCB;

enum class ResourceType
{
	Texture,
	Mesh,
	Animation,
	Database
};

class ResourceData
{
	std::string m_name;
	vector<std::string> m_sourceFiles;

	virtual ~ResourceData() {}
};

class TextureData : public ResourceData
{
};


class ResourceBuilderManager : public Singleton<ResourceBuilderManager>
{
public:
	// gather all data from file systems
	void LoadResourceDataAsync(const std::string &name, const LoadResourceDataCB& cb);
};

