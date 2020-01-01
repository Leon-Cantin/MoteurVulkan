#pragma once

#include "gfx_model.h"

struct GfxAsset {
	const GfxModel* modelAsset;
	std::vector<uint32_t> textureIndices;
};
