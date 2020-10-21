#include "vk_layers.h"

#include <iostream>

const std::vector<const char*> validationLayers = {
	"VK_LAYER_KHRONOS_validation"
};

static uint32_t detect_supported_validation_layers( VkLayerProperties * available_layers ) {
	uint32_t layerCount = 0;
	vkEnumerateInstanceLayerProperties( &layerCount, nullptr );
	vkEnumerateInstanceLayerProperties( &layerCount, available_layers );

	std::cout << "Available validation layers: " << std::endl;
	for( uint32_t i = 0; i < layerCount; ++i )
		std::cout << "\t" << available_layers[i].layerName << std::endl;

	return layerCount;
}

bool check_validation_layers( const char* const* required_layers, uint32_t required_layers_count )
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