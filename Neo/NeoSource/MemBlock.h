#pragma once

class MemBlock
{
public:
	static MemBlock CloneMem(u8* mem, size_t size)
	{
		u8* cloned = new u8[size];
		memcpy(cloned, mem, size);
		return MemBlock(cloned, size, false);
	}

	MemBlock() : m_mem(0), m_size(0), m_external(false) {}
	MemBlock(size_t size) : m_mem(0), m_size(0), m_external(false) { AllocMem(size); }
	MemBlock(u8 *mem, size_t size, bool external) : m_mem(mem), m_size(size), m_external(external) {}

	// copy constructor
	MemBlock(const MemBlock& o)
	{
		m_size = o.m_size;
		if (o.m_external)
		{
			m_mem = o.m_mem;
			m_external = true;
		}
		else
		{
			m_mem = new u8[o.m_size];
			memcpy(m_mem, o.m_mem, m_size);
		}
	}

	// move constructor
	MemBlock(MemBlock&& o) noexcept
	{
		m_mem = o.m_mem;
		m_size = o.m_size;
		m_external = o.m_external;
		
		if (!o.m_external)
		{
			o.m_mem = nullptr;
			o.m_size = 0;
		}
	}

	~MemBlock() { FreeMem(); }

	// allocate a new block - does not retain the old memory
	bool Resize(size_t size);

	// allocates a larger block - keeping the contains in tact
	bool Expand(size_t size);

	// set this to be an external block of memory - will not attempt to free it in destructor and will not be able to resize or expand it larger
	void SetExternal(u8 *mem, size_t size);

	// decompress to another block
	void DecompressTo(MemBlock &dest);

	// copy to another block
	void CopyTo(MemBlock &dest);

	// compress a block if possible
	void CompressTo(MemBlock &dest);

	u8 *Mem() { return m_mem; }
	u8 *MemEnd() { return m_mem + m_size; }
	const u8* Mem() const { return m_mem; }
	const u8* MemEnd() const { return m_mem + m_size; }
	size_t Size() const { return m_size; }

protected:
	bool FreeMem();
	void AllocMem(size_t size);
	u8 *m_mem = nullptr;
	size_t m_size = 0;
	bool m_external = false;	// this is external mem so we can't free it
};
