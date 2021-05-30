#pragma once
#include "gfx_image.h"
#include "gfx_model.h"
#include "gfx_asset.h"

#include "glm/vec4.hpp"

namespace AL
{
	R_HW::GfxImage* AL_GetImageSlot( const char* assetName );
	GfxModel* AL_GetModelSlot( const char* assetName );
	GfxAsset* AL_GetAssetSlot( const char* assetName );

	R_HW::GfxImage* LoadTexture( const char* assetName, const char* assetPath, I_ImageAlloctor* allocator );
	R_HW::GfxImage* LoadCubeTexture(const char* assetName, const char* assetPath, I_ImageAlloctor* allocator );
	R_HW::GfxImage* CreateSolidColorTexture(const char* assetName, glm::vec4 color, I_ImageAlloctor* allocator );
	GfxModel* CreateQuad( const char* assetName, float size, R_HW::I_BufferAllocator* allocator );
	GfxModel* CreateQuad( const char* assetName, float width, float height, R_HW::I_BufferAllocator* allocator );

	GfxModel* Load3DModel(const char* assetName, const char* assetPath, uint32_t hackIndex, R_HW::I_BufferAllocator* allocator );
	GfxModel* LoadglTf3DModel( const char* assetName, const char* assetPath, R_HW::I_BufferAllocator* allocator );
	GfxModel* RegisterGfxModel( const char* assetName, GfxModel&& model );

	void* GetAsset(const char* assetName);

	void Cleanup();
}