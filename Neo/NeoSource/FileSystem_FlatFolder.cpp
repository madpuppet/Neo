#include "Neo.h"
#include "FileSystem_FlatFolder.h"
#include <sys/stat.h>
#include <sys/types.h>
#include <map>

FileSystem_FlatFolder::FileSystem_FlatFolder(const String &name, const String &folder, int priority, bool writable, FileExcludes *excludes)
	: m_name(name), m_rootFolder(folder), m_priority(priority), m_writable(writable), m_excludes(excludes)
#if defined(PLATFORM_Windows)
    , m_readDirBuffer(0), m_rootFolderHandle(0)
#endif
{
	Log(STR("Mount Flat Folder FS [%s] : %s",name.CStr(), folder.CStr()));
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
			GetFullPathNameA(m_rootFolder.CStr(), 256, fullPath, 0);
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

bool FileSystem_FlatFolder::PopChangedFile(String &name)
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
						name = SZ(path).GetFilename();
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

bool FileSystem_FlatFolder::Read(const String &name, MemBlock &block)
{
	u64 hash = name.Hash64();
	auto entry = m_files.find(hash);
	if (entry == m_files.end())
		return false;

	FILE *fh = fopen(entry->second->fullPath.CStr(), "rb");
	if (!fh)
	{
		Error(STR("Failed to open file: %s", entry->second->fullPath.CStr()));
		return false;
	}

	//DMLOG("==> %s", entry->second->fullPath.CStr());

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
		Error(STR("ERROR Only read %d bytes from file: %s", sizeRead, entry->second->fullPath.CStr()));
		return false;
	}

	return true;
}

bool FileSystem_FlatFolder::GetAbsolutePath(const String &name, String &path)
{
	u64 hash = name.Hash64();
	auto entry = m_files.find(hash);
	if (entry == m_files.end())
		return false;
	path = entry->second->fullPath;
	return true;
}

bool FileSystem_FlatFolder::Write(const String &name, MemBlock &block)
{
	if (!m_writable)
		return false;

	String path = m_rootFolder.AddPath(name);
	FILE *fh = fopen(path.CStr(), "wb");
	if (fh == 0)
		return false;

	int writeSize = (int)fwrite(block.Mem(), 1, block.Size(), fh);
	fclose(fh);

	if (writeSize != block.Size())
	{
		Error(STR("ERROR Only wrote %d/%d bytes to file: %s", writeSize, block.Size(), path.CStr()));
		return false;
	}

	if (!Exists(name))
		AddEntry(name, path);

	return true;
}

bool FileSystem_FlatFolder::Exists(const String &name)
{
	u64 hash = name.Hash64();
	auto entry = m_files.find(hash);
	return (entry != m_files.end());
}

bool FileSystem_FlatFolder::GetSize(const String &name, u32 &size)
{
	u64 hash = name.Hash64();
	auto entry = m_files.find(hash);
	if (entry == m_files.end())
		return false;

	FILE *fh = fopen(entry->second->fullPath.CStr(), "rb");
	if (!fh)
	{
		Error(STR("Failed to open file: %s", entry->second->fullPath.CStr()));
		return false;
	}

	fseek(fh, 0, SEEK_END);
	size = (u32)ftell(fh);
	fclose(fh);
	return true;
}

bool FileSystem_FlatFolder::GetTime(const String &name, u64 &timestamp)
{
	u64 hash = name.Hash64();
	auto entry = m_files.find(hash);
	if (entry == m_files.end())
		return false;

#if defined(PLATFORM_Windows)
	HANDLE fh = CreateFile(entry->second->fullPath.CStr(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
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

bool FileSystem_FlatFolder::Delete(const String &name)
{
	u64 hash = name.Hash64();
	auto entry = m_files.find(hash);
	if (entry == m_files.end())
		return false;

#if defined(PLATFORM_Windows)
	String winName = entry->second->fullPath.Replace('/', '\\');
	if (remove(winName.CStr()) != 0)
	{
		Error(STR("Failed to delete file: %s", winName.CStr()));
	}
#elif defined(PLATFORM_IOS) || defined(PLATFORM_OSX)
	NSError *error;
	NSFileManager *fileMgr = [NSFileManager defaultManager];
	NSString *nsFilePath = [[NSString alloc] initWithUTF8String:entry->second->fullPath.CStr()];
	if ([fileMgr removeItemAtPath : nsFilePath error : &error] != YES)
		NSLog(@"Unable to delete file: %@",[error localizedDescription]);
#else
	Error("Platform Not Supported!");
#endif
	m_files.erase(entry);

	return true;
}

bool FileSystem_FlatFolder::Rename(const String &oldName, const String &newName)
{
	u64 hash = oldName.Hash64();
	auto entry = m_files.find(hash);
	if (entry == m_files.end())
		return false;

	if (Exists(newName))
	{
		Log(STR("Attempt to rename %s to %s but name already exists!", oldName.CStr(), newName.CStr()));
		return false;
	}

	String newPath = m_rootFolder.AddPath(newName);

#if defined(PLATFORM_Windows)

	if (entry->second->fullPath != newPath)
	{
		String winExisting = entry->second->fullPath.Replace('/', '\\');
		String winNew = newPath.Replace('/', '\\');
		if (MoveFile(winExisting.CStr(),winNew.CStr()))
		{
			RemoveEntry(winExisting.GetFilename());
			AddEntry(winNew.GetFilename(), winNew);
			return true;
		}
		Log(STR("Failed trying to rename: %s -> %s", winExisting.CStr(), winNew.CStr()));
	}
	return false;
	
#elif defined(PLATFORM_IOS) || defined(PLATFORM_OSX)

	if (entry->second->fullPath != newPath)
	{
		String iosExisting = entry->second->fullPath.Replace('\\', '/');
		String iosNew = newPath.Replace('\\', '/');
	
		NSError *error;

		// Create file manager
		NSFileManager *fileMgr = [NSFileManager defaultManager];

		NSString *nsSrcPath = [NSString stringWithUTF8String : iosExisting.CStr()];
		NSString *nsDestPath = [NSString stringWithUTF8String : iosNew.CStr()];

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
    DMLOG("Rename not supported");
    return true;
#else
#error Unsupported platform
#endif

	m_files.erase(entry);
	AddEntry(newName, newPath.Replace('\\', '/'));
	return true;
}

void FileSystem_FlatFolder::GetListByExt(const String &ext, std::vector<String> &list)
{
	for (auto entry : m_files)
	{
		if (entry.second->name.GetExtension() == ext)
		{
			if (std::find(list.begin(), list.end(), entry.second->name) == list.end())
			{
				list.push_back(entry.second->name);
			}
		}
	}
}

void FileSystem_FlatFolder::GetListByDelegate(const FileSystem_FilenameFilterDelegate &fileChecker, std::vector<String> &list)
{
	for (auto entry : m_files)
	{
		if (fileChecker(entry.second->name))
			list.push_back(entry.second->name);
	}
}

void FileSystem_FlatFolder::GetListByExcludes(FileExcludes *excludes, std::vector<String> &list)
{
	for (auto entry : m_files)
	{
		String dir, filename, ext;
		entry.second->fullPath.SplitIntoFileParts(0, &dir, &filename, &ext);
		if (!excludes->IsExcluded(dir.CStr(), filename.CStr(), ext.CStr()))
			list.push_back(entry.second->name);
	}
}

void FileSystem_FlatFolder::GetListByFolder(const String &folder, std::vector<String> &list, GetFolderListMode folderMode)
{
	String path = m_rootFolder.AddPath(folder);
	for (auto entry : m_files)
	{
		if (entry.second->fullPath.GetDirectory() == path)
		{
			if (std::find(list.begin(), list.end(), entry.second->name) == list.end())
			{
				list.push_back(entry.second->name);
			}
		}
	}
}

void FileSystem_FlatFolder::ScanFolder(const String &folder)
{
#if defined(PLATFORM_Windows)
	String pattern = folder + "/*.*";
	_finddata_t buffer;
	intptr_t handle;
	if ((handle = _findfirst(pattern.CStr(), &buffer)) != -1)
	{
		do
		{
			if (buffer.attrib & _A_SUBDIR)
			{
				if (buffer.name[0] != '.')
				{
					if (!m_excludes || !m_excludes->IsExcluded(buffer.name, 0, 0))
					{
						ScanFolder(folder.AddPath(buffer.name));
					}
				}
			}
			else
			{
				String path = folder.AddPath(buffer.name);
				String fs, dir, name, ext;
				path.SplitIntoFileParts(&fs, &dir, &name, &ext);
				if (!m_excludes || !m_excludes->IsExcluded(dir.CStr(), name.CStr(), ext.CStr()))
				{
					if (!AddEntry(buffer.name, path.Replace('\\', '/')))
					{
#if defined(_DEBUG)
						auto existing = m_files.find(StringHash64(buffer.name));
						Error(STR("Duplicate Files: %s == %s", path.CStr(), existing->second->fullPath.CStr()));
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
	String path = [resourcePath UTF8String];
	for (NSString *item in directoryContents)
	{
		String entry = [item UTF8String];
		String path = folder.AddPath(entry);
		String fs, dir, name, ext;
		path.SplitIntoFileParts(&fs, &dir, &name, &ext);
		if (!m_excludes || !m_excludes->IsExcluded(dir.CStr(), name.CStr(), ext.CStr()))
		{
			if (!AddEntry(entry, path.Replace('\\', '/')))
			{
				Error(STR("Unable to add entry: %s",path.CStr()));
			}
		}
	}
#elif defined(PLATFORM_Switch)
    DMLOG("FlatFolder::ScanFolder not implemented");
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

bool FileSystem_FlatFolder::StreamWriteBegin(FileHandle handle, const String &name)
{
	String path = m_rootFolder.AddPath(name);

	u64 hash = name.Hash64();
	auto entry = m_files.find(hash);
	if (entry != m_files.end())
	{
		path = entry->second->fullPath;
	}
	else if (!m_writable)
		return false;

	FILE *fh = fopen(path.CStr(), "wb");
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

bool FileSystem_FlatFolder::StreamReadBegin(FileHandle handle, const String &name)
{
	auto entry = m_files.find(name.Hash64());
	if (entry == m_files.end())
		return false;

	FILE *fh = fopen(entry->second->fullPath.CStr(), "rb");
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

bool FileSystem_FlatFolder::AddEntry(const String &name, const String &path)
{
	//DMLOG("Add Entry: %s,  %s",name.CStr(),path.CStr());
	
	FileEntry *entry = new FileEntry;
	entry->name = name;
	entry->fullPath = path;
	if (!m_files.insert(std::pair<u64, FileEntry*>(name.Hash64(), entry)).second)
	{
		delete entry;
		return false;
	}
	return true;
}

bool FileSystem_FlatFolder::RemoveEntry(const String &name)
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
