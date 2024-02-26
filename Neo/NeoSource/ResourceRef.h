#pragma once

template <class T, class F>
class ResourceRef
{
public:
	ResourceRef() : m_ptr(0) {}

	// copy - adds a reference to the resource
	ResourceRef(const ResourceRef &o) : m_ptr(0) { Set(o.m_ptr); }

	// move - just copy the ptr and clear the old pointer. reference count stays the same
	ResourceRef(ResourceRef&& o) noexcept : m_ptr(o.m_ptr) { o.m_ptr = 0; }

	// create from a type ptr
	ResourceRef(T* o)
	{
		m_ptr = o;
		if (m_ptr)
			m_ptr->IncRef();
	}

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

	// copy operator - takes another reference
	ResourceRef<T,F> &operator =(const ResourceRef<T,F> &o)
	{
		Set((T*)o.m_ptr);
		return *this;
	}

	// move operator - just clear current reference and copy over the ptr
	ResourceRef<T, F>& operator =(ResourceRef<T, F>&& o) noexcept
	{
		Destroy();
		m_ptr = o.m_ptr;
		o.m_ptr = nullptr;
		return *this;
	}

	// ptr to a resource -> takes a reference count to it
	ResourceRef<T,F> &operator =(const T *o)
	{
		Set((T*)o);
		return *this;
	}

	~ResourceRef()
	{
		Destroy();
	}

	void Create(const string &name)
	{
		Destroy();
		m_ptr = F::Instance().Create(name);
	}

protected:
	// on Destroy we just reduce ref count, factory will destroy the
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

