#include "Neo.h"
#include "FileSystem_FlatFolder.h"
#include "StringUtils.h"

FileSystem_FlatFolder::FileSystem_FlatFolder(const std::string &name, const std::string &folder, int priority, bool writable, FileExcludes *excludes)
	: m_name(name), m_rootFolder(folder), m_priority(priority), m_writable(writable), m_excludes(excludes)
#if defined(PLATFORM_Windows)
    , m_readDirBuffer(0), m_rootFolderHandle(0)
#endif
{
	Log(std::format("Mount Flat Folder FS [{}] : {}", name, folder));
	Rescan();
	
#if defined(PLATFORM_Windows)
	m_monitorFileChanges = false;
#endif
}

void FileSystem_FlatFolder::EnableMonitorFileChanges(bool enable)
{
#if defined(PLATFORM_Windows)
	if (m_monitorFileChanges != enable)
	{
		if (enable)
		{
			char fullPath[256];
			GetFullPathNameA(m_rootFolder.c_str(), 256, fullPath, 0);
			m_rootFolderHandle = ::CreateFileA(
				fullPath,           // pointer to the file name
				FILE_LIST_DIRECTORY,    // access (read/write) mode
				FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,  // full sharing
				NULL, // security descriptor
				OPEN_EXISTING,         // how to create
				FILE_FLAG_BACKUP_SEMANTICS // file attributes
				| FILE_FLAG_OVERLAPPED,
				NULL);                 // file with attributes to copy
			m_readDirBuffer = new u8[2048];
			StartReadDirCall();
		}
		else
		{
			::CloseHandle(m_rootFolderHandle);
		}
		m_monitorFileChanges = enable;
	}
#endif
}

#if defined(PLATFORM_Windows)
VOID CALLBACK FileIOCompletionRoutine(DWORD dwErrorCode, DWORD dwNumberOfBytesTransfered, LPOVERLAPPED lpOverlapped)
{
	// seem to need this stub!!?
}
void FileSystem_FlatFolder::StartReadDirCall()
{
	m_readDirOverlapped = { 0 };
	m_readDirOverlapped.hEvent = (HANDLE)this;
	ReadDirectoryChangesW(m_rootFolderHandle, m_readDirBuffer, 2048, true, 0x7f, &m_readDirBytesRead, &m_readDirOverlapped, FileIOCompletionRoutine);
}
#endif

bool FileSystem_FlatFolder::PopChangedFile(std::string &name)
{
#if defined(PLATFORM_Windows)
	if (m_monitorFileChanges)
	{
		DWORD result = WaitForSingleObject(m_rootFolderHandle, 0);
		if (result == 0)
		{
			FILE_NOTIFY_INFORMATION *change = (FILE_NOTIFY_INFORMATION*)m_readDirBuffer;
			for (;;)
			{
				if (change->FileNameLength > 0)
				{
					char path[512];
					memset(path, 0, sizeof(path));
					int len = WideCharToMultiByte(CP_UTF8, 0, change->FileName, change->FileNameLength/2, path, 512, 0, 0);
					if (len > 0)
					{
						path[len] = 0;
						name = StringGetFilename(path);
						m_changedFiles.push_back(name);
					}
				}
				if (change->NextEntryOffset)
				{
					change = (FILE_NOTIFY_INFORMATION*)((u8*)change + change->NextEntryOffset);
				}
				else
					break;
			}
			StartReadDirCall();
		}

		if (!m_changedFiles.empty())
		{
			name = m_changedFiles.back();
			m_changedFiles.pop_back();
			return true;
		}
	}
#endif

	return false;
}

FileSystem_FlatFolder::~FileSystem_FlatFolder()
{
#if defined(PLATFORM_Windows)
    if (m_rootFolderHandle)
	{
		CancelIo(m_rootFolderHandle);
		::CloseHandle(m_rootFolderHandle);
	}
    delete m_readDirBuffer;
#endif
	
	for (auto &entry : m_files)
	{
		delete entry.second;
	}

	for (auto &stream : m_activeStreams)
	{
		delete stream;
	}
}

bool FileSystem_FlatFolder::Read(const std::string &name, MemBlock &block)
{
	u64 hash = StringHash64(name);
	auto entry = m_files.find(hash);
	if (entry == m_files.end())
		return false;

	FILE *fh = fopen(entry->second->fullPath.c_str(), "rb");
	if (!fh)
	{
		Error(STR("Failed to open file: %s", entry->second->fullPath.c_str()));
		return false;
	}

	//DMLOG("==> %s", entry->second->fullPath.c_str());

	fseek(fh, 0, SEEK_END);
	u32 size = (u32)ftell(fh);
	int sizeRead = 0;
	if (block.Resize(size))
	{
		fseek(fh, 0, SEEK_SET);
		sizeRead = (int)fread(block.Mem(), 1, (int)size, fh);
	}
	fclose(fh);

	if (sizeRead != size)
	{
		Error(std::format("ERROR Only read {} bytes from file: {}", sizeRead, entry->second->fullPath));
		return false;
	}

	return true;
}

bool FileSystem_FlatFolder::GetAbsolutePath(const std::string &name, std::string &path)
{
	u64 hash = StringHash64(name);
	auto entry = m_files.find(hash);
	if (entry == m_files.end())
		return false;
	path = entry->second->fullPath;
	return true;
}

bool FileSystem_FlatFolder::Write(const std::string &name, MemBlock &block)
{
	if (!m_writable)
		return false;

	std::string path = StringAddPath(m_rootFolder,name);
	FILE *fh = fopen(path.c_str(), "wb");
	if (fh == 0)
		return false;

	int writeSize = (int)fwrite(block.Mem(), 1, block.Size(), fh);
	fclose(fh);

	if (writeSize != block.Size())
	{
		Error(std::format("ERROR Only wrote {}/{} bytes to file: {}", writeSize, block.Size(), path));
		return false;
	}

	if (!Exists(name))
		AddEntry(name, path);

	return true;
}

bool FileSystem_FlatFolder::Exists(const std::string &name)
{
	u64 hash = StringHash64(name);
	auto entry = m_files.find(hash);
	return (entry != m_files.end());
}

bool FileSystem_FlatFolder::GetSize(const std::string &name, u32 &size)
{
	u64 hash = StringHash64(name);
	auto entry = m_files.find(hash);
	if (entry == m_files.end())
		return false;

	FILE *fh = fopen(entry->second->fullPath.c_str(), "rb");
	if (!fh)
	{
		Error(std::format("Failed to open file: {}", entry->second->fullPath));
		return false;
	}

	fseek(fh, 0, SEEK_END);
	size = (u32)ftell(fh);
	fclose(fh);
	return true;
}

bool FileSystem_FlatFolder::GetTime(const std::string &name, u64 &timestamp)
{
	u64 hash = StringHash64(name);
	auto entry = m_files.find(hash);
	if (entry == m_files.end())
		return false;

#if defined(PLATFORM_Windows)
	HANDLE fh = CreateFile(entry->second->fullPath.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (fh != INVALID_HANDLE_VALUE)
	{
		BY_HANDLE_FILE_INFORMATION info;
		if (GetFileInformationByHandle(fh, &info))
			timestamp = ((u64)info.ftLastWriteTime.dwHighDateTime << 32) | info.ftLastWriteTime.dwLowDateTime;
		CloseHandle(fh);
		return true;
	}
#endif
	return false;
}

bool FileSystem_FlatFolder::Delete(const std::string &name)
{
	u64 hash = StringHash64(name);
	auto entry = m_files.find(hash);
	if (entry == m_files.end())
		return false;

#if defined(PLATFORM_Windows)
	std::string winName = StringReplace(entry->second->fullPath, '/', '\\');
	if (remove(winName.c_str()) != 0)
	{
		Error(std::format("Failed to delete file: {}", winName));
	}
#elif defined(PLATFORM_IOS) || defined(PLATFORM_OSX)
	NSError *error;
	NSFileManager *fileMgr = [NSFileManager defaultManager];
	NSString *nsFilePath = [[NSString alloc] initWithUTF8String:entry->second->fullPath.c_str()];
	if ([fileMgr removeItemAtPath : nsFilePath error : &error] != YES)
		NSLog(@"Unable to delete file: %@",[error localizedDescription]);
#else
	Error("Platform Not Supported!");
#endif
	m_files.erase(entry);

	return true;
}

bool FileSystem_FlatFolder::Rename(const std::string &oldName, const std::string &newName)
{
	u64 hash = StringHash64(oldName);
	auto entry = m_files.find(hash);
	if (entry == m_files.end())
		return false;

	if (Exists(newName))
	{
		Log(std::format("Attempt to rename {} to {} but name already exists!", oldName, newName));
		return false;
	}

	std::string newPath = StringAddPath(m_rootFolder, newName);

#if defined(PLATFORM_Windows)

	if (entry->second->fullPath != newPath)
	{
		std::string winExisting = StringReplace(entry->second->fullPath, '/', '\\');
		std::string winNew = StringReplace(newPath, '/', '\\');
		if (MoveFile(winExisting.c_str(),winNew.c_str()))
		{
			RemoveEntry(StringGetFilename(winExisting));
			AddEntry(StringGetFilename(winNew), winNew);
			return true;
		}
		Log(std::format("Failed trying to rename: {} -> {}", winExisting, winNew));
	}
	return false;
	
#elif defined(PLATFORM_IOS) || defined(PLATFORM_OSX)

	if (entry->second->fullPath != newPath)
	{
		std::string iosExisting = StringReplace(entry->second->fullPath, '\\', '/');
		std::string iosNew = StringReplace(newPath, '\\', '/');
	
		NSError *error;

		// Create file manager
		NSFileManager *fileMgr = [NSFileManager defaultManager];

		NSString *nsSrcPath = [NSString stringWithUTF8String : iosExisting.c_str()];
		NSString *nsDestPath = [NSString stringWithUTF8String : iosNew.c_str()];

		// Attempt the move
		if ([fileMgr moveItemAtPath : nsSrcPath toPath : nsDestPath error : &error] != YES)
		{
			NSLog(@"Unable to move file : %@",[error localizedDescription]);
			return false;
		}
		
		RemoveEntry(iosExisting.GetFilename());
		AddEntry(iosNew.GetFilename(), iosNew);
	}
	return true;
		
#elif defined(PLATFORM_Switch)
    Log("Rename not supported");
    return true;
#else
#error Unsupported platform
#endif

	m_files.erase(entry);
	AddEntry(newName, StringReplace(newPath, '\\', '/'));
	return true;
}

void FileSystem_FlatFolder::GetListByExt(const std::string &ext, std::vector<std::string> &list)
{
	for (auto entry : m_files)
	{
		if (StringGetExtension(entry.second->name) == ext)
		{
			if (std::find(list.begin(), list.end(), entry.second->name) == list.end())
			{
				list.push_back(entry.second->name);
			}
		}
	}
}

void FileSystem_FlatFolder::GetListByDelegate(const FileSystem_FilenameFilterDelegate &fileChecker, std::vector<std::string> &list)
{
	for (auto entry : m_files)
	{
		if (fileChecker(entry.second->name))
			list.push_back(entry.second->name);
	}
}

void FileSystem_FlatFolder::GetListByExcludes(FileExcludes *excludes, std::vector<std::string> &list)
{
	for (auto entry : m_files)
	{
		std::string dir, filename, ext;
		StringSplitIntoFileParts(entry.second->fullPath, 0, &dir, &filename, &ext);
		if (!excludes->IsExcluded(dir.c_str(), filename.c_str(), ext.c_str()))
			list.push_back(entry.second->name);
	}
}

void FileSystem_FlatFolder::GetListByFolder(const std::string &folder, std::vector<std::string> &list, GetFolderListMode folderMode)
{
	std::string path = StringAddPath(m_rootFolder, folder);
	for (auto entry : m_files)
	{
		if (StringGetDirectory(entry.second->fullPath) == path)
		{
			if (std::find(list.begin(), list.end(), entry.second->name) == list.end())
			{
				list.push_back(entry.second->name);
			}
		}
	}
}

void FileSystem_FlatFolder::ScanFolder(const std::string &folder)
{
#if defined(PLATFORM_Windows)
	std::string pattern = folder + "/*.*";
	_finddata_t buffer;
	intptr_t handle;
	if ((handle = _findfirst(pattern.c_str(), &buffer)) != -1)
	{
		do
		{
			if (buffer.attrib & _A_SUBDIR)
			{
				if (buffer.name[0] != '.')
				{
					if (!m_excludes || !m_excludes->IsExcluded(buffer.name, 0, 0))
					{
						ScanFolder(StringAddPath(folder, buffer.name));
					}
				}
			}
			else
			{
				std::string path = StringAddPath(folder, buffer.name);
				std::string fs, dir, name, ext;
				StringSplitIntoFileParts(path, &fs, &dir, &name, &ext);
				if (!m_excludes || !m_excludes->IsExcluded(dir.c_str(), name.c_str(), ext.c_str()))
				{
					if (!AddEntry(buffer.name, StringReplace(path, '\\', '/')))
					{
#if defined(_DEBUG)
						auto existing = m_files.find(StringHash64(buffer.name));
						Error(std::format("Duplicate Files: %s == %s", path, existing->second->fullPath));
#endif
					}
				}
			} // buffer.attrib & _A_SUBDIR
		} while (_findnext(handle, &buffer) == 0);
	}
	_findclose(handle);
#elif defined(PLATFORM_OSX) || defined(PLATFORM_IOS)
	NSString * resourcePath = [NSString stringWithUTF8String:folder.CStr()];
	NSError * error;
	NSArray * directoryContents = [[NSFileManager defaultManager] contentsOfDirectoryAtPath:resourcePath error:&error];
	std::string path = [resourcePath UTF8String];
	for (NSString *item in directoryContents)
	{
		std::string entry = [item UTF8String];
		std::string path = folder.AddPath(entry);
		std::string fs, dir, name, ext;
		path.SplitIntoFileParts(&fs, &dir, &name, &ext);
		if (!m_excludes || !m_excludes->IsExcluded(dir.c_str(), name.c_str(), ext.c_str()))
		{
			if (!AddEntry(entry, StringReplace(path, '\\', '/')))
			{
				Error(STR("Unable to add entry: %s",path.CStr()));
			}
		}
	}
#elif defined(PLATFORM_Switch)
    Log("FlatFolder::ScanFolder not implemented");
#else
#error Platform not supported
#endif
}

void FileSystem_FlatFolder::Rescan()
{
	for (auto item : m_files)
	{
		delete item.second;
	}
	m_files.clear();

	ScanFolder(m_rootFolder);
}

bool FileSystem_FlatFolder::StreamWriteBegin(FileHandle handle, const std::string &name)
{
	std::string path = StringAddPath(m_rootFolder, name);

	u64 hash = StringHash64(name);
	auto entry = m_files.find(hash);
	if (entry != m_files.end())
	{
		path = entry->second->fullPath;
	}
	else if (!m_writable)
		return false;

	FILE *fh = fopen(path.c_str(), "wb");
	if (fh != 0)
	{
		FileStream *fileStream = new FileStream;
		fileStream->name = name;
		fileStream->path = path;
		fileStream->fh = fh;
		fileStream->id = handle;
		fileStream->mode = FileStream::Write;
		m_activeStreams.push_back(fileStream);
		return true;
	}
	return false;
}

bool FileSystem_FlatFolder::StreamWrite(FileHandle handle, u8 *mem, u32 size)
{
	for (auto fileStream : m_activeStreams)
	{
		if (fileStream->id == handle)
		{
			return fwrite(mem, 1, size, fileStream->fh) == size;
		}
	}
	return false;
}

bool FileSystem_FlatFolder::StreamFlush(FileHandle handle)
{
	for (auto fileStream : m_activeStreams)
	{
		if (fileStream->id == handle)
		{
			fflush(fileStream->fh);
			return true;
		}
	}
	return false;
}

bool FileSystem_FlatFolder::StreamWriteEnd(FileHandle handle)
{
	for (auto it = m_activeStreams.begin(); it != m_activeStreams.end(); ++it)
	{
		FileStream *fileStream = *it;
		if (fileStream->id == handle && fileStream->mode == FileStream::Write)
		{
			fclose(fileStream->fh);
			AddEntry(fileStream->name, fileStream->path);
			delete fileStream;
			m_activeStreams.erase(it);
			return true;
		}
	}
	return false;
}

bool FileSystem_FlatFolder::StreamReadBegin(FileHandle handle, const std::string &name)
{
	auto entry = m_files.find(StringHash64(name));
	if (entry == m_files.end())
		return false;

	FILE *fh = fopen(entry->second->fullPath.c_str(), "rb");
	if (fh != 0)
	{
		FileStream *fileStream = new FileStream;
		fileStream->name = name;
		fileStream->path = entry->second->fullPath;
		fileStream->fh = fh;
		fileStream->id = handle;
		fileStream->mode = FileStream::Read;
		m_activeStreams.push_back(fileStream);
		return true;
	}
	return false;
}

bool FileSystem_FlatFolder::StreamRead(FileHandle handle, u8 *mem, u32 size, u32 &sizeRead)
{
	for (auto fileStream : m_activeStreams)
	{
		if (fileStream->id == handle && fileStream->mode == FileStream::Read)
		{
			sizeRead = (u32)fread(mem, 1, size, fileStream->fh);
			return sizeRead > 0;
		}
	}
	return false;
}

bool FileSystem_FlatFolder::StreamReadEnd(FileHandle handle)
{
	for (auto it = m_activeStreams.begin(); it != m_activeStreams.end(); ++it)
	{
		FileStream *fileStream = *it;
		if (fileStream->id == handle && fileStream->mode == FileStream::Read)
		{
			fclose(fileStream->fh);
			delete fileStream;
			m_activeStreams.erase(it);
			return true;
		}
	}
	return false;
}

bool FileSystem_FlatFolder::AddEntry(const std::string &name, const std::string &path)
{
	//DMLOG("Add Entry: %s,  %s",name.CStr(),path.CStr());
	
	FileEntry *entry = new FileEntry;
	entry->name = name;
	entry->fullPath = path;
	if (!m_files.insert(std::pair<u64, FileEntry*>(StringHash64(name), entry)).second)
	{
		delete entry;
		return false;
	}
	return true;
}

bool FileSystem_FlatFolder::RemoveEntry(const std::string &name)
{
	bool found = false;
	auto it = m_files.begin();
	while (it != m_files.end())
	{
		if (it->second->name == name)
		{
			it = m_files.erase(it);
			found = true;
		}
		else
			++it;
	}
	return true;
}
