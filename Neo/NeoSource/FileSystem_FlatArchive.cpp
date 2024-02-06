#include "Neo.h"
#include "FileSystem_FlatArchive.h"
#include "FileManager.h"
#include "Thread.h"
#include "MathUtils.h"
#include "StringUtils.h"

FileSystem_FlatArchive::FileSystem_FlatArchive(const string &name, const string &path, int priority)
	: m_name(name), m_path(path), m_priority(priority), m_dataStart(0)
{
	LOG(File, std::format("MOUNT ARCHIVE: {}", name));

	m_threadID = Thread::CurrentThreadID();
	m_fh = fopen(path.c_str(), "rb");
	if (m_fh != 0)
	{
		u32 tocSize;
		fread(&tocSize, 4, 1, m_fh);
		if (tocSize < 10000000)
		{
			m_tocMem.Resize(tocSize);
			m_dataStart = tocSize+4;
			if (fread(m_tocMem.Mem(), 1, tocSize, m_fh) == tocSize)
			{
				u8 *tocPtr = m_tocMem.Mem();
				u8 *tocEnd = m_tocMem.MemEnd();
				while (tocPtr < tocEnd)
				{
					TOCEntry *entry = (TOCEntry*)tocPtr;
					u64 hash = StringHash64(entry->name);
					m_entries.insert(std::pair<u64, TOCEntry*>(hash, entry));
					tocPtr += entry->tocEntrySize;

					LOG(File, std::format("TOC: <{}->{}> {}", entry->compressedSize, entry->decompressedSize, entry->name));
				}
			}
			else
			{
				Error(std::format("Error reading archive: {}", path));
			}
		}
		else
		{
			Error(std::format("Possibly bad archive! Toc size is {}", tocSize));
		}
	}
}

FileSystem_FlatArchive::~FileSystem_FlatArchive()
{
	if (m_fh)
		fclose(m_fh);
}

bool FileSystem_FlatArchive::Read(const string &name, MemBlock &block)
{
	// check if entry is in the TOC
	u64 hash = StringHash64(name);
	auto it = m_entries.find(hash);
	if (it == m_entries.end())
		return false;

	// if on the main thread, we can just use our open file pointer
	ThreadID currentThread = Thread::CurrentThreadID();
	auto entry = it->second;
	if (!block.Resize(entry->decompressedSize))
	{
		Error(std::format("Unable to fit file {} in supplied memory!", name));
		return false;
	}

	MemBlock temp(entry->compressedSize);
	if (currentThread == m_threadID)
	{
		// main thread can just seek and read for speed
		_fseeki64(m_fh, m_dataStart + entry->offset, SEEK_SET);
		fread(temp.Mem(), temp.Size(), 1, m_fh);
	}
	else
	{
		// different thread, so we need to open a new file instance to be safe...
		FILE *fh = fopen(m_path.c_str(), "rb");
		if (!fh)
		{
			Error(std::format("ERROR opening RKV {}", m_path));
			return false;
		}
		u64 seekPos = m_dataStart + entry->offset;
		if (_fseeki64(fh, seekPos, SEEK_SET) != 0)
		{
			Error(std::format("ERROR SEEKING RKV TO {} for file {}", m_dataStart + entry->offset, entry->name));
			return false;
		}
		if (fread(temp.Mem(), temp.Size(), 1, fh) != 1)
		{
			Error(std::format("ERROR READING File {} from Archive {}", entry->name, m_path));
			return false;
		}
		fclose(fh);
	}
	temp.DecompressTo(block);
	return true;
}

bool FileSystem_FlatArchive::Exists(const string &name)
{
	// check if entry is in the TOC
	auto it = m_entries.find(StringHash64(name));
	return (it != m_entries.end());
}

void FileSystem_FlatArchive::GetListByFolder(const string &folder, std::vector<string> &files, GetFolderListMode folderMode)
{
	LOG(File, "GetListByFolder NOT IMPLEMENTED!");
}

void FileSystem_FlatArchive::GetListByExcludes(FileExcludes *excludes, std::vector<string> &files)
{
	LOG(File, "GetListByExcludes NOT IMPLEMENTED!");
}

void FileSystem_FlatArchive::GetListByDelegate(const FileSystem_FilenameFilterDelegate &fileChecker, std::vector<string> &list)
{
	for (auto entry : m_entries)
	{
		if (fileChecker(entry.second->name))
			list.push_back(entry.second->name);
	}
}

bool FileSystem_FlatArchive::GetSize(const string &name, u32 &size)
{
	// check if entry is in the TOC
	auto it = m_entries.find(StringHash64(name));
	if (it == m_entries.end())
		return false;
	size = it->second->decompressedSize;
	return true;
}

bool FileSystem_FlatArchive::GetTime(const string &name, u64 &time)
{
	// check if entry is in the TOC
	if (Exists(name))
	{
		time = 0;
		return true;
	}
	return false;
}

void FileSystem_FlatArchive::GetListByExt(const string &ext, std::vector<string> &list)
{
	for (auto entry : m_entries)
	{
		const char *_ext = strrchr(entry.second->name, '.');
        auto name = (const char *)entry.second->name;
        auto is_equal = [name](const string& other) { return other == name; };
		if (_ext && ext == _ext && !std::any_of(list.begin(), list.end(), is_equal))
		{
			list.push_back(entry.second->name);
		}
	}
}

bool FileSystem_FlatArchive::StreamReadBegin(FileHandle handle, const string &name)
{
	if (!Exists(name))
		return false;

	auto fileStream = new FileStream;
	if (Read(name, fileStream->memory))
	{
		LOG(File, std::format("STREAM READ BEGIN {} -> {}", name, fileStream->memory.Size()));
		fileStream->id = handle;
		fileStream->readPtr = fileStream->memory.Mem();
		fileStream->remaining = (u32)fileStream->memory.Size();
		m_activeStreams.push_back(fileStream);
		return true;
	}
	else
	{
		Error(std::format("Load Error on Archive: {} - loading file {}", m_name, name));
	}
	return false;
}

bool FileSystem_FlatArchive::StreamRead(FileHandle handle, u8 *mem, u32 size, u32 &sizeRead)
{
	for (auto fileStream : m_activeStreams)
	{
		if (fileStream->id == handle)
		{
			sizeRead = Min(size, fileStream->remaining);
			if (sizeRead > 0)
			{
				memcpy(mem, fileStream->readPtr, sizeRead);
				fileStream->readPtr += sizeRead;
				fileStream->remaining -= sizeRead;
			}
			return true;
		}
	}
	return false;
}

bool FileSystem_FlatArchive::StreamReadEnd(FileHandle handle)
{
	for (auto it = m_activeStreams.begin(); it != m_activeStreams.end(); ++it)
	{
		FileStream *fileStream = *it;
		if (fileStream->id == handle)
		{
			delete fileStream;
			m_activeStreams.erase(it);
			return true;
		}
	}
	return false;
}

struct BTOCEntry
{
	string filename;
	MemBlock compressed;
	u64 tocEntryOffset;
	u32 tocEntrySize;
};

void FileSystem_FlatArchive::WriteArchive(const string &outputFile)
{
	FileManager &fm = FileManager::Instance();

	// first we build a list of all known files that match our excludes criteria
	string archiveName = outputFile + ".rkv";
	string excludesFile = outputFile + ".exclude";
	auto excludes = new FileExcludes(excludesFile);
	std::vector<string> files;
	fm.GetListByExcludes(excludes, files);

	// first pass - calculate the toc size
	u64 tocSize = 0;
	std::vector<BTOCEntry*> btocsList;
	for (auto filename : files)
	{
		BTOCEntry *btoc = new BTOCEntry;
		btoc->filename = filename;
		u32 nameSize = Max(4, (int)((filename.size()+1 + 3) & 0xfffc));
		btoc->tocEntryOffset = tocSize;
		btoc->tocEntrySize = sizeof(TOCEntry) + nameSize - 4;
		tocSize += btoc->tocEntrySize;
		btocsList.push_back(btoc);
	}

	// allocate the toc
	u8 *tocMem = new u8[tocSize];

	// second pass - build the toc by reading files in and compressing them and keeping them in memory for now
	u32 dataOffset = 0;
	MemBlock tmpMem;

    for (auto btoc : btocsList)
	{
		fm.Read(btoc->filename, tmpMem);
		tmpMem.CompressTo(btoc->compressed);
		TOCEntry *toc = (TOCEntry*)(tocMem + btoc->tocEntryOffset);

		// clear the toc so that trailing space is blank - we want the archive to generate EXACTLY the same every time
		memset(toc, 0, btoc->tocEntrySize);

		toc->compressedSize = (u32)btoc->compressed.Size();
		toc->decompressedSize = (u32)tmpMem.Size();
		strcpy(toc->name, btoc->filename.c_str());
		toc->offset = dataOffset;
		toc->tocEntrySize = btoc->tocEntrySize;
		dataOffset += toc->compressedSize;
		LOG(File, STR("TOC[%5d] : <%d -> %d>(%d%%) @ %8d %s", btoc->tocEntryOffset, toc->decompressedSize, toc->compressedSize, toc->compressedSize * 100 / toc->decompressedSize, toc->offset, toc->name));
	}

	// finally ready to write out the toc and those files out
	FILE *fh = fopen(archiveName.c_str(), "wb");
	if (fh)
	{
		fwrite(&tocSize, 4, 1, fh);
		fwrite(tocMem, tocSize, 1, fh);
		for (auto btoc : btocsList)
		{
			fwrite(btoc->compressed.Mem(), btoc->compressed.Size(), 1, fh);
		}
		fclose(fh);
	}
	else
	{
		Error(std::format("Unable to create archive: {}", outputFile));
	}
}
