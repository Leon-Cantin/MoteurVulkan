#pragma once

#include "vk_globals.h"
#include "scene_instance.h"
#include "gfx_image.h"
#include "bindless_textures.h"
#include "text_overlay.h"

void CompileScene( BindlessTexturesState* bindlessTexturesState );
void DrawFrame( uint32_t currentFrame, const SceneInstance* cameraSceneInstance, const std::vector<GfxAssetInstance>& drawList, const std::vector<TextZone>& textZones );

void InitRendererImp( VkSurfaceKHR swapchainSurface );
void CleanupRendererImp();