#pragma once

typedef std::function<void(stringlist)> CmdLineProcessCB;

template <typename T>
class CmdLineVar
{
protected:
	string m_name;
	string m_desc;
	T m_defaultValue;

	bool m_exists = false;
	T m_value;

public:
	CmdLineVar(string name, string desc, T defaultValue) : m_name(name), m_desc(desc), m_defaultValue(defaultValue)
	{
		NeoRegisterCommandLine(name, [this](stringlist values) {ProcessToken(values); }, [this]() {Dump();});
	}
	T Value() { return m_exists ? m_value : m_defaultValue; }
	bool Exists() { return m_exists; }

	void ProcessToken(stringlist values);
	void Dump();
};

void NeoRegisterCommandLine(const string &name, CmdLineProcessCB process, GenericCallback dump);
void NeoParseCommandLine(int argc, char* argv[]);
void NeoDumpCmdLineVars();
