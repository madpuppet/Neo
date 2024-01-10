#include "Neo.h"
#include "FileSystem_RawAccess.h"
#include <sys/stat.h>
#include <sys/types.h>
#include <map>

FileSystem_RawAccess::FileSystem_RawAccess(const String &name, int priority, bool writable)
	: m_name(name), m_priority(priority), m_writable(writable)
{
	Log(STR("Mount Raw Access [%s]",name.CStr()));
}

FileSystem_RawAccess::~FileSystem_RawAccess()
{
	for (auto fstream : m_activeStreams)
	{
		fclose(fstream->fh);
	}
}

bool FileSystem_RawAccess::Read(const String &name, MemBlock &block)
{
	FILE *fh = fopen(name.CStr(), "rb");
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
		Error(STR("ERROR Only read %d bytes from file: %s", sizeRead, name.CStr()));
		return false;
	}

	return true;
}

bool FileSystem_RawAccess::GetAbsolutePath(const String &name, String &path)
{
	path = name;
	FILE *fh = fopen(name.CStr(), "rb");
	if (!fh)
		return false;
	fclose(fh);
	return true;
}

bool FileSystem_RawAccess::Write(const String &name, MemBlock &block)
{
	if (!m_writable)
		return false;

	FILE *fh = fopen(name.CStr(), "wb");
	if (fh == 0)
		return false;

	int writeSize = (int)fwrite(block.Mem(), 1, block.Size(), fh);
	fclose(fh);

	if (writeSize != block.Size())
	{
		Error(STR("ERROR Only wrote %d/%d bytes to file: %s", writeSize, block.Size(), name.CStr()));
		return false;
	}
	return true;
}

bool FileSystem_RawAccess::Exists(const String &name)
{
	FILE *fh = fopen(name.CStr(), "rb");
	if (fh == 0)
		return false;
	fclose(fh);
	return true;
}

bool FileSystem_RawAccess::GetSize(const String &name, u32 &size)
{
	FILE *fh = fopen(name.CStr(), "rb");
	if (!fh)
		return false;

	fseek(fh, 0, SEEK_END);
	size = (u32)ftell(fh);
	fclose(fh);
	return true;
}

bool FileSystem_RawAccess::GetTime(const String &name, u64 &timestamp)
{
#if defined(PLATFORM_Windows)
	HANDLE fh = CreateFileA(name.CStr(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
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

bool FileSystem_RawAccess::Delete(const String &name)
{
#if defined(PLATFORM_Windows)
	if (Exists(name))
	{
		String winName = name.Replace('/', '\\');
		if (remove(winName.CStr()) != 0)
			return true;
	}
	return false;
#elif defined(PLATFORM_IOS) || defined(PLATFORM_OSX)
	if (Exists(name))
	{
		NSError *error;
		NSFileManager *fileMgr = [NSFileManager defaultManager];
		NSString *nsFilePath = [[NSString alloc] initWithUTF8String:name.CStr()];
		if ([fileMgr removeItemAtPath : nsFilePath error : &error] == YES)
			return true;
	}
	return false;
#else
	Error("Platform Not Supported!");
	return false;
#endif
}

bool FileSystem_RawAccess::Rename(const String &oldName, const String &newName)
{
	if (oldName == newName)
		return true;

	if (!Exists(oldName) || Exists(newName))
		return false;

#if defined(PLATFORM_Windows)

	String winExisting = oldName.Replace('/', '\\');
	String winNew = newName.Replace('/', '\\');
	if (MoveFileA(winExisting.CStr(), winNew.CStr()))
		return true;
	return false;

#elif defined(PLATFORM_IOS) || defined(PLATFORM_OSX)

	String iosExisting = oldName.Replace('\\', '/');
	String iosNew = newName.Replace('\\', '/');
	
	NSError *error;

	// Create file manager
	NSFileManager *fileMgr = [NSFileManager defaultManager];

	NSString *nsSrcPath = [NSString stringWithUTF8String : iosExisting.CStr()];
	NSString *nsDestPath = [NSString stringWithUTF8String : iosNew.CStr()];

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

void FileSystem_RawAccess::GetListByExt(const String &ext, std::vector<String> &list)
{
	// not supported by this FS
	// - generally not an error ... other filesystems will pick up the slack
}

void FileSystem_RawAccess::GetListByExcludes(FileExcludes *excludes, std::vector<String> &list)
{
	// not supported by this FS
	// - generally not an error ... other filesystems will pick up the slack
}

void FileSystem_RawAccess::GetListByDelegate(const FileSystem_FilenameFilterDelegate &fileChecker, std::vector<String> &files)
{
	// not supported by this FS
	// - generally not an error ... other filesystems will pick up the slack
}

void FileSystem_RawAccess::GetListByFolder(const String &folder, std::vector<String> &list, GetFolderListMode folderMode)
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
					String path = folder.AddPath(buffer.name);
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
					String path = folder.AddPath(buffer.name);
					list.push_back(path);
				}
			}
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
		list.push_back(path);
	}
#elif defined(PLATFORM_Switch)
    DMLOG("Get List by Folder NOT SUPPORTED!");
#else
#error Platform not supported
#endif
}

bool FileSystem_RawAccess::StreamWriteBegin(FileHandle handle, const String &path)
{
	if (!m_writable)
		return false;

	FILE *fh = fopen(path.CStr(), "wb");
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

bool FileSystem_RawAccess::StreamReadBegin(FileHandle handle, const String &path)
{
	FILE *fh = fopen(path.CStr(), "rb");
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
