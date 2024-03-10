#pragma once

//<REFLECT>
enum FSExcludeType
{
	FSExcludeType_Dir,
	FSExcludeType_File,
	FSExcludeType_Ext,
	FSExcludeType_MAX
};

class FileExcludes
{
public:
	FileExcludes(const string &path);
	~FileExcludes();

	bool IsExcluded(const char *dir, const char *filename, const char *ext);

protected:
	vector<u64> m_excludes[FSExcludeType_MAX];
};
