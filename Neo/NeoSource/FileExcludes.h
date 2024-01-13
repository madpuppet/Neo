#pragma once

class std::string;

class FileExcludes
{
public:
	FileExcludes(const std::string &path);
	~FileExcludes();

	bool IsExcluded(const char *dir, const char *filename, const char *ext);

protected:
	enum ExcludeType
	{
		ExcludeType_Dir,
		ExcludeType_File,
		ExcludeType_Ext,
		ExcludeType_MAX
	};
	std::vector<u64> m_excludes[ExcludeType_MAX];
};
