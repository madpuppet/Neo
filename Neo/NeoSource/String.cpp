#include "Neo.h"
#include "String.h"
#include <string>
#include <stdarg.h>
//#include "zlib.h"

String String::empty;

u64 StringHash64(const char* pString)
{
	u64 offsetBasis = 2166136261;
	u64 prime = 16777619;
	u64 hash = offsetBasis;
	const char* pCH = pString;
	while (*pCH)
	{
		int ch = tolower(*pCH);
		hash ^= (ch & 15);
		hash *= prime;
		hash ^= (ch >> 4);
		hash *= prime;
		pCH++;
	}
	return hash;
}

// some memory for building formatted strings
double StringToDouble(const char *str)
{
	double result = 0.0;
	double sign = 1.0;
	const char *in = str;
	char ch;
	while ((ch = *in++) != 0)
	{
		if (ch == '-')
		{
			sign = -1.0;
			continue;
		}

		if (ch == '.')
			break;

		// fallback to the slow routine if there are exponents
		if (ch == 'e')
			return atof(str);

		result = result*10.0f + (float)((int)ch - (int)'0');
	}
	if (ch == '.')
	{
		double frac = 0.1;
		while ((ch = *in++) != 0)
		{
			result += (double)((int)ch - (int)'0') * frac;
			frac *= 0.1;
		}
	}
	return (sign*result);
}

int StringToInt(const char *str)
{
    return (int)StringToDouble(str);
}

int StringToHex(const char *str)
{
	int result = 0;
	const char *in = str;
	char ch;
	while ((ch = *in++) != 0)
	{
		result = result << 4;
		if (ch >= 'a' && ch <= 'f')
			result += ((int)ch - 'a' + 10);
		else if (ch >= 'A' && ch <= 'F')
			result += ((int)ch - 'A' + 10);
		else if (ch >= '0' && ch <= '9')
			result += ((int)ch - '0');
	}
	return result;
}

u64 StringToHex64(const char *str)
{
	u64 result = 0;
	const char *in = str;
	char ch;
	while ((ch = *in++) != 0)
	{
		result = result << 4;
		if (ch >= 'a' && ch <= 'f')
			result += (u64)((int)ch - 'a' + 10);
		else if (ch >= 'A' && ch <= 'F')
			result += (u64)((int)ch - 'A' + 10);
		else if (ch >= '0' && ch <= '9')
			result += (u64)((int)ch - '0');
	}
	return result;
}

//===========================================================================================
bool String::ContainsAnyChar(const char *delimiters) const
{
	const char *ch = delimiters;
	const char *str = CStr();
	while (*ch)
	{
		if (strchr(str, *ch)) return true;
		ch++;
	}
	return false;
}

bool String::Compare(const char *str, int len) const
{
	if ((u32)len > length)
		return false;

	const char *src = CStr();
	for (int i = 0; i < len; i++)
	{
		if (tolower(str[i]) != tolower(src[i]))
			return false;
	}
	return true;
}

bool String::Contains(const String &subStr) const
{
	int maxSearchArea = length - subStr.Length();
	if (maxSearchArea < 0)
		return false;

	const char *srcStr = CStr();
	for (int i = 0; i <= maxSearchArea; i++)
	{
		if (subStr.Compare(&srcStr[i], subStr.Length()))
			return true;
	}
	return false;
}

//===============================================================================
//===============================================================================
String String::CreateFormatted(const char *pFormat, ...)
{
    char* stringTempBuffer = new char[1024 * 1024];
    va_list args;
	va_start (args, pFormat);
	vsprintf (stringTempBuffer, pFormat, args);
	va_end (args);
    String result = String(stringTempBuffer);
    delete stringTempBuffer;
    return result;
}

int String::SplitToBuffers(char *buffers, int bufferCount, int bufferSize) const
{
	int outIdx = 0;
	const char *in = CStr();
	char *out = buffers;
	while (true)
	{
		// skip spaces/tabs
		while (*in == ' ' || *in == '\t')
			in++;

		// scan in characters
		int len = 0;
		while (*in != ' ' && *in != '\t' && *in != 0)
		{
			if (len < bufferSize - 1)
			{
				*out++ = *in++;
				len++;
			}
			else
				in++;
		}

		*out++ = 0;
		outIdx++;
		out = buffers + outIdx*bufferSize;

		if (*in == 0 || outIdx == bufferCount)
			break;
	}
	return outIdx;
}


//===============================================================================
//===============================================================================
String::String(const char *pString)
{
	if (pString == 0)
	{
		length = 0;
		usealloc = 0;
		embed.buffer[0] = 0;
	}
	else
	{
		length = (u16)strlen(pString);
		if (length+1 > sizeof(embed.buffer))
		{
			usealloc = 1;
			alloc.bufferSize = (length+1+31)&0xffffffe0;
			alloc.pMem = (char*)malloc(alloc.bufferSize);
			memcpy(alloc.pMem, pString, length+1);
		}
		else
		{
			usealloc = 0;
			memcpy(embed.buffer, pString, length+1);
		}
	}
}

//===============================================================================
//===============================================================================
void String::SetLength(int _length)
{
	length = _length;
	if (length+1 > StorageSize())
	{
		if (usealloc)
			free(alloc.pMem);

		usealloc = 1;
		alloc.bufferSize = (length+1+31)&0xffffffe0;
		alloc.pMem = (char*)malloc(alloc.bufferSize);
	}
	RawStr()[length]=0;
}

//===============================================================================
//===============================================================================
String::String(const String &str)
{
	length = str.length;
	if (length+1 > sizeof(embed.buffer))
	{
		usealloc = 1;
		alloc.bufferSize = (length+1+31)&0xffffffe0;
		alloc.pMem = (char*)malloc(alloc.bufferSize);
		memcpy(alloc.pMem, str.CStr(), length+1);
	}
	else
	{
		usealloc = 0;
		memcpy(embed.buffer, str.CStr(), length+1);
	}
}

//===============================================================================
//===============================================================================
String& String::operator=( const String& rhs )
{
	// new string will always be this length..
	length = rhs.length;

	// if we already have alloc'd memory, see if this string fints
	if (usealloc == 1)
	{
		if (alloc.bufferSize > (u32)(rhs.length + 1))
		{
			// string fits, just copy it and return
			memcpy(alloc.pMem, rhs.CStr(), length + 1);
			return *this;
		}
		else
		{
			// string doesn't fit, clear our memory and fall through to regular case
			usealloc = 0;
			free(alloc.pMem);
		}
	}

	// does string fit in our embedded buffer?
	if (length+1 > sizeof(embed.buffer))
	{
		// nope, we'll need to alloc memory
		usealloc = 1;
		alloc.bufferSize = (length+1+31)&0xffffffe0;
		alloc.pMem = (char*)malloc(alloc.bufferSize);
		memcpy(alloc.pMem, rhs.CStr(), length+1);
	}
	else
	{
		// string fits - no need for memory, just copy it and return
		usealloc = 0;
		memcpy(embed.buffer, rhs.CStr(), length+1);
	}
	return *this;
}

//===============================================================================
//===============================================================================
String::String(u32 maxChars, const char *pString)
{
	length = 0;
	while (length<maxChars && pString[length]!=0)
		length++;

	if (length+1 > sizeof(embed.buffer))
	{
		usealloc = 1;
		alloc.bufferSize = (length+1+31)&0xffffffe0;
		alloc.pMem = (char*)malloc(alloc.bufferSize);
		memcpy(alloc.pMem, pString, length);
		alloc.pMem[length]=0;
	}
	else
	{
		usealloc = 0;
		memcpy(embed.buffer, pString, length);
		embed.buffer[length]=0;
	}
}

//===============================================================================
//===============================================================================
String::~String()
{
	if (usealloc)
		free(alloc.pMem);
}

void String::Nullify()
{
	if (usealloc)
		free(alloc.pMem);

	usealloc = 0;    
	length = 0;
	embed.buffer[0] = 0;
}

//===============================================================================
//===============================================================================
bool String::IsEqual(const char *pText) const
{
	const char *pStr = CStr();
	return (pText == NULL) ? pStr[0] == 0 : StringEqual(pStr, pText);
}

bool String::StartsWith(const char *pText) const
{
	const char *pStr = CStr();
	return (pText == NULL) ? pStr[0] == 0 : StringStarts(pStr, pText);
}

String String::StripPrefix(const String &prefix) const
{
	if (StartsWith(prefix.CStr()))
		return SubstrRight(Length() - prefix.Length());
	else
		return *this;
}

bool String::EndsWith(const char *pText) const
{
	u32 len = (u32)strlen(pText);
	if (length < len)
		return false;

	const char *ch = CStr();
	for (u32 i = 0; i < len; i++)
		if (ch[length - len + i] != pText[i])
			return false;

	return true;
}

//===============================================================================
//===============================================================================
bool String::IsEqual(const String &string) const
{
	return StringEqual(string.CStr(), CStr());
}

bool String::operator > (const String &string) const
{
	return StringCompare(string.CStr(), CStr()) > 0;
}

bool String::operator < (const String &string) const
{
	return StringCompare(CStr(), string.CStr()) < 0;
}

//===============================================================================
//===============================================================================
String operator + (const String &string1, const String &string2)
{
	return STR("%s%s",string1.CStr(),string2.CStr());
}

String operator + (const String &string1, char ascii)
{
	return STR("%s%c",string1.CStr(),ascii);
}

//===============================================================================
//===============================================================================
String operator + (const String &string1, const char *text2)
{
	return STR("%s%s",string1.CStr(),text2);
}

//===============================================================================
//===============================================================================
String operator + (const char *text2, const String &string1)
{
	return STR("%s%s",text2,string1.CStr());
}

//===============================================================================
//===============================================================================
String String::CloneSubStr(int firstChar, int lastChar) const
{
	char stringTempBuffer[2048];

	u32 len = (u32)(lastChar - firstChar + 1);
	Assert(len >= 0 && firstChar >= 0 && (u32)lastChar <= length, STR("Bad params for a CloneSubStr : len %d, firstChar %d, lastChar %d",len,firstChar,lastChar));

	if (len > 0)
		memcpy(stringTempBuffer, CStr() + firstChar, len);
	stringTempBuffer[len] = 0;
	return String(stringTempBuffer);
}

//===============================================================================
//===============================================================================
int String::FindInSubStr(const int start, const int end, const char c) const
{
	const char *pStorage = CStr();
	for (int i = start; i <= end && (u32)i < length; ++i)
	{
		if (pStorage[i] == c)
			return i;
	}
	return -1;
}

#if 0
static const char *LowerCase(const char *pString, int &size)
{
	static char lwrcase[256];
	char *pOut = lwrcase;
	size = 0;
	while (*pString)
	{
		*pOut++ = (char)tolower(*pString++);
		size++;
	}
	*pOut=0;
	return lwrcase;
}
#endif


//u32 String::GetCRC() const
//{
//  int dummy;
//  return crc32(0, (const Bytef *)LowerCase(CStr(), dummy), length);
//}

#if 0
class CopyFormatter
{
public:
	CopyFormatter(const char *pFormat);
	const char *Finish(const char *pInsertString, int length);

	enum Alignment { Left, Right, Center } alignment;
	int size;
	int decimals;
	int fillChar;
	bool err;

protected:
	char workBuffer[512], *pOut, *pFormat;
	const char *pIn;
};
#endif

void String::Set(const char *pString)
{
	u32 len = pString ? (u32)strlen(pString) : 0;
	if (len+1 <= StorageSize())
	{
		// string fits in our current memory
		length = (u16)len;
		char *out = usealloc ? alloc.pMem : embed.buffer;
		memcpy(out, pString, length+1);
	}
	else
	{
		// we need to allocate new memory
		if (usealloc)
			free(alloc.pMem);

		length = (u16)len;
		usealloc = 1;
		alloc.bufferSize = (length+1+31)&0xffffffe0;
		alloc.pMem = (char*)malloc(alloc.bufferSize);
		memcpy(alloc.pMem, pString, length+1);
	}
}

#if 0
CopyFormatter::CopyFormatter(const char *pBaseString)
{
	pIn = pBaseString;
	pOut = workBuffer;

	fillChar = ' ';
	size = 0;
	decimals = 0;
	alignment = Center;
	err = false;

	while (*pIn)
	{
		if (*pIn == '<')
		{
			if (pIn[1] == '<')
			{
				*pOut++ = *pIn++;
				pIn++;
			}
			else
			{
				// found a format string...
				pIn++;
				if (*pIn == '-')
				{
					alignment = Left;
					pIn++;
				}
				else if (*pIn == '+')
				{
					alignment = Right;
					pIn++;
				}

				size = 0;
				int order = 1000;
				while (*pIn >= '0' && *pIn <= '9')
				{
					order /= 10;
					size += (*pIn - '0') * order;
					pIn++;
				}
				size /= order;

				if (*pIn != '.' && *pIn != '>')
				{
					err = true; return;
				}

				if (*pIn == '.' && pIn[1])
				{
					pIn++;
					decimals = 0;
					int order = 1000;
					while (*pIn >= '0' && *pIn <= '9')
					{
						order /= 10;
						decimals += (*pIn - '0') * order;
						pIn++;
					}
					decimals /= order;
				}

				if (*pIn == '#' && pIn[1])
				{
					fillChar = pIn[1];
					pIn+=2;
				}

				if (*pIn != '>')
				{
					err = true; return;
				}

				pIn++;
				return;
			}
		}
		else
		{
			*pOut++ = *pIn++;
		}
	}
	err = true;
}

const char *CopyFormatter::Finish(const char *pInsertString, int length)
{
	if (!err)
	{
		if (size == 0)
			size = length;
		int firstWrite = 0;
		if (alignment == Center)
			firstWrite = (size - length) / 2;
		else if (alignment == Right)
			firstWrite = (size - length);

		for (int i=0; i<size; i++)
		{
			if (i < firstWrite || i > firstWrite+length-1)
			{
				*pOut++ = (char)fillChar;
			}
			else
			{
				*pOut++ = *pInsertString++;
			}
		}

		while (*pIn)
		{
			*pOut++ = *pIn++;
		}

		*pOut++ = 0;
	}
	return workBuffer;
}


String &String::operator <<(const char *pValue)
{
	if (pStorage)
	{
		CopyFormatter formatter(pStorage->pString);

		if (!formatter.err)
		{
			Set(formatter.Finish(pValue, pValue ? strlen(pValue) : 0));
		}
	}
	return *this;
}

String &String::operator <<(const String &value)
{
	return *this << value.CStr();
}

String &String::operator <<(int value)
{
	char buf[100]; 
	sprintf(buf, "%d", value); 
	return *this << buf;
}

String &String::operator <<(float value)
{
	if (pStorage)
	{
		CopyFormatter formatter(pStorage->pString);

		char buf[100]; 
		sprintf(buf, "%.*f", formatter.decimals, value);

		if (!formatter.err)
		{
			Set(formatter.Finish(buf, strlen(buf)));
		}
	}
	return *this;
}

String &String::operator <<(void *pValue)
{
	return *this;
}
#endif

String String::AsUppercase() const
{
	int _length = Length();
	String uppercase;
	uppercase.SetLength( _length );
	for (int i=0; i<_length; i++)
	{
		uppercase.RawStr()[i] = (char)toupper(CStr()[i]);
	}
	return uppercase;
}

String String::AsLowercase() const
{
	int _length = Length();
	String lowercase;
	lowercase.SetLength( _length );
	for (int i=0; i<_length; i++)
	{
		lowercase.RawStr()[i] = (char)tolower(CStr()[i]);
	}
	return lowercase;
}

//**************************************************************************************
// handy string tokeniser
//**************************************************************************************
DMTokeniser::DMTokeniser(const char *pString, char separator) : m_separator(separator)
{
	StringCopy(m_buffer, pString, 256); 
	m_pRead = m_buffer; 
	Next();
}

const char *DMTokeniser::Next()
{
	m_token[0] = 0;
	if (*m_pRead != 0)
	{
		char *pWrite = m_token;
		for (int i=0; i<255 && *m_pRead && *m_pRead != m_separator; i++)
		{
			*pWrite++ = *m_pRead++;
		}
		*pWrite = 0;
		if (*m_pRead == m_separator)
			m_pRead++;
	}
	return m_token[0] ? m_token : 0;
}

String String::RemoveDelimitedExtension(const char *delimiters) const
{
	for (int i=Length()-1; i>0; --i)
	{
		for (const char *ch=delimiters; *ch!=0; ch++)
		{
			if (CStr()[i] == *ch)
				return String::CreateFromRange(CStr(), CStr()+i-1);
		}
	}
	return String(CStr());
}


//===========================================================================================
void String::SplitIntoFSAndPath(String &fs, String &path) const
{
	if (!SplitString(fs, path, ':'))
	{
		path = fs;
		fs = "";
	}
}


bool String::SplitString(String &head, String &tail, char delimiter) const
{
	const char *mid = strrchr(CStr(),delimiter);
	if (!mid)
	{
		head = CStr();
		tail = "";
		return false;
	}
	else
	{
		head = String::CreateFromRange(CStr(),mid-1);
		tail = String::CreateFromRange(mid+1,CStr()+Length()-1);
		return true;
	}
}

u8 String::GetHexByteAt(int idx) const
{
	const char *ch = CStr();
	char highChar = ch[idx];
	char lowChar = ch[idx + 1];
	int highVal, lowVal;
	if (highChar >= 'A')
		highVal = highChar - 'A' + 10;
	else
		highVal = highChar - '0';
	if (lowChar >= 'A')
		lowVal = lowChar - 'A' + 10;
	else
		lowVal = lowChar - '0';
	return (highVal << 4) | lowVal;
}

u8 String::GetHexNibbleAt(int idx) const
{
	const char *ch = CStr();
	char highChar = ch[idx];
	int highVal;
	if (highChar >= 'A')
		highVal = highChar - 'A' + 10;
	else
		highVal = highChar - '0';
	return highVal;
}


//===========================================================================================
void String::SplitIntoFileParts(String *pFilesys, String *pDirectory, String *pFilename, String *pExt) const
{
	const char *pSTART = 0;
	const char *pFS = 0;
	const char *pDIR = 0;
	const char *pEXT = 0;
	const char *pEND = 0;

	const char *ch = CStr();

	pSTART = ch;
	while (*ch)
	{
		if (*ch == ':' && !pFS)
		{
			pFS = ch;
			pEXT = 0;
		}
		else if (*ch == '/' || *ch == '\\')
		{
			pDIR = ch;
			pEXT = 0;
		}
		else if (*ch == '.')
		{
			pEXT = ch;
		}
		ch++;
	}
	pEND = ch;

	if (pFilesys && pFS)
	{
		*pFilesys = CreateFromRange(pSTART, pFS-1);
	}

	if (pDirectory && pDIR && pDIR > pSTART)
	{
		*pDirectory = CreateFromRange(pFS ? pFS+1 : pSTART, pDIR-1);
	}
	if (pFilename)
	{
		const char *pFNAME_Start = (pDIR?pDIR+1:(pFS?pFS+1:pSTART));
		const char *pFNAME_End = pEXT?pEXT-1:pEND-1;
		if (pFNAME_Start <= pFNAME_End)
		{
			*pFilename = CreateFromRange(pFNAME_Start, pFNAME_End);
		}
	}
	if (pExt && pEXT)
	{
		*pExt = CreateFromRange(pEXT, pEND-1);
	}
}

//===========================================================================================
String String::AddPath(const String &path) const
{
	String usePath = path;
	if (path[0] == '.' && path[1] == '\\')
		usePath = path.SubstrRight(path.Length()-2);

	int len = Length();
	if (len > 0)
	{
		if (*this == ".")
			return usePath;

		char lastCh = (*this)[len-1];
		if (lastCh != '/' && lastCh != '\\' && lastCh != ':')
		{
			return (*this) + "/" + usePath;
		}
	}
	return (*this) + usePath;
}

String String::CreateFromRange(const char *pStart, const char *pEnd)
{
	int length = (int)(pEnd-pStart)+1;
	return String(length, pStart);
}

String String::CreateFromFloat(float value)
{
	char dec[16];
	char result[32];

	bool isNegative = (value < 0.0f);
	if (isNegative)
		value = -value;

	// decimals
	int iv = (int)value;
	int dec_idx = 0;
	while (iv > 0 && dec_idx<12)
	{
		int niv = iv/10;
		dec[dec_idx++] = (char)('0' + iv-(niv*10));
		iv = niv;
	}
	if (dec_idx == 0)
		dec[dec_idx++] = '0';

	// flip the dec
	int out_idx=0;
	if (isNegative)
		result[out_idx++]='-';
	while (dec_idx > 0)
		result[out_idx++]=dec[--dec_idx];

	// fractions
	float fval = value-(float)((int)value);
	int frac_idx = 0;
	if (fval > 0.00001f)
	{
		result[out_idx++] = '.';
		while (fval > 0.001f && frac_idx<5)
		{
			float nfval = fval*10.0f;
			int ifval = (int)nfval;
			result[out_idx++] = (char)('0' + ifval);
			fval = nfval-(float)ifval;
			frac_idx++;
		}
	}
	result[out_idx++] = 0;
	return SZ(result);
}

String String::CreateFromInt(int value)
{
    char str[16];
    int idx = 15;
    str[idx--] = 0;
    bool negative = (value < 0);
    do
    {
        str[idx--] = (value % 10) + '0';
        value /= 10;
    } while (value > 0);

    if (negative)
        str[idx--] = '-';

    return SZ(str);
}

String String::Replace(char oldChar, char newChar) const
{
	String result = *this;
	int count = result.Length();
	for (int i=0; i<count; i++)
	{
		if (result[i] == oldChar)
			result[i] = newChar;
	}
	return result;
}

bool StringToIndex(const String &name, const char **valueArray, int arraySize, int &idx)
{
	for (int i=0; i<arraySize; i++)
	{
		if (name == valueArray[i])
		{
			idx = i;
			return true;
		}
	}
	return false;
}

bool IndexToString(int idx, const char **valueArray, int arraySize, String &name)
{
	if (idx < 0 || idx >= arraySize)
		return false;
	name = valueArray[idx];
	return true;
}

//===============================================================================
static bool IsOneOf(char val, const char *pDelimiters)
{
  while ((*pDelimiters != 0) && (val != *pDelimiters))
    pDelimiters++;
    
  return (*pDelimiters == 0);
}

void String::ToStringList(std::vector<String> &stringList, const char *pDelimiters) const
{
  stringList.clear();
  const char *pStart = CStr();

  while (*pStart)
  {
    const char *pEnd = pStart;

    while (*pEnd != 0 && IsOneOf(*pEnd, pDelimiters))
      pEnd++;
      
    stringList.push_back( CreateFromRange(pStart,pEnd-1) );
    pStart = pEnd;

    if (*pStart != 0)
      pStart++;
  }
}

void String::ToStringListTrimmed(std::vector<String> &stringList, const char *pDelimiters) const
{
	stringList.clear();
	const char *pStart = CStr();

	while (*pStart)
	{
		const char *pEnd = pStart;

		while (*pEnd != 0 && IsOneOf(*pEnd, pDelimiters))
			pEnd++;


		const char *pTrimmedStart = pStart;
		while (*pTrimmedStart == ' ' && pTrimmedStart < pEnd)
			pTrimmedStart++;
		const char *pTrimmedEnd = pEnd-1;
		while (*pTrimmedEnd == ' ' && pTrimmedEnd > pTrimmedStart)
			pTrimmedEnd--;

		if (pTrimmedEnd >= pTrimmedStart)
			stringList.push_back(CreateFromRange(pTrimmedStart, pTrimmedEnd));
		pStart = pEnd;

		if (*pStart != 0)
			pStart++;
	}
}

u16 String::PopUTF8(int &idx) const
{
	const char *str = CStr();
	u16 result = 0;
    if (str[idx] == 0)
        return 0;
	
    if ((str[idx] & 0x80)==0)
    {
		result = (u16)str[idx++];
    }
	else if ((str[idx] & 0xe0) == 0xc0)
	{
		result = (((u16)(str[idx]&0x1f))<<6) | (str[idx+1]&0x3f);
		idx += 2;
	}
	if ((str[idx] & 0xf0) == 0xe0)
	{
		result = (((u16)(str[idx]&0xf))<<12) | (((u16)(str[idx+1]&0x3f))<<6) | ((u16)(str[idx+2]&0x3f));
		idx += 3;
	}
	return result;
}

int String::ConvertUTF8(u16 *buffer, int bufferLength)
{
	int in = 0;
	int out = 0;
	while (out < bufferLength)
	{
		u16 ch = PopUTF8(in);
		buffer[out++] = ch;
		if (ch==0)
			break;
	}
	return out;
}

void StringCopy(char* pBuffer, const char* pString, int bufferSize)
{
	for (int i = 0; i < bufferSize - 1 && *pString; i++)
	{
		*pBuffer++ = *pString++;
	}
	*pBuffer++ = 0;
}

#if defined(_WIN32)

bool StringEqual(const char* string1, const char* string2)
{
	return string1 && string2 && !_stricmp(string1, string2);
}

bool StringStarts(const char* string1, const char* startString)
{
	while (*string1 && *startString)
	{
		if ((*string1++ & ~32) != (*startString++ & ~32))
			return false;
	}
	return (*startString == 0);
}

bool StringContains(const char* string, const char* substr)
{
	const char* root = string;
	while (*root)
	{
		const char* sub = substr;
		const char* rootIt = root;
		while (*sub)
		{
			if ((*sub & ~32) != (*rootIt & ~32))
				break;
			sub++;
			rootIt++;
		}
		if (*sub == 0)
			return true;
		root++;
	}
	return false;
}

int StringCompare(const char* string1, const char* string2)
{
	if (string1 == 0 || string1[0] == 0)
		return (string2 == 0 || string2[0] == 0) ? 0 : 1;
	else if (string2 == 0 || string2[0] == 0)
		return -1;

	return _stricmp(string1, string2);
}

char* StringDup(const char* source)
{
	char* pMem = new char[strlen(source) + 1];
	strcpy(pMem, source);
	return pMem;
}

#else

bool StringEqual(const char* string1, const char* string2)
{
	return !strcasecmp(string1, string2);
}

int StringCompare(const char* string1, const char* string2)
{
	return strcasecmp(string1, string2);
}

char* StringDup(const char* source)
{
	return strdup(source);
}

bool StringStarts(const char* string1, const char* startString)
{
	while (*string1 && *startString)
	{
		if ((*string1++ & ~32) != (*startString++ & ~32))
			return false;
	}
	return true;
}

#endif

