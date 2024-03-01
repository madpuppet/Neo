#include "Neo.h"
#include "MemBlock.h"
#include "zlib.h"

bool MemBlock::Resize(size_t size)
{
	if (m_external)
		return (m_size >= size);
	FreeMem();
	AllocMem(size);
	return true;
}
bool MemBlock::Expand(size_t size)
{
	if (m_external)
		return (m_size >= size);

	if (m_size >= size)
		return true;

	u8 *newMem = new u8[size];
	memcpy(newMem, m_mem, m_size);
	delete m_mem;
	m_mem = newMem;
	m_size = size;
	return true;
}
void MemBlock::SetExternal(u8 *mem, size_t size)
{
	FreeMem();
	m_mem = mem;
	m_size = size;
}

bool MemBlock::FreeMem()
{
	if (!m_external)
	{
		delete m_mem;
		m_mem = 0;
		m_size = 0;
		return true;
	}
	else
	{
		m_mem = 0;
		m_size = 0;
		return false;
	}
}
void MemBlock::AllocMem(size_t size)
{
	if (size > 0)
	{
		m_mem = new u8[size];
		m_size = size;
	}
}
void MemBlock::DecompressTo(MemBlock &dest)
{
	// compressed memory has the decompress size in the first 4 bytes
	int decompressSize = *((u32*)m_mem);
	dest.Resize(decompressSize);

	if (decompressSize == m_size-4)
		memcpy(dest.Mem(), m_mem+4, m_size-4);
	else
	{
		// decompress
		z_stream strm;
		strm.zalloc = Z_NULL;
		strm.zfree = Z_NULL;
		strm.opaque = Z_NULL;
		strm.avail_in = 0;
		strm.next_in = Z_NULL;
		inflateInit(&strm);
		strm.avail_in = (u32)(m_size - 4);
		strm.next_in = (Bytef*)(m_mem + 4);
		strm.avail_out = decompressSize;
		strm.next_out = dest.Mem();
		inflate(&strm, Z_NO_FLUSH);
		Assert(strm.avail_out == 0, "Did not decompress to full buffer!");
		inflateEnd(&strm);
	}
}

// copy to another block
void MemBlock::CopyTo(MemBlock &dest)
{
	dest.Resize(m_size);
	memcpy(dest.Mem(), m_mem, m_size);
}

// compress a block
void MemBlock::CompressTo(MemBlock &dest)
{
	if (m_size == 0)
	{
		dest.Resize(0);
		return;
	}

	u32 outBufferSize = (u32)m_size + 256;
	u8 *compressBuffer = new u8[outBufferSize];

	// compress
	z_stream strm;
	strm.zalloc = Z_NULL;
	strm.zfree = Z_NULL;
	strm.opaque = Z_NULL;
	deflateInit(&strm, Z_DEFAULT_COMPRESSION);
	strm.avail_in = (u32)m_size;
	strm.next_in = (Bytef*)m_mem;
	strm.avail_out = outBufferSize;
	strm.next_out = compressBuffer;
	deflate(&strm, Z_FINISH);
	u32 compressedSize = outBufferSize - strm.avail_out;
	if (compressedSize < (u32)(m_size-4))
	{
		dest.Resize(compressedSize + 4);
		*((u32*)dest.Mem()) = (u32)m_size;
		memcpy(dest.Mem() + 4, compressBuffer, compressedSize);
	}
	else
	{
		dest.Resize((u32)m_size + 4);
		*((u32*)dest.Mem()) = (u32)m_size;
		memcpy(dest.Mem() + 4, m_mem, (u32)m_size);
	}

	deflateEnd(&strm);
	delete [] compressBuffer;
	//DMLOG("Compress chunk %d -> %d (%d%%)",m_size,compressedSize+4,(compressedSize+4)*100/rawSize);
}
