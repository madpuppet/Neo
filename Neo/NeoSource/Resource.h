#pragma once

#include "AssetRefresh.h"

namespace Mad
{
	template <class T>
	class Resource
	{
	public:
		Resource(const std::string &name, const std::string&source) : m_name(name), m_source(source), m_refCount(1), m_assetRefresh(name, DELEGATE(Resource::ReloadInternal)) {}
		virtual ~Resource() {}

		int IncRef() { return ++m_refCount; }
		int DecRef() { return --m_refCount; }
		int GetRef() { return m_refCount; }

		AssetRefresh *GetAssetRefresh() { return &m_assetRefresh; }

		DMString GetName() const { return m_name; }
		DMString GetSource() const { return m_source; }

	protected:
		void ReloadInternal(AssetRefresh*,void*)
		{
			m_assetRefresh.ClearFiles();
			Reload();
		}
		virtual void Reload() = 0;

		AssetRefresh m_assetRefresh;
		int m_refCount;
		std::string m_name;
		std::string m_source;
	};
}

