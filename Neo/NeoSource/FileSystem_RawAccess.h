#pragma once

#include "FileSystem.h"
#include "FileExcludes.h"

class FileSystem_RawAccess : public FileSystem
{
public:
	FileSystem_RawAccess(const string &name, int priority, bool writable);
	~FileSystem_RawAccess();

	void EnableMonitorFileChanges(bool enable);

	virtual bool CanWrite() const { return m_writable; }
	virtual int Priority() const { return m_priority; }
	virtual const string &Name() const { return m_name; }

	virtual bool GetAbsolutePath(const string &name, string &path);
	virtual bool Read(const string &name, MemBlock &block);
	virtual bool Write(const string &name, MemBlock &block);
	virtual bool Exists(const string &name);
	virtual bool GetSize(const string &name, u32 &size);
	virtual bool GetTime(const string &name, u64 &time);
	virtual bool Delete(const string &name);
	virtual bool Rename(const string &oldName, const string &newName);
	virtual void GetListByExt(const string &ext, std::vector<string> &list);
	virtual void GetListByExcludes(FileExcludes *excludes, std::vector<string> &files);
	virtual void GetListByFolder(const string &folder, std::vector<string> &files, GetFolderListMode folderMode);
	virtual void GetListByDelegate(const FileSystem_FilenameFilterDelegate &fileChecker, std::vector<string> &files);
	virtual void Rescan() {}

	virtual bool StreamWriteBegin(FileHandle handle, const string &name);
	virtual bool StreamWrite(FileHandle handle, u8 *mem, u32 size);
	virtual bool StreamFlush(FileHandle handle);
	virtual bool StreamWriteEnd(FileHandle handle);

	virtual bool StreamReadBegin(FileHandle handle, const string &name);
	virtual bool StreamRead(FileHandle handle, u8 *mem, u32 size, u32 &sizeRead);
	virtual bool StreamReadEnd(FileHandle handle);

	virtual bool PopChangedFile(string &name) { return false;	}

protected:
	bool m_writable;
	int m_priority;
	string m_name;

	struct FileStream
	{
		FileHandle id;
		string path;
		enum Mode
		{
			Read,
			Write
		} mode;
		FILE *fh;
	};
	std::vector<FileStream*> m_activeStreams;
};
