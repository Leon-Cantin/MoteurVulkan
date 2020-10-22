#pragma once

#include "vk_globals.h"

void MarkVkObject(uint64_t object, VkObjectType objetType, const char * name);

void SetupDebugCallback();
void DestroyDebugCallback();