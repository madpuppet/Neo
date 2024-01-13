#pragma once

template<class T>
class Singleton
{
	static T* s_instance;

public:
	Singleton()
	{
		Assert( !Exists(), "Multiple definitions of a single singleton??" );
		s_instance = (T*)this;
	}

	~Singleton()
	{
		Assert( s_instance == this, "Multiple definitions of a single singleton??" );
		s_instance = NULL;
	}

	static T*   Startup()
	{
		return new( T );
	}

	static void Shutdown()
	{
		delete( &Instance() );
	}

	static bool Exists()
	{
		return (s_instance != NULL);
	}

	static T &Instance() 
	{ 
		Assert( Exists(), "Requested non-existing singleton??" );
		return *s_instance; 
	}
};

template<class T> T* Singleton<T>::s_instance = NULL;
