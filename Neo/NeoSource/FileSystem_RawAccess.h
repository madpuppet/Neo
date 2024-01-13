#pragma once

#include "FileSystem.h"
#include "FileExcludes.h"

class FileSystem_RawAccess : public FileSystem
{
public:
	FileSystem_RawAccess(const std::string &name, int priority, bool writable);
	~FileSystem_RawAccess();

	void EnableMonitorFileChanges(bool enable);

	virtual bool CanWrite() const { return m_writable; }
	virtual int Priority() const { return m_priority; }
	virtual const std::string &Name() const { return m_name; }

	virtual bool GetAbsolutePath(const std::string &name, std::string &path);
	virtual bool Read(const std::string &name, MemBlock &block);
	virtual bool Write(const std::string &name, MemBlock &block);
	virtual bool Exists(const std::string &name);
	virtual bool GetSize(const std::string &name, u32 &size);
	virtual bool GetTime(const std::string &name, u64 &time);
	virtual bool Delete(const std::string &name);
	virtual bool Rename(const std::string &oldName, const std::string &newName);
	virtual void GetListByExt(const std::string &ext, std::vector<std::string> &list);
	virtual void GetListByExcludes(FileExcludes *excludes, std::vector<std::string> &files);
	virtual void GetListByFolder(const std::string &folder, std::vector<std::string> &files, GetFolderListMode folderMode);
	virtual void GetListByDelegate(const FileSystem_FilenameFilterDelegate &fileChecker, std::vector<std::string> &files);
	virtual void Rescan() {}

	virtual bool StreamWriteBegin(FileHandle handle, const std::string &name);
	virtual bool StreamWrite(FileHandle handle, u8 *mem, u32 size);
	virtual bool StreamFlush(FileHandle handle);
	virtual bool StreamWriteEnd(FileHandle handle);

	virtual bool StreamReadBegin(FileHandle handle, const std::string &name);
	virtual bool StreamRead(FileHandle handle, u8 *mem, u32 size, u32 &sizeRead);
	virtual bool StreamReadEnd(FileHandle handle);

	virtual bool PopChangedFile(std::string &name) { return false;	}

protected:
	bool m_writable;
	int m_priority;
	std::string m_name;

	struct FileStream
	{
		FileHandle id;
		std::string path;
		enum Mode
		{
			Read,
			Write
		} mode;
		FILE *fh;
	};
	std::vector<FileStream*> m_activeStreams;
};
