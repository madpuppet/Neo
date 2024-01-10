#pragma once

#include "FileSystem.h"
#include "FileExcludes.h"
#include "Thread.h"

class FileSystem_FlatArchive : public FileSystem
{
public:
	FileSystem_FlatArchive(const String &name, const String &path, int priority);
	~FileSystem_FlatArchive();

	// write an archive file
	// outputFile - full path name of output archive (ie ".\data.rkv")
	static void WriteArchive(const String &outputFile);

	virtual bool CanWrite() const { return false; }
	virtual int Priority() const { return m_priority; }
	virtual const String &Name() const { return m_name; }

	virtual bool Read(const String &name, MemBlock &block);
	virtual bool Exists(const String &name);
	virtual bool GetSize(const String &name, u32 &size);
	virtual bool GetTime(const String &name, u64 &time);
	virtual void GetListByExt(const String &ext, std::vector<String> &list);

	virtual bool StreamReadBegin(FileHandle handle, const String &name);
	virtual bool StreamRead(FileHandle handle, u8 *mem, u32 size, u32 &sizeRead);
	virtual bool StreamReadEnd(FileHandle handle);

	// not supported by archives...
	virtual void Rescan() {}
	virtual bool GetAbsolutePath(const String &name, String &path) { return false; }
	virtual bool Write(const String &name, MemBlock &block) { return false; }
	virtual bool Delete(const String &name) { return false; }
	virtual bool Rename(const String &oldName, const String &newName) { return false; }
	virtual bool StreamWriteBegin(FileHandle handle, const String &name) { return false; }
	virtual bool StreamWrite(FileHandle handle, u8 *mem, u32 size) { return false; }
	virtual bool StreamFlush(FileHandle handle) { return false; }
	virtual bool StreamWriteEnd(FileHandle handle) { return false; }
	virtual void GetListByFolder(const String &folder, std::vector<String> &files, GetFolderListMode folderMode);
	virtual void GetListByExcludes(FileExcludes *excludes, std::vector<String> &files);
	virtual void GetListByDelegate(const FileSystem_FilenameFilterDelegate &fileChecker, std::vector<String> &files);

	virtual bool PopChangedFile(String &name) { return false;	}

protected:
	struct TOCEntry
	{
		u32 tocEntrySize;
		u32 offset;
		u32 compressedSize;
		u32 decompressedSize;
		char name[4]; // name can be carry past end of this structure
	};
	std::map<u64,TOCEntry*> m_entries;
	String m_name;
	String m_path;
	MemBlock m_tocMem;
	u32 m_dataStart;
	FILE *m_fh;
	ThreadID m_threadID;
	int m_priority;

	struct FileStream
	{
		FileHandle id;
		TOCEntry *entry;
		MemBlock memory;
		u8 *readPtr;
		u32 remaining;
	};
	std::vector<FileStream*> m_activeStreams;
};
