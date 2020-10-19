#include "vk_framework.h"
#include "vk_debug.h"
#include "swapchain.h"

#include <iostream>
#include <algorithm>
#include <set>

#include "vk_loader.h"

namespace VK
{
	const bool enableVkObjectMarking = true;

	const std::vector<const char*> validationLayers = {
		"VK_LAYER_KHRONOS_validation"
	};

	const std::vector<const char*> required_device_extensions = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME,
	};

	static std::vector<const char*> get_required_extensions( bool enableValidationLayers )
	{
		std::vector<const char*> extensions;

		if( enableValidationLayers || enableVkObjectMarking )
			extensions.push_back( VK_EXT_DEBUG_UTILS_EXTENSION_NAME );
#ifdef _WIN32
		extensions.push_back( VK_KHR_WIN32_SURFACE_EXTENSION_NAME );
#elif __linux__
		extensions.push_back( VK_KHR_XCB_SURFACE_EXTENSION_NAME );
#endif // #ifdef WINDOWS_PROGRAM
		extensions.push_back( VK_KHR_SURFACE_EXTENSION_NAME );

		std::cout << "required extensions :" << std::endl;
		for( uint32_t i = 0; i < extensions.size(); ++i )
			std::cout << "\t" << extensions[i] << std::endl;

		return extensions;
	}

	static uint32_t detect_available_extensions( VkExtensionProperties* extensions )
	{
		uint32_t extension_count = 0;
		vkEnumerateInstanceExtensionProperties( nullptr, &extension_count, nullptr );
		vkEnumerateInstanceExtensionProperties( nullptr, &extension_count, extensions );

		std::cout << "Available extension: " << std::endl;
		for( uint32_t i = 0; i < extension_count; ++i )
			std::cout << "\t" << extensions[i].extensionName << std::endl;

		return extension_count;
	}

	static bool check_extensions( const char** required_extensions, uint32_t required_extensions_count )
	{
		uint32_t extension_count = 0;

		//Get available extensions
		vkEnumerateInstanceExtensionProperties( nullptr, &extension_count, nullptr );
		std::vector<VkExtensionProperties> detected_extensions( extension_count );
		detect_available_extensions( detected_extensions.data() );

		//Check if all required are available
		std::vector<bool> missing( required_extensions_count );
		bool any_missing = false;
		for( uint32_t i = 0; i < required_extensions_count; ++i )
		{
			bool found = false;
			for( VkExtensionProperties& extension : detected_extensions )
			{
				if( strcmp( required_extensions[i], extension.extensionName ) == 0 )
				{
					found = true;
					break;
				}
			}
			if( !found )
			{
				missing[i] = true;
				any_missing = true;
			}
		}

		//Print output
		if( any_missing )
		{
			std::cout << "Error Missing extensions:" << std::endl;
			for( uint32_t i = 0; i < required_extensions_count; ++i )
				if( missing[i] )
					std::cout << "\t" << required_extensions[i] << std::endl;
		}
		else
			std::cout << "Extensions check OK" << std::endl;

		return !any_missing;
	}

	static uint32_t detect_supported_validation_layers( VkLayerProperties * available_layers ) {
		uint32_t layerCount = 0;
		vkEnumerateInstanceLayerProperties( &layerCount, nullptr );
		vkEnumerateInstanceLayerProperties( &layerCount, available_layers );

		std::cout << "Available validation layers: " << std::endl;
		for( uint32_t i = 0; i < layerCount; ++i )
			std::cout << "\t" << available_layers[i].layerName << std::endl;

		return layerCount;
	}

	static bool check_validation_layers( const char* const* required_layers, uint32_t required_layers_count )
	{
		std::cout << "Required validation layers :" << std::endl;
		for( uint32_t i = 0; i < required_layers_count; ++i )
			std::cout << "\t" << required_layers[i] << std::endl;

		uint32_t layers_count = 0;

		//Get available extensions
		vkEnumerateInstanceLayerProperties( &layers_count, nullptr );
		std::vector<VkLayerProperties> detected_layers( layers_count );
		detect_supported_validation_layers( detected_layers.data() );

		//Check if all required are available
		std::vector<bool> missing( layers_count );
		bool any_missing = false;
		for( uint32_t i = 0; i < required_layers_count; ++i )
		{
			bool found = false;
			for( VkLayerProperties& layer : detected_layers )
			{
				if( strcmp( required_layers[i], layer.layerName ) == 0 )
				{
					found = true;
					break;
				}
			}
			if( !found )
			{
				missing[i] = true;
				any_missing = true;
			}
		}

		//Print output
		if( any_missing )
		{
			std::cout << "Error missing validation layers:" << std::endl;
			for( uint32_t i = 0; i < required_layers_count; ++i )
				if( missing[i] )
					std::cout << "\t" << required_layers[i] << std::endl;
		}
		else
			std::cout << "Validation layers check OK" << std::endl;

		return !any_missing;
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

	struct QueueFamilyIndices {
		std::optional<uint32_t> graphics_family;
		std::optional<uint32_t> present_family;
		std::optional<uint32_t> compute_family;
		std::optional<uint32_t> transfer_family;

		bool is_complete() {
			return graphics_family.has_value() && present_family.has_value() && compute_family.has_value() && transfer_family.has_value();
		}
	};

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
					&& (queueProperties.timestampValidBits > 0)
					&& (!indices.graphics_family.has_value()) )
					indices.graphics_family = i;

				if( !(queueProperties.queueFlags & VK_QUEUE_GRAPHICS_BIT)
					&& (queueProperties.queueFlags & VK_QUEUE_COMPUTE_BIT)
					&& (!indices.compute_family.has_value()) )
					indices.compute_family = i;

				if( queueProperties.queueFlags & VK_QUEUE_TRANSFER_BIT
					&& !(queueProperties.queueFlags & (VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT))
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

	GpuInstance CreateInstance( bool enableValidationLayer )
	{
		VkApplicationInfo app_info = {};
		app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		app_info.pApplicationName = "Hello triangle";
		app_info.applicationVersion = VK_MAKE_VERSION( 1, 0, 0 );
		app_info.pEngineName = "No Engine";
		app_info.engineVersion = VK_MAKE_VERSION( 1, 0, 0 );
		app_info.apiVersion = VK_API_VERSION_1_1;

		VkInstanceCreateInfo create_info = {};
		create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		create_info.pApplicationInfo = &app_info;

		//Check extensions
		std::vector<const char*> required_extensions = get_required_extensions( enableValidationLayer );
		check_extensions( required_extensions.data(), static_cast< uint32_t >(required_extensions.size()) );

		create_info.enabledExtensionCount = static_cast< uint32_t >(required_extensions.size());
		create_info.ppEnabledExtensionNames = required_extensions.data();

		//Check validation layers
		if( enableValidationLayer )
		{
			uint32_t validation_layers_count = static_cast< uint32_t >(validationLayers.size());
			check_validation_layers( validationLayers.data(), validation_layers_count );

			create_info.ppEnabledLayerNames = validationLayers.data();
			create_info.enabledLayerCount = validation_layers_count;
		}
		else
		{
			create_info.enabledLayerCount = 0;
		}

		GpuInstance instance;
		instance.validationLayerEnabled = enableValidationLayer;

		if( vkCreateInstance( &create_info, nullptr, &instance.instance ) != VK_SUCCESS )
			throw std::runtime_error( "failed to create instance" );

		//TODO HACK: Cheatin because of globals in SetupDebugCallBack
		g_gfx.instance.instance = instance.instance;

		if( enableValidationLayer )
			SetupDebugCallback();

		return instance;
	}

	void Destroy( Device* device )
	{
		vkDestroyDevice( device->device, nullptr );
	}

	void Destroy( GpuInstance* gpuInstance )
	{
		if( gpuInstance->validationLayerEnabled )
			DestroyDebugCallback();

		vkDestroyInstance( gpuInstance->instance, nullptr );
	}
}