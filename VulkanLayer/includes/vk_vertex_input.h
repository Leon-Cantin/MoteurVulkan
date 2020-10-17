#pragma once

#include "vk_globals.h"

//TODO: copied from glTF_loader.cpp
enum class eVIDataElementType : uint8_t
{
	BYTE = 0,
	UNSIGNED_BYTE,
	SHORT,
	UNSIGNED_SHORT,
	UNSIGNED_INT,
	FLOAT,
	ELEMENT_TYPE_COUNT
};
static const uint8_t COMPONENT_TYPE_SIZES[] = { 1, 1, 2, 2, 4, 4 };

struct VIDesc
{
	VIDataType dataType;
	eVIDataElementType elementType;
	uint8_t elementsCount;

	inline bool operator==( const VIDesc& other ) const
	{
		return this->dataType == other.dataType &&
			this->elementsCount == other.elementsCount &&
			this->elementType == other.elementType;
	}
};

struct VIBinding
{
	VIDesc desc;
	uint8_t location;
};

uint32_t GetBindingSize( const VIDesc* binding );
VkFormat GetBindingFormat( const VIDesc* binding );
void GetAPIVIBindingDescription( const VIBinding * bindingsDescs, uint32_t count, VkVertexInputBindingDescription* VIBDescs, VkVertexInputAttributeDescription* VIADescs );
