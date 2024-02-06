#pragma once

extern void NeoEnableFileLogging();
extern void NeoSetLogFilters(const stringlist& filters);
extern void NeoEnableLogFiltering(bool enable);
extern bool NeoLogEnabled(const string &filter);
extern void NeoLog(const string &filter, const string& text);
extern bool __NeoLogFilteringEnabled;
extern CmdLineVar<bool> CLV_DisableLogging;

#define LOG(filter, text) if (!CLV_DisableLogging.Value() && (!__NeoLogFilteringEnabled || NeoLogEnabled(#filter))) NeoLog(#filter, text)
