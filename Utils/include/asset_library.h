#pragma once
#include "gfx_image.h"
#include "gfx_model.h"

#include "glm/vec4.hpp"

namespace AL
{
	GfxImage* LoadTexture( const char* assetName, const char* assetPath, I_ImageAlloctor* allocator );
	GfxImage* LoadCubeTexture(const char* assetName, const char* assetPath, I_ImageAlloctor* allocator );
	GfxImage* CreateSolidColorTexture(const char* assetName, glm::vec4 color, I_ImageAlloctor* allocator );
	GfxModel* CreateQuad( const char* assetName, float size, I_BufferAllocator* allocator );

	GfxModel* Load3DModel(const char* assetName, const char* assetPath, uint32_t hackIndex, I_BufferAllocator* allocator );
	GfxModel* LoadglTf3DModel( const char* assetName, const char* assetPath, I_BufferAllocator* allocator );
	GfxModel* RegisterGfxModel( const char* assetName, GfxModel&& model );

	void* GetAsset(const char* assetName);

	void Cleanup();
}