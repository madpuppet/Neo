#pragma once

#include "Neo.h"
#include "FileExcludes.h"
#include "MemBlock.h"

enum GetFolderListMode
{
	GetFolderListMode_FilesOnly,
	GetFolderListMode_FilesOnlyRecurse,
	GetFolderListMode_FoldersOnly
};

typedef u32 FileHandle;

typedef std::function<void(class FileSystem*, const string&)> FileSystem_FileChangeCallback;
typedef std::function<bool(const string &)> FileSystem_FilenameFilterDelegate;

class FileSystem
{
public:
	virtual ~FileSystem() {};
	virtual bool CanWrite() const = 0;
	virtual int Priority() const = 0;
	virtual const string &Name() const = 0;
	virtual bool GetAbsolutePath(const string &name, string &path) = 0;
	virtual bool Read(const string &name, MemBlock &block) = 0;
	virtual bool Write(const string &name, MemBlock &block) = 0;
	virtual bool Exists(const string &name) = 0;
	virtual bool GetSize(const string &name, u32 &size) = 0;
	virtual bool GetTime(const string &name, u64 &time) = 0;
	virtual bool Delete(const string &name) = 0;
	virtual bool Rename(const string &oldName, const string &newName) = 0;
	virtual void GetListByExt(const string &ext, std::vector<string> &list) = 0;
	virtual void GetListByExcludes(FileExcludes *excludes, std::vector<string> &files) = 0;
	virtual void GetListByFolder(const string &folder, std::vector<string> &files, GetFolderListMode folderMode) = 0;
	virtual void GetListByDelegate(const FileSystem_FilenameFilterDelegate &fileChecker, std::vector<string> &files) = 0;

	virtual void Rescan() = 0;

	virtual bool StreamWriteBegin(FileHandle handle, const string &name) = 0;
	virtual bool StreamWrite(FileHandle handle, u8 *mem, u32 size) = 0;
	virtual bool StreamFlush(FileHandle handle) = 0;
	virtual bool StreamWriteEnd(FileHandle handle) = 0;

	virtual bool StreamReadBegin(FileHandle handle, const string &name) = 0;
	virtual bool StreamRead(FileHandle handle, u8 *mem, u32 size, u32 &sizeRead) = 0;
	virtual bool StreamReadEnd(FileHandle handle) = 0;

	virtual bool PopChangedFile(string &name) = 0;
};
