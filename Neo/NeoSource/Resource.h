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
	void MarkIsLoaded() { m_dataLoaded = true; }
	virtual enum AssetType GetAssetType() = 0;

protected:
	void OnLoadComplete();
	virtual void Reload() = 0;

	int m_refCount = 1;
	string m_name;
	bool m_dataLoaded;
	friend class ResourceLoadedManager;
};

