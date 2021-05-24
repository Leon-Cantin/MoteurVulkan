#include "vk_globals.h"
#include "swapchain.h"
#include "vk_debug.h"
#include "vk_layers.h"

#include <optional>
#include <string>
#include <set>

void DeviceWaitIdle( GfxDevice device )
{
	vkDeviceWaitIdle( device );
}

void Destroy( Device* device )
{
	vkDestroyDevice( device->device, nullptr );
	device->device = VK_NULL_HANDLE;
	device->compute_queue.queue = VK_NULL_HANDLE;
	device->graphics_queue.queue = VK_NULL_HANDLE;
	device->present_queue.queue = VK_NULL_HANDLE;
	device->transfer_queue.queue = VK_NULL_HANDLE;
}

struct QueueFamilyIndices {
	std::optional<uint32_t> graphics_family;
	std::optional<uint32_t> present_family;
	std::optional<uint32_t> compute_family;
	std::optional<uint32_t> transfer_family;

	bool is_complete() {
		return graphics_family.has_value() && present_family.has_value() && compute_family.has_value() && transfer_family.has_value();
	}
};

const std::vector<const char*> required_device_extensions = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME,
};

#define TIMESTAMP_REQUIRED_BITS 64
static QueueFamilyIndices find_queue_families( const VkPhysicalDevice device, DisplaySurface swapchainSurface ) {
	QueueFamilyIndices indices;

	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties( device, &queueFamilyCount, nullptr );

	std::vector<VkQueueFamilyProperties> queueFamilies( queueFamilyCount );
	vkGetPhysicalDeviceQueueFamilyProperties( device, &queueFamilyCount, queueFamilies.data() );

	for( uint32_t i = 0; i < queueFamilyCount; ++i )
	{
		const VkQueueFamilyProperties& queueProperties = queueFamilies[i];
		if( queueProperties.queueCount > 0 )
		{
			if( queueProperties.queueFlags & VK_QUEUE_GRAPHICS_BIT
				&& (queueProperties.timestampValidBits == TIMESTAMP_REQUIRED_BITS)
				&& (!indices.graphics_family.has_value()) )
				indices.graphics_family = i;

			if( !(queueProperties.queueFlags & VK_QUEUE_GRAPHICS_BIT)
				&& (queueProperties.queueFlags & VK_QUEUE_COMPUTE_BIT)
				&& (queueProperties.timestampValidBits == TIMESTAMP_REQUIRED_BITS)
				&& (!indices.compute_family.has_value()) )
				indices.compute_family = i;

			if( queueProperties.queueFlags & VK_QUEUE_TRANSFER_BIT
				&& !(queueProperties.queueFlags & (VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT))
				&& (queueProperties.timestampValidBits == TIMESTAMP_REQUIRED_BITS)
				&& (!indices.transfer_family.has_value()) )
				indices.transfer_family = i;

			VkBool32 present_support = false;
			vkGetPhysicalDeviceSurfaceSupportKHR( device, i, swapchainSurface, &present_support );
			if( present_support && !indices.present_family.has_value() )
				indices.present_family = i;

			if( indices.is_complete() )
				break;
		}
	}

	return indices;
}

static bool check_device_extension_support( VkPhysicalDevice device ) {
	uint32_t extension_count;
	vkEnumerateDeviceExtensionProperties( device, nullptr, &extension_count, nullptr );

	std::vector<VkExtensionProperties> available_extensions( extension_count );
	vkEnumerateDeviceExtensionProperties( device, nullptr, &extension_count, available_extensions.data() );

	std::set<std::string> requiredExtensions( required_device_extensions.begin(), required_device_extensions.end() );

	for( const auto& extension : available_extensions ) {
		requiredExtensions.erase( extension.extensionName );
	}

	return requiredExtensions.empty();
}

static bool is_device_suitable( const VkPhysicalDevice device, DisplaySurface swapchain_surface )
{
	VkPhysicalDeviceProperties deviceProperties;
	vkGetPhysicalDeviceProperties( device, &deviceProperties );

	VkPhysicalDeviceFeatures deviceFeatures;
	vkGetPhysicalDeviceFeatures( device, &deviceFeatures );

	bool suitable = deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;
	suitable |= find_queue_families( device, swapchain_surface ).is_complete();
	suitable |= check_device_extension_support( device );
	suitable |= deviceFeatures.samplerAnisotropy == VK_TRUE;
	suitable |= deviceFeatures.depthClamp == VK_TRUE;
	suitable |= deviceFeatures.shaderSampledImageArrayDynamicIndexing == VK_TRUE;

	if( suitable ) {
		SwapChainSupportDetails swapchain_details = query_swap_chain_support( device, swapchain_surface );
		suitable |= !swapchain_details.formats.empty() && !swapchain_details.present_modes.empty();
	}

	return suitable;
}

PhysicalDevice PickSuitablePhysicalDevice( DisplaySurface swapchainSurface, GpuInstance& instance )
{
	VkInstance vk_instance = instance.instance;
	uint32_t device_count = 0;
	vkEnumeratePhysicalDevices( vk_instance, &device_count, nullptr );
	if( device_count == 0 ) {
		throw std::runtime_error( "failed to find GPUs with Vulkan support!" );
	}

	std::vector<VkPhysicalDevice> devices( device_count );
	vkEnumeratePhysicalDevices( vk_instance, &device_count, devices.data() );

	VkPhysicalDevice vk_physicalDevice = VK_NULL_HANDLE;
	for( const auto& device : devices ) {
		if( is_device_suitable( device, swapchainSurface ) ) {
			vk_physicalDevice = device;
			break;
		}
	}

	if( vk_physicalDevice == VK_NULL_HANDLE ) {
		throw std::runtime_error( "failed to find a suitable GPU!" );
	}

	return vk_physicalDevice;
}

Device create_logical_device( DisplaySurface swapchainSurface, PhysicalDevice physicalDevice, bool enableValidationLayers )
{
	//Queues
	QueueFamilyIndices indices = find_queue_families( physicalDevice, swapchainSurface );

	std::set<uint32_t> unique_queue_families = { indices.graphics_family.value(), indices.present_family.value(), indices.compute_family.value(), indices.transfer_family.value() };
	std::vector<VkDeviceQueueCreateInfo> queue_create_infos;
	queue_create_infos.reserve( unique_queue_families.size() );

	float queue_priority = 1.0f;
	for( uint32_t queue_family_index : unique_queue_families ) {
		VkDeviceQueueCreateInfo queue_create_info = {};
		queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queue_create_info.queueFamilyIndex = queue_family_index;
		queue_create_info.queueCount = 1;
		queue_create_info.pQueuePriorities = &queue_priority;
		queue_create_infos.push_back( queue_create_info );
	}

	//Features
	VkPhysicalDeviceFeatures device_features = {};
	device_features.samplerAnisotropy = VK_TRUE;
	device_features.depthClamp = VK_TRUE;
	device_features.shaderSampledImageArrayDynamicIndexing = VK_TRUE;

	VkDeviceCreateInfo create_info = {};
	create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	create_info.pQueueCreateInfos = queue_create_infos.data();
	create_info.queueCreateInfoCount = static_cast< uint32_t >(queue_create_infos.size());
	create_info.pEnabledFeatures = &device_features;

	//Exensions
	create_info.enabledExtensionCount = static_cast< uint32_t >(required_device_extensions.size());
	create_info.ppEnabledExtensionNames = required_device_extensions.data();

	if( enableValidationLayers ) {
		create_info.enabledLayerCount = static_cast< uint32_t >(validationLayers.size());
		create_info.ppEnabledLayerNames = validationLayers.data();
	}
	else {
		create_info.enabledLayerCount = 0;
	}

	VkDevice vk_device = VK_NULL_HANDLE;
	if( vkCreateDevice( physicalDevice, &create_info, nullptr, &vk_device ) != VK_SUCCESS )
		throw std::runtime_error( "failed to create logical device!" );

	VkQueue graphics_queue = VK_NULL_HANDLE;
	VkQueue present_queue = VK_NULL_HANDLE;
	VkQueue compute_queue = VK_NULL_HANDLE;
	VkQueue transfer_queue = VK_NULL_HANDLE;
	vkGetDeviceQueue( vk_device, indices.graphics_family.value(), 0, &graphics_queue );
	vkGetDeviceQueue( vk_device, indices.present_family.value(), 0, &present_queue );
	vkGetDeviceQueue( vk_device, indices.compute_family.value(), 0, &compute_queue );
	vkGetDeviceQueue( vk_device, indices.transfer_family.value(), 0, &transfer_queue );

	//TODO HACK: Cheatin because of globals in MarkVkObject
	g_gfx.device.device = vk_device;

	MarkVkObject( ( uint64_t )(graphics_queue), VK_OBJECT_TYPE_QUEUE, "graphics_queue" );
	MarkVkObject( ( uint64_t )(present_queue), VK_OBJECT_TYPE_QUEUE, "present_queue" );
	MarkVkObject( ( uint64_t )(compute_queue), VK_OBJECT_TYPE_QUEUE, "compute_queue" );
	MarkVkObject( ( uint64_t )(transfer_queue), VK_OBJECT_TYPE_QUEUE, "transfer_queue" );

	Device device = {};
	device.device = vk_device;

	device.graphics_queue.queue = graphics_queue;
	device.graphics_queue.queueFamilyIndex = indices.graphics_family.value();

	device.present_queue.queue = present_queue;
	device.present_queue.queueFamilyIndex = indices.present_family.value();

	device.compute_queue.queue = compute_queue;
	device.compute_queue.queueFamilyIndex = indices.compute_family.value();

	device.transfer_queue.queue = transfer_queue;
	device.transfer_queue.queueFamilyIndex = indices.transfer_family.value();

	return device;
}