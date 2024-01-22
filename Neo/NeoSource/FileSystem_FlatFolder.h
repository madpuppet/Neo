#pragma once

#include "FileSystem.h"
#include "FileExcludes.h"
#include <map>

class FileSystem_FlatFolder : public FileSystem
{
public:
	FileSystem_FlatFolder(const string &name, const string &folder, int priority, bool writable, FileExcludes *excludes);
	~FileSystem_FlatFolder();

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
	virtual void Rescan();

	virtual bool StreamWriteBegin(FileHandle handle, const string &name);
	virtual bool StreamWrite(FileHandle handle, u8 *mem, u32 size);
	virtual bool StreamFlush(FileHandle handle);
	virtual bool StreamWriteEnd(FileHandle handle);

	virtual bool StreamReadBegin(FileHandle handle, const string &name);
	virtual bool StreamRead(FileHandle handle, u8 *mem, u32 size, u32 &sizeRead);
	virtual bool StreamReadEnd(FileHandle handle);

	virtual bool PopChangedFile(string &name);

protected:
	void ScanFolder(const string &folder);
	bool AddEntry(const string &name, const string &path);
	bool RemoveEntry(const string &name);

	struct FileEntry
	{
		string name;
		string fullPath;
	};
	std::map<u64, FileEntry*> m_files;
	bool m_writable;
	int m_priority;
	string m_name;
	string m_rootFolder;
	FileExcludes *m_excludes;

	struct FileStream
	{
		FileHandle id;
		string name;
		string path;
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
	std::vector<string> m_changedFiles;
#endif
};
