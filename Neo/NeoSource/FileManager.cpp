#include "Neo.h"
#include "FileManager.h"
#include "FileSystem_FlatFolder.h"
#include "Application.h"
#include "Thread.h"
#include "StringUtils.h"

#define SCOPED_MUTEX 	ScopedMutexLock critical(m_accessMutex)

static FileExcludes* s_excludes;
FileManager::FileManager() : m_nextUniqueFileHandle(0) 
{
	// load excludes file for filtering flatFolder data - this is a synchronous load using std c++ file functions in the current working directory
	s_excludes = new FileExcludes("all.exclude");

	// mount local filesystem
#if defined(PLATFORM_Windows)
	char buffer[256];
	if (::SHGetFolderPathA(NULL, CSIDL_APPDATA, NULL, 0, buffer) == 0)
	{
		std::string path = buffer;
		path = StringAddPath(path, GAME_NAME);
		::SHCreateDirectoryExA(NULL, path.c_str(), 0);
		FileSystem_FlatFolder* fflocal = new FileSystem_FlatFolder("local", path, 2, true, s_excludes);
		fflocal->EnableMonitorFileChanges(true);
		Mount(fflocal);
	}
#else
	Error("TODO: Mount local folder for this platform!");
#endif

}

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
bool FileManager::GetAbsolutePath(const std::string &name, std::string &path)
{
	SCOPED_MUTEX;

	std::string _fs, _d, _f, _e;
	StringSplitIntoFileParts(name, &_fs, &_d, &_f, &_e);
	std::string _path = StringAddPath(_d, _f + _e);

	for (auto fs : m_fileSystems)
	{
		if ((!_fs.empty() || _fs == fs->Name()) && fs->GetAbsolutePath(_path, path))
			return true;
	}
	return false;
}


bool FileManager::Read(const std::string &name, MemBlock &block)
{
	SCOPED_MUTEX;

	std::string _fs, _path;
	StringSplitIntoFSAndPath(name, _fs, _path);

	for (auto fs : m_fileSystems)
	{
        if ((_fs.empty() || _fs == fs->Name()) && fs->Read(_path, block))
        {
            return true;
        }
	}
        
    return false;
}

bool FileManager::Write(const std::string &name, MemBlock &block)
{
	SCOPED_MUTEX;

	std::string _fs, _path;
	StringSplitIntoFSAndPath(name, _fs, _path);

	for (auto fs : m_fileSystems)
	{
		if ((_fs.empty() || _fs == fs->Name()) && fs->Write(_path, block))
			return true;
	}
	return false;
}

bool FileManager::Exists(const std::string &name)
{
	SCOPED_MUTEX;

	std::string _fs, _path;
	StringSplitIntoFSAndPath(name, _fs, _path);

	for (auto fs : m_fileSystems)
	{
		if ((_fs.empty() || _fs == fs->Name()) && fs->Exists(_path))
			return true;
	}
	return false;
}

bool FileManager::GetSize(const std::string &name, u32 &size)
{
	SCOPED_MUTEX;
	for (auto fs : m_fileSystems)
	{
		if (fs->GetSize(name,size))
			return true;
	}
	return false;
}

bool FileManager::GetTime(const std::string &name, u64 &time)
{
	SCOPED_MUTEX;
	for (auto fs : m_fileSystems)
	{
		if (fs->GetTime(name, time))
			return true;
	}
	return false;
}

bool FileManager::Delete(const std::string &name)
{
	SCOPED_MUTEX;
	for (auto fs : m_fileSystems)
	{
		if (fs->Delete(name))
			return true;
	}
	return false;
}

bool FileManager::Rename(const std::string &oldName, const std::string &newName)
{
	SCOPED_MUTEX;
	for (auto fs : m_fileSystems)
	{
		if (fs->Rename(oldName, newName))
			return true;
	}
	return false;
}

void FileManager::GetListByExt(const std::string &ext, std::vector<std::string> &list)
{
	SCOPED_MUTEX;
	for (auto fs : m_fileSystems)
	{
		fs->GetListByExt(ext, list);
	}
}

void FileManager::GetListByDelegate(const FileSystem_FilenameFilterDelegate &fileChecker, std::vector<std::string> &list)
{
	SCOPED_MUTEX;
	for (auto fs : m_fileSystems)
	{
		fs->GetListByDelegate(fileChecker, list);
	}
}

void FileManager::GetListByExcludes(FileExcludes *excludes, std::vector<std::string> &list)
{
	SCOPED_MUTEX;
	for (auto fs : m_fileSystems)
	{
		fs->GetListByExcludes(excludes, list);
	}
}

void FileManager::GetListByFolder(const std::string &folder, std::vector<std::string> &list, GetFolderListMode folderMode)
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
std::string FileManager::MakeLocalPath(const std::string &name, bool createFolder)
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
	std::string path = [nsout cStringUsingEncoding : NSASCIIStringEncoding];
	return path;
//#elif defined(PLATFORM_IOS)
	std::string fixedName = name.AsLowercase();

	// try documents folder...
	NSArray *paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
	NSString *documentsDirectory = [paths objectAtIndex : 0];
	return SZ([documentsDirectory cStringUsingEncoding : NSASCIIStringEncoding]) + "/" + fixedName;
//#elif defined(PLATFORM_Android)
	// Build an android specific file system path for writing profiles and settings.
	DMASSERTD(g_internalDataPath, "internalDataPath has not been specified");
	return std::string::CreateFormatted("%s/%s", g_internalDataPath, name.CStr());
}
#endif

bool FileManager::StreamWriteBegin(FileHandle &handle, const std::string &name)
{
	SCOPED_MUTEX;
	handle = ++m_nextUniqueFileHandle;

	std::string _fs, _path;
	StringSplitIntoFSAndPath(name, _fs, _path);

	// first try only overwriting files that exist...
	for (auto fs : m_fileSystems)
	{
		if ((_fs.empty() || _fs == fs->Name()) && fs->Exists(_path) && fs->StreamWriteBegin(handle, _path))
			return true;
	}

	// otherwise, just write to whatever system first says it can - usually the settings folder..
	for (auto fs : m_fileSystems)
	{
		if ((_fs.empty() || _fs == fs->Name()) && fs->StreamWriteBegin(handle, _path))
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

bool FileManager::StreamReadBegin(FileHandle &handle, const std::string &name)
{
	Log(std::format("Stream Read {}", name));

	SCOPED_MUTEX;
	handle = ++m_nextUniqueFileHandle;

	std::string _fs, _path;
	StringSplitIntoFSAndPath(name, _fs, _path);

	for (auto fs : m_fileSystems)
	{
		if ((_fs.empty() || _fs == fs->Name()) && fs->Exists(_path) && fs->StreamReadBegin(handle, _path))
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
	auto it = std::find(m_onFileChange.begin(), m_onFileChange.end(), callback);
	Assert(it != m_onFileChange.end(), "Attempt to delete a callback that doesn't exist!");
	m_onFileChange.erase(it);
}

void FileManager::Update()
{
#if defined(PLATFORM_Windows)
	SCOPED_MUTEX;

	struct Change
	{
		FileSystem *fs;
		std::string filename;
	};
	std::vector<Change> *changes = 0;

	// check for file changes
	for (auto fs : m_fileSystems)
	{
		std::string filename;
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
