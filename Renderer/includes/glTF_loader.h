#pragma once

#include "gfx_model.h"

namespace glTF_L
{
	void LoadMesh( const char* fileName, GfxModel* model, I_BufferAllocator* allocator );
	void LoadCollisionData( const char* fileName, std::vector<glm::vec3>* vertices, std::vector<uint32_t>* indices );
}