#pragma once

#include "vk_globals.h"

bool IsRequiredMemoryType( uint32_t typeFilter, uint32_t memoryType );
uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);


