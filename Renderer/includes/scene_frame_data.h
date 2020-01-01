#pragma once

#include "gfx_asset.h"

#include <vector>

struct SceneInstanceSet
{
	uint32_t geometryBufferOffsets;
};

struct DrawListEntry
{
    const GfxAsset* asset;
	SceneInstanceSet descriptorSet;
};

struct SceneFrameData {
	std::vector<DrawListEntry> drawList;
};
