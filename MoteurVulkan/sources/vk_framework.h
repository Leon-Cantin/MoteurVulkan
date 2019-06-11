#pragma once

#include "vk_globals.h"

#include <vector>
#include <optional>

struct QueueFamilyIndices {
	std::optional<uint32_t> graphics_family;
	std::optional<uint32_t> present_family;
	std::optional<uint32_t> compute_family;
	std::optional<uint32_t> transfer_family;

	bool is_complete() {
		return graphics_family.has_value() && present_family.has_value() && compute_family.has_value() && transfer_family.has_value();
	}
};

struct SwapChainSupportDetails {
	VkSurfaceCapabilitiesKHR capabilities;
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> present_modes;
};

void InitFramework(int windowWidth, int windowHeight, const char * windowName);
void ShutdownFramework();

//TODO try to get rid of this
QueueFamilyIndices find_queue_families(const VkPhysicalDevice device);
SwapChainSupportDetails query_swap_chain_support(VkPhysicalDevice device, VkSurfaceKHR surface);