#pragma once

#include <cstdint>

namespace IH
{
	enum eKeyState
	{
		Pressed = 0,
		Released = 1
	};
	//Same as Windows Virtual keys
	enum eKeys : uint32_t
	{
		UNKNOWN = 0,
		SPACE = 0x20,
		BACKSPACE = 0x08,
		ENTER = 0x0D,
		A = 0x41,
		B,
		C,
		D,
		E,
		F,
		G,
		H,
		I,
		J,
		K,
		L,
		M,
		N,
		O,
		P,
		Q,
		R,
		S,
		T,
		U,
		V,
		W,
		X,
		Y,
		Z = 0x5B,
		TILD = 0xDE,
	};
}