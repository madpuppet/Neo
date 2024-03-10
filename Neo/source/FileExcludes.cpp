#include "Neo.h"
#include "FileExcludes.h"
#include "StringUtils.h"

FileExcludes::FileExcludes(const string &path)
{
	FSExcludeType excludeType = FSExcludeType_Ext;
	FILE *fh = fopen(path.c_str(), "r");
	if (fh)
	{
		char buf[80];
		while (fgets(buf, 80, fh) != 0)
		{
			int len = (int)strlen(buf);
			buf[len - 1] = 0;
			if (StringEqual(buf, "[file]"))
			{
				excludeType = FSExcludeType_File;
			}
			else if (StringEqual(buf, "[dir]"))
			{
				excludeType = FSExcludeType_Dir;
			}
			else if (StringEqual(buf, "[ext]"))
			{
				excludeType = FSExcludeType_Ext;
			}
			else if (buf[0])
			{
				if (excludeType == FSExcludeType_Dir)
				{
					string sanitizedDir = StringReplace(buf, '\\', '/');
					m_excludes[excludeType].push_back(StringHash64(sanitizedDir));
				}
				else
				{
					m_excludes[excludeType].push_back(StringHash64(buf));
				}
			}
		}
	}

	// default exclude types...
	m_excludes[FSExcludeType_Ext].push_back(StringHash64(".exclude"));
	m_excludes[FSExcludeType_Ext].push_back(StringHash64(".rkv"));
	m_excludes[FSExcludeType_Ext].push_back(StringHash64(".log"));
	m_excludes[FSExcludeType_Ext].push_back(StringHash64(".exe"));
	m_excludes[FSExcludeType_Ext].push_back(StringHash64(".bak"));
	m_excludes[FSExcludeType_Ext].push_back(StringHash64(".mase"));
	m_excludes[FSExcludeType_Ext].push_back(StringHash64(".zip"));
	m_excludes[FSExcludeType_Ext].push_back(StringHash64(".pdb"));
	m_excludes[FSExcludeType_Ext].push_back(StringHash64(".bmfc"));
}

FileExcludes::~FileExcludes()
{
}

bool FileExcludes::IsExcluded(const char *dir, const char *filename, const char *ext)
{
	u64 hash[FSExcludeType_MAX];
	hash[FSExcludeType_Dir] = dir ? StringHash64(dir) : 0;
	hash[FSExcludeType_File] = filename ? StringHash64(filename) : 0;
	hash[FSExcludeType_Ext] = ext ? StringHash64(ext) : 0;
	for (int i = 0; i < FSExcludeType_MAX; i++)
	{
		if (hash[i] != 0)
		{
			for (auto exclude : m_excludes[i])
			{
				if (exclude == hash[i])
					return true;
			}
		}
	}
	return false;
}
