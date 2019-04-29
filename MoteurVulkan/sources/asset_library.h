#pragma once
#include "vk_image.h"
#include "model_asset.h"

#include "glm/vec4.hpp"

GfxImage* AL_LoadTexture(const char* assetName, const char* assetPath);
GfxImage* AL_LoadCubeTexture(const char* assetName, const char* assetPath);
GfxImage* AL_CreateSolidColorTexture(const char* assetName, glm::vec4 color);

ModelAsset* AL_Load3DModel(const char* assetName, const char* assetPath, uint32_t hackIndex);

void* AL_GetAsset(const char* assetName);

void AL_Cleanup();