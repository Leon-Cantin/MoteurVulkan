#include "vk_vertex_input.h"
#include <stdexcept>

uint32_t GetBindingSize( const VIDesc* binding )
{
	return COMPONENT_TYPE_SIZES[( uint8_t )binding->elementType] * binding->elementsCount;
}

static VkFormat GetBindingFormat( const VIDesc* binding )
{
	if( binding->elementType == eVIDataElementType::FLOAT )
	{
		//A bit dangerous, there's 3 elements to reach the next float definition
		return static_cast< VkFormat >(static_cast< uint32_t >(VK_FORMAT_R32_SFLOAT) + (binding->elementsCount - 1) * 3);
	}
	else
	{
		throw std::runtime_error( "Unimplemented" );
	}
}

void GetAPIVIBindingDescription( const VIBinding * bindingsDescs, uint32_t count, VkVertexInputBindingDescription* VIBDescs, VkVertexInputAttributeDescription* VIADescs )
{
	for( uint32_t i = 0; i < count; ++i )
	{
		VIBDescs[i].binding = i;
		VIBDescs[i].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
		VIBDescs[i].stride = GetBindingSize( &bindingsDescs[i].desc );

		VIADescs[i].binding = i;
		VIADescs[i].format = GetBindingFormat( &bindingsDescs[i].desc );
		VIADescs[i].location = bindingsDescs[i].location;
		VIADescs[i].offset = 0;
	}
}