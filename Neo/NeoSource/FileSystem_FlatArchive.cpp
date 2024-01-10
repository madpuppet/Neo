#include "Neo.h"
#include "FileSystem_FlatArchive.h"
#include "FileManager.h"
#include "Thread.h"
#include "Math.h"

#include <algorithm>

FileSystem_FlatArchive::FileSystem_FlatArchive(const String &name, const String &path, int priority)
	: m_name(name), m_path(path), m_priority(priority), m_dataStart(0)
{
	Log(STR("MOUNT ARCHIVE: %s", name.CStr()));

	m_threadID = Thread::CurrentThreadID();
	m_fh = fopen(path.CStr(), "rb");
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

					Log(STR("TOC: <%d->%d> %s", entry->compressedSize, entry->decompressedSize, entry->name));
				}
			}
			else
			{
				Error(STR("Error reading archive: %s", path.CStr()));
			}
		}
		else
		{
			Error(STR("Possibly bad archive! Toc size is %d", tocSize));
		}
	}
}

FileSystem_FlatArchive::~FileSystem_FlatArchive()
{
	if (m_fh)
		fclose(m_fh);
}

bool FileSystem_FlatArchive::Read(const String &name, MemBlock &block)
{
	// check if entry is in the TOC
	auto it = m_entries.find(name.Hash64());
	if (it == m_entries.end())
		return false;

	// if on the main thread, we can just use our open file pointer
	ThreadID currentThread = Thread::CurrentThreadID();
	auto entry = it->second;
	if (!block.Resize(entry->decompressedSize))
	{
		Error(STR("Unable to fit file %s in supplied memory!", name.CStr()));
		return false;
	}

	MemBlock temp(entry->compressedSize);
	if (currentThread == m_threadID)
	{
		// main thread can just seek and read for speed
		fseek(m_fh, m_dataStart + entry->offset, SEEK_SET);
		fread(temp.Mem(), temp.Size(), 1, m_fh);
	}
	else
	{
		// different thread, so we need to open a new file instance to be safe...
		FILE *fh = fopen(m_path.CStr(), "rb");
		if (!fh)
		{
			Error(STR("ERROR opening RKV %s", m_path.CStr()));
			return false;
		}
		int seekPos = m_dataStart + entry->offset;
		if (fseek(fh, seekPos, SEEK_SET) != 0)
		{
			Error(STR("ERROR SEEKING RKV TO %d for file %s", m_dataStart + entry->offset, entry->name));
			return false;
		}
		if (fread(temp.Mem(), temp.Size(), 1, fh) != 1)
		{
			Error(STR("ERROR READING File %s from Archive %s", entry->name, m_path.CStr()));
			return false;
		}
		ftell(fh);
		fclose(fh);
	}
	temp.DecompressTo(block);
	return true;
}

bool FileSystem_FlatArchive::Exists(const String &name)
{
	// check if entry is in the TOC
	auto it = m_entries.find(name.Hash64());
	return (it != m_entries.end());
}

void FileSystem_FlatArchive::GetListByFolder(const String &folder, std::vector<String> &files, GetFolderListMode folderMode)
{
	Log("GetListByFolder NOT IMPLEMENTED!");
}

void FileSystem_FlatArchive::GetListByExcludes(FileExcludes *excludes, std::vector<String> &files)
{
	Log("GetListByExcludes NOT IMPLEMENTED!");
}

void FileSystem_FlatArchive::GetListByDelegate(const FileSystem_FilenameFilterDelegate &fileChecker, std::vector<String> &list)
{
	for (auto entry : m_entries)
	{
		if (fileChecker(entry.second->name))
			list.push_back(entry.second->name);
	}
}

bool FileSystem_FlatArchive::GetSize(const String &name, u32 &size)
{
	// check if entry is in the TOC
	auto it = m_entries.find(name.Hash64());
	if (it == m_entries.end())
		return false;
	size = it->second->decompressedSize;
	return true;
}

bool FileSystem_FlatArchive::GetTime(const String &name, u64 &time)
{
	// check if entry is in the TOC
	if (Exists(name))
	{
		time = 0;
		return true;
	}
	return false;
}

void FileSystem_FlatArchive::GetListByExt(const String &ext, std::vector<String> &list)
{
	for (auto entry : m_entries)
	{
		const char *_ext = strrchr(entry.second->name, '.');
        auto name = (const char *)entry.second->name;
        auto is_equal = [name](const String& other) { return other == name; };
		if (_ext && ext == _ext && !std::any_of(list.begin(), list.end(), is_equal))
		{
			list.push_back(entry.second->name);
		}
	}
}

bool FileSystem_FlatArchive::StreamReadBegin(FileHandle handle, const String &name)
{
	if (!Exists(name))
		return false;

	auto fileStream = new FileStream;
	if (Read(name, fileStream->memory))
	{
		Log(STR("STREAM READ BEGIN %s -> %d", name.CStr(), fileStream->memory.Size()));
		fileStream->id = handle;
		fileStream->readPtr = fileStream->memory.Mem();
		fileStream->remaining = fileStream->memory.Size();
		m_activeStreams.push_back(fileStream);
		return true;
	}
	else
	{
		Error(STR("Load Error on Archive: %s - loading file %s", m_name.CStr(), name.CStr()));
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
	String filename;
	MemBlock compressed;
	u32 tocEntryOffset;
	u32 tocEntrySize;
};

void FileSystem_FlatArchive::WriteArchive(const String &outputFile)
{
	FileManager &fm = FileManager::Instance();

	// first we build a list of all known files that match our excludes criteria
	String archiveName = outputFile + ".rkv";
	String excludesFile = outputFile + ".exclude";
	auto excludes = new FileExcludes(excludesFile);
	std::vector<String> files;
	fm.GetListByExcludes(excludes, files);

	// first pass - calculate the toc size
	u32 tocSize = 0;
	std::vector<BTOCEntry*> btocsList;
	for (auto filename : files)
	{
		BTOCEntry *btoc = new BTOCEntry;
		btoc->filename = filename;
		u32 nameSize = Max(4, (filename.Length()+1 + 3) & 0xfffc);
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

		toc->compressedSize = btoc->compressed.Size();
		toc->decompressedSize = tmpMem.Size();
		strcpy(toc->name, btoc->filename.CStr());
		toc->offset = dataOffset;
		toc->tocEntrySize = btoc->tocEntrySize;
		dataOffset += toc->compressedSize;
		Log(STR("TOC[%5d] : <%d -> %d>(%d%%) @ %8d %s", btoc->tocEntryOffset, toc->decompressedSize, toc->compressedSize, toc->compressedSize * 100 / toc->decompressedSize, toc->offset, toc->name));
	}

	// finally ready to write out the toc and those files out
	FILE *fh = fopen(archiveName.CStr(), "wb");
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
		Error(STR("Unable to create archive: %s", outputFile.CStr()));
	}
}
