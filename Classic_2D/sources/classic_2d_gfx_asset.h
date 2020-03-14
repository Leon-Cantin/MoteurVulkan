#pragma once
#include "gfx_asset.h"

glm::fquat defaultRotation = glm::angleAxis( glm::radians( 0.0f ), glm::vec3 { 0.0f, 1.0f, 0.0f } );
constexpr float quadSize = 1.0f;

GfxAsset CreateGfxAsset( const GfxModel* modelAsset, uint32_t albedoIndex )
{
	GfxAsset asset = { modelAsset, { albedoIndex } };
	return asset;
}
