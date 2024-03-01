#pragma once

extern void NeoEnableFileLogging();
extern void NeoSetLogFilters(const stringlist& filters);
extern void NeoEnableLogFiltering(bool enable);
extern bool NeoLogEnabled(const string &filter);
extern void NeoLog(const string &filter, const string& text);
extern bool __NeoLogFilteringEnabled;
extern CmdLineVar<bool> CLV_EnableLogging;

#define LOG(filter, text) if (CLV_EnableLogging.Value() && (!__NeoLogFilteringEnabled || NeoLogEnabled(#filter))) NeoLog(#filter, text)
