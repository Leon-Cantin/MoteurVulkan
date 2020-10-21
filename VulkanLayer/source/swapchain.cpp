#include "swapchain.h"

#include <limits>
#include <algorithm>
#include <stdexcept>

VkSurfaceFormatKHR choose_swap_surface_format(const std::vector<VkSurfaceFormatKHR>& available_formats)
{
	//VK_FORMAT_UNDEFINED means it has no preffered format
	if (available_formats.size() == 1 && available_formats[0].format == VK_FORMAT_UNDEFINED) {
		return { VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
	}

	for (const auto& available_format : available_formats) {
		if (available_format.format == VK_FORMAT_B8G8R8A8_UNORM && available_format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
			return available_format;
		}
	}

	throw std::runtime_error("Fuck this, this should be enougth for now.");
}

VkPresentModeKHR choose_swap_present_mode(const std::vector<VkPresentModeKHR> available_present_modes) {
	for (const auto& available_present_mode : available_present_modes) {
		if (available_present_mode == VK_PRESENT_MODE_MAILBOX_KHR) {
			return available_present_mode;
		}
	}

	//Garanteed to be available
	return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D choose_swap_extent(const VkSurfaceCapabilitiesKHR& capabilities, uint32_t maxWidth, uint32_t maxHeight) {
	//Return the only supported format or clamp our window's size if we are allowed something else
	if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
		return capabilities.currentExtent;
	else {
		VkExtent2D actual_extent = { maxWidth, maxHeight };
		actual_extent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, actual_extent.width));
		actual_extent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, actual_extent.height));
		return actual_extent;
	}
}

void CreateSwapChain( DisplaySurface vkSurface, uint32_t maxWidth, uint32_t maxHeight, Swapchain& o_swapchain)
{
	SwapChainSupportDetails swapChainSupport = query_swap_chain_support(g_gfx.physicalDevice, vkSurface);

	VkSurfaceFormatKHR surfaceFormat = choose_swap_surface_format(swapChainSupport.formats);
	VkPresentModeKHR presentMode = choose_swap_present_mode(swapChainSupport.present_modes);
	VkExtent2D extent = choose_swap_extent(swapChainSupport.capabilities, maxWidth, maxHeight);

	uint32_t image_count = swapChainSupport.capabilities.minImageCount + 1;
	if (swapChainSupport.capabilities.maxImageCount > 0 && image_count > swapChainSupport.capabilities.maxImageCount)
		image_count = swapChainSupport.capabilities.maxImageCount;

	VkSwapchainCreateInfoKHR create_info = {};
	create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	create_info.surface = vkSurface;
	create_info.minImageCount = image_count;
	create_info.imageFormat = surfaceFormat.format;
	create_info.imageColorSpace = surfaceFormat.colorSpace;
	create_info.imageExtent = extent;
	create_info.imageArrayLayers = 1;
	create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_STORAGE_BIT;

	//Add queue info
	uint32_t queueFamilyIndices[] = { g_gfx.device.graphics_queue.queueFamilyIndex, g_gfx.device.present_queue.queueFamilyIndex };
	if ( g_gfx.device.graphics_queue.queueFamilyIndex != g_gfx.device.present_queue.queueFamilyIndex ) {
		create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		create_info.queueFamilyIndexCount = 2;
		create_info.pQueueFamilyIndices = queueFamilyIndices;
	}
	else {
		create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		create_info.queueFamilyIndexCount = 0; // Optional
		create_info.pQueueFamilyIndices = nullptr; // Optional
	}

	create_info.preTransform = swapChainSupport.capabilities.currentTransform;
	create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	create_info.presentMode = presentMode;
	create_info.clipped = VK_TRUE;
	create_info.oldSwapchain = VK_NULL_HANDLE;

	if (vkCreateSwapchainKHR(g_gfx.device.device, &create_info, nullptr, &o_swapchain.swapchain) != VK_SUCCESS)
		throw std::runtime_error("failed to create swap chain!");

	//Query swap chain images
	vkGetSwapchainImagesKHR( g_gfx.device.device, o_swapchain.swapchain, &image_count, nullptr);
	std::vector<VkImage> swapChainImages(image_count);
	vkGetSwapchainImagesKHR( g_gfx.device.device, o_swapchain.swapchain, &image_count, swapChainImages.data());

	VkFormat swapchainFormat = surfaceFormat.format;

	o_swapchain.images.resize(image_count);
	o_swapchain.imageCount = image_count;
	o_swapchain.extent = extent;
	o_swapchain.presentMode = presentMode;
	o_swapchain.surfaceFormat = surfaceFormat;
	for( size_t i = 0; i < image_count; ++i )
	{
		const GfxMemAlloc memAlloc = {};
		o_swapchain.images[i] = { swapChainImages[i], VK_NULL_HANDLE, ToGfxFormat( surfaceFormat.format ), extent, 1, 1, memAlloc };
		o_swapchain.images[i].imageView = CreateImageView( o_swapchain.images[i] );
	}
}

void Destroy( Swapchain* swapchain )
{
	for( auto image : swapchain->images )
		Destroy( &image.imageView );

	vkDestroySwapchainKHR( g_gfx.device.device, swapchain->swapchain, nullptr );
	swapchain->swapchain = VK_NULL_HANDLE;
}

SwapChainSupportDetails query_swap_chain_support( VkPhysicalDevice device, DisplaySurface surface ) {
	SwapChainSupportDetails details;

	//Capabilities
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR( device, surface, &details.capabilities );

	//Formats
	uint32_t format_count;
	vkGetPhysicalDeviceSurfaceFormatsKHR( device, surface, &format_count, nullptr );

	if( format_count != 0 ) {
		details.formats.resize( format_count );
		vkGetPhysicalDeviceSurfaceFormatsKHR( device, surface, &format_count, details.formats.data() );
	}

	//Present modes
	uint32_t present_mode_count;
	vkGetPhysicalDeviceSurfacePresentModesKHR( device, surface, &present_mode_count, nullptr );

	if( present_mode_count != 0 ) {
		details.present_modes.resize( present_mode_count );
		vkGetPhysicalDeviceSurfacePresentModesKHR( device, surface, &present_mode_count, details.present_modes.data() );
	}

	return details;
}