#pragma once

typedef std::function<void(stringlist)> CmdLineProcessCB;

class CmdLineVarBase
{
protected:
	CmdLineVarBase(const string& name, const string& desc) : m_name(name), m_desc(desc) {}
	~CmdLineVarBase() {}

	string m_name;
	string m_desc;
	bool m_exists = false;

public:
	virtual void ProcessToken(stringlist values) = 0;
	virtual void Dump() = 0;
	bool Exists() { return m_exists; };
	const string& Name() { return m_name; };
	const string& Desc() { return m_desc; };
};

template <typename T>
class CmdLineVar : public CmdLineVarBase
{
protected:
	T m_defaultValue;
	T m_value;

public:
	CmdLineVar(const string &name, const string &desc, T defaultValue) : m_defaultValue(defaultValue), CmdLineVarBase(name, desc)
	{
		NeoRegisterCommandLine(this);
	}
	T Value() { return m_exists ? m_value : m_defaultValue; }

	virtual void ProcessToken(stringlist values) override;
	virtual void Dump() override;
};

void NeoRegisterCommandLine(CmdLineVarBase *clvb);
void NeoParseCommandLine(int argc, char* argv[]);
void NeoDumpCmdLineVars();
