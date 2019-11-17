#pragma once

#include "vk_globals.h"
#include "scene_instance.h"
#include "gfx_image.h"
#include "bindless_textures.h"

void CompileScene( BindlessTexturesState* bindlessTexturesState );
void DrawFrame( uint32_t currentFrame, const SceneInstance* cameraSceneInstance, const std::vector<std::pair<const SceneInstance*, const RenderableAsset*>>& drawList );

void InitRendererImp( VkSurfaceKHR swapchainSurface );
void CleanupRendererImp();