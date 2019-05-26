#pragma once
#include "vk_image.h"
#include "model_asset.h"

#include "glm/vec4.hpp"

namespace AL
{
	GfxImage* LoadTexture(const char* assetName, const char* assetPath);
	GfxImage* LoadCubeTexture(const char* assetName, const char* assetPath);
	GfxImage* CreateSolidColorTexture(const char* assetName, glm::vec4 color);

	ModelAsset* Load3DModel(const char* assetName, const char* assetPath, uint32_t hackIndex);

	void* GetAsset(const char* assetName);

	void Cleanup();
}