#pragma once

class RefCounted
{
public:
	RefCounted() : m_refCount(1) {}
	void IncRef() { m_refCount++; }
	int DecRef() { return --m_refCount; }
	int GetRef() { return m_refCount; }
protected:
	int m_refCount;
};

template <class T>
class RefCountedRef
{
public:
	RefCountedRef() : m_ptr(0) {}
	virtual ~RefCountedRef() { Destroy(); }
	RefCountedRef(const RefCountedRef &o) : m_ptr(0) { Set(o.m_ptr); }

	T* operator ->()              { return m_ptr; }
	const T* operator ->() const  { return m_ptr; }
	T* operator *()               { return m_ptr; }
	const T* operator *() const   { return m_ptr; }
	operator T *()                { return m_ptr; }
	operator const T *() const    { return m_ptr; }
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

	RefCountedRef &operator =(const RefCountedRef &o)
	{
		Set(o.m_ptr);
		return *this;
	}

	RefCountedRef &operator =(T *o)
	{
		Set(o);
		return *this;
	}

protected:	
	void CheckedDestroy()
	{
		Assert(!m_ptr || m_ptr->GetRef()==1, "Attempt to recreate instance, but old instance is still referenced - may be leaked!");
	}

	void Destroy()
	{
		if (m_ptr)
		{
			int _rc = m_ptr->DecRef();
			if (_rc == 0)
				delete m_ptr;
			m_ptr = 0;
		}
	}

	T *m_ptr;
};

