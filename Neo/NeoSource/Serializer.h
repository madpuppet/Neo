#pragma once

#include "FileManager.h"
#include "zlib.h"

enum SerializerError
{
	SerializerError_OK,
	SerializerError_FileNotFound,
	SerializerError_BadVersion,
	SerializerError_BufferEmpty,
	SerializerError_TruncatedBlock,
	SerializerError_BadData
};

class Serializer
{
public:
	Serializer() : m_error(SerializerError_OK) {}
	virtual ~Serializer() {}

	// has buffer remaining for read/write operations...
	// if inside a block, then it refers just to the block
	virtual bool HasBufferRemaining(u32 size = 1) = 0;

	// read or write mode?
	virtual bool IsReadMode() = 0;

	// start a scoped block
	// - allows deserilaizer to skip unknown blocks
	virtual void StartBlock() = 0;

	// end of block...
	// - if reading, this will skip to the end of the block
	virtual void EndBlock() = 0;

	// read/write bits that child classes must implement
	virtual u8 ReadU8() = 0;
	virtual void ReadMemory(u8 *mem, u32 size) = 0;
	virtual void WriteU8(u8 value) = 0;
	virtual void WriteMemory(const u8 *mem, u32 size) = 0;

    // restart reading/writing
    virtual void Restart() = 0;

	//=====================================================================================
	// check error code...
	bool IsGood() { return m_error == SerializerError_OK; }
	SerializerError GetError() { return m_error; }
	void SetError(SerializerError code) { m_error = code; }
	virtual u32 DataUsed() = 0;
	virtual u32 DataTotal() = 0;
	virtual u8 *DataStart() { return 0; }
	virtual int DataSize() { return 0; }

	//=====================================================================================
	// write interface wrappers
	void WriteF64(f64 value);
	void WriteF32(f32 value);
	void WriteU64(u64 value);
	void WriteU32(u32 value);
	void WriteU16(u16 value);
	void WriteI64(i64 value) { WriteU64((u64)value); }
	void WriteI32(i32 value) { WriteU32((u32)value); }
	void WriteI16(i16 value) { WriteU16((u16)value); }
	void WriteI8(i8 value) { WriteU8((u8)value); }
	void WriteBool(bool value) { WriteU8( value != 0 ); }
	void WriteString(const std::string &value);
    void WriteVec3(const glm::vec3 &value);
	void WriteIVec3(const glm::ivec3 &value);
	void WriteQuat(const glm::quat &value);
	void WriteID(u32 value);

	Serializer& operator << (f32 value) {WriteF32(value); return *this;}
	Serializer& operator << (f64 value) {WriteF64(value); return *this;}
	Serializer& operator << (u64 value) {WriteU64(value); return *this;}
	Serializer& operator << (u32 value) {WriteU32(value); return *this;}
	Serializer& operator << (u16 value) {WriteU16(value); return *this;}
	Serializer& operator << (u8 value) {WriteU8(value); return *this;}
	Serializer& operator << (i64 value) {WriteU64((u64)value); return *this;}
	Serializer& operator << (i32 value) {WriteU32((u32)value); return *this;}
	Serializer& operator << (i16 value) {WriteU16((u16)value); return *this;}
	Serializer& operator << (i8 value)  {WriteU8((u8)value); return *this;}
	Serializer& operator << (bool value) {WriteBool(value); return *this;}
	Serializer& operator << (const std::string &value) {WriteString(value); return *this;}
	Serializer& operator << (const ivec2 &value) {WriteI32(value.x);WriteI32(value.y);return *this;}
	Serializer& operator << (const ivec3 &value) {WriteI32(value.x);WriteI32(value.y);WriteI32(value.z);return *this;};
	Serializer& operator << (const vec3 &value) {WriteF32(value.x);WriteF32(value.y);WriteF32(value.z);return *this;}
	Serializer& operator << (const quat &value) {WriteF32(value.x);WriteF32(value.y);WriteF32(value.z);WriteF32(value.w);return *this;}

	//=====================================================================================
	// read interface wrappers
	double ReadF64();
	float ReadF32();
	u64 ReadU64();
	u32 ReadU32();
	u16 ReadU16() { return (ReadU8()<<8) | ReadU8(); }
	i32 ReadI32() { return (int)ReadU32(); }
	i16 ReadI16() { return (i16)ReadU16(); }
	i8 ReadI8()   { return (i8)ReadU8(); }
	i64 ReadI64() { return (i64)ReadU64(); }
	bool ReadBool() { return ReadU8() != 0; }
	string ReadString();
	vec3 ReadVec3();
    vec4 ReadVec4();
	ivec2 ReadIVec2();
	ivec3 ReadIVec3();
	quat ReadQuat();
	u32 ReadID();

	Serializer& operator >> (float &value) {value = ReadF32(); return *this;}
	Serializer& operator >> (double &value) {value = ReadF64(); return *this;}
	Serializer& operator >> (u64 &value) {value = ReadU64(); return *this;}
	Serializer& operator >> (u32 &value) {value = ReadU32(); return *this;}
	Serializer& operator >> (u16 &value) {value = ReadU16(); return *this;}
	Serializer& operator >> (u8 &value) {value = ReadU8(); return *this;}
	Serializer& operator >> (i64 &value) {value = (i64)ReadU64(); return *this;}
	Serializer& operator >> (i32 &value) {value = (i32)ReadU32(); return *this;}
	Serializer& operator >> (i16 &value) {value = (i16)ReadU16(); return *this;}
	Serializer& operator >> (i8 &value)  {value = (i8)ReadU8(); return *this;}
	Serializer& operator >> (bool &value) {value = ReadBool(); return *this;}
	Serializer& operator >> (string &value) {value = ReadString(); return *this;}
	Serializer& operator >> (ivec2 &value) {value = ReadIVec2(); return *this;}
	Serializer& operator >> (ivec3 &value) {value = ReadIVec3(); return *this;}
	Serializer& operator >> (vec3 &value) {value = ReadVec3(); return *this;}
	Serializer& operator >> (quat &value) {value = ReadQuat(); return *this;}

	//== ALTERNATIVE INTERFACES ======================
	// &  interface (as used by Boost)  useful for read/write from a structure where 1 routine is used for both
	Serializer& operator & (float &value) { return (IsReadMode() ? *this >> value : *this << value); }
	Serializer& operator & (double &value) { return (IsReadMode() ? *this >> value : *this << value); }
	Serializer& operator & (u64 &value) { return (IsReadMode() ? *this >> value : *this << value); }
	Serializer& operator & (u32 &value) { return (IsReadMode() ? *this >> value : *this << value); }
	Serializer& operator & (u16 &value) { return (IsReadMode() ? *this >> value : *this << value); }
	Serializer& operator & (u8 &value) { return (IsReadMode() ? *this >> value : *this << value); }
	Serializer& operator & (i64 &value) { return (IsReadMode() ? *this >> value : *this << value); }
	Serializer& operator & (i32 &value) { return (IsReadMode() ? *this >> value : *this << value); }
	Serializer& operator & (i16 &value) { return (IsReadMode() ? *this >> value : *this << value); }
	Serializer& operator & (i8 &value) { return (IsReadMode() ? *this >> value : *this << value); }
	Serializer& operator & (bool &value) { return (IsReadMode() ? *this >> value : *this << value); }
	Serializer& operator & (string &value) { return (IsReadMode() ? *this >> value : *this << value); }
	Serializer& operator & (ivec2 &value) { return (IsReadMode() ? *this >> value : *this << value); }
	Serializer& operator & (ivec3 &value) { return (IsReadMode() ? *this >> value : *this << value); }
	Serializer& operator & (vec3 &value) { return (IsReadMode() ? *this >> value : *this << value); }
	Serializer& operator & (quat &value) { return (IsReadMode() ? *this >> value : *this << value); }

	//== BLOCK HELPERS =======================
	void WriteBlockBool(u32 id, bool value) { WriteID(id); StartBlock(); WriteBool(value); EndBlock(); }
	void WriteBlockI32(u32 id, i32 value) { WriteID(id); StartBlock(); WriteI32(value); EndBlock(); }
	void ReadBlockBool(bool &value) { StartBlock(); value = ReadBool(); EndBlock(); }
	void ReadBlockI32(i32 &value) { StartBlock(); value = ReadI32(); EndBlock(); }

	void SerializeMemory(u8* &pMem, u32 &size);

protected:
	SerializerError m_error;
};

class Serializer_BinaryWrite : public Serializer
{
public:
	Serializer_BinaryWrite();
	Serializer_BinaryWrite(u8 *mem, int size);
	void Init(u8 *mem, int size);

	// get current binary memory for writing
	u8 *DataStart() { return (u8*)m_pMem; }
	int DataSize() { return m_memUsage; }

	virtual bool IsReadMode() { return false; }
	virtual void WriteU8(u8 value);
	virtual void WriteMemory(const u8* pMem, u32 size);

	// chunks include size and allow validation and skipping of the chunk
	virtual void StartBlock();
	virtual void EndBlock();

	// not supported by this class
	virtual u8 ReadU8() { return 0; }
	virtual void ReadMemory(u8 *pMem, u32 size) {};
	virtual bool HasBufferRemaining(u32) { return true; }

	void ResetWritePtr(u32 offset) { m_memUsage = offset; }

	virtual u32 DataUsed() { return m_memUsage;  }
	virtual u32 DataTotal() { return m_memSize; }

    virtual void Restart() { Error("cannot restart a writer"); }

protected:
	char *m_pMem;
	u32 m_memSize;
	u32 m_memUsage;
	bool m_ownMemory;

	static const int MaxChunks = 16;
	int m_chunkStackSize;
	int m_chunkStack[MaxChunks];
};

class Serializer_BinaryWriteGrow : public Serializer
{
public:
	Serializer_BinaryWriteGrow();

	// get current binary memory for writing
	u8 *DataStart() { return mem.empty() ? NULL : (u8*)&(mem[0]); }
	int DataSize() { return (int)mem.size(); }

	virtual bool IsReadMode() { return false; }

	virtual void WriteU8(u8 value);
	virtual void WriteMemory(const u8* pMem, u32 size);

	// chunks include size and allow validation and skipping of the chunk
	virtual void StartBlock();
	virtual void EndBlock();

	// not supported by this class
	virtual u8 ReadU8() { return 0; }
	virtual void ReadMemory(u8 *pMem, u32 size) {};
	virtual bool HasBufferRemaining(u32) { return true; }

	virtual u32 DataUsed() { return (u32)mem.size();  }
	virtual u32 DataTotal() { return (u32)mem.capacity(); }

    virtual void Restart() { Error("cannot restart a writer"); }

protected:
	array<u8> mem;
	static const int MaxChunks = 16;
	int m_chunkStackSize;
	int m_chunkStack[MaxChunks];
};

class Serializer_BinarySize : public Serializer
{
public:
	Serializer_BinarySize();
	void Init(u8 *mem, int size);

	int DataSize() { return m_memUsage; }

	virtual bool IsReadMode() { return false; }

	virtual void WriteU8(u8 value);
	virtual void WriteMemory(const u8* pMem, u32 size);

	// chunks include size and allow validation and skipping of the chunk
	virtual void StartBlock();
	virtual void EndBlock();

	// not supported by this class
	virtual u8 ReadU8() { return 0; }
	virtual void ReadMemory(u8 *pMem, u32 size) {};
	virtual bool HasBufferRemaining(u32) { return true; }

	void ResetWritePtr(u32 offset) { m_memUsage = offset; }

	virtual u32 DataUsed() { return m_memUsage;  }
	virtual u32 DataTotal() { return m_memUsage; }

    virtual void Restart() { m_memUsage = 0; }

protected:
	u32 m_memUsage;
};

class Serializer_BinaryRead : public Serializer
{
public:
	Serializer_BinaryRead(u8 *mem, u32 size);

	virtual bool IsReadMode() { return true; }
	virtual bool HasBufferRemaining(u32 size = 1);
	virtual int GetMemoryUsage() { return 0; }

	virtual u8 ReadU8();
	virtual void ReadMemory(u8 *pMem, u32 size);

	// chunks include size and allow validation and skipping of the chunk
	virtual void StartBlock();
	virtual void EndBlock();

	// not supported by this class
	virtual void WriteU8(u8 val) {}
	virtual void WriteMemory(const u8 *pMem, u32 size) {};

	// get the remaining memory in the buffer
	u8 *DataStart() { return (u8*)(&m_pMem[m_memUsage]); }
	int DataSize() { return (int)(m_memSize-m_memUsage); }

	virtual u32 DataUsed() { return m_memUsage;  }
	virtual u32 DataTotal() { return m_memSize; }

    virtual void Restart() { m_memUsage = 0; }

protected:
	bool BufferRemaining(int size);
	char *m_pMem;
	u32 m_memSize;
	u32 m_memUsage;
	int m_bitMarker;

	static const int MaxChunks = 16;
	int m_chunkStackSize;
	int m_chunkStack[MaxChunks];
};



class Serializer_BinaryStreamWrite : public Serializer
{
public:
	// raw serializer - just write to a fixed size block and blow up if we run out of memory
	Serializer_BinaryStreamWrite(const string &filename);
	void Finalise();

	// finish up - will write the file if a name has been given
	virtual ~Serializer_BinaryStreamWrite();

	virtual bool IsReadMode() { return false; }
	virtual int GetMemoryUsage() { return m_memUsed; }

	virtual void WriteU8(u8 value);
	virtual void WriteMemory(const u8* pMem, u32 size);

	// chunks include size and allow validation and skipping of the chunk
	virtual void StartBlock();
	virtual void EndBlock();

	// not supported on output streams
	virtual u8 ReadU8() { return 0; }
	virtual void ReadMemory(u8 *pMem, u32 size) { }
	virtual bool HasBufferRemaining(u32) { return true; }

	virtual u32 DataUsed() { return m_memUsed;  }
	virtual u32 DataTotal() { return m_memUsed; }

    virtual void Restart() { Error("cannot restart a writer"); }

protected:
	void Flush();

	static const int CHUNKSIZE = 512*1024;
	FileHandle m_fileHandle;
	u8 *m_rawMem;
	u8 *m_compressedMem;
	u32 m_memUsed;
	u8 m_encryptMask;

	static const int MaxChunks = 16;
	int m_chunkStackSize;
	int m_chunkStack[MaxChunks];
};

class Serializer_BinaryStreamRead : public Serializer
{
public:
	// raw serializer - just write to a fixed size block and blow up if we run out of memory
	Serializer_BinaryStreamRead(const string &filename);

	// finish up - will write the file if a name has been given
	virtual ~Serializer_BinaryStreamRead();
	virtual void Finalise();

	virtual bool IsReadMode() { return true; }
	virtual bool HasBufferRemaining(u32 size = 1);

	virtual u8 ReadU8();
	virtual void ReadMemory(u8 *mem, u32 size);

	// chunks include size and allow validation and skipping of the chunk
	virtual void StartBlock();
	virtual void EndBlock();

	// not supported on input streams
	virtual void WriteMemory(const u8 *pMem, u32 size) {}
	virtual void WriteU8(u8 value) {}

	virtual u32 DataTotal() { return m_fileSizeTotal; }
	virtual u32 DataUsed() { return DataTotal()-m_fileSizeRemaining;  }

    virtual void Restart() { m_memUsed = 0; }

protected:
	void ReadAndDecompress();
	void AbortRead(SerializerError errCode);

	static const int CHUNKSIZE = 1024*1024;
	u8 *m_rawMem;
	u8 *m_compressedMem;
	u32 m_memSize;
	u32 m_memUsed;
	u8 m_decryptMask;

	static const int MaxChunks = 16;
	int m_chunkStackSize;
	int m_chunkStack[MaxChunks];

	// don't bother streaming from archives...
	// may support it if files ever get that big...
	u32 m_fileSizeRemaining;
	u32 m_fileSizeTotal;
	FileHandle m_fileHandle;
	bool OpenFile(const string &filename);
	u32 ReadFileBlock(u8 *buffer, u32 size);
	void CloseFile();
};
