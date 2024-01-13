#pragma once

// case insensitive equal
inline bool StringEqual(const char* str1, const char* str2)
{
#if defined(PLATFORM_Windows)
	return str1 && str2 && !_stricmp(str1, str2);
#else
	return str1 && str2 && !strcasecmp(str1, str2);
#endif
}

// convert utf8 string to an array of u16 values
int StringToU16(std::string src, u16* buffer, int bufferLength);

// pop off next utf8 at index idx.  return 0 when finished string
u16 StringPopUTF8(const std::string& str, int& idx);

// replace all characters of oldChar with newChar.  utf8 friendly.
std::string StringReplace(const std::string& str, char oldChar, char newChar);

// generate hash64 from a string
u64 StringHash64(const std::string& str);

// split a path string into FileSys, DirectoryTree, Filename, and Extension
void StringSplitIntoFileParts(const std::string& str, std::string* pFilesys, std::string* pDirectory, std::string* pFilename, std::string* pExt);

// add a filename or path to an existing path
// inserts a '/' if necessary
std::string StringAddPath(const std::string& base, const std::string& path);

// split a string into left & right parts around a delimiter character
bool StringSplit(const std::string& input, std::string& left, std::string& right, char delimiter);

// split an absolute path string into a filesystem and a path
void StringSplitIntoFSAndPath(const std::string& str, std::string& fs, std::string& path);

// some wrappers for getting info frame paths
inline std::string StringGetDirectory(const std::string &path) { std::string fs, d; StringSplitIntoFileParts(path, &fs, &d, 0, 0); return fs + d; }
inline std::string StringGetPathNoExt(const std::string& path) { std::string fs, d, fn; StringSplitIntoFileParts(path, &fs, &d, &fn, 0); return StringAddPath(fs + d, fn); }
inline std::string StringGetFilename(const std::string& path) { std::string fn, e; StringSplitIntoFileParts(path, 0, 0, &fn, &e); return fn + e; }
inline std::string StringGetFilenameNoExt(const std::string& path) { std::string fn; StringSplitIntoFileParts(path, 0, 0, &fn, 0); return fn; }
inline std::string StringGetExtension(const std::string& path) { std::string e; StringSplitIntoFileParts(path, 0, 0, 0, &e); return e; }
