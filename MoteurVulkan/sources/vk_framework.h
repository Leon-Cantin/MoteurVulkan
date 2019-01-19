#pragma once

#include "vk_globals.h"

#include <vector>
#include <optional>

extern GLFWwindow* g_window;
extern VkSurfaceKHR g_windowSurface;

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

void InitFramework(int windowWidth, int windowHeight);
void ShutdownFramework();

VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pCallback);
void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT callback, const VkAllocationCallbacks* pAllocator);
std::vector<const char*> get_required_extensions();
uint32_t detect_available_extensions(VkExtensionProperties* extensions);
bool check_extensions(const char** required_extensions, uint32_t required_extensions_count);
uint32_t detect_supported_validation_layers(VkLayerProperties * available_layers);
bool check_validation_layers(const char* const* required_layers, uint32_t required_layers_count);
void create_instance();

QueueFamilyIndices find_queue_families(const VkPhysicalDevice device);
bool check_device_extension_support(VkPhysicalDevice device);
SwapChainSupportDetails query_swap_chain_support(VkPhysicalDevice device, VkSurfaceKHR surface);
bool is_device_suitable(const VkPhysicalDevice device);
void pick_physical_device();
void create_logical_device();
void create_surface();
std::vector<char> readFile(const std::string& filename);