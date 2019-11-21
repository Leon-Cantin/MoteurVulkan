#pragma once
#include "gfx_image.h"
#include "model_asset.h"

#include "glm/vec4.hpp"

namespace AL
{
	GfxImage* LoadTexture(const char* assetName, const char* assetPath);
	GfxImage* LoadCubeTexture(const char* assetName, const char* assetPath);
	GfxImage* CreateSolidColorTexture(const char* assetName, glm::vec4 color);
	GfxModel* CreateQuad( const char* assetName );

	GfxModel* Load3DModel(const char* assetName, const char* assetPath, uint32_t hackIndex);
	GfxModel* LoadglTf3DModel( const char* assetName, const char* assetPath );

	void* GetAsset(const char* assetName);

	void Cleanup();
}