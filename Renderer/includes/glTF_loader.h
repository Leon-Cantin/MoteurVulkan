#pragma once

#include "gfx_model.h"

namespace glTF_L
{
	void LoadMesh( const char* fileName, GfxModel* model, I_BufferAllocator* allocator );
}