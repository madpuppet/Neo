#pragma once

#include "FileSystem.h"
#include "FileExcludes.h"
#include <map>

class FileSystem_FlatFolder : public FileSystem
{
public:
	FileSystem_FlatFolder(const std::string &name, const std::string &folder, int priority, bool writable, FileExcludes *excludes);
	~FileSystem_FlatFolder();

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
	virtual void Rescan();

	virtual bool StreamWriteBegin(FileHandle handle, const std::string &name);
	virtual bool StreamWrite(FileHandle handle, u8 *mem, u32 size);
	virtual bool StreamFlush(FileHandle handle);
	virtual bool StreamWriteEnd(FileHandle handle);

	virtual bool StreamReadBegin(FileHandle handle, const std::string &name);
	virtual bool StreamRead(FileHandle handle, u8 *mem, u32 size, u32 &sizeRead);
	virtual bool StreamReadEnd(FileHandle handle);

	virtual bool PopChangedFile(std::string &name);

protected:
	void ScanFolder(const std::string &folder);
	bool AddEntry(const std::string &name, const std::string &path);
	bool RemoveEntry(const std::string &name);

	struct FileEntry
	{
		std::string name;
		std::string fullPath;
	};
	std::map<u64, FileEntry*> m_files;
	bool m_writable;
	int m_priority;
	std::string m_name;
	std::string m_rootFolder;
	FileExcludes *m_excludes;

	struct FileStream
	{
		FileHandle id;
		std::string name;
		std::string path;
		enum Mode
		{
			Read,
			Write
		} mode;
		FILE *fh;
	};
	std::vector<FileStream*> m_activeStreams;

#if defined(PLATFORM_Windows)
	bool m_monitorFileChanges;
	HANDLE m_rootFolderHandle;
	void StartReadDirCall();
	u8 *m_readDirBuffer;
	DWORD m_readDirBytesRead;
	OVERLAPPED m_readDirOverlapped;
	std::vector<std::string> m_changedFiles;
#endif
};
