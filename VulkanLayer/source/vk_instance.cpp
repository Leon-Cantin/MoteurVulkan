#include "vk_debug.h"

#include <iostream>
#include <algorithm>

#include "vk_loader.h"
#include "vk_layers.h"


const bool enableVkObjectMarking = true;

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

void Destroy( GpuInstance* gpuInstance )
{
	if( gpuInstance->validationLayerEnabled )
		DestroyDebugCallback();

	vkDestroyInstance( gpuInstance->instance, nullptr );
	gpuInstance->instance = VK_NULL_HANDLE;
}