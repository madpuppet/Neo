#pragma once

#include "Module.h"
#include "FileSystem.h"
#include "Thread.h"

class FileManager : public Module<FileManager>
{
public:
	FileManager();
	~FileManager();

	// mount a filesystem
	void Mount(FileSystem *fs);
	void Unmount(FileSystem *fs);

	// get the absolute path of a file if we can...  not all filesystems support this - ie. archives will return false
	bool GetAbsolutePath(const string &name, string &path);

	// read entire file at once
	// if mem is null, it will new the memory
	// return true on success
	bool Read(const string&a_sName, MemBlock &block);

	// write the block of memory to disk at once
	// returns true on success
	bool Write(const string&a_sName, MemBlock &block);

	// does the file exist in any filesystem
	// returns TRUE if the file exists and sets fs to which filesystem contains it
	bool Exists(const string&name);

	// get the size of the file
	// returns TRUE if the file was found
	bool GetSize(const string &name, u32 &size);

	// get the file date stamp for the file
	// returns TRUE if the file was found
	bool GetTime(const string &name, u64 &time);

	// delete the file from the first filesystem we find it in that 
	bool Delete(const string &name);

	// rename a file - newName must be compatible with the files's current filesystem
	bool Rename(const string &oldName, const string &newName);

	// get list of files of a particular extension
	void GetListByExt(const string &ext, std::vector<string> &list);

	// get list of files that are not excluded by the FileExcludes object
	void GetListByExcludes(FileExcludes *excludes, std::vector<string> &files);

	// get all files from a particular folder
	void GetListByFolder(const string &folder, std::vector<string> &files, GetFolderListMode folderMode);

	// get list of files that are not excluded by the FileExcludes object
	void GetListByDelegate(const FileSystem_FilenameFilterDelegate &fileChecker, std::vector<string> &files);

	// rebuild files for any filesystem that may have changed
	void Rescan();

	// create file for writing
	bool StreamWriteBegin(FileHandle &handle, const string &name);
	bool StreamWrite(FileHandle handle, u8 *mem, u32 size);
	bool StreamFlush(FileHandle handle);
	bool StreamWriteEnd(FileHandle handle);
	inline bool StreamWrite(FileHandle handle, const string &str) { return StreamWrite(handle, (u8*)str.c_str(), (u32)str.size()); }

	bool StreamReadBegin(FileHandle &handle, const string &name);
	bool StreamRead(FileHandle handle, u8 *mem, u32 size, u32 &sizeRead);
	bool StreamReadEnd(FileHandle handle);

	CallbackHandle AddFileChangeCallback(const FileSystem_FileChangeCallback &callback);
	void RemoveFileChangeCallback(CallbackHandle handle);

	// called each frame by Application
	void Update();

protected:
	Mutex m_accessMutex;

	vector<FileSystem*> m_fileSystems;
	vector<std::pair<CallbackHandle, FileSystem_FileChangeCallback>> m_onFileChange;
	enum ExcludeType
	{
		ExcludeType_Folder,
		ExcludeType_Ext,
		ExcludeType_File
	};
	vector<u64> m_excludeFolders;
	vector<u64> m_excludeExtenstions;
	u32 m_nextUniqueFileHandle;
};
