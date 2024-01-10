#pragma once

#include "Singleton.h"
#include "FileSystem.h"
#include "Thread.h"

class FileManager : public Singleton<FileManager>
{
public:
	FileManager();
	~FileManager();

	// mount a filesystem
	void Mount(FileSystem *fs);
	void Unmount(FileSystem *fs);

	// get the absolute path of a file if we can...  not all filesystems support this - ie. archives will return false
	bool GetAbsolutePath(const String &name, String &path);

	// read entire file at once
	// if mem is null, it will new the memory
	// return true on success
	bool Read(const String&a_sName, MemBlock &block);

	// write the block of memory to disk at once
	// returns true on success
	bool Write(const String&a_sName, MemBlock &block);

	// does the file exist in any filesystem
	// returns TRUE if the file exists and sets fs to which filesystem contains it
	bool Exists(const String&name);

	// get the size of the file
	// returns TRUE if the file was found
	bool GetSize(const String &name, u32 &size);

	// get the file date stamp for the file
	// returns TRUE if the file was found
	bool GetTime(const String &name, u64 &time);

	// delete the file from the first filesystem we find it in that 
	bool Delete(const String &name);

	// rename a file - newName must be compatible with the files's current filesystem
	bool Rename(const String &oldName, const String &newName);

	// get list of files of a particular extension
	void GetListByExt(const String &ext, std::vector<String> &list);

	// get list of files that are not excluded by the FileExcludes object
	void GetListByExcludes(FileExcludes *excludes, std::vector<String> &files);

	// get all files from a particular folder
	void GetListByFolder(const String &folder, std::vector<String> &files, GetFolderListMode folderMode);

	// get list of files that are not excluded by the FileExcludes object
	void GetListByDelegate(const FileSystem_FilenameFilterDelegate &fileChecker, std::vector<String> &files);

	// rebuild files for any filesystem that may have changed
	void Rescan();

	// create file for writing
	bool StreamWriteBegin(FileHandle &handle, const String &name);
	bool StreamWrite(FileHandle handle, u8 *mem, u32 size);
	bool StreamFlush(FileHandle handle);
	bool StreamWriteEnd(FileHandle handle);
	inline bool StreamWrite(FileHandle handle, const String &str) { return StreamWrite(handle, (u8*)str.CStr(), str.Length()); }

	bool StreamReadBegin(FileHandle &handle, const String &name);
	bool StreamRead(FileHandle handle, u8 *mem, u32 size, u32 &sizeRead);
	bool StreamReadEnd(FileHandle handle);

	void AddFileChangeCallback(const FileSystem_FileChangeCallback &callback);
	void RemoveFileChangeCallback(const FileSystem_FileChangeCallback &callback);

	// called each frame by Application
	void Update();

protected:
	Mutex m_accessMutex;

	std::vector<FileSystem*> m_fileSystems;
	std::vector<FileSystem_FileChangeCallback> m_onFileChange;
	enum ExcludeType
	{
		ExcludeType_Folder,
		ExcludeType_Ext,
		ExcludeType_File
	};
	std::vector<u64> m_excludeFolders;
	std::vector<u64> m_excludeExtenstions;
	u32 m_nextUniqueFileHandle;
};
