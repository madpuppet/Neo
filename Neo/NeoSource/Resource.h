#pragma once

class Resource
{
public:
	Resource(const std::string &name) : m_name(name) {}
	virtual ~Resource() {}

	int IncRef() { return ++m_refCount; }
	int DecRef() { return --m_refCount; }
	int GetRef() { return m_refCount; }

	std::string GetName() const { return m_name; }

	bool IsLoaded() { return m_dataLoaded; }

protected:
	virtual void Reload() = 0;

	int m_refCount = 1;
	bool m_dataLoaded = false;
	std::string m_name;
	std::string m_source;
};

