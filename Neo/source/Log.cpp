#include "Neo.h"
#include "Log.h"
#include "StringUtils.h"

CmdLineVar<bool> CLV_EnableLogging("logenable","enable logging",true);
CmdLineVar<string> CLV_LogFile("logfile", "set path to log file", "local:log.txt");
CmdLineVar<bool> CLV_LogTimes("logtime", "show timestamp on logs", true);

bool __NeoLogFilteringEnabled = false;

static set<string> s_filtersEnabled;
static FileHandle s_logHandle;

void NeoSetLogFilters(const stringlist& filters)
{
	s_filtersEnabled.clear();
	s_filtersEnabled.insert("Any");
	for (auto& token : filters)
	{
		s_filtersEnabled.insert(token);
	}
	__NeoLogFilteringEnabled = true;
}

bool NeoLogEnabled(const string& filter)
{
	return CLV_EnableLogging.Value() && (filter.empty() || (s_filtersEnabled.find(filter) != s_filtersEnabled.end()));
}

void NeoEnableFileLogging()
{
	if (CLV_EnableLogging.Value())
		FileManager::Instance().StreamWriteBegin(s_logHandle, CLV_LogFile.Value());
}

void NeoLog(const string& filter, const string& text)
{
	NOMEMTRACK();
	string outStr = filter + ": " + text + "\n";
	printf("%s", outStr.c_str());
#if defined(PLATFORM_Windows)
	OutputDebugString(outStr.c_str());
#endif
	if (s_logHandle != nullFileHandle)
	{
		FileManager::Instance().StreamWrite(s_logHandle, outStr);
		FileManager::Instance().StreamFlush(s_logHandle);
	}
}
