#pragma once

#include "vk_image.h"

struct Swapchain {
	std::vector<GfxImage> images;
	uint32_t imageCount;
	GfxSwapchain swapchain;
	GfxSurfaceFormat surfaceFormat;
	VkPresentModeKHR presentMode;
	VkExtent2D extent;
};

void CreateSwapChain(VkSurfaceKHR vkSurface, uint32_t maxWidth, uint32_t maxHeight, Swapchain& o_swapchain);
void Destroy( Swapchain* Swapchain );

struct SwapChainSupportDetails {
	VkSurfaceCapabilitiesKHR capabilities;
	std::vector<GfxSurfaceFormat> formats;
	std::vector<VkPresentModeKHR> present_modes;
};

SwapChainSupportDetails query_swap_chain_support( VkPhysicalDevice device, VkSurfaceKHR surface );

