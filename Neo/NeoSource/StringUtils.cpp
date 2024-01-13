#include "Neo.h"

u16 StringPopUTF8(const std::string& source, int& idx)
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

std::string StringReplace(const std::string& str, char oldChar, char newChar)
{
    char* buffer = new char[str.size()];
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
    return std::string(buffer);
}

u64 StringHash64(const std::string& str)
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

void StringSplitIntoFileParts(const std::string &str, std::string *pFilesys, std::string *pDirectory, std::string* pFilename, std::string* pExt)
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
        *pFilesys = std::string(pSTART, pFS);
    }

    if (pDirectory && pDIR && pDIR > pSTART)
    {
        const char* dirStart = pFS ? pFS + 1 : pSTART;
        size_t len = pDIR - dirStart;
        *pDirectory = std::string(dirStart, len);
    }
    if (pFilename)
    {
        const char* pFNAME_Start = (pDIR ? pDIR + 1 : (pFS ? pFS + 1 : pSTART));
        const char* pFNAME_End = pEXT ? pEXT : pEND;
        size_t len = pFNAME_End - pFNAME_Start;
        if (pFNAME_Start <= pFNAME_End)
        {
            *pFilename = std::string(pFNAME_Start, len);
        }
    }
    if (pExt && pEXT)
    {
        size_t len = pEND - pEXT;
        *pExt = std::string(pEXT, len);
    }
}

std::string StringAddPath(const std::string& base, const std::string& path)
{
    std::string result = base;

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

bool StringSplit(const std::string& input, std::string &left, std::string &right, char delimiter)
{
    size_t pos = input.find(delimiter);
    if (pos != std::string::npos) 
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



void StringSplitIntoFSAndPath(const std::string &str, std::string &fs, std::string &path)
{
    if (!StringSplit(str, fs, path, ':'))
    {
        path = fs;
        fs = "";
    }
}


