#pragma once

#include "Singleton.h"

// callback when resource data has been finally loaded
typedef FastDelegate::FastDelegate1<class ResourceData*> LoadResourceDataCB;

enum class ResourceType
{
	Texture,
	Mesh,
	Animation,
	Database
};

class ResourceData
{
protected:
	std::string m_name;
	std::vector<std::string> m_sourceFiles;
public:
	virtual ~ResourceData() {}
};

class TextureData : public ResourceData
{
public:
	~TextureData() {}
};


class ResourceBuilderManager : public Singleton<ResourceBuilderManager>
{
public:
	// gather all data from file systems
	void LoadResourceDataAsync(const std::string &name, const LoadResourceDataCB& cb);
};

