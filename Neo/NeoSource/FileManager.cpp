#include "Neo.h"
#include "FileManager.h"
#include "Application.h"
#include "Thread.h"

#include <algorithm>

#if defined(PLATFORM_Windows)
#include "shlobj.h"
#elif defined(PLATFORM_OSX) || defined(PLATFORM_IOS)
#import "Foundation/Foundation.h"
#endif

#define SCOPED_MUTEX 	ScopedMutexLock critical(m_accessMutex)

FileManager::FileManager() : m_nextUniqueFileHandle(0) {}

FileManager::~FileManager()
{
	for (auto fs : m_fileSystems)
	{
		delete fs;
	}
}

bool CompareFileSystemPriority(const FileSystem *a, const FileSystem *b)
{
	return a->Priority() < b->Priority();
}

void FileManager::Mount(FileSystem *fs)
{
	SCOPED_MUTEX;
	m_fileSystems.push_back(fs);
	std::sort(m_fileSystems.begin(), m_fileSystems.end(), CompareFileSystemPriority);
}

void FileManager::Unmount(FileSystem *fs)
{
	SCOPED_MUTEX;
	auto it = std::find(m_fileSystems.begin(), m_fileSystems.end(), fs);
	delete (*it);
	if (it != m_fileSystems.end())
		m_fileSystems.erase(it);
}

// get the absolute path of a file if we can...  not all filesystems support this - ie. archives will return false
bool FileManager::GetAbsolutePath(const String &name, String &path)
{
	SCOPED_MUTEX;

	String _fs, _d, _f, _e;
	name.SplitIntoFileParts(&_fs, &_d, &_f, &_e);
	String _path = _d.AddPath(_f + _e);

	for (auto fs : m_fileSystems)
	{
		if ((_fs.IsEmpty() || _fs == fs->Name()) && fs->GetAbsolutePath(_path, path))
			return true;
	}
	return false;
}


bool FileManager::Read(const String &name, MemBlock &block)
{
	SCOPED_MUTEX;

	String _fs, _path;
	name.SplitIntoFSAndPath(_fs, _path);

	for (auto fs : m_fileSystems)
	{
        if ((_fs.IsEmpty() || _fs == fs->Name()) && fs->Read(_path, block))
        {
            return true;
        }
	}
        
    return false;
}

bool FileManager::Write(const String &name, MemBlock &block)
{
	SCOPED_MUTEX;

	String _fs, _path;
	name.SplitIntoFSAndPath(_fs, _path);

	for (auto fs : m_fileSystems)
	{
		if ((_fs.IsEmpty() || _fs == fs->Name()) && fs->Write(_path, block))
			return true;
	}
	return false;
}

bool FileManager::Exists(const String &name)
{
	SCOPED_MUTEX;

	String _fs, _path;
	name.SplitIntoFSAndPath(_fs, _path);

	for (auto fs : m_fileSystems)
	{
		if ((_fs.IsEmpty() || _fs == fs->Name()) && fs->Exists(_path))
			return true;
	}
	return false;
}

bool FileManager::GetSize(const String &name, u32 &size)
{
	SCOPED_MUTEX;
	for (auto fs : m_fileSystems)
	{
		if (fs->GetSize(name,size))
			return true;
	}
	return false;
}

bool FileManager::GetTime(const String &name, u64 &time)
{
	SCOPED_MUTEX;
	for (auto fs : m_fileSystems)
	{
		if (fs->GetTime(name, time))
			return true;
	}
	return false;
}

bool FileManager::Delete(const String &name)
{
	SCOPED_MUTEX;
	for (auto fs : m_fileSystems)
	{
		if (fs->Delete(name))
			return true;
	}
	return false;
}

bool FileManager::Rename(const String &oldName, const String &newName)
{
	SCOPED_MUTEX;
	for (auto fs : m_fileSystems)
	{
		if (fs->Rename(oldName, newName))
			return true;
	}
	return false;
}

void FileManager::GetListByExt(const String &ext, std::vector<String> &list)
{
	SCOPED_MUTEX;
	for (auto fs : m_fileSystems)
	{
		fs->GetListByExt(ext, list);
	}
}

void FileManager::GetListByDelegate(const FileSystem_FilenameFilterDelegate &fileChecker, std::vector<String> &list)
{
	SCOPED_MUTEX;
	for (auto fs : m_fileSystems)
	{
		fs->GetListByDelegate(fileChecker, list);
	}
}

void FileManager::GetListByExcludes(FileExcludes *excludes, std::vector<String> &list)
{
	SCOPED_MUTEX;
	for (auto fs : m_fileSystems)
	{
		fs->GetListByExcludes(excludes, list);
	}
}

void FileManager::GetListByFolder(const String &folder, std::vector<String> &list, GetFolderListMode folderMode)
{
	SCOPED_MUTEX;
	for (auto fs : m_fileSystems)
	{
		fs->GetListByFolder(folder, list, folderMode);
	}
}

void FileManager::Rescan()
{
	SCOPED_MUTEX;
	for (auto fs : m_fileSystems)
	{
		fs->Rescan();
	}
}

#if 0 // keep osx,ios,android code around until they are ported to system init module for creating the local file system
String FileManager::MakeLocalPath(const String &name, bool createFolder)
{

//#elif defined(PLATFORM_OSX)
	NSString *nspath = @"~ / Library / Application Support / GangsOfAsia / ";
		NSString *nsexpanded = [nspath stringByExpandingTildeInPath];
	NSString *nsout = [nsexpanded stringByAppendingPathComponent : [[NSString alloc] initWithUTF8String:name.CStr()]];
	if (createFolder)
	{
		NSError * error;
		[[NSFileManager defaultManager] createDirectoryAtPath:nsexpanded withIntermediateDirectories : TRUE attributes : nil error : &error];
	}
	String path = [nsout cStringUsingEncoding : NSASCIIStringEncoding];
	return path;
//#elif defined(PLATFORM_IOS)
	String fixedName = name.AsLowercase();

	// try documents folder...
	NSArray *paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
	NSString *documentsDirectory = [paths objectAtIndex : 0];
	return SZ([documentsDirectory cStringUsingEncoding : NSASCIIStringEncoding]) + "/" + fixedName;
//#elif defined(PLATFORM_Android)
	// Build an android specific file system path for writing profiles and settings.
	DMASSERTD(g_internalDataPath, "internalDataPath has not been specified");
	return String::CreateFormatted("%s/%s", g_internalDataPath, name.CStr());
}
#endif

bool FileManager::StreamWriteBegin(FileHandle &handle, const String &name)
{
	SCOPED_MUTEX;
	handle = ++m_nextUniqueFileHandle;

	String _fs, _path;
	name.SplitIntoFSAndPath(_fs, _path);

	// first try only overwriting files that exist...
	for (auto fs : m_fileSystems)
	{
		if ((_fs.IsEmpty() || _fs == fs->Name()) && fs->Exists(_path) && fs->StreamWriteBegin(handle, _path))
			return true;
	}

	// otherwise, just write to whatever system first says it can - usually the settings folder..
	for (auto fs : m_fileSystems)
	{
		if ((_fs.IsEmpty() || _fs == fs->Name()) && fs->StreamWriteBegin(handle, _path))
			return true;
	}
	return false;
}

bool FileManager::StreamWrite(FileHandle handle, u8 *mem, u32 size)
{
	SCOPED_MUTEX;
	for (auto fs : m_fileSystems)
	{
		if (fs->StreamWrite(handle, mem, size))
			return true;
	}
	return false;
}

bool FileManager::StreamFlush(FileHandle handle)
{
	SCOPED_MUTEX;
	for (auto fs : m_fileSystems)
	{
		if (fs->StreamFlush(handle))
			return true;
	}
	return false;
}

bool FileManager::StreamWriteEnd(FileHandle handle)
{
	SCOPED_MUTEX;
	for (auto fs : m_fileSystems)
	{
		if (fs->StreamWriteEnd(handle))
			return true;
	}
	return false;
}

bool FileManager::StreamReadBegin(FileHandle &handle, const String &name)
{
	Log(STR("Stream Read %s", name.CStr()));

	SCOPED_MUTEX;
	handle = ++m_nextUniqueFileHandle;

	String _fs, _path;
	name.SplitIntoFSAndPath(_fs, _path);

	for (auto fs : m_fileSystems)
	{
		if ((_fs.IsEmpty() || _fs == fs->Name()) && fs->Exists(_path) && fs->StreamReadBegin(handle, _path))
			return true;
	}
	return false;
}

bool FileManager::StreamRead(FileHandle handle, u8 *mem, u32 size, u32 &sizeRead)
{
	SCOPED_MUTEX;
	for (auto fs : m_fileSystems)
	{
		if (fs->StreamRead(handle, mem, size, sizeRead))
			return true;
	}
	return false;
}

bool FileManager::StreamReadEnd(FileHandle handle)
{
	SCOPED_MUTEX;
	for (auto fs : m_fileSystems)
	{
		if (fs->StreamReadEnd(handle))
			return true;
	}
	return false;
}

void FileManager::AddFileChangeCallback(const FileSystem_FileChangeCallback &callback)
{
	SCOPED_MUTEX;
	m_onFileChange.push_back(callback);
}

void FileManager::RemoveFileChangeCallback(const FileSystem_FileChangeCallback &callback)
{
	SCOPED_MUTEX;
	for (std::vector<FileSystem_FileChangeCallback>::const_iterator it = m_onFileChange.begin(); it != m_onFileChange.end(); ++it)
	{
		if (*it == callback)
		{
			m_onFileChange.erase(it);
			return;
		}
	}
}

void FileManager::Update()
{
#if defined(PLATFORM_Windows)
	SCOPED_MUTEX;

	struct Change
	{
		FileSystem *fs;
		String filename;
	};
	std::vector<Change> *changes = 0;

	// check for file changes
	for (auto fs : m_fileSystems)
	{
		String filename;
		if (fs->PopChangedFile(filename))
		{
			if (!changes)
				changes = new std::vector<Change>();

			Change change;
			change.fs = fs;
			change.filename = filename;
			changes->push_back(change);
		}
	}

	// process all the changes - outside of a mutex since they may callback to the filesystem
	if (changes)
	{
		m_accessMutex.Release();
		for (auto &change : (*changes))
		{
			for (auto it = m_onFileChange.begin(); it != m_onFileChange.end(); ++it)
			{
				(*it)(change.fs, change.filename);
			}
		}
		m_accessMutex.Lock();
		delete changes;
	}
#endif
}
