#pragma once

#include "TimeManager.h"

class Resource
{
public:
	void Init(const string& name) {	m_name = name; m_creationStartTime = NeoTimeNow; }
	virtual ~Resource() {}
	virtual const string& GetType() const = 0;

	int IncRef() { return ++m_refCount; }
	int DecRef() { return --m_refCount; }
	int GetRef() { return m_refCount; }

	const string& GetName() const { return m_name; }
	bool IsLoaded() { return m_dataLoaded; }
	void MarkIsLoaded() { m_dataLoaded = true; }

	// some loaded assets can be marked as FailedToLoad - which means you need to use a default version of the resource, or abort and fix the problem
	bool FailedToLoad() { return m_failedToLoad; }

protected:
	void OnLoadComplete();
	virtual void Reload() = 0;

	int m_refCount = 1;
	string m_type;
	string m_name;
	bool m_dataLoaded = false;
	bool m_failedToLoad = false;
	friend class ResourceLoadedManager;

	double m_creationStartTime;
};

