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

typedef FastDelegate::FastDelegate2<class FileSystem*, const std::string&> FileSystem_FileChangeCallback;
typedef FastDelegate::FastDelegate1<const std::string&, bool> FileSystem_FilenameFilterDelegate;

class FileSystem
{
public:
	virtual ~FileSystem() {};
	virtual bool CanWrite() const = 0;
	virtual int Priority() const = 0;
	virtual const std::string &Name() const = 0;
	virtual bool GetAbsolutePath(const std::string &name, std::string &path) = 0;
	virtual bool Read(const std::string &name, MemBlock &block) = 0;
	virtual bool Write(const std::string &name, MemBlock &block) = 0;
	virtual bool Exists(const std::string &name) = 0;
	virtual bool GetSize(const std::string &name, u32 &size) = 0;
	virtual bool GetTime(const std::string &name, u64 &time) = 0;
	virtual bool Delete(const std::string &name) = 0;
	virtual bool Rename(const std::string &oldName, const std::string &newName) = 0;
	virtual void GetListByExt(const std::string &ext, std::vector<std::string> &list) = 0;
	virtual void GetListByExcludes(FileExcludes *excludes, std::vector<std::string> &files) = 0;
	virtual void GetListByFolder(const std::string &folder, std::vector<std::string> &files, GetFolderListMode folderMode) = 0;
	virtual void GetListByDelegate(const FileSystem_FilenameFilterDelegate &fileChecker, std::vector<std::string> &files) = 0;

	virtual void Rescan() = 0;

	virtual bool StreamWriteBegin(FileHandle handle, const std::string &name) = 0;
	virtual bool StreamWrite(FileHandle handle, u8 *mem, u32 size) = 0;
	virtual bool StreamFlush(FileHandle handle) = 0;
	virtual bool StreamWriteEnd(FileHandle handle) = 0;

	virtual bool StreamReadBegin(FileHandle handle, const std::string &name) = 0;
	virtual bool StreamRead(FileHandle handle, u8 *mem, u32 size, u32 &sizeRead) = 0;
	virtual bool StreamReadEnd(FileHandle handle) = 0;

	virtual bool PopChangedFile(std::string &name) = 0;
};
