#pragma once

#include <string>
#include <vector>
#include <stdlib.h>

#if defined(PLATFORM_Android)
#include <ctype.h>
#endif


#define STR(...) String::CreateFormatted(__VA_ARGS__)
#define SZ(_string) String(_string)
#define STRC(...) String::CreateFormatted(__VA_ARGS__).CStr()

// String manipulation
void StringCopy(char *pBuffer, const char *pString, int bufferSize);
bool StringEqual(const char *string1, const char *string2);
bool StringStarts(const char *string1, const char *startString);
bool StringContains(const char *string, const char *substr);
int StringCompare(const char *string1, const char *string2);
char *StringDup(const char *source);
double StringToDouble(const char *str);
int StringToInt(const char *str);
int StringToHex(const char *str);

uint64_t Pork() { return 0; }
u64 StringToHex64(const char *str);
u64 StringHash64(const char* pString);

class String
{
protected:
	u32 length : 31;
	u32 usealloc : 1;
	union
	{
		struct
		{
			char *pMem;
			u32 bufferSize;
		} alloc;
		struct
		{
			char buffer[28];
		} embed;
	};
	u32 StorageSize() { return usealloc ? alloc.bufferSize : sizeof(embed.buffer); } 
	char *Storage() { return usealloc ? alloc.pMem : embed.buffer; }
	const char *Storage() const { return usealloc ? alloc.pMem : embed.buffer; }

public:
	static String CreateFormatted(const char *pFormat, ...);
	static String empty;
	static String CreateFromRange(const char *pStart, const char *pEnd);
	static String CreateFromFloat(float value);
    static String CreateFromInt(int value);

	String() : usealloc(0), length(0) { embed.buffer[0] = 0; }
	String(const char *pString);
	String(const String &string);
	String& operator=( const String& rhs );

	String(u32 maxChars, const char *pString);
	~String();
	void Nullify();

	// returns length
	int ConvertUTF8(u16 *buffer, int bufferLength);
	u16 PopUTF8(int &idx) const;

	int Length() const { return length; }
	bool IsEqual(const char *pString) const;
	bool IsEqual(const String &string) const;
	bool IsEmpty() const { return length == 0; }
	String Replace(char oldChar, char newChar) const;

	String AsUppercase() const;
	String AsLowercase() const;

	bool StartsWith(const char *pText) const;
	bool EndsWith(const char *pText) const;
	bool ContainsAnyChar(const char *delimiters) const;
	bool Contains(const String &subStr) const;

	bool Compare(const char *str, int len) const;

	void Set(const char *pString);

	// split across the delimiters
	void ToStringList(std::vector<String> &stringList, const char *pDelimiters) const;
	void ToStringListTrimmed(std::vector<String> &stringList, const char *pDelimiters) const;

	// fast split string list into some predefined buffers
	// - buffers =  char buffers[bufferCount][bufferSize];
	int SplitToBuffers(char *buffers, int bufferCount, int bufferSize) const;

	//  u32 GetCRC() const;

	const char *CStr() const                          { return usealloc ? alloc.pMem : embed.buffer; }
	const char *CStrDup() const                       { return StringDup(CStr()); }
	const char *c_str() const						  { return usealloc ? alloc.pMem : embed.buffer; }

	// these two can be used together for direct access to the string buffer
	void SetLength(int len);
	char *RawStr() { return usealloc ? alloc.pMem : embed.buffer; }

	bool operator ==(const char *pText) const         { return IsEqual(pText); }
	bool operator ==(const String &string) const    { return IsEqual(string); }
	bool operator !=(const char *pText) const         { return !IsEqual(pText); }
	bool operator !=(const String &string) const    { return !IsEqual(string); }
	void operator +=(const char *pText)               { *this = *this + pText; }
	void operator +=(const String &string)          { *this = *this + string; }
	void operator +=(char ascii)                      { *this = *this + ascii; }

	bool operator > (const String &string) const;
	bool operator < (const String &string) const;

	String &operator <<(const char *pValue);
	String &operator <<(const String &value);
	String &operator <<(int value);
	String &operator <<(float value);
	String &operator <<(void *pValue);
	const char &operator [](int index) const { return *(CStr()+index); }
	char &operator [](int index) { return *((char*)(CStr()+index)); }

	friend String operator + (const String &string1, const String &string2);
	friend String operator + (const String &string1, const char *text2);
	friend String operator + (const char *text2, const String &string1);
	friend String operator + (const String &string, char ascii);

	int FindInSubStr(const int start, const int end, const char c) const;
	String CloneSubStr(int firstChar, int lastChar) const;
	String SubstrLeft(int count) const                        { return count >= Length() ? *this : CreateFromRange(Storage(), Storage()+count-1); }
	String SubstrRight(int count) const                       { return count >= Length() ? *this : CreateFromRange(Storage()+(Length()-count), Storage()+Length()-1); }
	String StripPrefix(const String &prefix) const;

	// split filesystem path into bits
	void SplitIntoFileParts(String *pFilesys, String *pDirectory, String *pFilename, String *pExt) const;
	void SplitIntoFSAndPath(String &fs, String &path) const;
	bool SplitString(String &head, String &tail, char delimiter) const;

	// returns pieces of a filesystem
	String GetDirectory() const                               { String fs, d; SplitIntoFileParts(&fs,&d,0,0); return fs+d; }
	String GetPathNoExt() const                               { String fs,d,fn; SplitIntoFileParts(&fs,&d,&fn,0); return (fs+d).AddPath(fn); }
	String GetFilename() const                                { String fn,e; SplitIntoFileParts(0,0,&fn,&e); return fn+e; }
	String GetFilenameNoExt() const                           { String fn; SplitIntoFileParts(0,0,&fn,0); return fn; }
	String GetExtension() const                               { String e; SplitIntoFileParts(0,0,0,&e); return e; }
	String AddPath(const String &path) const;
	String RemoveDelimitedExtension(const char *delimiters) const;
	u64 Hash64() const { return StringHash64(CStr()); }

	bool AsBool() const                                         { return IsEqual("true") || IsEqual("1"); }
	float AsFloat() const                                       { return (float)atof(CStr()); }
	int AsInt() const											  { return atoi(CStr()); }
	u8 GetHexByteAt(int idx) const;
	u8 GetHexNibbleAt(int idx) const;
};

// convert enums and flags using null terminated string arrays
bool StringToIndex(const String &name, const char **valueArray, int arraySize, int &idx);
bool IndexToString(int idx, const char **valueArray, int arraySize, String &name);

// handy string tokeniser
class DMTokeniser
{
public:
	DMTokeniser(const char *pString, char separator);

	const char *Get() { return m_token[0] ? m_token : 0; }
	const char *Next();

protected:
	char m_separator;
	char *m_pRead;
	char m_buffer[256];
	char m_token[256];
};
