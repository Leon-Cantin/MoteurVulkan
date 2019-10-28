#pragma once

namespace MEM
{
	template< class T >
	void ZeroMem( T* ptr )
	{
		ZeroMemory( ptr, sizeof( T ) );
	}
}
