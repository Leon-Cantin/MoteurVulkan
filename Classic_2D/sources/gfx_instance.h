#pragma once

#include "vk_globals.h"
#include "gfx_model.h"
#include "gfx_asset.h"
#include "scene_instance.h"

#include "glm/mat4x4.hpp"

struct GfxInstanceData {
	glm::mat4 model;
	uint32_t texturesIndexes[4];
	uint32_t dithering[4];
};

struct GfxAssetInstance
{
	const GfxAsset* asset;
	SceneInstance instanceData;
	bool useDithering;
};