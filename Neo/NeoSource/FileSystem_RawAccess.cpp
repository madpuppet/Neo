#include "Neo.h"
#include "FileSystem_RawAccess.h"
#include "StringUtils.h"

FileSystem_RawAccess::FileSystem_RawAccess(const std::string &name, int priority, bool writable)
	: m_name(name), m_priority(priority), m_writable(writable)
{
	Log(std::format("Mount Raw Access [{}]",name));
}

FileSystem_RawAccess::~FileSystem_RawAccess()
{
	for (auto fstream : m_activeStreams)
	{
		fclose(fstream->fh);
	}
}

bool FileSystem_RawAccess::Read(const std::string &name, MemBlock &block)
{
	FILE *fh = fopen(name.c_str(), "rb");
	if (!fh)
		return false;

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
		Error(std::format("ERROR Only read {} bytes from file: {}", sizeRead, name));
		return false;
	}

	return true;
}

bool FileSystem_RawAccess::GetAbsolutePath(const std::string &name, std::string &path)
{
	path = name;
	FILE *fh = fopen(name.c_str(), "rb");
	if (!fh)
		return false;
	fclose(fh);
	return true;
}

bool FileSystem_RawAccess::Write(const std::string &name, MemBlock &block)
{
	if (!m_writable)
		return false;

	FILE *fh = fopen(name.c_str(), "wb");
	if (fh == 0)
		return false;

	int writeSize = (int)fwrite(block.Mem(), 1, block.Size(), fh);
	fclose(fh);

	if (writeSize != block.Size())
	{
		Error(std::format("ERROR Only wrote {}/{} bytes to file: {}", writeSize, block.Size(), name));
		return false;
	}
	return true;
}

bool FileSystem_RawAccess::Exists(const std::string &name)
{
	FILE *fh = fopen(name.c_str(), "rb");
	if (fh == 0)
		return false;
	fclose(fh);
	return true;
}

bool FileSystem_RawAccess::GetSize(const std::string &name, u32 &size)
{
	FILE *fh = fopen(name.c_str(), "rb");
	if (!fh)
		return false;

	fseek(fh, 0, SEEK_END);
	size = (u32)ftell(fh);
	fclose(fh);
	return true;
}

bool FileSystem_RawAccess::GetTime(const std::string &name, u64 &timestamp)
{
#if defined(PLATFORM_Windows)
	HANDLE fh = CreateFileA(name.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
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

bool FileSystem_RawAccess::Delete(const std::string &name)
{
#if defined(PLATFORM_Windows)
	if (Exists(name))
	{
		std::string winName = StringReplace(name, '/', '\\');
		if (remove(winName.c_str()) != 0)
			return true;
	}
	return false;
#elif defined(PLATFORM_IOS) || defined(PLATFORM_OSX)
	if (Exists(name))
	{
		NSError *error;
		NSFileManager *fileMgr = [NSFileManager defaultManager];
		NSString *nsFilePath = [[NSString alloc] initWithUTF8String:name.c_str()];
		if ([fileMgr removeItemAtPath : nsFilePath error : &error] == YES)
			return true;
	}
	return false;
#else
	Error("Platform Not Supported!");
	return false;
#endif
}

bool FileSystem_RawAccess::Rename(const std::string &oldName, const std::string &newName)
{
	if (oldName == newName)
		return true;

	if (!Exists(oldName) || Exists(newName))
		return false;

#if defined(PLATFORM_Windows)

	std::string winExisting = StringReplace(oldName, '/', '\\');
	std::string winNew = StringReplace(newName, '/', '\\');
	if (MoveFileA(winExisting.c_str(), winNew.c_str()))
		return true;
	return false;

#elif defined(PLATFORM_IOS) || defined(PLATFORM_OSX)

	std::string iosExisting = oldName.Replace('\\', '/');
	std::string iosNew = newName.Replace('\\', '/');
	
	NSError *error;

	// Create file manager
	NSFileManager *fileMgr = [NSFileManager defaultManager];

	NSString *nsSrcPath = [NSString stringWithUTF8String : iosExisting.c_str()];
	NSString *nsDestPath = [NSString stringWithUTF8String : iosNew.c_str()];

	// Attempt the move
	if ([fileMgr moveItemAtPath : nsSrcPath toPath : nsDestPath error : &error] != YES)
		return false;
	return true;
		
#elif defined(PLATFORM_Switch)
    DMLOG("RENAME NOT SUPPORTED!");
    return true;
#else
#error Unsupported platform
	return false;
#endif
}

void FileSystem_RawAccess::GetListByExt(const std::string &ext, std::vector<std::string> &list)
{
	// not supported by this FS
	// - generally not an error ... other filesystems will pick up the slack
}

void FileSystem_RawAccess::GetListByExcludes(FileExcludes *excludes, std::vector<std::string> &list)
{
	// not supported by this FS
	// - generally not an error ... other filesystems will pick up the slack
}

void FileSystem_RawAccess::GetListByDelegate(const FileSystem_FilenameFilterDelegate &fileChecker, std::vector<std::string> &files)
{
	// not supported by this FS
	// - generally not an error ... other filesystems will pick up the slack
}

void FileSystem_RawAccess::GetListByFolder(const std::string &folder, std::vector<std::string> &list, GetFolderListMode folderMode)
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
					std::string path = StringAddPath(folder, buffer.name);
					if (folderMode == GetFolderListMode_FoldersOnly)
						list.push_back(path);
					else if (folderMode == GetFolderListMode_FilesOnlyRecurse)
						GetListByFolder(path, list, folderMode);
				}
			}
			else
			{
				if (folderMode == GetFolderListMode_FilesOnly || folderMode == GetFolderListMode_FilesOnlyRecurse)
				{
					std::string path = StringAddPath(folder, buffer.name);
					list.push_back(path);
				}
			}
		} while (_findnext(handle, &buffer) == 0);
	}
	_findclose(handle);
#elif defined(PLATFORM_OSX) || defined(PLATFORM_IOS)
	NSString * resourcePath = [NSString stringWithUTF8String:folder.c_str()];
	NSError * error;
	NSArray * directoryContents = [[NSFileManager defaultManager] contentsOfDirectoryAtPath:resourcePath error:&error];
	std::string path = [resourcePath UTF8String];
	for (NSString *item in directoryContents)
	{
		std::string entry = [item UTF8String];
		std::string path = folder.AddPath(entry);
		list.push_back(path);
	}
#elif defined(PLATFORM_Switch)
    DMLOG("Get List by Folder NOT SUPPORTED!");
#else
#error Platform not supported
#endif
}

bool FileSystem_RawAccess::StreamWriteBegin(FileHandle handle, const std::string &path)
{
	if (!m_writable)
		return false;

	FILE *fh = fopen(path.c_str(), "wb");
	if (fh != 0)
	{
		FileStream *fileStream = new FileStream;
		fileStream->path = path;
		fileStream->fh = fh;
		fileStream->id = handle;
		fileStream->mode = FileStream::Write;
		m_activeStreams.push_back(fileStream);
		return true;
	}
	return false;
}

bool FileSystem_RawAccess::StreamWrite(FileHandle handle, u8 *mem, u32 size)
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

bool FileSystem_RawAccess::StreamFlush(FileHandle handle)
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

bool FileSystem_RawAccess::StreamWriteEnd(FileHandle handle)
{
	for (auto it = m_activeStreams.begin(); it != m_activeStreams.end(); ++it)
	{
		FileStream *fileStream = *it;
		if (fileStream->id == handle && fileStream->mode == FileStream::Write)
		{
			fclose(fileStream->fh);
			delete fileStream;
			m_activeStreams.erase(it);
			return true;
		}
	}
	return false;
}

bool FileSystem_RawAccess::StreamReadBegin(FileHandle handle, const std::string &path)
{
	FILE *fh = fopen(path.c_str(), "rb");
	if (fh != 0)
	{
		FileStream *fileStream = new FileStream;
		fileStream->path = path;
		fileStream->fh = fh;
		fileStream->id = handle;
		fileStream->mode = FileStream::Read;
		m_activeStreams.push_back(fileStream);
		return true;
	}
	return false;
}

bool FileSystem_RawAccess::StreamRead(FileHandle handle, u8 *mem, u32 size, u32 &sizeRead)
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

bool FileSystem_RawAccess::StreamReadEnd(FileHandle handle)
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
