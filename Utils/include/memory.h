#pragma once
#include <cstring>

namespace MEM
{
	template< class T >
	void zero( T* ptr )
	{
		memset( ptr, 0, sizeof( T ) );
	}

	template< class T >
	void zero( T* ptr, size_t size )
	{
	 	memset( ptr, 0, size );
	}
}
