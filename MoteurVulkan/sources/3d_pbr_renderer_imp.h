#pragma once

#include "vk_globals.h"
#include "scene_instance.h"
#include "gfx_image.h"
#include "bindless_textures.h"

struct LightUniform {
	glm::mat4 shadowMatrix;
	glm::vec3 position;
	float intensity;
};

void CompileScene( BindlessTexturesState* bindlessTexturesState, const GfxImage* skyboxImage );
void DrawFrame( uint32_t currentFrame, const SceneInstance* cameraSceneInstance, LightUniform* light, const std::vector<std::pair<const SceneInstance*, const RenderableAsset*>>& drawList );
void ForceReloadShaders();

void ReloadSceneShaders();

void InitRendererImp( VkSurfaceKHR swapchainSurface );
void CleanupRendererImp();