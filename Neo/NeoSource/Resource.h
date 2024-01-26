#pragma once

typedef std::function<void(class Resource*)> ResourceLoadedCB;

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
	virtual void OnLoadComplete() { m_dataLoaded = true; }
	virtual void Reload() = 0;

	int m_refCount = 1;
	string m_name;
	bool m_dataLoaded;
	vector<ResourceLoadedCB> m_onLoadedCB;

	friend class ResourceLoadedManager;
	void AddOnLoadedCB(const ResourceLoadedCB& cb) { m_onLoadedCB.emplace_back(cb); }
	void TriggerOnLoadedCBs()
	{
		for (auto& cb : m_onLoadedCB) cb(this);
	}
};

