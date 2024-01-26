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

// case insensitive equal
inline bool StringEqual(const string &str1, const string &str2)
{
	return StringEqual(str1.c_str(), str2.c_str());
}

inline bool StringContainsAny(const string& str, const string& chars)
{
    for (char ch : chars) {
        if (str.find(ch) != std::string::npos)
        {
            return true;
        }
    }
    return false;
}

// convert utf8 string to an array of u16 values
int StringToU16(string src, u16* buffer, int bufferLength);

// pop off next utf8 at index idx.  return 0 when finished string
u16 StringPopUTF8(const string& str, int& idx);

// replace all characters of oldChar with newChar.  utf8 friendly.
string StringReplace(const string& str, char oldChar, char newChar);

// generate hash64 from a string
u64 StringHash64(const string& str);

// split a path string into FileSys, DirectoryTree, Filename, and Extension
void StringSplitIntoFileParts(const string& str, string* pFilesys, string* pDirectory, string* pFilename, string* pExt);

// add a filename or path to an existing path
// inserts a '/' if necessary
string StringAddPath(const string& base, const string& path);

// split a string into left & right parts around a delimiter character
bool StringSplit(const string& input, string& left, string& right, char delimiter);

// split an absolute path string into a filesystem and a path
void StringSplitIntoFSAndPath(const string& str, string& fs, string& path);

// some wrappers for getting info frame paths
inline string StringGetDirectory(const string &path) { string fs, d; StringSplitIntoFileParts(path, &fs, &d, 0, 0); return fs + d; }
inline string StringGetPathNoExt(const string& path) { string fs, d, fn; StringSplitIntoFileParts(path, &fs, &d, &fn, 0); return StringAddPath(fs + d, fn); }
inline string StringGetFilename(const string& path) { string fn, e; StringSplitIntoFileParts(path, 0, 0, &fn, &e); return fn + e; }
inline string StringGetFilenameNoExt(const string& path) { string fn; StringSplitIntoFileParts(path, 0, 0, &fn, 0); return fn; }
inline string StringGetExtension(const string& path) { string e; StringSplitIntoFileParts(path, 0, 0, 0, &e); return e; }

// some converters
inline bool StringToBool(const string& str) { return StringEqual(str, "true") || str == "1"; }
inline f32 StringToF32(const string& str) { return (f32)atof(str.c_str()); }
inline i32 StringToI32(const string& str) { return (i32)atoi(str.c_str()); }
u32 StringToHex(const string& str);
u64 StringToHex64(const string& str);

u8 StringGetHexByteAt(const string& str, int idx);
u8 StringGetHexNibbleAt(const string& str, int idx);

// some misc stuff
u64 GenerateRandomU64();

// find the index of this string in the list
// returns -1 if not found in list
int StringFindInList(const string& str, const stringlist &list);



