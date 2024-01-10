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

typedef std::function<void(class FileSystem*, const String&)> FileSystem_FileChangeCallback;
typedef std::function<bool(const String &)> FileSystem_FilenameFilterDelegate;

class FileSystem
{
public:
	virtual ~FileSystem() {};
	virtual bool CanWrite() const = 0;
	virtual int Priority() const = 0;
	virtual const String &Name() const = 0;
	virtual bool GetAbsolutePath(const String &name, String &path) = 0;
	virtual bool Read(const String &name, MemBlock &block) = 0;
	virtual bool Write(const String &name, MemBlock &block) = 0;
	virtual bool Exists(const String &name) = 0;
	virtual bool GetSize(const String &name, u32 &size) = 0;
	virtual bool GetTime(const String &name, u64 &time) = 0;
	virtual bool Delete(const String &name) = 0;
	virtual bool Rename(const String &oldName, const String &newName) = 0;
	virtual void GetListByExt(const String &ext, std::vector<String> &list) = 0;
	virtual void GetListByExcludes(FileExcludes *excludes, std::vector<String> &files) = 0;
	virtual void GetListByFolder(const String &folder, std::vector<String> &files, GetFolderListMode folderMode) = 0;
	virtual void GetListByDelegate(const FileSystem_FilenameFilterDelegate &fileChecker, std::vector<String> &files) = 0;

	virtual void Rescan() = 0;

	virtual bool StreamWriteBegin(FileHandle handle, const String &name) = 0;
	virtual bool StreamWrite(FileHandle handle, u8 *mem, u32 size) = 0;
	virtual bool StreamFlush(FileHandle handle) = 0;
	virtual bool StreamWriteEnd(FileHandle handle) = 0;

	virtual bool StreamReadBegin(FileHandle handle, const String &name) = 0;
	virtual bool StreamRead(FileHandle handle, u8 *mem, u32 size, u32 &sizeRead) = 0;
	virtual bool StreamReadEnd(FileHandle handle) = 0;

	virtual bool PopChangedFile(String &name) = 0;
};
