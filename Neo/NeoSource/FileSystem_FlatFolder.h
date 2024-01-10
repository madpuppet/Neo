#pragma once

#include "FileSystem.h"
#include "FileExcludes.h"
#include <map>

class FileSystem_FlatFolder : public FileSystem
{
public:
	FileSystem_FlatFolder(const String &name, const String &folder, int priority, bool writable, FileExcludes *excludes);
	~FileSystem_FlatFolder();

	void EnableMonitorFileChanges(bool enable);

	virtual bool CanWrite() const { return m_writable; }
	virtual int Priority() const { return m_priority; }
	virtual const String &Name() const { return m_name; }

	virtual bool GetAbsolutePath(const String &name, String &path);
	virtual bool Read(const String &name, MemBlock &block);
	virtual bool Write(const String &name, MemBlock &block);
	virtual bool Exists(const String &name);
	virtual bool GetSize(const String &name, u32 &size);
	virtual bool GetTime(const String &name, u64 &time);
	virtual bool Delete(const String &name);
	virtual bool Rename(const String &oldName, const String &newName);
	virtual void GetListByExt(const String &ext, std::vector<String> &list);
	virtual void GetListByExcludes(FileExcludes *excludes, std::vector<String> &files);
	virtual void GetListByFolder(const String &folder, std::vector<String> &files, GetFolderListMode folderMode);
	virtual void GetListByDelegate(const FileSystem_FilenameFilterDelegate &fileChecker, std::vector<String> &files);
	virtual void Rescan();

	virtual bool StreamWriteBegin(FileHandle handle, const String &name);
	virtual bool StreamWrite(FileHandle handle, u8 *mem, u32 size);
	virtual bool StreamFlush(FileHandle handle);
	virtual bool StreamWriteEnd(FileHandle handle);

	virtual bool StreamReadBegin(FileHandle handle, const String &name);
	virtual bool StreamRead(FileHandle handle, u8 *mem, u32 size, u32 &sizeRead);
	virtual bool StreamReadEnd(FileHandle handle);

	virtual bool PopChangedFile(String &name);

protected:
	void ScanFolder(const String &folder);
	bool AddEntry(const String &name, const String &path);
	bool RemoveEntry(const String &name);

	struct FileEntry
	{
		String name;
		String fullPath;
	};
	std::map<u64, FileEntry*> m_files;
	bool m_writable;
	int m_priority;
	String m_name;
	String m_rootFolder;
	FileExcludes *m_excludes;

	struct FileStream
	{
		FileHandle id;
		String name;
		String path;
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
	std::vector<String> m_changedFiles;
#endif
};
