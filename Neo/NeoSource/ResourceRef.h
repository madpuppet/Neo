#pragma once

#include "AssetRefresh.h"
#include <map>

namespace Mad
{
	template <class T, class F>
	class ResourceRef
	{
	public:
		ResourceRef() : m_ptr(0) {}
		ResourceRef(const ResourceRef &o) : m_ptr(0) { Set(o.m_ptr); }

		T* operator *() const         { return const_cast<T*>(m_ptr); }
		T* operator ->() const        { return const_cast<T*>(m_ptr); }
		operator T *() const          { return const_cast<T*>(m_ptr); }

		bool operator==(T *o) const   { return m_ptr == o; }
		bool operator!=(T *o) const   { return m_ptr != o; }

		void Set(T *o)
		{
			if (m_ptr != o)
			{
				Destroy();
				m_ptr = o;
				if (m_ptr)
					m_ptr->IncRef();
			}
		}

		ResourceRef<T,F> &operator =(const ResourceRef<T,F> &o)
		{
			Set((T*)o.m_ptr);
			return *this;
		}

		ResourceRef<T,F> &operator =(const T *o)
		{
			Set((T*)o);
			return *this;
		}

		~ResourceRef()
		{
			Destroy();
		}

		void Create(const DMString &name)
		{
			Destroy();
			m_ptr = F::Instance().Create(name, name);
		}

		void Create(const DMString &name, const DMString &source)
		{
			Destroy();
			m_ptr = F::Instance().Create(name, source);
		}

	protected:
		void CheckedDestroy()
		{
			DMASSERTD(!m_ptr || m_ptr->GetRef() == 1, 
				STR("Attempt to re-create resource but current instance ('%s') is still being referenced externally - may be leaked!",m_ptr->GetName().CStr()));
			Destroy();
		}

		void Destroy()
		{
			if (m_ptr != 0)
			{
				F::Instance().Destroy(m_ptr);
				m_ptr = 0;
			}
		}

		T* m_ptr;
	};
}
