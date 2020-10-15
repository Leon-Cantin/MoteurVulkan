#pragma once

#include "vk_image.h"

struct Swapchain {
	std::vector<GfxImage> images;
	uint32_t imageCount;
	VkSwapchainKHR vkSwapchain;
	VkSurfaceFormatKHR surfaceFormat;
	VkPresentModeKHR presentMode;
	VkExtent2D extent;
};

void createSwapChain(VkSurfaceKHR vkSurface, uint32_t maxWidth, uint32_t maxHeight, Swapchain& o_swapchain);

struct SwapChainSupportDetails {
	VkSurfaceCapabilitiesKHR capabilities;
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> present_modes;
};

SwapChainSupportDetails query_swap_chain_support( VkPhysicalDevice device, VkSurfaceKHR surface );

