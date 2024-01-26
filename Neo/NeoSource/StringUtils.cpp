#include "Neo.h"
#include "StringUtils.h"
#include <random>

u16 StringPopUTF8(const string& source, int& idx)
{
    const char* str = source.c_str();
    u16 result = 0;
    if (str[idx] == 0)
        return 0;

    if ((str[idx] & 0x80) == 0)
    {
        result = (u16)str[idx++];
    }
    else if ((str[idx] & 0xe0) == 0xc0)
    {
        result = (((u16)(str[idx] & 0x1f)) << 6) | (str[idx + 1] & 0x3f);
        idx += 2;
    }
    if ((str[idx] & 0xf0) == 0xe0)
    {
        result = (((u16)(str[idx] & 0xf)) << 12) | (((u16)(str[idx + 1] & 0x3f)) << 6) | ((u16)(str[idx + 2] & 0x3f));
        idx += 3;
    }
    return result;
}

string StringReplace(const string& str, char oldChar, char newChar)
{
    char* buffer = new char[str.size()+1];
    int inIdx = 0;
    int idx = 0;
    int outIdx = 0;
    u16 val;
    while (val = StringPopUTF8(str, idx))
    {
        if (val == (u16)oldChar)
        {
            buffer[outIdx++] = newChar;
            inIdx = idx;
        }
        else
        {
            while (inIdx < idx)
                buffer[outIdx++] = str[inIdx++];
        }
    }
    buffer[outIdx++] = 0;
    string result(buffer);
    delete buffer;
    return result;
}

u64 StringHash64(const string& str)
{
    u64 offsetBasis = 2166136261;
    u64 prime = 16777619;
    u64 hash = offsetBasis;
    const char* pCH = str.c_str();
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

void StringSplitIntoFileParts(const string &str, string *pFilesys, string *pDirectory, string* pFilename, string* pExt)
{
    const char* pSTART = 0;
    const char* pFS = 0;
    const char* pDIR = 0;
    const char* pEXT = 0;
    const char* pEND = 0;

    const char* ch = str.c_str();
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
        size_t len = pFS - pSTART;
        *pFilesys = string(pSTART, pFS);
    }

    if (pDirectory && pDIR && pDIR > pSTART)
    {
        const char* dirStart = pFS ? pFS + 1 : pSTART;
        size_t len = pDIR - dirStart;
        *pDirectory = string(dirStart, len);
    }
    if (pFilename)
    {
        const char* pFNAME_Start = (pDIR ? pDIR + 1 : (pFS ? pFS + 1 : pSTART));
        const char* pFNAME_End = pEXT ? pEXT : pEND;
        size_t len = pFNAME_End - pFNAME_Start;
        if (pFNAME_Start <= pFNAME_End)
        {
            *pFilename = string(pFNAME_Start, len);
        }
    }
    if (pExt && pEXT)
    {
        size_t len = pEND - pEXT;
        *pExt = string(pEXT, len);
    }
}

string StringAddPath(const string& base, const string& path)
{
    string result = base;

    // Check if the first string is empty or doesn't end with a backslash
    if (base.back() != '\\' || base.back() == '.') 
    {
        // Insert a backslash between the strings
        result += '\\';
    }

    // Concatenate the second string to the first
    result += path;
    return result;
}

bool StringSplit(const string& input, string &left, string &right, char delimiter)
{
    size_t pos = input.find(delimiter);
    if (pos != string::npos) 
    {
        left = input.substr(0, pos);
        right = input.substr(pos + 1);
        return true;
    }
    else 
    {
        left = input;
        right = "";
        return false;
    }
}



void StringSplitIntoFSAndPath(const string &str, string &fs, string &path)
{
    if (!StringSplit(str, fs, path, ':'))
    {
        path = fs;
        fs = "";
    }
}


u8 StringGetHexByteAt(const string &str, int idx)
{
    const char* ch = str.c_str();
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

u8 StringGetHexNibbleAt(const string &str, int idx)
{
    const char* ch = str.c_str();
    char highChar = ch[idx];
    int highVal;
    if (highChar >= 'A')
        highVal = highChar - 'A' + 10;
    else
        highVal = highChar - '0';
    return highVal;
}

u64 GenerateRandomU64()
{
    static std::random_device rd;
    static std::mt19937_64 generator(rd());
    static std::uniform_int_distribution<uint64_t> distribution(std::numeric_limits<uint64_t>::min(), std::numeric_limits<uint64_t>::max());

    // Create a random number generator engine (Mersenne Twister 64-bit version)
    // Define a uniform distribution for 64-bit unsigned integers
    // Generate a random 64-bit unsigned integer
    return distribution(generator);
}

u32 StringToHex(const string &str)
{
    u32 result = 0;
    const char* in = str.c_str();
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

u64 StringToHex64(const string &str)
{
    u64 result = 0;
    const char* in = str.c_str();
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


int StringFindInList(const string& str, const stringlist& list)
{
    for (int i = 0; i < list.size(); i++)
    {
        if (StringEqual(str, list[i]))
            return i;
    }
    return -1;
}


