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

	// some loaded assets can be marked as FailedToLoad - which means you need to use a default version of the resource, or abort and fix the problem
	bool FailedToLoad() { return m_failedToLoad; }

protected:
	void OnLoadComplete();
	virtual void Reload() = 0;

	int m_refCount = 1;
	string m_name;
	bool m_dataLoaded = false;
	bool m_failedToLoad = false;
	friend class ResourceLoadedManager;
};

