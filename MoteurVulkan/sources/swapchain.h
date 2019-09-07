#pragma once

#include "gfx_image.h"

struct Swapchain {
	std::vector<GfxImage> images;
	uint32_t imageCount;
	VkSwapchainKHR vkSwapchain;
	VkSurfaceFormatKHR surfaceFormat;
	VkPresentModeKHR presentMode;
	VkExtent2D extent;
};

void createSwapChain(VkSurfaceKHR vkSurface, uint32_t maxWidth, uint32_t maxHeight, Swapchain& o_swapchain);

