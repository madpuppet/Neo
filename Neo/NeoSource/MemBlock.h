#pragma once

class MemBlock
{
public:
	static MemBlock* CloneMem(u8* mem, int size)
	{
		u8* cloned = new u8[size];
		memcpy(cloned, mem, size);
		return new MemBlock(cloned, size, false);
	}

	MemBlock() : m_mem(0), m_size(0), m_external(false) {}
	MemBlock(u32 size) : m_mem(0), m_size(0), m_external(false) { AllocMem(size); }
	MemBlock(u8 *mem, u32 size, bool external) : m_mem(mem), m_size(size), m_external(external) {}
	~MemBlock() { FreeMem(); }

	// allocate a new block - does not retain the old memory
	bool Resize(u32 size);

	// allocates a larger block - keeping the contains in tact
	bool Expand(u32 size);

	// set this to be an external block of memory - will not attempt to free it in destructor and will not be able to resize or expand it larger
	void SetExternal(u8 *mem, u32 size);

	// decompress to another block
	void DecompressTo(MemBlock &dest);

	// copy to another block
	void CopyTo(MemBlock &dest);

	// compress a block if possible
	void CompressTo(MemBlock &dest);

	u8 *Mem() { return m_mem; }
	u32 Size() { return m_size; }
	u8 *MemEnd() { return m_mem + m_size; }

protected:
	bool FreeMem();
	void AllocMem(u32 size);
	u8 *m_mem;
	u32 m_size;
	bool m_external;	// this is external mem so we can't free it
};
