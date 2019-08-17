#pragma once

#include "vk_globals.h"
#include "scene_instance.h"
#include "vk_image.h"

struct LightUniform {
	glm::mat4 shadowMatrix;
	glm::vec3 position;
	float intensity;
};

SceneInstanceSet*  CreateGeometryInstanceDescriptorSet();
void CompileScene( const GfxImage* albedoImage, const GfxImage* normalImage, const GfxImage* skyboxImage );
void DrawFrame( uint32_t currentFrame, const SceneInstance* cameraSceneInstance, LightUniform* light, const std::vector<std::pair<const SceneInstance*, const SceneRenderableAsset*>>& drawList );
void ForceReloadShaders();

void ReloadSceneShaders();

void InitRendererImp();
void CleanupRendererImp();