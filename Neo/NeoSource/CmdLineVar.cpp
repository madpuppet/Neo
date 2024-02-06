#include "Neo.h"
#include "CmdLineVar.h"
#include "StringUtils.h"

struct CmdLineCallbacks
{
	CmdLineProcessCB process;
	GenericCallback dump;
};
static hashtable<string, CmdLineCallbacks> s_cmdLines;

void NeoRegisterCommandLine(const string& name, CmdLineProcessCB process, GenericCallback dump)
{
	s_cmdLines[name] = { process, dump };
}

void NeoParseCommandLine(int argc, char* argv[])
{
	for (int i = 1; i < argc; i++)
	{
		stringlist tokenValue = StringSplit(string(argv[i]), '=');
		if (tokenValue.size() != 1 && tokenValue.size() != 2)
		{
			LOG(Error, STR("Cannot parse cmdline token: '{}'   Expected 'token=value[,value,value]'", argv[i]));
		}
		else
		{
			auto it = s_cmdLines.find(tokenValue[0]);
			if (it != s_cmdLines.end())
			{
				stringlist tokens;
				if (tokenValue.size() == 2)
					tokens = StringSplit(tokenValue[1], ',');
				it->second.process(tokens);
			}
			else
			{
				LOG(Error, STR("Unknown cmdline '{}'", tokenValue[0]));
			}
		}
	}
}

void NeoDumpCmdLineVars()
{
	LOG(Any, "## DUMP COMMAND LINE VARIABLES");
	for (auto it : s_cmdLines)
	{
		it.second.dump();
	}
}

void CmdLineVar<bool>::Dump()
{
	LOG(Any, STR("-- token: {}\ndesc: {}\ndefault: {}", m_name, m_desc, m_defaultValue));
	if (m_exists) LOG(CmdLine, STR("value: {}", m_value));
}

void CmdLineVar<bool>::ProcessToken(stringlist values)
{
	m_exists = true;
	if (values.empty() || StringEqual(values[0], "1") || StringEqual(values[0], "true"))
		m_value = true;
	else
		m_value = false;
}

void CmdLineVar<string>::Dump()
{
	LOG(Any, STR("-- token: {}\ndesc: {}\ndefault: {}", m_name, m_desc, m_defaultValue));
	if (m_exists) LOG(CmdLine, STR("value: {}", m_value));
}

void CmdLineVar<string>::ProcessToken(stringlist values)
{
	m_exists = true;
	if (values.size() > 0)
		m_value = values[0];
}

void CmdLineVar<stringlist>::Dump()
{
	string defaultValue;
	for (int i = 0; i < m_defaultValue.size(); i++)
	{
		defaultValue += m_defaultValue[i];
		if (i < m_defaultValue.size() - 1)
			defaultValue += ", ";
	}

	LOG(Any, STR("-- token: {}\ndesc: {}\ndefault: {}", m_name, m_desc, defaultValue));
	if (m_exists)
	{
		string strValue;
		for (int i = 0; i < m_value.size(); i++)
		{
			strValue += m_value[i];
			if (i < m_value.size() - 1)
				strValue += ", ";
		}

		LOG(Any, STR("value: {}", strValue));
	}
}

void CmdLineVar<stringlist>::ProcessToken(stringlist values)
{
	m_exists = true;
	m_value = values;
}
