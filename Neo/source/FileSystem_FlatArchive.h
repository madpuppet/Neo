#pragma once

#include "FileSystem.h"
#include "FileExcludes.h"
#include "Thread.h"

class FileSystem_FlatArchive : public FileSystem
{
public:
	FileSystem_FlatArchive(const string &name, const string &path, int priority);
	~FileSystem_FlatArchive();

	// write an archive file
	// outputFile - full path name of output archive (ie ".\data.rkv")
	static void WriteArchive(const string &outputFile);

	virtual bool CanWrite() const { return false; }
	virtual int Priority() const { return m_priority; }
	virtual const string &Name() const { return m_name; }

	virtual bool Read(const string &name, MemBlock &block);
	virtual bool Exists(const string &name);
	virtual bool GetSize(const string &name, u32 &size);
	virtual bool GetTime(const string &name, u64 &time);
	virtual void GetListByExt(const string &ext, std::vector<string> &list);

	virtual bool StreamReadBegin(FileHandle handle, const string &name);
	virtual bool StreamRead(FileHandle handle, u8 *mem, u32 size, u32 &sizeRead);
	virtual bool StreamReadEnd(FileHandle handle);

	// not supported by archives...
	virtual void Rescan() {}
	virtual bool GetAbsolutePath(const string &name, string &path) { return false; }
	virtual bool Write(const string &name, MemBlock &block) { return false; }
	virtual bool Delete(const string &name) { return false; }
	virtual bool Rename(const string &oldName, const string &newName) { return false; }
	virtual bool StreamWriteBegin(FileHandle handle, const string &name) override { return false; }
	virtual bool StreamWrite(FileHandle handle, u8 *mem, u32 size) { return false; }
	virtual bool StreamFlush(FileHandle handle) { return false; }
	virtual bool StreamWriteEnd(FileHandle handle) { return false; }
	virtual void GetListByFolder(const string &folder, std::vector<string> &files, GetFolderListMode folderMode);
	virtual void GetListByExcludes(FileExcludes *excludes, std::vector<string> &files);
	virtual void GetListByDelegate(const FileSystem_FilenameFilterDelegate &fileChecker, std::vector<string> &files);

	virtual bool PopChangedFile(string &name) { return false;	}

protected:
	struct TOCEntry
	{
		u64 offset;
		u32 tocEntrySize;
		u32 compressedSize;
		u32 decompressedSize;
		char name[4]; // name can be carry past end of this structure
	};
	std::map<u64,TOCEntry*> m_entries;
	string m_name;
	string m_path;
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
