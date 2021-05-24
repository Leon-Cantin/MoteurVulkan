#pragma once

#include "gfx_model.h"
#include "scene_instance.h"
#include "gfx_asset.h"
#include "gfx_image.h"

namespace glTF_L
{
	typedef GfxModel* ( RegisterGfxModelCallback_t )(const char* name);
	typedef GfxAsset* ( RegisterGfxAssetCallback_t )(const char* name);
	typedef SceneInstance* ( RegisterSceneInstanceCallback_t )(const char* name, GfxAsset* asset);
	typedef uint32_t( LoadTextureCallback_t )( const char* name, I_ImageAlloctor* allocator );

	void LoadScene( const char* fileName, RegisterGfxModelCallback_t registerGfxModelCallback, RegisterGfxAssetCallback_t registerGfxAssetCallback,
		RegisterSceneInstanceCallback_t registerSceneInstanceCallback, LoadTextureCallback_t loadTextureCallback, I_BufferAllocator* allocator, I_ImageAlloctor* imageAllocator );
	void LoadMesh( const char* fileName, GfxModel* model, I_BufferAllocator* allocator );
	void LoadCollisionData( const char* fileName, std::vector<glm::vec3>* vertices, std::vector<uint32_t>* indices );
}