#pragma once

template <unsigned int count>
struct bitfield
{
	typedef char bitfield_type;
	constexpr size_t nbsub = count / (sizeof(bitfield_type) * 8);
	bitfield_type[nbsub] field;

	bool operator[](size_t loc)
	{
		field[]
		return
	}
};