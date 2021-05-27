#pragma once

#include "vk_globals.h"
#include "gfx_image.h"
#include "bindless_textures.h"
#include "gfx_instance.h"

struct LightUniform {
	glm::mat4 shadowMatrix;
	glm::vec3 position;
	float intensity;
};

void CompileScene( BindlessTexturesState* bindlessTexturesState, const GfxImage* skyboxImage );
void DrawFrame( uint32_t currentFrame, const SceneInstance* cameraSceneInstance, LightUniform* light, const std::vector<GfxAssetInstance>& drawList );

void InitRendererImp( const DisplaySurface* swapchainSurface );
void CleanupRendererImp();

void SetBtDebugDraw( bool value );