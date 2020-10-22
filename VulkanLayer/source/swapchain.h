#pragma once

#include "vk_globals.h"

struct SwapChainSupportDetails {
	VkSurfaceCapabilitiesKHR capabilities;
	std::vector<GfxSurfaceFormat> formats;
	std::vector<VkPresentModeKHR> present_modes;
};

SwapChainSupportDetails query_swap_chain_support( VkPhysicalDevice device, DisplaySurface surface );

