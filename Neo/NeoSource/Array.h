#pragma once

template<typename T>
class Array
{
public:
	Array() : m_size(0), m_items(0) {}
	~Array() { Clear(); }
	Array(const Array<T>& o)
	{
		Init(o);
	}
	Array(Array<T>&& o)
	{
		m_size = o.m_size;
		m_items = o.m_items;
		o.m_size = 0;
		o.m_items = nullptr;
	}

	Array& operator = (const Array<T>& o)
	{
		Resize((u32)o.size());
		for (size_t i = 0; i < o.size(); i++)
		{
			m_items[i] = o.m_items[i];
		}
		return *this;
	}

	u32 m_size;
	T* m_items;

	bool IsEmpty()
	{
		return m_size == 0;
	}

	void Clear()
	{
		if (m_size > 0)
		{
			for (u32 i = 0; i < m_size; i++)
			{
				m_items[i].~T();
			}
			free(m_items);
			m_items = 0;
			m_size = 0;
		}
	}

	void Remove(int idx)
	{
		Assert(idx < (int)m_size, "Bad IDX to delete!");
		for (u32 i = (u32)idx; i < m_size - 1; i++)
		{
			m_items[i] = m_items[i + 1];
		}
		Resize(m_size - 1);
	}

	void Add(const T& item)
	{
		Resize(m_size + 1);
		m_items[m_size - 1] = item;
	}

	void Insert(int idx, const T& item)
	{
		Resize(m_size + 1);
		for (int i = m_size - 2; i >= idx; --i)
			m_items[i + 1] = m_items[i];
		m_items[idx] = item;
	}

	inline T& operator [] (u32 a_index)
	{
		Assert(a_index < m_size, "Array index out of bounds!");
		return m_items[a_index];
	}

	inline T operator [] (u32 a_index) const
	{
		Assert(a_index < m_size, "Array index out of bounds!");
		return m_items[a_index];
	}

	inline void operator = (const std::vector<T>& o)
	{
		Resize((u32)o.size());
		for (size_t i = 0; i < o.size(); i++)
			m_items[i] = o[i];
	}

	void Resize(u32 size)
	{
		if (m_size != size)
		{
			if (size == 0)
			{
				for (u32 i = 0; i < m_size; i++)
					(&m_items[i])->~T();
				free(m_items);

				m_items = 0;
				m_size = 0;
			}
			else
			{
				// allocate a new array of items
				T* newItems = (T*)malloc(size * sizeof(T));
				for (u32 i = 0; i < size; i++)
				{
					// construct and copy over the old items to the new array
					new (&newItems[i]) T();
					if (i < m_size)
						newItems[i] = m_items[i];
				}

				// destruct the old items and free the memory
				if (m_size > 0)
				{
					for (u32 i = 0; i < m_size; i++)
					{
						m_items[i].~T();
					}
					free(m_items);
				}

				// we can now point to the new items/size
				m_items = newItems;
				m_size = size;
			}
		}
	}

	// vector style interface
	// these needed to allow for(auto item : array) {}
	T* begin() { return &m_items[0]; }
	T* end() { return &m_items[m_size]; }
	const T* begin() const { return &m_items[0]; }
	const T* end() const { return &m_items[m_size]; }

	T& front()
	{
		Assert(m_size > 0, "Attempt to get front of empty list!");
		return m_items[0];
	}
	T& back()
	{
		Assert(m_size > 0, "Attempt to get back of empty list!");
		return m_items[m_size - 1];
	}
	void erase(const T* item)
	{
		Assert(m_size > 0 && item >= &m_items[0] && item <= &m_items[m_size - 1], "Bad Item for erase");
		if (item != &m_items[m_size - 1])
		{
			*(T*)item = m_items[m_size - 1];
		}
		m_size--;
	}
	void push_back(const T& item)
	{
		Resize(m_size + 1);
		m_items[m_size - 1] = item;
	}
	void pop_back()
	{
		Resize(m_size - 1);
	}
	size_t size() const
	{
		return (size_t)m_size;
	}
	bool empty() const
	{
		return m_size == 0;
	}

private:
	void Init(const Array<T>& o)
	{
		if (o.m_size == 0)
		{
			m_size = 0;
			m_items = 0;
		}
		else
		{
			m_size = o.m_size;
			m_items = (T*)malloc(sizeof(T) * m_size);
			for (u32 i = 0; i < m_size; i++)
			{
				new (&m_items[i]) T();
				m_items[i] = o.m_items[i];
			}
		}
	}
};

template <class T, int SIZE>
class FixedArray
{
public:
	FixedArray() { memset(m_items, 0, sizeof(m_items)); }

	inline T& operator [] (unsigned int a_index)
	{
		Assert(a_index < SIZE, "Array index out of bounds!");
		return m_items[a_index];
	}

	inline T operator [] (unsigned int a_index) const
	{
		Assert(a_index < SIZE, "Array index out of bounds!");
		return m_items[a_index];
	}

	T* GetAddress() { return m_items; }

	// STL style arguments
	// these needed to allow for(auto item : array) {}
	T* begin() { return &m_items[0]; }
	T* end() { return &m_items[SIZE]; }
	const T* begin() const { return &m_items[0]; }
	const T* end() const { return &m_items[SIZE]; }
	size_t size() const
	{
		return (size_t)SIZE;
	}

private:
	T m_items[SIZE];
};
