#include "Neo.h"
#include "Serializer.h"
#include "MathUtils.h"

// safe cross platform method of converting floats to ints
// most other methods suffer from aliasing optimisation errors
union FloatConvertor
{
	float fValue;
	u32 value;
};

union DoubleConvertor
{
	double fValue;
	u64 value;
};

float Serializer::ReadF32()
{
	FloatConvertor convert;
	convert.value = ReadU32();
	return convert.fValue;
}

double Serializer::ReadF64()
{
	DoubleConvertor convert;
	convert.value = ReadU64();
	return convert.fValue;
}

u64 Serializer::ReadU64()
{
	u64 value=0;
	for (int i=0; i<8; i++)
		value = (value<<8) | ReadU8();
	return value;
}

u32 Serializer::ReadU32()
{
	u32 value=0;
	for (int i=0; i<4; i++)
		value = (value<<8) | ReadU8();
	return value;
}

string Serializer::ReadString()
{
	string value;
	u32 length = ReadU32();

	if (length > 0)
	{
		if (HasBufferRemaining(length))
		{
			if (length < 256)
			{
				u8 buff[256];
				ReadMemory(buff, length);
				value = string(buff, buff+length);
			}
			else
			{
				u8* buff = new u8[length];
				ReadMemory(buff, length);
				value = string(buff, buff + length);
				delete[] buff;
			}
		}
		else
		{
			Error("String reading is corrupt!");
		}
	}

	return value;
}

vec3 Serializer::ReadVec3()
{
	vec3 vec;
	vec.x = ReadF32(); vec.y = ReadF32(); vec.z = ReadF32();
	return vec;
}

vec4 Serializer::ReadVec4()
{
    vec4 vec;
    vec.x = ReadF32(); vec.y = ReadF32(); vec.z = ReadF32(); vec.w = ReadF32();
    return vec;
}

ivec2 Serializer::ReadIVec2()
{
	ivec2 vec;
	vec.x = ReadI32(); vec.y = ReadI32();
	return vec;
}

ivec3 Serializer::ReadIVec3()
{
	ivec3 vec;
	vec.x = ReadI32(); vec.y = ReadI32(); vec.z = ReadI32();
	return vec;
}

quat Serializer::ReadQuat()
{
	quat vec;
	vec.x = ReadF32(); vec.y = ReadF32(); vec.z = ReadF32(); vec.w = ReadF32();
	return vec;
}

u32 Serializer::ReadID()
{
	return ReadU32();
}

void Serializer::WriteF64(double value)
{
	DoubleConvertor cv;
	cv.fValue = value;
	WriteU64(cv.value);
}

void Serializer::WriteF32(float value)
{
	FloatConvertor cv;
	cv.fValue = value;
	WriteU32(cv.value);
}

void Serializer::WriteU64(u64 value)
{
	WriteU8((u8)(value >> 56));
	WriteU8((u8)(value >> 48));
	WriteU8((u8)(value >> 40));
	WriteU8((u8)(value >> 32));
	WriteU8((u8)(value >> 24));
	WriteU8((u8)(value >> 16));
	WriteU8((u8)(value >> 8));
	WriteU8((u8)(value));
}

void Serializer::WriteU32(u32 value)
{
	WriteU8((u8)(value >> 24));
	WriteU8((u8)(value >> 16));
	WriteU8((u8)(value >> 8));
	WriteU8((u8)(value));
}

void Serializer::WriteU16(u16 value)
{
	WriteU8((u8)(value >> 8));
	WriteU8((u8)(value));
}

void Serializer::WriteString(const string &value)
{
	u32 length = (u32)value.size();
	WriteU32(length);
	if (length > 0)
		WriteMemory((u8*)(value.c_str()), length);
}

void Serializer::WriteVec3(const vec3 &value)
{
	WriteF32(value.x); WriteF32(value.y); WriteF32(value.z);
}

void Serializer::WriteIVec3(const ivec3 &value)
{
	WriteI32(value.x); WriteI32(value.y); WriteI32(value.z);
}

void Serializer::WriteQuat(const quat &value)
{
	WriteF32(value.x); WriteF32(value.y); WriteF32(value.z);
}

void Serializer::WriteID(u32 value)
{
	WriteU32(value);
}

void Serializer::SerializeMemory(u8* &pMem, u32 &size)
{
	if (IsReadMode())
	{
		size = ReadU32();
		if (size > 0)
		{
			pMem = (u8 *)malloc(size);
			ReadMemory(pMem,size);
		}
		else
			pMem = 0;
	}
	else
	{
		WriteU32(size);
		if (size > 0)
			WriteMemory(pMem,size);
	}
}


//=============================================================================================
// BINARY WRITE
//=============================================================================================

Serializer_BinaryWrite::Serializer_BinaryWrite(u8 *mem, int size)
{
	// write to this file...
	m_pMem = (char *)mem;
	m_memSize = size;
	m_memUsage = 0;
	m_chunkStackSize = 0;
}

void Serializer_BinaryWrite::WriteMemory(const u8 *pMem, u32 size)
{
	Assert(m_memUsage+size <= m_memSize, "Serializer Write out of memory!");
	memcpy(m_pMem+m_memUsage, pMem, size);
	m_memUsage += size;
}

void Serializer_BinaryWrite::WriteU8(u8 value)
{
	Assert(m_memUsage+1 <= m_memSize, "Serializer Write out of memory!");
	m_pMem[m_memUsage] = (char)value;
	m_memUsage += 1;
}

void Serializer_BinaryWrite::StartBlock()
{
	Assert(m_memUsage+4 <= m_memSize, "Serializer Write out of memory!");
	m_chunkStack[m_chunkStackSize++] = m_memUsage;
	m_memUsage += 4;
}

void Serializer_BinaryWrite::EndBlock()
{
	Assert(m_chunkStackSize > 0, "No open chunks!");
	int offset = m_chunkStack[--m_chunkStackSize];
	int value = m_memUsage - offset - 4;
	m_pMem[offset+0] = (u8)(value >> 24);
	m_pMem[offset+1] = (u8)(value >> 16);
	m_pMem[offset+2] = (u8)(value >> 8);
	m_pMem[offset+3] = (u8)value;
}

//=============================================================================================
// BINARY WRITE GROW
//=============================================================================================

Serializer_BinaryWriteGrow::Serializer_BinaryWriteGrow()
{
	m_chunkStackSize = 0;
}

void Serializer_BinaryWriteGrow::WriteMemory(const u8 *pMem, u32 size)
{
	u32 oldSize = (int)mem.size();
	u32 newSize = oldSize + size;
	mem.reserve(Max(oldSize * 2, newSize));
	mem.resize(newSize);
	memcpy(&mem[oldSize], pMem, size);
}

void Serializer_BinaryWriteGrow::WriteU8(u8 value)
{
	mem.push_back((char)value);
}

void Serializer_BinaryWriteGrow::StartBlock()
{
	m_chunkStack[m_chunkStackSize++] = (int)mem.size();

	int oldSize = (int)mem.size();
	int newSize = oldSize + 4;
	mem.reserve(Max(oldSize * 2, newSize));
	mem.resize( newSize );
}

void Serializer_BinaryWriteGrow::EndBlock()
{
	Assert(m_chunkStackSize > 0, "No open chunks!");
	int offset = m_chunkStack[--m_chunkStackSize];
	int value = (int)mem.size() - offset - 4;
	mem[offset+0] = (u8)(value >> 24);
	mem[offset+1] = (u8)(value >> 16);
	mem[offset+2] = (u8)(value >> 8);
	mem[offset+3] = (u8)value;
}

//=============================================================================================
// BINARY SIZE
//=============================================================================================

Serializer_BinarySize::Serializer_BinarySize()
{
	// write to this file...
	m_memUsage = 0;
}

void Serializer_BinarySize::WriteMemory(const u8 *pMem, u32 size)
{
	m_memUsage += size;
}

void Serializer_BinarySize::WriteU8(u8 value)
{
	m_memUsage += 1;
}

void Serializer_BinarySize::StartBlock()
{
	m_memUsage += 4;
}

void Serializer_BinarySize::EndBlock()
{
}


//=============================================================================================
// BINARY READ
//=============================================================================================

Serializer_BinaryRead::Serializer_BinaryRead(u8 *mem, u32 size) : m_pMem((char *)mem), m_memSize(size), m_memUsage(0),
	m_bitMarker(0), m_chunkStackSize(0) {}

void Serializer_BinaryRead::ReadMemory(u8 *pMem, u32 size)
{
	Assert(m_memUsage+size <= m_memSize, "Out of buffer in serializer!");
	memcpy(pMem, (u8*)m_pMem + m_memUsage, size);
	m_memUsage += size;
}

u8 Serializer_BinaryRead::ReadU8()
{
	if (m_memUsage+1 > m_memSize)
	{
		Log("Serializer block out of mem!");
		SetError(SerializerError_TruncatedBlock);
		return 0;
	}

	return m_pMem[m_memUsage++];
}

void Serializer_BinaryRead::StartBlock()
{
	u32 size;
	*this >> size;
	if (m_memUsage + size > m_memSize)
	{
		Log("Serializer block out of mem!");
		SetError(SerializerError_TruncatedBlock);
		return;
	}
	m_chunkStack[m_chunkStackSize++] = m_memUsage + size;
}

bool Serializer_BinaryRead::HasBufferRemaining(u32 size)
{
	u32 memSize = (m_chunkStackSize > 0) ? (u32)(m_chunkStack[m_chunkStackSize-1]) : m_memSize;
	return (memSize - m_memUsage) >= size;
}

void Serializer_BinaryRead::EndBlock()
{
	Assert(m_chunkStackSize > 0, "no open chunks!");
	m_memUsage = m_chunkStack[--m_chunkStackSize];
}

//=====================================================================================================================
// Binary stream compressor
//=====================================================================================================================

Serializer_BinaryStreamWrite::Serializer_BinaryStreamWrite(const string &filename)
{
	m_memUsed = 0;
	m_chunkStackSize = 0;
	m_encryptMask = 0x95;

	if (FileManager::Instance().StreamWriteBegin(m_fileHandle, filename))
	{
		m_rawMem = new u8[CHUNKSIZE];
		m_compressedMem = new u8[CHUNKSIZE];
	}
	else
	{
		Log(string("Failed to create file: ") + filename);
		m_rawMem = 0;
		m_compressedMem = 0;
	}
}

#if 0
static void DumpMem(const char *title, u8 *mem, int size)
{
	DMLOG(title);
	int i=0;
	DMString line;
	while (i<size)
	{
		line += STR("%d,",*mem++);
		if ((i & 7) == 7)
		{
			DMLOG(line.CStr());
			line="";
		}
		i++;
	}
	if (!line.IsEmpty())
		DMLOG(line.CStr());
}
#endif

void Serializer_BinaryStreamWrite::Flush()
{
	Assert(m_chunkStackSize == 0, "Attempt to flush while a block is open!");
	if (m_memUsed > 0)
	{
		// compress
		z_stream strm;
		strm.zalloc = Z_NULL;
		strm.zfree = Z_NULL;
		strm.opaque = Z_NULL;
		deflateInit(&strm, Z_DEFAULT_COMPRESSION);
		strm.avail_in = m_memUsed;
		strm.next_in = m_rawMem;
		strm.avail_out = CHUNKSIZE;
		strm.next_out = m_compressedMem;
		deflate(&strm, Z_FINISH);
		u32 compressedSize = CHUNKSIZE - strm.avail_out;
		deflateEnd(&strm);
		//DMLOG("Compress chunk %d -> %d (%d%%)",m_memUsed,compressedSize,compressedSize*100/m_memUsed);

		// encrypt
		u32 chksum = 0;
		u8 *encrypt = m_compressedMem;
		u8 mask = m_encryptMask;
		for (u32 i=0; i<compressedSize; i++)
		{
			u8 decryptedData = *encrypt;
			u8 encryptedData = decryptedData ^ mask;
			*encrypt++ = encryptedData;
			mask = encryptedData;
			chksum += mask;
		}
		m_encryptMask = mask;

		// write header size, crc
		u8 header[8];
		header[0] = (u8)(compressedSize >> 24);
		header[1] = (u8)(compressedSize >> 16);
		header[2] = (u8)(compressedSize >> 8);
		header[3] = (u8)(compressedSize);
		header[4] = (u8)(chksum >> 24);
		header[5] = (u8)(chksum >> 16);
		header[6] = (u8)(chksum >> 8);
		header[7] = (u8)(chksum);
		FileManager::Instance().StreamWrite(m_fileHandle, header, 8);

		// write
		FileManager::Instance().StreamWrite(m_fileHandle, m_compressedMem, compressedSize);
		m_memUsed = 0;
	}
}

Serializer_BinaryStreamWrite::~Serializer_BinaryStreamWrite()
{
	Finalise();
}

void Serializer_BinaryStreamWrite::Finalise()
{
	if (m_fileHandle)
	{
		Flush();
		FileManager::Instance().StreamWriteEnd(m_fileHandle);
		m_fileHandle = 0;
	}
	delete m_rawMem;
	delete m_compressedMem;
}

void Serializer_BinaryStreamWrite::WriteU8(u8 value)
{
	Assert(m_memUsed < CHUNKSIZE, "Out of mem buffer");
	m_rawMem[m_memUsed++] = value;
}

void Serializer_BinaryStreamWrite::WriteMemory(const u8* outMem, u32 size)
{
	memcpy(m_rawMem + m_memUsed, outMem, size);
	m_memUsed += size;
}

void Serializer_BinaryStreamWrite::StartBlock()
{
	m_chunkStack[m_chunkStackSize++] = m_memUsed;
	m_memUsed += 4;
}

void Serializer_BinaryStreamWrite::EndBlock()
{
	Assert(m_chunkStackSize > 0, "Attempt to end a block but none open!");

	int offset = m_chunkStack[--m_chunkStackSize];
	int value = m_memUsed - offset - 4;
	m_rawMem[offset+0] = (u8)(value >> 24);
	m_rawMem[offset+1] = (u8)(value >> 16);
	m_rawMem[offset+2] = (u8)(value >> 8);
	m_rawMem[offset+3] = (u8)value;

	if (m_chunkStackSize == 0 && m_memUsed > CHUNKSIZE/2)
		Flush();
}

//=====================================================================================================================
// Binary stream decompressor
//=====================================================================================================================

bool Serializer_BinaryStreamRead::OpenFile(const string &filename)
{
	if (!FileManager::Instance().GetSize(filename, m_fileSizeTotal))
		return false;

	m_fileSizeRemaining = m_fileSizeTotal;

	if (FileManager::Instance().StreamReadBegin(m_fileHandle, filename))
		return true;

	return false;
}
u32 Serializer_BinaryStreamRead::ReadFileBlock(u8 *buffer, u32 size)
{
	u32 sizeToRead = Min(size, m_fileSizeRemaining);
	u32 sizeRead = 0;
	if (sizeToRead > 0)
	{
		FileManager::Instance().StreamRead(m_fileHandle, buffer, sizeToRead, sizeRead);
		m_fileSizeRemaining -= sizeRead;
	}
	return sizeRead;
}
void Serializer_BinaryStreamRead::CloseFile()
{
	FileManager::Instance().StreamReadEnd(m_fileHandle);
}

Serializer_BinaryStreamRead::Serializer_BinaryStreamRead(const string &filename)
{
	m_decryptMask = 0x95;
	m_chunkStackSize = 0;

	if (OpenFile(filename))
	{
		m_rawMem = new u8[CHUNKSIZE];
		m_compressedMem = new u8[CHUNKSIZE];
		ReadAndDecompress();
	}
	else
	{
		m_rawMem = 0;
		m_compressedMem = 0;
		m_decryptMask = 0;
		m_memUsed = 0;
		m_memSize = 0;
		m_chunkStackSize = 0;
		SetError(SerializerError_FileNotFound);
		Log(string("Unable to open stream: ") + filename);
	}
}

Serializer_BinaryStreamRead::~Serializer_BinaryStreamRead()
{
	Finalise();
}

void Serializer_BinaryStreamRead::Finalise()
{
	CloseFile();
	delete m_rawMem;
	delete m_compressedMem;
}

void Serializer_BinaryStreamRead::AbortRead(SerializerError errCode)
{
	if (errCode != SerializerError_OK)
	{
		SetError(errCode);
	}
	m_memSize = 0;
	m_memUsed = 0;
	m_chunkStackSize = 0;
}

void Serializer_BinaryStreamRead::ReadAndDecompress()
{
	u8 header[8];
	int readAmount = ReadFileBlock(header,8);
	
	if (readAmount == 0)
	{
		// file complete
		AbortRead(SerializerError_OK);
		return;
	}

	if (readAmount < 8)
	{
		AbortRead(SerializerError_TruncatedBlock);
		return;
	}

	u32 compressedSize = ((u32)header[0]<<24) | ((u32)header[1]<<16) | ((u32)header[2]<<8) | (u32)header[3];
	u32 filechksum = ((u32)header[4]<<24) | ((u32)header[5]<<16) | ((u32)header[6]<<8) | (u32)header[7];

	readAmount = ReadFileBlock(m_compressedMem, compressedSize);
	if (readAmount != compressedSize)
	{
		AbortRead(SerializerError_TruncatedBlock);
		return;
	}

	// decrypt...
	u32 chksum = 0;
	u8 *decrypt = m_compressedMem;
	u8 mask = m_decryptMask;
	for (u32 i=0; i<compressedSize; i++)
	{
		u8 encryptedData = *decrypt;
		u8 decryptedData = encryptedData ^ mask;
		*decrypt++ = decryptedData;
		mask = encryptedData;
		chksum += mask;
	}
	m_decryptMask = mask;
	if (chksum != filechksum)
	{
		AbortRead(SerializerError_BadData);
		return;
	}

	// decompress
	z_stream strm;
	strm.zalloc = Z_NULL;
	strm.zfree = Z_NULL;
	strm.opaque = Z_NULL;
	strm.avail_in = 0;
	strm.next_in = Z_NULL;
	inflateInit(&strm);
	strm.avail_in = compressedSize;
	strm.next_in = m_compressedMem;
	strm.avail_out = CHUNKSIZE;
	strm.next_out = m_rawMem;
	inflate(&strm, Z_NO_FLUSH);
	m_memSize = CHUNKSIZE - strm.avail_out;
	inflateEnd(&strm);
	m_memUsed = 0;
}

u8 Serializer_BinaryStreamRead::ReadU8()
{
	if (m_memUsed + 1 <= m_memSize)
		return m_rawMem[m_memUsed++];
	else
	{
		Error("Out of memory on read!");
		return 0;
	}
}

void Serializer_BinaryStreamRead::ReadMemory(u8 *pMem, u32 size)
{
	if (m_memUsed + size <= m_memSize)
	{
		memcpy(pMem, m_rawMem+m_memUsed, size);
		m_memUsed += size;
	}
	else
	{
		Error("Out of memory on read!");
	}
}

void Serializer_BinaryStreamRead::StartBlock()
{
	u32 size;
	*this >> size;
	m_chunkStack[m_chunkStackSize++] = m_memUsed + size;
}

void Serializer_BinaryStreamRead::EndBlock()
{
	Assert(m_chunkStackSize > 0, "Tried to endblock, but there are no blocks to end!!!");
	m_memUsed = m_chunkStack[--m_chunkStackSize];
	if (m_memUsed == m_memSize && m_chunkStackSize == 0)
		ReadAndDecompress();
}

bool Serializer_BinaryStreamRead::HasBufferRemaining(u32 size)
{
	int memSize = (m_chunkStackSize > 0) ? m_chunkStack[m_chunkStackSize-1] : m_memSize;
	int remaining = memSize-m_memUsed;
	return (remaining >= 0) && ((u32)remaining >= size);
}
