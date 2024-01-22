#pragma once

class Resource
{
public:
	Resource(const string &name) : m_name(name) {}
	virtual ~Resource() {}

	int IncRef() { return ++m_refCount; }
	int DecRef() { return --m_refCount; }
	int GetRef() { return m_refCount; }

	string GetName() const { return m_name; }

	bool IsLoaded() { return m_dataLoaded; }

protected:
	virtual void Reload() = 0;

	int m_refCount = 1;
	bool m_dataLoaded = false;
	string m_name;
	string m_source;
};

