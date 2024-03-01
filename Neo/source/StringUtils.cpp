#include "Neo.h"
#include "StringUtils.h"
#include <random>

#include <iostream>
#include <string>
#include <cstdint>

vector<u32> StringToUnicode(const string& utf8_string)
{
    vector<u32> unicode_values;

    for (size_t i = 0; i < utf8_string.length();)
    {
        uint32_t unicode_val = 0;
        uint8_t num_bytes = 0;

        if ((utf8_string[i] & 0b10000000) == 0)
        {
            // 1-byte character
            unicode_val = utf8_string[i];
            num_bytes = 1;
        }
        else if ((utf8_string[i] & 0b11100000) == 0b11000000)
        {
            // 2-byte character
            unicode_val = utf8_string[i] & 0b00011111;
            unicode_val <<= 6;
            unicode_val |= utf8_string[i + 1] & 0b00111111;
            num_bytes = 2;
        }
        else if ((utf8_string[i] & 0b11110000) == 0b11100000)
        {
            // 3-byte character
            unicode_val = utf8_string[i] & 0b00001111;
            unicode_val <<= 6;
            unicode_val |= utf8_string[i + 1] & 0b00111111;
            unicode_val <<= 6;
            unicode_val |= utf8_string[i + 2] & 0b00111111;
            num_bytes = 3;
        }
        else if ((utf8_string[i] & 0b11111000) == 0b11110000)
        {
            // 4-byte character
            unicode_val = utf8_string[i] & 0b00000111;
            unicode_val <<= 6;
            unicode_val |= utf8_string[i + 1] & 0b00111111;
            unicode_val <<= 6;
            unicode_val |= utf8_string[i + 2] & 0b00111111;
            unicode_val <<= 6;
            unicode_val |= utf8_string[i + 3] & 0b00111111;
            num_bytes = 4;
        }
        else
        {
            // Invalid UTF-8 sequence - just return the original string
            unicode_values.clear();
            for (auto ch : utf8_string)
                unicode_values.push_back((u32)ch);
            break;
        }

        unicode_values.push_back(unicode_val);
        i += num_bytes;
    }

    return unicode_values;
}

string StringReplace(const string& str, char oldChar, char newChar)
{
    string replace = str;
    for (size_t i = 0; i < str.size(); i++)
        if (str[i] == oldChar)
            replace[i] = newChar;
    return replace;
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

// Split a string into a vector of strings using a comma as a delimiter
vector<string> StringSplit(const string& src, char delimiter)
{
    vector<string> result;
    std::istringstream iss(src);
    string token;

    while (std::getline(iss, token, delimiter)) 
    {
        result.push_back(StringTrim(token));
    }
    return result;
}

vector<string> StringSplitIntoTokens(const string& src)
{
    vector<string> result;
    string token;
    enum Mode
    {
        None,
        Alphabetic,
        WhiteSpace,
        Other
    } currMode = None;
    for (auto ch : src)
    {
        Mode chMode = None;
        if (ch == '\r')
            continue;

        if ((ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z') || ch == '_' || (ch >= '0' && ch <= '9'))
        {
            chMode = Alphabetic;
        }
        else if (ch == ' ' || ch == '\t')
        {
            chMode = WhiteSpace;
        }
        else
        {
            chMode = Other;
        }

        if (currMode != None && currMode != WhiteSpace && currMode != chMode)
        {
            result.push_back(token);
            token.clear();
        }

        currMode = chMode;
        if (currMode != WhiteSpace)
            token += ch;
    }
    if (currMode != WhiteSpace && currMode != None)
        result.push_back(token);

    return result;
}

string StringTrim(const string& str)
{
    size_t first = str.find_first_not_of(" \t\n\r");
    size_t last = str.find_last_not_of(" \t\n\r");

    if (first == std::string::npos || last == std::string::npos)
        return "";

    return str.substr(first, last - first + 1);
}
