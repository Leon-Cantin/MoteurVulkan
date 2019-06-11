#pragma once

#include <cstdint>

namespace IH
{
	enum key_status
	{
		PRESSED,
		RELEASED
	};

	enum keys : uint32_t
	{
		UNKNOWN,
		ENTER,
		BACKSPACE,
		GRAVE_ACCENT,
		W,
		A,
		S,
		D
	};
}