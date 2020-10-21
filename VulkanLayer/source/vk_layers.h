#pragma once
#include "vk_globals.h"
#include <vector>

extern const std::vector<const char*> validationLayers;
bool check_validation_layers( const char* const* required_layers, uint32_t required_layers_count );