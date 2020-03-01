#pragma once
#include "gfx_image.h"
#include "gfx_model.h"

#include "glm/vec4.hpp"

namespace AL
{
	GfxImage* LoadTexture( const char* assetName, const char* assetPath, I_ImageAlloctor* allocator );
	GfxImage* LoadCubeTexture(const char* assetName, const char* assetPath);
	GfxImage* CreateSolidColorTexture(const char* assetName, glm::vec4 color);
	GfxModel* CreateQuad( const char* assetName, float size );

	GfxModel* Load3DModel(const char* assetName, const char* assetPath, uint32_t hackIndex);
	GfxModel* LoadglTf3DModel( const char* assetName, const char* assetPath );

	void* GetAsset(const char* assetName);

	void Cleanup();
}